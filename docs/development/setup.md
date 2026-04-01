# Development Environment Setup

## Overview

This guide provides step-by-step instructions for setting up a complete OpenRideDash development environment on Windows, macOS, and Linux. Follow these instructions to get everything you need to build, flash, and debug the firmware.

## Prerequisites

### Hardware Requirements

- **Computer** with USB port
- **ESP32-C3 Dev Board** (or compatible ESP32 with CAN)
- **USB Cable** (data capable)
- **Optional**: CAN transceiver, display, keypad for hardware testing

### Software Requirements

- **Operating System**: Windows 10/11, macOS 10.15+, or Linux (Ubuntu 20.04+ recommended)
- **Disk Space**: ~2GB for tools and dependencies
- **RAM**: 4GB minimum, 8GB recommended
- **Internet Connection** for downloading tools and libraries

## Windows Setup

### 1. Install Visual Studio Code

1. Download VS Code from [code.visualstudio.com](https://code.visualstudio.com/)
2. Run the installer with default options
3. Launch VS Code after installation

### 2. Install PlatformIO Extension

1. Open VS Code
2. Click Extensions icon (Ctrl+Shift+X)
3. Search for "PlatformIO IDE"
4. Click Install
5. Restart VS Code when prompted

### 3. Install USB Drivers

ESP32-C3 requires USB drivers for programming:

1. **For CP210x-based boards** (common):
   - Download [CP210x USB Drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
   - Run installer and follow prompts

2. **For CH340-based boards**:
   - Download [CH340 Drivers](https://sparks.gogo.co.nz/ch340.html)
   - Install according to instructions

3. **Verify Installation**:
   - Connect ESP32 board via USB
   - Open Device Manager (Win+X → Device Manager)
   - Look for "Ports (COM & LPT)" → "USB Serial Device" or similar
   - Note the COM port number (e.g., COM3)

### 4. Install Git

1. Download Git from [git-scm.com](https://git-scm.com/download/win)
2. Run installer with default options
3. Verify installation:
   ```cmd
   git --version
   ```

### 5. Install Python (Optional)

PlatformIO includes its own Python, but you may want system Python for scripts:

1. Download Python 3.9+ from [python.org](https://python.org)
2. Check "Add Python to PATH" during installation
3. Verify installation:
   ```cmd
   python --version
   ```

## macOS Setup

### 1. Install Visual Studio Code

```bash
# Using Homebrew (recommended)
brew install --cask visual-studio-code

# Or download from website
# https://code.visualstudio.com/download
```

### 2. Install PlatformIO Extension

1. Open VS Code
2. Command+Shift+P → "Extensions: Install Extensions"
3. Search for "PlatformIO IDE" and install
4. Restart VS Code

### 3. Install USB Drivers

ESP32-C3 typically works without additional drivers on macOS, but if you have issues:

```bash
# For CP210x boards
brew install --cask silicon-labs-vcp-driver

# For CH340 boards
brew install --cask wch-ch34x-usb-serial-driver
```

### 4. Install Git

```bash
# Git is usually pre-installed, but you can update:
brew install git
```

### 5. Verify Serial Port

Connect ESP32 and check the port:

```bash
ls /dev/cu.*
# Look for something like /dev/cu.usbserial-XXXX
```

## Linux Setup (Ubuntu/Debian)

### 1. Install Visual Studio Code

```bash
# Download and install .deb package
wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > packages.microsoft.gpg
sudo install -o root -g root -m 644 packages.microsoft.gpg /etc/apt/trusted.gpg.d/
sudo sh -c 'echo "deb [arch=amd64 signed-by=/etc/apt/trusted.gpg.d/packages.microsoft.gpg] https://packages.microsoft.com/repos/vscode stable main" > /etc/apt/sources.list.d/vscode.list'
sudo apt update
sudo apt install code
```

### 2. Install PlatformIO Extension

Same as other platforms: install via VS Code Extensions marketplace.

### 3. Install USB Drivers

Linux typically has built-in support, but you may need to add user to dialout group:

```bash
# Add user to dialout group for serial port access
sudo usermod -a -G dialout $USER
sudo usermod -a -G tty $USER

# Log out and back in for changes to take effect
```

### 4. Install Git and Python

```bash
sudo apt update
sudo apt install git python3 python3-pip
```

### 5. Verify Serial Port

```bash
# Connect ESP32
ls /dev/ttyUSB*
# Should show something like /dev/ttyUSB0
```

## PlatformIO Configuration

### First-Time Setup

1. **Open PlatformIO Home**:
   - Click PlatformIO icon in VS Code sidebar
   - Or press Ctrl+Shift+P → "PlatformIO: Home"

2. **Create New Project** (for testing):
   - Click "New Project"
   - Name: "test-project"
   - Board: "ESP32-C3-DevKitM-1"
   - Framework: "Arduino"
   - Location: Choose a directory
   - Click "Finish"

3. **Wait for initialization** (first time may take several minutes)

### Verify Installation

Create a simple test sketch:

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("PlatformIO test successful!");
}

void loop() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  delay(1000);
  Serial.println("Blink");
}
```

Build and upload:

1. Click checkmark (✓) in bottom bar to build
2. Click right arrow (→) to upload
3. Open serial monitor (plug icon) to see output

## Clone OpenRideDash Repository

### Using Git

```bash
# Clone the firmware repository
git clone https://github.com/open-ride-dash/firmware.git
cd firmware

# Open in VS Code
code .
```

### Using PlatformIO

1. In PlatformIO Home, click "Open Project"
2. Navigate to the cloned `firmware` directory
3. Click "Open"

## Project Configuration

### Select Build Environment

OpenRideDash supports multiple hardware configurations. Select the appropriate environment:

1. **Open `platformio.ini`**
2. **Check available environments**:

   ```ini
   [env:esp32-c3-devkit]
   ; ESP32-C3 with native CAN

   [env:esp32-mcp2515]
   ; ESP32 with MCP2515 CAN

   [env:native]
   ; Native simulation for testing
   ```

3. **Select environment** in VS Code:
   - Bottom status bar shows current environment
   - Click to change to `esp32-c3-devkit`

### Configure Serial Port

If PlatformIO doesn't auto-detect your serial port:

1. **Find port name**:
   - Windows: `COM3` or similar
   - macOS: `/dev/cu.usbserial-XXXX`
   - Linux: `/dev/ttyUSB0`

2. **Add to `platformio.ini`**:
   ```ini
   upload_port = COM3
   monitor_port = COM3
   ```

### Configure WiFi (Optional)

For OTA updates or WiFi debugging, add WiFi credentials:

1. **Create `include/secrets.h`** (not tracked by git):

   ```cpp
   #ifndef SECRETS_H
   #define SECRETS_H

   #define WIFI_SSID "your-ssid"
   #define WIFI_PASSWORD "your-password"

   #endif
   ```

2. **Add to `platformio.ini`**:

   ```ini
   build_flags =
       -DWIFI_SSID=\"${sysenv.WIFI_SSID}\"
       -DWIFI_PASSWORD=\"${sysenv.WIFI_PASSWORD}\"
   ```

3. **Set environment variables**:

   ```bash
   # Linux/macOS
   export WIFI_SSID="your-ssid"
   export WIFI_PASSWORD="your-password"

   # Windows (Command Prompt)
   set WIFI_SSID=your-ssid
   set WIFI_PASSWORD=your-password

   # Windows (PowerShell)
   $env:WIFI_SSID="your-ssid"
   $env:WIFI_PASSWORD="your-password"
   ```

## Build and Upload

### First Build

1. **Clean build** (recommended for first time):
   - PlatformIO sidebar → Project Tasks → esp32-c3-devkit → Clean
   - Or terminal: `pio run -t clean`

2. **Build project**:
   - Click checkmark (✓) in bottom bar
   - Or terminal: `pio run`

3. **Upload to device**:
   - Click right arrow (→) in bottom bar
   - Or terminal: `pio run -t upload`

### Monitor Serial Output

1. **Open Serial Monitor**:
   - Click plug icon (🔌) in bottom bar
   - Or PlatformIO sidebar → Project Tasks → Monitor

2. **Configure baud rate** (115200 by default):
   ```ini
   monitor_speed = 115200
   ```

## Testing the Setup

### Hardware Test

1. **Connect all components**:
   - ESP32 board
   - CAN transceiver (if available)
   - Display (if available)
   - Keypad (if available)

2. **Upload test firmware**:

   ```bash
   pio run -e esp32-c3-devkit -t upload
   ```

3. **Verify operation**:
   - LED should blink
   - Serial monitor should show startup messages
   - Display should show welcome screen (if connected)

### Software Test

Run unit tests to verify development environment:

```bash
# Run native tests (no hardware required)
pio test -e native

# Run hardware tests (requires connected device)
pio test -e esp32-c3-devkit
```

## Troubleshooting Setup Issues

### Common Problems

#### PlatformIO Not Installing

- **Solution**: Check internet connection, disable firewall/vpn temporarily
- **Alternative**: Install PlatformIO Core CLI separately

#### Upload Permission Denied

- **Linux**: Ensure user is in `dialout` group, logout and login again
- **macOS**: May need to approve kernel extension (System Preferences → Security)
- **Windows**: Try different USB port, reinstall drivers

#### Board Not Detected

- **Check USB cable**: Some cables are power-only, try a different cable
- **Check drivers**: Reinstall USB-to-serial drivers
- **Try different USB port**: Some USB 3.0 ports have compatibility issues

#### Build Errors

- **Clean build**: `pio run -t clean`
- **Update PlatformIO**: `pio upgrade`
- **Check library versions**: Update `lib_deps` in `platformio.ini`

### Getting Help

- **PlatformIO Documentation**: [docs.platformio.org](https://docs.platformio.org)
- **ESP32 Forum**: [esp32.com](https://esp32.com)
- **OpenRideDash Discord**: [discord.gg/open-ride-dash](https://discord.gg/open-ride-dash)
- **GitHub Issues**: [github.com/open-ride-dash/firmware/issues](https://github.com/open-ride-dash/firmware/issues)

## Next Steps

Now that your development environment is set up:

1. **Explore the codebase** - Review project structure
2. **Run examples** - Try the example sketches in `examples/` directory
3. **Modify configuration** - Adjust settings for your hardware
4. **Start developing** - Implement new features or fix bugs
5. **Join the community** - Share your progress and get help

## Additional Resources

- **[PlatformIO Tutorial](https://docs.platformio.org/en/latest/tutorials/index.html)** - Comprehensive PlatformIO guide
- **[ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)** - Hardware specifications
- **[Arduino Core for ESP32](https://github.com/espressif/arduino-esp32)** - Framework documentation
- **[OpenRideDash Hardware Guide](../hardware/guide.md)** - Hardware assembly instructions

---

_Your development environment is now ready! If you encounter any issues, don't hesitate to ask for help in the community forums. Happy coding!_
