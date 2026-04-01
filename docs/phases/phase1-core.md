# Phase 1: Core System (Minimum Viable Product)

## Objective

Establish a reliable embedded system capable of interfacing with the bike controller over CAN, handling user input, and displaying basic telemetry.

## Success Criteria

- ✅ CAN communication established with e-bike controller
- ✅ Basic telemetry (voltage, current, speed) displayed
- ✅ User input via keypad functional
- ✅ System runs stable for 24+ hours
- ✅ All components communicate without blocking

## Scope

### Hardware Components

- **ESP32-C3** dev board with CAN support (or ESP32 + external CAN module)
- **Buck converter** module (80V → 5V)
- **CAN connector** (5-pin automotive)
- **Keypad connector** (6-pin JST)
- **SSD1283A** transflective display module

### Firmware Features

- PlatformIO project using Arduino framework
- Drivers and initialization for:
  - CAN interface
  - Display controller
  - Keypad input
- Keypad input handling using interrupts
- CAN message transmission for controller power-on
- CAN message parsing and data extraction
- Basic display output showing telemetry data

## Hardware Setup

### Bill of Materials

```
Qty  Component                     Specification                 Notes
─────────────────────────────────────────────────────────────────────────
1     ESP32-C3 Dev Board           160MHz, 4MB flash, CAN        With USB-C
1     Buck Converter               LX8015, 80V→5V, 1.5A          Adjustable
1     CAN Transceiver              TJA1050                       5V operation
1     Display Module               ST7789, 1.54" 240x240         SPI
1     Display Module               SSD1283A, 1.6" 130x130        Alternative, SPI
1     Keypad                       6-button membrane             Custom or DPC245
1     CAN Connector                5-pin automotive              Waterproof
2     1N4148 Diodes                SMD 0805
2     3.3 KOhm Resistors           SMD 0805
1     470 Ohm Resistor             SMD 0805
1     1.5 KOhm Resistor            SMD 0805
1     Prototyping Board            Perf board or custom PCB      For assembly
```

### Assembly Instructions

#### Step 1: Power System

1. Solder buck converter input terminals
2. Connect input protection (fuse, TVS diode optional)
3. Verify 5V output with multimeter
4. Test under load (500mA minimum)

#### Step 2: Microcontroller

1. Solder ESP32-C3 to prototyping board
2. Connect power (5V→3.3V LDO if needed)
3. Add decoupling capacitors (10µF + 0.1µF)
4. Connect USB for programming

#### Step 3: CAN Interface

1. Connect TJA1050 to ESP32-C3 CAN pins
2. Add 120Ω termination resistor (if end-of-line)
3. Connect CAN_H and CAN_L to connector
4. Add TVS diodes for protection (optional)

#### Step 4: Display

1. Connect display SPI pins (MOSI, MISO, SCK, CS, DC, RST)
2. Add backlight control if available
3. Secure display to enclosure

#### Step 5: Keypad

1. Connect keypad buttons to GPIO with pull-up resistors
2. Add debouncing capacitors (0.1µF)
3. Connect to waterproof connector

### Controller Power and Walk Mode Circuit

The Bafang M500/M600 motor controller provides two control lines for power management and walk‑mode activation. These lines are accessible through the display connector (6‑pin JST SM series) and must be driven with open‑drain outputs to avoid damaging the controller.

#### Pin Assignments (Bafang Display Connector)

| Pin | Signal | Description                       |
| --- | ------ | --------------------------------- |
| 1   | +5V    | Display power (from controller)   |
| 2   | CAN_H  | CAN bus high                      |
| 3   | CAN_L  | CAN bus low                       |
| 4   | PWR    | Power on/off control (active low) |
| 5   | WALK   | Walk‑mode control (active low)    |
| 6   | GND    | Ground                            |

#### Circuit Design

The interface circuit uses two GPIOs of the ESP32‑C3 configured as open‑drain outputs. Each output is protected with a series resistor and a clamping diode.

**Power‑On/Off Circuit**

- GPIO‑A → 470 Ω resistor → 1N4148 diode (cathode to GPIO) → PWR pin
- A 3.3 kΩ pull‑up resistor connects PWR pin to +5V (internal to controller).
- To turn the controller on, drive GPIO‑A low (sink current). To turn it off, set GPIO‑A high‑impedance (let the internal pull‑up keep PWR high).

**Walk‑Mode Circuit**

- GPIO‑B → 1.5 kΩ resistor → 1N4148 diode (cathode to GPIO) → WALK pin
- A 3.3 kΩ pull‑up resistor connects WALK pin to +5V.
- To activate walk mode, drive GPIO‑B low. Release to deactivate.

**Protection**

- The 1N4148 diodes prevent reverse current when the controller’s internal voltage differs from the GPIO voltage.
- The series resistors limit current to safe levels (≈10 mA) and protect against accidental shorts.

#### Wiring Diagram

```
   ESP32‑C3                          Bafang Connector
   GPIO‑A (IO4) ──┬── 470 Ω ────┬── 1N4148 ──── PWR (Pin 4)
                  │             │
                 GND            │
                                │
   GPIO‑B (IO5) ──┬── 1.5 kΩ ──┼── 1N4148 ──── WALK (Pin 5)
                  │            │
                 GND           │
                               │
   +5V (Pin 1) ────────────────┴── 3.3 kΩ ──── PWR / WALK (internal pull‑up)
```

#### Firmware Considerations

- Both GPIOs must be configured as open‑drain outputs (`GPIO_MODE_OUTPUT_OD`).
- The default state after reset should be high‑impedance (controller off, walk mode inactive).
- A deliberate low pulse of at least 100 ms is required to toggle power.
- Walk mode should be activated only while the button is held (typical timeout 30 seconds).

#### Bill of Materials Additions

The components required for this circuit are already listed in the Phase 1 BoM:

- 2 × 1N4148 diodes (SMD 0805)
- 2 × 3.3 kΩ resistors (SMD 0805)
- 1 × 470 Ω resistor (SMD 0805)
- 1 × 1.5 kΩ resistor (SMD 0805)

Refer to the [OpenSourceEBike documentation](https://opensourceebike.github.io/easy_diy_display_ebike_display/build_display-bafang_m500_M600.html) for the original circuit design and further details.

## Firmware Implementation

### Project Structure

```
open-ride-dash/
├── src/
│   ├── main.cpp
│   ├── tasks/
│   │   ├── can_task.cpp
│   │   ├── display_task.cpp
│   │   └── keypad_task.cpp
│   ├── drivers/
│   │   ├── ssd1283a.cpp
│   │   └── tja1050_can.cpp
│   └── model/
│       └── system_state.cpp
├── include/
│   ├── config.h
│   └── pins.h
└── platformio.ini
```

### Core Tasks

#### CAN Manager Task

```cpp
// tasks/can_task.cpp
class CanTask : public BaseTask {
public:
    void setup() override {
        // Initialize CAN at 500kbps
        CAN.begin(500000);

        // Set up filters for Bafang messages
        setupBafangFilters();
    }

    void run() override {
        while (true) {
            // Check for received messages
            if (CAN.available()) {
                processCanMessage();
            }

            // Send periodic messages if needed
            sendPeriodicMessages();

            vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz
        }
    }

private:
    void processCanMessage() {
        CanMessage msg;
        CAN.read(msg);

        // Parse Bafang-specific messages
        if (isBatteryMessage(msg)) {
            updateBatteryData(msg);
        } else if (isSpeedMessage(msg)) {
            updateSpeedData(msg);
        }
    }
};
```

#### Display Task

```cpp
// tasks/display_task.cpp
class DisplayTask : public BaseTask {
public:
    void setup() override {
        display.init();
        display.clear();
        display.setRotation(1);
    }

    void run() override {
        while (true) {
            updateDisplay();
            vTaskDelay(pdMS_TO_TICKS(33)); // ~30Hz
        }
    }

private:
    void updateDisplay() {
        display.clear();

        // Draw battery voltage
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Bat: %.1fV", systemState.batteryVoltage);
        display.drawText(10, 10, buffer);

        // Draw current
        snprintf(buffer, sizeof(buffer), "Cur: %.1fA", systemState.batteryCurrent);
        display.drawText(10, 30, buffer);

        // Draw speed
        snprintf(buffer, sizeof(buffer), "Spd: %.1fkm/h", systemState.speed);
        display.drawText(10, 50, buffer);

        display.update();
    }

    SSD1283A display;
};
```

#### Keypad Task

```cpp
// tasks/keypad_task.cpp
class KeypadTask : public BaseTask {
public:
    void setup() override {
        // Configure GPIO with interrupts
        for (int i = 0; i < BUTTON_COUNT; i++) {
            pinMode(buttonPins[i], INPUT_PULLUP);
            attachInterrupt(digitalPinToInterrupt(buttonPins[i]),
                           buttonISR, FALLING);
        }
    }

    void run() override {
        while (true) {
            // Process button events from ISR
            processButtonEvents();

            // Handle long press detection
            checkLongPress();

            vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz
        }
    }

private:
    static void buttonISR() {
        // Just set a flag, process in task context
        buttonPressed = true;
    }
};
```

### Data Model

```cpp
// model/system_state.cpp
struct SystemState {
    // Power system
    float batteryVoltage = 0.0f;
    float batteryCurrent = 0.0f;
    uint8_t batteryPercent = 0;

    // Drive system
    float speed = 0.0f;          // km/h
    uint8_t pasLevel = 0;        // 0-5
    bool controllerPowered = false;

    // User interface
    DisplayPage currentPage = DisplayPage::MAIN;
    uint32_t lastUserInteraction = 0;

    // System status
    bool canConnected = false;
    uint32_t canErrorCount = 0;

    // Update timestamps
    uint32_t lastCanUpdate = 0;
    uint32_t lastDisplayUpdate = 0;
};

// Global system state (protected by mutex)
SystemState systemState;
SemaphoreHandle_t stateMutex;
```

## CAN Protocol Implementation

### Bafang CAN Messages (Example)

```
Battery Voltage Message (ID: 0x123)
Data: [0x01][voltage_high][voltage_low][0x00][0x00][0x00][0x00][0x00]
Voltage = (voltage_high << 8 | voltage_low) * 0.1

Speed Message (ID: 0x456)
Data: [0x02][speed_high][speed_low][0x00][0x00][0x00][0x00][0x00]
Speed = (speed_high << 8 | speed_low) * 0.1 km/h
```

### Message Processing

```cpp
void processBafangMessage(const CanMessage& msg) {
    switch (msg.id) {
        case 0x123:  // Battery voltage
            systemState.batteryVoltage = parseBatteryVoltage(msg.data);
            break;

        case 0x456:  // Speed
            systemState.speed = parseSpeed(msg.data);
            break;

        case 0x789:  // Current
            systemState.batteryCurrent = parseCurrent(msg.data);
            break;

        case 0xABC:  // PAS level
            systemState.pasLevel = parsePasLevel(msg.data);
            break;
    }
}
```

### Controller Power-On

```cpp
void powerOnController() {
    CanMessage msg;
    msg.id = 0x100;  // Power control message
    msg.length = 8;
    msg.data[0] = 0x01;  // Power on command
    // ... fill rest of data

    CAN.send(msg);

    systemState.controllerPowered = true;
}
```

## Development Tasks Checklist

### Hardware Bring-Up

- [ ] Assemble ESP32 + CAN hardware on bench
- [ ] Verify stable 5V output from buck converter
- [ ] Confirm CAN transceiver operation (loopback test)
- [ ] Connect display and verify initialization
- [ ] Connect keypad and verify electrical input

### Project Scaffold

- [ ] Create PlatformIO project (Arduino framework)
- [ ] Set up basic project structure (tasks, drivers, model)
- [ ] Implement logging over serial
- [ ] Configure build system with feature flags

### CAN Foundation

- [ ] Initialize CAN interface at 500kbps
- [ ] Capture raw CAN frames from bike controller
- [ ] Identify key messages (voltage, speed, current, PAS)
- [ ] Validate ability to send CAN frames
- [ ] Implement controller power-on message

### Input Handling

- [ ] Implement keypad interrupt handler
- [ ] Store key events in non-blocking queue
- [ ] Detect long press vs short press
- [ ] Map buttons to system functions

### Data Model

- [ ] Define SystemState structure
- [ ] Implement thread-safe access with mutex
- [ ] Update model from parsed CAN messages
- [ ] Add data validation and range checking

### Display Output

- [ ] Initialize display driver
- [ ] Implement basic text rendering
- [ ] Create main display layout
- [ ] Update display at fixed interval (30Hz)
- [ ] Add display page switching

### Architecture Validation

- [ ] Ensure each task has proper setup() and run()
- [ ] Verify no blocking calls in task loops
- [ ] Confirm callbacks only set flags
- [ ] Validate stable timing under load
- [ ] Test memory usage and stack sizes

## Testing Procedures

### Unit Tests

```cpp
TEST(CanParserTest, BatteryVoltageParsing) {
    CanMessage msg = {0x123, 8, {0x01, 0x01, 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00}};
    float voltage = parseBatteryVoltage(msg.data);
    EXPECT_NEAR(voltage, 48.0f, 0.1f);
}
```

### Integration Tests

1. **Power Cycle Test**: 24-hour continuous operation
2. **CAN Stress Test**: High message rate handling
3. **Display Refresh Test**: Ensure no flicker or artifacts
4. **Keypad Response Test**: Verify all buttons work reliably

### Bench Testing

1. Simulate bike power supply (36V-52V DC)
2. Use CAN bus simulator or second device
3. Measure power consumption in different states
4. Validate thermal performance

## Troubleshooting

### Common Issues

#### CAN Communication Problems

- **No messages received**: Check termination resistor, baud rate
- **Garbled data**: Verify ground connections, check for noise
- **Controller not responding**: Confirm power-on message format

#### Display Issues

- **Blank screen**: Check SPI connections, contrast setting
- **Flickering**: Verify stable power supply, check refresh rate
- **Inverted colors**: Adjust display initialization sequence

#### Keypad Issues

- **Ghost presses**: Add debouncing, check pull-up resistors
- **No response**: Verify interrupt configuration, check GPIO mode
- **Multiple buttons**: Check for short circuits between lines

### Debugging Tools

1. **Serial Monitor**: System logs and debug output
2. **CAN Analyzer**: CANable or similar for bus monitoring
3. **Logic Analyzer**: For timing analysis of signals
4. **Multimeter**: Voltage and continuity checks

## Performance Metrics

### Timing Requirements

- **CAN processing**: < 10ms latency
- **Display update**: 30Hz refresh rate
- **Keypad response**: < 100ms
- **System startup**: < 3 seconds

### Resource Usage Targets

- **RAM usage**: < 70% of available
- **Flash usage**: < 80% of available
- **CPU usage**: < 60% average
- **Power consumption**: < 500mA typical

## Next Steps

After completing Phase 1 successfully, proceed to:

1. **[Phase 2: BLE Connectivity](../phases/phase2-ble.md)** - Add Bluetooth communication
2. **[Hardware Enclosure](../hardware/assembly.md)** - Build weatherproof enclosure
3. **[Mobile App Planning](../phases/phase3-mobile.md)** - Design companion application

## Additional Resources

- **[Hardware Guide](../hardware/guide.md)** - Detailed assembly instructions
- **[Firmware Architecture](../architecture/firmware-architecture.md)** - Design patterns
- **[Development Setup](../development/setup.md)** - Environment configuration
- **[Troubleshooting Guide](../support/troubleshooting.md)** - Problem resolution

---

_Phase 1 establishes the foundation upon which all other features are built. Focus on stability and reliability before adding complexity._
