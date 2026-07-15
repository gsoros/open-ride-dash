#include "tasks/wifiserial.h"

#include "util.h"
#include "build_info.h"  // whoami

extern Api api;

WifiSerial::WifiSerial(uint16_t port) : port(port) {
    instance = this;
}

const char* WifiSerial::taskName() const {
    return "WifiSerial";
}

void WifiSerial::setup(bool withWifi) {
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

void WifiSerial::taskRun() {
    drainLogQueue();

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
            Util::trimInPlace(line);
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

void WifiSerial::receiveReply(const Api::Reply& reply) {
    bool client = instance && instance->wifiClient && instance->wifiClient.connected();
    if (!client) {
        return;
    }

    char line[300];
    if (reply.code != Api::ReplyCode::SUCCESS) {
        snprintf(line, sizeof(line), "API [%s] Error (%s): %s",
                 reply.command,
                 Api::replyCodeToString(reply.code),
                 (char*)reply.data);
    } else {
        snprintf(line, sizeof(line), "API [%s] Reply: %s",
                 reply.command,
                 (char*)reply.data);
    }

    if (client)
        instance->wifiClient.println(line);
}

void WifiSerial::setEcho(bool enable) {
    echo = enable;
}

int WifiSerial::queue_vprintf(const char* fmt, va_list args) {
    if (instance == nullptr || instance->logQueue == nullptr) return 0;
    LogLine line;
    int len = vsnprintf(line.text, sizeof(line.text), fmt, args);
    // Never block the caller here - this can run on any task, including
    // the NimBLE host task. If the queue's full we just drop the line
    // rather than stall whatever called ESP_LOGx.
    xQueueSend(instance->logQueue, &line, 0);
    return len;
}

void WifiSerial::drainLogQueue() {
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

void WifiSerial::flush() {
#ifdef FEATURE_SERIAL
    Serial.flush();
#endif
    bool client = instance && instance->wifiClient && instance->wifiClient.connected();
    if (client) instance->wifiClient.flush();
}

void WifiSerial::disconnectWithNotice(const char* message) {
    if (wifiClient && wifiClient.connected()) {
        wifiClient.println(message);
        wifiClient.flush();
        delay(100);
        wifiClient.stop();
        logClientActive = false;
        ESP_LOGI(taskName(), "Client disconnected after notice.");
    }
}

bool WifiSerial::parseEchoValue(const char* args, bool* value) {
    args = Util::skipWhitespace(args);

    char token[6] = {};
    if (!Util::nextToken(args, token, sizeof(token))) return false;

    const char* rest = Util::skipWhitespace(args);
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

Api::Reply WifiSerial::setEchoCommand(const char* args) {
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