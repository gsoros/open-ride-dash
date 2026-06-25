#ifndef ALARM_H
#define ALARM_H

/*
 * Alarm task
 * When armed and motion is detected, a warning is issued.
 * If the warning is not cleared within 1.5 seconds, the alarm is latched until disarmed.
 */

#include <Arduino.h>
#include <Wire.h>
#include <MPU9250.h>
#include <atomic>

#include "config.h"
#include "task.h"
#include "api.h"

class Alarm : public Task {
   public:
    virtual const char* taskName() override;
    virtual void setup();
    virtual bool taskStart(float frequency = -1.0f, uint32_t stack = 0, int8_t priority = -1) override;
    virtual void taskRun() override;

    bool arm();
    bool disarm();
    bool isArmed() const;
    bool isLatched() const;
    bool isWarning() const;

   protected:
    MPU9250 _mpu;
    enum State { ALARM_DISARMED,
                 ALARM_ARMED_IDLE,
                 ALARM_WARNING,
                 ALARM_LATCHED };
    std::atomic<State> _state = ALARM_DISARMED;
    std::atomic<uint32_t> _stateChangeTimestamp{0};

    // Motion within this timeframe triggers a warning
    static constexpr uint32_t WARNING_GRACE_PERIOD_MS = 2000;

    // Motion between WARNING_GRACE_PERIOD_MS and LATCH_DELAY_MS triggers the latch
    static constexpr uint32_t LATCH_DELAY_MS = 10000;

    // Motion threshold, *4mg (0.004g), 25 = 100mg
    static constexpr uint8_t MPU_THRESHOLD = 25;

    // Motion sampling frequency: 0-11 (0.24-500Hz), 5 = 7.81Hz
    static constexpr uint8_t MPU_FREQUENCY = 5;

    Api::Reply _armCommand(const char* args);
    Api::Reply _disarmCommand(const char* args);

    void _clearMPUInterrupt();

    static std::atomic<uint32_t> _mpuInterruptTimestamp;
    static std::atomic<TaskHandle_t> _mpuInterruptTargetTaskHandle;
    static void IRAM_ATTR _mpuInterruptHandler();

    inline String _stateToString(State s) {
        switch (s) {
            case ALARM_DISARMED:
                return "disarmed";
            case ALARM_ARMED_IDLE:
                return "armed idle";
            case ALARM_WARNING:
                return "warning";
            case ALARM_LATCHED:
                return "latched";
            default:
                return "invalid";
        }
    }
};

#endif  // ALARM_H