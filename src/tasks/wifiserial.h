#ifndef WIFISERIAL_H
#define WIFISERIAL_H

/*
   TODO: Move Serial IO handling to the main loop or the API task.
*/

#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <cstring>
#include "task.h"
#include "api.h"
#include "config.h"
#include "build_info.h"  // whoami

class WifiSerial : public Task, public ApiClient {
   public:
    WifiSerial(uint16_t port = 23) : port(port) {
        instance = this;
    }

    virtual const char* taskName() const override {
        return "WifiSerial";
    }

    virtual void setup(bool withWifi = true) {
        instance = this;
        logQueue = xQueueCreate(8, sizeof(LogLine));

        esp_log_set_vprintf(&queue_vprintf);
        if (withWifi) {
            wifiServer.begin();
            ESP_LOGI(taskName(), "Ready on port %d", port);
        }
        apiClientSetup(taskName());
        api.registerCommand(
            "echo",
            [this](const char* args) {
                return setEchoCommand(args);
            },
            "Usage: echo 0|1|True|true|False|false|On|on|Off|off\nEnables or disables WifiSerial input echo.");
    }

    virtual void taskRun() override {
        drainLogQueue();

#ifdef FEATURE_SERIAL
        static char serialBuf[128] = {};
        static size_t serialBufLen = 0;

        while (Serial.available()) {
            char c = Serial.read();
            Serial.print(c);
            if (c == '\n' || c == '\r') {
                if (serialBufLen > 0) {
                    serialBuf[serialBufLen] = '\0';
                    trimInPlace(serialBuf);
                    if (serialBuf[0] != '\0') {
                        if (!apiClientQueueCommand(serialBuf)) {
                            ESP_LOGE(taskName(), "API request dropped");
                        }
                    }
                    serialBufLen = 0;
                    serialBuf[0] = '\0';
                }
            } else {
                if (serialBufLen < sizeof(serialBuf) - 1) {
                    serialBuf[serialBufLen++] = c;
                } else {
                    ESP_LOGE(taskName(), "Serial buffer overflow");
                    serialBufLen = 0;
                    serialBuf[0] = '\0';
                }
            }
        }

#endif

        if (!wifiClient.connected()) {
            if (!logClientActive) {
                WiFiClient newClient = wifiServer.accept();
                if (newClient) {
                    wifiClient = newClient;
                    logClientActive = true;
                    ESP_LOGI(taskName(), "Client connected.");
                    wifiClient.printf("=== Welcome to %s ===\n", whoami);
                }
            } else {
                wifiClient.stop();
                logClientActive = false;
                ESP_LOGI(taskName(), "Client disconnected.");
            }
        } else if (wifiClient.available()) {
            char line[256] = {};
            int len = wifiClient.readBytesUntil('\n', line, sizeof(line) - 1);
            if (len > 0) {
                line[len] = '\0';
                trimInPlace(line);
                // Echo back any received data if echo is enabled
                if (echo) {
                    wifiClient.println(line);
                }
                // Send command to API (non-blocking) and ask for a reply on our queue
                if (line[0] != '\0') {
                    bool ok = apiClientQueueCommand(line);
                    if (!ok) {
                        wifiClient.println("Error: API request dropped");
                    }
                }
            }
        }

        // Check for API replies destined to this WifiSerial instance
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
        if (reply.code != Api::ReplyCode::SUCCESS) {
            char errorText[32];
            api.replyCodeToString(reply.code, errorText, sizeof(errorText));
            snprintf(line, sizeof(line), "API [%s] Error (%s): %s",
                     reply.command,
                     errorText,
                     (char*)reply.data);
        } else {
            snprintf(line, sizeof(line), "API [%s] Reply: %s",
                     reply.command,
                     (char*)reply.data);
        }
        if (serial)
            Serial.println(line);

        if (client)
            instance->wifiClient.println(line);
    }

    void setEcho(bool enable) {
        echo = enable;
    }

    static void trimInPlace(char* text) {
        if (text == nullptr) return;
        char* start = text;
        while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') ++start;
        if (start != text) {
            size_t len = strlen(start) + 1;
            memmove(text, start, len);
        }
        size_t len = strlen(text);
        while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t' || text[len - 1] == '\r' || text[len - 1] == '\n')) {
            text[--len] = '\0';
        }
    }

    static int queue_vprintf(const char* fmt, va_list args) {
        if (instance == nullptr || instance->logQueue == nullptr) return 0;
        LogLine line;
        int len = vsnprintf(line.text, sizeof(line.text), fmt, args);
        // Never block the caller here - this can run on any task, including
        // the NimBLE host task. If the queue's full we just drop the line
        // rather than stall whatever called ESP_LOGx.
        xQueueSend(instance->logQueue, &line, 0);
        return len;
    }

    void drainLogQueue() {
        if (logQueue == nullptr) return;
        LogLine line;
        // Cap lines drained per loop iteration so a burst of log output
        // can't starve the rest of taskRun().
        for (int i = 0; i < 10 && xQueueReceive(logQueue, &line, 0) == pdTRUE; ++i) {
#ifdef FEATURE_SERIAL
            Serial.print(line.text);
#endif
            if (wifiClient && wifiClient.connected()) {
                wifiClient.print(line.text);
            }
        }
    }

    void flush() {
#ifdef FEATURE_SERIAL
        Serial.flush();
#endif
        bool client = instance && instance->wifiClient && instance->wifiClient.connected();
        if (client) instance->wifiClient.flush();
    }

    void disconnectWithNotice(const char* message) {
        if (wifiClient && wifiClient.connected()) {
            wifiClient.println(message);
            wifiClient.flush();
            delay(100);
            wifiClient.stop();
            logClientActive = false;
            ESP_LOGI(taskName(), "Client disconnected after notice.");
        }
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
            reply.code = Api::ReplyCode::INVALID_ARGS;
            snprintf((char*)reply.data, sizeof(reply.data), "Usage: echo 0|1|True|true|False|false|On|on|Off|off");
            return reply;
        }

        setEcho(enable);
        snprintf((char*)reply.data, sizeof(reply.data), "WifiSerial echo %s", enable ? "enabled" : "disabled");
        return reply;
    }

    // std=GNU++17 allows inline static member variables
    inline static WifiSerial* instance = nullptr;
    uint16_t port = 23;
    WiFiServer wifiServer{port};
    WiFiClient wifiClient;
    bool echo = false;
    bool logClientActive = false;

    struct LogLine {
        // 200 chars covers the usual app log lines with room to
        // spare; bump if something legitimately gets truncated.
        char text[200];
    };
    QueueHandle_t logQueue = nullptr;
};

extern WifiSerial wifiSerial;

#endif  // WIFISERIAL_H