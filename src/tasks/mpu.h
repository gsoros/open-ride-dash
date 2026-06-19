#ifndef MPU_H
#define MPU_H

#include "config.h"
#include "model/state.h"
#include "task.h"

extern State state;

class MPU : public Task {
   public:
    virtual const char* taskName() override {
        return "MPU";
    }

    virtual void setup() {
    }

    virtual void taskRun() override {
    }

   protected:
};

#endif  // MPU_H