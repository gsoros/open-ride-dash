# Getting Started

Welcome to OpenRideDash! This guide will help you get up and running with your own OpenRideDash display system.

## Quick Start

### For Users (Pre-built Firmware)

1. **Purchase or gather components** (see [Hardware Guide](hardware/guide.md))
2. **Assemble hardware** (see [Assembly Instructions](hardware/assembly.md))
3. **Download latest firmware** from [Releases](https://github.com/open-ride-dash/firmware/releases)
4. **Flash firmware** using [PlatformIO](development/setup.md) or ESP Flash Tool
5. **Connect to e-bike** and power on
6. **Configure settings** via mobile app or serial interface

### For Developers

1. **Set up development environment** ([Development Setup](development/setup.md))
2. **Clone repository**:
   ```bash
   git clone https://github.com/open-ride-dash/firmware.git
   cd firmware
   ```
3. **Build and upload**:
   ```bash
   pio run -e esp32-c3-devkit -t upload
   ```
4. **Monitor output**:
   ```bash
   pio device monitor
   ```

## Step-by-Step Guide

### Step 1: Hardware Preparation

#### Required Components

- ESP32-C3 development board with CAN
- Buck converter (80V to 5V)
- CAN transceiver (SN65HVD230)
- Display (ST7789 or compatible)
- Keypad (3 to 6-button membrane)
- Enclosure materials

#### Assembly

Follow the [Hardware Assembly](hardware/assembly.md) guide for detailed instructions.

### Step 2: Software Installation

#### Option A: Pre-built Firmware (Easiest)

1. Download the latest `.bin` file from GitHub Releases
2. Use ESP Flash Download Tool or `esptool.py` to flash:
   ```bash
   esptool.py --chip esp32c3 --port /dev/ttyUSB0 write_flash 0x0 firmware.bin
   ```

#### Option B: Build from Source (Recommended for customization)

1. Install [PlatformIO](development/setup.md)
2. Clone the repository
3. Configure for your hardware in `platformio.ini`
4. Build and upload

### Step 3: Initial Configuration

#### Serial Configuration

1. Connect via USB serial (115200 baud)
2. Use terminal to access configuration menu:
   ```
   OpenRideDash Configuration Menu
   -------------------------------
   1. Set CAN baud rate
   2. Configure display
   3. Set button mappings
   4. Save configuration
   ```

#### Mobile App Configuration

1. Install OpenRideDash mobile app (iOS/Android)
2. Enable Bluetooth on your phone
3. Connect to "OpenRideDash-XXXX" device
4. Use app to configure all settings

### Step 4: Integration with E-Bike

#### CAN Bus Connection

1. **Identify CAN wires** on your e-bike controller
   - Typically: CAN_H (yellow), CAN_L (green), GND (black)
2. **Connect to OpenRideDash**:
   - CAN_H → CAN_H
   - CAN_L → CAN_L
   - GND → Common ground
3. **Add termination resistor** (120Ω) if OpenRideDash is at end of bus

#### Power Connection

1. **Connect buck converter input** to battery:
   - Positive (red) to battery positive
   - Negative (black) to battery negative
2. **Verify output voltage** (5.0V ±0.2V)
3. **Connect to OpenRideDash** 5V and GND

### Step 5: Testing

#### Basic Function Test

1. Power on the system
2. Verify display shows startup screen
3. Check serial output for errors
4. Test each button
5. Verify CAN communication (if controller powered)

#### Road Test (Safe Environment)

1. **Start with walk-along test** - Push bike, verify speed reading
2. **Low-speed test** - Ride slowly, check all functions
3. **Full functionality test** - Test PAS levels, lights, etc.

## Configuration Examples

### Basic Configuration (Bafang M560)

```ini
# platformio.ini
build_flags =
    -DUSE_ST7789
    -DUSE_ESP32_CAN
    -DCAN_BAUDRATE=500000
    -DCONTROLLER_TYPE=BAFANG_M560
```

### Advanced Configuration (Custom Display)

```cpp
// include/config.h
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 320
#define DISPLAY_ROTATION 1
#define BACKLIGHT_PIN 21
#define BACKLIGHT_PWM true
```

## Troubleshooting First Steps

### Common Issues

#### No Power

- Check input voltage (should be 36V-80V)
- Verify buck converter output (5V)
- Check fuse (if installed)

#### Display Not Working

- Check contrast setting (may be too low)
- Verify SPI connections
- Check backlight power

#### CAN Not Communicating

- Verify termination resistor (120Ω)
- Check baud rate (typically 500kbps)
- Ensure CAN_H and CAN_L not swapped

#### Buttons Not Responding

- Check GPIO assignments in `pins.h`
- Verify pull-up resistors
- Add debouncing capacitors (0.1µF)

### Getting Help

If you're stuck:

1. **Check [Troubleshooting Guide](support/troubleshooting.md)**
2. **Search [GitHub Issues](https://github.com/open-ride-dash/firmware/issues)**
3. **Ask on [Community Forum](https://forum.open-ride-dash.org)**
4. **Join [Discord](https://discord.gg/open-ride-dash)**

## Next Steps

After successful setup:

1. **Explore features** - Try different display modes, data logging
2. **Customize** - Modify firmware for your specific needs
3. **Contribute** - Share improvements with the community
4. **Build another** - Help friends build their own OpenRideDash

## Additional Resources

- **[Hardware Guide](hardware/guide.md)** - Detailed component information
- **[Development Guide](development/guide.md)** - Advanced customization
- **[API Documentation](api/ble-api.md)** - BLE communication protocol
- **[Mobile App Guide](phases/phase3-mobile.md)** - App features and usage

---

_Congratulations on setting up OpenRideDash! Remember to ride safely and always test new features in a controlled environment before relying on them during rides._
