#ifndef BLE_H
#define BLE_H

/*
    BLE (GAP Peripheral, GATT Server) task using NimBLE.

    IMPLEMENTATION PLAN:

    Goals:
    - Provide basic telemetry to mobile fitness apps using standard services.
    - Provide richer telemetry to the custom mobile app.
    - Provide an API bridge to the custom mobile app via NUS once the app is ready.

    Constraints and policies:
    - Maximum connections: 1. Advertising stops immediately upon connection.
    - Security: use authenticated pairing and keypad confirmation for the first pairing of CTS/NUS.
      Standard services should remain broadly compatible unless there is a clear need to lock them down.
    - Advertising payload should fit the legacy 31-byte limit and include flags, appearance,
      and the mandatory service UUIDs for CSC/CPS compatibility.
    - Minimize BLE stack footprint, memory use, and fragmentation.
    - Avoid blocking other tasks, especially CAN and Display.
    - Avoid race conditions with other tasks.

    Service plan:
    1. DIS + BAS
       - Implement Device Information Service and Battery Service first.
       - Add basic advertising and connection handling.
       - Validate pairing behavior and the initial security model early.

    2. CSC + CPS
       - Add Cycling Speed & Cadence and Cycling Power services.
       - Send updates at a low rate, typically 1-2 Hz max, and only when values change.
       - Keep the rolling counter logic simple and deterministic.

    3. CTS
       - Add a compact custom telemetry service for the mobile app.
       - Use a fixed-size payload derived from the bike state snapshot.
       - Apply the same change-based rate limiting as the standard services.

    4. NUS
       - Add this last, once the mobile app is in active development.
       - Keep a hardcoded payload limit, for example 250 bytes.
       - Keep the initial protocol simple and best-effort; fragmentation can be deferred.

    GATT profile:
    - DIS: 0x180A, read-only device information.
    - CSC: 0x1816, measurement characteristic 0x2A5B, notify.
    - CPS: 0x1818, measurement characteristic 0x2A63, notify.
    - BAS: 0x180F, level characteristic 0x2A19, read/notify.
    - CTS: custom 128-bit service with a telemetry characteristic, notify.
    - NUS: Nordic UART service with RX/TX characteristics, write for RX and read/notify for TX.

    Implementation notes:
    - The BLE task owns the BLE state machine, connection lifecycle, and outgoing notifications.
    - Work from a fixed snapshot of the bike state rather than reading live state directly in several places.
    - Keep notification paths non-blocking, bounded, and based on fixed-size buffers.
    - Use a small state machine: init -> advertise -> connected -> secured -> ready -> disconnect.
    - Prefer change detection and coalescing over sending every update.
    - Avoid blocking the task with long serialization or large buffers.
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>

#include "task.h"

class Ble : public Task {
   public:
    virtual const char* taskName() const override {
        return "BLE";
    }

    virtual void setup();
    virtual void taskRun() override;

    void handleConnect();
    void handleDisconnect();

   private:
    void updateBatteryLevel();

    bool _connected = false;
    uint8_t _batteryLevel = 0;
    uint32_t _lastBatteryPublishMs = 0;
    BLEServer* _server = nullptr;
    BLECharacteristic* _batteryCharacteristic = nullptr;
};

#endif  // BLE_H