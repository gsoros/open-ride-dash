#ifndef ST7789_H
#define ST7789_H

#include <TFT_eSPI.h>
#include "display.h"

class ST7789 : public DisplayDriver {
   public:
    ST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst = -1, int8_t bl = -1)
        : bl(bl) {
        // TFT_eSPI library uses preprocessor macros to define pin numbers, so we can't pass them as parameters.
        (void)cs;
        (void)dc;
        (void)mosi;
        (void)sclk;
        (void)rst;
    }

    void setup() override {
        tft.init();
        tft.setRotation(0);
        setBrightnessPercent(100);
        clear();
    }

    void clear() override {
        tft.fillScreen(TFT_BLACK);
    }

    void drawText(const char* text) override {
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(32);
        tft.setCursor(20, 40);
        tft.print(text);
    }

    void fillScreen(uint16_t color) override {
        tft.fillScreen(color);
    }

    void setRotation(uint8_t rotation) override {
        tft.setRotation(rotation);
    }

    void setBrightnessPercent(uint8_t p) override {
        if (!hasBacklight()) return;
        analogWrite((uint8_t)bl, (p * 255) / 100);
    }

    bool hasBacklight() override {
        return bl >= 0;
    }

   private:
    TFT_eSPI tft;
    int8_t bl = -1;  // backlight pin
};

#endif  // ST7789_H
