#ifndef ST7789_H
#define ST7789_H

#include <Arduino_GFX_Library.h>

#include "display.h"

#include "RobotoMono_bold48pt7b_digits.h"
#include "RobotoMono_bold30pt7b_digits.h"

class ST7789 : public DisplayDriver {
   public:
    ST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sck, int8_t rst = -1, int8_t bl = -1, uint8_t spi = SPI2_HOST, uint8_t rot = 0, uint16_t w = 240, uint16_t h = 240)
        : bl(bl), w(w), h(h) {
        bus = new Arduino_ESP32SPIDMA(dc, cs, sck, mosi, GFX_NOT_DEFINED, spi);
        tft = new Arduino_ST7789(bus, rst, rot, true, w, h);
        // canvas = new Arduino_Canvas_Mono(w, h, tft, 0, 0);
        // idth 232 is perfectly divisible by 8 (232 / 8 = 29 bytes per row)
        canvasMajor = new Arduino_Canvas_Mono(232, 140, tft, 4, 0);
        canvasMinor1 = new Arduino_Canvas_Mono((w - 15) / 2, h - 145, tft, 5, 145);
        canvasMinor2 = new Arduino_Canvas_Mono((w - 15) / 2, h - 145, tft, (w - 15) / 2 + 10, 145);
    }

    void setup() override {
        if (bl >= 0) {
            pinMode(bl, OUTPUT);
            digitalWrite(bl, LOW);
        }

        // 1. Initialize hardware screen FIRST
        if (!tft->begin()) {
            ESP_LOGE(tag, "TFT Hardware begin failed");
            while (true) delay(1000);
        }

        // 2. Initialize ALL canvases with GFX_SKIP_OUTPUT_BEGIN
        if (!canvasMajor->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !canvasMinor1->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !canvasMinor2->begin(GFX_SKIP_OUTPUT_BEGIN)) {
            ESP_LOGE(tag, "GFX canvas begin() failed");
            while (true) delay(1000);
        }

        clear();

        canvasMajor->setTextColor(RGB565_WHITE, RGB565_BLACK);
        canvasMinor1->setTextColor(RGB565_WHITE, RGB565_BLACK);
        canvasMinor2->setTextColor(RGB565_WHITE, RGB565_BLACK);

        canvasMajor->fillScreen(RGB565_BLACK);
        canvasMinor1->fillScreen(RGB565_BLACK);
        canvasMinor2->fillScreen(RGB565_BLACK);

        canvasMajor->drawRect(0, 0, canvasMajor->width(), canvasMajor->height(), RGB565_WHITE);
        canvasMinor1->drawRect(0, 0, canvasMinor1->width(), canvasMinor1->height(), RGB565_WHITE);
        canvasMinor2->drawRect(0, 0, canvasMinor2->width(), canvasMinor2->height(), RGB565_WHITE);

        canvasMajor->flush();
        canvasMinor1->flush();
        canvasMinor2->flush();
        setBrightnessPercent(100);
        delay(2000);  // show rects for 2 seconds
    }

    void clear() override {
        fillScreen(RGB565_BLACK);
    }

    void drawText(const char* buf) override {
        tft->setFont(largeFont);
        tft->setTextSize(1);
        tft->setCursor(5, 140);
        tft->print(buf);
        tft->flush();
    }

    void drawMajor(float v) override {
        char buf[3];
        snprintf(buf, sizeof(buf), "%.0f", v);
        canvasMajor->setFont(largeFont);
        canvasMajor->setTextSize(2);
        canvasMajor->fillRect(0, 0, canvasMajor->width(), canvasMajor->height(), RGB565_BLACK);
        canvasMajor->setCursor(0, canvasMajor->height() - 5);
        canvasMajor->print(buf);
        canvasMajor->flush();
    }

    void drawMinor1(float v) override {
        char buf[4];
        snprintf(buf, sizeof(buf), "%.0f", v);
        canvasMinor1->setFont(v >= 100.0f ? mediumFont : largeFont);
        canvasMinor1->setTextSize(1);
        canvasMinor1->fillRect(0, 0, canvasMinor1->width(), canvasMinor1->height(), RGB565_BLACK);
        canvasMinor1->setCursor(0, canvasMinor1->height() - 2);
        canvasMinor1->print(buf);
        canvasMinor1->flush();
    }

    void drawMinor2(float v) override {
        char buf[3];
        snprintf(buf, sizeof(buf), "%.0f", v);
        canvasMinor2->setFont(largeFont);
        canvasMinor2->setTextSize(1);
        canvasMinor2->fillRect(0, 0, canvasMinor2->width(), canvasMinor2->height(), RGB565_BLACK);
        canvasMinor2->setCursor(0, canvasMinor2->height() - 2);
        canvasMinor2->print(buf);
        canvasMinor2->flush();
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
    const char* tag = "ST7789";
    int8_t bl;
    uint16_t w;
    uint16_t h;

    Arduino_DataBus* bus = nullptr;
    Arduino_GFX* tft = nullptr;

    Arduino_Canvas_Mono* canvas = nullptr;
    Arduino_Canvas_Mono* canvasMajor = nullptr;
    Arduino_Canvas_Mono* canvasMinor1 = nullptr;
    Arduino_Canvas_Mono* canvasMinor2 = nullptr;

    const GFXfont* largeFont = &RobotoMono_Bold48pt7b;
    const GFXfont* mediumFont = &RobotoMono_Bold30pt7b;
};

#endif  // ST7789_H
