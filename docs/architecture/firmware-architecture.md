# Firmware Architecture

## Overview

OpenRideDash firmware follows a task-based, event-driven architecture using FreeRTOS on the ESP32 platform. The design emphasizes modularity, hardware abstraction, and deterministic behavior.

## Core Architecture Principles

### 1. Task-Based Execution

- Each major subsystem runs as an independent FreeRTOS task
- Tasks have explicit priorities and stack allocations
- No central `loop()` function controls everything

### 2. Hardware Abstraction

- All hardware access through abstract interfaces
- Concrete implementations selected at compile time
- Mock implementations for testing

### 3. Event-Driven Design

- Interrupts only set flags or update state
- Processing deferred to task context
- Non-blocking operations throughout

### 4. Data-Centric Design

- Central data model holds system state
- Tasks subscribe to data changes
- Immutable data structures where possible

## Task Model

### Task Hierarchy

```
High Priority (10-15)
├── CAN Manager Task        (Priority 15)
├── Keypad Handler Task     (Priority 14)
└── BLE Server Task         (Priority 13)

Medium Priority (5-9)
├── Display Update Task     (Priority 9)
├── Sensor Polling Task     (Priority 8)
└── Power Management Task   (Priority 7)

Low Priority (1-4)
└── Logging/Diagnostic Task (Priority 3)
```

### Task Responsibilities

#### CAN Manager Task

```cpp
class CanManagerTask : public BaseTask {
    void setup() {
        // Initialize CAN hardware
        // Configure filters and masks
    }

    void run() {
        while (true) {
            // Receive CAN messages
            // Parse and update data model
            // Transmit periodic messages
            vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz
        }
    }
};
```

#### Display Update Task

```cpp
class DisplayTask : public BaseTask {
    void setup() {
        // Initialize display hardware
        // Load fonts and graphics
    }

    void run() {
        while (true) {
            // Update display based on data model
            // Handle screen transitions
            // Manage backlight
            vTaskDelay(pdMS_TO_TICKS(33)); // ~30Hz
        }
    }
};
```

## Hardware Abstraction Layer

### Interface Definitions

```cpp
// Abstract display interface
class Display {
public:
    virtual void init() = 0;
    virtual void clear() = 0;
    virtual void drawText(int x, int y, const char* text) = 0;
    virtual void update() = 0;
    virtual ~Display() = default;
};

// Abstract CAN driver interface
class CanDriver {
public:
    virtual bool init(uint32_t baudrate) = 0;
    virtual bool send(const CanMessage& msg) = 0;
    virtual bool receive(CanMessage& msg) = 0;
    virtual int available() = 0;
    virtual ~CanDriver() = default;
};
```

### Concrete Implementations

```cpp
// ST7789 display implementation
class ST7789Display : public Display {
    // Hardware-specific implementation
};

// ESP32-C3 native CAN implementation
class Esp32CanDriver : public CanDriver {
    // Native CAN controller implementation
};

// MCP2515 SPI CAN implementation
class Mcp2515CanDriver : public CanDriver {
    // External CAN controller implementation
};
```

### Factory Pattern

```cpp
std::unique_ptr<Display> createDisplay() {
#ifdef USE_ST7789
    return std::make_unique<ST7789Display>();
#elif defined(USE_ILI9341)
    return std::make_unique<ILI9341Display>();
#else
    #error "No display selected"
#endif
}
```

## Data Model Design

### Central State Structure

```cpp
struct SystemState {
    // Power system
    float batteryVoltage;
    float batteryCurrent;
    uint8_t batteryPercent;

    // Drive system
    float speed;            // km/h
    uint8_t pasLevel;       // 0-5
    uint32_t odometer;      // meters

    // User interface
    DisplayMode displayMode;
    uint8_t backlightLevel;

    // System status
    bool canConnected;
    bool bleConnected;
    ErrorCode lastError;

    // Timestamps
    uint32_t lastUpdate;
    uint32_t bootTime;
};
```

### Data Flow Pattern

```
Raw Data → Parsing → Validation → State Update → Notification
   ↑          ↑          ↑            ↑              ↑
Hardware   CAN Msg   Range Check  Atomic Update  Subscribers
```

## Event System

### Event Types

```cpp
enum class EventType {
    KeyPress,
    KeyRelease,
    KeyLongPress,
    CanMessageReceived,
    CanMessageSent,
    BleConnected,
    BleDisconnected,
    SensorUpdate,
    ErrorOccurred,
    SystemTick
};
```

### Event Queue Pattern

```cpp
class EventSystem {
public:
    void post(Event event) {
        xQueueSend(eventQueue, &event, portMAX_DELAY);
    }

    bool receive(Event& event, TickType_t timeout) {
        return xQueueReceive(eventQueue, &event, timeout) == pdTRUE;
    }

private:
    QueueHandle_t eventQueue;
};
```

## Communication Patterns

### Inter-Task Communication

1. **Shared State**: Atomic access to `SystemState`
2. **Message Queues**: For event passing between tasks
3. **Semaphores/Mutexes**: For resource protection
4. **Task Notifications**: For simple signaling

### Example: Keypad to CAN Communication

```
Keypad Task → KeyPress Event → Event Queue → CAN Task → CAN Message
```

## Configuration Management

### Persistent Storage

```cpp
class ConfigManager {
public:
    bool load();
    bool save();

    template<typename T>
    bool get(const char* key, T& value);

    template<typename T>
    bool set(const char* key, const T& value);

private:
    Preferences preferences;  // ESP32 non-volatile storage
};
```

### Configuration Structure

```cpp
struct SystemConfig {
    // Display settings
    uint8_t brightness;
    uint8_t contrast;
    DisplayLayout layout;

    // CAN settings
    uint32_t canBaudrate;
    CanFilter filters[4];

    // BLE settings
    char deviceName[32];
    uint16_t bleInterval;

    // Power management
    uint16_t sleepTimeout;
    uint8_t lowPowerThreshold;
};
```

## Error Handling

### Error Hierarchy

```cpp
enum class ErrorSeverity {
    DEBUG,      // Informational only
    WARNING,    // Non-critical issue
    ERROR,      // Function impaired
    CRITICAL,   // System unstable
    FATAL       // Immediate shutdown required
};

struct SystemError {
    ErrorCode code;
    ErrorSeverity severity;
    const char* message;
    uint32_t timestamp;
    uint32_t taskId;
};
```

### Error Recovery Strategies

1. **Retry with Backoff**: For transient communication errors
2. **Reinitialization**: For hardware in unknown state
3. **Graceful Degradation**: Disable non-essential features
4. **Safe Mode**: Minimal functionality with error display

## Power Management

### Sleep States

```cpp
enum class PowerState {
    ACTIVE,         // Full operation
    IDLE,           // Reduced update rates
    LIGHT_SLEEP,    // CPU halted, peripherals active
    DEEP_SLEEP      // CPU off, wake by interrupt
};
```

### State Transitions

```
ACTIVE → (timeout) → IDLE → (timeout) → LIGHT_SLEEP → (timeout) → DEEP_SLEEP
    ↑                       ↑                       ↑
   Keypress              Keypress                External Wake
```

## Build System

### PlatformIO Configuration

```ini
[env:esp32-c3-devkit]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
board_build.f_cpu = 160000000L

build_flags =
    -DUSE_ST7789
    -DUSE_ESP32_CAN
    -DLOG_LEVEL=3

lib_deps =
    arduino-libraries/Arduino-CAN@^0.4.0
    bodmer/TFT_eSPI@^2.5.0
```

### Feature Flags

```cpp
// Feature selection at compile time
#ifdef USE_BLE
    #include "BLEServer.h"
#endif

#ifdef USE_SENSORS
    #include "SensorManager.h"
#endif
```

## Testing Architecture

### Unit Testing

```cpp
TEST(CanManagerTest, MessageParsing) {
    CanManager manager;
    CanMessage msg = {0x123, 8, {0x01, 0x02, 0x03, 0x04}};

    manager.processMessage(msg);

    EXPECT_EQ(manager.getBatteryVoltage(), 48.0f);
}
```

### Integration Testing

```cpp
class IntegrationTest {
    void testCompleteSystem() {
        // Initialize all components
        // Simulate CAN messages
        // Verify display output
        // Test BLE communication
    }
};
```

## Code Organization

```
src/
├── tasks/           # FreeRTOS task implementations
│   ├── can_task.cpp
│   ├── display_task.cpp
│   └── keypad_task.cpp
├── drivers/         # Hardware drivers
│   ├── display/
│   ├── can/
│   └── sensors/
├── interfaces/      # Abstract interfaces
│   ├── display.h
│   ├── can_driver.h
│   └── keypad.h
├── model/           # Data structures
│   ├── system_state.cpp
│   └── config.cpp
├── services/        # Business logic
│   ├── can_service.cpp
│   └── ble_service.cpp
└── utils/           # Utilities
    ├── logging.cpp
    └── event_system.cpp
```

## Development Guidelines

### Code Style

- **Naming**: camelCase for variables, PascalCase for classes
- **Indentation**: 4 spaces, no tabs
- **Comments**: Doxygen-style for public APIs
- **Error Handling**: Always check return values

### Memory Management

- **Stack Allocation**: Prefer stack for small, short-lived objects
- **Heap Allocation**: Use `std::unique_ptr` for owned resources
- **Pool Allocation**: For frequently allocated/deallocated objects

### Performance Considerations

- **Minimize Copies**: Pass by const reference where possible
- **Cache Awareness**: Keep related data together
- **Avoid Dynamic Allocation**: In time-critical paths

## Next Steps

- **Implementation Start**: See [Phase 1: Core System](../phases/phase1-core.md)
- **Development Setup**: Check [Development Guide](../development/guide.md)
- **Hardware Integration**: Review [Hardware Architecture](hardware-architecture.md)
- **API Design**: Refer to [BLE API Documentation](../api/ble-api.md)

---

_This firmware architecture provides a robust foundation for reliable e-bike display operation while maintaining the flexibility needed for customization and future expansion._
