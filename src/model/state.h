#ifndef STATE_H
#define STATE_H

#include <Arduino.h>

class State {
   public:
    struct Snapshot {
        float speed = 0.0f;    // km/h
        uint8_t pasLevel = 0;  // Pedal Assist Level (0-5)

        float batteryLevel = 0.0f;    // SOC percentage
        float batteryVoltage = 0.0f;  // V
        float batteryCurrent = 0.0f;  // A

        float motorPower = 0.0f;        // W
        float motorTemperature = 0.0f;  // °C
    };

    State();

    Snapshot getSnapshot();

    void setup();

    void registerApiCommands();

    void setSpeed(float v);
    float getSpeed();

    void setPASLevel(uint8_t l);
    uint8_t getPASLevel();

    void setBatteryLevel(float v);
    float getBatteryLevel();

    void setBatteryVoltage(float v);
    float getBatteryVoltage();

    void setBatteryCurrent(float v);
    float getBatteryCurrent();

    void setMotorPower(float v);
    float getMotorPower();

    void setMotorTemperature(float v);
    float getMotorTemperature();

   private:
    void ensureMutex();

    float speed;
    uint8_t pasLevel;

    float batteryLevel;
    float batteryVoltage;
    float batteryCurrent;

    float motorPower;
    float motorTemperature;

    SemaphoreHandle_t mutex;
};

#endif  // STATE_H