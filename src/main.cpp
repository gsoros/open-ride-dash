#include <Arduino.h>

#include "config.h"
#include "model/state.h"
#include "tasks/wifi.h"
#include "tasks/ota.h"
#include "tasks/wifiserial.h"
#include "tasks/system_monitor.h"
#include "tasks/api.h"
#include "tasks/ble.h"
#include "tasks/display.h"
#include "tasks/can_sim.h"
#include "tasks/can.h"
#include "tasks/keypad.h"
#include "tasks/alarm.h"

State state;
Api api;
Ble ble;
// CANSim canSim;
CAN can;
Display display;
Wifi wifi;
OTA ota;
WifiSerial wifiSerial(23);
SystemMonitor systemMonitor;
Keypad keypad;
Alarm alarmTask;  // 'alarm' is a reserved word

void setup() {
    esp_log_level_set("*", ESP_LOG_DEBUG);
    ulong t = millis();

#ifdef FEATURE_SERIAL
    Serial.setTxTimeoutMs(0);
    Serial.setDebugOutput(false);
    Serial.begin(115200);
    while (!Serial && millis() - t < 100) delay(10);
    // delay(100);
#endif

    state.setup();

    api.setup();
    api.taskStart(10.0f, 4096);

    ble.setup();
    ble.taskStart(1.0f, 4096);

    display.setup();
    display.taskStart(50.0f, 4096);

    wifi.setup();
    wifi.taskStart(1.0f, 8192);  // AP mode does not work with lower stack size
    wifi.waitForReady();

    if (wifi.isEnabled()) {
        ota.setup();
        ota.taskStart(2.0f, 100.0f, 4096);
    }

    wifiSerial.setup(wifi.isEnabled());
    wifiSerial.taskStart(10.0f, 4096);

    // canSim.setup();
    // canSim.taskStart(100.0f, 2048);

    can.setup();
    can.taskStart(100.0f, 4096);

    keypad.setup();
    keypad.taskStart(100.0f, 2048);

    alarmTask.setup();
    alarmTask.taskStart(100.0f, 2048);

    // static Task* tasksToMonitor[] = {&api, &wifi, &ota, &wifiSerial, &display, &can};
    // systemMonitor.setup(tasksToMonitor, sizeof(tasksToMonitor) / sizeof(tasksToMonitor[0]));
    // systemMonitor.taskStart(10.0f, 4096);
}

void loop() {
    vTaskDelete(nullptr);
}
