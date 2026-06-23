#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>
#include <OneButton.h>

#include "config.h"
#include "display.h"
#include "task.h"
#include "ui/ui_events.h"

extern Display display;

class Keypad : public Task {
   public:
    virtual const char* taskName() override {
        return "Keypad";
    }

    virtual void setup() {
        keyUp.setup(PIN_KEY_UP, INPUT_PULLUP, true);        // Active LOW
        keyDown.setup(PIN_KEY_DOWN, INPUT_PULLUP, true);    // Active LOW
        keyPower.setup(PIN_KEY_POWER, INPUT_PULLUP, true);  // Active LOW

        keyUp.attachClick([](void* s) { ((Keypad*)s)->keyUpClick(); }, this);
        keyDown.attachClick([](void* s) { ((Keypad*)s)->keyDownClick(); }, this);
        keyPower.attachClick([](void* s) { ((Keypad*)s)->keyPowerClick(); }, this);

        keyUp.attachDuringLongPress([](void* s) { ((Keypad*)s)->keyUpLongPress(); }, this);
        keyDown.attachDuringLongPress([](void* s) { ((Keypad*)s)->keyDownLongPress(); }, this);
        keyPower.attachDuringLongPress([](void* s) { ((Keypad*)s)->keyPowerLongPress(); }, this);
    }

    virtual void taskRun() override {
        // uint32_t t = millis();

        keyUp.tick();
        keyDown.tick();
        keyPower.tick();

        if (!pageMenuChordInProgress()) {
            pageMenuChordHandled = false;
        }
    }

    void keyUpClick() {
        ESP_LOGD(taskName(), "Key up click");
        display.queueUiEvent(UiEvent::UpClick);
    }

    void keyDownClick() {
        ESP_LOGD(taskName(), "Key down click");
        display.queueUiEvent(UiEvent::DownClick);
    }

    void keyPowerClick() {
        ESP_LOGD(taskName(), "Key power click");
        display.queueUiEvent(UiEvent::SelectClick);
    }

    void keyUpLongPress() {
        if (handlePageMenuChord()) return;
        display.queueUiEvent(UiEvent::UpLongPress);
    }

    void keyDownLongPress() {
        if (handlePageMenuChord()) return;
        display.queueUiEvent(UiEvent::DownLongPress);
    }

    void keyPowerLongPress() {
        ESP_LOGD(taskName(), "Key power long press");
        display.queueUiEvent(UiEvent::PowerLongPress);
    }

   protected:
    OneButton keyUp;
    OneButton keyDown;
    OneButton keyPower;
    bool pageMenuChordHandled = false;

    bool pageMenuChordPressed() {
        return keyUp.isLongPressed() && keyDown.isLongPressed();
    }

    bool pageMenuChordInProgress() {
        return !keyUp.isIdle() && !keyDown.isIdle();
    }

    bool handlePageMenuChord() {
        if (!pageMenuChordPressed()) return pageMenuChordInProgress();
        if (!pageMenuChordHandled) {
            ESP_LOGD(taskName(), "Key up/down long press (menu)");
            display.queueUiEvent(UiEvent::MenuChord);
            pageMenuChordHandled = true;
        }
        return true;
    }
};

#endif  // KEYPAD_H
