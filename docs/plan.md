# OpenRideDash (ORD)

## Overview

An open, modular firmware and hardware platform for ESP32-based e-bike displays communicating over CAN, with optional BLE connectivity and mobile app integration.

---

# Phase 1 — Core System (Minimum Viable Product)

## Objective

Establish a reliable embedded system capable of interfacing with the bike controller over CAN, handling user input, and displaying basic telemetry.

## Scope

### Hardware

- ESP32-C3 dev board with CAN support (or ESP32 + external CAN module)
- Buck converter module (80V → 5V)
- CAN connector (5-pin)
- Keypad connector (6-pin)
- SSD1283A transflective display module

### Firmware

- PlatformIO project using Arduino framework
- Drivers and initialization for:
  - CAN
  - Display
  - Keypad

- Keypad input handling using interrupts
- CAN message transmission for controller power-on (long press button0)
- CAN message parsing
- Data model for runtime values:
  - Battery voltage (Vbat)
  - Current
  - Speed
  - PAS level

- Basic display output (e.g., Vbat and current)

### Development Tasks

- Bench assembly of hardware
- Verify CAN transceiver (TJA1050 or equivalent; validate MCP2515 if used)
- Reverse-engineering and validation of Bafang CAN messages
- Optional use of CANable for sniffing
- Scaffold project structure and core classes

### Architecture Constraints

- Each subsystem implemented as an independent class (Keypad, CAN, Display)
- Each class exposes `setup()` and `loop()`
- Independent loop frequencies per task
- No blocking I/O in loops
- Interrupts and callbacks only update state and exit

---

# Phase 1 — Execution Checklist (1–2 Week Target)

## Hardware Bring-Up

- [ ] Assemble ESP32 + CAN hardware on bench
- [ ] Verify stable 5V output from buck converter
- [ ] Confirm CAN transceiver operation (loopback or known-good device)
- [ ] Connect display and verify basic initialization
- [ ] Connect keypad and verify electrical input

## Project Scaffold

- [ ] Create PlatformIO project (Arduino framework)
- [ ] Set up basic project structure (classes: CAN, Display, Keypad)
- [ ] Implement logging over serial

## CAN Foundation

- [ ] Initialize CAN interface
- [ ] Capture raw CAN frames (sniffing)
- [ ] Identify key messages (voltage, speed, etc.)
- [ ] Validate ability to send a simple CAN frame
- [ ] Implement controller power-on message (long press button0)

## Input Handling

- [ ] Implement keypad interrupt handler
- [ ] Store last key event (non-blocking)
- [ ] Detect long press vs short press

## Data Model

- [ ] Define struct/class for runtime values:
  - [ ] Vbat
  - [ ] Current
  - [ ] Speed
  - [ ] PAS level

- [ ] Update model from parsed CAN messages

## Display Output

- [ ] Initialize display driver
- [ ] Render basic text
- [ ] Display Vbat and current
- [ ] Update display at fixed interval

## Architecture Validation

- [ ] Ensure each subsystem has setup() and loop()
- [ ] Verify no blocking calls in loops
- [ ] Confirm callbacks only set flags
- [ ] Validate stable loop timing under load

## Basic Integration Test

- [ ] Power system from bike (or simulated supply)
- [ ] Turn controller on via keypad
- [ ] Confirm live telemetry updates
- [ ] Verify system runs reliably for extended period

---

# Phase 2 — Connectivity (BLE Foundation)

## Objective

Introduce BLE communication for telemetry and external interaction while maintaining system stability.

## Scope

### BLE Server

- Define BLE API protocol
- Implement BLE service (NUS-style or equivalent)

### Standard BLE Services

- Cycling services (0x1816, 0x1818)
- Temperature service (0x181A)
- Battery service (0x180F)
- Custom telemetry services where no standard exists

### System Integration

- Extend data model to support BLE publication
- Introduce BLE update throttling
- Ensure non-blocking interaction with existing tasks

### Optional Hardware Integration

- Ambient temperature sensor (DS18B20)
- Light sensor for automatic backlight/headlight control
- Accelerometer for motion-based wake/sleep/alarm

---

# Phase 3 — Mobile Application & User Interaction

## Objective

Enable external control, visualization, and data recording via a mobile application.

## Scope

### Mobile Application (Flutter)

- Device discovery, pairing, and connection
- Subscription to telemetry data
- Real-time display of values

### Data Recording

- Record ride data to GPX
- Include:
  - Location
  - Human power, if torque sensor data is in CAN
  - Assist power
  - Temperature

### Configuration Interface

- GUI for:
  - Screen configuration
  - Keypad behavior
  - Sleep timeout
  - Auto backlight
  - Motion wakeup
  - PAS levels
  - Walk-assist speed
  - BLE update rate

### Security

- BLE pairing support
- GUI for passkey management

### OTA Control

- OTA disabled by default
- App provides controls to:
  - Enable OTA
  - Trigger sleep mode

---

# Phase 4 — Advanced Features & Ecosystem Integration

## Objective

Expand functionality, improve robustness, and enable interoperability with external tools.

## Scope

### OTA Updates

- OTA server implementation
- OTA mode behavior:
  - Suspend all non-essential tasks
  - Optionally keep keypad active for exit
  - Display OTA progress

### BLE Client Functionality

- Connect to BLE heart rate monitor (HRM)
- Display HR data locally
- Forward HR data to mobile application

### CAN–BLE Bridge

- Provide bridge functionality for external tools (e.g., OpenBafangTool)

### Modular Firmware Design

- Generic `Controller` interface
- Controller-specific implementation (`Bafang_M560`)
- Drop-in modules for:
  - Communication protocols
  - Hardware configurations

### Power Management

- Deep sleep implementation:
  - Triggered by timeout, API command, or keypad
  - Wake via keypad or motion interrupt

### Testing & Maintenance

- Unit tests for all testable components
- Continuous README updates after feature/UI changes

---

# Hardware Enclosure (Cross-Phase Considerations)

## Construction

- Epoxy potting for weather and shock protection
- 3D-printed mold:
  - Display facing down
  - Connectors facing upward

- Mold removal after curing; surface finishing as needed

## Mounting

- Embedded threaded inserts for bracket attachment
- Mold includes alignment holes for precise positioning

## Display Protection

- Glass cover bonded with optical adhesive
- Prevent condensation and improve durability

## Thermal Management

- Validate thermal performance before potting
- Consider heat sink or radiator block for ESP32 and/or buck converter

## Sensor Placement

- Ambient temperature sensor mounted underside
- Shielded from sunlight
- Thermally isolated from enclosure and electronics

---

# System Design Notes

## Task Model

- Subsystems: Keypad, CAN, BLE Server, Display, optional BLE Client
- Each task:
  - Own class
  - Own execution frequency

- No blocking operations

## Event Handling

- Callbacks and interrupts only update state
- Processing deferred to main loop

### Examples

- `BLEServer::onWrite()` → copy buffer, set flag, return
- `BLEServer::loop()` → process via API handler
- `Keypad::onKeyPress()` → store key event, return
- `Keypad::loop()` → invoke API actions

## Platform Choice

- Arduino framework preferred for library availability and familiarity
- ESP-IDF acknowledged as more efficient and flexible, particularly for CAN timing

## Modularity and Hardware Abstraction

### Design Principles

The firmware is structured around clear abstraction layers to ensure portability, maintainability, and ease of extension. All hardware-specific functionality must be encapsulated behind well-defined interfaces.

### Interfaces

Abstract base classes must be defined for all hardware modules, including but not limited to:

- `Display`
- `Keypad`
- `TempSensor`
- `CanDriver`

Concrete implementations provide hardware-specific behavior, for example:

- `SSD1283A` implements `Display`
- `BafangDPC245Keypad` implements `Keypad`
- `DS18B20` implements `TempSensor`
- `MCP2515Driver` or native ESP32 CAN implements `CanDriver`

CAN is treated as a core subsystem but still follows the same abstraction principle to allow flexibility in hardware selection and enable testing via mock implementations.

Controller-specific logic (e.g. Bafang protocols) must be separated from the CAN driver via a `Controller` interface.

---

### Compile-Time Configuration

Hardware selection and optional features are controlled via compile-time flags defined in `platformio.ini`.

Example:

```ini
build_flags =
    -DUSE_SSD1283A
    -DUSE_DS18B20
```

Multiple build environments may be defined to represent different hardware configurations.

---

### Factory Pattern

Object instantiation must be centralized using factory functions. Compile-time flags are handled only within these factories.

Example:

```cpp
std::unique_ptr<Display> createDisplay() {
#ifdef USE_SSD1283A
    return std::make_unique<SSD1283A>();
#else
    #error "No display selected"
#endif
}
```

This ensures that conditional compilation does not leak into application logic.

---

### Optional Components

Optional features (e.g. temperature sensor) must not introduce conditional logic throughout the codebase.

Instead, use the null-object pattern:

```cpp
class NoTempSensor : public TempSensor {
public:
    float read() override { return NAN; }
};
```

Factory functions return either a real implementation or a no-op substitute, allowing the rest of the system to operate without `#ifdef` checks.

---

### Codebase Rules

- `#ifdef` usage is restricted to:
  - factory functions
  - low-level hardware initialization

- Application logic must remain free of conditional compilation
- All modules must expose consistent `setup()` and `loop()` interfaces
- Communication between modules must occur through well-defined interfaces and data structures

---

### Benefits

This approach ensures:

- Clean separation between hardware and logic
- Reduced complexity from conditional compilation
- Easier testing and mocking of components
- Scalability as new hardware or features are added
- Maintainable and readable codebase over time

## Task Model (FreeRTOS Integration)

The system uses FreeRTOS tasks (available within the Arduino framework on ESP32) as the primary execution model. Each major subsystem (e.g. CAN, Display, BLE, Keypad) should be implemented as a self-contained task.

### Design Approach

- Each subsystem is implemented as a class that encapsulates:
  - initialization (`setup()` or constructor)
  - a task loop (`run()` or similar)
  - its own timing and state

- A lightweight base class should wrap FreeRTOS task creation (`xTaskCreate` / `xTaskCreatePinnedToCore`) and provide a consistent structure.

### Responsibilities of the Task Base Class

- Create and manage the FreeRTOS task
- Provide a static task entry function that dispatches to the instance method
- Optionally enforce a fixed loop frequency
- Handle basic lifecycle concerns (start/stop if needed)

### Execution Model

- Each task runs independently under FreeRTOS
- No central scheduler (`loop()`) is responsible for application logic
- Inter-task communication should use:
  - queues, or
  - well-defined shared data structures with minimal coupling

### Guidelines

- Assign task priorities explicitly (e.g. CAN higher, Display lower)
- Avoid blocking operations unless explicitly intended
- Prefer deterministic loop timing over ad-hoc delays
- Minimize shared mutable state; prefer message passing where practical

This approach provides deterministic behavior, clear separation of concerns, and an easy migration path to full ESP-IDF if required.

---

# Summary

The project is structured as a staged system:

1. Core embedded functionality
2. BLE connectivity
3. Mobile application and user interaction
4. Advanced features and ecosystem integration

Each phase builds on a stable foundation, reducing integration risk while preserving the full intended feature set.
