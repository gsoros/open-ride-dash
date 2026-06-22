#ifndef ALARM_H
#define ALARM_H

/*
 * Alarm task
 * When armed and motion is detected, a warning is issued.
 * If the warning is not cleared within 1.5 seconds, the alarm is latched until disarmed.
 */

#include <Arduino.h>
#include <Wire.h>
#include <MPU9250.h>

#include "config.h"
#include "task.h"
#include "api.h"

// Global handle to the MPU interrupt target task
TaskHandle_t mpuInterruptTargetTaskHandle = NULL;

// Lightweight, IRAM-safe ISR
// Global as there is only one MPU interrupt handler
void IRAM_ATTR mpuInterruptHandler() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (mpuInterruptTargetTaskHandle != NULL) {
        // Unblock the Alarm task immediately from the interrupt context
        vTaskNotifyGiveFromISR(mpuInterruptTargetTaskHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR();
    }
}

class Alarm : public Task {
   public:
    virtual const char* taskName() override {
        return "Alarm";
    }

    virtual void setup() {
        if (!Wire.begin(I2C_SDA, I2C_SCL)) {  // MPU is the only I2C device on the bus
            ESP_LOGE(taskName(), "Failed to initialize I2C");
        }

        MPU9250Setting s;
        s.skip_mag = true;  // compass not needed
        s.fifo_sample_rate = FIFO_SAMPLE_RATE::SMPL_125HZ;
        // s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_10HZ;
        // s.gyro_dlpf_cfg = GYRO_DLPF_CFG::DLPF_3600HZ;
        // s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_10HZ;
        // s.accel_dlpf_cfg = ACCEL_DLPF_CFG::DLPF_420HZ;
        // s.mag_output_bits = MAG_OUTPUT_BITS::M14BITS;
        if (!_mpu.setup(0x68, s))
            ESP_LOGE(taskName(), "Setup error");
        // device->selectFilter(QuatFilterSel::MAHONY);
        _mpu.selectFilter(QuatFilterSel::NONE);

        _clearMPUInterrupt();

        pinMode(MPU_INT, INPUT);
        attachInterrupt(digitalPinToInterrupt(MPU_INT), mpuInterruptHandler, RISING);

        api.registerCommand(
            "arm", [this](const char* args) { return _armCommand(args); },
            "Usage: arm\nArms the alarm.");
        api.registerCommand(
            "disarm", [this](const char* args) { return _disarmCommand(args); },
            "Usage: disarm\nDisarms the alarm.");
    }

    virtual bool taskStart(float frequency = -1.0f, uint32_t stack = 0, int8_t priority = -1) override {
        if (!Task::taskStart(frequency, stack, priority)) {
            ESP_LOGE(taskName(), "Task::taskStart() failed");
            return false;
        }

        // Save this task's handle so the ISR knows who to notify
        mpuInterruptTargetTaskHandle = _taskHandle.load(std::memory_order_acquire);
        if (mpuInterruptTargetTaskHandle == NULL) {
            ESP_LOGE(taskName(), "Failed to save MPU interrupt target task handle");
            return false;
        } else
            ESP_LOGD(taskName(), "Saved MPU interrupt target task handle");

        _mpu.enableWomSleep();

        return true;
    }

    virtual void taskRun() override {
        _mpu.update();

        /*
        static uint32_t lastLog = 0;
        if (millis() - lastLog > 500) {
            lastLog = millis();
            ESP_LOGD(taskName(), "X: %.2f, Y: %.2f, Z: %.2f", _mpu.getAccX(), _mpu.getAccY(), _mpu.getAccZ());
        }
        */

        // If we are disarmed,do nothing
        if (_state.load(std::memory_order_relaxed) == ALARM_DISARMED) return;

        // If we are armed idle, block indefinitely until the ISR sends a notification
        if (_state.load(std::memory_order_relaxed) == ALARM_ARMED_IDLE) {
            ESP_LOGD(taskName(), "Armed idle, blocking indefinitely");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            // Motion detected! Clear the interrupt register immediately over I2C
            _clearMPUInterrupt();
            _state.store(ALARM_WARNING, std::memory_order_relaxed);
            _stateTimer = millis();
            ESP_LOGI(taskName(), "Triggered");
        }

        // If we are actively processing the warning, poll using short non-blocking ticks
        if (_state.load(std::memory_order_relaxed) == ALARM_WARNING) {
            uint32_t t = millis();
            static uint32_t lastWarningLog = 0;
            if (t - lastWarningLog >= 500) {
                lastWarningLog = t;
                ESP_LOGI(taskName(), "Warning");
            }
            // Check if another interrupt arrived during the warning period
            if (ulTaskNotifyTake(pdTRUE, 0) == pdTRUE) {
                _clearMPUInterrupt();
                _state.store(ALARM_LATCHED, std::memory_order_relaxed);
                _stateTimer = t;
                ESP_LOGI(taskName(), "Latched");
            } else if (t - _stateTimer >= LATCH_DELAY_MS) {
                // Warning timed out with no further motion
                _clearMPUInterrupt();
                _state.store(ALARM_ARMED_IDLE, std::memory_order_relaxed);
                ESP_LOGI(taskName(), "Warning timed out");
            }
            vTaskDelay(pdMS_TO_TICKS(20));  // Short evaluation delay, TODO: why if task is not running with no delay?
        }

        if (_state.load(std::memory_order_relaxed) == ALARM_LATCHED) {
            uint32_t t = millis();

            static uint32_t lastLog = 0;
            if (t - lastLog >= 10000) {
                lastLog = t;
                ESP_LOGD(taskName(), "Latched for %u ms", t - _stateTimer);
            }

            // Keep the hardware buffer clear so it doesn't lock up
            if (ulTaskNotifyTake(pdTRUE, 0) == pdTRUE) {
                _clearMPUInterrupt();
            }
        }
    }

    bool arm() {
        _state.store(ALARM_ARMED_IDLE, std::memory_order_relaxed);
        ESP_LOGD(taskName(), "Armed idle");
        return true;
    }

    bool disarm() {
        _state.store(ALARM_DISARMED, std::memory_order_relaxed);
        ESP_LOGD(taskName(), "Disarmed");
        return true;
    }

    bool isArmed() const {
        return _state.load(std::memory_order_relaxed) != ALARM_DISARMED;
    }
    bool isLatched() const {
        return _state.load(std::memory_order_relaxed) == ALARM_LATCHED;
    }
    bool isWarning() const {
        return _state.load(std::memory_order_relaxed) == ALARM_WARNING;
    }

   protected:
    MPU9250 _mpu;
    enum State { ALARM_DISARMED,
                 ALARM_ARMED_IDLE,
                 ALARM_WARNING,
                 ALARM_LATCHED };
    std::atomic<State> _state = ALARM_DISARMED;

    static constexpr uint32_t LATCH_DELAY_MS = 1500;
    uint32_t _stateTimer = 0;

    Api::Reply _armCommand(const char* args) {
        ESP_LOGI(taskName(), "Arming");
        bool success = arm();
        Api::Reply reply = {};
        reply.code = success ? Api::ReplyCode::SUCCESS : Api::ReplyCode::EXECUTION_ERROR;
        snprintf((char*)reply.data, sizeof(reply.data), success ? "Alarm armed" : "Failed to arm alarm");
        return reply;
    }

    Api::Reply _disarmCommand(const char* args) {
        ESP_LOGI(taskName(), "Disarming");
        bool success = disarm();
        Api::Reply reply = {};
        reply.code = success ? Api::ReplyCode::SUCCESS : Api::ReplyCode::EXECUTION_ERROR;
        snprintf((char*)reply.data, sizeof(reply.data), success ? "Alarm disarmed" : "Failed to disarm alarm");
        return reply;
    }

    // Helper to clear the I2C interrupt line outside the ISR
    void _clearMPUInterrupt() {
        Wire.beginTransmission(0x68);       // Start transmission at MPU address
        Wire.write(0x3A);                   // INT_STATUS register
        Wire.endTransmission(false);        // End transmission, keep the connection open
        Wire.requestFrom(0x68, 1);          // Request 1 byte from MPU
        if (Wire.available()) Wire.read();  // Reading this byte clears the hardware latch
    }
};

#endif  // ALARM_H