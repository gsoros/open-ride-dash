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
- **ST7789** IPS TFT display module

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
1     CAN Transceiver              SN65HVD230                    3.3V operation
1     Display Module               ST7789, 1.54" 240x240         SPI
1     Keypad                       3 to 6-button membrane        Custom or DPC245
1     CAN Connector                5-pin automotive              Waterproof
2     1N4148 Diodes                SMD 0805
2     3.3 KOhm Resistors           SMD 0805
1     470 Ohm Resistor             SMD 0805
1     1.5 KOhm Resistor            SMD 0805
1     Prototyping Board            Perf board or custom PCB      For assembly
1     Quartz Glass Sheet           ~ 30x30x1mm
1     Optical Adhesive             LOCA UV Cure
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

1. Connect SN65HVD230 to ESP32-C3 CAN pins
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

The Bafang M500/M600 motor controller communicates with the display over the **5-pin EB-BUS (Higo Mini-F) connector**. Two of those pins — **CTRL** and **P+VBAT** — handle power sequencing. Understanding the difference between them is critical.

#### Pin Assignments (Bafang EB-BUS Motor Controller Connector, 5-pin Higo Mini-F)

| Pin | Signal | Description                                                                                                                     |
| --- | ------ | ------------------------------------------------------------------------------------------------------------------------------- |
| 1   | P+VBAT | Battery voltage **output** from controller. 0 V when off; asserts Vbat once power-on completes. Feeds the DC-DC converter.      |
| 2   | GND    | Ground                                                                                                                          |
| 3   | CTRL   | Power control signal. Held at Vbat by internal pull-up when idle. Remote POWER button pulls it toward GND to initiate power-on. |
| 4   | CAN_H  | CAN bus high                                                                                                                    |
| 5   | CAN_L  | CAN bus low                                                                                                                     |

> ⚠️ **Both P+VBAT and CTRL sit at battery voltage (36–52 V) under various conditions.** Do not connect either directly to a microcontroller GPIO. See protection circuit below.

#### How Power-On Actually Works

This is a **hardware-initiated, firmware-sustained** sequence — not a software-controlled one:

1. **At rest:** CTRL is held at Vbat by an internal pull-up inside the controller. P+VBAT is 0 V.
2. **Short press of the POWER button on the remote:** The button connects CTRL through a resistor/diode network to GND, pulling CTRL down to ~4.5 V. The controller detects this transition.
3. **Long press of the POWER button:** CTRL is pulled closer to GND. The controller responds by asserting P+VBAT (battery voltage), which powers the DC-DC converter and thus the microcontroller and display.
4. **Firmware takes over:** Once the MCU boots, it **must immediately begin transmitting valid CAN messages** to the controller. If the controller receives no valid CAN traffic within a few seconds, it cuts P+VBAT and shuts everything down.
5. **Power-off:** Long press again pulls CTRL low; the controller de-asserts P+VBAT.

**The MCU cannot initiate power-on in software.** There is no GPIO the firmware can wiggle to turn the system on — only the physical button can do that. The firmware's power-related responsibility is exclusively keeping the system _alive_ via CAN after the button brings it up.

#### Walk Mode

The **DOWN button** on the remote uses an identical circuit: it pulls a dedicated signal line toward GND through D2/R4/R3, and the MCU reads that as an input. The motor controller independently monitors this line and activates walk assist when it is pulled low. Again, this is a hardware button circuit — the MCU reads the button state but the controller responds to the raw signal directly.

#### Circuit Design — CTRL and DOWN Input Protection

The resistor/diode networks around CTRL and DOWN are **input protection and level-shifting circuits**, not output drive circuits. They allow a 3.3 V MCU to safely read lines that can sit at battery voltage.

**POWER button / CTRL input (D1, R1, R2):**

```
CTRL (from motor) ──── R1 (470 Ω) ──── D1 anode
                                        D1 cathode ──── MCU GPIO (CTRL input)
                                                   └─── R2 (3K3) ──── +3.3 V
```

- When CTRL is at Vbat and button is not pressed: D1 is reverse-biased; MCU GPIO is pulled to 3.3 V by R2 — safe.
- When button pulls CTRL to ~4.5 V: D1 conducts slightly; R1 drops voltage further; MCU GPIO sees a safe logic-high level.
- R2 provides the MCU-side pull-up so the GPIO has a defined level when CTRL is not being driven.

**DOWN button / walk mode input (D2, R3, R4):** identical principle with R3 (3K3) and R4 (1K5).

The MCU GPIOs are **never exposed to battery voltage** as long as the circuit is built correctly as above.

#### Wiring Diagram

```
   Bafang EB-BUS (5-pin Higo Mini-F)          Prototyping board
   P+VBAT (Pin 1) ──────────────────────────── DC-DC IN+ (STH0548S05)
   GND    (Pin 2) ──────────────────────────── DC-DC IN− / system GND
   CTRL   (Pin 3) ──── R1 (470 Ω) ──── D1 ─── MCU GPIO (input, reads power state)
                                               └── R2 (3K3) → +3.3 V
   CAN_H  (Pin 4) ──────────────────────────── SN65HVD230 CAN_H
   CAN_L  (Pin 5) ──────────────────────────── SN65HVD230 CAN_L

   850C Remote (6-pin control unit connector)
   DOWN button ──── R4 (1K5) ──── D2 ─────── MCU GPIO (input, reads walk mode)
                                             └── R3 (3K3) → +3.3 V
   POWER button ─── connected to CTRL line (pulls CTRL toward GND when pressed)
```

#### Protections

- SMBJ58A across VBAT - GND
- 72V 0.3A PPTC Polyfuse on the Vbat line
- 16V 1000µF Electrolytic Bulk Cap: where the 5V enters PCB from the buck converter
- 100V 100nF Ceramic Caps: as close as possible to the VCC pin of the SN65HVD230, another near the ESP32-C3 power pins, another before the buck
- 100V 68µF Electrolytic Bulk Cap: before the buck
- SMBJ5.0A: across 5V and GND rails (.CA bidirectional variant acceptable)

#### Firmware Considerations

- CTRL and DOWN GPIOs are **inputs**, not outputs. Configure them as digital inputs with no internal pull-up (external R2/R3 handle that).
- **CAN keepalive is not optional.** The exact message sequence needed to keep the controller alive after power-on is in the [OpenSourceEBike CAN logs](https://github.com/OpenSourceEBike/Bafang_M500_M600/blob/main/Hardware/Display/DP_C240_241/CAN_stop_sequence.txt). The placeholder `powerOnController()` must be replaced with the real sequence before any bench testing on a live motor.
- The MCU can monitor CTRL to detect user power-off intent (button long-press will pull CTRL low again) and do a clean shutdown before P+VBAT is cut.
- Walk mode: monitor the DOWN GPIO; signal is active while button is held. The controller enforces the ~6 km/h speed limit and the 30-second timeout autonomously — no firmware enforcement needed.

#### Bill of Materials Additions

The components required for this circuit are already listed in the Phase 1 BoM:

- 2 × 1N4148 diodes (SMD 0805) — D1 (CTRL), D2 (DOWN/walk)
- 2 × 3.3 kΩ resistors (SMD 0805) — R2 (CTRL pull-up), R3 (DOWN pull-up)
- 1 × 470 Ω resistor (SMD 0805) — R1 (CTRL series protection)
- 1 × 1.5 kΩ resistor (SMD 0805) — R4 (DOWN series protection)

Refer to the [OpenSourceEBike documentation](https://opensourceebike.github.io/easy_diy_display_ebike_display/build_display-bafang_m500_M600.html) for the original schematic and further details.

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
│   │   ├── ST7789.cpp
│   │   └── SN65HVD230_can.cpp
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

    ST7789 display;
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
