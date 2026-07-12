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
    Sleep,
    PasskeyStart,  // Sent when a passkey is required
    PasskeyEnd,    // Sent after every authentication, even if the passkey was never used (reconnect) or when a wrong passkey was entered
};

#endif  // UI_EVENTS_H
