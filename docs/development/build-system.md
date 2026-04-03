# Build System Configuration

## Overview

OpenRideDash uses PlatformIO as its primary build system, providing a consistent development experience across different hardware configurations. This document explains the build system configuration, customization options, and advanced build features.

## PlatformIO Configuration

### Core Configuration File

The main configuration file is `platformio.ini` located in the project root. This file defines:

- **Build environments** for different hardware configurations
- **Library dependencies** and versions
- **Compiler flags** and optimization settings
- **Upload settings** for different devices
- **Test configurations** for unit and integration tests

### Basic Configuration Structure

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

monitor_speed = 115200
```

## Build Environments

### Available Environments

OpenRideDash supports multiple hardware configurations through different build environments:

#### ESP32-C3 with Native CAN

```ini
[env:esp32-c3-devkit]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
```

#### ESP32 with MCP2515 CAN

```ini
[env:esp32-mcp2515]
platform = espressif32
board = esp32-devkitc
framework = arduino
build_flags =
    -DUSE_MCP2515
    -DUSE_ST7789
```

#### Native Simulation (No Hardware)

```ini
[env:native]
platform = native
framework = arduino
build_flags =
    -DSIMULATION_MODE
```

#### ESP32-S3 (Future Support)

```ini
[env:esp32-s3-devkit]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
```

### Environment Selection

Select environment based on your hardware:

1. **In VS Code**: Click environment name in bottom status bar
2. **Via CLI**: Use `-e` flag: `pio run -e esp32-c3-devkit`
3. **Default environment**: First environment in `platformio.ini`

## Build Flags

### Feature Flags

Feature flags control which components are included in the build:

```ini
build_flags =
    # Display selection
    -DUSE_ST7789
    # -DUSE_ILI9341

    # CAN controller selection
    -DUSE_ESP32_CAN
    # -DUSE_MCP2515

    # Optional features
    -DUSE_BLE
    -DUSE_SENSORS
    -DUSE_WIFI

    # Debugging
    -DLOG_LEVEL=3          # 0=off, 1=error, 2=warning, 3=info, 4=debug

    # Performance tuning
    -DOPTIMIZE_SPI
    -DUSE_DMA
```

### Common Build Flags

| Flag            | Description                        | Default              |
| --------------- | ---------------------------------- | -------------------- |
| `USE_ST7789`    | ST7789 display driver              | Enabled              |
| `USE_ILI9341`   | ILI9341 display driver             | Disabled             |
| `USE_ESP32_CAN` | Native ESP32 CAN controller        | Enabled for ESP32-C3 |
| `USE_MCP2515`   | External MCP2515 CAN controller    | Disabled             |
| `USE_BLE`       | Bluetooth Low Energy support       | Enabled              |
| `USE_SENSORS`   | Sensor support (temp, light, etc.) | Disabled             |
| `USE_WIFI`      | WiFi support for OTA updates       | Disabled             |
| `LOG_LEVEL`     | Debug logging verbosity            | 3 (Info)             |
| `OPTIMIZE_SPI`  | SPI optimization for display       | Enabled              |
| `USE_DMA`       | Use DMA for SPI transfers          | Disabled             |

### Conditional Compilation

In code, use feature flags like:

```cpp
#if defined(USE_ST7789)
    #include "ST7789.h"
    ST7789 display;
#endif

#if defined(USE_BLE)
    #include "BLEServer.h"
    BLEServer bleServer;
#endif
```

## Library Management

### Library Dependencies

PlatformIO automatically downloads and manages libraries specified in `lib_deps`:

```ini
lib_deps =
    # CAN libraries
    arduino-libraries/Arduino-CAN@^0.4.0

    # Display libraries
    bodmer/TFT_eSPI@^2.5.0

    # BLE libraries
    https://github.com/open-ride-dash/blelib.git

    # Utility libraries
    arduino-libraries/ArduinoLog@^1.0.0
```

### Library Versions

Specify version constraints:

- `@^1.0.0`: Version 1.0.0 or higher, but less than 2.0.0
- `@~1.0.0`: Version 1.0.0 or higher, but less than 1.1.0
- `@1.0.0`: Exactly version 1.0.0

### Local Libraries

For custom libraries not in PlatformIO registry:

```ini
lib_deps =
    file://../libs/custom-library
    git+https://github.com/user/repo.git
```

## Compiler Optimization

### Optimization Levels

PlatformIO uses GCC optimization flags. Configure via `build_flags`:

```ini
build_flags =
    -Os                  # Optimize for size (default)
    # -O2                # Optimize for performance
    # -O3                # Aggressive optimization
    # -Og                # Optimize for debugging
```

### ESP32-Specific Optimizations

```ini
build_flags =
    # Memory optimization
    -DCONFIG_FREERTOS_UNICORE=1

    # Flash optimization
    -DCONFIG_SPIRAM_SUPPORT=0

    # Performance tuning
    -DCONFIG_ESP32_DEFAULT_CPU_FREQ_160=1
```

### Linker Optimization

Control memory allocation and linking:

```ini
board_build.ldscript = eagle.flash.4m.ld
board_build.flash_mode = dio
board_build.partitions = default_4MB.csv
```

## Memory Configuration

### Partition Tables

ESP32 uses partition tables to allocate flash memory. OpenRideDash includes custom partitions:

```ini
board_build.partitions = partitions.csv
```

Example partition table (`partitions.csv`):

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
phy_init, data, phy,     0xe000,  0x1000,
factory,  app,  factory, 0x10000, 1M,
ota_0,    app,  ota_0,   0x110000, 1M,
ota_1,    app,  ota_1,   0x210000, 1M,
ffat,     data, fat,     0x310000, 1M,
```

### Memory Usage Monitoring

Check memory usage after build:

```bash
pio run -t size
```

Output example:

```
Memory Usage -> ESP32-C3
----------------
RAM:   [=         ]  25.8% (used 105408 bytes from 408960 bytes)
Flash: [===       ]  34.5% (used 452624 bytes from 1310720 bytes)
```

## Build Process

### Build Stages

1. **Preprocessing**: Expand macros, process headers
2. **Compilation**: Convert source to object files
3. **Linking**: Combine objects into executable
4. **ELF Processing**: Create firmware binary
5. **Post-processing**: Generate binaries for upload

### Build Output

Build artifacts are stored in:

- `.pio/build/<env>/firmware.bin` - Main firmware binary
- `.pio/build/<env>/firmware.elf` - ELF file for debugging
- `.pio/build/<env>/partitions.bin` - Partition table binary

### Incremental Builds

PlatformIO performs incremental builds automatically:

- Only rebuilds changed files
- Cache used for unchanged dependencies
- Clean build: `pio run -t clean`

## Advanced Build Features

### Multi-environment Builds

Build all environments simultaneously:

```bash
pio run
```

Build specific environments:

```bash
pio run -e esp32-c3-devkit -e esp32-mcp2515
```

### Custom Build Scripts

Add custom build steps via `extra_scripts`:

```ini
extra_scripts =
    pre:scripts/pre_build.py
    post:scripts/post_build.py
    pre:scripts/check_size.py
```

Example pre-build script (`scripts/pre_build.py`):

```python
Import("env")

def check_build_flags():
    print("Checking build flags...")
    flags = env.GetProjectOption("build_flags")
    print(f"Build flags: {flags}")

env.AddPreAction("build", check_build_flags)
```

### Environment Variables

Use environment variables in configuration:

```ini
build_flags =
    -DWIFI_SSID=\"${sysenv.WIFI_SSID}\"
    -DWIFI_PASSWORD=\"${sysenv.WIFI_PASSWORD}\"
```

Set environment variables:

```bash
export WIFI_SSID="my-network"
export WIFI_PASSWORD="my-password"
```

### Conditional Configuration

Conditional configuration based on environment:

```ini
[env:esp32-c3-devkit]
build_flags =
    ${common.build_flags}
    -DUSE_ESP32_CAN

[env:esp32-mcp2515]
build_flags =
    ${common.build_flags}
    -DUSE_MCP2515

[env:common]
build_flags =
    -DUSE_ST7789
    -DLOG_LEVEL=3
```

## Testing Integration

### Unit Tests

Configure test environment:

```ini
[env:native]
platform = native
framework = arduino
test_framework = unity
lib_deps =
    unity@^2.5.2
```

Run tests:

```bash
pio test -e native
```

### Integration Tests

Integration tests require hardware:

```ini
[env:test-integration]
extends = esp32-c3-devkit
test_framework = custom
lib_deps =
    testlib@file://../tests/integration
```

## Upload Configuration

### Serial Upload

Configure serial upload parameters:

```ini
upload_port = COM3          # Windows
upload_port = /dev/ttyUSB0  # Linux
upload_port = /dev/cu.usbserial-XXXX  # macOS

upload_speed = 921600
upload_protocol = esp32c3
```

### OTA Upload

Configure OTA upload via WiFi:

```ini
upload_port = 192.168.1.100
upload_protocol = espota
upload_flags =
    --auth=password
```

### Custom Upload Scripts

Add custom upload actions:

```ini
extra_scripts =
    post:scripts/upload_validation.py
```

## Monitoring Configuration

### Serial Monitor

Configure serial monitor:

```ini
monitor_port = ${upload_port}
monitor_speed = 115200
monitor_filters =
    colorize
    esp32_exception_decoder
    log2file
```

### Custom Monitor Filters

Add custom filters:

```ini
monitor_filters =
    direct
    time
    send_on_enter
    echo
```

## CI/CD Integration

### GitHub Actions

Example GitHub Actions workflow for automated builds:

```yaml
name: PlatformIO CI
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions/setup-python@v2
      - name: Build
        run: |
          pip install platformio
          pio run
      - name: Test
        run: pio test -e native
```

### Automated Release Builds

Build for release with specific configuration:

```ini
[env:release]
extends = esp32-c3-devkit
build_flags =
    ${esp32-c3-devkit.build_flags}
    -DRELEASE_BUILD=1
    -DLOG_LEVEL=1
```

## Performance Optimization

### Build Time Optimization

Reduce build time:

1. **Parallel builds**: PlatformIO builds in parallel automatically
2. **Cache optimization**: Use `pio cache` commands
3. **Selective compilation**: Only build needed environments

### Binary Size Optimization

Reduce firmware size:

1. **Remove debug symbols**: `-DLOG_LEVEL=0` for release
2. **Optimize for size**: Use `-Os` compiler flag
3. **Strip unused code**: Enable LTO (Link Time Optimization)

```ini
build_flags =
    -Os
    -flto
    -DNDEBUG
```

## Troubleshooting Build Issues

### Common Build Problems

#### Missing Libraries

```
Error: Could not find library X
```

Solution: Check `lib_deps`, run `pio lib install`

#### Compiler Errors

```
Error: undefined reference to function Y
```

Solution: Check if source file is included in build, verify function exists

#### Memory Overflow

```
Error: section `.iram1.text' will not fit in region
```

Solution: Reduce IRAM usage, move functions to flash with `IRAM_ATTR`

#### Flash Overflow

```
Error: region `flash' overflowed
```

Solution: Enable compression, optimize code size, use larger flash mode

### Debugging Build Process

Use verbose output:

```bash
pio run -v
```

Check build configuration:

```bash
pio run --dump-config
```

## Customization Examples

### Custom Hardware Configuration

Example for custom display with different pins:

```ini
[env:custom-display]
extends = esp32-c3-devkit
build_flags =
    ${esp32-c3-devkit.build_flags}
    -DDISPLAY_TYPE=CUSTOM
    -DDISPLAY_SCK_PIN=14
    -DDISPLAY_MOSI_PIN=13
    -DDISPLAY_CS_PIN=15
```

### Production Build Configuration

Example for production release:

```ini
[env:production]
extends = esp32-c3-devkit
build_flags =
    -DUSE_ST7789
    -DUSE_ESP32_CAN
    -DUSE_BLE
    -DLOG_LEVEL=1          # Only errors
    -DRELEASE_BUILD=1
    -Os                    # Optimize for size
    -DNDEBUG               # Remove debug code

board_build.flash_mode = dio
board_build.flash_size = 4MB
```

## Migration from Arduino IDE

### Comparison

| Feature               | PlatformIO           | Arduino IDE    |
| --------------------- | -------------------- | -------------- |
| Library management    | Automatic, versioned | Manual, global |
| Multiple environments | Yes                  | No             |
| Build customization   | Extensive            | Limited        |
| Testing integration   | Built-in             | Manual         |
| CI/CD support         | Excellent            | Limited        |

### Migration Steps

1. **Export Arduino project** to PlatformIO structure
2. **Convert libraries** to `lib_deps` format
3. **Configure build flags** equivalent to Arduino settings
4. **Test build** and verify functionality

## Next Steps

After configuring the build system:

1. **Explore [Development Guide](guide.md)** for development practices
2. **Check [Setup Guide](setup.md)** for environment setup
3. **Review [Optimization Guide](optimization.md)** for performance tuning
4. **Join the community** for build system discussions

## Additional Resources

- **[PlatformIO Documentation](https://docs.platformio.org)** - Official PlatformIO docs
- **[ESP32 Build System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html)** - ESP-IDF build system
- **[OpenRideDash Build Examples](https://github.com/open-ride-dash/firmware/tree/main/examples)** - Example configurations
- **[Community Build Configurations](https://forum.open-ride-dash.org/c/build-configs)** - Shared configurations

---

_The build system is a powerful tool for managing OpenRideDash development. Proper configuration ensures consistent builds across different hardware and simplifies the development process._
