#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#define LED 8

const char* ssid = "_forestWizard";
const char* password = "At4fo6hu";
const char* hostname = "ord-debug";

int serial_vprintf(const char* fmt, va_list args);
int telnet_vprintf(const char* fmt, va_list args);

WiFiServer logServer(23);
WiFiClient logClient;

void setup() {
    const char* tag = "setup";

    esp_log_level_set("*", ESP_LOG_DEBUG);

    Serial.begin(115200);
    Serial.setDebugOutput(false);

    pinMode(LED, OUTPUT);

    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, password);
    ESP_LOGI(tag, "Connecting to WiFi");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(250);
    }
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGW(tag, "WiFi connect failed");
    } else {
        ESP_LOGI(tag, "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
    }

    MDNS.begin(hostname);

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
    ESP_LOGI(tag, "OTA ready");

    logServer.begin();

    esp_log_set_vprintf(&serial_vprintf);

    ESP_LOGI(tag, "Log server ready on port 23");
}

void loop() {
    const char* tag = "loop";
    ArduinoOTA.handle();

    static unsigned long lastClientCheck = 0;
    if (millis() - lastClientCheck > 1000) {
        lastClientCheck = millis();
        if (!logClient.connected()) {
            static bool logClientActive = false;
            if (!logClientActive) {
                WiFiClient newClient = logServer.available();
                if (newClient) {
                    logClient = newClient;
                    logClientActive = true;
                    ESP_LOGI(tag, "Log client connected.");
                    esp_log_set_vprintf(&telnet_vprintf);
                    logClient.println("=== Welcome ===");
                }
            } else {
                logClient.stop();
                logClientActive = false;
                esp_log_set_vprintf(&serial_vprintf);
                ESP_LOGI(tag, "Log client disconnected.");
            }
        }
    }

    static unsigned long lastPrint = 0;
    static bool ledState = LOW;
    if (millis() - lastPrint > (ledState ? 1970 : 30)) {
        lastPrint = millis();
        ledState = !ledState;
        digitalWrite(LED, ledState);
        if (ledState) {
            ESP_LOGD(tag, "LED");
        }
    }
    delay(10);  // Let's not hog the single-core CPU
}

int serial_vprintf(const char* fmt, va_list args) {
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    return Serial.print(buf);
}

int telnet_vprintf(const char* fmt, va_list args) {
    char buf[256];
    int res = 0;
    vsnprintf(buf, sizeof(buf), fmt, args);
    if (logClient && logClient.connected()) {
        res = logClient.print(buf);
    }
    // Serial.print(buf);
    return res;
}
