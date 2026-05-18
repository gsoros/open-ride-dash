#include "state.h"
#include "tasks/api.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

State::State()
    : speed(0.0f), pasLevel(0), batteryLevel(0.0f), batteryVoltage(0.0f), batteryCurrent(0.0f), motorPower(0.0f), motorTemperature(0.0f), mutex(nullptr) {}

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
                setSpeed(newSpeed);
            }
            snprintf((char*)reply.data, sizeof(reply.data), "%.2f", getSpeed());
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
        s.speed = speed;
        s.pasLevel = pasLevel;
        s.batteryLevel = batteryLevel;
        s.batteryVoltage = batteryVoltage;
        s.batteryCurrent = batteryCurrent;
        s.motorPower = motorPower;
        s.motorTemperature = motorTemperature;
        xSemaphoreGive(mutex);
    }
    return s;
}

void State::setSpeed(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        speed = v;
        xSemaphoreGive(mutex);
    }
}

float State::getSpeed() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = speed;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::setPASLevel(uint8_t l) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        pasLevel = l;
        xSemaphoreGive(mutex);
    }
}

uint8_t State::getPASLevel() {
    ensureMutex();
    uint8_t v = 0;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = pasLevel;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::setBatteryLevel(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        batteryLevel = v;
        xSemaphoreGive(mutex);
    }
}

float State::getBatteryLevel() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = batteryLevel;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::setBatteryVoltage(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        batteryVoltage = v;
        xSemaphoreGive(mutex);
    }
}

float State::getBatteryVoltage() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = batteryVoltage;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::setBatteryCurrent(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        batteryCurrent = v;
        xSemaphoreGive(mutex);
    }
}

float State::getBatteryCurrent() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = batteryCurrent;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::setMotorPower(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        motorPower = v;
        xSemaphoreGive(mutex);
    }
}

float State::getMotorPower() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = motorPower;
        xSemaphoreGive(mutex);
    }
    return v;
}

void State::setMotorTemperature(float v) {
    ensureMutex();
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        motorTemperature = v;
        xSemaphoreGive(mutex);
    }
}

float State::getMotorTemperature() {
    ensureMutex();
    float v = 0.0f;
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        v = motorTemperature;
        xSemaphoreGive(mutex);
    }
    return v;
}
