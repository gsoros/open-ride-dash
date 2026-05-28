#include "keypad.h"

void Keypad::keyUpChanged() {
    /*
    static ulong lastUs = 0;
    ulong now = micros();
    if (now - lastUs < 500) return;  // .5ms debounce
    lastUs = now;
    */
    state.keyUp = digitalRead(KEY_UP) == LOW;
}

void Keypad::keyDownChanged() {
    /*
    static ulong lastUs = 0;
    ulong now = micros();
    if (now - lastUs < 500) return;  // .5ms debounce
    lastUs = now;
    */
    state.keyDown = digitalRead(KEY_DOWN) == LOW;
}

void Keypad::keyPowerChanged() {
    /*
    static ulong lastUs = 0;
    ulong now = micros();
    if (now - lastUs < 500) return;  // .5ms debounce
    lastUs = now;
    */
    state.keyPower = digitalRead(KEY_POWER) == LOW;
}