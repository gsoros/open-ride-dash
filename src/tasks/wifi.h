#include <WiFi.h>
#include <ESPmDNS.h>
#include "task.h"
#include "config.h"

class Wifi : public Task {
   public:
    virtual const char* taskName() override {
        return "WiFi";
    }

    virtual void setup() override {
        WiFi.mode(WIFI_MODE_STA);
        WiFi.begin(ssid, password);
        ESP_LOGI(taskName(), "Connecting to WiFi");
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(250);
        }
        if (WiFi.status() != WL_CONNECTED) {
            ESP_LOGW(taskName(), "WiFi connect failed");
        } else {
            ESP_LOGI(taskName(), "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
        }
        ESP_LOGI(taskName(), "Starting mDNS with hostname: %s", hostname);
        MDNS.begin(hostname);
        ready = true;
    }

    virtual void run() override {
        ESP_LOGD(taskName(), "Status: %d", WiFi.status());
    }

    bool isReady() const {
        return ready;
    }

    void waitForConnection() {
        while (!isReady()) {
            ESP_LOGD(taskName(), "Waiting for WiFi...");
            delay(100);
        }
    }

   protected:
    bool ready = false;
};