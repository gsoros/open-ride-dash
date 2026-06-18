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
};

#endif  // UI_EVENTS_H
