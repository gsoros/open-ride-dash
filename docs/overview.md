# OpenRideDash Overview

## Project Vision

OpenRideDash (ORD) aims to democratize e-bike display technology by providing a comprehensive, open-source platform that anyone can build, modify, and extend. Traditional e-bike displays are proprietary black boxes; ORD turns them into transparent, customizable systems.

## Core Philosophy

### Open Source, Open Hardware

- All firmware source code is open and documented
- Hardware designs are shared and modifiable
- No proprietary components or locked-down features

### Modular by Design

- Hardware abstraction layers allow component swapping
- Plugin architecture for features and protocols
- Clear separation between core system and optional features

### DIY-Friendly

- Designed for at-home assembly with common tools
- Comprehensive documentation and testing guides
- Progressive complexity - start simple, add features as needed

## Key Features

### 🚀 Core Capabilities

- **CAN Bus Communication** - Industry-standard interface for e-bike controllers
- **ESP32 Platform** - Powerful, WiFi/BLE capable microcontroller
- **Display** - Readable in direct sunlight
- **Physical Keypad** - Tactile controls for safety and reliability

### 📡 Connectivity

- **BLE 5.0** - Low-energy communication with mobile devices
- **Standard BLE Services** - Compatibility with fitness apps and devices
- **Custom BLE API** - Full control and configuration from mobile apps

### 📱 Mobile Integration

- **Cross-platform App** - Built with Flutter for iOS and Android
- **Real-time Telemetry** - Live data display and recording
- **Configuration Interface** - Graphical setup and tuning
- **Data Export** - GPX recording with power and cadence data

### ⚙️ Advanced Features

- **OTA Updates** - Wireless firmware updates via BLE
- **Power Management** - Deep sleep modes for battery conservation
- **Sensor Integration** - Temperature, light, motion sensors
- **Ecosystem Compatibility** - Works with existing e-bike tools

## Target Audience

### 1. E-Bike Enthusiasts

- DIY builders wanting custom displays
- Owners of proprietary systems seeking more control
- Tinkerers interested in bike telemetry and data logging

### 2. Developers & Makers

- Embedded developers learning CAN bus systems
- IoT developers exploring BLE applications
- Open source contributors looking for meaningful projects

### 3. Small Manufacturers

- Companies needing customizable display solutions
- Prototyping teams wanting flexible development platforms
- Service providers offering aftermarket upgrades

## System Requirements

### Hardware Requirements

- ESP32-C3 or ESP32 with CAN support
- CAN transceiver (SN65HVD230 or equivalent)
- ST7789 display
- Custom keypad or button array
- 80V to 5V buck converter
- Basic soldering tools and skills

### Software Requirements

- PlatformIO or Arduino IDE
- Python 3.x (for development tools)
- Git for version control
- Basic C++ knowledge for firmware modification

### Skill Requirements

- Basic electronics knowledge
- Ability to read schematics
- Comfort with command-line tools
- Willingness to learn and troubleshoot

## Project Scope

### In Scope

- Firmware for ESP32-based displays
- Hardware reference designs
- Mobile application (Flutter)
- Comprehensive documentation
- Testing and validation procedures
- Community support structure

### Out of Scope (Phase 1)

- Production-ready PCBs (reference designs only)
- Commercial certification (CE, FCC, etc.)
- Proprietary controller protocols (beyond documented reverse-engineering)
- Cloud services or subscription features

## Success Metrics

### Technical Success

- ✅ Reliable CAN communication with Bafang controllers
- ✅ Stable BLE connectivity with mobile app
- ✅ 24+ hour battery life with typical usage
- ✅ < 100ms response time for user inputs

### Community Success

- ✅ Active contributor community
- ✅ Multiple successful builds documented
- ✅ Regular feature updates and improvements
- ✅ Comprehensive issue resolution

### User Success

- ✅ Clear, achievable build instructions
- ✅ Reliable operation in real-world conditions
- ✅ Meaningful customization options
- ✅ Positive user feedback and testimonials

## Getting Started

If you're ready to begin, here's your path:

1. **Start with the [System Architecture](architecture/system-architecture.md)** to understand the big picture
2. **Review [Phase 1: Core System](phases/phase1-core.md)** for the minimum viable product
3. **Check the [Hardware Guide](hardware/guide.md)** for component selection and assembly
4. **Set up your [Development Environment](development/setup.md)**

## Next Steps

- **For new users**: Continue to [System Architecture](architecture/system-architecture.md)
- **For developers**: Jump to [Phase 1 Implementation](phases/phase1-core.md)
- **For hardware builders**: Go to [Hardware Guide](hardware/guide.md)

---

_"Building the open future of e-bike technology, one component at a time."_
