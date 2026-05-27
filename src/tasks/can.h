#ifndef CAN_H
#define CAN_H

#include "pins.h"
#include "model/state.h"
#include "task.h"

extern State state;

class CAN : public Task {
   public:
    virtual const char* taskName() override {
        return "CAN";
    }

    virtual void setup() {
    }

    virtual void taskRun() override {
    }

   protected:
};

#endif  // CAN_H