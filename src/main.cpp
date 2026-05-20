#include <Arduino.h>

#include "config.h"
#include "tasks/blink.h"
#include "tasks/wifi.h"
#include "tasks/ota.h"
#include "tasks/telnet.h"
#include "tasks/system_monitor.h"
#include "tasks/api.h"
#include "tasks/display.h"
#include "model/state.h"

State state;
Api api;
Blink blink;
Display display;
Wifi wifi;
OTA ota;
Telnet telnet(23);
SystemMonitor systemMonitor;

void setup() {
    const char* tag = "setup";

    esp_log_level_set("*", ESP_LOG_DEBUG);

    Serial.begin(115200);
    // Serial.setDebugOutput(false);
    while (!Serial) delay(10);

    state.setup();

    api.setup();
    api.taskStart(10.0f, 4096);

    wifi.setup();
    wifi.taskStart(0.0f, 4096);

    wifi.waitForConnection();

    ota.setHostname(wifi.getHostname());
    ota.setup();
    ota.taskStart(2.0f, 100.0f, 4096);

    telnet.setup();
    telnet.taskStart(10.0f, 4096);

    blink.setup();
    blink.taskStart(1.0f);

    display.setup();
    display.taskStart(1.0f, 4096);

    static Task* tasksToMonitor[] = {&api, &wifi, &ota, &telnet, &blink, &display};
    systemMonitor.setup(tasksToMonitor, sizeof(tasksToMonitor) / sizeof(tasksToMonitor[0]));
    systemMonitor.taskStart(0.05f, 4096);
}

void loop() {
    vTaskDelete(nullptr);
}
