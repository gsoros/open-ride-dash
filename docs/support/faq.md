# Frequently Asked Questions (FAQ)

## General Questions

### What is OpenRideDash?

OpenRideDash is an open-source, modular firmware and hardware platform for ESP32-based e-bike displays. It communicates with e-bike controllers over CAN bus, provides Bluetooth connectivity for mobile apps, and offers customizable display options.

### Who is OpenRideDash for?

- **E-bike enthusiasts** who want more control over their display
- **DIY builders** creating custom e-bikes
- **Developers** interested in embedded systems and IoT
- **Researchers** studying e-bike telemetry and efficiency
- **Small manufacturers** needing customizable display solutions

### Is OpenRideDash compatible with my e-bike?

OpenRideDash is designed to work with any e-bike controller that uses CAN bus communication. It has been specifically tested with Bafang systems (M560) but can be adapted to other controllers with protocol documentation.

### Do I need programming skills to use OpenRideDash?

Basic skills are helpful but not required:

- **Basic users**: Can use pre-built firmware with default settings
- **Intermediate users**: Can customize configuration via mobile app
- **Advanced users**: Can modify firmware code for custom features

### Is OpenRideDash safe to use on my e-bike?

OpenRideDash is designed with safety in mind but comes with important disclaimers:

- **Not certified** for road use in any jurisdiction
- **Use at your own risk** - improper installation could affect bike operation
- **Always test** in controlled environment before road use
- **Keep original display** as backup for critical rides

## Hardware Questions

### What hardware do I need to build OpenRideDash?

Minimum components:

1. ESP32-C3 development board (with CAN)
2. Buck converter (80V to 5V)
3. CAN transceiver (SN65HVD230)
4. Display (ST7789 or compatible)
5. Keypad (3 to 6-button membrane)
6. Enclosure materials

See the [Hardware Guide](../hardware/guide.md) for complete bill of materials.

### Can I use ESP32 instead of ESP32-C3?

Yes, but ESP32 doesn't have native CAN controller. You'll need:

- External CAN controller (MCP2515)
- Additional SPI connection
- Slightly different firmware configuration

### What display types are supported?

Currently supported:

- **ST7789** (sunlight readable)

More displays can be added by implementing the display interface.

### How do I power OpenRideDash from my e-bike battery?

Connect the buck converter input to your battery (typically 36V, 48V, or 52V). Ensure:

- Input voltage range matches your battery (20V-80V typical)
- Proper polarity (positive to positive, negative to negative)
- Fuse protection (5A recommended)
- Waterproof connections

### Can I add sensors to OpenRideDash?

Yes, supported sensors include:

- Temperature (DS18B20)
- Light (BH1750)
- Accelerometer (MPU6050)
- GPS (NEO-6M)
- Heart rate monitor (via BLE)

### How weatherproof is the enclosure?

The reference design uses epoxy potting for complete waterproofing, achieving approximately IP67 rating. However, this depends on proper assembly. Connectors should be waterproof automotive types.

## Firmware Questions

### How do I update the firmware?

Three methods:

1. **USB Serial** (development) - Using PlatformIO or Arduino IDE
2. **OTA via BLE** (production) - Using mobile app
3. **OTA via WiFi** (optional) - If WiFi is enabled

### Can I customize the display layout?

Yes, display layouts are fully customizable:

- Modify the display task code
- Create custom screens with different data arrangements
- Add graphics or icons
- Adjust refresh rates

### How do I configure CAN bus parameters?

Configuration options:

- **Baud rate** (125k, 250k, 500k, 1M)
- **Filters** to ignore unwanted messages
- **Mode** (normal, listen-only, loopback)
- **Retry behavior** on transmission failure

### What programming languages are used?

- **Firmware**: C++ (Arduino framework)
- **Mobile App**: Dart (Flutter framework)
- **Tools**: Python for build scripts and utilities

### Can I contribute to the firmware?

Absolutely! OpenRideDash is open source. Contributions are welcome:

1. Fork the repository on GitHub
2. Make your changes
3. Submit a pull request
4. See [Contributing Guide](../../CONTRIBUTING.md) for details

## Mobile App Questions

### Is there a mobile app for OpenRideDash?

Yes, a Flutter-based mobile app is available for:

- **iOS** (TestFlight, eventually App Store)
- **Android** (Google Play, APK download)

### What can the mobile app do?

- Discover and connect to OpenRideDash via BLE
- Display real-time telemetry (speed, battery, power, etc.)
- Configure system settings
- Record ride data (GPX export)
- Initiate firmware updates (OTA)
- View ride history and statistics

### Is the mobile app open source?

Yes, the mobile app source code is available on GitHub under the same open-source license.

### Can I use third-party apps with OpenRideDash?

Yes, OpenRideDash exposes standard BLE services:

- **Cycling Power Service** (for fitness apps like Strava, Wahoo)
- **Device Information Service**
- **Battery Service**

Many fitness apps can connect directly to read power and speed data.

## CAN Protocol Questions

### What CAN protocol does OpenRideDash use?

OpenRideDash implements the Bafang CAN protocol (reverse-engineered). It can be extended to support other protocols by implementing the `Controller` interface.

### How do I find the right CAN messages for my controller?

1. **Use a CAN sniffer** (CANable, PCAN-USB)
2. **Capture data** while operating the bike
3. **Identify patterns** (voltage changes with throttle, speed changes)
4. **Test with OpenRideDash** using the captured IDs

### Can OpenRideDash work with non-Bafang controllers?

Yes, but you'll need to:

1. Document the CAN protocol for your controller
2. Implement a protocol parser
3. Add to the firmware (or create a plugin)
4. Test thoroughly

### What CAN bus speed should I use?

Most e-bike controllers use 500kbps. Check your controller documentation or sniff the bus to confirm.

## Performance Questions

### How accurate are the readings?

Accuracy depends on components:

- **Voltage**: ±0.1V with proper calibration
- **Current**: ±0.5A typical (depends on shunt accuracy)
- **Speed**: ±0.5km/h (depends on wheel size setting)
- **Temperature**: ±1°C (DS18B20 specification)

### What's the refresh rate of the display?

Configurable, typically:

- **Telemetry values**: 5-10Hz
- **Display full refresh**: 30Hz
- **Graphs/animations**: 20-30Hz

### How much power does OpenRideDash consume?

Typical consumption:

- **Active operation**: 150-250mA @ 5V
- **BLE connected**: +20mA
- **Display backlight**: +50mA (max)
- **Deep sleep**: < 100µA

### What's the maximum range for BLE communication?

Line of sight range:

- **Indoor**: 10-20 meters
- **Outdoor**: 20-50 meters
- **With obstacles**: 5-15 meters

Range can be improved with external antenna (not typically needed for handlebar mounting).

## Troubleshooting Questions

### My device won't power on. What should I check?

1. **Input voltage** - Measure at battery connector
2. **Buck converter output** - Should be 5V
3. **Fuse** - Check if blown
4. **Polarity** - Ensure correct connection
5. **Short circuits** - Check for wiring errors

See [Troubleshooting Guide](troubleshooting.md) for detailed steps.

### The display shows nothing. What's wrong?

Common causes:

1. **Contrast setting** too high/low
2. **Backlight not powered** (check backlight connection)
3. **SPI connection** issue (check CS, DC, RESET pins)
4. **Display initialization** failed (check serial logs)

### CAN communication isn't working. How do I debug?

1. **Check termination** - 120Ω resistor if end-of-line
2. **Verify baud rate** - Must match controller (usually 500kbps)
3. **Check CAN_H/CAN_L** - Not swapped, proper voltage (2.5V differential)
4. **Test with known-good device** - Verify your CAN hardware works

### BLE isn't showing up in my phone. What to do?

1. **Ensure BLE is enabled** in firmware configuration
2. **Check advertising interval** (may be too long)
3. **Verify ESP32 antenna** is not obstructed
4. **Try different BLE scanner app** (nRF Connect recommended)
5. **Check for interference** from other 2.4GHz devices

### Buttons aren't working. How do I fix?

1. **Check GPIO configuration** in `pins.h`
2. **Verify pull-up resistors** (internal or external)
3. **Add debouncing capacitors** (0.1µF across button)
4. **Test continuity** when button pressed
5. **Check for conflicting GPIO usage**

## Development Questions

### How do I set up a development environment?

Follow the [Development Setup Guide](../development/setup.md) for step-by-step instructions for Windows, macOS, and Linux.

### What IDE is recommended?

**Visual Studio Code with PlatformIO extension** is the recommended setup. Alternatives:

- **PlatformIO CLI** for command-line development
- **Arduino IDE** with ESP32 board support (limited features)

### How do I add a new feature?

General process:

1. **Understand the architecture** (task-based, event-driven)
2. **Create interface** (if hardware-related)
3. **Implement concrete class**
4. **Add to factory functions**
5. **Create task** (if needed)
6. **Test thoroughly**
7. **Document** the feature

### How do I debug the firmware?

Multiple methods:

- **Serial logging** (primary method)
- **GDB debugging** with JTAG or serial
- **Logic analyzer** for hardware signals
- **Unit tests** for isolated components

### Can I run OpenRideDash in simulation?

Yes, there's a native build environment that simulates hardware:

```bash
pio run -e native
pio test -e native
```

This allows testing logic without physical hardware.

## Community & Support

### How can I contribute?

Many ways to contribute:

- **Code** - Fix bugs, add features
- **Documentation** - Improve guides, add examples
- **Testing** - Test on different hardware, report issues
- **Translation** - Translate documentation/app
- **Hardware** - Design PCB improvements, enclosures

### Can I use OpenRideDash commercially?

Yes, under the terms of the open-source license (GPL). You must:

- Include the license and copyright notice
- State changes if you modify the code
- Not hold the original authors liable

Commercial support and customization services may be available from community members.

## Legal & Safety

### Is OpenRideDash legal to use on public roads?

Regulations vary by country and region. Generally:

- **Display-only systems** are usually unregulated
- **Modifications to motor controller** may affect type approval
- **Check local laws** regarding e-bike modifications
- **Consult with authorities** if unsure

### What safety precautions should I take?

1. **Electrical safety** - High voltage (up to 80V DC) can be dangerous
2. **Mechanical safety** - Secure mounting to prevent detachment
3. **Software safety** - Test thoroughly before relying on critical functions
4. **Backup systems** - Keep original display as fallback
5. **Regular inspection** - Check connections and enclosure integrity

### What warranty does OpenRideDash have?

OpenRideDash is provided "as is" without any warranty. See the LICENSE file for full terms. Community members may offer support but no guarantees.

### Can OpenRideDash damage my e-bike?

Improper installation or software bugs could potentially cause issues:

- **Incorrect CAN messages** might confuse controller
- **Power supply issues** could affect other electronics
- **Always test** with caution and monitor system behavior

## Miscellaneous

### How long does it take to build?

Experience level matters:

- **Experienced builder**: 8-16 hours
- **Intermediate**: 16-32 hours
- **Beginner**: 32+ hours (including learning time)

### Can I buy a pre-built OpenRideDash?

Currently, OpenRideDash is DIY-only. However, community members may offer assembly services. Check the forum for options.

### What's the difference between OpenRideDash and commercial displays?

| Feature       | OpenRideDash        | Commercial Displays   |
| ------------- | ------------------- | --------------------- |
| Customization | Full control        | Limited or none       |
| Openness      | Fully open          | Usually closed        |
| Features      | Community-driven    | Manufacturer-defined  |
| Support       | Community           | Manufacturer warranty |
| Updates       | Frequent, community | Rare, manufacturer    |

### Can I use OpenRideDash on other vehicles?

Potentially yes, with modifications:

- **Electric scooters** - Similar voltage ranges
- **Electric motorcycles** - Higher power, similar CAN protocols
- **Solar vehicles** - Custom telemetry needs
- **Data logging** - General CAN bus monitoring

---

_Have a question not answered here? Please ask on the [Community Forum](https://forum.open-ride-dash.org) so we can add it to this FAQ!_
