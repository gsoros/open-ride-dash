#ifndef UI_EVENTS_H
#define UI_EVENTS_H

#include <Arduino.h>

enum class UiEvent : uint8_t {
    UpClick,
    DownClick,
    PowerClick,
    UpLong,
    DownLong,
    PowerLong,
    MenuChord,     // Sent when the menu chord is pressed
    Sleep,         // Sent when the system is about to sleep.
    Restart,       // Sent when the system is about to restart.
    PasskeyStart,  // Sent when a passkey is required. Actual passkey is State::blePassKey().
    PasskeyEnd,    // Passkey requirement ended. Sent after every authentication, even if the passkey was never used (reconnect) or when a wrong passkey was entered.
    OtaChange,     // Sent when the OTA state changes. Actual state is State::ota().
    WifiChange,    // Sent on WiFi status changes (STA and AP start/stop/connect/disconnect).
    BleChange,     // Sent on BLE status changes (enabled/disabled/connect/disconnect).
    NextPage,      // Request the Display task to advance to the next metrics page.
};

#endif  // UI_EVENTS_H
