#ifndef ST7789_H
#define ST7789_H

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "display.h"

class ST7789 : public DisplayDriver {
   public:
    ST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst = -1, int8_t bl = -1)
        : tft(cs, dc, mosi, sclk, rst), bl(bl) {}

    void setup() override {
        tft.init(240, 240);
        tft.setRotation(2);
        setBrightnessPercent(100);
        clear();
    }

    void clear() override {
        tft.fillScreen(ST77XX_BLACK);
    }

    void drawText(const char* text) override {
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(16);
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
    Adafruit_ST7789 tft;
    int8_t bl = -1;  // backlight pin
};

#endif  // ST7789_H