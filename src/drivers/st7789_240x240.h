/* ST7789 240x240 Display Driver */

#ifndef ST7789_240x240_H
#define ST7789_240x240_H

#include <Arduino_GFX_Library.h>

#include "display_driver.h"
#include "model/state.h"

extern State state;

/*
TODO: Implement page switching and a menu system

Preparation

Analyze the mockups include/display_mockups/*.png.
Calculate the pixel coordinates of each metric area, and define a data structure to hold the coordinates and dimensions of each area. Define a data structure to hold the metrics to be displayed on each page, and the order of the pages. Define a data structure to hold the menu items and their actions.
The font we are using is Roboto Mono. Define the required font sizes and character ranges, I will generate the GFX font files using the fontconvert utility.
Displaying metric units is not necessary at this stage.
The empty area at the middle of the display will later be used for graphical elements like a speedometer and a battery level indicator, icons for BLE and WiFi, etc. For now we can just leave it blank.
Feel free to ask me any questions about the requirements or the implementation details. The goal of this stage is to implement a basic page switching mechanism with a simple fade transition, and a simple menusystem, to lay the groundwork for the more complex features in later stages.


Stage 1

Remove the current hard-coded metrics and replace with a page system. Each page will have a set of metrics to display, and the user can switch between pages using the KEY_UP and KEY_DOWN buttons.
A page switch consists of
  - clearing the metric areas
  - drawing the metric labels
  - gradually fading out the labels while fading in the live metric values over 1.5 seconds (tuneable).
The live metric values are updated every ~100ms, but during the fade transition the values should be updated at a faster rate (still limited and tuneable).
Update a metric only if it has changed since the last update, to improve performance.


Stage 2

Implement cunfigurable pages. Start with a hard-coded default set. The user can configure which metrics are displayed on each page, and the order of the pages. Settings are saved to non-volatile memory (Preferences) and restored on power-up. Implement page configuration via the API.


Stage 3

Implement a menu system. The user can enter the menu using a simultaneous long press of the KEY_UP and KEY_DOWN buttons, navigate through the menu using the KEY_UP and KEY_DOWN buttons, and select menu items using the KEY_POWER button. The menu system should allow the user to configure the pages and metrics displayed on each page. The menu system should use the API, and be intuitive and easy to use, with clear feedback for the user. Consider using a library for the menu system, or implement a simple custom menu system. The menu system should be easy to extend with new menu items and actions.


General coding guidelines

Validate all user input and API commands. If an invalid value is received, ignore it and log a warning message. Do not crash or hang the system.

Check for null pointers and invalid values before using them. If a null pointer or invalid value is detected, log an error message and return from the function.

Avoid deep nesting of if statements. Use early returns to simplify the code and improve readability.

Keep cyclomatic complexity low. If a function is too complex, break it down into smaller functions with clear responsibilities.

Prefer constexpr and const variables over #define macros. Use constexpr for compile-time constants and const for run-time constants.

Prefer local static variables over class-level variables.

Avoid repeated code. Use functions and classes to encapsulate common functionality.

The data structures below are suggestions only. You are free to use your own data structures and algorithms as long as they meet the requirements and are efficient and maintainable. For example, the Metric struct may be implemented as a class with methods for getting and formatting the value, instead of using function pointers. But keep memory usage and performance in mind, as this is an embedded system with limited resources.

We are using monochrome graphics for maximum readability and memory efficiency. In later stages we can consider using a limited number of colors for highlighting, but the primary focus is on readability and clarity.

keep in mind that GFX fonts are positioned with the baseline at y=0, so the y coordinate of the text is the baseline, not the top of the text. This is important for aligning text and graphics correctly. To render text cleanly at the top-left of a bounding box, the cursor Y position must be offset downward by the font's internal ascender height (font_struct->glyName->yOffset).

*/

#include "RobotoMono_bold48pt7b_digits.h"
#include "RobotoMono_bold30pt7b_digits.h"

class ST7789_240x240 : public DisplayDriver {
   public:
    enum MetricID {
        METRIC_SPEED,       // km/h
        METRIC_CADENCE,     // RPM
        METRIC_PAS,         // Pedal Assist Level, -1 = Walk assist, 0 = Off, 1-5 = PAS levels
        METRIC_MOTOR_PWR,   // Watts
        METRIC_HUMAN_PWR,   // Watts
        METRIC_VOLTAGE,     // battery voltage
        METRIC_SOC,         // battery State Of Charge
        METRIC_MOTOR_TEMP,  // °C
        METRIC_TRIP,        // km, unused, TODO: Parse CAN trip data, add resettable trip functionality
        METRIC_ODO,         // km, unused, TODO: Parse CAN odometer data
        METRIC_RANGE,       // km, unused, TODO: Implement live range estimation based on current battery level, user-configurable battery capacity, and power consumption
        METRIC_HEART_RATE,  // bpm, unused, TODO: Add BLE heart rate support
        METRIC_BODY_TEMP,   // °C, unused, TODO: Add BLE body temperature support
        METRIC_COUNT        // total number of metrics
    };

    struct Metric {
        MetricID id;
        const char* name;  // Short name for the metric, e.g. "Speed", "Cadence", etc.
        char label[5];     // All-Caps label for the metric, max 4 chars + null terminator
        const char* unit;

        // Function to get the value of the metric from the State::Snapshot
        // If the snapshot is null, get the value from the latest state using getSnapshot(true).
        std::function<uint16_t(State::Snapshot* snapshot)> value = nullptr;

        // Function to format the value of the metric into a string for display
        // If the value is null, get the value from the latest state using value().
        // isNumeric is set to true if formatted string is numeric, false if it contains non-numeric characters (e.g. ODO: "12k", PAS: "off", PAS: "wlk").
        std::function<void(char* buffer, size_t bufferSize, uint16_t* value, bool* isNumeric)> format = nullptr;
    };

    struct PageLayout {
        MetricID major;
        MetricID minor1;
        MetricID minor2;
    };

    /*
    PageLayout pages[3] = {
        {METRIC_SPEED, METRIC_MOTOR_PWR, METRIC_HUMAN_PWR},
        {METRIC_PAS, METRIC_BODY_TEMP, METRIC_CADENCE},
        {METRIC_HEART_RATE, METRIC_SOC, METRIC_RANGE}
    };

    uint8_t currentPage = 0;
    uint8_t totalPages = sizeof(pages) / sizeof(PageLayout);
    */

    struct Area {
        uint8_t width() { return canvas ? (uint8_t)canvas->width() : 0; }
        uint8_t height() { return canvas ? (uint8_t)canvas->height() : 0; }
        uint8_t x = 0;
        uint8_t y = 0;
        Arduino_Canvas_Mono* canvas = nullptr;
        GFXfont* twoDigitFont = nullptr;
        GFXfont* threeDigitFont = nullptr;
        GFXfont* labelFont = nullptr;
        std::function<bool(void)> inverted = nullptr;
        std::function<bool(void)> dirty = nullptr;
        Area(
            uint8_t w,
            uint8_t h,
            uint8_t x,
            uint8_t y,
            Arduino_GFX* bus,
            GFXfont* twoDigitFont,
            GFXfont* threeDigitFont,
            GFXfont* labelFont,
            std::function<bool(void)> inverted,
            std::function<bool(void)> dirty) : x(x),
                                               y(y),
                                               twoDigitFont(twoDigitFont),
                                               threeDigitFont(threeDigitFont),
                                               labelFont(labelFont),
                                               inverted(inverted),
                                               dirty(dirty) {
            canvas = new Arduino_Canvas_Mono((int16_t)w, (int16_t)h, bus, (int16_t)x, (int16_t)y);
        };
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
