#include "tasks/keypad.h"

#include "tasks/display.h"
#include "tasks/api.h"

extern Display display;
extern Api api;

const char* Keypad::taskName() const {
    return "Keypad";
}

void Keypad::setup() {
    keyUp.setup(PIN_KEY_UP, INPUT_PULLUP, true);        // Active LOW
    keyDown.setup(PIN_KEY_DOWN, INPUT_PULLUP, true);    // Active LOW
    keyPower.setup(PIN_KEY_POWER, INPUT_PULLUP, true);  // Active LOW

    keyUp.attachClick([](void* s) { ((Keypad*)s)->keyUpClick(); }, this);
    keyDown.attachClick([](void* s) { ((Keypad*)s)->keyDownClick(); }, this);
    keyPower.attachClick([](void* s) { ((Keypad*)s)->keyPowerClick(); }, this);

    keyUp.attachDuringLongPress([](void* s) { ((Keypad*)s)->keyUpLongPress(); }, this);
    keyDown.attachDuringLongPress([](void* s) { ((Keypad*)s)->keyDownLongPress(); }, this);
    keyPower.attachDuringLongPress([](void* s) { ((Keypad*)s)->keyPowerLongPress(); }, this);

    api.registerCommand(
        "key",
        [this](const char* args) {
            return keyCommand(args);
        },
        "Usage: key up|down|upLong|downLong|power|menu\nSimulate a key press.");
}

void Keypad::taskRun() {
    // uint32_t t = millis();

    keyUp.tick();
    keyDown.tick();
    keyPower.tick();

    if (!menuChordInProgress()) {
        menuChordHandled = false;
    }
}

void Keypad::keyUpClick() {
    ESP_LOGD(taskName(), "Key up click");
    display.queueUiEvent(UiEvent::UpClick);
}

void Keypad::keyDownClick() {
    ESP_LOGD(taskName(), "Key down click");
    display.queueUiEvent(UiEvent::DownClick);
}

void Keypad::keyPowerClick() {
    ESP_LOGD(taskName(), "Key power click");
    display.queueUiEvent(UiEvent::PowerClick);
}

void Keypad::keyUpLongPress() {
    if (handleMenuChord()) return;
    display.queueUiEvent(UiEvent::UpLong);
}

void Keypad::keyDownLongPress() {
    if (handleMenuChord()) return;
    display.queueUiEvent(UiEvent::DownLong);
}

void Keypad::keyPowerLongPress() {
    if (lastLongpressLog + longpressLogInterval < millis()) {
        ESP_LOGD(taskName(), "Key power long press");
        lastLongpressLog = millis();
    }
    display.queueUiEvent(UiEvent::PowerLong);
}

bool Keypad::menuChordPressed() {
    return keyUp.isLongPressed() && keyDown.isLongPressed();
}

bool Keypad::menuChordInProgress() {
    return !keyUp.isIdle() && !keyDown.isIdle();
}

bool Keypad::handleMenuChord() {
    if (!menuChordPressed()) return menuChordInProgress();
    if (!menuChordHandled) {
        ESP_LOGD(taskName(), "Key up/down long press (menu)");
        display.queueUiEvent(UiEvent::MenuChord);
        menuChordHandled = true;
    }
    return true;
}

Api::Reply Keypad::keyCommand(const char* args) {
    if (strcmp(args, "up") == 0) {
        keyUpClick();
    } else if (strcmp(args, "down") == 0) {
        keyDownClick();
    } else if (strcmp(args, "upLong") == 0) {
        keyUpLongPress();
    } else if (strcmp(args, "downLong") == 0) {
        keyDownLongPress();
    } else if (strcmp(args, "power") == 0) {
        keyPowerClick();
    } else if (strcmp(args, "menu") == 0) {
        display.queueUiEvent(UiEvent::MenuChord);
    } else {
        Api::Reply reply = {};
        reply.code = Api::Reply::Code::InvalidArgs;
        snprintf((char*)reply.data, sizeof(reply.data), "Usage: key up|down|upLong|downLong|power|menu");
        return reply;
    }
    Api::Reply reply = {};
    reply.code = Api::Reply::Code::Success;
    snprintf((char*)reply.data, sizeof(reply.data), "Key event simulated: %s", args);
    return reply;
}