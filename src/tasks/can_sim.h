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
            targetSpeed = (float)random(5, 51);
        }

        // ---- CAN message injection every 100–500ms ----
        if (now - lastMessageTime >= nextMsgInterval) {
            // Inject current speed into shared state
            state.setSpeed(currentSpeed);

            lastMessageTime = now;
            nextMsgInterval = (unsigned long)random(100, 501);

            if (!isStopping) {
                int roll = random(0, 10);  // 10 % chance to "stop"
                if (roll == 0) {
                    // Enter stopping mode — decelerate to 0 for 1–2 s
                    isStopping = true;
                    stopStartTime = now;
                    stopDuration = (unsigned long)random(1000, 2001);
                    targetSpeed = 0.0f;
                } else {
                    // Normal riding: random target 0–50 km/h
                    targetSpeed = (float)random(0, 51);

                    // 5 % chance of a spike to 51–60 km/h
                    if (random(0, 20) == 0) {
                        targetSpeed = (float)random(51, 61);
                    }
                }
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