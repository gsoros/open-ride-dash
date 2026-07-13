#ifndef UI_EVENTS_H
#define UI_EVENTS_H

#include <Arduino.h>

enum class UiEvent : uint8_t {
    UpClick,
    DownClick,
    SelectClick,
    UpLongPress,
    DownLongPress,
    PowerLongPress,
    MenuChord,
    Sleep,         // Sent when the system is about to sleep.
    PasskeyStart,  // Sent when a passkey is required. Actual passkey is State::blePassKey().
    PasskeyEnd,    // Passkey requirement ended. Sent after every authentication, even if the passkey was never used (reconnect) or when a wrong passkey was entered.
    OtaChange,     // Sent when the OTA state changes. Actual state is State::ota().
};

#endif  // UI_EVENTS_H
