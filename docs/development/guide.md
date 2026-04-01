# Development Guide

## Introduction

This guide provides comprehensive instructions for setting up the OpenRideDash development environment, building the firmware, and contributing to the project. Whether you're a beginner or an experienced embedded developer, this guide will help you get started.

## Development Workflow

### Overview

OpenRideDash development follows a structured workflow:

1. **Environment Setup** - Install tools and dependencies
2. **Project Configuration** - Configure PlatformIO and hardware settings
3. **Code Development** - Write and test firmware features
4. **Build & Flash** - Compile and upload to device
5. **Testing & Debugging** - Verify functionality and fix issues
6. **Contribution** - Submit changes via pull requests

### Recommended Tools

- **PlatformIO** - Primary development environment (VS Code extension or CLI)
- **VS Code** - Recommended IDE with PlatformIO extension
- **Git** - Version control
- **Python 3.x** - Required for build tools
- **Serial Terminal** (e.g., `screen`, `minicom`, or PlatformIO Serial Monitor)

## Environment Setup

### PlatformIO Installation

#### Option 1: VS Code Extension (Recommended)

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Open Extensions view (Ctrl+Shift+X)
3. Search for "PlatformIO IDE" and install
4. Restart VS Code when prompted

#### Option 2: PlatformIO Core (CLI)

```bash
# Install PlatformIO Core
python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py)"

# Add to PATH (add to ~/.bashrc or ~/.zshrc)
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

### Project Setup

1. **Clone Repository**

   ```bash
   git clone https://github.com/open-ride-dash/firmware.git
   cd firmware
   ```

2. **Open in PlatformIO**

   ```bash
   code .
   ```

   Or use PlatformIO Home → Open Project

3. **Install Dependencies**
   PlatformIO will automatically install required libraries when you first build.

### Hardware Setup for Development

1. **Connect ESP32 Dev Board** via USB
2. **Connect CAN Transceiver** (optional for early development)
3. **Connect Display** (optional, can use serial output for testing)
4. **Verify COM Port** - Note the serial port assigned to your device

## Project Structure

```
open-ride-dash-firmware/
├── src/                    # Source code
│   ├── main.cpp           # Application entry point
│   ├── tasks/             # FreeRTOS task implementations
│   │   ├── can_task.cpp
│   │   ├── display_task.cpp
│   │   └── keypad_task.cpp
│   ├── drivers/           # Hardware drivers
│   │   ├── display/
│   │   ├── can/
│   │   └── sensors/
│   ├── interfaces/        # Abstract interfaces
│   │   ├── display.h
│   │   ├── can_driver.h
│   │   └── keypad.h
│   ├── model/             # Data structures
│   │   ├── system_state.cpp
│   │   └── config.cpp
│   ├── services/          # Business logic
│   │   ├── can_service.cpp
│   │   └── ble_service.cpp
│   └── utils/             # Utilities
│       ├── logging.cpp
│       └── event_system.cpp
├── include/               # Header files
│   ├── config.h          # Configuration constants
│   ├── pins.h            # Pin definitions
│   └── version.h         # Version information
├── test/                 # Unit tests
│   ├── unit/
│   └── integration/
├── platformio.ini        # Build configuration
└── README.md
```

## Build System

### PlatformIO Configuration

The `platformio.ini` file defines build environments, libraries, and flags:

```ini
[env:esp32-c3-devkit]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
board_build.f_cpu = 160000000L

build_flags =
    -DUSE_SSD1283A
    -DUSE_ESP32_CAN
    -DLOG_LEVEL=3
    -DCONFIG_BLE_ENABLED=1

lib_deps =
    arduino-libraries/Arduino-CAN@^0.4.0
    bodmer/TFT_eSPI@^2.5.0
    blelib=git+https://github.com/open-ride-dash/blelib.git

monitor_speed = 115200
```

### Build Commands

```bash
# Build project
pio run

# Build specific environment
pio run -e esp32-c3-devkit

# Clean build
pio run -t clean

# Build and upload
pio run -t upload

# Upload only
pio run -t upload

# Monitor serial output
pio device monitor
```

### Using VS Code

1. Open PlatformIO sidebar (alien icon)
2. Select environment from "Project Tasks"
3. Run tasks:
   - Build (Ctrl+Alt+B)
   - Upload (Ctrl+Alt+U)
   - Clean
   - Monitor (Ctrl+Alt+S)

## Configuration Management

### Compile-Time Configuration

Hardware selection and feature flags are controlled via `build_flags` in `platformio.ini`:

```ini
build_flags =
    -DUSE_SSD1283A          # SSD1283A display
    -DUSE_ILI9341           # Alternative display
    -DUSE_ESP32_CAN         # Native ESP32 CAN
    -DUSE_MCP2515           # External MCP2515 CAN
    -DUSE_BLE               # Enable BLE
    -DUSE_SENSORS           # Enable sensor support
    -DLOG_LEVEL=3           # Debug logging level
```

### Runtime Configuration

Runtime configuration is stored in non-volatile memory and can be modified via BLE or serial:

```cpp
#include "config.h"

SystemConfig config;

void loadConfig() {
    preferences.begin("open-ride-dash", false);
    config.brightness = preferences.getUChar("brightness", 80);
    config.canBaudrate = preferences.getUInt("can_baud", 500000);
    preferences.end();
}
```

## Development Practices

### Code Style

- **Indentation**: 4 spaces, no tabs
- **Naming**: camelCase for variables, PascalCase for classes
- **Comments**: Doxygen-style for public APIs
- **Header Guards**: `#pragma once` preferred
- **Includes**: Order: system headers, library headers, project headers

### Memory Management

- **Stack Allocation**: Prefer for small, short-lived objects
- **Heap Allocation**: Use `std::unique_ptr` for owned resources
- **Pool Allocation**: For frequently allocated/deallocated objects
- **Avoid Fragmentation**: Allocate once at startup when possible

### Error Handling

- **Return Codes**: Use `esp_err_t` for ESP32-specific functions
- **Exceptions**: Not used (disabled in Arduino framework)
- **Logging**: Use `LOG_*` macros for different severity levels
- **Assertions**: Use `assert()` for invariants in debug builds

## Testing

### Unit Tests

Unit tests are located in the `test/unit` directory and use the Unity framework:

```cpp
#include <unity.h>
#include "can_parser.h"

void test_battery_voltage_parsing() {
    CanMessage msg = {0x123, 8, {0x01, 0x01, 0x2C, 0x00}};
    float voltage = parseBatteryVoltage(msg.data);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 48.0, voltage);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_battery_voltage_parsing);
    UNITY_END();
}

void loop() {}
```

Run tests with:

```bash
pio test -e native
```

### Integration Tests

Integration tests verify hardware interaction using mock interfaces:

```cpp
class IntegrationTest {
    void test_can_to_display_flow() {
        // Simulate CAN message
        injectCanMessage(batteryVoltageMsg);

        // Verify display shows correct voltage
        verifyDisplayText("48.0V");
    }
};
```

### Hardware Testing

See [Hardware Testing Guide](../hardware/testing.md) for physical testing procedures.

## Debugging

### Serial Debugging

Enable debug logging by setting `LOG_LEVEL` in build flags:

```cpp
LOG_D("CAN message received: ID=0x%X", msg.id);
LOG_I("System started, free heap: %d", ESP.getFreeHeap());
LOG_W("Low battery: %d%%", batteryPercent);
LOG_E("CAN bus error: %d", errorCode);
```

View logs with:

```bash
pio device monitor
```

### GDB Debugging

PlatformIO supports GDB debugging with JTAG or serial:

1. **Configure debugging** in `platformio.ini`:

   ```ini
   debug_tool = esp-builtin
   debug_port = /dev/ttyUSB0
   ```

2. **Start debugging** in VS Code: F5 or Run → Start Debugging

### Memory Debugging

Check heap usage:

```cpp
#include <esp_heap_caps.h>

void printMemoryInfo() {
    LOG_I("Free heap: %d", ESP.getFreeHeap());
    LOG_I("Min free heap: %d", ESP.getMinFreeHeap());
    LOG_I("Max alloc heap: %d", ESP.getMaxAllocHeap());
}
```

## Performance Optimization

### Task Priorities

Assign appropriate FreeRTOS task priorities:

```cpp
// High priority for time-critical tasks
xTaskCreate(canTask, "CAN", 4096, NULL, 15, NULL);

// Medium priority for user interface
xTaskCreate(displayTask, "Display", 4096, NULL, 9, NULL);

// Low priority for background tasks
xTaskCreate(loggingTask, "Logging", 2048, NULL, 3, NULL);
```

### Power Optimization

- **Use light sleep** when idle
- **Reduce BLE advertising interval** when not needed
- **Dim display** in low-light conditions
- **Disable unused peripherals**

### Code Optimization

- **Use `constexpr`** for compile-time calculations
- **Avoid virtual functions** in hot paths
- **Use fixed-point arithmetic** instead of floating-point when possible
- **Minimize memory copies**

## Contribution Guidelines

### Getting Started

1. **Fork the repository** on GitHub
2. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Make your changes**
4. **Write tests** for new functionality
5. **Ensure code style** consistency
6. **Submit a pull request**

### Pull Request Process

1. **Update documentation** if needed
2. **Add changelog entry** in `CHANGELOG.md`
3. **Ensure all tests pass**
4. **Request review** from maintainers
5. **Address feedback** and update PR

### Code Review Checklist

- [ ] Code follows project style guidelines
- [ ] Tests added for new functionality
- [ ] Documentation updated
- [ ] No regression in existing tests
- [ ] Performance considered
- [ ] Security implications considered

## Release Process

### Versioning

OpenRideDash uses [Semantic Versioning](https://semver.org/):

- **Major**: Breaking changes
- **Minor**: New features, backward compatible
- **Patch**: Bug fixes, minor improvements

### Release Steps

1. **Create release branch** from `main`
2. **Update version numbers** in `version.h` and `platformio.ini`
3. **Run full test suite** including hardware tests
4. **Build release binaries** for all supported hardware
5. **Create GitHub release** with changelog and binaries
6. **Update documentation** with new features
7. **Announce release** to community

## Troubleshooting

### Common Issues

#### Build Failures

- **Missing libraries**: Run `pio lib install` or check `lib_deps`
- **Compiler errors**: Check `build_flags` and `framework` settings
- **Memory overflow**: Adjust `board_build.flash_mode` or optimize code

#### Upload Failures

- **Wrong port**: Check `upload_port` in `platformio.ini`
- **Permission denied**: Add user to `dialout` group (Linux)
- **Device not in bootloader**: Hold BOOT button while uploading

#### Runtime Issues

- **Watchdog resets**: Check for blocking operations in tasks
- **Memory leaks**: Use heap debugging tools
- **CAN communication issues**: Verify termination and baud rate

### Getting Help

- **Check [Troubleshooting Guide](../support/troubleshooting.md)**
- **Search [GitHub Issues](https://github.com/open-ride-dash/firmware/issues)**
- **Ask on [Community Forum](https://forum.open-ride-dash.org)**
- **Join [Discord Server](https://discord.gg/open-ride-dash)**

## Next Steps

- **Explore [API Documentation](../api/ble-api.md)** for BLE communication
- **Review [Hardware Guide](../hardware/guide.md)** for assembly instructions
- **Check [Mobile App Development](../phases/phase3-mobile.md)** for app integration
- **Join the community** to contribute and get support

---

_Happy coding! The OpenRideDash project thrives on community contributions. Don't hesitate to ask questions, share your builds, or suggest improvements._
