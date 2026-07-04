#ifndef CAN_SIM_H
#define CAN_SIM_H

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
            // Accelerate toward target at up to maxAccel unit/h/s
            float accel = maxAccel * dt;
            if (target - currentValue <= accel) {
                currentValue = target;
            } else {
                currentValue += accel;
            }
        } else if (currentValue > target) {
            // Decelerate toward target at up to maxDecel unit/h/s
            float decel = maxDecel * dt;
            if (currentValue - target <= decel) {
                currentValue = target;
            } else {
                currentValue -= decel;
            }
        }

        currentValue = fmax(0.0f, currentValue);

        // ---- Stopping timeout ----
        if (isStopping && (now - stopStartTime >= stopDuration)) {
            isStopping = false;
            target = (float)random(normalMin, normalMax + 1);
        }

        // ---- CAN message injection every minInterval - maxInterval ms ----
        if (now - lastInjection < nextInterval) {
            return;
        }

        isInjectable = true;

        lastInjection = now;
        nextInterval = (unsigned long)random(minInterval, maxInterval + 1);

        if (isStopping) {
            return;
        }

        // stopProbability % chance to "stop"
        if (random(0, 101) < stopProbability) {
            // Enter stopping mode — decelerate to 0 for minStopLength - maxStopLength ms
            isStopping = true;
            stopStartTime = now;
            stopDuration = (unsigned long)random(minStopLength, maxStopLength + 1);
            target = 0.0f;
        } else {
            // burstProbability % chance of a downhill stretch
            if (random(0, 101) < burstProbability) {
                target = (float)random(burstMin, burstMax + 1);
            } else {
                // Normal riding
                target = (float)random(normalMin, normalMax + 1);
            }
        }
    }

    float maxAccel = 5.0f;
    float maxDecel = 10.0f;

    uint16_t normalMin = 15;  // Normal range
    uint16_t normalMax = 40;

    uint16_t minInterval = 100;  // Injection interval
    uint16_t maxInterval = 500;

    uint8_t stopProbability = 1;
    uint16_t minStopLength = 3000;
    uint16_t maxStopLength = 8000;

    uint8_t burstProbability = 25;
    uint16_t burstMin = 50;
    uint16_t burstMax = 60;

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
    virtual const char* taskName() const override {
        return "CANSim";
    }

    virtual void setup() {
        randomSeed(esp_random());
        speedSim = new NumberSim();
        powerSim = new NumberSim();
        powerSim->normalMin = 200;
        powerSim->normalMax = 500;
        powerSim->burstMin = 700;
        powerSim->burstMax = 900;
        powerSim->maxAccel = 100.0f;
        powerSim->maxDecel = 500.0f;
        powerSim->stopProbability = 2;
    }

    virtual void taskRun() override {
        speedSim->run();
        if (speedSim->isInjectable) {
            // state.speed(speedSim->currentValue);
        }
        powerSim->run();
        if (powerSim->isInjectable) {
            // state.motorPower(powerSim->currentValue);
        }
    }

   protected:
    NumberSim* speedSim = nullptr;
    NumberSim* powerSim = nullptr;
};

#endif  // CAN_SIM_H