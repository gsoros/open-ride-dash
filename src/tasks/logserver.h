#include <WiFi.h>
#include "task.h"

class LogServer : public Task {
   public:
    LogServer() {
        s_instance = this;
    }

    virtual const char* taskName() override {
        return "LogServer";
    }

    virtual void setup() override {
        s_instance = this;
        logServer.begin();
        ESP_LOGI(taskName(), "Log server ready on port 23");
    }

    virtual void run() override {
        if (!logClient.connected()) {
            static bool logClientActive = false;
            if (!logClientActive) {
                WiFiClient newClient = logServer.available();
                if (newClient) {
                    logClient = newClient;
                    logClientActive = true;
                    ESP_LOGI(taskName(), "Log client connected.");
                    esp_log_set_vprintf(&telnet_vprintf);
                    logClient.println("=== Welcome ===");
                }
            } else {
                logClient.stop();
                logClientActive = false;
                esp_log_set_vprintf(&serial_vprintf);
                ESP_LOGI(taskName(), "Log client disconnected.");
            }
        }
    }

    static int serial_vprintf(const char* fmt, va_list args) {
        char buf[256];
        vsnprintf(buf, sizeof(buf), fmt, args);
        return Serial.print(buf);
    }

    static int telnet_vprintf(const char* fmt, va_list args) {
        char buf[256];
        int res = 0;
        vsnprintf(buf, sizeof(buf), fmt, args);
        if (s_instance && s_instance->logClient && s_instance->logClient.connected()) {
            res = s_instance->logClient.print(buf);
        }
        // Serial.print(buf);
        return res;
    }

   protected:
    inline static LogServer* s_instance = nullptr;
    WiFiServer logServer{23};
    WiFiClient logClient;
};
