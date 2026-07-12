#include "state.h"
#include "tasks/api.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

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
    return true;
}

void State::registerApiCommands() {
    // Example: Register a command to get/set speed
    api.registerCommand(
        "state",
        [this](const char* args) {
            Api::Reply reply = {};
            State::Snapshot s = getSnapshot();
            snprintf((char*)reply.data, sizeof(reply.data), "{\"speed\":%.2f,\"pas\":%d,\"torque\":%u,\"cadence\":%u,\"current\":%.1f,\"voltage\":%.1f,\"motor\":%uC,\"contr\":%uC,\"wheelM\":%.1f,\"wheelSi\":%.1f,\"wheelC\":%u}",
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
                     s.wheelCircumference);
            return reply;
        },
        "Usage: state\nShows some current values.");
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
    ESP_LOGD(tag, "pasLevel(%d)", l);
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
    if (persist) {
        if (!preferencesReady) {
            ESP_LOGE(tag, "batteryCapacity() preferences not ready");
            return;
        }
        preferences.putUInt("batteryCapacity", v);
    }
}
uint16_t State::batteryCapacity() {
    return getUInt16(&_latest.batteryCapacity);
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
