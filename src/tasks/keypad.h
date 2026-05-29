#ifndef KEYPAD_H
#define KEYPAD_H

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
        pinMode(KEY_UP, INPUT_PULLUP);
        pinMode(KEY_DOWN, INPUT_PULLUP);
        pinMode(KEY_POWER, INPUT_PULLUP);
        keyUpChanged();  // Read initial state
        keyDownChanged();
        keyPowerChanged();
        attachInterrupt(KEY_UP, keyUpChanged, CHANGE);
        attachInterrupt(KEY_DOWN, keyDownChanged, CHANGE);
        attachInterrupt(KEY_POWER, keyPowerChanged, CHANGE);

        Task::taskSetup();
    }

    virtual void taskRun() override {
        uint32_t t = millis();
        if (t < 3000) return;
        static bool keyUpLast = false;
        static bool keyDownLast = false;
        static bool pasIncrement = false;
        static bool pasDecrement = false;
        if (state.keyUp != keyUpLast) {
            keyUpLast = state.keyUp;
            if (state.keyUp && !pasIncrement) {  // pressed
                int8_t pasLevel = state.pasLevel();
                if (pasLevel < 5) {
                    state.pasLevel(pasLevel + 1);
                    pasIncrement = true;
                }
            } else if (!state.keyUp) {  // released
                pasIncrement = false;
            }
        }
        if (state.keyDown != keyDownLast) {
            keyDownLast = state.keyDown;
            if (state.keyDown && !pasDecrement) {  // pressed
                int8_t pasLevel = state.pasLevel();
                if (pasLevel > -1) {
                    state.pasLevel(pasLevel - 1);
                    pasDecrement = true;
                }
            } else if (!state.keyDown) {  // released
                pasDecrement = false;
            }
        }

        // state.keyUp = digitalRead(KEY_UP) == LOW;
        // state.keyDown = digitalRead(KEY_DOWN) == LOW;
        // state.keyPower = digitalRead(KEY_POWER) == LOW;

        return;
        static ulong last = 0;
        if (millis() - last < 100) return;
        last = millis();
        ESP_LOGD(taskName(), "State: up=%d, down=%d, power=%d",
                 state.keyUp, state.keyDown, state.keyPower);
    }

   protected:
    static void IRAM_ATTR keyUpChanged();
    static void IRAM_ATTR keyDownChanged();
    static void IRAM_ATTR keyPowerChanged();
};

#endif  // KEYPAD_H