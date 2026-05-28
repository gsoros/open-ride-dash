#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <ACAN_ESP32.h>

#include "pins.h"
#include "model/state.h"
#include "task.h"

extern State state;

class CAN : public Task {
   public:
    virtual const char* taskName() override {
        return "CAN";
    }

    virtual void setup() {
        ACAN_ESP32_Settings settings(250000);
        settings.mTxPin = (gpio_num_t)CAN_TX;
        settings.mRxPin = (gpio_num_t)CAN_RX;
        settings.mRequestedCANMode = ACAN_ESP32_Settings::LoopBackMode;
        const uint32_t errorCode = ACAN_ESP32::can.begin(settings);
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
        CANMessage frame;
        if (ACAN_ESP32::can.tryToSend(frame)) {
            sent += 1;
        }
        while (ACAN_ESP32::can.receive(frame)) {
            received += 1;
        }
        static ulong last = 0;
        if (millis() - last > 10000) {
            last = millis();
            ESP_LOGD(taskName(), "Sent: %u, Received: %u", sent, received);
        }
    }

   protected:
};

#endif  // CAN_H