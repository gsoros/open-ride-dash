/* Display Driver Interface */

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>

class DisplayDriver {
   public:
    virtual void setup() = 0;
    virtual void clear() = 0;
    virtual void splash() = 0;
    virtual void update() = 0;
    virtual void fillScreen(uint16_t color) = 0;
    virtual void setRotation(uint8_t rotation) = 0;
    virtual void setBrightnessPercent(uint8_t percent) {};
    virtual bool hasBacklight() { return false; };
    virtual bool setBacklight(uint8_t level) { return false; };
    virtual bool hasBrightnessControl() { return false; };
    virtual void onSleep() {};
    virtual bool showPasskey(uint32_t passkey) { return false; };
    virtual void exitPasskey() {};

    virtual void nextPage() {};
    virtual uint8_t currentPage() { return 0; };
};

#endif  // DISPLAY_DRIVER_H
