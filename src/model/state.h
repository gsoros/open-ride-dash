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

/*

enum MetricID {
    METRIC_SPEED,
    METRIC_CADENCE,
    METRIC_PAS,
    METRIC_MOTOR_PWR,
    METRIC_HUMAN_PWR,
    METRIC_VOLTAGE,
    METRIC_SOC,
    METRIC_RANGE,
    METRIC_HEART_RATE,
    METRIC_BODY_TEMP,
    METRIC_COUNT // Keeps track of total metrics
};

// Global struct to hold live data
struct TelemetryData {
    float values[METRIC_COUNT];
    const char* units[METRIC_COUNT];
};
TelemetryData liveData = {
    .units = {"km/h", "rpm", "PAS", "W", "W", "V", "%", "km", "bpm", "°C"}
};

// Define what goes on each page
struct PageLayout {
    MetricID major;
    MetricID minor1;
    MetricID minor2;
};

PageLayout pages[] = {
    { METRIC_SPEED,     METRIC_PAS,       METRIC_SOC },       // Page 1: Standard Cruise
    { METRIC_HUMAN_PWR, METRIC_MOTOR_PWR, METRIC_CADENCE },   // Page 2: Power & Performance
    { METRIC_SPEED,     METRIC_HEART_RATE,METRIC_RANGE }       // Page 3: Fitness & Range
};
uint8_t currentPage = 0;
uint8_t totalPages = sizeof(pages) / sizeof(PageLayout);


*/