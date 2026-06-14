/* ST7789 240x240 Display Driver */

#ifndef ST7789_240x240_H
#define ST7789_240x240_H

#include <Arduino_GFX_Library.h>

#include "display_driver.h"
#include "model/state.h"

extern State state;

#include "RobotoMono_bold48pt7b_digits.h"
#include "RobotoMono_bold30pt7b_digits.h"

class ST7789_240x240 : public DisplayDriver {
   public:
    /*

    enum MetricID {
        METRIC_SPEED,
        METRIC_CADENCE,
        METRIC_PAS,
        METRIC_MOTOR_PWR,
        METRIC_HUMAN_PWR,
        METRIC_VOLTAGE,
        METRIC_SOC,
        METRIC_RANGE,
        METRIC_HEART_RATE,
        METRIC_BODY_TEMP,
        METRIC_COUNT
    };

    struct Metric {
        MetricID id;
        const char* name;
        float value;
        const char* unit;
    };

    // Global struct to hold live data
    struct TelemetryData {
        float values[METRIC_COUNT];
        const char* units[METRIC_COUNT];
    };

    TelemetryData liveData = {
        .units = {
            "km/h",
            "rpm",
            "PAS",
            "W",
            "W",
            "V",
            "%",
            "km",
            "bpm",
            "°C"
        }
    };

    // Define what goes on each page
    struct PageLayout {
        MetricID major;
        MetricID minor1;
        MetricID minor2;
    };

    PageLayout pages[] = {
        { METRIC_SPEED,     METRIC_PAS,        METRIC_SOC },       // Page 1: Standard Cruise
        { METRIC_HUMAN_PWR, METRIC_MOTOR_PWR,  METRIC_CADENCE },   // Page 2: Power & Performance
        { METRIC_SPEED,     METRIC_HEART_RATE, METRIC_RANGE }      // Page 3: Fitness & Range
    };

    uint8_t currentPage = 0;
    uint8_t totalPages = sizeof(pages) / sizeof(PageLayout);


    */

    struct Area {
        uint8_t x = 0;
        uint8_t y = 0;
        uint8_t w = 0;
        uint8_t h = 0;
        Arduino_Canvas_Mono* canvas = nullptr;
        GFXfont* twoDigitFont = nullptr;
        GFXfont* threeDigitFont = nullptr;
        std::function<bool(void)> inverted = nullptr;
        std::function<bool(void)> needsRefresh = nullptr;
    };

    ST7789_240x240(
        int8_t cs,
        int8_t dc,
        int8_t mosi,
        int8_t sck,
        int8_t rst = -1,
        int8_t bl = -1,
        uint8_t spi = SPI2_HOST,
        uint8_t rot = 0,
        uint16_t w = 240,
        uint16_t h = 240)
        : bl(bl), w(w), h(h) {
        bus = new Arduino_ESP32SPIDMA(dc, cs, sck, mosi, GFX_NOT_DEFINED, spi);
        tft = new Arduino_ST7789(bus, rst, rot, true, w, h);
        // canvas = new Arduino_Canvas_Mono(w, h, tft, 0, 0);
        // width 232 is perfectly divisible by 8 (232 / 8 = 29 bytes per row)
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

        clear();

        if (!canvasMajor->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !canvasMinor1->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !canvasMinor2->begin(GFX_SKIP_OUTPUT_BEGIN)) {
            while (true) {
                ESP_LOGE(tag, "canvas begin() failed");
                delay(1000);
            }
        }

        canvasMajor->setTextColor(WHITE, BLACK);
        canvasMinor1->setTextColor(WHITE, BLACK);
        canvasMinor2->setTextColor(WHITE, BLACK);
    }

    void splash() override {
        canvasMajor->fillScreen(BLACK);
        canvasMinor1->fillScreen(BLACK);
        canvasMinor2->fillScreen(BLACK);
        canvasMajor->drawRect(0, 0, canvasMajor->width(), canvasMajor->height(), WHITE);
        canvasMinor1->drawRect(0, 0, canvasMinor1->width(), canvasMinor1->height(), WHITE);
        canvasMinor2->drawRect(0, 0, canvasMinor2->width(), canvasMinor2->height(), WHITE);

        canvasMajor->flush();
        canvasMinor1->flush();
        canvasMinor2->flush();
    }

    virtual void update() override {
        ulong t = millis();
        if (t < 2000) return;  // show splash for at least 2 seconds
        static ulong last = 0;
        if (t - last < 100) return;  // update at most every 100 ms
        last = t;

        State::Snapshot s = state.getSnapshot();
        drawMajor(s.humanPower());
        drawMinor1((float)s.cadence);
        drawMinor2((float)s.pasLevel);

        // ESP_LOGD(taskName(), "Update took %d ms", millis() - t);
    }

    void clear() override {
        fillScreen(BLACK);
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

    void drawMajor(float v) {
        bool invert = keyUpClick;
        keyUpClick = false;
        drawCanvas(
            canvasMajor,
            v,
            v >= 100.0f ? mediumFont : largeFont,
            invert,
            2,
            5);
    }

    void drawMinor1(float v) {
        bool invert = keyDownClick;
        keyDownClick = false;
        drawCanvas(
            canvasMinor1,
            v,
            v >= 100.0f ? mediumFont : largeFont,
            invert,
            1,
            2);
    }

    void drawMinor2(float v) {
        bool invert = keyPowerClick;
        keyPowerClick = false;
        drawCanvas(
            canvasMinor2,
            v,
            v >= 100.0f ? mediumFont : largeFont,
            invert,
            1,
            2);
    }

    void fillScreen(uint16_t color) override {
        tft->fillScreen(color);
    }

    void setRotation(uint8_t rotation) override {
        tft->setRotation(rotation);
    }

    // exponential curve biased towards the low end
    void setBrightnessPercent(uint8_t p) override {
        if (!hasBacklight()) return;
        if (p == 0) {
            analogWrite((uint8_t)bl, 0);
            return;
        }

        // loosely follows Weber's law
        // uint8_t val = (uint8_t)(255.0f * (expf(p / 100.0f) - 1.0f) / (expf(1.0f) - 1.0f));

        // tunable gamma-squared curve
        constexpr float GAMMA = 3.0f;
        uint8_t val = (uint8_t)(254.0f * powf(p / 100.0f, GAMMA)) + 1;

        ESP_LOGD(tag, "setBrightnessPercent(%d) -> %d", p, val);
        analogWrite((uint8_t)bl, val);
    }

    bool hasBacklight() override {
        return bl >= 0;
    }

   protected:
    const char* tag = "ST7789_240x240";
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

#endif  // ST7789_240x240_H
