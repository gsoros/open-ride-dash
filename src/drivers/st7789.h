#ifndef ST7789_H
#define ST7789_H

#include <Arduino_GFX_Library.h>

#include "display.h"

#include "RobotoMono_bold48pt7b_digits.h"

class ST7789 : public DisplayDriver {
   public:
    ST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sck, int8_t rst = -1, int8_t bl = -1, uint8_t spi = SPI2_HOST, uint8_t rot = 0, uint16_t w = 240, uint16_t h = 240)
        : bl(bl) {
        bus = new Arduino_ESP32SPIDMA(dc, cs, sck, mosi, GFX_NOT_DEFINED, spi);
        tft = new Arduino_ST7789(bus, rst, rot, true, w, h);
        canvas = new Arduino_Canvas_Mono(w, h, tft);
    }

    void setup() override {
        if (bl >= 0) {
            pinMode(bl, OUTPUT);
            digitalWrite(bl, LOW);
        }

        if (!canvas->begin()) {
            while (true) {
                ESP_LOGE(tag, "canvas begin() failed");
                delay(10000);
            }
        }
        canvas->setTextColor(WHITE, BLACK);
    }

    void clear() override {
        fillScreen(BLACK);
    }

    void drawText(const char* buf) override {
        canvas->setFont(largeFont);
        canvas->setTextSize(2);
        canvas->setCursor(5, 140);
        canvas->print(buf);
        canvas->setTextSize(1);
        canvas->setCursor(5, 235);
        canvas->print(buf);
        canvas->setCursor(120, 235);
        canvas->print(buf);
        canvas->flush();
    }

    void fillScreen(uint16_t color) override {
        canvas->fillScreen(color);
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
    Arduino_Canvas_Mono* canvas = nullptr;
    const char* tag = "ST7789";

    // RobotoMono_Bold48pt7b
    const GFXfont* largeFont = &RobotoMono_Bold48pt7b;
};

#endif  // ST7789_H
