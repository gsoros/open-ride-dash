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