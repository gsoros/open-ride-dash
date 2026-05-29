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
        // settings.mRequestedCANMode = ACANSettings::LoopBackMode;  // Receive own messages
        // TODO: Setup HW filters
        const uint32_t errorCode = ACAN::can.begin(settings);
        if (errorCode == 0) {
            ESP_LOGD(taskName(), "Init Success");
        } else {
            ESP_LOGE(taskName(), "Init failed, error: 0x%X", errorCode);
        }
        Task::taskSetup();
    }

    virtual void taskRun() override {
        static uint32_t sent = 0;
        static uint32_t received = 0;
        static uint16_t rxBufferOverflows = 0;
        static uint16_t txBufferOverflows = 0;
        uint32_t t = millis();

        if (ACAN::can.driverReceiveBufferPeakCount() >= ACAN::can.driverReceiveBufferSize()) {
            rxBufferOverflows++;
            ACAN::can.resetDriverReceiveBufferPeakCount();
            ESP_LOGW(taskName(), "RX Buffer overflows: %u", rxBufferOverflows);
        }
        if (ACAN::can.driverTransmitBufferPeakCount() >= ACAN::can.driverTransmitBufferSize()) {
            txBufferOverflows++;
            ACAN::can.resetDriverTransmitBufferPeakCount();
            ESP_LOGW(taskName(), "TX Buffer overflows: %u", txBufferOverflows);
        }

        // https://github.com/OpenSourceEBike/EV_Display_Bluetooth_Ant/blob/main/firmware/display/can.c
        // https://github.com/andrey-pr/Bafang_M500_M600-Readme/blob/main/CANBUS/readme.md

        ACANMessage frame;
        uint32_t rxLoop = 0;
        while (ACAN::can.receive(frame)) {
            received += 1;

            switch (frame.id) {
                case 0x01F83100: {
                    // Direct little-endian reconstruction
                    uint16_t rawTorque = (uint16_t)(frame.data[1] << 8) | frame.data[0];
                    state.torque(rawTorque);
                    state.cadence(frame.data[2]);
                    break;
                }
                case 0x02F83201: {
                    state.wheelSpeed_x10((uint16_t)(frame.data[1] << 8) | frame.data[0]);
                    state.batteryCurrent_x20((uint16_t)(frame.data[3] << 8) | frame.data[2]);
                    state.batteryVoltage_x100((uint16_t)(frame.data[5] << 8) | frame.data[4]);
                    state.motorTemp((int8_t)frame.data[6] - 40);
                    state.controllerTemp((int8_t)frame.data[7] - 40);
                    break;
                }
                case 0x02F83203: {
                    state.wheelMaxSpeed_x100((uint16_t)(frame.data[1] << 8) | frame.data[0]);
                    uint8_t highNibble = frame.data[3] >> 4;
                    uint8_t lowNibble = frame.data[2] & 0x0F;
                    state.wheelSize((highNibble * 10) + lowNibble);
                    state.wheelCircumference((uint16_t)(frame.data[5] << 8) | frame.data[4]);
                    break;
                }
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

        // static uint32_t lastLog = 0;
        // if (t - lastLog > 3000) {
        //     lastLog = t;
        //     ESP_LOGD(taskName(), "Sent: %u, Received: %u", sent, received);
        // }
    }

   protected:
};

#endif  // CAN_H