#include <WiFi.h>
#include "task.h"
#include "api.h"

class Telnet : public Task, public ApiClient {
   public:
    Telnet(uint16_t port = 23) : port(port) {
        instance = this;
    }

    virtual const char* taskName() override {
        return "Telnet";
    }

    virtual void setup() {
        instance = this;
        wifiServer.begin();
        ESP_LOGI(taskName(), "Ready on port %d", port);
        apiClientSetup(taskName());
        taskSetup();
    }

    virtual void taskRun() override {
        if (!wifiClient.connected()) {
            static bool logClientActive = false;
            if (!logClientActive) {
                WiFiClient newClient = wifiServer.available();
                if (newClient) {
                    wifiClient = newClient;
                    logClientActive = true;
                    ESP_LOGI(taskName(), "Client connected.");
                    esp_log_set_vprintf(&telnet_vprintf);
                    wifiClient.println("=== Welcome ===");
                }
            } else {
                wifiClient.stop();
                logClientActive = false;
                esp_log_set_vprintf(&serial_vprintf);
                ESP_LOGI(taskName(), "Client disconnected.");
            }
        } else if (wifiClient.available()) {
            // Echo back any received data
            String line = wifiClient.readStringUntil('\n');
            if (echo) {
                ESP_LOGI(taskName(), "Received from client: %s", line.c_str());
            }
            // Send command to API (non-blocking) and ask for a reply on our queue
            if (line.length() > 0) {
                bool ok = apiClientQueueCommand(line.c_str());
                if (!ok) {
                    wifiClient.println("API busy: request dropped");
                }
            }
        }

        // Check for API replies destined to this Telnet instance
        apiClientReceiveReplies();
    }

    void receiveReply(const Api::Reply& reply) {
        if (instance && instance->wifiClient && instance->wifiClient.connected()) {
            char line[300];
            snprintf(line, sizeof(line), "API Reply [%s] (err=%d): %s", reply.command, reply.errorCode,
                     (char*)reply.data);
            instance->wifiClient.println(line);
        }
    }

    void setEcho(bool enable) {
        echo = enable;
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
        if (instance && instance->wifiClient && instance->wifiClient.connected()) {
            res = instance->wifiClient.print(buf);
        }
        // Serial.print(buf);
        return res;
    }

   protected:
    // std=GNU++17 allows inline static member variables
    inline static Telnet* instance = nullptr;
    uint16_t port = 23;
    WiFiServer wifiServer{port};
    WiFiClient wifiClient;
    bool echo = true;
};
