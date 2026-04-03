# Troubleshooting Guide

## Introduction

This guide helps you diagnose and fix common issues with OpenRideDash hardware and firmware. If you encounter problems not covered here, please check the [FAQ](faq.md) or ask for help on the [community forum](https://forum.open-ride-dash.org).

## Quick Reference

### Common Symptoms and Solutions

| Symptom              | Possible Causes                 | Quick Fix                                |
| -------------------- | ------------------------------- | ---------------------------------------- |
| No power             | Dead battery, loose connection  | Check input voltage, verify connections  |
| Display blank        | Power issue, contrast setting   | Adjust contrast, check backlight         |
| CAN not working      | Wrong baud rate, no termination | Verify 500kbps, add 120Ω resistor        |
| BLE not visible      | Not advertising, out of range   | Reset device, check advertising interval |
| Buttons unresponsive | GPIO configuration, debouncing  | Check pin assignments, add capacitors    |
| Frequent resets      | Power instability, watchdog     | Add capacitors, check power supply       |

## Power Issues

### Device Won't Power On

**Symptoms:**

- No LED illumination
- No response from serial port
- Display completely dark

**Diagnosis Steps:**

1. **Check Input Voltage**
   - Measure voltage at input terminals (should be 36V-80V)
   - If using bench supply, verify it's providing power
   - Check polarity (positive to positive, negative to negative)

2. **Check Buck Converter**
   - Measure output voltage (should be 5.0V ±0.2V)
   - If no output, adjust potentiometer (if present)
   - Check for overheating or burning smell

3. **Check ESP32 Power**
   - Measure voltage at ESP32 VIN pin (should be 5V)
   - Check 3.3V regulator output (if using external regulator)
   - Verify decoupling capacitors are installed

4. **Check for Shorts**
   - Use multimeter in continuity mode
   - Check between 5V and GND (should not be shorted)
   - Check between 3.3V and GND

**Solutions:**

- **Replace blown fuse** (if installed)
- **Re-solder cold joints** on power connections
- **Replace buck converter** if output is unstable
- **Check input protection diodes** for shorts

### Device Powers On But Resets Frequently

**Symptoms:**

- Device starts, then restarts after few seconds
- Serial monitor shows watchdog resets
- LED blinks rapidly

**Possible Causes:**

1. **Insufficient Power Supply**
   - Current draw exceeds supply capability
   - Voltage drops under load

2. **Software Issue**
   - Watchdog timer triggered by stuck task
   - Stack overflow
   - Memory corruption

3. **Hardware Issue**
   - Bad capacitor causing voltage spikes
   - Ground loop or noise

**Diagnosis:**

- Monitor serial output for reset reasons
- Measure current draw during operation
- Check for error messages in logs

**Solutions:**

- **Increase power supply capacity** (minimum 1A recommended)
- **Add bulk capacitor** (100-470µF) at input
- **Check task priorities** and watchdog timeouts
- **Enable core dumps** for crash analysis

## Display Issues

### Blank Display (No Backlight)

**Symptoms:**

- Display appears completely black
- No visible pixels even with flashlight

**Diagnosis:**

1. **Check Power to Display**
   - Measure voltage at display VCC pin
   - Verify backlight voltage (may be separate)

2. **Check Control Signals**
   - Verify RESET pin is pulled high
   - Check CS (chip select) is active low
   - Verify SPI clock and data lines

3. **Check Initialization**
   - Serial monitor should show display init messages
   - Look for initialization errors

**Solutions:**

- **Adjust contrast** via software (often too high/low)
- **Check SPI frequency** (too high can cause issues)
- **Verify display driver** matches your display
- **Re-seat display connector**

### Display Shows Garbage/Corruption

**Symptoms:**

- Random pixels or patterns
- Text unreadable
- Screen flickering

**Possible Causes:**

- **SPI timing issues** (clock speed too high)
- **Noise on data lines**
- **Incorrect display initialization**
- **Memory corruption in display buffer**

**Solutions:**

- **Reduce SPI clock speed** in display driver
- **Add pull-up resistors** on SPI lines (10kΩ)
- **Check ground connections** between ESP32 and display
- **Verify display rotation** setting

### Backlight Issues

**Symptoms:**

- Backlight too dim or too bright
- Backlight flickering
- Backlight not turning on

**Solutions:**

- **Adjust backlight PWM frequency** (500Hz-1kHz recommended)
- **Check backlight driver circuit** (transistor/mosfet)
- **Verify backlight voltage** (may need 5V vs 3.3V)
- **Enable/disable backlight control** in software

## CAN Bus Issues

### No CAN Communication

**Symptoms:**

- No messages received from controller
- CAN error counters increasing
- "CAN disconnected" message on display

**Diagnosis Steps:**

1. **Physical Layer Check**
   - Verify CAN_H and CAN_L are not swapped
   - Check termination resistor (120Ω if end-of-line)
   - Measure voltage between CAN_H and CAN_L (should be ~2.5V differential)

2. **Configuration Check**
   - Verify baud rate matches controller (typically 500kbps)
   - Check CAN mode (normal vs listen only)
   - Verify filters are not blocking all messages

3. **Hardware Check**
   - Check SN65HVD230 power (3.3V)
   - Verify TX/RX connections to ESP32
   - Check for shorts on CAN lines

**Solutions:**

- **Add termination resistor** if missing
- **Verify baud rate** with known-good device
- **Check CAN transceiver** with loopback test
- **Inspect CAN connector** for bent pins

### CAN Errors (Bus Off, Error Passive)

**Symptoms:**

- CAN error counters high
- Intermittent communication
- System logs show CAN errors

**Possible Causes:**

- **Electrical noise** from motor or converter
- **Ground potential differences**
- **Faulty CAN transceiver**
- **Baud rate mismatch**

**Solutions:**

- **Add common mode choke** to CAN lines
- **Improve grounding** (single point ground)
- **Add TVS diodes** for surge protection
- **Reduce CAN cable length** if possible

### CAN Message Corruption

**Symptoms:**

- Valid messages but incorrect data
- Missing messages
- CRC errors

**Solutions:**

- **Check CAN cable routing** (away from power lines)
- **Verify CAN ID filtering** settings
- **Increase receive buffer size**
- **Check for CAN bus overload** (too many devices)

## BLE (Bluetooth) Issues

### Device Not Discoverable

**Symptoms:**

- Mobile app cannot find device
- No advertising in BLE scanner apps
- BLE status shows "not advertising"

**Diagnosis:**

1. **Check BLE Status**
   - Serial monitor should show "BLE advertising started"
   - Verify BLE is enabled in configuration

2. **Check Advertising Parameters**
   - Advertising interval may be too long
   - Device name may be changed

3. **Check Hardware**
   - ESP32 antenna connection
   - RF interference from other components

**Solutions:**

- **Reset BLE stack** (toggle BLE on/off)
- **Reduce advertising interval** (100ms recommended)
- **Check for conflicting BLE devices**
- **Verify ESP32 BLE antenna** is not obstructed

### Connection Drops Frequently

**Symptoms:**

- Connection established but drops after few seconds
- Intermittent data transfer
- "Disconnected" messages

**Possible Causes:**

- **Weak signal** (distance or obstruction)
- **Interference** from WiFi or other RF sources
- **Power saving modes** interrupting connection
- **MTU size mismatch**

**Solutions:**

- **Reduce distance** between devices
- **Change WiFi channel** if using WiFi simultaneously
- **Adjust connection parameters** (increase interval, latency)
- **Disable BLE power saving** during active connection

### Data Transfer Issues

**Symptoms:**

- Notifications not arriving
- Slow data transfer
- Missing packets

**Solutions:**

- **Increase MTU size** (negotiate larger MTU)
- **Adjust notification interval**
- **Check BLE buffer sizes**
- **Verify characteristic properties** (notify vs indicate)

## Keypad/Button Issues

### Buttons Not Responding

**Symptoms:**

- Pressing buttons has no effect
- Some buttons work, others don't
- Intermittent response

**Diagnosis:**

1. **Hardware Check**
   - Verify button connections (continuity when pressed)
   - Check pull-up resistors (10kΩ recommended)
   - Verify GPIO configuration (INPUT_PULLUP)

2. **Software Check**
   - Check GPIO pin assignments in code
   - Verify interrupt handlers are registered
   - Check debouncing logic

**Solutions:**

- **Add debouncing capacitors** (0.1µF across button)
- **Increase pull-up resistor value** if too weak
- **Check for conflicting GPIO usage**
- **Verify button ground connection**

### Ghost Presses/False Triggers

**Symptoms:**

- Buttons trigger without being pressed
- Multiple buttons trigger simultaneously
- Random button events

**Possible Causes:**

- **Noise on GPIO lines**
- **Insufficient debouncing**
- **Floating inputs** (missing pull-up/down)
- **Short between button lines**

**Solutions:**

- **Add hardware debouncing** (RC filter)
- **Enable internal pull-up/pull-down**
- **Increase software debounce delay**
- **Check for solder bridges** between traces

### Long Press Not Detected

**Symptoms:**

- Short press works, long press doesn't
- Inconsistent long press detection

**Solutions:**

- **Adjust long-press threshold** (typically 500-1000ms)
- **Check timer implementation** for overflow
- **Verify button state machine** logic
- **Test with different button types** (some have longer bounce)

## Sensor Issues

### Temperature Sensor Reading Incorrect

**Symptoms:**

- Temperature readings unrealistic (e.g., -127°C, 85°C)
- Readings not changing
- Sensor not detected

**Diagnosis:**

- **Check wiring** (VCC, GND, DATA)
- **Verify pull-up resistor** (4.7kΩ for DS18B20)
- **Check one-wire bus** for other devices
- **Verify sensor address** (if using multiple sensors)

**Solutions:**

- **Add external pull-up resistor** if internal is weak
- **Reduce one-wire bus length** (< 20m recommended)
- **Check for parasitic power issues**
- **Replace sensor** if damaged

### Light Sensor Not Working

**Symptoms:**

- No change in readings with light variation
- Constant maximum or minimum value
- I²C communication errors

**Solutions:**

- **Check I²C address** (BH1750 typically 0x23 or 0x5C)
- **Verify I²C pull-up resistors** (4.7kΩ on SDA/SCL)
- **Check sensor orientation** (not covered)
- **Test with known light levels**

### Accelerometer Issues

**Symptoms:**

- No motion detection
- Incorrect orientation detection
- High noise in readings

**Solutions:**

- **Calibrate accelerometer** (flat surface, multiple orientations)
- **Adjust sensitivity settings**
- **Add low-pass filtering** in software
- **Check I²C communication** speed (400kHz max for MPU6050)

## Firmware Issues

### Compilation Errors

**Common Errors and Solutions:**

1. **"No such file or directory"**
   - Check library paths in `platformio.ini`
   - Verify library installation: `pio lib install <library>`

2. **"undefined reference to..."**
   - Missing implementation of declared function
   - Check if source file is included in build

3. **"section `.iram1.text' will not fit"**
   - Code too large for IRAM
   - Move functions to flash with `IRAM_ATTR` or optimize

4. **"region `flash' overflowed"**
   - Firmware too large for flash
   - Enable compression: `board_build.flash_mode = dio`
   - Remove unused features

### Runtime Errors

**Watchdog Resets:**

- **Cause**: Task not yielding for too long
- **Solution**: Add `vTaskDelay()` or `yield()` in loops
- **Check**: Stack sizes, infinite loops

**Heap Corruption:**

- **Symptoms**: Random crashes, memory allocation failures
- **Solution**: Use `heap_caps_check_integrity()` to diagnose
- **Prevention**: Avoid heap fragmentation, use pools

**Stack Overflow:**

- **Symptoms**: Crash at random points, corrupted variables
- **Solution**: Increase stack size in `xTaskCreate()`
- **Diagnosis**: Use `uxTaskGetStackHighWaterMark()`

### OTA Update Issues

**Update Fails:**

- **Check**: Sufficient flash space (two firmware slots required)
- **Verify**: BLE connection stability during transfer
- **Ensure**: Firmware signature validation passes

**Device Boots to Old Firmware:**

- **Cause**: Bootloader not switching partitions
- **Solution**: Manually set boot partition via serial
- **Check**: OTA rollback on error may be enabled

## Hardware-Specific Issues

### ESP32-C3 Specific

**USB Serial Issues:**

- **Problem**: No serial output after flash
- **Solution**: Hold BOOT button while pressing RESET, then release BOOT
- **Check**: USB driver installation

**WiFi/BLE Coexistence:**

- **Problem**: WiFi drops when BLE active
- **Solution**: Adjust coexistence parameters in `sdkconfig`
- **Alternative**: Use separate cores for WiFi and BLE

### MCP2515 CAN Issues

**SPI Communication:**

- **Problem**: MCP2515 not responding
- **Solution**: Check CS (chip select) pin, SPI mode (0,0)
- **Verify**: MCP2515 crystal frequency (8MHz or 16MHz)

**Configuration Errors:**

- **Problem**: CAN messages not sent/received
- **Solution**: Verify MCP2515 initialization sequence
- **Check**: Interrupt pin configuration

## Environmental Issues

### Temperature Extremes

**Cold Weather Issues:**

- Display response slow
- Battery voltage reading inaccurate
- Buttons stiff or unresponsive

**Solutions:**

- **Use low-temperature display** (specified for -20°C)
- **Add heater circuit** for critical components
- **Allow warm-up time** before operation

**Hot Weather Issues:**

- Overheating components
- Display fading
- Thermal shutdown

**Solutions:**

- **Add heat sinking** to buck converter and ESP32
- **Improve ventilation** in enclosure
- **Reduce power consumption** in high temps

### Water Ingress

**Symptoms:**

- Corrosion on connectors
- Intermittent operation in wet conditions
- Display fogging

**Solutions:**

- **Improve sealing** with silicone or epoxy
- **Use conformal coating** on PCB
- **Add drainage holes** (positioned downward)

## Diagnostic Tools

### Essential Tools

1. **Multimeter**
   - Voltage measurements
   - Continuity testing
   - Current draw measurement

2. **Logic Analyzer**
   - SPI/I²C/CAN signal analysis
   - Timing measurements
   - Protocol debugging

3. **Serial Monitor**
   - System log output
   - Error messages
   - Debug prints

4. **BLE Scanner App**
   - nRF Connect (recommended)
   - LightBlue Explorer
   - BLE Scanner

### Advanced Tools

1. **Oscilloscope**
   - Signal integrity analysis
   - Noise measurement
   - Power supply ripple

2. **CAN Bus Analyzer**
   - CANable (recommended)
   - PCAN-USB
   - USB-CAN adapter

3. **Thermal Camera**
   - Hot spot identification
   - Thermal management verification

## Getting Further Help

If you've tried the solutions above and still have issues:

1. **Check the FAQ** for additional solutions
2. **Search GitHub Issues** for similar problems
3. **Ask on the Community Forum** with:
   - Detailed description of the problem
   - Steps you've already tried
   - Photos of your setup
   - Serial log output (if available)
4. **Join the Discord** for real-time help

## Prevention Tips

1. **Test Incrementally** - Build and test each subsystem separately
2. **Document Changes** - Keep notes of modifications and their effects
3. **Use Version Control** - Commit frequently to track changes
4. **Backup Configurations** - Save working configurations before changes
5. **Regular Maintenance** - Periodically check connections and seals

---

_Remember: Troubleshooting is a systematic process. Start with the simplest explanations, work through each possibility methodically, and don't hesitate to ask for help when stuck. The OpenRideDash community is here to support you!_
