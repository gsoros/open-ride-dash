#include "state.h"
#include "tasks/api.h"
#include "config.h"
#include "util.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#ifdef FEATURE_SIM
#include "tasks/sim.h"
extern Sim sim;  // for sim.isEnabled()
#endif

float State::Snapshot::speed() {
    return (float)speed_x100 / 100.0f;
}

float State::Snapshot::motorPower() {
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

float State::Snapshot::humanPower() {
    // power (W) = cadence (RPM) * torque (Nm) * 2 * pi / 60
    float power = (float)cadence * (float)(torque + TORQUE_OFFSET) / TORQUE_NM_FACTOR * 0.104719755f;

    // Log while debugging the mysterious "power" bug, unless sim is enabled
    static bool shouldLog = true;
#ifdef FEATURE_SIM
    if (sim.isEnabled()) shouldLog = false;
#endif
    if (shouldLog) {
        static float lastPower = 0.0f;
        if (lastPower != power) {
            ESP_LOGD(tag, "humanPower() Cadence: %u, Torque: %u, Power: %.1f", cadence, torque, power);
            lastPower = power;
        }
    }

    // Simple exponential moving average
    static float filtered = -1.0f;  // uninitialised sentinel
    constexpr float alpha = 0.2f;   // smoothing factor (0 < alpha ≤ 1)
    if (filtered < 0.0f) {
        filtered = power;  // first call: seed the filter
    } else {
        filtered = alpha * power + (1.0f - alpha) * filtered;
    }

    return filtered;
}

float State::Snapshot::soc() {
    constexpr float sagFactor = 0.003f;  // Vcell/Apack (10A → 0.03V sag)
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
    float cellVoltage = (packVoltage / BATTERY_PACK_CELL_COUNT) + (current * sagFactor);

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

float State::Snapshot::range() {
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

State::State() {
    mutex = xSemaphoreCreateMutex();
}

void State::setup() {
    if (!preferencesSetup("state"))
        ESP_LOGE(tag, "preferencesSetup() failed");
    if (!restoreFromPreferences())
        ESP_LOGE(tag, "restoreFromPreferences() failed");
    registerApiCommands();
    ESP_LOGD(tag, "Initialized");
}

bool State::restoreFromPreferences() {
    if (!preferencesReady) {
        ESP_LOGE(tag, "restoreFromPreferences() preferences not ready");
        return false;
    }
    uint16_t capacity = (uint16_t)preferences.getUInt("batteryCapacity", DEFAULT_BATTERY_CAPACITY);
    ESP_LOGD(tag, "restoreFromPreferences() batteryCapacity: %u", capacity);
    if (capacity < 10 || capacity > 10000) {
        ESP_LOGW(tag, "restoreFromPreferences() batteryCapacity out of range, using default %u", DEFAULT_BATTERY_CAPACITY);
        capacity = DEFAULT_BATTERY_CAPACITY;
    }
    batteryCapacity(capacity, false);

    String storedHostname = preferences.getString("hostname", DEFAULT_HOSTNAME);
    hostname(storedHostname.c_str());
    ESP_LOGD(tag, "restoreFromPreferences() hostname: %s", _latest.hostname);

    return true;
}

void State::registerApiCommands() {
    api.registerCommand(
        "state",
        [this](const char* args) {
            Api::Reply reply = {};
            State::Snapshot s = getSnapshot();
            snprintf((char*)reply.data, sizeof(reply.data), "{\"speed\":%.2f,\"pas\":%d,\"torque\":%u,\"cadence\":%u,\"current\":%.1f,\"voltage\":%.1f,\"motor\":%uC,\"contr\":%uC,\"wheelM\":%.1f,\"wheelSi\":%.1f,\"wheelC\":%u,\"heartRate\":%u}",
                     s.speed(),
                     s.pasLevelRequested,
                     s.torque,
                     s.cadence,
                     s.batteryCurrent_x100 / 100.0f,
                     s.batteryVoltage_x100 / 100.0f,
                     s.motorTemp,
                     s.controllerTemp,
                     s.maxAssistSpeed_x100 / 100.0f,
                     s.wheelSize_x10 / 10.0f,
                     s.wheelCircumference,
                     s.heartRate);
            return reply;
        },
        "Usage: state\nShows some current values.");
}

void State::blePassKey(uint32_t v) {
    setUInt32(&_latest.blePassKey, v);
}
uint32_t State::blePassKey() {
    return getUInt32(&_latest.blePassKey);
}
void State::odo_mx10(uint32_t v) {
    setUInt32(&_latest.odo_mx10, v);
}
uint32_t State::odo_mx10() {
    return getUInt32(&_latest.odo_mx10);
}
void State::trip_mx10(uint32_t v) {
    setUInt32(&_latest.trip_mx10, v);
}
uint32_t State::trip_mx10() {
    return getUInt32(&_latest.trip_mx10);
}
void State::pasLevelRequested(int8_t l) {
    ESP_LOGD(tag, "pasLevelRequested(%d)", l);
    setInt8(&_latest.pasLevelRequested, l);
}
int8_t State::pasLevelRequested() {
    return getInt8(&_latest.pasLevelRequested);
}
void State::pasLevel(int8_t l) {
    // ESP_LOGD(tag, "pasLevel(%d)", l);
    setInt8(&_latest.pasLevel, l);
}
int8_t State::pasLevel() {
    return getInt8(&_latest.pasLevel);
}
void State::torque(uint16_t v) {
    setUInt16(&_latest.torque, v);
}
uint16_t State::torque() {
    return getUInt16(&_latest.torque);
}
void State::cadence(uint8_t v) {
    setUInt8(&_latest.cadence, v);
}
uint8_t State::cadence() {
    return getUInt8(&_latest.cadence);
}
void State::batteryCapacity(uint16_t v, bool persist) {
    setUInt16(&_latest.batteryCapacity, v);
    if (!persist) return;
    if (!preferencesReady) {
        ESP_LOGE(tag, "batteryCapacity() prefs not ready");
        return;
    }
    preferences.putUInt("batteryCapacity", v);
}
uint16_t State::batteryCapacity() {
    return getUInt16(&_latest.batteryCapacity);
}
void State::ota(int16_t v) {
    setInt16(&_latest.ota, v);
}
int16_t State::ota() {
    return getInt16(&_latest.ota);
}
void State::speed_x100(uint16_t v) {
    setUInt16(&_latest.speed_x100, v);
}
uint16_t State::speed_x100() {
    return getUInt16(&_latest.speed_x100);
}
void State::batteryCurrent_x100(uint16_t v) {
    setUInt16(&_latest.batteryCurrent_x100, v);
}
uint16_t State::batteryCurrent_x100() {
    return getUInt16(&_latest.batteryCurrent_x100);
}
void State::batteryVoltage_x100(uint16_t v) {
    setUInt16(&_latest.batteryVoltage_x100, v);
}
uint16_t State::batteryVoltage_x100() {
    return getUInt16(&_latest.batteryVoltage_x100);
}
void State::motorTemp(uint8_t v) {
    setUInt8(&_latest.motorTemp, v);
}
uint8_t State::motorTemp() {
    return getUInt8(&_latest.motorTemp);
}
void State::controllerTemp(uint8_t v) {
    setUInt8(&_latest.controllerTemp, v);
}
uint8_t State::controllerTemp() {
    return getUInt8(&_latest.controllerTemp);
}

void State::maxAssistSpeed_x100(uint16_t v) {
    setUInt16(&_latest.maxAssistSpeed_x100, v);
}
uint16_t State::maxAssistSpeed_x100() {
    return getUInt16(&_latest.maxAssistSpeed_x100);
}
void State::wheelSize_x10(uint16_t v) {
    setUInt16(&_latest.wheelSize_x10, v);
}
uint16_t State::wheelSize_x10() {
    return getUInt16(&_latest.wheelSize_x10);
}
void State::wheelCircumference(uint16_t v) {
    setUInt16(&_latest.wheelCircumference, v);
}
uint16_t State::wheelCircumference() {
    return getUInt16(&_latest.wheelCircumference);
}

void State::controllerAlive(bool v) {
    ESP_LOGD(tag, "Controller alive: %s", v ? "true" : "false");
    setBool(&_latest.controllerAlive, v);
}
bool State::controllerAlive() {
    return getBool(&_latest.controllerAlive);
}
void State::heartRate(uint8_t v) {
    setUInt8(&_latest.heartRate, v);
}
uint8_t State::heartRate() {
    return getUInt8(&_latest.heartRate);
}

void State::hostname(const char* v) {
    if (v == nullptr) return;
    if (acquireMutex()) {
        Util::copyString(_latest.hostname, sizeof(_latest.hostname), v);
        releaseMutex();
    }
    if (!preferencesReady) {
        ESP_LOGE(tag, "hostname() prefs not ready");
        return;
    }
    preferences.putString("hostname", v);
}
const char* State::hostname() {
    return _latest.hostname;
}

bool State::acquireMutex() {
    return xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE;
}

void State::releaseMutex() {
    xSemaphoreGive(mutex);
}

State::Snapshot State::getSnapshot(bool withMutex) {
    Snapshot s = {};
    if (!withMutex || (withMutex && acquireMutex())) {
        s = _latest;
        if (withMutex) releaseMutex();
    }
    return s;
}

void State::setSnapshot(Snapshot s, bool withMutex) {
    if (!withMutex || (withMutex && acquireMutex())) {
        _latest = s;
        if (withMutex) releaseMutex();
    }
}

void State::setInt8(int8_t* i, int8_t v) {
    if (acquireMutex()) {
        *i = v;
        releaseMutex();
    }
}

void State::setUInt8(uint8_t* i, uint8_t v) {
    if (acquireMutex()) {
        *i = v;
        releaseMutex();
    }
}

void State::setInt16(int16_t* i, int16_t v) {
    if (acquireMutex()) {
        *i = v;
        releaseMutex();
    }
}

void State::setUInt16(uint16_t* i, uint16_t v) {
    if (acquireMutex()) {
        *i = v;
        releaseMutex();
    }
}

void State::setInt32(int32_t* i, int32_t v) {
    if (acquireMutex()) {
        *i = v;
        releaseMutex();
    }
}

void State::setUInt32(uint32_t* i, uint32_t v) {
    if (acquireMutex()) {
        *i = v;
        releaseMutex();
    }
}

void State::setFloat(float* f, float v) {
    if (acquireMutex()) {
        *f = v;
        releaseMutex();
    }
}

void State::setBool(bool* b, bool v) {
    if (acquireMutex()) {
        *b = v;
        releaseMutex();
    }
}

int8_t State::getInt8(int8_t* i) {
    int8_t v = 0;
    if (acquireMutex()) {
        v = *i;
        releaseMutex();
    }
    return v;
}

uint8_t State::getUInt8(uint8_t* i) {
    uint8_t v = 0;
    if (acquireMutex()) {
        v = *i;
        releaseMutex();
    }
    return v;
}

int16_t State::getInt16(int16_t* i) {
    int16_t v = 0;
    if (acquireMutex()) {
        v = *i;
        releaseMutex();
    }
    return v;
}

uint16_t State::getUInt16(uint16_t* i) {
    uint16_t v = 0;
    if (acquireMutex()) {
        v = *i;
        releaseMutex();
    }
    return v;
}

int32_t State::getInt32(int32_t* i) {
    int32_t v = 0;
    if (acquireMutex()) {
        v = *i;
        releaseMutex();
    }
    return v;
}

uint32_t State::getUInt32(uint32_t* i) {
    uint32_t v = 0;
    if (acquireMutex()) {
        v = *i;
        releaseMutex();
    }
    return v;
}

float State::getFloat(float* f) {
    float v = 0.0f;
    if (acquireMutex()) {
        v = *f;
        releaseMutex();
    }
    return v;
}

bool State::getBool(bool* b) {
    bool v = false;
    if (acquireMutex()) {
        v = *b;
        releaseMutex();
    }
    return v;
}
