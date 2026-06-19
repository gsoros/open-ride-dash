#include <Arduino.h>
#include <ArduinoOTA.h>
#include <esp_ota_ops.h>
#include "task.h"
#include "has_preferences.h"
#include "config.h"
#include "wifi.h"

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
*/

extern Wifi wifi;

class OTA : public Task, public HasPreferences {
   public:
    enum PartitionState {
        VALID,
        PENDING,
        FAILED
    };

    virtual void setup() {
        logCurrentBootPartition();

        if (!preferencesSetup(taskName()))
            ESP_LOGE(taskName(), "Failed to open preferences, using defaults");

        static constexpr const char* hostnameKey = "hostname";
        if (!preferencesReady || !preferences.isKey(hostnameKey))
            ESP_LOGW(taskName(), "Hostname not found in preferences, using defaults");
        else
            setHostname(preferences.getString(hostnameKey, default_hostname).c_str());

        if (!preferencesReady)
            ESP_LOGE(taskName(), "Prefs not ready");
        else {
            if (!preferences.isKey(partitionStateKey) || !preferences.isKey(crashCountKey)) {
                ESP_LOGE(taskName(), "Keys missing (Fresh flash or NVS wipe). Initializing...");
                if (!preferences.putInt(partitionStateKey, PartitionState::VALID))
                    ESP_LOGE(taskName(), "Failed to mark partition VALID");
                if (!preferences.putInt(crashCountKey, 0))
                    ESP_LOGE(taskName(), "Failed to reset crash count");
            } else {
                partitionState = (PartitionState)(preferences.getInt(partitionStateKey));
                ESP_LOGD(taskName(), "Got partition state from NVS: %s", partitionStateToString(partitionState));
                crashCount = preferences.getInt(crashCountKey);
                ESP_LOGD(taskName(), "Got crash count from NVS: %d/%d", crashCount, crashCountMax);
            }
        }

        ESP_LOGI(taskName(), "Partition state: %s, crash count: %d/%d",
                 partitionStateToString(partitionState), crashCount, crashCountMax);

        if (partitionState == PartitionState::FAILED) {
            ESP_LOGE(taskName(), "Partition state is FAILED! (Failed rollback?)");
            // TODO: What to do here? If we have a third, factory partition, we could try to boot it.
        } else if (partitionState == PartitionState::PENDING) {
            crashCount++;
            ESP_LOGD(taskName(), "OTA boot count in PENDING state: %d/%d", crashCount, crashCountMax);
            if (crashCount > crashCountMax) {
                ESP_LOGE(taskName(), "[CRITICAL] Boot loop limit reached! Executing rollback...");
                logNextBootPartition();
                if (!preferencesReady)
                    ESP_LOGE(taskName(), "Prefs not ready");
                else {
                    if (!preferences.putInt(partitionStateKey, (int)PartitionState::FAILED))
                        ESP_LOGE(taskName(), "Failed to mark partition as FAILED");
                    if (!preferences.putInt(crashCountKey, 0))
                        ESP_LOGE(taskName(), "Failed to reset crash count");
                }
                if (esp_ota_mark_app_invalid_rollback_and_reboot() != ESP_OK) {
                    ESP_LOGE(taskName(), "Cannot rollback to previous partition");
                }
            }
            if (!preferences.putInt(crashCountKey, crashCount))
                ESP_LOGE(taskName(), "Failed to save crash count = %d", crashCount);
            stabilityTestRunning = true;
            stabilityTestStartTime = millis();
        }

        ArduinoOTA.setHostname(hostname.c_str());
        ArduinoOTA.onStart([this]() {
            taskSetFrequency(uploadFrequencyHz);
            ESP_LOGI(taskName(), "Start");
        });
        ArduinoOTA.onEnd([this]() {
            logNextBootPartition();
            ESP_LOGI(taskName(), "Marking partition as PENDING");
            if (!preferencesReady)
                ESP_LOGE(taskName(), "Prefs not ready");
            else {
                if (!preferences.putInt(partitionStateKey, (int)PartitionState::PENDING))
                    ESP_LOGE(taskName(), "Failed to mark partition as PENDING");
                if (!preferences.putInt(crashCountKey, 0))
                    ESP_LOGE(taskName(), "Failed to reset crash count");
            }
            taskSetFrequency(idleFrequencyHz);
        });
        ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
            static uint32_t lastProgressLog = 0;
            if (millis() - lastProgressLog > 3000 || progress == total) {
                lastProgressLog = millis();
                ESP_LOGI(taskName(), "Progress: %u%%", (progress / (total / 100)));
            }
        });
        ArduinoOTA.onError([this](ota_error_t err) {
            taskSetFrequency(idleFrequencyHz);
            ESP_LOGE(taskName(), "Error: %s", otaErrorToString(err));
        });
        ArduinoOTA.begin();

        ESP_LOGI(taskName(), "Ready");
    }

    using Task::taskStart;

    bool taskStart(float idleFrequency,
                   float uploadFrequency,
                   uint32_t stack = 0,
                   int8_t priority = -1) {
        idleFrequencyHz = idleFrequency;
        uploadFrequencyHz = uploadFrequency;
        return Task::taskStart(idleFrequencyHz, stack, priority);
    }

    virtual void taskRun() override {
        ArduinoOTA.handle();

        if (stabilityTestRunning) {
            uint32_t t = millis();
            static uint32_t lastStabilityLog = 0;
            if (t - lastStabilityLog > 10000) {
                lastStabilityLog = t;
                ESP_LOGD(taskName(), "Stability timer: %d/%ds",
                         (t - stabilityTestStartTime) / 1000, stabilityTimeValid / 1000);
            }
            if (!wifi.isConnected()) {
                ESP_LOGE(taskName(), "WiFi not connected, resetting stability timer");
                stabilityTestStartTime = t;
            } else if (t - stabilityTestStartTime >= stabilityTimeValid) {
                ESP_LOGD(taskName(), "Firmware survived %ds with active WiFi, partition is valid", stabilityTimeValid / 1000);
                if (!preferencesReady)
                    ESP_LOGE(taskName(), "Prefs not ready, cannot mark partition VALID");
                else {
                    if (!preferences.putInt(partitionStateKey, (int)PartitionState::VALID))
                        ESP_LOGE(taskName(), "Failed to mark partition VALID");
                    if (!preferences.putInt(crashCountKey, 0))
                        ESP_LOGE(taskName(), "Failed to reset crash count");
                }
                stabilityTestRunning = false;
            }
        }
    }

    virtual const char* taskName() override {
        return "OTA";
    }

    void setHostname(const char* newHostname) {
        hostname = (newHostname != nullptr && newHostname[0] != '\0') ? newHostname : default_hostname;
    }

   protected:
    float idleFrequencyHz = 10.0f;
    float uploadFrequencyHz = 100.0f;
    String hostname = default_hostname;

    static constexpr const char* partitionStateKey = "partitionState";
    static constexpr const char* crashCountKey = "crashCount";
    PartitionState partitionState = PartitionState::VALID;
    uint8_t crashCount = 0;
    static constexpr uint8_t crashCountMax = 3;  // rolls back on 4th boot
    static constexpr uint32_t stabilityTimeValid = 60000;
    uint32_t stabilityTestStartTime = 0;
    bool stabilityTestRunning = false;

    void logCurrentBootPartition() {
        const esp_partition_t* running = esp_ota_get_running_partition();
        ESP_LOGI(taskName(), "Current boot partition: %s", running ? running->label : "Unknown");
    }

    void logNextBootPartition() {
        const esp_partition_t* running = esp_ota_get_running_partition();
        const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);
        ESP_LOGI(taskName(), "Next Boot Partition: %s",
                 next ? next->label : running ? running->label
                                              : "Unknown");
    }

    const char* partitionStateToString(PartitionState state) {
        switch (state) {
            case VALID:
                return "VALID";
            case PENDING:
                return "PENDING";
            case FAILED:
                return "FAILED";
            default:
                return "UNKNOWN";
        }
    }

    const char* otaErrorToString(ota_error_t err) {
        switch (err) {
            case OTA_AUTH_ERROR:
                return "OTA_AUTH_ERROR";
            case OTA_BEGIN_ERROR:
                return "OTA_BEGIN_ERROR";
            case OTA_CONNECT_ERROR:
                return "OTA_CONNECT_ERROR";
            case OTA_RECEIVE_ERROR:
                return "OTA_RECEIVE_ERROR";
            case OTA_END_ERROR:
                return "OTA_END_ERROR";
            default:
                return "UNKNOWN";
        }
    }
};
