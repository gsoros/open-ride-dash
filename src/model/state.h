#ifndef STATE_H
#define STATE_H

#include <Arduino.h>

class State {
   public:
    struct Snapshot {
        uint32_t odo_mx10 = 0;             // m * 10
        uint32_t trip_mx10 = 0;            // m * 10
        uint16_t torque = 0;               // Raw sensor reading (needs calibration factor to become Nm)
        uint16_t wheelSpeed_x10 = 0;       // RPM * 10
        uint16_t wheelMaxSpeed_x100 = 0;   // RPM * 100
        uint16_t batteryVoltage_x100 = 0;  // V * 100
        uint16_t batteryCurrent_x20 = 0;   // A * 20
        uint16_t wheelCircumference = 0;   // mm
        int8_t pasLevelRequested = 0;      // Pedal Assist Level (-1 walk assist, 0 off, 1-5 PAS)
        int8_t pasLevel = 0;               // - || -
        uint8_t cadence = 0;               // RPM
        uint8_t motorTemp = 0;             // °C
        uint8_t controllerTemp = 0;        // °C
        uint8_t wheelSize = 0;             // inches

        // Calculates speed in km/h
        float speed() {
            return (float)wheelSpeed_x10 * (float)wheelCircumference * 0.000006f;
        }

        // Calculates motor power in Watts
        float motorPower() {
            float current = (float)batteryCurrent_x20 / 20.0f;
            float voltage = (float)batteryVoltage_x100 / 100.0f;
            return current * voltage;
        }

        // Calculates human mechanical power in Watts
        float humanPower() {
            // TODO: Replace with a real raw-to-Nm calibration factor
            static constexpr float torqueNmFactor = 33.0f;
            // cadence (RPM) * torque (Nm) * 2 * pi / 60 to get Watts
            return (float)cadence * (float)torque / torqueNmFactor * 0.104719755f;
        }
    };

    State();

    void setup();

    void registerApiCommands();

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

    void wheelSpeed_x10(uint16_t v);
    uint16_t wheelSpeed_x10();
    void batteryCurrent_x20(uint16_t v);
    uint16_t batteryCurrent_x20();
    void batteryVoltage_x100(uint16_t v);
    uint16_t batteryVoltage_x100();
    void motorTemp(uint8_t v);
    uint8_t motorTemp();
    void controllerTemp(uint8_t v);
    uint8_t controllerTemp();

    void wheelMaxSpeed_x100(uint16_t v);
    uint16_t wheelMaxSpeed_x100();
    void wheelSize(uint8_t v);
    uint8_t wheelSize();
    void wheelCircumference(uint16_t v);
    uint16_t wheelCircumference();

    bool aquireMutex();
    void releaseMutex();
    Snapshot getSnapshot(bool withMutex = true);
    void setSnapshot(Snapshot s, bool withMutex = true);

   protected:
    SemaphoreHandle_t mutex = nullptr;
    Snapshot _latest;

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
};

#endif  // STATE_H
