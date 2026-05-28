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
        "speed",
        [this](const char* args) {
            Api::Reply reply = {};
            if (args != nullptr && args[0] != '\0') {
                char* endPtr = nullptr;
                float newSpeed = strtof(args, &endPtr);
                if (endPtr == args || (*endPtr != '\0' && *endPtr != ' ' && *endPtr != '\t' && *endPtr != '\r' && *endPtr != '\n')) {
                    reply.errorCode = Api::ErrorCode::INVALID_ARGS;
                    snprintf((char*)reply.data, sizeof(reply.data), "Invalid speed value: %s", args);
                    return reply;
                }
                speed(newSpeed);
            }
            snprintf((char*)reply.data, sizeof(reply.data), "%.2f", speed());
            return reply;
        },
        "Usage: speed [<speed>]\nShows or sets the current speed.");
}

void State::ensureMutex() {
    if (mutex == nullptr) {
        mutex = xSemaphoreCreateMutex();
    }
}

State::Snapshot State::getSnapshot() {
    ensureMutex();
    Snapshot s;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        s.speed = _speed;
        s.pasLevel = _pasLevel;
        s.batteryLevel = _batteryLevel;
        s.batteryVoltage = _batteryVoltage;
        s.batteryCurrent = _batteryCurrent;
        s.motorPower = _motorPower;
        s.motorTemperature = _motorTemperature;
        xSemaphoreGive(mutex);
    }
    return s;
}

void State::speed(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        _speed = v;
        xSemaphoreGive(mutex);
    }
}

float State::speed() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = _speed;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::pasLevel(uint8_t l) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        _pasLevel = l;
        xSemaphoreGive(mutex);
    }
}

uint8_t State::pasLevel() {
    ensureMutex();
    uint8_t v = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = _pasLevel;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::batteryLevel(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        _batteryLevel = v;
        xSemaphoreGive(mutex);
    }
}

float State::batteryLevel() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = _batteryLevel;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::batteryVoltage(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        _batteryVoltage = v;
        xSemaphoreGive(mutex);
    }
}

float State::batteryVoltage() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = _batteryVoltage;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::batteryCurrent(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        _batteryCurrent = v;
        xSemaphoreGive(mutex);
    }
}

float State::batteryCurrent() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = _batteryCurrent;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::motorPower(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        _motorPower = v;
        xSemaphoreGive(mutex);
    }
}

float State::motorPower() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = _motorPower;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::motorTemperature(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        _motorTemperature = v;
        xSemaphoreGive(mutex);
    }
}

float State::motorTemperature() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = _motorTemperature;
        xSemaphoreGive(mutex);
    }
    return v;
}
