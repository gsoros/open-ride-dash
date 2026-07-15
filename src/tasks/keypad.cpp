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
        "Usage: key <up|down|upLong|downLong|power|menu>\nSimulate a key press.");
}

void Keypad::taskRun() {
    // uint32_t t = millis();

    keyUp.tick();
    keyDown.tick();
    keyPower.tick();

    if (!pageMenuChordInProgress()) {
        pageMenuChordHandled = false;
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
    display.queueUiEvent(UiEvent::SelectClick);
}

void Keypad::keyUpLongPress() {
    if (handlePageMenuChord()) return;
    display.queueUiEvent(UiEvent::UpLongPress);
}

void Keypad::keyDownLongPress() {
    if (handlePageMenuChord()) return;
    display.queueUiEvent(UiEvent::DownLongPress);
}

void Keypad::keyPowerLongPress() {
    if (lastLongpressLog + longpressLogInterval < millis()) {
        ESP_LOGD(taskName(), "Key power long press");
        lastLongpressLog = millis();
    }
    display.queueUiEvent(UiEvent::PowerLongPress);
}

bool Keypad::pageMenuChordPressed() {
    return keyUp.isLongPressed() && keyDown.isLongPressed();
}

bool Keypad::pageMenuChordInProgress() {
    return !keyUp.isIdle() && !keyDown.isIdle();
}

bool Keypad::handlePageMenuChord() {
    if (!pageMenuChordPressed()) return pageMenuChordInProgress();
    if (!pageMenuChordHandled) {
        ESP_LOGD(taskName(), "Key up/down long press (menu)");
        display.queueUiEvent(UiEvent::MenuChord);
        pageMenuChordHandled = true;
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
        reply.code = Api::ReplyCode::INVALID_ARGS;
        snprintf((char*)reply.data, sizeof(reply.data), "Invalid argument: %s", args);
        return reply;
    }
    Api::Reply reply = {};
    reply.code = Api::ReplyCode::SUCCESS;
    snprintf((char*)reply.data, sizeof(reply.data), "Key event simulated: %s", args);
    return reply;
}