#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>
#include <OneButton.h>

#include "config.h"
#include "model/state.h"
#include "display.h"
#include "task.h"

extern State state;
extern Display display;

class Keypad : public Task {
   public:
    virtual const char* taskName() override {
        return "Keypad";
    }

    virtual void setup() {
        keyUp.setup(KEY_UP, INPUT_PULLUP, true);        // Active LOW
        keyDown.setup(KEY_DOWN, INPUT_PULLUP, true);    // Active LOW
        keyPower.setup(KEY_POWER, INPUT_PULLUP, true);  // Active LOW

        keyUp.attachClick([](void* s) { ((Keypad*)s)->keyUpClick(); }, this);
        keyDown.attachClick([](void* s) { ((Keypad*)s)->keyDownClick(); }, this);
        keyPower.attachClick([](void* s) { ((Keypad*)s)->keyPowerClick(); }, this);

        keyUp.attachDuringLongPress([](void* s) { ((Keypad*)s)->keyUpLongPress(); }, this);
        keyDown.attachDuringLongPress([](void* s) { ((Keypad*)s)->keyDownLongPress(); }, this);
        keyPower.attachDuringLongPress([](void* s) { ((Keypad*)s)->keyPowerLongPress(); }, this);

        Task::taskSetup();
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
        if (display.keyUpClick()) return;
        auto pasLevelRequested = state.pasLevelRequested();
        if (pasLevelRequested < 5) {
            state.pasLevelRequested(pasLevelRequested + 1);
        }
    }

    void keyDownClick() {
        ESP_LOGD(taskName(), "Key down click");
        if (display.keyDownClick()) return;
        auto pasLevelRequested = state.pasLevelRequested();
        if (pasLevelRequested > -1) {
            state.pasLevelRequested(pasLevelRequested - 1);
        }
    }

    void keyPowerClick() {
        ESP_LOGD(taskName(), "Key power click");
        display.keyPowerClick();
    }

    void keyUpLongPress() {
        if (handlePageMenuChord()) return;
        if (display.menuActive()) return;
        if (lastBrightnessChange + 50 > millis()) return;
        static uint32_t lastLog = 0;
        if (millis() - lastLog > 500) {
            ESP_LOGD(taskName(), "Key up long press (increase brightness)");
            lastLog = millis();
        }
        display.increaseBrightness();
        lastBrightnessChange = millis();
    }

    void keyDownLongPress() {
        if (handlePageMenuChord()) return;
        if (display.menuActive()) return;
        if (lastBrightnessChange + 50 > millis()) return;
        bool isWalkAssist = state.pasLevel() == -1;
        static uint32_t lastLog = 0;
        if (millis() - lastLog > 500) {
            ESP_LOGW(taskName(), "Key down long press (%s)",
                     isWalkAssist ? "walk assist" : "decrease brightness");
            lastLog = millis();
        }
        if (isWalkAssist) return;
        display.decreaseBrightness();
        lastBrightnessChange = millis();
    }

    void keyPowerLongPress() {
        ESP_LOGD(taskName(), "Key power long press");
    }

   protected:
    OneButton keyUp;
    OneButton keyDown;
    OneButton keyPower;
    uint32_t lastBrightnessChange = 0;
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
            display.enterMenu();
            pageMenuChordHandled = true;
        }
        return true;
    }
};

#endif  // KEYPAD_H
