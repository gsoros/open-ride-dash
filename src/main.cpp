#include <Arduino.h>

#include "config.h"
#include "tasks/blink.h"
#include "tasks/wifi.h"
#include "tasks/ota.h"
#include "tasks/telnet.h"
#include "tasks/system_monitor.h"
#include "tasks/api.h"

Api api;
Blink blink;
Wifi wifi;
OTA ota;
Telnet telnet(23);
SystemMonitor systemMonitor;

void setup() {
    const char* tag = "setup";

    esp_log_level_set("*", ESP_LOG_DEBUG);

    Serial.begin(115200);
    // Serial.setDebugOutput(false);

    api.setup();
    api.taskStart(10.0f, 4096);

    wifi.setup();
    wifi.taskStart(0.0f, 4096);

    wifi.waitForConnection();

    ota.setup();
    ota.taskStart(10.0f, 100.0f, 4096);

    telnet.setup();
    telnet.taskStart(10.0f, 4096);

    blink.setup();
    blink.taskStart(1.0f);

    static Task* tasksToMonitor[] = {&api, &wifi, &ota, &telnet, &blink};
    systemMonitor.setup(tasksToMonitor, sizeof(tasksToMonitor) / sizeof(tasksToMonitor[0]));
    systemMonitor.taskStart(0.1f, 4096);
}

void loop() {
    vTaskDelete(nullptr);  // Delete the loop task since we're using FreeRTOS tasks
}
