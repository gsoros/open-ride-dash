#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include "has_preferences.h"

class State : public HasPreferences {
   public:
    struct Snapshot {
        // TODO: Keep the order of these fields or use __attribute__((packed)) ?
        char hostname[32] = {};                               // Device hostname (shared by WiFi, BLE, OTA)
        uint32_t blePassKey = 0;                              // Passkey used for BLE pairing/bonding
        uint32_t odo_mx10 = 0;                                // m * 10
        uint32_t trip_mx10 = 0;                               // m * 10
        uint16_t torque = 0;                                  // Raw sensor reading (needs calibration factor to become Nm)
        uint16_t speed_x100 = 0;                              // km/h * 100
        uint16_t maxAssistSpeed_x100 = 0;                     // RPM * 100
        uint16_t batteryVoltage_x100 = 0;                     // V * 100
        uint16_t batteryCurrent_x100 = 0;                     // A * 100
        uint16_t wheelSize_x10 = 0;                           // inches * 10
        uint16_t wheelCircumference = 0;                      // mm
        uint16_t batteryCapacity = DEFAULT_BATTERY_CAPACITY;  // Wh
        int16_t ota = OTA_IDLE;                               // OTA state (-4 = idle, -3 = start, -2 = end, -1 = error, 0-100 = progress)
        int8_t pasLevelRequested = 0;                         // Pedal Assist Level (-1 walk assist, 0 off, 1-5 PAS) requested by UI
        int8_t pasLevel = 0;                                  // Actual PAS level updated by CAN
        uint8_t cadence = 0;                                  // RPM
        uint8_t motorTemp = 0;                                // °C
        uint8_t controllerTemp = 0;                           // °C
        bool controllerAlive = false;                         // true if motor controller is reachable
        uint8_t heartRate = 0;                                // BPM, pushed from phone via CTS HR char (0 = none)

        // Calculates speed in km/h
        float speed();

        // Calculates motor power in Watts
        float motorPower();

        // Calculates human mechanical power in Watts
        float humanPower();

        /* Estimates live battery state of charge in %
         * assumes minVoltage = 3.3V, maxVoltage = 4.2V, 13s pack
         * assumes a constant sag factor (10A pack discharge results in 0.03V drop per cell)
         * uses a typical INR21700-50E discharge curve
         * does not consider pack temperature
         * uses a simple exponential moving average filter
         */
        float soc();

        /* Calculates live range estimation in km
         * uses a simple exponential moving average filter
         */
        float range();
    };

    State();

    void setup();
    bool restoreFromPreferences();
    void registerApiCommands();

    void blePassKey(uint32_t v);
    uint32_t blePassKey();
    void odo_mx10(uint32_t v);
    uint32_t odo_mx10();
    void trip_mx10(uint32_t v);
    uint32_t trip_mx10();
    void pasLevelRequested(int8_t l);
    int8_t pasLevelRequested();
    void pasLevel(int8_t l);
    int8_t pasLevel();
    void torque(uint16_t v);
    uint16_t torque();
    void cadence(uint8_t v);
    uint8_t cadence();
    void batteryCapacity(uint16_t v, bool persist = true);
    uint16_t batteryCapacity();
    void ota(int16_t v);
    int16_t ota();
    void speed_x100(uint16_t v);
    uint16_t speed_x100();
    void batteryCurrent_x100(uint16_t v);
    uint16_t batteryCurrent_x100();
    void batteryVoltage_x100(uint16_t v);
    uint16_t batteryVoltage_x100();
    void motorTemp(uint8_t v);
    uint8_t motorTemp();
    void controllerTemp(uint8_t v);
    uint8_t controllerTemp();
    void maxAssistSpeed_x100(uint16_t v);
    uint16_t maxAssistSpeed_x100();
    void wheelSize_x10(uint16_t v);
    uint16_t wheelSize_x10();
    void wheelCircumference(uint16_t v);
    uint16_t wheelCircumference();
    void controllerAlive(bool v);
    bool controllerAlive();
    void heartRate(uint8_t v);
    uint8_t heartRate();
    void hostname(const char* v);
    const char* hostname();

    bool acquireMutex();
    void releaseMutex();
    Snapshot getSnapshot(bool withMutex = true);
    void setSnapshot(Snapshot s, bool withMutex = true);

    // Battery cell voltage range in Volts
    static constexpr float BATTERY_CELL_VOLTAGE_MIN = 3.3f;
    static constexpr float BATTERY_CELL_VOLTAGE_MAX = 4.2f;

    // Battery pack series cell count
    static constexpr uint8_t BATTERY_PACK_CELL_COUNT = 13;

    // Raw sensor value at rest: 750 counts
    static constexpr int16_t TORQUE_REST_RAW = 750;
    static constexpr int16_t TORQUE_OFFSET = TORQUE_REST_RAW * -1;

    // Value with an 18.46 kg mass hanging from a horizontal 170 mm crank: 1875 counts.
    // Raw delta = 1875 - 750 = 1125 counts
    // Torque = 18.46 * 9.80665 * 0.170 = 30.78 Nm
    // factor = 1125 / 30.78 = 36.55 counts/Nm
    static constexpr float TORQUE_NM_FACTOR = 36.55f;

    static constexpr int8_t PAS_OFF = 0;
    static constexpr int8_t PAS_WALK_ASSIST = -1;

    static constexpr int8_t OTA_IDLE = -4;
    static constexpr int8_t OTA_START = -3;
    static constexpr int8_t OTA_END = -2;
    static constexpr int8_t OTA_ERROR = -1;

   protected:
    SemaphoreHandle_t mutex = nullptr;
    Snapshot _latest;

    static constexpr uint16_t DEFAULT_BATTERY_CAPACITY = 720;  // Wh

    void setInt8(int8_t* i, int8_t v);
    void setUInt8(uint8_t* i, uint8_t v);
    void setInt16(int16_t* i, int16_t v);
    void setUInt16(uint16_t* i, uint16_t v);
    void setInt32(int32_t* i, int32_t v);
    void setUInt32(uint32_t* i, uint32_t v);
    void setFloat(float* f, float v);
    void setBool(bool* b, bool v);

    int8_t getInt8(int8_t* i);
    uint8_t getUInt8(uint8_t* i);
    int16_t getInt16(int16_t* i);
    uint16_t getUInt16(uint16_t* i);
    int32_t getInt32(int32_t* i);
    uint32_t getUInt32(uint32_t* i);
    float getFloat(float* f);
    bool getBool(bool* b);

    static constexpr const char* tag = "State";
};

#endif  // STATE_H
