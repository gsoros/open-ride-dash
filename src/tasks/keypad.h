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
    }

    virtual void taskRun() override {
    }

   protected:
};

#endif  // CAN_H