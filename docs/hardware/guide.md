# Hardware Guide

## Introduction

This guide covers the hardware components, assembly, and testing procedures for building your own OpenRideDash display unit. Whether you're building a prototype or a production-ready unit, this guide provides step-by-step instructions.

## Component Selection

### Required Components

| Component       | Specification          | Recommended Part    | Notes                                      |
| --------------- | ---------------------- | ------------------- | ------------------------------------------ |
| Microcontroller | ESP32-C3 with CAN      | ESP32-C3-DevKitM-1  | Must have integrated CAN controller        |
| Buck Converter  | 80V to 5V, 1A+         | LM2596-based module | Adjustable output, over-current protection |
| CAN Transceiver | TJA1050 or equivalent  | TJA1050 module      | 5V operation, high-speed CAN               |
| Display         | SSD1283A transflective | 2.4" 240x320 SPI    | Sunlight readable, low power               |
| Keypad          | 6-button membrane      | Custom or DPC245    | Waterproof, tactile feedback               |
| CAN Connector   | 5-pin automotive       | Deutsch DT06-5S     | Waterproof, locking                        |
| Enclosure       | 3D printed + epoxy     | Custom design       | Weatherproof, shock resistant              |

### Optional Components

| Component          | Purpose                        | Recommended Part |
| ------------------ | ------------------------------ | ---------------- |
| Temperature Sensor | Ambient temperature monitoring | DS18B20          |
| Light Sensor       | Automatic backlight adjustment | BH1750           |
| Accelerometer      | Motion detection, tilt sensing | MPU6050          |
| GPS Module         | Location tracking              | NEO-6M           |
| SD Card Module     | Data logging                   | MicroSD adapter  |

### Tools Required

- Soldering iron and solder
- Wire cutters/strippers
- Multimeter
- Screwdrivers
- Heat gun (for heat shrink)
- 3D printer (for enclosure)
- Epoxy resin (for potting)

## Assembly Instructions

### Step 1: Power System Assembly

1. **Prepare Buck Converter**
   - Adjust output voltage to 5.0V using multimeter
   - Solder input wires (red/black) to input terminals
   - Add input protection (fuse, TVS diode optional)
   - Test with 12V power source before connecting to high voltage

2. **Connect to ESP32**
   - Solder 5V output to ESP32 VIN pin
   - Solder GND to ESP32 GND
   - Add 10µF and 0.1µF capacitors near ESP32 power pins

### Step 2: CAN Interface Assembly

1. **Wire TJA1050 Module**
   - Connect VCC to 5V
   - Connect GND to common ground
   - Connect CAN_H and CAN_L to connector
   - Connect TX to ESP32 GPIO 5 (CAN TX)
   - Connect RX to ESP32 GPIO 4 (CAN RX)

2. **Add Termination**
   - If this is the end of the CAN bus, add 120Ω resistor between CAN_H and CAN_L
   - Solder resistor directly to connector or module

3. **Protection Circuits**
   - Add TVS diodes (SMBJ5.0A) between CAN_H/GND and CAN_L/GND
   - Optional: Add common mode choke for noise suppression

### Step 3: Display Assembly

1. **Connect Display Pins**
   - SSD1283A typically uses SPI interface:
     - SCK → GPIO 18
     - MOSI → GPIO 23
     - MISO → GPIO 19 (optional)
     - CS → GPIO 5
     - DC → GPIO 17
     - RST → GPIO 16
     - Backlight → GPIO 21 (PWM capable)

2. **Secure Display**
   - Use double-sided tape or mounting screws
   - Ensure display is aligned with enclosure window
   - Protect ribbon cable from sharp edges

### Step 4: Keypad Assembly

1. **Button Connections**
   - Connect each button to GPIO with internal pull-up
   - Recommended GPIOs: 32, 33, 25, 26, 27, 14
   - Add 0.1µF capacitors between button and ground for debouncing

2. **Waterproofing**
   - Use silicone sealant around button edges
   - Consider waterproof membrane keypad
   - Test each button for reliable operation

### Step 5: Optional Sensors

1. **Temperature Sensor (DS18B20)**
   - Connect VCC to 3.3V
   - Connect GND to ground
   - Connect DATA to GPIO 13 with 4.7kΩ pull-up resistor

2. **Light Sensor (BH1750)**
   - Connect VCC to 3.3V
   - Connect GND to ground
   - Connect SDA to GPIO 21
   - Connect SCL to GPIO 22

3. **Accelerometer (MPU6050)**
   - Connect VCC to 3.3V
   - Connect GND to ground
   - Connect SDA to GPIO 21
   - Connect SCL to GPIO 22
   - Connect INT to GPIO 34 (optional)

## Wiring Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                         Power Input                         │
│                    (36V-80V from battery)                   │
└─────────────────────────────┬───────────────────────────────┘
                              │
                    ┌─────────▼──────────┐
                    │   Buck Converter   │
                    │     80V → 5V       │
                    └─────────┬──────────┘
                              │ 5V
           ┌──────────────────┼──────────────────┐
           │                  │                  │
    ┌──────▼──────┐   ┌──────▼──────┐   ┌───────▼──────┐
    │   ESP32-C3  │   │  TJA1050    │   │   Display    │
    │             │   │   CAN       │   │   SSD1283A   │
    └──────┬──────┘   └──────┬──────┘   └───────┬──────┘
           │                  │                  │
    ┌──────▼──────┐   ┌──────▼──────┐   ┌───────▼──────┐
    │   Keypad    │   │   Sensors   │   │   Backlight  │
    │  (6 buttons)│   │ (optional)  │   │   Control    │
    └─────────────┘   └─────────────┘   └──────────────┘
```

## Enclosure Construction

### 3D Printing

1. **Print Settings**
   - Material: PETG or ABS (weather resistant)
   - Layer height: 0.2mm
   - Infill: 40%
   - Supports: Yes for overhangs
   - Orientation: Display side down for best surface finish

2. **Design Files**
   - Download STL files from [OpenRideDash Hardware Repository](https://github.com/open-ride-dash/hardware)
   - Includes main body, cover, and mounting brackets

### Epoxy Potting

1. **Preparation**
   - Clean enclosure with isopropyl alcohol
   - Apply mold release to interior surfaces
   - Position electronics securely with temporary supports

2. **Mixing and Pouring**
   - Use two-part epoxy with 1:1 ratio
   - Mix thoroughly for 2 minutes
   - Pour slowly to avoid air bubbles
   - Fill until components are completely covered

3. **Curing**
   - Allow 24 hours at room temperature
   - Post-cure at 60°C for 2 hours (optional)
   - Remove from mold after full cure

### Final Assembly

1. **Glass Cover Installation**
   - Cut tempered glass to match display opening
   - Apply optical adhesive (LOCA) to display surface
   - Carefully place glass, avoiding bubbles
   - Cure with UV light (if using UV adhesive)

2. **Connector Sealing**
   - Apply silicone sealant around connector openings
   - Use waterproof gaskets for screw terminals
   - Test with water spray before final installation

## Testing Procedures

### Initial Power-Up Test

1. **Visual Inspection**
   - Check for solder bridges, cold joints
   - Verify component orientation
   - Ensure no loose wires or connectors

2. **Continuity Test**
   - Check for shorts between power rails
   - Verify ground connections
   - Test button continuity

3. **Power Test**
   - Apply 12V to input, measure 5V output
   - Check ESP32 power LED illuminates
   - Measure current draw (should be < 100mA idle)

### Functional Testing

1. **CAN Bus Test**
   - Connect to known-good CAN device or use loopback
   - Send test message, verify reception
   - Check termination resistor value (120Ω if end-of-line)

2. **Display Test**
   - Upload test firmware with basic graphics
   - Verify all pixels work, no dead lines
   - Test backlight brightness control

3. **Keypad Test**
   - Press each button, verify GPIO readings
   - Test long-press detection
   - Check for ghosting or cross-talk

4. **Sensor Test** (if installed)
   - Read temperature, verify reasonable values
   - Test light sensor response to changing light
   - Verify accelerometer readings

### Environmental Testing

1. **Temperature Test**
   - Operate at -10°C to +50°C
   - Verify display remains readable
   - Check for condensation issues

2. **Vibration Test**
   - Simulate handlebar mounting vibrations
   - Run for 24 hours with continuous operation
   - Check for loose connections or component failure

3. **Water Resistance Test**
   - IP65 rating target
   - Spray test with water from all directions
   - Submersion test optional (not recommended for prolonged)

## Troubleshooting

### Common Issues

#### No Power

- Check input voltage polarity
- Verify buck converter output adjustment
- Check for blown fuse or protection diode

#### CAN Communication Issues

- Verify termination resistor (120Ω)
- Check CAN_H and CAN_L are not swapped
- Confirm baud rate matches controller (typically 500kbps)

#### Display Problems

- Check SPI connections, especially CS and DC
- Verify backlight voltage (may need 5V or 3.3V)
- Adjust contrast via software initialization

#### Button Issues

- Check pull-up resistors are enabled in software
- Verify debouncing capacitors are installed
- Test for short circuits between button lines

#### Sensor Failures

- Verify I²C address matches software
- Check pull-up resistors on SDA/SCL lines
- Confirm sensor is getting proper voltage

### Debugging Tools

1. **Multimeter**
   - Voltage measurements at key points
   - Continuity testing for broken connections
   - Current draw measurements

2. **Logic Analyzer**
   - SPI communication debugging
   - CAN bus signal analysis
   - Button timing measurements

3. **Serial Monitor**
   - ESP32 debug output
   - System status messages
   - Error reporting

## Maintenance

### Regular Checks

1. **Visual Inspection**
   - Check for cracks in enclosure
   - Verify connector integrity
   - Look for moisture ingress

2. **Electrical Checks**
   - Measure battery voltage accuracy
   - Verify CAN communication reliability
   - Test all buttons still functional

3. **Software Updates**
   - Check for firmware updates regularly
   - Backup configuration before updating
   - Test new features thoroughly

### Repair Procedures

1. **Component Replacement**
   - Desolder faulty component
   - Clean pad with solder wick
   - Solder new component, verify orientation

2. **Water Damage**
   - Immediately power off device
   - Disassemble and dry components
   - Clean with isopropyl alcohol
   - Test before reassembly

3. **Display Replacement**
   - Carefully remove glass cover
   - Desolder display connector
   - Install new display, recalibrate if needed

## Safety Considerations

### Electrical Safety

- **High Voltage**: Input up to 80V DC - handle with care
- **Isolation**: Ensure proper isolation between high and low voltage sections
- **Fusing**: Always include appropriate fuses for over-current protection

### Mechanical Safety

- **Mounting**: Secure enclosure to handlebars with appropriate brackets
- **Sharp Edges**: File smooth any sharp edges on enclosure
- **Weight Distribution**: Balance device to avoid handlebar imbalance

### Environmental Safety

- **Temperature Limits**: Do not operate outside -20°C to +60°C
- **Water Exposure**: While water resistant, avoid submersion
- **UV Exposure**: Use UV-resistant materials for outdoor use

## Next Steps

After completing hardware assembly and testing:

1. **Proceed to [Firmware Installation](../development/guide.md)**
2. **Configure system using [Mobile App](../phases/phase3-mobile.md)**
3. **Join the [Community Forum](https://forum.open-ride-dash.org) for support**

## Additional Resources

- **[Hardware Repository](https://github.com/open-ride-dash/hardware)** - CAD files, schematics
- **[Bill of Materials](https://github.com/open-ride-dash/hardware/blob/main/BOM.md)** - Complete parts list
- **[Assembly Video](https://youtube.com/open-ride-dash)** - Step-by-step video guide
- **[Troubleshooting Guide](../support/troubleshooting.md)** - Common problems and solutions

---

_Building your own OpenRideDash unit is a rewarding project that combines electronics, software, and mechanical skills. Take your time, follow safety procedures, and don't hesitate to ask for help in the community forums._
