#include <Arduino.h>

class DisplayDriver {
   public:
    virtual void setup() = 0;
    virtual void clear() = 0;
    virtual void drawText(const char* text) = 0;
    virtual void fillScreen(uint16_t color) = 0;
    virtual void setRotation(uint8_t rotation) = 0;
    virtual void setBrightness(uint8_t brightness) {};
    virtual bool hasBacklight() { return false; };
};