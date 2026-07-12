#include "ble.h"

/*
    BLE task implementation plan.

    Scope:
    - Provide basic BLE telemetry via standard services first.
    - Expose a compact custom telemetry service for the mobile app.
    - Add a simple UART-like bridge later, once the app is ready.

    Architecture:
    - The BLE task owns the BLE state machine, connection lifecycle, and outgoing notifications.
    - It should work from a fixed snapshot of the bike state, not by reading live state directly
      from multiple places. The natural input is State::Snapshot from model/state.h.
    - Notification paths should be non-blocking, bounded, and use fixed-size buffers.

    Phased implementation:
    1. DIS + BAS
       - Implement Device Information Service and Battery Service.
       - Add basic advertising and connection handling.
       - Validate the first pairing flow and confirm the security model early.

    2. CSC + CPS
       - Add Cycling Speed & Cadence and Cycling Power services.
       - Send updates at a low rate, e.g. 1-2 Hz maximum, and only when values change.
       - Keep the rolling counter logic simple and deterministic.

    3. CTS
       - Add a compact custom telemetry service for the mobile app.
       - Payload should be fixed-size and contain the most useful fields from the state snapshot,
         such as speed, range, assist level, battery voltage/current, and temperatures.
       - Apply the same rate limiting and change suppression as the standard services.

    4. NUS
       - Add this last, once the mobile app is in active development.
       - Keep a hardcoded payload limit, for example 250 bytes.
       - The initial version can stay simple and best-effort; fragmentation can be deferred until
         there is a concrete app requirement.

    Security:
    - The goal is to prevent random BLE clients from sending API commands to the e-bike.
    - For CTS and NUS, require authenticated pairing and a confirmation step on the keypad
      for the first pairing.
    - Keep standard services broadly compatible unless there is a clear reason to lock them down.

    Implementation notes:
    - Use a small state machine: init -> advertise -> connected -> secured -> ready -> disconnect.
    - Avoid dynamic allocation on the notification path.
    - Prefer change detection and coalescing over sending every update.
    - Avoid blocking the task with long serialization or large buffers.
*/