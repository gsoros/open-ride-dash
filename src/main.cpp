#include <Arduino.h>

#include "config.h"
#include "tasks/blink.h"
#include "tasks/wifi.h"
#include "tasks/ota.h"
#include "tasks/logserver.h"

Blink blink;
Wifi wifi;
OTA ota;
LogServer logServer;

void setup() {
    const char* tag = "setup";

    esp_log_level_set("*", ESP_LOG_DEBUG);

    Serial.begin(115200);
    // Serial.setDebugOutput(false);

    wifi.taskStart(0.1f, 16384);

    wifi.waitForConnection();  // Ensure WiFi is connected before starting other tasks that depend on it

    ota.taskStart(10.0f, 100.0f, 16384);

    logServer.taskStart(1.0f, 16384);

    blink.taskStart(1.0f);
}

void loop() {
    vTaskDelete(nullptr);  // Delete the loop task since we're using FreeRTOS tasks
}
