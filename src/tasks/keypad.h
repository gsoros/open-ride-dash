#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>
#include <OneButton.h>

#include "pins.h"
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
    }

    void keyUpClick() {
        ESP_LOGD(taskName(), "Key up click");
        state.keyUpClick = true;
        auto pasLevel = state.pasLevel();
        if (pasLevel < 5) {
            state.pasLevel(pasLevel + 1);
        }
    }

    void keyDownClick() {
        ESP_LOGD(taskName(), "Key down click");
        state.keyDownClick = true;
        auto pasLevel = state.pasLevel();
        if (pasLevel > -1) {
            state.pasLevel(pasLevel - 1);
        }
    }

    void keyPowerClick() {
        ESP_LOGD(taskName(), "Key power click");
        state.keyPowerClick = true;
    }

    void keyUpLongPress() {
        ESP_LOGD(taskName(), "Key up long press");
        if (lastBrightnessChange + 50 > millis()) return;
        display.increaseBrightness();
        lastBrightnessChange = millis();
    }

    void keyDownLongPress() {
        ESP_LOGD(taskName(), "Key down long press");
        if (lastBrightnessChange + 50 > millis()) return;
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
};

#endif  // KEYPAD_H