#include "tasks/alarm.h"

std::atomic<uint32_t> Alarm::_mpuInterruptTimestamp{0};
std::atomic<TaskHandle_t> Alarm::_mpuInterruptTargetTaskHandle{NULL};

const char* Alarm::taskName() {
    return "Alarm";
}

void Alarm::setup() {
    if (!Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL)) {  // MPU is the only I2C device on the bus
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

    pinMode(PIN_MPU_INT, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_MPU_INT), _mpuInterruptHandler, RISING);

    api.registerCommand(
        "arm", [this](const char* args) { return _armCommand(args); },
        "Usage: arm\nArms the alarm.");
    api.registerCommand(
        "disarm", [this](const char* args) { return _disarmCommand(args); },
        "Usage: disarm\nDisarms the alarm.");
}

bool Alarm::taskStart(float frequency, uint32_t stack, int8_t priority) {
    if (!Task::taskStart(frequency, stack, priority)) {
        ESP_LOGE(taskName(), "Task::taskStart() failed");
        return false;
    }

    // Save this task's handle so the ISR knows who to notify
    TaskHandle_t h = _taskHandle.load(std::memory_order_acquire);
    _mpuInterruptTargetTaskHandle.store(h, std::memory_order_release);
    if (h == NULL) {
        ESP_LOGE(taskName(), "Failed to save MPU interrupt target task handle");
        return false;
    } else
        ESP_LOGD(taskName(), "Saved MPU interrupt target task handle");

    _mpu.enableWomSleep(MPU_THRESHOLD, MPU_FREQUENCY);

    return true;
}

void Alarm::taskRun() {
    _mpu.update();

    State s = _state.load(std::memory_order_relaxed);

    // If we are disarmed, do nothing and decrease task frequency
    if (s == ALARM_DISARMED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        return;
    }

    // If we are armed idle, block indefinitely until the ISR sends a notification
    if (s == ALARM_ARMED_IDLE) {
        ESP_LOGD(taskName(), "Armed, blocking indefinitely");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Motion detected! Clear the interrupt register immediately over I2C
        _clearMPUInterrupt();
        s = ALARM_WARNING;
        _state.store(s, std::memory_order_relaxed);
        _stateChangeTimestamp.store(millis(), std::memory_order_release);

        ESP_LOGI(taskName(), "Warning triggered");
    }

    _clearMPUInterrupt();

    if (s == ALARM_WARNING) {
        uint32_t t = millis();
        uint32_t lastInterrupt = _mpuInterruptTimestamp.load(std::memory_order_acquire);
        uint32_t timeSinceLastInterrupt = t - lastInterrupt;
        uint32_t timeSinceWarningStart = t - _stateChangeTimestamp.load(std::memory_order_acquire);
        if (timeSinceWarningStart < WARNING_GRACE_PERIOD_MS) {  // Don't latch during grace period
            if (lastInterrupt != 0) {
                ESP_LOGD(taskName(), "Movement during grace period");
            }
            _mpuInterruptTimestamp.store(0, std::memory_order_release);
        } else if (timeSinceLastInterrupt <= LATCH_DELAY_MS) {  // Latch within window
            _state.store(ALARM_LATCHED, std::memory_order_relaxed);
            _stateChangeTimestamp.store(t, std::memory_order_release);
        } else if (timeSinceWarningStart >= LATCH_DELAY_MS) {  // Warning timed out with no further motion
            _state.store(ALARM_ARMED_IDLE, std::memory_order_relaxed);
            _stateChangeTimestamp.store(t, std::memory_order_release);
        }
    }

    static State lastState = s;
    if (s != lastState) {
        lastState = s;
        ESP_LOGD(taskName(), "State: %s", _stateToString(s).c_str());
    }
}

bool Alarm::arm() {
    _state.store(ALARM_ARMED_IDLE, std::memory_order_relaxed);
    _stateChangeTimestamp.store(millis(), std::memory_order_release);
    ESP_LOGD(taskName(), "Armed idle");
    return true;
}

bool Alarm::disarm() {
    _state.store(ALARM_DISARMED, std::memory_order_relaxed);
    _stateChangeTimestamp.store(millis(), std::memory_order_release);
    ESP_LOGD(taskName(), "Disarmed");
    return true;
}

bool Alarm::isArmed() const {
    return _state.load(std::memory_order_relaxed) != ALARM_DISARMED;
}

bool Alarm::isLatched() const {
    return _state.load(std::memory_order_relaxed) == ALARM_LATCHED;
}

bool Alarm::isWarning() const {
    return _state.load(std::memory_order_relaxed) == ALARM_WARNING;
}

Api::Reply Alarm::_armCommand(const char* args) {
    ESP_LOGI(taskName(), "Arming");
    bool success = arm();
    Api::Reply reply = {};
    reply.code = success ? Api::ReplyCode::SUCCESS : Api::ReplyCode::EXECUTION_ERROR;
    snprintf((char*)reply.data, sizeof(reply.data), success ? "Alarm armed" : "Failed to arm alarm");
    return reply;
}

Api::Reply Alarm::_disarmCommand(const char* args) {
    ESP_LOGI(taskName(), "Disarming");
    bool success = disarm();
    Api::Reply reply = {};
    reply.code = success ? Api::ReplyCode::SUCCESS : Api::ReplyCode::EXECUTION_ERROR;
    snprintf((char*)reply.data, sizeof(reply.data), success ? "Alarm disarmed" : "Failed to disarm alarm");
    return reply;
}

void Alarm::_clearMPUInterrupt() {
    Wire.beginTransmission(0x68);       // Start transmission at MPU address
    Wire.write(0x3A);                   // INT_STATUS register
    Wire.endTransmission(false);        // End transmission, keep the connection open
    Wire.requestFrom(0x68, 1);          // Request 1 byte from MPU
    if (Wire.available()) Wire.read();  // Reading this byte clears the hardware latch
}

void IRAM_ATTR Alarm::_mpuInterruptHandler() {
    uint32_t ts = millis();
    _mpuInterruptTimestamp.store(ts, std::memory_order_release);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TaskHandle_t h = _mpuInterruptTargetTaskHandle.load(std::memory_order_acquire);
    if (h != NULL) {
        // Unblock the Alarm task immediately from the interrupt context
        vTaskNotifyGiveFromISR(h, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
    }
}
