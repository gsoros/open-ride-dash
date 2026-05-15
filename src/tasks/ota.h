#include <ArduinoOTA.h>
#include "task.h"
#include "config.h"

class OTA : public Task {
   public:
    virtual const char* taskName() override {
        return "OTA";
    }

    virtual void setup() override {
        ArduinoOTA.setHostname(hostname);
        ArduinoOTA.onStart([]() { ESP_LOGI("OTA", "Start"); });
        ArduinoOTA.onEnd([]() { ESP_LOGI("OTA", "End"); });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            ESP_LOGI("OTA", "Progress: %u%%", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t err) {
            ESP_LOGE("OTA", "Error[%u]", err);
        });
        ArduinoOTA.begin();
        ESP_LOGI(taskName(), "OTA ready");
    }

    virtual void run() override {
        ArduinoOTA.handle();
    }
};