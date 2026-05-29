#include "state.h"
#include "tasks/api.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

State::State() {}

void State::setup() {
    registerApiCommands();
}

void State::registerApiCommands() {
    // Example: Register a command to get/set speed
    api.registerCommand(
        "state",
        [this](const char* args) {
            Api::Reply reply = {};
            State::Snapshot s = getSnapshot();
            snprintf((char*)reply.data, sizeof(reply.data), "{\"speed\":%.2f,\"pas\":%d,\"torque\":%u,\"cadence\":%u,\"wheelSp\":%.1f,\"current\":%.1f,\"voltage\":%.1f,\"motor\":%uC,\"contr\":%uC,\"wheelM\":%.1f,\"wheelSi\":%u,\"wheelC\":%u}",
                     s.speed(),
                     s.pasLevel,
                     s.torque,
                     s.cadence,
                     s.wheelSpeed_x10 / 10.0f,
                     s.batteryCurrent_x20 / 20.0f,
                     s.batteryVoltage_x100 / 100.0f,
                     s.motorTemp,
                     s.controllerTemp,
                     s.wheelMaxSpeed_x100 / 100.0f,
                     s.wheelSize,
                     s.wheelCircumference);
            return reply;
        },
        "Usage: state\nShows some current values.");
}

void State::pasLevel(int8_t l) {
    setInt8(&_latest.pasLevel, l);
}
int8_t State::pasLevel() {
    return getInt8(&_latest.pasLevel);
}
void State::torque(uint32_t v) {
    setUInt32(&_latest.torque, v);
}
uint32_t State::torque() {
    return getUInt32(&_latest.torque);
}
void State::cadence(uint8_t v) {
    setUInt8(&_latest.cadence, v);
}
uint8_t State::cadence() {
    return getUInt8(&_latest.cadence);
}

void State::wheelSpeed_x10(uint16_t v) {
    setUInt16(&_latest.wheelSpeed_x10, v);
}
uint16_t State::wheelSpeed_x10() {
    return getUInt16(&_latest.wheelSpeed_x10);
}
void State::batteryCurrent_x20(uint16_t v) {
    setUInt16(&_latest.batteryCurrent_x20, v);
}
uint16_t State::batteryCurrent_x20() {
    return getUInt16(&_latest.batteryCurrent_x20);
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

void State::wheelMaxSpeed_x100(uint16_t v) {
    setUInt16(&_latest.wheelMaxSpeed_x100, v);
}
uint16_t State::wheelMaxSpeed_x100() {
    return getUInt16(&_latest.wheelMaxSpeed_x100);
}
void State::wheelSize(uint8_t v) {
    setUInt8(&_latest.wheelSize, v);
}
uint8_t State::wheelSize() {
    return getUInt8(&_latest.wheelSize);
}
void State::wheelCircumference(uint16_t v) {
    setUInt16(&_latest.wheelCircumference, v);
}
uint16_t State::wheelCircumference() {
    return getUInt16(&_latest.wheelCircumference);
}

State::Snapshot State::getSnapshot() {
    ensureMutex();
    Snapshot s = {};
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        s = _latest;
        xSemaphoreGive(mutex);
    }
    return s;
}

void State::setInt8(int8_t* i, int8_t v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        *i = v;
        xSemaphoreGive(mutex);
    }
}

void State::setUInt8(uint8_t* i, uint8_t v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        *i = v;
        xSemaphoreGive(mutex);
    }
}

void State::setInt16(int16_t* i, int16_t v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        *i = v;
        xSemaphoreGive(mutex);
    }
}

void State::setUInt16(uint16_t* i, uint16_t v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        *i = v;
        xSemaphoreGive(mutex);
    }
}

void State::setInt32(int32_t* i, int32_t v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        *i = v;
        xSemaphoreGive(mutex);
    }
}

void State::setUInt32(uint32_t* i, uint32_t v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        *i = v;
        xSemaphoreGive(mutex);
    }
}

void State::setFloat(float* f, float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        *f = v;
        xSemaphoreGive(mutex);
    }
}

void State::setBool(bool* b, bool v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        *b = v;
        xSemaphoreGive(mutex);
    }
}

int8_t State::getInt8(int8_t* i) {
    ensureMutex();
    int8_t v = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = *i;
        xSemaphoreGive(mutex);
    }
    return v;
}

uint8_t State::getUInt8(uint8_t* i) {
    ensureMutex();
    uint8_t v = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = *i;
        xSemaphoreGive(mutex);
    }
    return v;
}

int16_t State::getInt16(int16_t* i) {
    ensureMutex();
    int16_t v = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = *i;
        xSemaphoreGive(mutex);
    }
    return v;
}

uint16_t State::getUInt16(uint16_t* i) {
    ensureMutex();
    uint16_t v = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = *i;
        xSemaphoreGive(mutex);
    }
    return v;
}

int32_t State::getInt32(int32_t* i) {
    ensureMutex();
    int32_t v = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = *i;
        xSemaphoreGive(mutex);
    }
    return v;
}

uint32_t State::getUInt32(uint32_t* i) {
    ensureMutex();
    uint32_t v = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = *i;
        xSemaphoreGive(mutex);
    }
    return v;
}

float State::getFloat(float* f) {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = *f;
        xSemaphoreGive(mutex);
    }
    return v;
}

bool State::getBool(bool* b) {
    ensureMutex();
    bool v = false;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = *b;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::ensureMutex() {
    if (mutex == nullptr) {
        mutex = xSemaphoreCreateMutex();
    }
}