#ifndef BLE_H
#define BLE_H

/*
    BLE (GAP Peripheral, GATT Server) task using NimBLE.

    GOALS:

    - Provide comprehensive telemetry data to the custom mobile app.
    - Provide basic telemetry data to mobile fitness apps using standard services.
    - Provide an API bridge to the custom mobile app via NUS.


    CONSTRAINTS & POLICIES:

    - Maximum connections: 1. Advertising stops immediately upon connection.
    - Security: Enforces LE Secure Connections with MITM protection (Passkey/Numeric Comparison)
      for CTS and NUS.
    - Advertising Data: Includes Flags, Appearance (0x0484 - Cycling Computer),
      and Mandatory UUIDs (0x1816, 0x1818) to fit 31-byte legacy limit.
    - Minimize BLE stack footprint.
    - Minimize BLE stack memory usage.
    - Minimize BLE stack memory fragmentation.
    - Avoid blocking other tasks, especially CAN and Display.
    - Avoid race conditions with other tasks.


    GATT PROFILE:

    ┌───────────────────────────────┬─────────────┬──────────────────────┬────────────────────────┐
    │ Service Name                  │ UUID        │ Characteristic(s)    │ Properties / Flags     │
    ├───────────────────────────────┼─────────────┼──────────────────────┼────────────────────────┤
    │ Device Information (DIS)      │ 0x180A      │ Model, Mfr, FW Ver   │ READ (Unencrypted)     │
    │ Cycling Speed & Cadence (CSC) │ 0x1816      │ Measurement (0x2A5B) │ NOTIFY (1-2Hz)         │
    │ Cycling Power (CPS)           │ 0x1818      │ Measurement (0x2A63) │ NOTIFY (Human Only)    │
    │ Battery Service (BAS)         │ 0x180F      │ Level (0x2A19)       │ READ, NOTIFY (On Chg)  │
    │ Custom Telemetry Service (CTS)│ 128B Random │ Telemetry (0x0002)   │ NOTIFY (Motor/Range)   │
    │ Nordic UART Service (NUS)     │ 6e400001... │ RX (6e400002...)     │ WRITE (Encrypted/Auth) │
    │                               │             │ TX (6e400003...)     │ READ, NOTIFY (On Chg)  │
    └───────────────────────────────┴─────────────┴──────────────────────┴────────────────────────┘


    RATE LIMITING:

    - CSC / CPS / CTS: Dispatched at 1Hz or 2Hz intervals maximum, and only on change.
    - BAS: Dispatched only when SOC percentage changes, at 0.2Hz maximum.
    - NUS: Event-driven asynchronously upon app write action.


*/

#include "task.h"

class Ble : public Task {
   public:
    virtual const char* taskName() const override {
        return "BLE";
    }

    virtual void setup();
    virtual void taskRun() override;
};

#endif  // BLE_H