#include "tasks/alarm.h"

#include "model/state.h"
extern State state;

#include "tasks/display.h"
extern Display display;

std::atomic<uint32_t> Alarm::_mpuInterruptTimestamp{0};
std::atomic<TaskHandle_t> Alarm::_mpuInterruptTargetTaskHandle{NULL};

const char* Alarm::taskName() const {
    return "Alarm";
}

void Alarm::setup() {
    // The MPU is the only I2C device on the bus
    if (!Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL)) {
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

    esp_deep_sleep_enable_gpio_wakeup(BIT(PIN_MPU_INT), ESP_GPIO_WAKEUP_GPIO_HIGH);
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) {
        ESP_LOGI(taskName(), "Woke up from deep sleep due to MPU interrupt");
    }
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

    _mpuInterruptTimestamp.store(millis(), std::memory_order_release);
    _mpu.enableWomSleep(MPU_THRESHOLD, MPU_FREQUENCY);

    return true;
}

void Alarm::taskRun() {
    uint32_t t = millis();
    _mpu.update();
    State s = _state.load(std::memory_order_relaxed);

    // If we are disarmed, check whether we need to go to sleep,
    // if not, decrease task frequency and do nothing
    if (s == DISARMED) {
        _sleepIfNeeded();
        vTaskDelay(pdMS_TO_TICKS(500));
        return;
    }

    // If we are armed idle, block indefinitely until the ISR sends a notification
    if (s == ARMED_IDLE) {
        ESP_LOGD(taskName(), "Armed, blocking indefinitely");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Motion detected! Clear the interrupt register immediately
        _clearMPUInterrupt();
        s = WARNING;
        _state.store(s, std::memory_order_relaxed);
        _stateChangeTimestamp.store(millis(), std::memory_order_release);

        ESP_LOGI(taskName(), "Warning triggered");
    }

    _clearMPUInterrupt();

    if (s == WARNING) {
        uint32_t lastInterrupt = _mpuInterruptTimestamp.load(std::memory_order_acquire);
        uint32_t timeSinceLastInterrupt = t - lastInterrupt;
        uint32_t timeSinceWarningStart = t - _stateChangeTimestamp.load(std::memory_order_acquire);
        if (timeSinceWarningStart < WARNING_GRACE_PERIOD_MS) {  // Don't latch during grace period
            if (lastInterrupt != 0) {
                ESP_LOGD(taskName(), "Movement during grace period");
            }
            _mpuInterruptTimestamp.store(0, std::memory_order_release);
        } else if (timeSinceLastInterrupt <= LATCH_DELAY_MS) {  // Latch within window
            _state.store(LATCHED, std::memory_order_relaxed);
            _stateChangeTimestamp.store(t, std::memory_order_release);
        } else if (timeSinceWarningStart >= LATCH_DELAY_MS) {  // Warning timed out with no further motion
            _state.store(ARMED_IDLE, std::memory_order_relaxed);
            _stateChangeTimestamp.store(t, std::memory_order_release);
        }
    }

    static State lastState = DISARMED;
    if (s != lastState) {
        lastState = s;
        ESP_LOGD(taskName(), "State: %s", _stateToString(s));
    }
}

bool Alarm::arm() {
    _state.store(ARMED_IDLE, std::memory_order_relaxed);
    _stateChangeTimestamp.store(millis(), std::memory_order_release);
    ESP_LOGD(taskName(), "Armed idle");
    return true;
}

bool Alarm::disarm() {
    _state.store(DISARMED, std::memory_order_relaxed);
    _stateChangeTimestamp.store(millis(), std::memory_order_release);
    ESP_LOGD(taskName(), "Disarmed");
    return true;
}

bool Alarm::isArmed() const {
    return _state.load(std::memory_order_relaxed) != DISARMED;
}

bool Alarm::isLatched() const {
    return _state.load(std::memory_order_relaxed) == LATCHED;
}

bool Alarm::isWarning() const {
    return _state.load(std::memory_order_relaxed) == WARNING;
}

Api::Reply Alarm::_armCommand(const char* args) {
    ESP_LOGI(taskName(), "Arming");
    bool success = arm();
    Api::Reply reply = {};
    reply.code = success ? Api::Reply::Code::Success : Api::Reply::Code::ExecutionError;
    snprintf((char*)reply.data, sizeof(reply.data), success ? "Alarm armed" : "Failed to arm alarm");
    return reply;
}

Api::Reply Alarm::_disarmCommand(const char* args) {
    ESP_LOGI(taskName(), "Disarming");
    bool success = disarm();
    Api::Reply reply = {};
    reply.code = success ? Api::Reply::Code::Success : Api::Reply::Code::ExecutionError;
    snprintf((char*)reply.data, sizeof(reply.data), success ? "Alarm disarmed" : "Failed to disarm alarm");
    return reply;
}

void Alarm::_clearMPUInterrupt() const {
    Wire.beginTransmission(0x68);       // Start transmission at MPU address
    Wire.write(0x3A);                   // INT_STATUS register
    Wire.endTransmission(false);        // End transmission, keep the connection open
    Wire.requestFrom(0x68, 1);          // Request 1 byte from MPU
    if (Wire.available()) Wire.read();  // Reading this byte clears the hardware latch
}

void Alarm::_sleepIfNeeded() const {
    if (isArmed() || state.controllerAlive()) return;

    uint32_t t = millis();
    uint32_t lastInterrupt = _mpuInterruptTimestamp.load(std::memory_order_acquire);
    uint32_t timeSinceLastInterrupt = t - lastInterrupt;
    if (timeSinceLastInterrupt >= SLEEP_DELAY) {
        ESP_LOGD(taskName(), "Last motion %ds ago, going to deep sleep",
                 timeSinceLastInterrupt / 1000);
        display.queueUiEvent(UiEvent::Sleep);
        vTaskDelay(pdMS_TO_TICKS(1000));
        _clearMPUInterrupt();
        esp_deep_sleep_start();
        return;  // Should never reach here
    }
    static uint lastLog = 0;
    uint32_t sleepInS = (lastInterrupt + SLEEP_DELAY - t) / 1000;
    uint32_t logFreqMs = sleepInS > 60 ? 60000 : 10000;
    if (lastLog == 0 || t - lastLog > logFreqMs) {
        ESP_LOGD(taskName(), "Sleep in %dm %ds", sleepInS / 60, sleepInS % 60);
        lastLog = t;
    }
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
