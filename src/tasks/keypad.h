#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>
#include <OneButton.h>

#include "pins.h"
#include "model/state.h"
#include "task.h"

extern State state;

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

   protected:
    OneButton keyUp;
    OneButton keyDown;
    OneButton keyPower;
};

#endif  // KEYPAD_H