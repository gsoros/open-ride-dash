#pragma once

#include "model/state.h"
#include "task.h"

extern State state;

class NumberSim {
   public:
    NumberSim() {
        unsigned long now = millis();
        lastInjection = now;
        lastUpdate = now;
        nextInterval = (unsigned long)random(100, 501);
        target = (float)random(0, 51);
    }

    void run() {
        unsigned long now = millis();
        isInjectable = false;

        // ---- Physics simulation (continuous, every call ~10ms) ----
        float dt = (now - lastUpdate) / 1000.0f;
        if (dt <= 0.0f) dt = 0.01f;
        if (dt > 0.1f) dt = 0.1f;  // clamp after long pauses
        lastUpdate = now;

        if (currentValue < target) {
            // Accelerate toward target at up to 5 km/h/s
            float maxAccel = 5.0f * dt;
            if (target - currentValue <= maxAccel) {
                currentValue = target;
            } else {
                currentValue += maxAccel;
            }
        } else if (currentValue > target) {
            // Decelerate toward target at up to 10 km/h/s
            float maxDecel = 10.0f * dt;
            if (currentValue - target <= maxDecel) {
                currentValue = target;
            } else {
                currentValue -= maxDecel;
            }
        }

        currentValue = fmax(0.0f, currentValue);

        // ---- Stopping timeout ----
        if (isStopping && (now - stopStartTime >= stopDuration)) {
            isStopping = false;
            target = (float)random(25, 51);
        }

        // ---- CAN message injection every 100–500ms ----
        if (now - lastInjection < nextInterval) {
            return;
        }

        isInjectable = true;

        lastInjection = now;
        nextInterval = (unsigned long)random(100, 501);

        if (isStopping) {
            return;
        }

        // 1 % chance to "stop"
        if (random(0, 101) == 0) {
            // Enter stopping mode — decelerate to 0 for 3-8 s
            isStopping = true;
            stopStartTime = now;
            stopDuration = (unsigned long)random(3000, 8001);
            target = 0.0f;
        } else {
            // 50 % chance of a downhill stretch
            if (random(0, 2) == 0) {
                target = (float)random(51, 61);
            } else {
                // Normal riding
                target = (float)random(15, 41);
            }
        }
    }

    bool isInjectable = false;
    float currentValue = 0.0f;

   protected:
    float target = 0.0f;
    bool isStopping = false;

    // ---- CAN message timing ----
    unsigned long lastInjection = 0;
    unsigned long nextInterval = 100;

    // ---- Physics timing ----
    unsigned long lastUpdate = 0;

    // ---- Stop state ----
    unsigned long stopStartTime = 0;
    unsigned long stopDuration = 2000;
};

// Simulated CAN injections
class CANSim : public Task {
   public:
    virtual const char* taskName() override {
        return "CANSim";
    }

    virtual void setup() {
        randomSeed(esp_random());
        speedSim = new NumberSim();
    }

    virtual void taskRun() override {
        speedSim->run();
        if (speedSim->isInjectable) {
            state.setSpeed(speedSim->currentValue);
        }
    }

   protected:
    NumberSim* speedSim;
};
