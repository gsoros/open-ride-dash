#ifndef ST7789_H
#define ST7789_H

#include <Arduino_GFX_Library.h>

#include "display.h"
#include "model/state.h"

extern State state;

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

        if (!tft->begin()) {
            ESP_LOGE(tag, "begin() failed");
            while (true) delay(1000);
        }

        if (!canvasMajor->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !canvasMinor1->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !canvasMinor2->begin(GFX_SKIP_OUTPUT_BEGIN)) {
            ESP_LOGE(tag, "canvas begin() failed");
            while (true) delay(1000);
        }

        clear();

        canvasMajor->setTextColor(WHITE, BLACK);
        canvasMinor1->setTextColor(WHITE, BLACK);
        canvasMinor2->setTextColor(WHITE, BLACK);

        canvasMajor->fillScreen(BLACK);
        canvasMinor1->fillScreen(BLACK);
        canvasMinor2->fillScreen(BLACK);

        canvasMajor->drawRect(0, 0, canvasMajor->width(), canvasMajor->height(), WHITE);
        canvasMinor1->drawRect(0, 0, canvasMinor1->width(), canvasMinor1->height(), WHITE);
        canvasMinor2->drawRect(0, 0, canvasMinor2->width(), canvasMinor2->height(), WHITE);

        canvasMajor->flush();
        canvasMinor1->flush();
        canvasMinor2->flush();
        setBrightnessPercent(100);
        delay(2000);  // show rects for 2 seconds
    }

    void clear() override {
        fillScreen(BLACK);
    }

    void drawText(const char* buf) override {
        tft->setFont(largeFont);
        tft->setTextSize(1);
        tft->setCursor(5, 140);
        tft->print(buf);
        tft->flush();
    }

    void drawCanvas(
        Arduino_Canvas_Mono* canvas,
        float v,
        const GFXfont* font,
        bool invertColors = false,
        uint8_t size = 1,
        uint8_t verticalOffset = 0,
        uint16_t fg = WHITE,
        uint16_t bg = BLACK) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%.0f", v);
        canvas->setFont(font);
        canvas->setTextSize(size);
        uint16_t textColor = invertColors ? bg : fg;
        uint16_t backgroundColor = invertColors ? fg : bg;
        canvas->setTextColor(textColor, backgroundColor);
        canvas->fillRect(0, 0, canvas->width(), canvas->height(), backgroundColor);
        canvas->setCursor(0, canvas->height() - verticalOffset);
        canvas->print(buf);
        canvas->flush();
    }

    void drawMajor(float v) override {
        drawCanvas(
            canvasMajor,
            v,
            v >= 100.0f ? mediumFont : largeFont,
            state.keyUp,
            2,
            5);
    }

    void drawMinor1(float v) override {
        drawCanvas(
            canvasMinor1,
            v,
            v >= 100.0f ? mediumFont : largeFont,
            state.keyDown,
            1,
            2);
    }

    void drawMinor2(float v) override {
        drawCanvas(
            canvasMinor2,
            v,
            v >= 100.0f ? mediumFont : largeFont,
            state.keyPower,
            1,
            2);
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
