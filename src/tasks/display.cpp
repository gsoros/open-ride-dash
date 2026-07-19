#include "tasks/display.h"

#include "model/state.h"
extern State state;

#include "tasks/wifi.h"
extern Wifi wifi;

std::unique_ptr<DisplayDriver> makeDisplayDriver() {
#if ORD_DISPLAY == st7789_240x240
    return std::make_unique<ST7789_240x240>(
        PIN_TFT_CS,
        PIN_TFT_DC,
        PIN_SPI_MOSI,
        PIN_SPI_SCK,
        PIN_TFT_RST,
        PIN_TFT_BL,
        SPI2_HOST,
        TFT_ROTATION);
#elif ORD_DISPLAY == ili9341_240x320
    return std::make_unique<ILI9341_240x320>(
        PIN_TFT_CS,
        PIN_TFT_DC,
        PIN_SPI_MOSI,
        PIN_SPI_SCK,
        PIN_TFT_RST,
        PIN_TFT_BL,
        SPI2_HOST,
        TFT_ROTATION);
#else
#error "Unsupported display. Please define ORD_DISPLAY in platformio.ini"
#endif
}

const char* Display::taskName() const {
    return "Display";
}

void Display::setup() {
    uiEventQueue = xQueueCreate(UI_EVENT_QUEUE_LENGTH, sizeof(UiEvent));
    if (uiEventQueue == nullptr) {
        ESP_LOGE(taskName(), "Failed to create UI event queue");
    }
    output->setup();
    loadPreferences();
    output->setBrightnessPercent(brightnessPercent);
    output->splash();
    // Initialize menu label based on saved brightness
    menu.onBrightnessChange(savedBrightnessPercent == brightnessPercent);
    apiClientSetup(taskName());
    api.registerCommand(
        "next",
        [this](const char* args) {
            return nextPageCommand(args);
        },
        "Usage: next\nSwitches to the next page.");
    // Update WiFi and BLE menu items based on current state,
    // in case those setup() calls ran before the menu was created.
    menu.onWifiChange();
    menu.onBleChange();
}

void Display::taskRun() {
    processUiEvents();

    // Resolve the desired screen from logical inputs (priority order):
    // 1. Menu  2. BLE Passkey  3. AP SSID  4. Metrics Page
    Screen desired = resolveScreen();
    if (desired != currentScreen) {
        exitScreen(currentScreen);
        currentScreen = desired;
        screenEntered = false;
    }

    if (!screenEntered) {
        screenEntered = enterScreen(currentScreen);
        if (!screenEntered) {
            // enter can fail (e.g. showPasskey/showApSsid returned false); retry next frame.
            return;
        }
    }

    runScreen(currentScreen);
}

bool Display::queueUiEvent(UiEvent event) {
    if (uiEventQueue == nullptr) {
        ESP_LOGE(taskName(), "UI event queue is null");
        return false;
    }
    BaseType_t res = xQueueSend(uiEventQueue, &event, 0);
    if (res != pdTRUE) {
        ESP_LOGW(taskName(), "UI event queue full, dropping event %u", (uint8_t)event);
        return false;
    }
    return true;
}

bool Display::increaseBrightness() {
    static constexpr uint8_t step = 2;
    brightnessPercent += brightnessPercent <= 100 - step ? step : 1;
    if (brightnessPercent > 100) {
        brightnessPercent = 100;
        return false;
    }
    output->setBrightnessPercent(brightnessPercent);
    return true;
}

bool Display::decreaseBrightness() {
    static constexpr uint8_t step = 2;
    brightnessPercent -= brightnessPercent >= step ? step : 1;
    if (brightnessPercent < 2) {
        brightnessPercent = 1;
        return false;
    }
    output->setBrightnessPercent(brightnessPercent);
    return true;
}

bool Display::saveBrightness() {
    if (!preferencesReady) {
        ESP_LOGW(taskName(), "Preferences not ready, cannot save brightness");
        return false;
    }
    if (preferences.putUChar(brightnessPrefKey, brightnessPercent)) {
        savedBrightnessPercent = brightnessPercent;
        menu.onBrightnessChange(true);
        ESP_LOGD(taskName(), "Saved brightness: %d%%", brightnessPercent);
        return true;
    }
    ESP_LOGE(taskName(), "Failed to save brightness");
    return false;
}

Api::Reply Display::nextPageCommand(const char* args) {
    ESP_LOGI(taskName(), "Next page");
    // Route through the UI event queue so the page transition (which drives the
    // shared SPI bus) only ever runs inside the Display task. Invoking
    // output->nextPage() synchronously from the API task would race with the
    // Display task's per-frame output->update() and corrupt the SPI/DMA state.
    if (!queueUiEvent(UiEvent::NextPage)) {
        Api::Reply reply = {};
        reply.code = Api::Reply::Code::ExecutionError;
        snprintf((char*)reply.data, sizeof(reply.data), "UI event queue full");
        return reply;
    }
    Api::Reply reply = {};
    reply.code = Api::Reply::Code::Success;
    snprintf((char*)reply.data, sizeof(reply.data), "Page %d", output->currentPage());
    return reply;
}

void Display::receiveReply(const Api::Reply& reply) {
    ESP_LOGD(taskName(), "Received API reply: %s: %s", reply.command, Api::Reply::codeToString(reply.code));
}

bool Display::loadPreferences() {
    if (preferencesSetup("display", false)) {
        savedBrightnessPercent = preferences.getUChar(brightnessPrefKey, savedBrightnessPercent);
        brightnessPercent = savedBrightnessPercent;
        ESP_LOGD(taskName(), "Loaded brightness from preferences: %d%%", brightnessPercent);
        return true;
    }
    ESP_LOGW(taskName(), "Failed to setup preferences for display");
    return false;
}

void Display::processUiEvents() {
    if (uiEventQueue == nullptr) return;

    UiEvent event;
    while (xQueueReceive(uiEventQueue, &event, 0) == pdTRUE) {
        handleUiEvent(event);
    }
}

void Display::handleUiEvent(UiEvent event) {
    switch (event) {
        case UiEvent::UpClick:
            if (menu.previousItem()) return;
            adjustPasLevel(1);
            return;
        case UiEvent::DownClick:
            if (menu.nextItem()) return;
            adjustPasLevel(-1);
            return;
        case UiEvent::PowerClick:
            if (menu.active()) {
                menu.selectItem();
                return;
            }
            output->nextPage();
            return;
        case UiEvent::UpLong:
            handleUpLongPress();
            return;
        case UiEvent::DownLong:
            handleDownLongPress();
            return;
        case UiEvent::PowerLong:
            // ESP_LOGD(taskName(), "Key power long press");
            return;
        case UiEvent::MenuChord:
            if (menu.active())
                menu.exit();
            else
                menu.enter();
            return;
        case UiEvent::Sleep:
            ESP_LOGD(taskName(), "Sleep");
            output->onSleep();
            return;
        case UiEvent::Restart:
            ESP_LOGD(taskName(), "Restart");
            output->onRestart();
            return;
        case UiEvent::PasskeyStart:
            ESP_LOGD(taskName(), "PasskeyStart: %06u", state.blePassKey());
            passkeyActive = true;
            return;
        case UiEvent::PasskeyEnd:
            ESP_LOGD(taskName(), "PasskeyEnd");
            passkeyActive = false;
            return;
        case UiEvent::OtaChange:
            ESP_LOGD(taskName(), "OtaChange: %d", state.ota());
            return;
        case UiEvent::WifiChange:
            ESP_LOGD(taskName(), "WifiChange");
            apActive = wifi.isApEnabled();
            menu.onWifiChange();
            output->onWifiChange();
            return;
        case UiEvent::BleChange:
            ESP_LOGD(taskName(), "BleChange");
            menu.onBleChange();
            output->onBleChange();
            return;
        case UiEvent::NextPage:
            ESP_LOGD(taskName(), "NextPage");
            output->nextPage();
            return;
        default:
            ESP_LOGW(taskName(), "Unhandled UI event: %u", (uint8_t)event);
            return;
    }
}

Display::Screen Display::resolveScreen() const {
    if (menu.active()) return Screen::Menu;
    if (passkeyActive) return Screen::Passkey;
    if (apActive) return Screen::ApSsid;
    return Screen::Metrics;
}

bool Display::enterScreen(Screen s) {
    switch (s) {
        case Screen::Menu:
            // Menu is redrawn every frame, so enter always succeeds.
            return true;
        case Screen::Passkey:
            return output->showPasskey(state.blePassKey());
        case Screen::ApSsid:
            return output->showApSsid(wifi.getApSsid());
        case Screen::Metrics:
            return true;
    }
    return true;
}

void Display::exitScreen(Screen s) {
    switch (s) {
        case Screen::Menu:
            output->exitMenu();
            menu.markRendered();
            break;
        case Screen::Passkey:
            output->exitPasskey();
            break;
        case Screen::ApSsid:
            output->exitApSsid();
            break;
        case Screen::Metrics:
            break;
    }
}

void Display::runScreen(Screen s) {
    switch (s) {
        case Screen::Menu:
            // Redraw every frame so selection/dirty changes are reflected.
            if (output->showMenu(menu.snapshot())) {
                menu.markRendered();
            }
            break;
        case Screen::Passkey:
        case Screen::ApSsid:
            // Static screens: drawn once on enter, nothing per-frame.
            break;
        case Screen::Metrics:
            output->update();
            break;
    }
}

void Display::adjustPasLevel(int8_t delta) {
    if (delta != -1 && delta != 1) {
        ESP_LOGW(taskName(), "Invalid delta: %d", delta);
        return;
    }
    State::Snapshot s = state.getSnapshot();
    int8_t next = s.pasLevelRequested + delta;
    if (next < -1 || next > 5) return;
    if (!s.controllerAlive) {
        ESP_LOGW(taskName(), "Controller not alive, ignoring PAS level adjustment");
        return;
    }
    state.pasLevelRequested(next);
}

void Display::handleUpLongPress() {
    if (lastBrightnessChange + brightnessChangeDelay > millis()) return;
    if (shouldLogLongPress())
        ESP_LOGD(taskName(), "Key up long press (increase brightness)");
    if (!increaseBrightness()) return;
    lastBrightnessChange = millis();
    menu.onBrightnessChange(brightnessPercent == savedBrightnessPercent);
}

void Display::handleDownLongPress() {
    if (lastBrightnessChange + brightnessChangeDelay > millis()) return;
    bool isWalkAssist = state.pasLevel() == State::PAS_WALK_ASSIST;
    if (shouldLogLongPress())
        ESP_LOGW(taskName(), "Key down long press (%s)",
                 isWalkAssist ? "walk assist" : "decrease brightness");
    if (isWalkAssist) return;
    if (!decreaseBrightness()) return;
    lastBrightnessChange = millis();
    menu.onBrightnessChange(brightnessPercent == savedBrightnessPercent);
}

bool Display::shouldLogLongPress() {
    static uint32_t lastLongPressLog = 0;
    uint32_t now = millis();
    if (now - lastLongPressLog <= 500) return false;
    lastLongPressLog = now;
    return true;
}