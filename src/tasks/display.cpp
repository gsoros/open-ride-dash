#include "tasks/display.h"

#include "model/state.h"
extern State state;

#include "tasks/wifi.h"
extern Wifi wifi;

const char* Display::taskName() const {
    return "Display";
}

void Display::setup() {
    uiEventQueue = xQueueCreate(UI_EVENT_QUEUE_LENGTH, sizeof(UiEvent));
    if (uiEventQueue == nullptr) {
        ESP_LOGE(taskName(), "Failed to create UI event queue");
    }
    output.setup();
    loadPreferences();
    output.setBrightnessPercent(brightnessPercent);
    output.splash();
    // Initialize menu label based on saved brightness
    menu.onBrightnessChange(savedBrightnessPercent == brightnessPercent);
    apiClientSetup(taskName());
    api.registerCommand(
        "nextpage",
        [this](const char* args) {
            return nextPageCommand(args);
        },
        "Usage: nextpage\nSwitches to the next page.");
}

void Display::taskRun() {
    processUiEvents();

    // Present the current screen in order of priority:
    // 1. Menu
    // 2. BLE Passkey
    // 3. AP SSID
    // 4. Metrics Page

    static enum DebugLogType { DBGL_NORMAL,
                               DBGL_MENU,
                               DBGL_PASSKEY,
                               DBGL_AP } lastLogType = DBGL_NORMAL;
    if (syncMenuDisplay()) {
        if (lastLogType != DBGL_MENU) {
            ESP_LOGD(taskName(), "Menu mode active, skipping updates");
            lastLogType = DBGL_MENU;
        }
        return;
    }
    if (syncPasskeyDisplay()) {
        if (lastLogType != DBGL_PASSKEY) {
            ESP_LOGD(taskName(), "Passkey mode active, skipping updates");
            lastLogType = DBGL_PASSKEY;
        }
        return;
    }
    if (syncApDisplay()) {
        if (lastLogType != DBGL_AP) {
            ESP_LOGD(taskName(), "AP mode active, skipping updates");
            lastLogType = DBGL_AP;
        }
        return;
    }

    if (lastLogType != DBGL_NORMAL) {
        ESP_LOGD(taskName(), "No active mode, resuming normal updates");
        lastLogType = DBGL_NORMAL;
    }
    output.update();
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
    output.setBrightnessPercent(brightnessPercent);
    return true;
}

bool Display::decreaseBrightness() {
    static constexpr uint8_t step = 2;
    brightnessPercent -= brightnessPercent >= step ? step : 1;
    if (brightnessPercent < 2) {
        brightnessPercent = 1;
        return false;
    }
    output.setBrightnessPercent(brightnessPercent);
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
    ESP_LOGI(taskName(), "Switching to next page");
    output.nextPage();
    Api::Reply reply = {};
    reply.code = Api::Reply::Code::Success;
    snprintf((char*)reply.data, sizeof(reply.data), "Page %d", output.currentPage());
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
            output.nextPage();
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
            output.onSleep();
            return;
        case UiEvent::Restart:
            ESP_LOGD(taskName(), "Restart");
            output.onRestart();
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
            output.onWifiChange();
            return;
        default:
            ESP_LOGW(taskName(), "Unhandled UI event: %u", (uint8_t)event);
            return;
    }
}

bool Display::syncPasskeyDisplay() {
    if (passkeyActive) {
        if (!passkeyShown && output.showPasskey(state.blePassKey())) {
            passkeyShown = true;
            menuShown = false;
            apSsidShown = false;
        }
        return true;
    }
    if (passkeyShown) {
        output.exitPasskey();
        passkeyShown = false;
    }
    return false;
}

bool Display::syncApDisplay() {
    if (passkeyActive) return false;  // don't show AP while a passkey is active
    if (apActive) {
        const char* apSsid = wifi.getApSsid();
        if (!apSsidShown && output.showApSsid(apSsid)) {
            apSsidShown = true;
            menuShown = false;
            passkeyShown = false;
        }
        return true;
    }
    if (apSsidShown) {
        output.exitApSsid();
        apSsidShown = false;
    }
    return false;
}

bool Display::syncMenuDisplay() {
    if (menu.active()) {
        Menu::Snapshot snapshot = menu.snapshot();
        if (output.showMenu(snapshot)) {
            menu.markRendered();
        }
        menuShown = true;
        passkeyShown = false;
        apSsidShown = false;
        return true;
    }

    if (!menuShown) return false;
    output.exitMenu();
    menu.markRendered();
    menuShown = false;
    return false;
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