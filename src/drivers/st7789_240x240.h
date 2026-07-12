/* ST7789 240x240 Display Driver */

#ifndef ST7789_240x240_H
#define ST7789_240x240_H

#include <Arduino_GFX_Library.h>
#include <cstring>
#include <functional>

#include "display_driver.h"
#include "model/state.h"
#include "ui/menu.h"

extern State state;

#ifndef U8G2_FONT_SECTION
#define U8G2_FONT_SECTION(name) __attribute__((section(".text." name)))
#endif

#include "fonts/u8g2/RobotoMono_Bold_90px_digits.h"
#include "fonts/u8g2/RobotoMono_Bold_62px_caps_digits.h"
#include "fonts/u8g2/RobotoMono_Regular_24px_alpha_digits.h"

class Canvas : public Arduino_Canvas_Mono {
   public:
    Canvas(int16_t w, int16_t h, Arduino_G* output = nullptr, int16_t x = 0, int16_t y = 0, bool verticalByte = false)
        : Arduino_Canvas_Mono(w, h, output, x, y, verticalByte) {}

    int16_t w() const { return _width; }
    int16_t h() const { return _height; }
};

class ST7789_240x240 : public DisplayDriver {
   public:
    enum MetricID {
        METRIC_SPEED,       // km/h
        METRIC_CADENCE,     // RPM
        METRIC_PAS,         // Pedal Assist Level, -1 = Walk assist, 0 = Off, 1-5 = PAS levels
        METRIC_MOTOR_PWR,   // W
        METRIC_HUMAN_PWR,   // W
        METRIC_VOLTAGE,     // V, battery voltage
        METRIC_SOC,         // %, battery State Of Charge
        METRIC_MOTOR_TEMP,  // °C
        METRIC_TRIP,        // km, unused, TODO: Parse CAN trip data, add resettable trip functionality
        METRIC_ODO,         // km, unused, TODO: Parse CAN odometer data
        METRIC_RANGE,       // km
        METRIC_HEART_RATE,  // BPM, unused, TODO: Add BLE heart rate support
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
        uint16_t h = 240);

    void setup() override;
    void splash() override;
    void update() override;
    void clear() override;
    void fillScreen(uint16_t color) override;
    void setRotation(uint8_t rotation) override;
    void setBrightnessPercent(uint8_t p) override;
    bool saveBrightnessPercent();
    bool hasBacklight() override { return bl_pin >= 0; }
    bool setBacklight(uint8_t level) override;
    void onSleep() override;

    void nextPage() override;
    uint8_t currentPage() override;

    bool showMenu(const Menu::Snapshot& menu);
    void exitMenu();

   protected:
    const char* tag = "ST7789_240x240";
    int8_t bl_pin;    // backlight pin, -1 if not used
    uint16_t width;   // display width
    uint16_t height;  // display height

    Arduino_DataBus* bus = nullptr;  // Data bus
    Arduino_GFX* tft = nullptr;      // Display

    Canvas* canvasMajor = nullptr;       // Main metric canvas, top, full width
    Canvas* canvasMinor1 = nullptr;      // Bottom left metric canvas
    Canvas* canvasMinor2 = nullptr;      // Bottom right metric canvas
    Canvas* canvasFullScreen = nullptr;  // Full screen canvas
    Canvas* transitionLabelMajor = nullptr;
    Canvas* transitionValueMajor = nullptr;
    Canvas* transitionLabelMinor1 = nullptr;
    Canvas* transitionValueMinor1 = nullptr;
    Canvas* transitionLabelMinor2 = nullptr;
    Canvas* transitionValueMinor2 = nullptr;

    // Font for main slot, digits only
    const uint8_t* largeFont = u8g2_font_RobotoMono_Bold_90px_digits;

    // Capital letters and digits
    const uint8_t* mediumFont = u8g2_font_RobotoMono_Bold_62px_caps_digits;

    // Capital letters and digits
    const uint8_t* labelFont = mediumFont;

    // Alphanumeric characters
    const uint8_t* smallFont = u8g2_font_RobotoMono_Bold_24px_alpha_digits;

    static constexpr uint32_t PAGE_UPDATE_MS = 100;        // normal update rate
    static constexpr uint32_t TRANSITION_MS = 2000;        // total page transition time
    static constexpr uint32_t TRANSITION_STATIC_MS = 500;  // static hold time before blend begins
    static constexpr uint32_t TRANSITION_UPDATE_MS = 40;   // update rate during transition
    static constexpr uint32_t FEEDBACK_FADE_MS = TRANSITION_MS - TRANSITION_STATIC_MS;
    static constexpr uint8_t SLOT_COUNT = 3;            // number of metric slots
    static constexpr uint8_t PAGE_COUNT = 3;            // number of pages
    static constexpr uint8_t MAX_METRIC_VALUE_LEN = 3;  // max length of a metric value

    enum DisplayMode {
        MODE_SPLASH,
        MODE_PAGE,
        MODE_LABEL_TRANSITION,
        MODE_FEEDBACK,
        MODE_MENU,
    };

    inline String displayModeToStr(DisplayMode m) {
        switch (m) {
            case MODE_SPLASH:
                return "splash";
            case MODE_PAGE:
                return "page";
            case MODE_LABEL_TRANSITION:
                return "label_transition";
            case MODE_FEEDBACK:
                return "feedback";
            case MODE_MENU:
                return "menu";
            default:
                return "unknown";
        }
    }

    enum SlotIndex {
        SLOT_MAJOR = 0,
        SLOT_MINOR1 = 1,
        SLOT_MINOR2 = 2,
    };

    struct MetricSlot {
        Canvas* canvas;
        Canvas* labelCanvas;
        Canvas* valueCanvas;
        uint8_t valueTextSize;
        uint8_t labelTextSize;
        uint8_t valueVerticalOffset;
        uint8_t labelVerticalOffset;
    };

    struct RenderedMetric {
        char value[MAX_METRIC_VALUE_LEN + 1] = {};
        bool valid = false;
    };

    inline static constexpr MetricDefinition metricDefinitions[METRIC_COUNT] = {
        {METRIC_SPEED, "Speed", "SPD", "km/h"},
        {METRIC_CADENCE, "Cadence", "CAD", "RPM"},
        {METRIC_PAS, "Assist Level", "PAS", ""},
        {METRIC_MOTOR_PWR, "Motor Power", "MOW", "W"},
        {METRIC_HUMAN_PWR, "Human Power", "HUW", "W"},
        {METRIC_VOLTAGE, "Voltage", "VOL", "V"},
        {METRIC_SOC, "State of Charge", "SOC", "%"},
        {METRIC_MOTOR_TEMP, "Motor Temp", "TMP", "C"},
        {METRIC_TRIP, "Trip", "TRP", "km"},
        {METRIC_ODO, "Odometer", "ODO", "km"},
        {METRIC_RANGE, "Range", "RNG", "km"},
        {METRIC_HEART_RATE, "Heart Rate", "HRT", "BPM"},
        {METRIC_BODY_TEMP, "Body Temp", "BDY", "C"},
    };

    inline static constexpr PageLayout defaultPages[PAGE_COUNT] = {
        {METRIC_SPEED, METRIC_SOC, METRIC_RANGE},
        {METRIC_HUMAN_PWR, METRIC_CADENCE, METRIC_MOTOR_PWR},
        {METRIC_PAS, METRIC_VOLTAGE, METRIC_MOTOR_TEMP},
    };

    DisplayMode _displayMode = MODE_SPLASH;
    uint8_t _currentPage = 0;  // default to page 0 (first page)
    uint32_t _lastPageUpdate = 0;
    uint32_t _transitionStart = 0;
    uint32_t _lastTransitionUpdate = 0;
    uint32_t _feedbackStart = 0;
    uint32_t _lastPasFeedbackUpdate = 0;
    RenderedMetric _renderedMetrics[SLOT_COUNT];
    int8_t _lastPasLevel = 0;
    bool _pasFeedbackInitialized = false;

    void startPageTransition(uint8_t page);
    void finishPageTransition();
    void drawTransitionFrame(uint32_t now);
    void drawTransitionSlot(uint8_t slotIndex, MetricID id, State::Snapshot& s, uint8_t blendStep);
    bool startPasFeedbackIfNeeded(uint32_t now, State::Snapshot& s);
    void startFeedback(State::Snapshot& s, uint32_t now);
    void finishFeedback(State::Snapshot& s);
    void drawFeedbackFrame(uint32_t now, State::Snapshot& s);
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
    void renderTextToCanvas(Canvas* canvas, const char* text, const uint8_t* font, uint8_t textSize, uint8_t verticalOffset, bool invertColors = false);
    void blendMonoCanvases(Canvas* fromCanvas, Canvas* toCanvas, Canvas* outputCanvas, uint8_t blendStep);
    uint8_t bayerThreshold(uint16_t x, uint16_t y) const;
    bool formatMetricValue(MetricID id, State::Snapshot& s, char* buffer, size_t bufferSize, bool* isNumeric);
    bool formatPasValue(int8_t pas, char* buffer, size_t bufferSize, bool* isNumeric);
    uint16_t roundedMetricValue(float value) const;
    uint16_t cappedMetricValue(uint32_t value, uint16_t cap = 999) const;
    void abbreviatedMetricValue(char* buffer, size_t bufferSize, uint32_t value, bool* isNumeric);
    void formatUInt(char* buffer, size_t bufferSize, uint16_t value);
    void drawMenu(const Menu::Snapshot& menu);
    void drawMenuLine(const char* text, int16_t baseline, bool selected);
};

#endif  // ST7789_240x240_H
