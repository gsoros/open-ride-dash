#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "task.h"
#include "has_preferences.h"
#include "config.h"

class Wifi;

/*
The Arduino framework is shipped with precompiled ESP-IDF libraries without rollback support.
We could use a dual Arduino/ESP-IDF build, but we chose to use a rollback-enabled bootloader
and handle the rollback logic here. The criteria for marking an OTA update as valid are that
WiFi stays connected for at least 60 seconds, and that the system doesn't crash 3 times in a
row. This requires a WiFi connection to be available for the entire 60 seconds after an OTA
update, otherwise the update is rolled back. Power cuts or clean reboots during this time will
also trigger a rollback. Redundancy for NVS failures could be further improved by using two
copies of the partition state and the crash counter and comparing them. Also, if we have enough
space for a third partition, we could use it to store a factory partition that can only be
updated over USB and used to recover from a failed OTA rollback. Another improvement could be
to use RTC memory to track crash count.

TODO: We had a bricking event where the system was stuck in a boot loop and the OTA rollback
didn't work. We introduced a bug that caused the system to crash only if WiFi was disabled at
boot. We uploaded the firmware, and then disabled WiFi after the 60s stability period. Maybe we
should start counting crashes even when WiFi is disabled, and rollback if the system crashes
3 times in a row, regardless of WiFi state? Boot, increment crash count, reset crash count
after 60s of uptime, rollback if crash count exceeds limit. If we do this, we should also
reset the crash count on a clean shutdown or reboot, so that we don't rollback if the system
is powered off cleanly before the 60s limit is reached 3 times in a row. The user would still
need to be aware that power cycling the system can potentially trigger a rollback.
*/

extern Wifi wifi;

class OTA : public Task, public HasPreferences {
   public:
    enum PartitionState {
        VALID,
        PENDING,
        FAILED
    };

    virtual void setup();

    using Task::taskStart;

    bool taskStart(float idleFrequency,
                   float uploadFrequency,
                   uint32_t stack = 0,
                   int8_t priority = -1);

    virtual void taskRun() override;

    virtual const char* taskName() const override;

   protected:
    float idleFrequencyHz = 10.0f;
    float uploadFrequencyHz = 100.0f;

    static constexpr const char* partitionStateKey = "partitionState";
    static constexpr const char* crashCountKey = "crashCount";
    PartitionState partitionState = PartitionState::VALID;
    uint8_t crashCount = 0;
    static constexpr uint8_t crashCountMax = 3;  // rolls back on 4th boot
    static constexpr uint32_t stabilityTimeValid = 60000;
    uint32_t stabilityTestStartTime = 0;
    bool stabilityTestRunning = false;

    void logCurrentBootPartition();
    void logNextBootPartition();
    const char* partitionStateToString(PartitionState state);
    const char* otaErrorToString(ota_error_t err);
};

#endif  // OTA_H
