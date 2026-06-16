#ifndef TELNET_H
#define TELNET_H

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
        api.registerCommand(
            "echo",
            [this](const char* args) {
                return setEchoCommand(args);
            },
            "Usage: echo 0|1|True|true|False|false|On|on|Off|off\nEnables or disables Telnet input echo.");
        Task::taskSetup();
    }

    virtual void taskRun() override {
#ifdef FEATURE_SERIAL
        static String serialBuf = "";

        while (Serial.available()) {
            char c = Serial.read();
            Serial.print(c);
            if (c == '\n' || c == '\r') {
                if (serialBuf.length() > 0) {
                    // ESP_LOGD(taskName(), "Received from serial: '%s'", serialBuf.c_str());
                    if (!apiClientQueueCommand(serialBuf.c_str())) {
                        ESP_LOGE(taskName(), "API request dropped");
                    }
                    serialBuf = "";
                }
            } else {
                serialBuf += c;
                if (serialBuf.length() > 63) {
                    ESP_LOGE(taskName(), "Serial buffer overflow");
                    serialBuf = "";
                }
            }
        }

#endif

        if (!wifiClient.connected()) {
            static bool logClientActive = false;
            if (!logClientActive) {
                WiFiClient newClient = wifiServer.accept();
                if (newClient) {
                    wifiClient = newClient;
                    logClientActive = true;
                    ESP_LOGI(taskName(), "Client connected.");
                    esp_log_set_vprintf(&telnet_vprintf);
#if defined(ORD_NAME) && defined(ORD_VERSION) && defined(BUILDTAG) && defined(BUILD_TIMESTAMP)
                    wifiClient.printf("=== Welcome to %s v%s%s b%s ===\n",
                                      ORD_NAME, ORD_VERSION, BUILDTAG, BUILD_TIMESTAMP);
#else
                    wifiClient.println("=== Welcome ===");
#endif
                }
            } else {
                wifiClient.stop();
                logClientActive = false;
                esp_log_set_vprintf(&serial_vprintf);
                ESP_LOGI(taskName(), "Client disconnected.");
            }
        } else if (wifiClient.available()) {
            String line = wifiClient.readStringUntil('\n');
            // Echo back any received data if echo is enabled
            if (echo) {
                // ESP_LOGI(taskName(), "Received from client: %s", line.c_str());
                wifiClient.println(line);
            }
            // Send command to API (non-blocking) and ask for a reply on our queue
            line.trim();
            if (line.length() > 0) {
                bool ok = apiClientQueueCommand(line.c_str());
                if (!ok) {
                    wifiClient.println("Error: API request dropped");
                }
            }
        }

        // Check for API replies destined to this Telnet instance
        apiClientReceiveReplies();
    }

    void receiveReply(const Api::Reply& reply) override {
#ifdef FEATURE_SERIAL
        bool serial = true;
#else
        bool serial = false;
#endif
        bool client = instance && instance->wifiClient && instance->wifiClient.connected();
        if (!client && !serial) {
            return;
        }

        char line[300];
        if (reply.errorCode != Api::ErrorCode::SUCCESS) {
            char errorText[32];
            api.errorCodeToString(reply.errorCode, errorText, sizeof(errorText));
            snprintf(line, sizeof(line), "API [%s] Error (%s): %s",
                     reply.command,
                     errorText,
                     (char*)reply.data);
        } else {
            snprintf(line, sizeof(line), "API [%s] Reply: %s",
                     reply.command,
                     (char*)reply.data);
        }
#ifdef FEATURE_SERIAL
        Serial.println(line);
#endif
        if (client) {
            instance->wifiClient.println(line);
        }
    }

    void setEcho(bool enable) {
        echo = enable;
    }

    static int serial_vprintf(const char* fmt, va_list args) {
        char buf[256];
        vsnprintf(buf, sizeof(buf), fmt, args);
#ifdef FEATURE_SERIAL
        return Serial.print(buf);
#else
        return sizeof(buf);
#endif
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

    void flush() {
#ifdef FEATURE_SERIAL
        Serial.flush();
#endif
        bool client = instance && instance->wifiClient && instance->wifiClient.connected();
        if (client) instance->wifiClient.flush();
    }

   protected:
    static bool parseEchoValue(const char* args, bool* value) {
        while (*args == ' ' || *args == '\t') ++args;

        char token[6] = {};
        size_t length = 0;
        while (args[length] != '\0' && args[length] != ' ' && args[length] != '\t' && args[length] != '\r' && args[length] != '\n') {
            if (length >= sizeof(token) - 1) return false;
            token[length] = args[length];
            ++length;
        }

        const char* rest = args + length;
        while (*rest == ' ' || *rest == '\t' || *rest == '\r' || *rest == '\n') ++rest;
        if (*rest != '\0') return false;

        if (strcmp(token, "1") == 0 || strcmp(token, "True") == 0 || strcmp(token, "true") == 0 ||
            strcmp(token, "On") == 0 || strcmp(token, "on") == 0) {
            *value = true;
            return true;
        }
        if (strcmp(token, "0") == 0 || strcmp(token, "False") == 0 || strcmp(token, "false") == 0 ||
            strcmp(token, "Off") == 0 || strcmp(token, "off") == 0) {
            *value = false;
            return true;
        }
        return false;
    }

    Api::Reply setEchoCommand(const char* args) {
        Api::Reply reply = {};
        bool enable = false;
        if (!parseEchoValue(args, &enable)) {
            reply.errorCode = Api::ErrorCode::INVALID_ARGS;
            snprintf((char*)reply.data, sizeof(reply.data), "Usage: echo 0|1|True|true|False|false|On|on|Off|off");
            return reply;
        }

        setEcho(enable);
        snprintf((char*)reply.data, sizeof(reply.data), "Telnet echo %s", enable ? "enabled" : "disabled");
        return reply;
    }

    // std=GNU++17 allows inline static member variables
    inline static Telnet* instance = nullptr;
    uint16_t port = 23;
    WiFiServer wifiServer{port};
    WiFiClient wifiClient;
    bool echo = false;
};

#endif  // TELNET_H