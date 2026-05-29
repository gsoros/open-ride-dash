#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <ACAN_ESP32.h>

#include "pins.h"
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
        ACANSettings settings(250000);
        settings.mTxPin = (gpio_num_t)CAN_TX;
        settings.mRxPin = (gpio_num_t)CAN_RX;
        // settings.mRequestedCANMode = ACANSettings::LoopBackMode; // Receive own messages
        const uint32_t errorCode = ACAN::can.begin(settings);
        if (errorCode == 0) {
            ESP_LOGD(taskName(), "Init Success");
        } else {
            ESP_LOGE(taskName(), "Init failed, error: 0x%X\n", errorCode);
        }
        Task::taskSetup();
    }

    virtual void taskRun() override {
        static uint32_t sent = 0;
        static uint32_t received = 0;
        ACANMessage frame;

        uint32_t t = millis();
        // Drain the receive buffer so it doesn't overflow
        while (ACAN::can.receive(frame)) {
            received += 1;

            static uint32_t lastReceiveLog = 0;
            if (t - lastReceiveLog < 200) continue;
            lastReceiveLog = t;

            // Mask out the lower bytes to group IDs by source component
            uint32_t baseId = frame.id & 0xFF000000;

            // Filter for target packets
            // The prefix 0x02 typically designates real-time data and
            // contains speed, cadences, current, and error codes
            if (baseId == 0x02000000) {
                // data[0] and data[1] usually contain wheel speed or RPM.
                ESP_LOGI("CAN telemtry", "ID: 0x%08X Data: %02X %02X %02X %02X",
                         frame.id, frame.data[0], frame.data[1], frame.data[2], frame.data[3]);
            }
            // The 0x2f prefix indicates parameter query/response blocks and status polling.
            // The motor and the battery use these to negotiate handshake tokens,
            // firmware versions, and SoC.
            if (baseId == 0x2f000000) {
                ESP_LOGI("CAN parameter", "ID: 0x%08X Data: %02X %02X %02X %02X",
                         frame.id, frame.data[0], frame.data[1], frame.data[2], frame.data[3]);
            }
        }

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
            switch (state.pasLevel()) {
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

            if (ACAN::can.tryToSend(keepalive)) {
                sent += 1;
                lastKeepalive = t;
            }
        }

        static uint32_t lastLog = 0;
        if (t - lastLog > 10000) {
            lastLog = t;
            ESP_LOGD(taskName(), "Sent: %u, Received: %u", sent, received);
        }
    }

   protected:
};

#endif  // CAN_H