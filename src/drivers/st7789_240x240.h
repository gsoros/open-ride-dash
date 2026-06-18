/* ST7789 240x240 Display Driver */

/*
TODO: Implement page switching and a menu system

Preparation

Analyze the mockups include/display_mockups/*.png.
Calculate the pixel coordinates of each metric area, and define a data structure to hold the coordinates and dimensions of each area. Define a data structure to hold the metrics to be displayed on each page, and the order of the pages. Define a data structure to hold the menu items and their actions.
The font we are using is Roboto Mono.
Displaying metric units is not necessary at this stage.
The empty area at the middle of the display will later be used for graphical elements like a speedometer and a battery level indicator, icons for BLE and WiFi, etc. For now we can just leave it blank.
Feel free to ask me any questions about the requirements or the implementation details. The goal of this stage is to implement a basic page switching mechanism with a simple fade transition, and a simple menu system, to lay the groundwork for the more complex features in later stages.


Stage 1 - Done

Remove the current hard-coded metrics and replace with a page system. Each page will have a set of metrics to display, and the user can switch between pages using the KEY_UP and KEY_DOWN buttons.
A page switch consists of
  - clearing the metric areas
  - drawing the metric labels
  - gradually fading out the labels while fading in the live metric values over 1.5 seconds (tuneable).
The live metric values are updated every ~100ms, but during the fade transition the values should be updated at a faster rate (still limited and tuneable).
Update a metric only if it has changed since the last update, to improve performance.


Stage 2 - In Progress

Implement cunfigurable pages. Start with a hard-coded default set. The user can configure which metrics are displayed on each page, and the order of the pages. Settings are saved to non-volatile memory (Preferences) and restored on power-up. Implement page configuration via the API.


Stage 3 - In Progress

Implement a menu system. The user can enter the menu using a simultaneous long press of the KEY_UP and KEY_DOWN buttons, navigate through the menu using the KEY_UP and KEY_DOWN buttons, and select menu items using the KEY_POWER button. The menu system should allow the user to configure the pages and metrics displayed on each page. The menu system should use the API, and be intuitive and easy to use, with clear feedback for the user. Consider using a library for the menu system, or implement a simple custom menu system. The menu system should be easy to extend with new menu items and actions.
Progress update: Basic menu state and input routing moved into a small MVC-style UI layer before adding real menu actions.
  - Keypad should only translate physical button state into UI events such as UpClick, DownClick, SelectClick, UpLongPress, DownLongPress, and MenuChord.
  - A display-independent UI/Menu controller should own menu state and behavior: enterMenu(), previousMenuItem(), nextMenuItem(), selectMenuItem(), and exitMenu().
  - Menu handlers should live in the controller or model/API layer, not in the keypad task or display driver.
  - Display drivers should only render the current UI snapshot, including menu title/items/selection/dirty state, and should not execute menu actions.
  - Communication from keypad to UI should use a small FreeRTOS queue or equivalent synchronized event path instead of cross-task mutable flags.
  - Keep Display as a facade/task initially if useful, but make ST7789_240x240 a renderer for page/menu snapshots so display hardware can be swapped more easily.


General coding guidelines

Validate all user input and API commands. If an invalid value is received, ignore it and log a warning message. Do not crash or hang the system.

Check for null pointers and invalid values before using them. If a null pointer or invalid value is detected, log an error message and return from the function.

Avoid deep nesting of if statements. Use early returns to simplify the code and improve readability.

Keep cyclomatic complexity low. If a function is too complex, break it down into smaller functions with clear responsibilities.

Prefer constexpr and const variables over #define macros. Use constexpr for compile-time constants and const for run-time constants.

Prefer local static variables over class-level variables.

Avoid repeated code. Use functions and classes to encapsulate common functionality.

We are using monochrome graphics for maximum readability and memory efficiency. In later stages we can consider using a limited number of colors for highlighting, but the primary focus is on readability and clarity.

*/

#ifndef ST7789_240x240_H
#define ST7789_240x240_H

#include <Arduino_GFX_Library.h>
#include <cstring>
#include <functional>

#include "display_driver.h"
#include "model/state.h"
#include "ui/menu_controller.h"

extern State state;

#ifndef U8G2_FONT_SECTION
#define U8G2_FONT_SECTION(name) __attribute__((section(".text." name)))
#endif

#include "fonts/u8g2/RobotoMono_Bold_90px_digits.h"
#include "fonts/u8g2/RobotoMono_Bold_48px_caps_digits.h"
#include "fonts/u8g2/RobotoMono_Regular_24px_alpha_digits.h"

class ST7789_240x240 : public DisplayDriver {
   public:
    enum MetricID {
        METRIC_SPEED,       // km/h
        METRIC_CADENCE,     // RPM
        METRIC_PAS,         // Pedal Assist Level, -1 = Walk assist, 0 = Off, 1-5 = PAS levels
        METRIC_MOTOR_PWR,   // Watts
        METRIC_HUMAN_PWR,   // Watts
        METRIC_VOLTAGE,     // V, battery voltage
        METRIC_SOC,         // %, battery State Of Charge
        METRIC_MOTOR_TEMP,  // °C
        METRIC_TRIP,        // km, unused, TODO: Parse CAN trip data, add resettable trip functionality
        METRIC_ODO,         // km, unused, TODO: Parse CAN odometer data
        METRIC_RANGE,       // km, unused, TODO: Implement live range estimation based on current battery level, user-configurable battery capacity, and power consumption
        METRIC_HEART_RATE,  // bpm, unused, TODO: Add BLE heart rate support
        METRIC_BODY_TEMP,   // °C, unused, TODO: Add BLE body temperature support
        METRIC_COUNT        // total number of metrics
    };

    struct PageLayout {
        MetricID major;
        MetricID minor1;
        MetricID minor2;
    };

    struct MetricDefinition {
        MetricID id;
        const char* name;
        const char* label;
        const char* unit;
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
        : bl_pin(bl), width(w), height(h) {
        bus = new Arduino_ESP32SPIDMA(dc, cs, sck, mosi, GFX_NOT_DEFINED, spi);
        tft = new Arduino_ST7789(bus, rst, rot, true, w, h);
        canvasMajor = new Arduino_Canvas_Mono(w - 8, 140, tft, 4, 0);
        canvasMinor1 = new Arduino_Canvas_Mono((w - 16) / 2, h - 144, tft, 4, 144);
        canvasMinor2 = new Arduino_Canvas_Mono((w - 16) / 2, h - 144, tft, (w - 16) / 2 + 12, 144);
        canvasMenu = new Arduino_Canvas_Mono(w, h, tft, 0, 0);
        transitionLabelMajor = new Arduino_Canvas_Mono(canvasMajor->width(), canvasMajor->height(), nullptr);
        transitionValueMajor = new Arduino_Canvas_Mono(canvasMajor->width(), canvasMajor->height(), nullptr);
        transitionLabelMinor1 = new Arduino_Canvas_Mono(canvasMinor1->width(), canvasMinor1->height(), nullptr);
        transitionValueMinor1 = new Arduino_Canvas_Mono(canvasMinor1->width(), canvasMinor1->height(), nullptr);
        transitionLabelMinor2 = new Arduino_Canvas_Mono(canvasMinor2->width(), canvasMinor2->height(), nullptr);
        transitionValueMinor2 = new Arduino_Canvas_Mono(canvasMinor2->width(), canvasMinor2->height(), nullptr);
    }

    void setup() override;
    void splash() override;
    void update() override;
    void clear() override;
    void fillScreen(uint16_t color) override;
    void setRotation(uint8_t rotation) override;
    void setBrightnessPercent(uint8_t p) override;
    bool hasBacklight() override { return bl_pin >= 0; }

    void nextPage() override;
    uint8_t currentPage() override;

    bool showMenu(const MenuSnapshot& menu);
    void exitMenu();

   protected:
    const char* tag = "ST7789_240x240";
    int8_t bl_pin;    // backlight pin, -1 if not used
    uint16_t width;   // display width
    uint16_t height;  // display height

    Arduino_DataBus* bus = nullptr;
    Arduino_GFX* tft = nullptr;

    Arduino_Canvas_Mono* canvas = nullptr;
    Arduino_Canvas_Mono* canvasMajor = nullptr;
    Arduino_Canvas_Mono* canvasMinor1 = nullptr;
    Arduino_Canvas_Mono* canvasMinor2 = nullptr;
    Arduino_Canvas_Mono* canvasMenu = nullptr;
    Arduino_Canvas_Mono* transitionLabelMajor = nullptr;
    Arduino_Canvas_Mono* transitionValueMajor = nullptr;
    Arduino_Canvas_Mono* transitionLabelMinor1 = nullptr;
    Arduino_Canvas_Mono* transitionValueMinor1 = nullptr;
    Arduino_Canvas_Mono* transitionLabelMinor2 = nullptr;
    Arduino_Canvas_Mono* transitionValueMinor2 = nullptr;

    const uint8_t* largeFont = u8g2_font_RobotoMono_Bold_90px_digits;
    const uint8_t* mediumFont = u8g2_font_RobotoMono_Bold_48px_caps_digits;
    const uint8_t* labelFont = u8g2_font_RobotoMono_Bold_48px_caps_digits;
    const uint8_t* menuFont = u8g2_font_RobotoMono_Bold_24px_alpha_digits;

    static constexpr uint32_t PAGE_UPDATE_MS = 100;                 // normal update rate
    static constexpr uint32_t PAGE_TRANSITION_MS = 2000;        // total page transition time
    static constexpr uint32_t PAGE_TRANSITION_STATIC_MS = 500;  // static hold time before blend begins
    static constexpr uint32_t PAGE_TRANSITION_UPDATE_MS = 40;   // update rate during transition
    static constexpr uint32_t PAS_FEEDBACK_FADE_MS = PAGE_TRANSITION_MS - PAGE_TRANSITION_STATIC_MS;
    static constexpr uint8_t SLOT_COUNT = 3;                    // number of canvases to draw on each page
    static constexpr uint8_t PAGE_COUNT = 3;                    // number of pages

    enum DisplayMode {
        MODE_SPLASH,
        MODE_PAGE,
        MODE_PAGE_TRANSITION,
        MODE_PAS_FEEDBACK,
        MODE_MENU,
    };

    enum SlotIndex {
        SLOT_MAJOR = 0,
        SLOT_MINOR1 = 1,
        SLOT_MINOR2 = 2,
    };

    struct MetricSlot {
        Arduino_Canvas_Mono* canvas;
        Arduino_Canvas_Mono* labelCanvas;
        Arduino_Canvas_Mono* valueCanvas;
        uint8_t valueTextSize;
        uint8_t labelTextSize;
        uint8_t valueVerticalOffset;
        uint8_t labelVerticalOffset;
    };

    struct RenderedMetric {
        char value[6] = {};
        bool valid = false;
    };

    inline static constexpr MetricDefinition metricDefinitions[METRIC_COUNT] = {
        {METRIC_SPEED, "Speed", "SPD", "km/h"},
        {METRIC_CADENCE, "Cadence", "CAD", "RPM"},
        {METRIC_PAS, "Assist", "PAS", ""},
        {METRIC_MOTOR_PWR, "Motor Power", "MOW", "W"},
        {METRIC_HUMAN_PWR, "Human Power", "HUW", "W"},
        {METRIC_VOLTAGE, "Voltage", "VOL", "V"},
        {METRIC_SOC, "State of Charge", "SOC", "%"},
        {METRIC_MOTOR_TEMP, "Motor Temp", "TMP", "C"},
        {METRIC_TRIP, "Trip", "TRP", "km"},
        {METRIC_ODO, "Odometer", "ODO", "km"},
        {METRIC_RANGE, "Range", "RNG", "km"},
        {METRIC_HEART_RATE, "Heart Rate", "HRT", "bpm"},
        {METRIC_BODY_TEMP, "Body Temp", "BDY", "C"},
    };

    inline static constexpr PageLayout defaultPages[PAGE_COUNT] = {
        {METRIC_SPEED, METRIC_MOTOR_PWR, METRIC_HUMAN_PWR},
        {METRIC_PAS, METRIC_MOTOR_TEMP, METRIC_CADENCE},
        {METRIC_HEART_RATE, METRIC_SOC, METRIC_RANGE},
    };

    DisplayMode _displayMode = MODE_SPLASH;
    uint8_t _currentPage = 0;
    uint32_t _transitionStart = 0;
    uint32_t _lastPageUpdate = 0;
    uint32_t _lastTransitionUpdate = 0;
    uint32_t _pasFeedbackStart = 0;
    uint32_t _lastPasFeedbackUpdate = 0;
    int8_t _lastCanPasLevel = 0;
    bool _pasFeedbackInitialized = false;
    RenderedMetric _renderedMetrics[SLOT_COUNT];

    void startPageTransition(uint8_t page);
    void finishPageTransition();
    void drawTransitionFrame(uint32_t now);
    void drawTransitionSlot(uint8_t slotIndex, MetricID id, State::Snapshot& s, uint8_t blendStep);
    bool startPasFeedbackIfNeeded(uint32_t now, State::Snapshot& s);
    void startPasFeedback(State::Snapshot& s, uint32_t now);
    void finishPasFeedback(State::Snapshot& s);
    void drawPasFeedbackFrame(uint32_t now, State::Snapshot& s);
    void drawPasFeedbackValue(State::Snapshot& s);
    void drawPasFeedbackBlend(State::Snapshot& s, uint8_t blendStep);
    void clearMetricSlots();
    void invalidateRenderedMetrics();
    const PageLayout& currentPageLayout() const;
    MetricSlot metricSlot(uint8_t slotIndex) const;
    MetricID pageMetric(uint8_t slotIndex) const;
    bool pageIncludesMetric(MetricID id) const;
    const MetricDefinition* metricDefinition(MetricID id) const;
    void drawPageLabels();
    void drawPageValues(State::Snapshot& s, bool force, bool remember = true);
    void drawMetricValue(uint8_t slotIndex, MetricID id, State::Snapshot& s, bool force, bool remember);
    const uint8_t* selectValueFont(const char* value, bool isNumeric) const;
    void drawSlotText(MetricSlot& slot, const char* text, const uint8_t* font, uint8_t textSize, uint8_t verticalOffset, bool invertColors = false);
    void renderTextToCanvas(Arduino_Canvas_Mono* canvas, const char* text, const uint8_t* font, uint8_t textSize, uint8_t verticalOffset, bool invertColors = false);
    void blendMonoCanvases(Arduino_Canvas_Mono* fromCanvas, Arduino_Canvas_Mono* toCanvas, Arduino_Canvas_Mono* outputCanvas, uint8_t blendStep);
    uint8_t bayerThreshold(uint16_t x, uint16_t y) const;
    bool formatMetricValue(MetricID id, State::Snapshot& s, char* buffer, size_t bufferSize, bool* isNumeric);
    bool formatPasValue(int8_t pas, char* buffer, size_t bufferSize, bool* isNumeric);
    uint16_t roundedMetricValue(float value) const;
    uint16_t cappedMetricValue(uint32_t value) const;
    void formatUInt(char* buffer, size_t bufferSize, uint16_t value);
    void drawMenu(const MenuSnapshot& menu);
    void drawMenuLine(const char* text, int16_t baseline, bool selected);
};

#endif  // ST7789_240x240_H
