#ifndef CAN_H
#define CAN_H

#include <Arduino.h>
#include <ACAN_ESP32.h>

#include "config.h"
#include "task.h"

typedef ACAN_ESP32 ACAN;
typedef CANMessage ACANMessage;
typedef ACAN_ESP32_Settings ACANSettings;

class CAN : public Task {
   public:
    virtual const char* taskName() const override;
    virtual void setup();
    virtual void taskRun() override;

   protected:
    void hexToStr(char* buf, size_t bufSize,
                  const uint8_t* data, size_t dataSize);
};

#endif  // CAN_H