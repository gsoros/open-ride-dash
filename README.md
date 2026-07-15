# OpenRideDash

OpenRideDash is open-source firmware for an ESP32-C3 based e-bike dashboard. It provides a compact ride display, CAN data integration, BLE and Wi-Fi connectivity, OTA updates, Shell diagnostics, and keypad-driven menus on a 240x240 ST7789 display. Currently the Bafang M560 mid-drive controller is supported.

## Features

- ESP32-C3 Super Mini support
- ST7789 240x240 color display driver
- CAN bus data integration for e-bike metrics
- BLE cycling services, API bridge for the mobile app
- Wi-Fi connectivity and OTA firmware updates
- Serial over WiFi access for diagnostics and command handling
- Custom keypad/menu navigation
- Task-based architecture for modular firmware components
- Rolling firmware bootloader support via custom bootloader replacement

## Hardware

- ESP32-C3 development board (`esp32-c3-devkitm-1`)
- ST7789 240x240 display
- CAN bus interface
- Keypad 

## Software Stack

- PlatformIO with Arduino framework
- ESP32 platform version `6.13.0`
- Arduino GFX library
- Patched MPU9250 dependency 
- `ACAN_ESP32` for CAN support
- `OneButton` for keypad/button handling

## Repository Structure

- platformio.ini — build and environment configuration
- src — main firmware source
  - main.cpp — system initialization and task startup
  - `drivers/` — display and hardware drivers
  - `model/` — shared state model
  - `tasks/` — modular runtime components (Wi-Fi, OTA, WifiSerial, CAN, display, keypad, alarm)
- include — shared headers, config, and font assets
- bootloader — bootloader binary, compiled from `esp-idf/components/esptool_py/bootloader`
- ota_nofs_4MB.csv — partition table for rollback-enabled OTA
- version, version.py, build_info.py — version/build metadata injection

## Build & Flash

1. Install PlatformIO
2. Connect the ESP32-C3 board
3. Use the `devel` environment for development builds:
   - `pio run -e devel`
   - `pio run -e devel -t upload`
4. Use `prod` for production builds
5. OTA builds are available via `develOTA` and `prodOTA`

## Notes

- `loop()` is intentionally empty; tasks are scheduled via FreeRTOS.
- The project applies a custom bootloader replacement during the build.
- The display driver uses multiple canvas layers and animated page transitions.

## Planned Features

- Mobile app for recording and sharing ride data


## License

This project is licensed under the terms of the included LICENSE.
