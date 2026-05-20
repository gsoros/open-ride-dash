#include <ArduinoOTA.h>
#include "task.h"
#include "config.h"

/*
TODO: Research rollback strategies for failed OTA updates.

E.g. record successful boot after a delay and revert to previous firmware
after a number of unsuccessful boots.

This is working well in ESPHome (IDF framework), see
https://esphome.io/changelog/2026.1.0/#ota-rollback-support. It is not
clear if it is possible to implement this in Arduino framework, but it
may be worth investigating.

This could in theory allow us to drop the requirement for physical access
for recovery after a bad OTA flash, which would be a big win for remote
debugging and development. However, it adds complexity, storage, and
potential failure modes, so we should research it more before implementing.
*/

class OTA : public Task {
   public:
    using Task::taskStart;

    bool taskStart(float idleFrequency,
                   float uploadFrequency,
                   uint32_t stack = 0,
                   int8_t priority = -1) {
        idleFrequencyHz = idleFrequency;
        uploadFrequencyHz = uploadFrequency;
        return Task::taskStart(idleFrequencyHz, stack, priority);
    }

    virtual const char* taskName() override {
        return "OTA";
    }

    void setHostname(const char* newHostname) {
        hostname = (newHostname != nullptr && newHostname[0] != '\0') ? newHostname : default_hostname;
    }

    virtual void setup() {
        ArduinoOTA.setHostname(hostname.c_str());
        ArduinoOTA.onStart([this]() {
            taskSetFrequency(uploadFrequencyHz);
            ESP_LOGI("OTA", "Start");
        });
        ArduinoOTA.onEnd([this]() {
            taskSetFrequency(idleFrequencyHz);
            ESP_LOGI("OTA", "End");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            ESP_LOGI("OTA", "Progress: %u%%", (progress / (total / 100)));
        });
        ArduinoOTA.onError([this](ota_error_t err) {
            taskSetFrequency(idleFrequencyHz);
            ESP_LOGE("OTA", "Error[%u]", err);
        });
        ArduinoOTA.begin();
        ESP_LOGI(taskName(), "OTA ready");
        Task::taskSetup();
    }

    virtual void taskRun() override {
        ArduinoOTA.handle();
    }

   protected:
    float idleFrequencyHz = 10.0f;
    float uploadFrequencyHz = 100.0f;
    String hostname = default_hostname;
};
