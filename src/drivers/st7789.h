#ifndef ST7789_H
#define ST7789_H

#include <Arduino_GFX_Library.h>

#include "display.h"

// #include "testFont.h"

class ST7789 : public DisplayDriver {
   public:
    ST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sck, int8_t rst = -1, int8_t bl = -1, uint8_t spi = SPI2_HOST, uint8_t rot = 0, uint16_t w = 240, uint16_t h = 240)
        : bl(bl) {
        bus = new Arduino_ESP32SPIDMA(dc, cs, sck, mosi, GFX_NOT_DEFINED, spi);
        tft = new Arduino_ST7789(bus, rst, rot, true, w, h);
    }

    void setup() override {
        if (bl >= 0) {
            pinMode(bl, OUTPUT);
            digitalWrite(bl, LOW);
        }

        if (!tft->begin()) {
            while (true) {
                ESP_LOGE(tag, "begin() failed");
                delay(10000);
            }
        }
    }

    void clear() override {
        tft->fillScreen(BLACK);
    }

    void drawText(const char* text) override {
        tft->setTextColor(WHITE);
        // tft->setFreeFont(largeFont);
        tft->setTextSize(8);
        // tft->drawString(text, 20, 40);
        tft->setCursor(20, 40);
        tft->print(text);
    }

    void fillScreen(uint16_t color) override {
        tft->fillScreen(color);
    }

    void setRotation(uint8_t rotation) override {
        tft->setRotation(rotation);
    }

    void setBrightnessPercent(uint8_t p) override {
        if (!hasBacklight()) return;
        analogWrite((uint8_t)bl, (p * 255) / 100);
    }

    bool hasBacklight() override {
        return bl >= 0;
    }

   protected:
    int8_t bl;
    Arduino_DataBus* bus = nullptr;
    Arduino_GFX* tft = nullptr;
    const char* tag = "ST7789";

    // Font aliases
    // const GFXfont* largeFont = &testFont32pt8b;
};

#endif  // ST7789_H
