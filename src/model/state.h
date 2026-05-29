#ifndef STATE_H
#define STATE_H

#include <Arduino.h>

class State {
   public:
    struct Snapshot {
        float speed = 0.0f;   // km/h
        int8_t pasLevel = 0;  // Pedal Assist Level (0-5)

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

    void speed(float v);
    float speed();

    void pasLevel(int8_t l);
    int8_t pasLevel();

    void batteryLevel(float v);
    float batteryLevel();

    void batteryVoltage(float v);
    float batteryVoltage();

    void batteryCurrent(float v);
    float batteryCurrent();

    void motorPower(float v);
    float motorPower();

    void motorTemperature(float v);
    float motorTemperature();

    volatile bool keyUp = false;
    volatile bool keyDown = false;
    volatile bool keyPower = false;

   private:
    void ensureMutex();

    float _speed = 0.0f;
    int8_t _pasLevel = 0;

    float _batteryLevel = 0.0f;
    float _batteryVoltage = 0.0f;
    float _batteryCurrent = 0.0f;

    float _motorPower = 0.0f;
    float _motorTemperature = 0.0f;

    SemaphoreHandle_t mutex = nullptr;
};

#endif  // STATE_H
