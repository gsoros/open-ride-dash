# Hardware Assembly Guide

## Introduction

This guide provides detailed, step-by-step instructions for assembling your OpenRideDash hardware. Follow these instructions carefully to ensure a reliable, weatherproof final product.

## Safety First

### Electrical Safety

- **High Voltage**: Up to 80V DC from e-bike batteries can be lethal. Always disconnect battery before working on connections.
- **Capacitors**: Buck converters may retain charge. Discharge by shorting terminals with resistor before handling.
- **ESD Protection**: Use anti-static wrist strap when handling ESP32 and other sensitive components.

### Personal Safety

- **Eye Protection**: Wear safety glasses when soldering or cutting.
- **Ventilation**: Work in well-ventilated area when using solder, epoxy, or solvents.
- **Fire Safety**: Keep fire extinguisher nearby when working with lithium batteries.

## Tools Required

### Essential Tools

- Soldering iron (temperature controlled, 60W recommended)
- Solder (lead-free or leaded, 0.8mm diameter)
- Wire cutters/strippers
- Multimeter
- Screwdrivers (Phillips and flathead)
- Needle-nose pliers
- Helping hands or PCB holder

### Optional but Helpful

- Heat gun (for heat shrink tubing)
- Third hand with magnifying glass
- Desoldering pump or wick
- Digital calipers
- Bench power supply

### Consumables

- Heat shrink tubing (various sizes)
- Electrical tape
- Cable ties
- Isopropyl alcohol (for cleaning)
- Flux (for better solder joints)

## Component Preparation

### ESP32-C3 Board Preparation

1. **Inspect Board**
   - Check for visible damage or manufacturing defects
   - Verify all pins are straight and undamaged

2. **Optional: Solder Headers**
   - For development: solder male headers to all pins
   - For production: solder wires directly to pads
   - Use flux for better solder flow

3. **Test Basic Function**
   - Connect via USB, verify power LED illuminates
   - Upload blink sketch to confirm programming works

### Buck Converter Preparation

1. **Adjust Output Voltage**
   - Connect input to 12V power source
   - Measure output with multimeter
   - Adjust potentiometer until output reads 5.00V ±0.05V
   - Mark final position with paint pen

2. **Add Input Protection** (Optional but recommended)
   - Solder fuse holder in series with positive input
   - Add TVS diode across input (SMBJ80A for 80V systems)
   - Add 100µF electrolytic capacitor across input

3. **Add Output Filtering**
   - Solder 10µF tantalum or electrolytic capacitor across output
   - Add 0.1µF ceramic capacitor close to ESP32 power pins

### CAN Transceiver Preparation

1. **Module Inspection**
   - Verify SN65HVD230 chip orientation
   - Check for solder bridges on module

2. **Add Termination Resistor**
   - If module doesn't have termination, add 120Ω resistor between CAN_H and CAN_L
   - Solder resistor directly to module pads

3. **Add Protection**
   - Solder TVS diodes (SMBJ5.0A) from CAN_H to GND and CAN_L to GND
   - Add common mode choke if operating in noisy environment

## Step-by-Step Assembly

### Step 1: Power System Assembly

#### 1.1 Prepare Input Connector

```
Materials:
- 2-pin waterproof connector (e.g., XT60, Anderson Powerpole)
- 14 AWG silicone wire (red and black)
- Heat shrink tubing
```

Procedure:

1. Cut red and black wires to 15cm length
2. Strip 5mm insulation from each end
3. Crimp or solder connector terminals to one end
4. Apply heat shrink over connections
5. Solder other ends to buck converter input terminals
   - Red to positive (+)
   - Black to negative (-)

#### 1.2 Prepare Output Wiring

```
Materials:
- 22 AWG wire (red and black)
- JST-XH 2-pin connector
```

Procedure:

1. Cut red (5V) and black (GND) wires to 10cm length
2. Strip 3mm from each end
3. Crimp or solder to JST connector
4. Solder other ends to buck converter output
5. Add heat shrink to all connections

### Step 2: ESP32-C3 Wiring

#### 2.1 Power Connections

1. Connect JST connector from buck converter to ESP32:
   - Red wire to ESP32 VIN pin
   - Black wire to ESP32 GND pin

2. Add decoupling capacitors:
   - 10µF tantalum between VIN and GND
   - 0.1µF ceramic between 3.3V and GND (if using 3.3V peripherals)

#### 2.2 CAN Connections

```
ESP32-C3 → SN65HVD230
GPIO4   → RX
GPIO5   → TX
3.3V    → VCC (if module uses 3.3V logic)
GND     → GND
```

Procedure:

1. Cut four 10cm wires (22 AWG)
2. Solder to SN65HVD230 module pins
3. Connect to ESP32 according to pinout
4. Add 120Ω termination resistor if not already on module

#### 2.3 Display Connections

```
ST7789 → ESP32-C3
VCC      → 3.3V or 5V (check display specs)
GND      → GND
SCK      → GPIO18
MOSI     → GPIO23
MISO     → GPIO19 (optional)
CS       → GPIO5
DC       → GPIO17
RST      → GPIO16
BL       → GPIO21 (PWM for backlight)
```

Procedure:

1. Use ribbon cable or individual wires
2. Keep wires short (< 15cm) to reduce noise
3. Add 0.1µF capacitor between BL and GND for backlight smoothing

### Step 3: Keypad Assembly

#### 3.1 Button Wiring

```
Button Layout (typical 3-button):
UP    → GPIO32
DOWN  → GPIO33
OK    → GPIO27
```

```
Button Layout (6-button):
UP    → GPIO32
DOWN  → GPIO33
LEFT  → GPIO25
RIGHT → GPIO26
OK    → GPIO27
BACK  → GPIO14
```

Procedure:

1. Cut six 15cm wires (22 AWG, different colors if possible)
2. Solder one end to each button terminal
3. Connect other ends to ESP32 GPIO pins
4. Add 10kΩ pull-up resistors between each GPIO and 3.3V
5. Add 0.1µF capacitors between each GPIO and GND for debouncing

#### 3.2 Waterproofing

1. Apply silicone sealant around button edges
2. Use waterproof membrane if available
3. Test each button for proper operation before sealing

### Step 4: Optional Sensor Integration

#### 4.1 Temperature Sensor (DS18B20)

1. Connect VDD to 3.3V
2. Connect GND to ground
3. Connect DATA to GPIO13 with 4.7kΩ pull-up resistor to 3.3V
4. Waterproof sensor with heat shrink or epoxy

#### 4.2 Light Sensor (BH1750)

1. Connect VCC to 3.3V
2. Connect GND to ground
3. Connect SDA to GPIO21
4. Connect SCL to GPIO22
5. Add 4.7kΩ pull-up resistors on SDA and SCL

#### 4.3 Accelerometer (MPU6050)

1. Connect VCC to 3.3V
2. Connect GND to ground
3. Connect SDA to GPIO21
4. Connect SCL to GPIO22
5. Connect INT to GPIO34 (optional)

### Step 5: Enclosure Preparation

#### 5.1 3D Printing

1. **Print Settings**:
   - Material: PETG (recommended for weather resistance)
   - Layer height: 0.2mm
   - Infill: 40%
   - Supports: Yes for overhangs
   - Brim: Yes for better bed adhesion

2. **Print Parts**:
   - Main enclosure body
   - Front cover
   - Mounting brackets (if needed)
   - Connector covers

3. **Post-Processing**:
   - Remove supports carefully
   - Sand rough edges
   - Clean with isopropyl alcohol

#### 5.2 Glass Cover Installation

1. Cut tempered glass to match display opening (allow 0.5mm clearance)
2. Apply optical adhesive (LOCA) to display surface
3. Carefully place glass, avoiding bubbles
4. Cure with UV light (if using UV-cure adhesive)
5. Apply silicone sealant around edges

### Step 6: Final Assembly

#### 6.1 Component Placement

1. **Dry Fit**: Place all components in enclosure without adhesive
2. **Check Clearances**: Ensure no components touch each other or enclosure walls
3. **Connector Access**: Verify all connectors are accessible
4. **Display Alignment**: Center display behind glass window

#### 6.2 Securing Components

1. **ESP32**: Use nylon standoffs and screws, or double-sided foam tape
2. **Buck Converter**: Secure with screws or adhesive thermal pad
3. **Display**: Use double-sided tape around edges
4. **Sensors**: Secure with small amount of silicone

#### 6.3 Cable Management

1. **Route Wires Neatly**: Use cable ties or adhesive clips
2. **Avoid Sharp Edges**: Protect wires from enclosure edges with grommets
3. **Strain Relief**: Secure cables near connectors to prevent pull-out
4. **Label Wires**: Use labels or color coding for future maintenance

### Step 7: Epoxy Potting

#### 7.1 Preparation

1. **Clean Components**: Remove dust and oils with isopropyl alcohol
2. **Apply Release Agent**: Spray mold release on enclosure interior
3. **Mask Areas**: Cover connectors, buttons, and display with tape
4. **Position Components**: Secure with temporary adhesive putty

#### 7.2 Mixing Epoxy

1. **Choose Epoxy**: Use two-part polyurethane or silicone potting compound
2. **Measure Accurately**: Follow manufacturer's ratio precisely
3. **Mix Thoroughly**: Stir for recommended time (typically 2-3 minutes)
4. **Degas**: Remove bubbles by vacuum or letting sit for few minutes

#### 7.3 Pouring

1. **Pour Slowly**: Pour epoxy in thin stream to minimize bubbles
2. **Fill Completely**: Ensure all components are covered
3. **Remove Bubbles**: Use toothpick or vibration to pop surface bubbles
4. **Top Up**: Add more epoxy after initial settling

#### 7.4 Curing

1. **Initial Cure**: Let sit at room temperature for 24 hours
2. **Post-Cure**: Optional heat cure at 60°C for 2 hours (if recommended)
3. **Demold**: Carefully remove from enclosure mold
4. **Trim Flash**: Remove any epoxy that seeped into unwanted areas

### Step 8: Final Connections

#### 8.1 External Connectors

1. **CAN Connector**: Install 5-pin waterproof automotive connector
2. **Power Connector**: Install input connector (XT60, etc.)
3. **Keypad Connector**: Install waterproof JST connector
4. **USB Access**: Consider waterproof USB port for programming

#### 8.2 Sealing

1. **Apply Sealant**: Use silicone around all connector openings
2. **Test Waterproofing**: Spray with water before final installation
3. **Add Gaskets**: Use rubber gaskets for screw holes

## Testing During Assembly

### Stage 1: After Power System

- Test buck converter output (5.00V ±0.05V)
- Verify no shorts between power rails
- Check input polarity protection

### Stage 2: After ESP32 Wiring

- Connect USB, verify serial communication
- Upload test sketch, verify LED blinking
- Check all GPIOs with multimeter

### Stage 3: After Display Connection

- Upload display test sketch
- Verify all pixels work
- Test backlight brightness control

### Stage 4: After CAN Connection

- Connect to CAN bus simulator or known-good device
- Verify message transmission and reception
- Check error counters remain zero

### Stage 5: After Keypad Connection

- Test each button with GPIO test sketch
- Verify debouncing works correctly
- Test long-press detection

### Stage 6: Final Assembly

- Power on with all components connected
- Run comprehensive test sketch
- Monitor temperature during extended operation

## Troubleshooting Assembly Issues

### Common Problems

#### Poor Solder Joints

- **Symptoms**: Intermittent connections, high resistance
- **Solution**: Reheat joint, add flux, ensure proper wetting
- **Prevention**: Clean surfaces, use appropriate temperature

#### Short Circuits

- **Symptoms**: Excessive current draw, components overheating
- **Solution**: Use multimeter to locate short, inspect for solder bridges
- **Prevention**: Check continuity before powering on

#### Display Issues

- **Symptoms**: Blank screen, garbled display, flickering
- **Solution**: Check SPI connections, verify power, adjust contrast
- **Prevention**: Use shielded cable for longer runs

#### CAN Communication Problems

- **Symptoms**: No messages, high error counters
- **Solution**: Verify termination, check CAN_H/CAN_L not swapped
- **Prevention**: Use twisted pair cable, proper grounding

#### Button Bounce

- **Symptoms**: Multiple triggers from single press
- **Solution**: Add hardware debouncing (RC filter)
- **Prevention**: Use capacitors and proper pull-up resistors

## Enclosure Finishing

### Surface Finish

1. **Sanding**: Start with 220 grit, progress to 400 grit for smooth finish
2. **Priming**: Apply plastic primer for better paint adhesion
3. **Painting**: Use UV-resistant paint for outdoor use
4. **Clear Coat**: Apply clear coat for added protection

### Mounting Options

1. **Handlebar Mount**: Use standard 22.2mm or 31.8mm clamp
2. **Stem Mount**: Custom bracket for stem integration
3. **Frame Mount**: Adhesive or bolt-on frame mount
4. **Quick Release**: For easy removal when parking

### Final Inspection

1. **Visual**: Check for defects, proper sealing
2. **Functional**: Test all features
3. **Environmental**: Verify waterproofing with spray test
4. **Documentation**: Take photos for your build log

## Maintenance and Repair

### Regular Maintenance

- **Monthly**: Check connector integrity, clean display
- **Seasonal**: Inspect for water ingress, test all functions
- **Annual**: Reapply sealant if needed, check battery connections

### Repair Procedures

1. **Component Replacement**: Carefully cut away epoxy around failed component
2. **Resoldering**: Use low-temperature solder for repairs
3. **Re-potting**: Mix small amount of epoxy for local repair
4. **Display Replacement**: May require complete rework if fully potted

## Build Variations

### Minimal Build

- ESP32-C3 + CAN only
- Serial output for debugging
- No display or keypad
- For data logging only

### Standard Build

- As described in this guide
- Full display and keypad
- Optional sensors
- Weatherproof enclosure

### Advanced Build

- Dual CAN interfaces
- Multiple displays
- Extensive sensor suite
- WiFi/Bluetooth gateway functionality

## Next Steps

After successful assembly:

1. **Proceed to [Firmware Installation](../development/setup.md)**
2. **Configure system using [Mobile App](../phases/phase3-mobile.md)**
3. **Test with your e-bike in controlled environment**
4. **Share your build on [Community Forum](https://forum.open-ride-dash.org)**

## Additional Resources

- **[Hardware Repository](https://github.com/open-ride-dash/hardware)** - CAD files, schematics
- **[Video Tutorials](https://youtube.com/open-ride-dash)** - Assembly demonstrations
- **[Community Builds](https://forum.open-ride-dash.org/c/builds)** - Inspiration from other builders
- **[Troubleshooting Guide](../support/troubleshooting.md)** - Problem-solving help

---

_Take your time with assembly—careful work now will result in a reliable, long-lasting OpenRideDash unit. Don't hesitate to ask for help in the community forums if you encounter difficulties._
