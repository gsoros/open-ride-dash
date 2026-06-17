#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include "has_preferences.h"

class State : public HasPreferences {
   public:
    struct Snapshot {
        uint32_t odo_mx10 = 0;                                      // m * 10
        uint32_t trip_mx10 = 0;                                     // m * 10
        uint16_t torque = 0;                                        // Raw sensor reading (needs calibration factor to become Nm)
        uint16_t wheelSpeed_x10 = 0;                                // RPM * 10
        uint16_t wheelMaxSpeed_x100 = 0;                            // RPM * 100
        uint16_t batteryVoltage_x100 = 0;                           // V * 100
        uint16_t batteryCurrent_x20 = 0;                            // A * 20
        uint16_t wheelCircumference = 0;                            // mm
        uint16_t batteryCapacity_Wh = DEFAULT_BATTERY_CAPACITY_Wh;  // Wh
        int8_t pasLevelRequested = 0;                               // Pedal Assist Level (-1 walk assist, 0 off, 1-5 PAS)
        int8_t pasLevel = 0;                                        // - || -
        uint8_t cadence = 0;                                        // RPM
        uint8_t motorTemp = 0;                                      // °C
        uint8_t controllerTemp = 0;                                 // °C
        uint8_t wheelSize = 0;                                      // inches

        // Calculates speed in km/h
        float speed() {
            return (float)wheelSpeed_x10 * (float)wheelCircumference * 0.000006f;
        }

        // Calculates motor power in Watts
        float motorPower() {
            float current = (float)batteryCurrent_x20 / 20.0f;
            float voltage = (float)batteryVoltage_x100 / 100.0f;
            float power = current * voltage;

            static uint32_t lastLog = 0;
            if (millis() - lastLog > 1000) {
                ESP_LOGD(tag, "motorPower()Current: %.1f, Voltage: %.1f, Power: %.1f", current, voltage, power);
                lastLog = millis();
            }
            return power;
        }

        // Calculates human mechanical power in Watts
        float humanPower() {
            // TODO: Replace with a real raw-to-Nm calibration factor
            static constexpr float torqueNmFactor = 33.0f;
            // cadence (RPM) * torque (Nm) * 2 * pi / 60 to get Watts
            float p = (float)cadence * (float)torque / torqueNmFactor * 0.104719755f;

            static uint32_t lastLog = 0;
            if (millis() - lastLog > 1000) {
                ESP_LOGD(tag, "humanPower() Cadence: %u, Torque: %u, Power: %.1f", cadence, torque, p);
                lastLog = millis();
            }
            return p;
        }

        // Estimates live battery state of charge in %
        // assumes minVoltage = 3.3V, maxVoltage = 4.2V, 13s pack
        // assumes a constant sag factor (10A pack discharge results in 0.03V drop per cell)
        // uses a typical INR21700-50E discharge curve
        // does not consider pack temperature
        float soc() {
            static constexpr float minCellVoltage = 3.3f;
            static constexpr float maxCellVoltage = 4.2f;
            static constexpr uint8_t numCells = 13;     // 48V nominal pack voltage
            static constexpr float sagFactor = 0.003f;  // Vcell/Apack (10A → 0.03V sag)
            float soc = -1.0f;

            // INR21700-50E discharge curve: {voltage, soc%}
            // Derived from typical 0.2C discharge curve, 3.3–4.2V range
            static constexpr float curve[][2] = {
                {4.20f, 100.0f},  // surface charge effect, cell "relaxes" after a full charge top-up
                {4.10f, 100.0f},  // flat — no interpolation artifact, just 100% across the top
                {4.05f, 85.0f},
                {4.00f, 79.0f},
                {3.95f, 73.0f},
                {3.90f, 67.0f},
                {3.85f, 61.0f},
                {3.80f, 55.0f},
                {3.75f, 49.0f},
                {3.70f, 43.0f},
                {3.65f, 36.0f},
                {3.60f, 28.0f},
                {3.55f, 20.0f},
                {3.50f, 13.0f},
                {3.45f, 7.0f},
                {3.40f, 3.0f},
                {3.35f, 1.0f},
                {3.30f, 0.0f},
            };
            static constexpr size_t curveLen = sizeof(curve) / sizeof(curve[0]);

            float packVoltage = (float)batteryVoltage_x100 / 100.0f;
            float current = (float)batteryCurrent_x20 / 20.0f;

            // Compensate for voltage sag: estimate open-circuit voltage
            float cellVoltage = (packVoltage / numCells) + (current * sagFactor);

            // Clamp to curve bounds
            if (soc < 0.0f && cellVoltage >= curve[0][0]) soc = curve[0][1];
            if (soc < 0.0f && cellVoltage <= curve[curveLen - 1][0]) soc = curve[curveLen - 1][1];

            // Linear interpolation between curve points
            if (soc < 0.0f) {
                for (size_t i = 0; i < curveLen - 1; i++) {
                    float vHi = curve[i][0];
                    float vLo = curve[i + 1][0];
                    if (cellVoltage <= vHi && cellVoltage >= vLo) {
                        float t = (cellVoltage - vLo) / (vHi - vLo);
                        soc = curve[i + 1][1] + t * (curve[i][1] - curve[i + 1][1]);
                    }
                }
            }

            static uint32_t lastLog = 0;
            if (millis() - lastLog > 1000) {
                ESP_LOGD(tag, "soc() Cell voltage: %.1f, SOC: %.1f%%", cellVoltage, soc);
                lastLog = millis();
            }

            return soc < 0.0f ? 0.0f :  //
                       soc > 100.0f ? 100.0f
                                    : soc;
        }

        // Calculates live range estimation in km
        // TODO: consider adding a moving average filter to smooth out the output
        float range() {
            float speed_kmph = speed();
            float soc_pct = soc();
            float humanP_W = humanPower();
            float motorP_W = motorPower();
            float range = -1.0f;

            if (range < 0.0f && speed_kmph < 0.5f || motorP_W <= 0.0f) range = 0.0f;

            float remainingEnergy_Wh = batteryCapacity_Wh * (soc_pct / 100.0f);

            // Efficiency: Wh per km (motor only — human power is free)
            float motorEfficiency_WhPerKm = motorP_W / speed_kmph;

            // Guard against insane values (e.g. spinning wheel stationary)
            if (motorEfficiency_WhPerKm > 50.0f || motorEfficiency_WhPerKm < 0.5f)
                range = 0.0f;

            // No need to guard against divide by zero, < 0.5f is already checked
            if (range < 0.0f) range = remainingEnergy_Wh / motorEfficiency_WhPerKm;

            static uint32_t lastLog = 0;
            if (millis() - lastLog > 1000) {
                ESP_LOGD(tag, "range() Speed: %.1f km/h, Motor power: %.1f W, Human power: %.1f W, Remaining energy: %.1f Wh, Range: %.1f km",
                         speed_kmph, motorP_W, humanP_W, remainingEnergy_Wh, range);
                lastLog = millis();
            }

            return range;
        }
    };

    State();

    void setup();

    bool restoreFromPreferences();

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
    void batteryCapacity_Wh(uint16_t v, bool persist = true);
    uint16_t batteryCapacity_Wh();

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

    static constexpr uint16_t DEFAULT_BATTERY_CAPACITY_Wh = 720;

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
