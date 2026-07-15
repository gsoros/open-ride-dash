#include "tasks/can.h"

#include "model/state.h"
#include "util.h"

extern State state;

const char* CAN::taskName() const {
    return "CAN";
}

void CAN::setup() {
    ACANSettings settings(250000);  // 250 kb/s, Bafang M500/M600 default
    settings.mTxPin = (gpio_num_t)PIN_CAN_TX;
    settings.mRxPin = (gpio_num_t)PIN_CAN_RX;
    // settings.mRequestedCANMode = ACANSettings::LoopBackMode;  // Receive own messages
    // TODO: Setup HW filters when all frames are identified
    const uint32_t errorCode = ACAN::can.begin(settings);
    if (errorCode != 0) {
        while (true) {
            ESP_LOGE(taskName(), "can.begin() failed, error: 0x%X", errorCode);
            delay(1000);
        }
    }
}

void CAN::taskRun() {
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
            rxBufferOverflows = 0;
        }
    }
    if (ACAN::can.driverTransmitBufferPeakCount() >= ACAN::can.driverTransmitBufferSize()) {
        txBufferOverflows++;
        ACAN::can.resetDriverTransmitBufferPeakCount();
        if (t - lastOverflowLog > overflowLogInterval) {
            ESP_LOGW(taskName(), "TX Buffer overflows: %u", txBufferOverflows);
            lastOverflowLog = t;
            txBufferOverflows = 0;
        }
    }

    // https://github.com/OpenSourceEBike/EV_Display_Bluetooth_Ant/blob/main/firmware/display/can.c
    // https://github.com/andrey-pr/Bafang_M500_M600-Readme/blob/main/CANBUS/readme.md

    ACANMessage frame;

    while (ACAN::can.receive(frame)) {
        received++;
        t = millis();
        /*
        static uint16_t duplicateCount = 0;
        static uint32_t lastFrameId = 0;
        static uint8_t lastFrameData[8] = {};
        if (frame.id == lastFrameId && memcmp(frame.data, lastFrameData, frame.len) == 0) {
            duplicateCount++;
            if (duplicateCount % 10 == 0) {
                char hexbuf[32] = {};
                Util::hexToStr(hexbuf, sizeof(hexbuf), frame.data, frame.len);
                ESP_LOGD(taskName(), "Received duplicate frame ID 0x%X, data: [%s], count: %u", frame.id, hexbuf, duplicateCount);
            }
            continue;
        }
        lastFrameId = frame.id;
        memcpy(lastFrameData, frame.data, frame.len);
        */
        switch (frame.id) {
            case 0x02F022F8: {  // [4] Unknown
                // Unknown, try parsing as uint32
                static uint32_t last = 0;
                uint32_t u = ((uint32_t)frame.data[3] << 24) | ((uint32_t)frame.data[2] << 16) | ((uint32_t)frame.data[1] << 8) | (uint32_t)frame.data[0];
                if (u == last) break;
                last = u;
                ESP_LOGD(taskName(), "Unknown frame 0x02F022F8 parsed as uint32: %u", u);
                break;
            }

            case 0x02F83000: {  // [4] Uptime heartbeat
                // fires every 10 seconds, B[0:3] LE uint32 is a tick counter where 1 tick = 10 seconds.
                static uint32_t lastUptime = 0;
                uint32_t uptime_sx10 = ((uint32_t)frame.data[3] << 24) | ((uint32_t)frame.data[2] << 16) | ((uint32_t)frame.data[1] << 8) | (uint32_t)frame.data[0];
                if (uptime_sx10 == lastUptime) break;
                lastUptime = uptime_sx10;
                static uint32_t lastLog = 0;
                if (lastLog + 60000 < t) {
                    lastLog = t;
                    ESP_LOGD(taskName(), "Parsed uptime: %.2f minutes", uptime_sx10 / 6.0f);
                }
                break;
            }

            case 0x02F83200: {  // [8] Cadence and torque
                                /*
                                Byte   Field              Detail
                                [0]    status             0x5B = active, 0x5A = shutting down
                                [1]    sequence           +1 every 2 seconds, stops when idle
                                [2]    unknown 1          always 0x00
                                [3]    cadence            RPM
                                [4:5]  torque             750: idle
                                [6:7]  unknown 2
                                */
                uint8_t status = frame.data[0];
                uint8_t sequence = frame.data[1];
                uint8_t unknown1 = frame.data[2];
                uint8_t cadence = frame.data[3];
                uint16_t rawTorque = (((uint16_t)frame.data[5] << 8) | (uint16_t)frame.data[4]);
                uint16_t unknown2 = (((uint16_t)frame.data[7] << 8) | (uint16_t)frame.data[6]);
                static uint8_t lastCadence = 0;
                static uint16_t lastTorque = 0;
                if ((int)rawTorque + State::TORQUE_OFFSET < 0) {
                    ESP_LOGW(taskName(), "Parsed raw torque %u is below expected offset %d, ignoring", rawTorque, State::TORQUE_OFFSET);
                    break;
                }
                uint16_t torque = rawTorque + State::TORQUE_OFFSET;
                // Ignore unchanged values
                if (cadence == lastCadence && torque == lastTorque) break;
                // Workaround for periodic drops in torque readings despite steady input,
                // which causes humanPower() to report 0W. If cadence is above 80% of last
                // cadence and torque is below 20% of last torque, ignore the reading for
                // 3 seconds. The origins of this issue are unknown, it may be a bug in
                // the Bafang firmware or a misinterpretation of frame payloads.
                static uint32_t lastValidTorqueTime = 0;
                if (cadence > 0 &&
                    cadence > (uint8_t)(lastCadence * .8f) &&
                    torque < (uint16_t)(lastTorque * .2f) &&
                    t - lastValidTorqueTime < 3000) {
                    ESP_LOGW(taskName(), "Parsed torque %u is below 20%% of last torque %u while cadence delta is %d, ignoring", torque, lastTorque, cadence - lastCadence);
                    break;
                }
                lastValidTorqueTime = t;
                lastCadence = cadence;
                lastTorque = torque;
                float humanPower = 0.0f;
                if (state.acquireMutex()) {
                    State::Snapshot s = state.getSnapshot(false);
                    s.cadence = cadence;
                    s.torque = rawTorque;
                    state.setSnapshot(s, false);
                    state.releaseMutex();
                    humanPower = s.humanPower();
                }
                ESP_LOGD(taskName(),
                         "Parsed cadence: %u, torque: %u, human power: %.1f, "
                         "status: %u, seq: %u, unknown1: %u, unknown2: %u",
                         cadence, torque, humanPower,
                         status, sequence, unknown1, unknown2);
                break;
            }

            case 0x02F83201: {  // [8] Speed, current, voltage, motor temp, controller temp
                uint16_t speed_x100 = ((uint16_t)frame.data[1] << 8) | (uint16_t)frame.data[0];
                uint16_t batteryCurrent_x100 = ((uint16_t)frame.data[3] << 8) | (uint16_t)frame.data[2];
                uint16_t batteryVoltage_x100 = ((uint16_t)frame.data[5] << 8) | (uint16_t)frame.data[4];
                int8_t motorTemp = (int8_t)frame.data[6] - 40;
                int8_t controllerTemp = (int8_t)frame.data[7] - 40;
                if (state.acquireMutex()) {
                    State::Snapshot s = state.getSnapshot(false);
                    uint16_t voltageDelta_x100 = std::abs(s.batteryVoltage_x100 - batteryVoltage_x100);
                    if (s.speed_x100 == speed_x100 &&
                        s.batteryCurrent_x100 == batteryCurrent_x100 &&
                        voltageDelta_x100 <= 10 &&
                        s.motorTemp == motorTemp &&
                        s.controllerTemp == controllerTemp) {
                        // ignore voltage jitter <= 100 mV
                        state.releaseMutex();
                        break;
                    }
                    s.speed_x100 = speed_x100;
                    s.batteryCurrent_x100 = batteryCurrent_x100;
                    s.batteryVoltage_x100 = batteryVoltage_x100;
                    s.motorTemp = motorTemp;
                    s.controllerTemp = controllerTemp;
                    state.setSnapshot(s, false);
                    state.releaseMutex();
                }
                char speedBuf[64] = {};
                snprintf(speedBuf, sizeof(speedBuf),
                         "%.1f, current: %.1f, voltage: %.1f",
                         speed_x100 / 100.0f,
                         batteryCurrent_x100 / 100.0f,
                         batteryVoltage_x100 / 100.0f);
                static char lastSpeedBuf[64] = {};
                if (strcmp(speedBuf, lastSpeedBuf) != 0) {
                    ESP_LOGD(taskName(), "Parsed speed: %s", speedBuf);
                    strncpy(lastSpeedBuf, speedBuf, sizeof(lastSpeedBuf));
                }
                break;
            }

            case 0x02F83203: {  // [8] Max assist speed, wheel size, wheel circumference
                uint16_t maxAssistSpeed_x100 = ((uint16_t)frame.data[1] << 8) | (uint16_t)frame.data[0];
                // 12.4-bit fixed-point integer spread across the byte boundary
                uint16_t wheelSize_x10 = (((frame.data[3] << 4) | (frame.data[2] >> 4)) * 10) + (frame.data[2] & 0x0F);
                uint16_t wheelCircumference = ((uint16_t)frame.data[5] << 8) | (uint16_t)frame.data[4];
                static uint16_t lastMaxAssistSpeed = 0;
                static uint16_t lastWheelSize = 0;
                static uint16_t lastWheelCircumference = 0;
                if (maxAssistSpeed_x100 == lastMaxAssistSpeed && wheelSize_x10 == lastWheelSize && wheelCircumference == lastWheelCircumference) break;
                lastMaxAssistSpeed = maxAssistSpeed_x100;
                lastWheelSize = wheelSize_x10;
                lastWheelCircumference = wheelCircumference;
                state.maxAssistSpeed_x100(maxAssistSpeed_x100);
                state.wheelSize_x10(wheelSize_x10);
                state.wheelCircumference(wheelCircumference);
                ESP_LOGD(taskName(),
                         "Parsed max assist speed: %d km/h, wheel size: %.1f\", wheel circumference: %d mm",
                         maxAssistSpeed_x100 / 100, wheelSize_x10 / 10.0f, wheelCircumference);
                break;
            }

            case 0x02F83204: {  // [1 or 8] Unknown, every ~50 ms, data: 0x10000000
                /*
                 * TODO: We are relying on these "Hello" and "Goodbye" messages
                 * to detect when the controller is alive. For redundancy, we
                 * should also track 0x02F83000 frames, and consider marking
                 * the controller as "dead" if we haven't received those in a
                 * while.
                 */
                bool handled = false;
                if (frame.len == 1) {
                    static bool lastAlive = state.controllerAlive();
                    if (frame.data[0] == 0x00) {
                        if (lastAlive) {
                            ESP_LOGD(taskName(), "Received: Goodbye");
                            state.controllerAlive(false);
                            lastAlive = false;
                        }
                        handled = true;
                    }
                    if (frame.data[0] == 0x01) {
                        if (!lastAlive) {
                            ESP_LOGD(taskName(), "Received: Hello");
                            state.controllerAlive(true);
                            lastAlive = true;
                        }
                        handled = true;
                    }
                }
                if (handled) break;
                static char lastHexbuf[32] = {};
                char hexbuf[32] = {};
                Util::hexToStr(hexbuf, sizeof(hexbuf), frame.data, frame.len);
                if (strcmp(hexbuf, lastHexbuf) == 0) break;
                ESP_LOGD(taskName(), "UnParsed ID 0x02F83204, len: %d, data: [%s]",
                         frame.len, hexbuf);
                strncpy(lastHexbuf, hexbuf, sizeof(lastHexbuf));
                break;
            }

            case 0x02F83206: {  // [4] PAS level
                // byte[0] matches keepalive pasByte values (0x0b, 0x0d, etc.),
                // bytes[1:3]  might encode error flags or mode state
                int8_t newPas = INT8_MIN;
                switch (frame.data[0]) {
                    case 0x06:  // Walk assist
                        newPas = State::PAS_WALK_ASSIST;
                        break;
                    case 0x00:  // Off
                        newPas = State::PAS_OFF;
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
                static int8_t lastPas = INT8_MIN;  // CAN is the only source of PAS level changes
                if (newPas != lastPas) {           // so we can safely ignore the repeated identical values
                    ESP_LOGD(taskName(), "Parsed PAS level: %d%s", newPas,
                             newPas == State::PAS_OFF           ? " (off)"          //
                             : newPas == State::PAS_WALK_ASSIST ? " (walk assist)"  //
                                                                : "");
                    lastPas = newPas;
                    state.pasLevel(newPas);
                }
                break;
            }

            case 0x02F83208: {  // [2] Unknown
                // High-frequency bursts: raw torque sensor tick stream?
                // 5-8 frames at ~200ms intervals with values like
                // 0x2E, 0x30, 0x46, 0x9C that look like inter-pulse timing
                uint16_t tick = ((uint16_t)frame.data[1] << 8) | (uint16_t)frame.data[0];
                char hexbuf[32] = {};
                static char lastHexbuf[32] = {};
                Util::hexToStr(hexbuf, sizeof(hexbuf), frame.data, frame.len);
                if (strcmp(hexbuf, lastHexbuf) == 0) {
                    static uint32_t lastLog = 0;
                    static uint16_t numDuplicates = 0;
                    numDuplicates++;
                    if (t - lastLog > 60000) {
                        ESP_LOGD(taskName(), "Received %d duplicate 0x02F83208 frames, tick: %d, data: [%s]", numDuplicates, tick, hexbuf);
                        lastLog = t;
                        numDuplicates = 0;
                    }
                    break;
                }
                strncpy(lastHexbuf, hexbuf, sizeof(lastHexbuf));
                ESP_LOGD(taskName(), "UnParsed ID 0x02F83208 (%d: torque tick? ), len: %d, data: [%s]",
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
                    Util::hexToStr(hexbuf, sizeof(hexbuf), frame.data, frame.len);
                    ESP_LOGD(taskName(), "ODO wrong len: %d, data: [%s]", frame.len, hexbuf);
                    break;
                }
                uint32_t odo_mx10 = ((uint32_t)frame.data[3] << 24) | ((uint32_t)frame.data[2] << 16) | ((uint32_t)frame.data[1] << 8) | (uint32_t)frame.data[0];
                uint32_t trip_mx10 = ((uint32_t)frame.data[7] << 24) | ((uint32_t)frame.data[6] << 16) | ((uint32_t)frame.data[5] << 8) | (uint32_t)frame.data[4];
                if (state.acquireMutex()) {
                    State::Snapshot s = state.getSnapshot(false);
                    s.odo_mx10 = odo_mx10;
                    s.trip_mx10 = trip_mx10;
                    state.setSnapshot(s, false);
                    state.releaseMutex();
                }
                static uint32_t lastOdo = 0;
                static uint32_t lastTrip = 0;
                if (odo_mx10 != lastOdo || trip_mx10 != lastTrip) {
                    ESP_LOGD(taskName(), "Parsed Odometer: %.0f km, trip: %.1f km",
                             odo_mx10 / 100.0f, trip_mx10 / 100.0f);
                    lastOdo = odo_mx10;
                    lastTrip = trip_mx10;
                }
                break;
            }

            default: {
                if (frame.data[0] == 0 && frame.data[1] == 0 && frame.data[2] == 0 && frame.data[3] == 0) break;  // Ignore empty frames

                char hexbuf[32] = {};
                Util::hexToStr(hexbuf, sizeof(hexbuf), frame.data, frame.len);
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
