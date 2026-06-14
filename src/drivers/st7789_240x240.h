/* ST7789 240x240 Display Driver */

#ifndef ST7789_240x240_H
#define ST7789_240x240_H

#include <Arduino_GFX_Library.h>
#include <cstring>
#include <functional>

#include "display_driver.h"
#include "model/state.h"

extern State state;

#include "RobotoMono_bold48pt7b_digits.h"
#include "RobotoMono_bold30pt7b_digits.h"
#include "RobotoMono_bold30pt7b_caps.h"

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

    struct MetricDefinition {
        MetricID id;
        const char* name;
        const char* label;
        const char* unit;
    };

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
        canvasMenu = new Arduino_Canvas_Mono(w, h, tft, 0, 0);
        transitionLabelMajor = new Arduino_Canvas_Mono(232, 140, nullptr);
        transitionValueMajor = new Arduino_Canvas_Mono(232, 140, nullptr);
        transitionLabelMinor1 = new Arduino_Canvas_Mono((w - 15) / 2, h - 145, nullptr);
        transitionValueMinor1 = new Arduino_Canvas_Mono((w - 15) / 2, h - 145, nullptr);
        transitionLabelMinor2 = new Arduino_Canvas_Mono((w - 15) / 2, h - 145, nullptr);
        transitionValueMinor2 = new Arduino_Canvas_Mono((w - 15) / 2, h - 145, nullptr);
    }

    void setup() override;
    void splash() override;
    void update() override;
    void clear() override;
    void fillScreen(uint16_t color) override;
    void setRotation(uint8_t rotation) override;
    void setBrightnessPercent(uint8_t p) override;
    bool hasBacklight() override { return bl >= 0; }

    bool menuActive() const { return displayMode == MODE_MENU; }
    void handlePowerClick();
    bool menuPrevious();
    bool menuNext();
    void enterMenu();

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
    Arduino_Canvas_Mono* canvasMenu = nullptr;
    Arduino_Canvas_Mono* transitionLabelMajor = nullptr;
    Arduino_Canvas_Mono* transitionValueMajor = nullptr;
    Arduino_Canvas_Mono* transitionLabelMinor1 = nullptr;
    Arduino_Canvas_Mono* transitionValueMinor1 = nullptr;
    Arduino_Canvas_Mono* transitionLabelMinor2 = nullptr;
    Arduino_Canvas_Mono* transitionValueMinor2 = nullptr;

    const GFXfont* largeFont = &RobotoMono_Bold48pt7b;
    const GFXfont* mediumFont = &RobotoMono_Bold30pt7b;
    const GFXfont* labelFont = &RobotoMono_Bold30pt7b_Caps;

    static constexpr uint32_t PAGE_UPDATE_MS = 100;
    static constexpr uint32_t PAGE_TRANSITION_MS = 1500;
    static constexpr uint32_t PAGE_TRANSITION_UPDATE_MS = 40;
    static constexpr uint8_t SLOT_COUNT = 3;
    static constexpr uint8_t PAGE_COUNT = 3;
    static constexpr uint8_t MENU_ITEM_COUNT = 2;

    enum DisplayMode {
        MODE_SPLASH,
        MODE_PAGE,
        MODE_PAGE_TRANSITION,
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

    inline static constexpr const char* menuItems[MENU_ITEM_COUNT] = {
        "PAGE",
        "EXIT",
    };

    DisplayMode displayMode = MODE_SPLASH;
    uint8_t currentPage = 0;
    uint8_t selectedMenuItem = 0;
    bool menuDirty = false;
    uint32_t transitionStart = 0;
    uint32_t lastPageUpdate = 0;
    uint32_t lastTransitionUpdate = 0;
    RenderedMetric renderedMetrics[SLOT_COUNT];

    void nextPage();
    void startPageTransition(uint8_t page);
    void finishPageTransition();
    void drawTransitionFrame(uint32_t now);
    void drawTransitionSlot(uint8_t slotIndex, MetricID id, State::Snapshot& s, uint8_t blendStep);
    void clearMetricSlots();
    void invalidateRenderedMetrics();
    const PageLayout& currentPageLayout() const;
    MetricSlot metricSlot(uint8_t slotIndex) const;
    MetricID pageMetric(uint8_t slotIndex) const;
    const MetricDefinition* metricDefinition(MetricID id) const;
    void drawPageLabels();
    void drawPageValues(State::Snapshot& s, bool force, bool remember = true);
    void drawMetricValue(uint8_t slotIndex, MetricID id, State::Snapshot& s, bool force, bool remember);
    const GFXfont* selectValueFont(const char* value, bool isNumeric) const;
    void drawSlotText(MetricSlot& slot, const char* text, const GFXfont* font, uint8_t textSize, uint8_t verticalOffset, bool invertColors = false);
    void renderTextToCanvas(Arduino_Canvas_Mono* canvas, const char* text, const GFXfont* font, uint8_t textSize, uint8_t verticalOffset, bool invertColors = false);
    void blendMonoCanvases(Arduino_Canvas_Mono* fromCanvas, Arduino_Canvas_Mono* toCanvas, Arduino_Canvas_Mono* outputCanvas, uint8_t blendStep);
    uint8_t bayerThreshold(uint16_t x, uint16_t y) const;
    bool formatMetricValue(MetricID id, State::Snapshot& s, char* buffer, size_t bufferSize, bool* isNumeric);
    bool formatPasValue(int8_t pas, char* buffer, size_t bufferSize, bool* isNumeric);
    uint16_t roundedMetricValue(float value) const;
    uint16_t cappedMetricValue(uint32_t value) const;
    void formatUInt(char* buffer, size_t bufferSize, uint16_t value);
    void drawMenu();
    void drawMenuLine(const char* text, int16_t baseline, bool selected);
    void selectMenuItem();
};

#endif  // ST7789_240x240_H