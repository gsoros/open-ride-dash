# Performance Optimization Guide

## Introduction

This guide provides techniques and strategies for optimizing OpenRideDash performance, focusing on resource efficiency, power consumption, and responsiveness. Proper optimization ensures the system runs smoothly even under demanding conditions.

## Optimization Philosophy

### Goals

- **Responsiveness**: Fast response to user inputs (< 100ms)
- **Efficiency**: Low power consumption for battery operation
- **Reliability**: Stable operation under varying conditions
- **Scalability**: Ability to add features without degrading performance

### Trade-offs

Optimization often involves trade-offs:

- **Speed vs Power**: Faster processing consumes more power
- **Memory vs Features**: More features require more memory
- **Complexity vs Performance**: Simple code may be less efficient

## Performance Metrics

### Key Performance Indicators

| Metric                 | Target              | Measurement Method                  |
| ---------------------- | ------------------- | ----------------------------------- |
| CAN Processing Latency | < 10ms              | Timing from reception to processing |
| Display Update Rate    | 30Hz                | Frame timing measurement            |
| Button Response Time   | < 100ms             | GPIO interrupt to action            |
| BLE Update Interval    | Configurable 1-10Hz | Notification timing                 |
| Power Consumption      | < 500mA active      | Current measurement                 |
| Startup Time           | < 3 seconds         | Boot to operational timing          |

### Baseline Measurement

Before optimization, measure baseline performance:

```cpp
void measurePerformance() {
    uint32_t start = micros();

    // Perform operation to measure
    updateDisplay();

    uint32_t end = micros();
    uint32_t duration = end - start;

    LOG_D("Operation took %u microseconds", duration);
}
```

## Memory Optimization

### RAM Usage Optimization

ESP32-C3 has 400KB SRAM. Optimize RAM usage:

#### 1. Stack Size Optimization

Configure appropriate stack sizes for tasks:

```cpp
// Default stack sizes
#define CAN_TASK_STACK_SIZE 4096
#define DISPLAY_TASK_STACK_SIZE 4096
#define KEYPAD_TASK_STACK_SIZE 2048
#define BLE_TASK_STACK_SIZE 4096

// Reduce if possible
#define CAN_TASK_STACK_SIZE 3072
#define DISPLAY_TASK_STACK_SIZE 3072
```

Monitor stack usage with high water mark:

```cpp
void checkStackUsage() {
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
    LOG_D("Stack high water mark: %u", highWaterMark);

    if (highWaterMark < 100) {
        LOG_W("Stack nearly exhausted!");
    }
}
```

#### 2. Heap Usage Optimization

Minimize dynamic memory allocation:

```cpp
// Avoid frequent allocations
void processMessage(const CanMessage& msg) {
    // Use stack allocation instead of heap
    ParsedData data;
    parseMessage(msg, &data);  // Pass pointer to stack variable

    // Avoid std::string for small strings
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Value: %.1f", data.value);
}
```

Use memory pools for frequent allocations:

```cpp
// Create memory pool for CAN messages
#define CAN_POOL_SIZE 32
#define CAN_MESSAGE_SIZE sizeof(CanMessage)

CanMessage* canPool[CAN_POOL_SIZE];

CanMessage* allocateCanMessage() {
    for (int i = 0; i < CAN_POOL_SIZE; i++) {
        if (canPool[i] == nullptr) {
            canPool[i] = static_cast<CanMessage*>(malloc(CAN_MESSAGE_SIZE));
            return canPool[i];
        }
    }
    return nullptr;
}
```

#### 3. Data Structure Optimization

Choose optimal data structures:

```cpp
// Use packed structures to reduce memory
struct __attribute__((packed)) TelemetryData {
    float batteryVoltage;
    float batteryCurrent;
    float speed;
    uint8_t pasLevel;
    uint32_t timestamp;
};

// Use bitfields for flags
struct SystemFlags {
    uint8_t canConnected : 1;
    uint8_t bleConnected : 1;
    uint8_t displayOn : 1;
    uint8_t powerSaveMode : 1;
    uint8_t errorState : 4;
};
```

### Flash Usage Optimization

ESP32-C3 typically has 4MB flash. Optimize flash usage:

#### 1. Code Size Reduction

Use compiler optimization flags:

```ini
# platformio.ini
build_flags =
    -Os                    # Optimize for size
    -ffunction-sections
    -fdata-sections
```

Remove unused code:

- Disable features via build flags (`-DUSE_BLE=0`)
- Remove debug code in release builds (`-DNDEBUG`)

#### 2. Function Placement

Place rarely used functions in flash:

```cpp
// Mark function to be placed in flash (not IRAM)
void rarelyUsedFunction() __attribute__((noinline));

// Mark critical functions to stay in IRAM
void criticalInterruptHandler() __attribute__((always_inline));
```

#### 3. String Optimization

Reduce string usage:

```cpp
// Use PROGMEM for constant strings
const char* const errorMessages[] PROGMEM = {
    "CAN Error",
    "BLE Error",
    "Display Error"
};

// Use short error codes instead of strings
enum ErrorCode {
    ERR_CAN = 0x01,
    ERR_BLE = 0x02,
    ERR_DISPLAY = 0x03
};
```

## Performance Optimization

### Task Scheduling Optimization

FreeRTOS task scheduling can be optimized:

#### 1. Task Priority Optimization

Assign appropriate priorities:

```cpp
// High priority for time-critical tasks
xTaskCreate(canTask, "CAN", CAN_STACK_SIZE, NULL, 15, NULL);

// Medium priority for user interface
xTaskCreate(displayTask, "Display", DISPLAY_STACK_SIZE, NULL, 9, NULL);

// Low priority for background tasks
xTaskCreate(loggingTask, "Logging", LOG_STACK_SIZE, NULL, 3, NULL);
```

#### 2. Task Frequency Optimization

Adjust task frequencies based on need:

```cpp
// CAN task needs high frequency (100Hz)
void canTask(void* parameter) {
    while (true) {
        processCanMessages();
        vTaskDelay(pdMS_TO_TICKS(10));  // 100Hz
    }
}

// Display task moderate frequency (30Hz)
void displayTask(void* parameter) {
    while (true) {
        updateDisplay();
        vTaskDelay(pdMS_TO_TICKS(33));  // ~30Hz
    }
}

// Logging task low frequency (1Hz)
void loggingTask(void* parameter) {
    while (true) {
        logSystemStatus();
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1Hz
    }
}
```

#### 3. Task Consolidation

Combine low-priority tasks:

```cpp
// Combine sensor reading and logging
void sensorAndLogTask(void* parameter) {
    while (true) {
        readSensors();
        logSensorData();
        vTaskDelay(pdMS_TO_TICKS(100));  // 10Hz
    }
}
```

### Algorithm Optimization

#### 1. CAN Message Processing

Optimize CAN message parsing:

```cpp
// Use lookup table for CAN ID parsing
const CanParserEntry parserTable[] = {
    {0x123, parseBatteryMessage},
    {0x456, parseSpeedMessage},
    {0x789, parseCurrentMessage}
};

void processCanMessage(const CanMessage& msg) {
    for (const auto& entry : parserTable) {
        if (msg.id == entry.id) {
            entry.parseFunction(msg);
            return;
        }
    }
}
```

#### 2. Display Rendering Optimization

Optimize display updates:

```cpp
// Use partial updates instead of full redraw
void updateDisplayPartial() {
    // Only update changed areas
    if (batteryVoltageChanged) {
        drawBatteryVoltage();
    }

    if (speedChanged) {
        drawSpeed();
    }

    // Update only changed region
    display.updatePartial(changeRegion);
}

// Cache rendered values
float lastBatteryVoltage = 0.0f;
void drawBatteryVoltage() {
    if (batteryVoltage != lastBatteryVoltage) {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%2.1fV", batteryVoltage);
        display.drawText(10, 10, buffer);
        lastBatteryVoltage = batteryVoltage;
    }
}
```

#### 3. Mathematical Optimization

Use efficient calculations:

```cpp
// Use fixed-point arithmetic instead of floating-point
int32_t fixedBatteryVoltage = batteryVoltage * 100;  // Store as integer * 100

// Use lookup tables for complex calculations
const float voltageCorrectionTable[] = {
    0.95f, 0.96f, 0.97f, 0.98f, 0.99f, 1.00f
};

float correctVoltage(float rawVoltage) {
    int index = (int)(rawVoltage / 0.1f);
    if (index >= 0 && index < sizeof(voltageCorrectionTable)) {
        return rawVoltage * voltageCorrectionTable[index];
    }
    return rawVoltage;
}
```

## Power Optimization

### Sleep Mode Optimization

ESP32-C3 supports multiple sleep modes:

#### 1. Light Sleep

Light sleep preserves CPU state but halts execution:

```cpp
void enterLightSleep(uint32_t durationMs) {
    // Configure wakeup sources
    esp_sleep_enable_timer_wakeup(durationMs * 1000);

    // Enter light sleep
    esp_light_sleep_start();

    // Resume execution after wakeup
}
```

#### 2. Deep Sleep

Deep sleep powers down most components:

```cpp
void enterDeepSleep(uint32_t durationMs) {
    // Save critical state to RTC memory
    saveSystemState();

    // Configure wakeup sources
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);  // Keypad wake
    esp_sleep_enable_timer_wakeup(durationMs * 1000);

    // Disable peripherals
    display.powerOff();
    can.powerOff();
    ble.powerOff();

    // Enter deep sleep
    esp_deep_sleep_start();
}
```

#### 3. Sleep Strategy

Implement smart sleep strategy:

```cpp
enum SleepLevel {
    ACTIVE,
    IDLE,
    LIGHT_SLEEP,
    DEEP_SLEEP
};

void manageSleep() {
    uint32_t idleTime = millis() - lastActivity;

    if (idleTime > 1000) {          // 1 second idle
        setSleepLevel(IDLE);
    } else if (idleTime > 30000) {  // 30 seconds idle
        setSleepLevel(LIGHT_SLEEP);
    } else if (idleTime > 60000) {  // 60 seconds idle
        setSleepLevel(DEEP_SLEEP);
    }
}
```

### Peripheral Power Management

#### 1. Display Power Management

Optimize display power consumption:

```cpp
void optimizeDisplayPower() {
    // Adjust backlight based on ambient light
    float lux = lightSensor.read();

    if (lux > 1000) {        // Bright sunlight
        display.setBacklight(30);  // Low backlight, save power
    } else if (lux > 100) {  // Indoor
        display.setBacklight(70);  // Medium backlight
    } else {                 // Dark
        display.setBacklight(100); // Full backlight
    }

    // Turn off display when not needed
    if (sleepLevel == DEEP_SLEEP) {
        display.powerOff();
    }
}
```

#### 2. CAN Power Management

Optimize CAN power:

```cpp
void optimizeCanPower() {
    // Reduce CAN activity when idle
    if (sleepLevel == IDLE) {
        can.setListenOnlyMode();  // Only listen, don't transmit
    }

    // Disable CAN when sleeping
    if (sleepLevel >= LIGHT_SLEEP) {
        can.powerOff();
    }
}
```

#### 3. BLE Power Management

Optimize BLE power:

```cpp
void optimizeBlePower() {
    // Adjust advertising interval based on connection state
    if (ble.isConnected()) {
        ble.setAdvertisingInterval(1000);  // Slow advertising
    } else {
        ble.setAdvertisingInterval(100);   // Fast advertising for discovery
    }

    // Reduce BLE power when idle
    if (sleepLevel == IDLE) {
        ble.setConnectionInterval(100, 200);  // Longer intervals
    }
}
```

## Communication Optimization

### CAN Communication Optimization

#### 1. Message Filtering

Filter unnecessary CAN messages:

```cpp
void setupCanFilters() {
    // Only listen to relevant messages
    CanFilter filters[] = {
        {0x123, 0x7FF},  // Battery voltage messages
        {0x456, 0x7FF},  // Speed messages
        {0x789, 0x7FF},  // Current messages
    };

    can.setFilters(filters, 3);
}
```

#### 2. Message Batching

Batch multiple updates into single message:

```cpp
void sendBatchedUpdates() {
    BatchedMessage batch;
    batch.batteryVoltage = systemState.batteryVoltage;
    batch.speed = systemState.speed;
    batch.current = systemState.batteryCurrent;
    batch.timestamp = millis();

    // Send batched message instead of individual updates
    can.send(batch);
}
```

### BLE Communication Optimization

#### 1. Data Compression

Compress data before transmission:

```cpp
void sendCompressedTelemetry() {
    TelemetryData data = getTelemetryData();

    // Compress data
    CompressedTelemetry compressed;
    compressTelemetry(data, &compressed);

    // Send compressed data
    ble.sendTelemetry(compressed);
}
```

#### 2. Selective Updates

Only send changed data:

```cpp
void sendSelectiveUpdates() {
    TelemetryData data = getTelemetryData();

    // Check what changed
    if (data.batteryVoltage != lastSent.batteryVoltage) {
        ble.sendBatteryUpdate(data.batteryVoltage);
    }

    if (data.speed != lastSent.speed) {
        ble.sendSpeedUpdate(data.speed);
    }

    lastSent = data;
}
```

#### 3. Connection Parameter Optimization

Optimize BLE connection parameters:

```cpp
void optimizeConnectionParameters() {
    BLEConnectionParameters params;

    if (highDataRateNeeded) {
        params.minInterval = 7;   // 7.5ms
        params.maxInterval = 15;  // 15ms
        params.latency = 0;
        params.timeout = 400;     // 4 seconds
    } else {
        params.minInterval = 100; // 100ms
        params.maxInterval = 200; // 200ms
        params.latency = 2;
        params.timeout = 600;     // 6 seconds
    }

    ble.setConnectionParameters(params);
}
```

## Real-time Optimization

### Interrupt Optimization

#### 1. Interrupt Handler Optimization

Keep interrupt handlers minimal:

```cpp
// Minimal interrupt handler
void keypadInterruptHandler() {
    // Only set flag, process in task context
    keyPressed = true;
    keyPressTime = millis();
}

// Process in task
void keypadTask(void* parameter) {
    while (true) {
        if (keyPressed) {
            processKeyPress();
            keyPressed = false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

#### 2. Interrupt Priority

Set appropriate interrupt priorities:

```cpp
// CAN interrupt highest priority
gpio_set_intr_type(CAN_INT_PIN, GPIO_INTR_NEGEDGE);
gpio_isr_handler_add(CAN_INT_PIN, canInterruptHandler, NULL);

// Keypad interrupt medium priority
gpio_set_intr_type(KEYPAD_INT_PIN, GPIO_INTR_NEGEDGE);
gpio_isr_handler_add(KEYPAD_INT_PIN, keypadInterruptHandler, NULL);
```

### Timing Optimization

#### 1. Critical Path Optimization

Identify and optimize critical paths:

```cpp
// Critical path: button press → display update
void optimizeCriticalPath() {
    // Measure critical path timing
    uint32_t start = micros();

    buttonPressHandler();
    updateDisplay();

    uint32_t end = micros();
    uint32_t duration = end - start;

    LOG_D("Critical path duration: %u us", duration);

    // Optimize if too long
    if (duration > 100000) {  // > 100ms
        LOG_W("Critical path too long, need optimization");
    }
}
```

#### 2. Deadline Management

Ensure real-time deadlines are met:

```cpp
void ensureDeadlines() {
    uint32_t canDeadline = lastCanProcess + 10;  // 10ms deadline

    if (millis() > canDeadline) {
        LOG_W("CAN processing deadline missed!");

        // Emergency processing
        emergencyCanProcessing();
    }
}
```

## Tooling for Optimization

### Performance Profiling Tools

#### 1. Serial Profiling

Basic profiling via serial output:

```cpp
void profileOperation(const char* operationName) {
    uint32_t start = micros();

    // Operation to profile

    uint32_t end = micros();
    uint32_t duration = end - start;

    LOG_D("[PROFILE] %s: %u us", operationName, duration);
}
```

#### 2. Task Profiling

Profile task execution:

```cpp
void profileTaskExecution() {
    TaskStatus_t taskStatus;
    vTaskGetInfo(NULL, &taskStatus, true, eRunning);

    LOG_D("Task CPU usage: %u%%", taskStatus.ulRunTimeCounter);
}
```

#### 3. Memory Profiling

Profile memory usage:

```cpp
void profileMemoryUsage() {
    multi_heap_info_t heapInfo;
    heap_caps_get_info(&heapInfo, MALLOC_CAP_INTERNAL);

    LOG_D("Free heap: %u, Used heap: %u",
          heapInfo.total_free_bytes,
          heapInfo.total_allocated_bytes);
}
```

### Optimization Validation

#### 1. Before/After Comparison

Compare performance before and after optimization:

```cpp
void validateOptimization() {
    uint32_t before = measurePerformance("before");

    applyOptimization();

    uint32_t after = measurePerformance("after");

    float improvement = ((float)(before - after) / before) * 100.0f;
    LOG_I("Optimization improvement: %.1f%%", improvement);
}
```

#### 2. Regression Testing

Ensure optimization doesn't break functionality:

```cpp
void regressionTest() {
    // Run all tests
    testCanCommunication();
    testDisplay();
    testKeypad();
    testBLE();

    // Compare results with baseline
    if (testResults != baselineResults) {
        LOG_E("Optimization caused regression!");
    }
}
```

## Optimization Examples

### Example 1: Optimizing Display Refresh

Before optimization (full refresh every frame):

```cpp
void updateDisplay() {
    display.clear();
    drawBatteryVoltage();
    drawSpeed();
    drawCurrent();
    drawPasLevel();
    display.update();
}
```

After optimization (partial refresh):

```cpp
void updateDisplayOptimized() {
    DisplayRegion changedRegion = getChangedRegion();

    if (changedRegion.area > 0) {
        display.clearRegion(changedRegion);

        if (changedRegion.includesBattery) {
            drawBatteryVoltage();
        }

        if (changedRegion.includesSpeed) {
            drawSpeed();
        }

        display.updateRegion(changedRegion);
    }
}
```

### Example 2: Optimizing CAN Message Processing

Before optimization (linear search):

```cpp
void processCanMessage(const CanMessage& msg) {
    if (msg.id == 0x123) {
        parseBatteryMessage(msg);
    } else if (msg.id == 0x456) {
        parseSpeedMessage(msg);
    } else if (msg.id == 0x789) {
        parseCurrentMessage(msg);
    } // ... many more checks
}
```

After optimization (lookup table):

```cpp
const CanParserEntry parserTable[] = {
    {0x123, parseBatteryMessage},
    {0x456, parseSpeedMessage},
    {0x789, parseCurrentMessage},
    // ... more entries
};

void processCanMessageOptimized(const CanMessage& msg) {
    for (const auto& entry : parserTable) {
        if (msg.id == entry.id) {
            entry.parseFunction(msg);
            return;
        }
    }
}
```

### Example 3: Optimizing Power Consumption

Before optimization (constant power):

```cpp
void runSystem() {
    display.setBacklight(100);
    ble.setAdvertisingInterval(100);
    can.setNormalMode();
    // Always full power
}
```

After optimization (adaptive power):

```cpp
void runSystemOptimized() {
    // Adaptive backlight
    float lux = lightSensor.read();
    display.setBacklight(adaptiveBacklightLevel(lux));

    // Adaptive BLE advertising
    if (ble.isConnected()) {
        ble.setAdvertisingInterval(1000);
    } else {
        ble.setAdvertisingInterval(100);
    }

    // Adaptive CAN mode
    if (systemState.controllerPowered) {
        can.setNormalMode();
    } else {
        can.setListenOnlyMode();
    }
}
```

## Optimization Checklist

### Memory Optimization Checklist

- [ ] Reduce stack sizes where possible
- [ ] Minimize dynamic memory allocation
- [ ] Use packed data structures
- [ ] Remove unused code and strings
- [ ] Use PROGMEM for constant data

### Performance Optimization Checklist

- [ ] Optimize task priorities and frequencies
- [ ] Use efficient algorithms and data structures
- [ ] Implement partial updates where possible
- [ ] Cache frequently used values
- [ ] Use lookup tables instead of calculations

### Power Optimization Checklist

- [ ] Implement sleep modes appropriately
- [ ] Adjust peripheral power based on usage
- [ ] Optimize communication intervals
- [ ] Use adaptive brightness and settings
- [ ] Disable unused peripherals

### Communication Optimization Checklist

- [ ] Filter unnecessary messages
- [ ] Batch multiple updates
- [ ] Compress data before transmission
- [ ] Send only changed data
- [ ] Optimize connection parameters

## Next Steps

After applying optimizations:

1. **Measure performance improvements**
2. **Validate functionality with regression tests**
3. **Document optimization changes**
4. **Share optimizations with community**
5. **Consider further optimization opportunities**

## Additional Resources

- **[ESP32 Optimization Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/performance/speed.html)** - ESP-IDF performance guide
- **[FreeRTOS Optimization](https://www.freertos.org/implementation/task_priorities.html)** - FreeRTOS optimization techniques
- **[Memory Optimization Techniques](https://github.com/open-ride-dash/firmware/wiki/Memory-Optimization)** - Community memory optimization wiki
- **[Power Optimization Examples](https://github.com/open-ride-dash/firmware/tree/main/examples/optimization)** - Example optimization code

---

_Optimization is an iterative process. Start with the most critical bottlenecks, measure improvements, and continue refining. Remember that readability and maintainability should not be sacrificed for minor performance gains._
