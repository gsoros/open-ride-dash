#include "tasks/ota.h"

#include <ArduinoOTA.h>
#include <esp_ota_ops.h>

#include "tasks/wifi.h"
#include "tasks/wifiserial.h"
#include "tasks/display.h"
#include "model/state.h"
#include "ui/events.h"

extern Wifi wifi;
extern Display display;
extern WifiSerial wifiSerial;
extern State state;

const char* OTA::taskName() const {
    return "OTA";
}

void OTA::setup() {
    logCurrentBootPartition();

    if (!preferencesSetup(taskName()))
        ESP_LOGE(taskName(), "Failed to open preferences, using defaults");

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

    ArduinoOTA.setHostname(state.hostname());
    ArduinoOTA.onStart([this]() {
        taskSetFrequency(uploadFrequencyHz);
        ESP_LOGI(taskName(), "Start");
        state.ota(State::OTA_START);
        display.queueUiEvent(UiEvent::OtaChange);
        // TODO: BLE disconnect, server->stop, deinit to stop sharing radio between BLE and WiFi and speed up OTA?
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
        state.ota(State::OTA_END);
        display.queueUiEvent(UiEvent::OtaChange);
        ESP_LOGI(taskName(), "Enabling WiFi");
        // api.queueCommand("wifi on");  // make sure WiFi STA is enabled
        wifiSerial.disconnectWithNotice("Update done, disconnecting");
        taskSetFrequency(idleFrequencyHz);
        delay(1000);
        // NOTE: reboot is initiated by ArduinoOTA
    });
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
        uint8_t percent = std::clamp((progress / (total / 100U)), 0U, 100U);
        uint8_t lastOtaState = state.ota();
        if (percent == lastOtaState) return;
        state.ota(percent);
        display.queueUiEvent(UiEvent::OtaChange);
        static uint32_t lastProgressLog = 0;
        if (millis() - lastProgressLog > 3000 || progress == total) {
            lastProgressLog = millis();
            ESP_LOGI(taskName(), "Progress: %u%%", percent);
        }
    });
    ArduinoOTA.onError([this](ota_error_t err) {
        taskSetFrequency(idleFrequencyHz);
        ESP_LOGE(taskName(), "Error: %s", otaErrorToString(err));
        state.ota(State::OTA_ERROR);
        display.queueUiEvent(UiEvent::OtaChange);
        // TODO: reboot to restart BLE?
    });

    wifi.waitForReady();  // make sure WiFi is ready before starting OTA
    ArduinoOTA.begin();

    ESP_LOGI(taskName(), "Ready");
}

bool OTA::taskStart(float idleFrequency,
                    float uploadFrequency,
                    uint32_t stack,
                    int8_t priority) {
    idleFrequencyHz = idleFrequency;
    uploadFrequencyHz = uploadFrequency;
    return Task::taskStart(idleFrequencyHz, stack, priority);
}

void OTA::taskRun() {
    ArduinoOTA.handle();

    if (stabilityTestRunning) {
        uint32_t t = millis();
        static uint32_t lastStabilityLog = 0;
        if (t - lastStabilityLog > 10000) {
            lastStabilityLog = t;
            ESP_LOGD(taskName(), "Stability timer: %d/%ds",
                     (t - stabilityTestStartTime) / 1000, stabilityTimeValid / 1000);
        }
        if (!wifi.isConnected() && !wifi.hasApClient()) {
            ESP_LOGE(taskName(), "WiFi not connected and no AP client, resetting stability timer");
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

void OTA::logCurrentBootPartition() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    ESP_LOGI(taskName(), "Current boot partition: %s", running ? running->label : "Unknown");
}

void OTA::logNextBootPartition() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(taskName(), "Next Boot Partition: %s",
             next ? next->label : running ? running->label
                                          : "Unknown");
}

const char* OTA::partitionStateToString(PartitionState state) {
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

const char* OTA::otaErrorToString(ota_error_t err) {
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