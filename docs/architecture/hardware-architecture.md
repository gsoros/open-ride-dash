# Hardware Architecture

## Overview

OpenRideDash hardware is designed for reliability, modularity, and DIY assembly. The architecture prioritizes clear separation between components, making it easy to source parts, assemble, and troubleshoot.

## System Block Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     E-Bike Power System                     │
│                   (36V, 48V, or 52V Battery)                │
└─────────────────────────────┬───────────────────────────────┘
                              │
                    ┌─────────▼──────────┐
                    │   Buck Converter   │
                    │      (80V→5V)      │
                    └─────────┬──────────┘
                              │
          ┌───────────────────┼───────────────────┐
          │                   │                   │
┌─────────▼──────────┐ ┌─────▼───────┐ ┌──────────▼──────────┐
│     ESP32-C3       │ │   CAN       │ │      Display        │
│   Microcontroller  │ │ Transceiver │ │    (ST7789)         │
│                    │ │ (SN65HVD230)│ │                     │
└─────────┬──────────┘ └──────┬──────┘ └──────────┬──────────┘
          │                   │                   │
          └───────────────────┼───────────────────┘
                              │
                    ┌─────────▼──────────┐
                    │     Keypad/        │
                    │     Buttons        │
                    └────────────────────┘
```

## Core Components

### 1. Microcontroller: ESP32-C3

- **Why ESP32-C3**: Integrated CAN controller, BLE 5.0, RISC-V architecture
- **Alternative**: ESP32 with external CAN controller (MCP2515)
- **Key Features**:
  - 160MHz RISC-V single-core processor
  - 400KB SRAM, 4MB flash (typical)
  - Integrated CAN 2.0 controller
  - BLE 5.0 with mesh support
  - Low-power modes for battery operation

### 2. Power System

- **Input**: 36V-80V DC from e-bike battery
- **Regulation**: Buck converter to 5V
- **Requirements**:
  - Input range: 20V-80V DC
  - Output: 5V @ 1A minimum
  - Efficiency: >85% at typical loads
  - Protection: Over-voltage, under-voltage, reverse polarity

### 3. CAN Interface

- **Transceiver**: SN65HVD230
- **Connector**: 5-pin automotive (CAN_H, CAN_L, GND, optional +12V)
- **Configuration**:
  - Baud rate: 500kbps (Bafang standard)
  - Termination: 120Ω resistor (if end-of-line)
  - Protection: TVS diodes on CAN lines

### 4. Display Module

- **Type**: ST7789 LCD
- **Size**: 2.4" typical (variable based on availability)
- **Interface**: SPI or parallel (depending on module)
- **Features**:
  - Sunlight readable without backlight
  - Low power consumption
  - Wide viewing angles
  - Compatible with common controllers

### 5. User Input

- **Options**:
  - Custom membrane keypad (6 buttons typical)
  - Tactile switches with waterproof boots
  - Rotary encoder with push button
- **Requirements**:
  - Weather resistant
  - Tactile feedback
  - Support for long/short press detection

## Optional Components

### 1. Environmental Sensors

- **Temperature**: DS18B20 (1-wire interface)
- **Light Sensor**: BH1750 or equivalent (I²C)
- **Accelerometer**: MPU6050 or BMA423 (I²C/SPI)

### 2. Connectivity Expansion

- **WiFi**: Already integrated in ESP32
- **GPS**: Optional for mobile app integration
- **SD Card**: For standalone data logging

### 3. Protection Circuits

- **TVS Diodes**: CAN lines, power input
- **Fuses**: Power input protection
- **EMI Filters**: For noisy e-bike environments

## Electrical Specifications

### Power Requirements

```
Component          Voltage   Current (max)   Notes
────────────────────────────────────────────────────
ESP32-C3           3.3V      250mA          Peak during RF
CAN Transceiver    5V        70mA           Active transmission
Display            3.3V/5V   100mA          Backlight dependent
Sensors            3.3V      10-50mA        Depending on sensors
────────────────────────────────────────────────────
Total (typical)              ~500mA         Without backlight
Total (maximum)              ~800mA         All systems active
```

### Signal Levels

- **CAN**: Differential 2.5V nominal (CAN_H: 3.5V, CAN_L: 1.5V)
- **I²C**: 3.3V, 100-400kHz
- **SPI**: 3.3V, up to 10MHz
- **GPIO**: 3.3V logic, 5V tolerant with caution

## Physical Architecture

### Board Layout Considerations

1. **Power Isolation**: Keep switching converter away from analog signals
2. **CAN Routing**: Differential pair routing with length matching
3. **Ground Planes**: Solid ground plane for noise reduction
4. **Thermal Management**: Heat sinking for buck converter if needed

### Connector Strategy

```
Connector   Type          Purpose                  Pins
──────────────────────────────────────────────────────────
PWR         Screw terminal  Battery input          2
CAN         Automotive       CAN bus connection     5
KEYPAD      JST/XH           User input            6-8
DISPLAY     FPC/connector    Display interface     10-20
SENSORS     JST/header       Optional sensors      4-6
DEBUG       Serial header    Programming/debug     6
```

## Enclosure Design

### Requirements

- **Weather Resistance**: IP65 or better for outdoor use
- **Thermal Management**: Heat dissipation for electronics
- **Mounting**: Secure attachment to handlebars or frame
- **Display Protection**: Scratch-resistant cover with optical bonding

### Construction Approach

1. **3D Printed Mold**: Create custom shape for epoxy potting
2. **Epoxy Potting**: Complete environmental sealing
3. **Glass Cover**: Optical adhesive bonding to display
4. **Threaded Inserts**: For secure mounting brackets

### Thermal Considerations

- **Heat Sources**: Buck converter (efficiency losses), ESP32 (RF operation)
- **Dissipation**: Potting material thermal conductivity, heat sinking
- **Monitoring**: Optional temperature sensor for thermal protection

## Modularity and Compatibility

### Hardware Abstraction

- **Interface Definitions**: Clear contracts for each component type
- **Factory Pattern**: Runtime selection of concrete implementations
- **Configuration Flags**: Compile-time hardware selection

### Supported Variants

1. **Basic Configuration**: ESP32-C3 + CAN + Display + Keypad
2. **Enhanced Configuration**: Add sensors and additional interfaces
3. **Custom Configurations**: User-defined component combinations

### Compatibility Matrix

```
Feature              ESP32-C3   ESP32 + MCP2515   Notes
────────────────────────────────────────────────────────────
Native CAN           ✓          ✗                 ESP32-C3 only
External CAN         ✗          ✓                 Needs MCP2515
BLE 5.0              ✓          ✓                 Both support
WiFi                 ✓          ✓                 Both support
GPIO Count           22         34                ESP32 has more
Power Consumption    Lower      Higher            MCP2515 adds power
```

## Testing and Validation

### Hardware Testing Points

1. **Power Input**: Verify voltage range and protection
2. **CAN Bus**: Loopback test and communication validation
3. **Display**: Test patterns and refresh rate
4. **Keypad**: Button response and debouncing
5. **Sensors**: Calibration and accuracy verification

### Environmental Testing

- **Temperature**: -20°C to +60°C operation
- **Humidity**: 0-95% non-condensing
- **Vibration**: Handlebar mounting simulation
- **Water Resistance**: Spray and immersion testing

## Bill of Materials (Reference)

### Required Components

```
Qty  Component                     Specification                 Notes
─────────────────────────────────────────────────────────────────────────
1     ESP32-C3 Dev Board           160MHz, 4MB flash, CAN        AliExpress
1     Buck Converter               80V→5V, 1A minimum            LM2596 based
1     CAN Transceiver              SN65HVD230 or equivalent      3.3V operation
1     Display Module               ST7789, 2.4"                  SPI interface
1     Keypad                       3 to 6-button membrane             Custom design
1     CAN Connector                5-pin automotive              Green 5-pin Higo
1     Enclosure                    3D printed + epoxy            Weather resistant
```

### Optional Components

```
Qty  Component                     Interface      Purpose
──────────────────────────────────────────────────────────────────
1     Temperature Sensor         1-wire/DS18B20  Ambient temp
1     Light Sensor               I²C/BH1750      Auto backlight
1     Accelerometer              I²C/MPU6050     Motion detection
1     GPS Module                 UART            Location tracking
```

## Safety Considerations

### Electrical Safety

- **Isolation**: Proper isolation between high-voltage and low-voltage sections
- **Protection**: Fuses, TVS diodes, and reverse polarity protection
- **Certification**: DIY project - not certified for commercial use

### Mechanical Safety

- **Mounting**: Secure attachment to prevent detachment while riding
- **Sealing**: Complete weatherproofing for outdoor operation
- **Materials**: UV-resistant, temperature-stable materials

### Operational Safety

- **Fail-Safe Modes**: Graceful degradation on system failure
- **Error Indication**: Clear visual indicators for fault conditions
- **Recovery**: User-accessible reset and recovery procedures

## Next Steps

- **Detailed Assembly**: See [Hardware Guide](../hardware/guide.md)
- **Testing Procedures**: Refer to [Hardware Testing](../hardware/testing.md)
- **Firmware Integration**: Check [Firmware Architecture](firmware-architecture.md)
- **Getting Started**: Begin with [Phase 1 Implementation](../phases/phase1-core.md)

---

_This hardware architecture provides a solid foundation for a reliable, customizable e-bike display system that balances performance, cost, and DIY accessibility._
