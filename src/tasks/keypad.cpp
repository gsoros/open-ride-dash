#include "keypad.h"

#define KEYPAD_DEBOUNCE_US 100000  // 100 ms

void Keypad::keyUpChanged() {
#ifdef KEYPAD_DEBOUNCE_US
    static uint32_t last = 0;
    uint32_t now = micros();
    if (now - last < KEYPAD_DEBOUNCE_US) return;
    last = now;
#endif
    state.keyUp = digitalRead(KEY_UP) == LOW;
}

void Keypad::keyDownChanged() {
#ifdef KEYPAD_DEBOUNCE_US
    static uint32_t last = 0;
    uint32_t now = micros();
    if (now - last < KEYPAD_DEBOUNCE_US) return;
    last = now;
#endif
    state.keyDown = digitalRead(KEY_DOWN) == LOW;
}

void Keypad::keyPowerChanged() {
#ifdef KEYPAD_DEBOUNCE_US
    static uint32_t last = 0;
    uint32_t now = micros();
    if (now - last < KEYPAD_DEBOUNCE_US) return;
    last = now;
#endif
    state.keyPower = digitalRead(KEY_POWER) == LOW;
}