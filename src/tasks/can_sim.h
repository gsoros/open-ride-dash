#pragma once

#include "model/state.h"
#include "task.h"

extern State state;

// Simulated CAN injections
class CANSim : public Task {
   public:
    virtual const char* taskName() override {
        return "CANSim";
    }

    virtual void setup() {
        randomSeed(esp_random());

        unsigned long now = millis();
        lastMessageTime = now;
        lastSimUpdateTime = now;
        nextMsgInterval = (unsigned long)random(100, 501);
        targetSpeed = (float)random(0, 51);
    }

    /*
    The simulation logic performs two main functions:
    1. Physics Simulation: Updates the `currentSpeed` every loop iteration using a delta-time (dt)
       calculation. It smoothly interpolates the speed toward a `targetSpeed` using defined
       acceleration (5 km/h/s) and deceleration (10 km/h/s) constants.
    2. State Machine & Timing: Every 100-500ms, it pushes the current speed to the global state.
       It also manages random behavior changes, such as simulating downhill stretches or
       "traffic light" stops where the target speed is set to zero for a random duration.
    */
    virtual void taskRun() override {
        unsigned long now = millis();

        // ---- Physics simulation (continuous, every call ~10ms) ----
        float dt = (now - lastSimUpdateTime) / 1000.0f;
        if (dt <= 0.0f) dt = 0.01f;
        if (dt > 0.1f) dt = 0.1f;  // clamp after long pauses
        lastSimUpdateTime = now;

        if (currentSpeed < targetSpeed) {
            // Accelerate toward target at up to 5 km/h/s
            float maxAccel = 5.0f * dt;
            if (targetSpeed - currentSpeed <= maxAccel) {
                currentSpeed = targetSpeed;
            } else {
                currentSpeed += maxAccel;
            }
        } else if (currentSpeed > targetSpeed) {
            // Decelerate toward target at up to 10 km/h/s
            float maxDecel = 10.0f * dt;
            if (currentSpeed - targetSpeed <= maxDecel) {
                currentSpeed = targetSpeed;
            } else {
                currentSpeed -= maxDecel;
            }
        }

        currentSpeed = fmax(0.0f, currentSpeed);

        // ---- Stopping timeout ----
        if (isStopping && (now - stopStartTime >= stopDuration)) {
            isStopping = false;
            targetSpeed = (float)random(25, 51);
        }

        // ---- CAN message injection every 100–500ms ----
        if (now - lastMessageTime < nextMsgInterval) {
            return;
        }

        // Inject current speed into shared state
        state.setSpeed(currentSpeed);

        lastMessageTime = now;
        nextMsgInterval = (unsigned long)random(100, 501);

        if (isStopping) {
            return;
        }

        // 1 % chance to "stop"
        if (random(0, 101) == 0) {
            // Enter stopping mode — decelerate to 0 for 3-8 s
            isStopping = true;
            stopStartTime = now;
            stopDuration = (unsigned long)random(3000, 8001);
            targetSpeed = 0.0f;
        } else {
            // 50 % chance of a downhill stretch
            if (random(0, 2) == 0) {
                targetSpeed = (float)random(51, 61);
            } else {
                // Normal riding
                targetSpeed = (float)random(15, 41);
            }
        }
    }

   protected:
    // ---- Simulation state (reusable for future parameters) ----
    float currentSpeed = 0.0f;
    float targetSpeed = 0.0f;
    bool isStopping = false;

    // ---- CAN message timing ----
    unsigned long lastMessageTime = 0;
    unsigned long nextMsgInterval = 100;

    // ---- Physics timing ----
    unsigned long lastSimUpdateTime = 0;

    // ---- "Traffic light" stop state ----
    unsigned long stopStartTime = 0;
    unsigned long stopDuration = 2000;
};