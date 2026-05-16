#include <ArduinoOTA.h>
#include "task.h"
#include "config.h"

class OTA : public Task {
   public:
    using Task::taskStart;

    void taskStart(float idleFrequency,
                   float uploadFrequency,
                   uint32_t stack = 0,
                   int8_t priority = -1) {
        idleFrequencyHz = idleFrequency;
        uploadFrequencyHz = uploadFrequency;
        Task::taskStart(idleFrequencyHz, stack, priority);
    }

    virtual const char* taskName() override {
        return "OTA";
    }

    virtual void setup() override {
        ArduinoOTA.setHostname(hostname);
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
    }

    virtual void run() override {
        ArduinoOTA.handle();
    }

   protected:
    float idleFrequencyHz = 10.0f;
    float uploadFrequencyHz = 100.0f;
};
