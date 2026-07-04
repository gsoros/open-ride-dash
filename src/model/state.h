#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include "has_preferences.h"

class State : public HasPreferences {
   public:
    struct Snapshot {
        uint32_t odo_mx10 = 0;             // m * 10
        uint32_t trip_mx10 = 0;            // m * 10
        uint16_t torque = 0;               // Raw sensor reading (needs calibration factor to become Nm)
        uint16_t wheelSpeed_x10 = 0;       // RPM * 10
        uint16_t wheelMaxSpeed_x100 = 0;   // RPM * 100
        uint16_t batteryVoltage_x100 = 0;  // V * 100
        uint16_t batteryCurrent_x100 = 0;  // A * 100
        uint16_t wheelCircumference = 0;   // mm
        uint16_t batteryCapacity =         // Wh
            DEFAULT_BATTERY_CAPACITY;      // 720 Wh
        int8_t pasLevelRequested = 0;      // Pedal Assist Level (-1 walk assist, 0 off, 1-5 PAS) requested by UI
        int8_t pasLevel = 0;               // Actual PAS level updated by CAN
        uint8_t cadence = 0;               // RPM
        uint8_t motorTemp = 0;             // °C
        uint8_t controllerTemp = 0;        // °C
        uint8_t wheelSize = 0;             // inches
        bool controllerAlive = false;      // true if motor controller is reachable

        // Calculates speed in km/h
        float speed() {
            return (float)wheelSpeed_x10 * (float)wheelCircumference * 0.000006f;
        }

        // Calculates motor power in Watts
        float motorPower() {
            float current = (float)batteryCurrent_x100 / 100.0f;
            float voltage = (float)batteryVoltage_x100 / 100.0f;
            float power = current * voltage;

            /*
            static uint32_t lastLog = 0;
            if (millis() - lastLog > 1000) {
                ESP_LOGD(tag, "motorPower() Current: %.1f, Voltage: %.1f, Power: %.1f", current, voltage, power);
                lastLog = millis();
            }
            */
            return power;
        }

        // Calculates human mechanical power in Watts
        float humanPower() {
            // Raw sensor value at rest: 750 counts.
            // Value with an 18.46 kg mass hanging from a horizontal 170 mm crank: 1875 counts.
            // Raw delta = 1875 - 750 = 1125 counts
            // Torque = 18.46 * 9.80665 * 0.170 = 30.78 Nm
            // torqueNmFactor = 1125 / 30.78 = 36.55 counts/Nm
            constexpr float torqueNmFactor = 36.55f;
            constexpr int16_t torqueOffset = -750;
            // power (W) = cadence (RPM) * torque (Nm) * 2 * pi / 60
            float power = (float)cadence * (float)(torque + torqueOffset) / torqueNmFactor * 0.104719755f;

            // Simple exponential moving average
            static float filtered = -1.0f;  // uninitialised sentinel
            constexpr float alpha = 0.1f;   // smoothing factor (0 < alpha ≤ 1)
            if (filtered < 0.0f) {
                filtered = power;  // first call: seed the filter
            } else {
                filtered = alpha * power + (1.0f - alpha) * filtered;
            }

            /*
            static uint32_t lastLog = 0;
            if (millis() - lastLog > 1000) {
                ESP_LOGD(tag, "humanPower() Cadence: %u, Torque: %u, Power: %.1f", cadence, torque, filtered);
                lastLog = millis();
            }
            */
            return filtered;
        }

        /* Estimates live battery state of charge in %
         * assumes minVoltage = 3.3V, maxVoltage = 4.2V, 13s pack
         * assumes a constant sag factor (10A pack discharge results in 0.03V drop per cell)
         * uses a typical INR21700-50E discharge curve
         * does not consider pack temperature
         * uses a simple exponential moving average filter
         */
        float soc() {
            constexpr float minCellVoltage = 3.3f;
            constexpr float maxCellVoltage = 4.2f;
            constexpr uint8_t numCellsSeries = 13;  // 48V nominal pack voltage
            constexpr float sagFactor = 0.003f;     // Vcell/Apack (10A → 0.03V sag)
            float soc = -1.0f;

            // INR21700-50E discharge curve: {voltage, soc%}
            // Derived from typical 0.2C discharge curve, 3.3–4.2V range
            constexpr float curve[][2] = {
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
            constexpr size_t curveLen = sizeof(curve) / sizeof(curve[0]);

            float packVoltage = (float)batteryVoltage_x100 / 100.0f;
            float current = (float)batteryCurrent_x100 / 100.0f;

            // Compensate for voltage sag: estimate open-circuit voltage
            float cellVoltage = (packVoltage / numCellsSeries) + (current * sagFactor);

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

            // Simple exponential moving average
            static float filtered = -1.0f;  // uninitialised sentinel
            constexpr float alpha = 0.1f;   // smoothing factor (0 < alpha ≤ 1)
            if (filtered < 0.0f) {
                filtered = soc;  // first call: seed the filter
            } else {
                filtered = alpha * soc + (1.0f - alpha) * filtered;
            }

            /*
            static uint32_t lastLog = 0;
            if (millis() - lastLog > 1000) {
                ESP_LOGD(tag, "soc() Cell voltage: %.3f, SOC: %.1f%%", cellVoltage, filtered);
                lastLog = millis();
            }
            */

            return filtered < 0.0f ? 0.0f :  // clamp to 0-100%
                       filtered > 100.0f ? 100.0f
                                         : filtered;
        }

        /* Calculates live range estimation in km
         * uses a simple exponential moving average filter
         */
        float range() {
            float speed_kmph = speed();
            float soc_pct = soc();
            float humanP_W = humanPower();
            float motorP_W = motorPower();
            float rawRange = -1.0f;

            if (rawRange < 0.0f && (speed_kmph < 0.5f || motorP_W <= 0.0f))
                rawRange = 0.0f;

            float remainingEnergy_Wh = batteryCapacity * (soc_pct / 100.0f);
            float motorEfficiency_WhPerKm = motorP_W / speed_kmph;

            if (motorEfficiency_WhPerKm > 50.0f || motorEfficiency_WhPerKm < 0.5f)
                rawRange = 0.0f;

            if (rawRange < 0.0f)
                rawRange = remainingEnergy_Wh / motorEfficiency_WhPerKm;

            // Simple exponential moving average
            static float filteredRange = -1.0f;  // uninitialised sentinel
            constexpr float alpha = 0.1f;        // smoothing factor (0 < alpha ≤ 1)
            if (filteredRange < 0.0f) {
                filteredRange = rawRange;  // first call: seed the filter
            } else {
                filteredRange = alpha * rawRange + (1.0f - alpha) * filteredRange;
            }

            /*
            static uint32_t lastLog = 0;
            if (millis() - lastLog > 1000) {
                ESP_LOGD(tag,
                         "range() Speed: %.1f km/h, Motor: %.1f W, Human: %.1f W, "
                         "Energy: %.1f Wh, Raw: %.1f km, Filtered: %.1f km",
                         speed_kmph, motorP_W, humanP_W, remainingEnergy_Wh,
                         rawRange, filteredRange);
                lastLog = millis();
            }
            */

            return filteredRange;
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
    void batteryCapacity(uint16_t v, bool persist = true);
    uint16_t batteryCapacity();

    void wheelSpeed_x10(uint16_t v);
    uint16_t wheelSpeed_x10();
    void batteryCurrent_x100(uint16_t v);
    uint16_t batteryCurrent_x100();
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

    void controllerAlive(bool v);
    bool controllerAlive();

    bool aquireMutex();
    void releaseMutex();
    Snapshot getSnapshot(bool withMutex = true);
    void setSnapshot(Snapshot s, bool withMutex = true);

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
