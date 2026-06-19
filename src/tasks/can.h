#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <ACAN_ESP32.h>

#include "config.h"
#include "model/state.h"
#include "task.h"

extern State state;

typedef ACAN_ESP32 ACAN;
typedef CANMessage ACANMessage;
typedef ACAN_ESP32_Settings ACANSettings;

class CAN : public Task {
   public:
    virtual const char* taskName() override {
        return "CAN";
    }

    virtual void setup() {
        ACANSettings settings(250000);  // 250 kb/s, Bafang M500/M600 default
        settings.mTxPin = (gpio_num_t)CAN_TX;
        settings.mRxPin = (gpio_num_t)CAN_RX;
        // settings.mRequestedCANMode = ACANSettings::LoopBackMode;  // Receive own messages
        // TODO: Setup HW filters when all frames are known
        const uint32_t errorCode = ACAN::can.begin(settings);
        if (errorCode != 0) {
            while (true) {
                ESP_LOGE(taskName(), "can.begin() failed, error: 0x%X", errorCode);
                delay(1000);
            }
        }
    }

    virtual void taskRun() override {
        static uint32_t sent = 0;                               // count of sent messages
        static uint32_t received = 0;                           // count of received messages
        static uint16_t rxBufferOverflows = 0;                  // count of receive buffer overflows
        static uint16_t txBufferOverflows = 0;                  // count of transmit buffer overflows
        static uint32_t lastOverflowLog = 0;                    // timestamp of last overflow log
        static constexpr uint32_t overflowLogInterval = 60000;  // overflow log interval in ms
        uint32_t t = millis();

        if (ACAN::can.driverReceiveBufferPeakCount() >= ACAN::can.driverReceiveBufferSize()) {
            rxBufferOverflows++;
            ACAN::can.resetDriverReceiveBufferPeakCount();
            if (t - lastOverflowLog > overflowLogInterval) {
                ESP_LOGW(taskName(), "RX Buffer overflows: %u", rxBufferOverflows);
                lastOverflowLog = t;
            }
        }
        if (ACAN::can.driverTransmitBufferPeakCount() >= ACAN::can.driverTransmitBufferSize()) {
            txBufferOverflows++;
            ACAN::can.resetDriverTransmitBufferPeakCount();
            if (t - lastOverflowLog > overflowLogInterval) {
                ESP_LOGW(taskName(), "TX Buffer overflows: %u", txBufferOverflows);
                lastOverflowLog = t;
            }
        }

        // https://github.com/OpenSourceEBike/EV_Display_Bluetooth_Ant/blob/main/firmware/display/can.c
        // https://github.com/andrey-pr/Bafang_M500_M600-Readme/blob/main/CANBUS/readme.md

        ACANMessage frame;

        while (ACAN::can.receive(frame)) {
            received++;
            /*
            static uint16_t duplicateCount = 0;
            static uint32_t lastFrameId = 0;
            static uint8_t lastFrameData[8] = {};
            if (frame.id == lastFrameId && memcmp(frame.data, lastFrameData, frame.len) == 0) {
                duplicateCount++;
                if (duplicateCount % 10 == 0) {
                    char hexbuf[32] = {};
                    hexToStr(hexbuf, sizeof(hexbuf), frame.data, frame.len);
                    ESP_LOGD(taskName(), "Received duplicate frame ID 0x%X, data: [%s], count: %u", frame.id, hexbuf, duplicateCount);
                }
                continue;
            }
            lastFrameId = frame.id;
            memcpy(lastFrameData, frame.data, frame.len);
            */
            switch (frame.id) {
                case 0x02F83000: {  // [4] Uptime heartbeat
                    // fires every 10 seconds, B[0:3] LE uint32 is a tick counter where 1 tick = 10 seconds.
                    static uint32_t lastUptime = 0;
                    uint32_t uptime_sx10 = ((uint32_t)frame.data[3] << 24) | ((uint32_t)frame.data[2] << 16) | ((uint32_t)frame.data[1] << 8) | (uint32_t)frame.data[0];
                    if (uptime_sx10 == lastUptime) break;
                    lastUptime = uptime_sx10;
                    ESP_LOGD(taskName(), "Parsed: uptime: %.2f minutes", uptime_sx10 / 6.0f);
                    break;
                }

                case 0x02F83200: {  // [8] Cadence and torque
                    /*
                    Working theory:

                    Byte   Field              Detail
                    [0]    motor status       0x5B = active, 0x5A = shutting down
                    [1]    sequence counter   +1 every 2 seconds while motor is active, stops when idle
                    [2]    unknown            always 0x00
                    [3]    cadence            RPM
                    [4:5]  torque             750: idle
                    [6:7]  unknown
                    */
                    uint8_t cadence = frame.data[3];
                    uint16_t torque = (((uint16_t)frame.data[5] << 8) | (uint16_t)frame.data[4]) - 750;
                    uint16_t unknown = (((uint16_t)frame.data[7] << 8) | (uint16_t)frame.data[6]);
                    static uint8_t lastCadence = 0;
                    static uint16_t lastTorque = 0;
                    if (cadence == lastCadence && torque == lastTorque) break;
                    lastCadence = cadence;
                    lastTorque = torque;
                    float humanPower = 0.0f;
                    if (state.aquireMutex()) {
                        State::Snapshot s = state.getSnapshot(false);
                        s.cadence = cadence;
                        s.torque = torque;
                        state.setSnapshot(s, false);
                        state.releaseMutex();
                        humanPower = s.humanPower();
                    }
                    ESP_LOGD(taskName(), "Parsed: cadence: %u, raw torque: %u, human power: %.1f, unknown: %u", cadence, torque, humanPower, unknown);
                    break;
                }

                case 0x02F83201: {  // [8] Wheel speed, current, voltage, motor temp, controller temp
                    uint16_t wheelSpeed_x10 = ((uint16_t)frame.data[1] << 8) | (uint16_t)frame.data[0];
                    state.wheelSpeed_x10(wheelSpeed_x10);
                    uint16_t batteryCurrent_x20 = ((uint16_t)frame.data[3] << 8) | (uint16_t)frame.data[2];
                    state.batteryCurrent_x20(batteryCurrent_x20);
                    uint16_t batteryVoltage_x100 = ((uint16_t)frame.data[5] << 8) | (uint16_t)frame.data[4];
                    state.batteryVoltage_x100(batteryVoltage_x100);
                    state.motorTemp((int8_t)frame.data[6] - 40);
                    state.controllerTemp((int8_t)frame.data[7] - 40);
                    char speedBuf[64] = {};
                    snprintf(speedBuf, sizeof(speedBuf),
                             "%.1f, current: %.1f, voltage: %.1f",
                             wheelSpeed_x10 / 10.0f,
                             batteryCurrent_x20 / 20.0f,
                             batteryVoltage_x100 / 100.0f);
                    static char lastSpeedBuf[64] = {};
                    if (strcmp(speedBuf, lastSpeedBuf) != 0) {
                        ESP_LOGD(taskName(), "Parsed: wheel speed: %s", speedBuf);
                        strncpy(lastSpeedBuf, speedBuf, sizeof(lastSpeedBuf));
                    }
                    break;
                }

                case 0x02F83203: {  // [8] Wheel max speed, wheel size, wheel circumference
                    uint16_t wheelMaxSpeed_x100 = ((uint16_t)frame.data[1] << 8) | (uint16_t)frame.data[0];
                    uint8_t highNibble = frame.data[3] >> 4;
                    uint8_t lowNibble = frame.data[2] & 0x0F;
                    uint8_t wheelSize = (highNibble * 10) + lowNibble;
                    uint16_t wheelCircumference = ((uint16_t)frame.data[5] << 8) | (uint16_t)frame.data[4];
                    static uint16_t lastWheelMaxSpeed = 0;
                    static uint8_t lastWheelSize = 0;
                    static uint16_t lastWheelCircumference = 0;
                    if (wheelMaxSpeed_x100 == lastWheelMaxSpeed && wheelSize == lastWheelSize && wheelCircumference == lastWheelCircumference) break;
                    lastWheelMaxSpeed = wheelMaxSpeed_x100;
                    lastWheelSize = wheelSize;
                    lastWheelCircumference = wheelCircumference;
                    state.wheelMaxSpeed_x100(((uint16_t)frame.data[1] << 8) | (uint32_t)frame.data[0]);
                    state.wheelSize(wheelSize);
                    state.wheelCircumference(wheelCircumference);
                    ESP_LOGD(taskName(),
                             "Parsed: wheel max speed: %.1f km/h, wheel size: %d mm, wheel circumference: %d mm",
                             wheelMaxSpeed_x100 / 100.0f, wheelSize, wheelCircumference);
                    break;
                }

                case 0x02F83204: {  // [8] Unknown, every ~50 ms, data: 0x10000000
                    static char last02F83204Hexbuf[32] = {};
                    char hexbuf[32] = {};
                    hexToStr(hexbuf, sizeof(hexbuf), frame.data, frame.len);
                    if (strcmp(hexbuf, last02F83204Hexbuf) == 0) break;
                    ESP_LOGD(taskName(), "Unparsed: ID 0x02F83204 (prob. every ~50 ms), len: %d, data: [%s]", frame.len, hexbuf);
                    strncpy(last02F83204Hexbuf, hexbuf, sizeof(last02F83204Hexbuf));
                    break;
                }

                case 0x02F83206: {  // [4] PAS level
                    // byte[0] matches keepalive pasByte values (0x0b, 0x0d, etc.),
                    // bytes[1:3]  might encode error flags or mode state
                    int8_t newPas = INT8_MIN;
                    switch (frame.data[0]) {
                        case 0x06:  // Walk assist
                            newPas = -1;
                            break;
                        case 0x00:  // Off
                            newPas = 0;
                            break;
                        case 0x0b:
                            newPas = 1;
                            break;
                        case 0x0d:
                            newPas = 2;
                            break;
                        case 0x15:
                            newPas = 3;
                            break;
                        case 0x17:
                            newPas = 4;
                            break;
                        case 0x03:
                            newPas = 5;
                            break;
                        default:
                            ESP_LOGE(taskName(), "Unknown PAS level: 0x%X", frame.data[0]);
                            break;
                    }
                    if (newPas == INT8_MIN) break;
                    static int8_t lastPas = INT8_MIN;
                    if (newPas != lastPas) {
                        ESP_LOGD(taskName(), "Parsed: PAS level: %d%s", newPas,
                                 newPas == 0    ? " (off)"          //
                                 : newPas == -1 ? " (walk assist)"  //
                                                : "");
                        lastPas = newPas;
                    }
                    state.pasLevel(newPas);
                    break;
                }

                case 0x02F83208: {  // [2] ???
                    // High-frequency bursts: raw torque sensor tick stream?
                    // 5-8 frames at ~200ms intervals with values like
                    // 0x2E, 0x30, 0x46, 0x9C that look like inter-pulse timing
                    uint16_t tick = ((uint16_t)frame.data[1] << 8) | (uint16_t)frame.data[0];
                    char hexbuf[32] = {};
                    static char lastHexbuf[32] = {};
                    hexToStr(hexbuf, sizeof(hexbuf), frame.data, frame.len);
                    if (strcmp(hexbuf, lastHexbuf) == 0) {
                        static uint32_t lastLog = 0;
                        static uint16_t numDuplicates = 0;
                        numDuplicates++;
                        if (t - lastLog > 15000) {
                            ESP_LOGD(taskName(), "Received %d duplicate 0x02F83208 frames, tick: %d, data: [%s]", numDuplicates, tick, hexbuf);
                            lastLog = t;
                            numDuplicates = 0;
                        }
                        break;
                    }
                    strncpy(lastHexbuf, hexbuf, sizeof(lastHexbuf));
                    ESP_LOGD(taskName(), "Unparsed: ID 0x02F83208 (%d: torque tick? ), len: %d, data: [%s]",
                             tick, frame.len, hexbuf);
                    break;
                }

                case 0x02F83210: {  // [8] time in motion counter
                    // B[0:3] = constant 0x1AF10635 (serial or session token),
                    // B[4:7] = increments exactly every 1000ms while the wheel is moving,
                    // pauses when stationary. Likely time-in-motion seconds
                    static uint32_t lastTimeInMotion = 0;
                    uint32_t timeInMotion = ((uint32_t)frame.data[7] << 24) | ((uint32_t)frame.data[6] << 16) | ((uint32_t)frame.data[5] << 8) | (uint32_t)frame.data[4];
                    if (timeInMotion == lastTimeInMotion) break;
                    lastTimeInMotion = timeInMotion;
                    ESP_LOGD(taskName(), "Parsed time-in-motion: %d", timeInMotion);
                    break;
                }

                case 0x02F8320E: {  // [8] Odo and trip distance
                    if (frame.len != 8) {
                        char hexbuf[32] = {};
                        hexToStr(hexbuf, sizeof(hexbuf), frame.data, frame.len);
                        ESP_LOGD(taskName(), "ODO wrong len: %d, data: [%s]", frame.len, hexbuf);
                        break;
                    }
                    uint32_t odo_mx10 = ((uint32_t)frame.data[3] << 24) | ((uint32_t)frame.data[2] << 16) | ((uint32_t)frame.data[1] << 8) | (uint32_t)frame.data[0];
                    uint32_t trip_mx10 = ((uint32_t)frame.data[7] << 24) | ((uint32_t)frame.data[6] << 16) | ((uint32_t)frame.data[5] << 8) | (uint32_t)frame.data[4];
                    state.odo_mx10(odo_mx10);
                    state.trip_mx10(trip_mx10);
                    static uint32_t lastOdo = 0;
                    static uint32_t lastTrip = 0;
                    if (odo_mx10 != lastOdo || trip_mx10 != lastTrip) {
                        ESP_LOGD(taskName(), "Parsed: Odometer: %.0f km, trip: %.1f km",
                                 odo_mx10 / 100.0f, trip_mx10 / 100.0f);
                        lastOdo = odo_mx10;
                        lastTrip = trip_mx10;
                    }
                    break;
                }

                default: {
                    if (frame.data[0] == 0 && frame.data[1] == 0 && frame.data[2] == 0 && frame.data[3] == 0) break;  // Ignore empty frames

                    char hexbuf[32] = {};
                    hexToStr(hexbuf, sizeof(hexbuf), frame.data, frame.len);
                    static char lastHexbuf[32] = {};
                    static uint16_t repeatedCount = 0;
                    static uint32_t lastRepeatLog = 0;
                    if (strcmp(hexbuf, lastHexbuf) == 0) {
                        repeatedCount++;
                        if (lastRepeatLog == 0 || millis() - lastRepeatLog > 10000) {  // Log repeated frames after 10 seconds
                            ESP_LOGD(taskName(), "Unknown frame repeated %d times ID: 0x%08X len=%d Data: [%s]",
                                     repeatedCount, frame.id, frame.len, hexbuf);
                            repeatedCount = 0;  // Reset counter
                            lastRepeatLog = millis();
                        }
                        break;
                    } else {
                        repeatedCount = 0;  // Reset counter on new frame
                        lastRepeatLog = 0;  // Reset log timer
                    }
                    strncpy(lastHexbuf, hexbuf, sizeof(lastHexbuf));
                    ESP_LOGD(taskName(), "Unknown frame ID: 0x%08X len=%d Data: [%s]", frame.id, frame.len, hexbuf);
                    break;
                }
            }
        }

    kepalive: {
        t = millis();
        static uint32_t lastKeepalive = 0;
        static ACANMessage keepalive;
        static bool keepaliveInitialized = false;

        // One-time setup of the static message frame structure
        if (!keepaliveInitialized) {
            keepalive.ext = true;       // Extended frame (29-bit identifier)
            keepalive.id = 0x03106300;  // Bafang M5xx M6xx Magic ID
            keepalive.data[0] = 0x05;   // Total assist levels configured
            ;                           // data[1] set dynamically below
            keepalive.data[2] = 0x00;   // Bit 0: Light state, Bit 1: Up/Down state
            keepalive.data[3] = 0x01;   // Bit 0: ON/OFF master state
            keepalive.len = 4;
            keepaliveInitialized = true;
        }

        // Send keepalive message every ~100ms
        if (t - lastKeepalive > 100) {
            uint8_t pasByte = 0x00;
            switch (state.pasLevelRequested()) {
                case -1:  // Walk assist
                    pasByte = 0x06;
                    break;
                case 0:
                    pasByte = 0x00;
                    break;
                case 1:
                    pasByte = 0x0b;
                    break;
                case 2:
                    pasByte = 0x0d;
                    break;
                case 3:
                    pasByte = 0x15;
                    break;
                case 4:
                    pasByte = 0x17;
                    break;
                case 5:
                    pasByte = 0x03;
                    break;
                default:
                    pasByte = 0x00;
                    break;
            }

            keepalive.data[1] = pasByte;

            if (!ACAN::can.tryToSend(keepalive)) {
                static uint32_t lastSendFailLog = 0;
                static uint32_t lastSendFailCount = 0;
                lastSendFailCount++;
                if (t - lastSendFailLog > 60000) {
                    ESP_LOGW(taskName(), "Failed to send %u keepalive messages", lastSendFailCount);
                    lastSendFailCount = 0;
                    lastSendFailLog = t;
                }
            } else {
                sent += 1;
                lastKeepalive = t;
            }
        }
    }
    }

   protected:
    void hexToStr(char* buf, size_t bufSize,
                  const uint8_t* data, size_t dataSize) {
        if (bufSize == 0) return;
        if (dataSize > 0 && bufSize < dataSize * 3) return;

        static const char hex[] = "0123456789ABCDEF";

        char* p = buf;

        for (size_t i = 0; i < dataSize; ++i) {
            *p++ = hex[data[i] >> 4];
            *p++ = hex[data[i] & 0x0F];

            if (i + 1 < dataSize)
                *p++ = ' ';
        }

        *p = '\0';
    }
};

#endif  // CAN_H