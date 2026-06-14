/* ST7789 240x240 Display Driver */

#ifndef ST7789_240x240_H
#define ST7789_240x240_H

#include <Arduino_GFX_Library.h>
#include <cstring>
#include <functional>

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
#include "RobotoMono_bold30pt7b_caps.h"

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
            !canvasMinor2->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !canvasMenu->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !transitionLabelMajor->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !transitionValueMajor->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !transitionLabelMinor1->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !transitionValueMinor1->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !transitionLabelMinor2->begin(GFX_SKIP_OUTPUT_BEGIN) ||
            !transitionValueMinor2->begin(GFX_SKIP_OUTPUT_BEGIN)) {
            while (true) {
                ESP_LOGE(tag, "canvas begin() failed");
                delay(1000);
            }
        }

        canvasMajor->setTextColor(WHITE, BLACK);
        canvasMinor1->setTextColor(WHITE, BLACK);
        canvasMinor2->setTextColor(WHITE, BLACK);
        canvasMenu->setTextColor(WHITE, BLACK);
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

        if (keyPowerClick) {
            keyPowerClick = false;
            handlePowerClick();
        }

        if (displayMode == MODE_SPLASH) {
            startPageTransition(currentPage);
            return;
        }

        if (displayMode == MODE_MENU) {
            if (menuDirty) drawMenu();
            return;
        }

        if (displayMode == MODE_PAGE_TRANSITION) {
            if (t - lastTransitionUpdate < PAGE_TRANSITION_UPDATE_MS) return;
            lastTransitionUpdate = t;
            drawTransitionFrame(t);
            return;
        }

        if (t - lastPageUpdate < PAGE_UPDATE_MS) return;
        lastPageUpdate = t;

        State::Snapshot s = state.getSnapshot();
        drawPageValues(s, false);

        // ESP_LOGD(taskName(), "Update took %d ms", millis() - t);
    }

    void clear() override {
        fillScreen(BLACK);
    }

    bool menuActive() const {
        return displayMode == MODE_MENU;
    }

    void handlePowerClick() {
        if (displayMode == MODE_MENU) {
            selectMenuItem();
            return;
        }
        nextPage();
    }

    bool menuPrevious() {
        if (!menuActive()) return false;
        selectedMenuItem = selectedMenuItem == 0 ? MENU_ITEM_COUNT - 1 : selectedMenuItem - 1;
        menuDirty = true;
        return true;
    }

    bool menuNext() {
        if (!menuActive()) return false;
        selectedMenuItem = (selectedMenuItem + 1) % MENU_ITEM_COUNT;
        menuDirty = true;
        return true;
    }

    void enterMenu() {
        if (displayMode == MODE_MENU) return;
        displayMode = MODE_MENU;
        selectedMenuItem = 0;
        menuDirty = true;
        ESP_LOGI(tag, "Entering display menu");
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

    void nextPage() {
        uint8_t next = (currentPage + 1) % PAGE_COUNT;
        startPageTransition(next);
    }

    void startPageTransition(uint8_t page) {
        if (page >= PAGE_COUNT) {
            ESP_LOGW(tag, "Ignoring invalid page index: %u", page);
            return;
        }

        currentPage = page;
        displayMode = MODE_PAGE_TRANSITION;
        transitionStart = millis();
        lastTransitionUpdate = 0;
        invalidateRenderedMetrics();
        clearMetricSlots();
        drawPageLabels();
        ESP_LOGD(tag, "Switched to page %u", currentPage);
    }

    void finishPageTransition() {
        displayMode = MODE_PAGE;
        invalidateRenderedMetrics();
        State::Snapshot s = state.getSnapshot();
        drawPageValues(s, true);
        lastPageUpdate = millis();
    }

    void drawTransitionFrame(uint32_t now) {
        uint32_t elapsed = now - transitionStart;
        if (elapsed >= PAGE_TRANSITION_MS) {
            finishPageTransition();
            return;
        }

        State::Snapshot s = state.getSnapshot();
        uint8_t blendStep = (uint8_t)((elapsed * 17U) / PAGE_TRANSITION_MS);
        if (blendStep > 16) blendStep = 16;

        for (uint8_t i = 0; i < SLOT_COUNT; i++) {
            drawTransitionSlot(i, pageMetric(i), s, blendStep);
        }
    }

    void drawTransitionSlot(uint8_t slotIndex, MetricID id, State::Snapshot& s, uint8_t blendStep) {
        MetricSlot slot = metricSlot(slotIndex);
        const MetricDefinition* metric = metricDefinition(id);
        if (metric == nullptr || slot.canvas == nullptr || slot.labelCanvas == nullptr || slot.valueCanvas == nullptr) return;

        char value[sizeof(renderedMetrics[slotIndex].value)] = {};
        bool isNumeric = true;
        if (!formatMetricValue(id, s, value, sizeof(value), &isNumeric)) return;

        const GFXfont* valueFont = selectValueFont(value, isNumeric);
        uint8_t valueTextSize = isNumeric ? slot.valueTextSize : slot.labelTextSize;
        uint8_t valueVerticalOffset = isNumeric ? slot.valueVerticalOffset : slot.labelVerticalOffset;

        renderTextToCanvas(slot.labelCanvas, metric->label, labelFont, slot.labelTextSize, slot.labelVerticalOffset);
        renderTextToCanvas(slot.valueCanvas, value, valueFont, valueTextSize, valueVerticalOffset);
        blendMonoCanvases(slot.labelCanvas, slot.valueCanvas, slot.canvas, blendStep);
        slot.canvas->flush();
    }

    void clearMetricSlots() {
        canvasMajor->fillScreen(BLACK);
        canvasMinor1->fillScreen(BLACK);
        canvasMinor2->fillScreen(BLACK);
        canvasMajor->flush();
        canvasMinor1->flush();
        canvasMinor2->flush();
    }

    void invalidateRenderedMetrics() {
        for (uint8_t i = 0; i < SLOT_COUNT; i++) {
            renderedMetrics[i].value[0] = '\0';
            renderedMetrics[i].valid = false;
        }
    }

    const PageLayout& currentPageLayout() const {
        return defaultPages[currentPage];
    }

    MetricSlot metricSlot(uint8_t slotIndex) const {
        switch (slotIndex) {
            case SLOT_MAJOR:
                return {canvasMajor, transitionLabelMajor, transitionValueMajor, 2, 2, 5, 8};
            case SLOT_MINOR1:
                return {canvasMinor1, transitionLabelMinor1, transitionValueMinor1, 1, 1, 2, 2};
            case SLOT_MINOR2:
                return {canvasMinor2, transitionLabelMinor2, transitionValueMinor2, 1, 1, 2, 2};
            default:
                return {nullptr, nullptr, nullptr, 1, 1, 0, 0};
        }
    }

    MetricID pageMetric(uint8_t slotIndex) const {
        const PageLayout& layout = currentPageLayout();
        switch (slotIndex) {
            case SLOT_MAJOR:
                return layout.major;
            case SLOT_MINOR1:
                return layout.minor1;
            case SLOT_MINOR2:
                return layout.minor2;
            default:
                return METRIC_COUNT;
        }
    }

    const MetricDefinition* metricDefinition(MetricID id) const {
        if (id < 0 || id >= METRIC_COUNT) return nullptr;
        return &metricDefinitions[id];
    }

    void drawPageLabels() {
        for (uint8_t i = 0; i < SLOT_COUNT; i++) {
            const MetricDefinition* metric = metricDefinition(pageMetric(i));
            MetricSlot slot = metricSlot(i);
            if (metric == nullptr || slot.canvas == nullptr) continue;
            drawSlotText(slot, metric->label, labelFont, slot.labelTextSize, slot.labelVerticalOffset);
        }
    }

    void drawPageValues(State::Snapshot& s, bool force, bool remember = true) {
        for (uint8_t i = 0; i < SLOT_COUNT; i++) {
            drawMetricValue(i, pageMetric(i), s, force, remember);
        }
    }

    void drawMetricValue(uint8_t slotIndex, MetricID id, State::Snapshot& s, bool force, bool remember) {
        MetricSlot slot = metricSlot(slotIndex);
        if (slot.canvas == nullptr) return;

        char value[sizeof(renderedMetrics[slotIndex].value)] = {};
        bool isNumeric = true;
        if (!formatMetricValue(id, s, value, sizeof(value), &isNumeric)) return;

        RenderedMetric& rendered = renderedMetrics[slotIndex];
        if (remember && !force && rendered.valid && strcmp(rendered.value, value) == 0) return;

        const GFXfont* font = selectValueFont(value, isNumeric);
        uint8_t textSize = isNumeric ? slot.valueTextSize : slot.labelTextSize;
        uint8_t verticalOffset = isNumeric ? slot.valueVerticalOffset : slot.labelVerticalOffset;
        drawSlotText(slot, value, font, textSize, verticalOffset);

        if (!remember) return;
        strncpy(rendered.value, value, sizeof(rendered.value) - 1);
        rendered.value[sizeof(rendered.value) - 1] = '\0';
        rendered.valid = true;
    }

    const GFXfont* selectValueFont(const char* value, bool isNumeric) const {
        if (!isNumeric) return labelFont;
        return strlen(value) >= 3 ? mediumFont : largeFont;
    }

    void drawSlotText(
        MetricSlot& slot,
        const char* text,
        const GFXfont* font,
        uint8_t textSize,
        uint8_t verticalOffset,
        bool invertColors = false) {
        if (slot.canvas == nullptr || text == nullptr || font == nullptr) return;

        renderTextToCanvas(slot.canvas, text, font, textSize, verticalOffset, invertColors);
        slot.canvas->flush();
    }

    void renderTextToCanvas(
        Arduino_Canvas_Mono* canvas,
        const char* text,
        const GFXfont* font,
        uint8_t textSize,
        uint8_t verticalOffset,
        bool invertColors = false) {
        if (canvas == nullptr || text == nullptr || font == nullptr) return;

        uint16_t textColor = invertColors ? BLACK : WHITE;
        uint16_t backgroundColor = invertColors ? WHITE : BLACK;
        canvas->setFont(font);
        canvas->setTextSize(textSize);
        canvas->setTextColor(textColor, backgroundColor);
        canvas->fillRect(0, 0, canvas->width(), canvas->height(), backgroundColor);

        int16_t x1 = 0;
        int16_t y1 = 0;
        uint16_t textWidth = 0;
        uint16_t textHeight = 0;
        canvas->getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);

        int16_t cursorX = -x1;
        if (textWidth < canvas->width()) {
            cursorX = (canvas->width() - textWidth) / 2 - x1;
        }

        canvas->setCursor(cursorX, canvas->height() - verticalOffset);
        canvas->print(text);
    }

    void blendMonoCanvases(
        Arduino_Canvas_Mono* fromCanvas,
        Arduino_Canvas_Mono* toCanvas,
        Arduino_Canvas_Mono* outputCanvas,
        uint8_t blendStep) {
        if (fromCanvas == nullptr || toCanvas == nullptr || outputCanvas == nullptr) return;

        uint8_t* from = fromCanvas->getFramebuffer();
        uint8_t* to = toCanvas->getFramebuffer();
        uint8_t* output = outputCanvas->getFramebuffer();
        if (from == nullptr || to == nullptr || output == nullptr) return;

        uint16_t width = outputCanvas->width();
        uint16_t height = outputCanvas->height();
        uint16_t rowBytes = (width + 7) / 8;

        for (uint16_t y = 0; y < height; y++) {
            for (uint16_t byteX = 0; byteX < rowBytes; byteX++) {
                uint16_t index = y * rowBytes + byteX;
                uint8_t fromByte = from[index];
                uint8_t toByte = to[index];
                uint8_t outByte = 0;

                for (uint8_t bit = 0; bit < 8; bit++) {
                    uint16_t x = byteX * 8 + bit;
                    if (x >= width) break;

                    uint8_t mask = 0x80 >> bit;
                    bool fromPixel = (fromByte & mask) != 0;
                    bool toPixel = (toByte & mask) != 0;
                    bool outPixel = fromPixel;

                    if (fromPixel != toPixel) {
                        outPixel = bayerThreshold(x, y) < blendStep ? toPixel : fromPixel;
                    }

                    if (outPixel) outByte |= mask;
                }

                output[index] = outByte;
            }
        }
    }

    uint8_t bayerThreshold(uint16_t x, uint16_t y) const {
        static constexpr uint8_t bayer4x4[4][4] = {
            {0, 8, 2, 10},
            {12, 4, 14, 6},
            {3, 11, 1, 9},
            {15, 7, 13, 5},
        };
        return bayer4x4[y & 3][x & 3];
    }

    bool formatMetricValue(MetricID id, State::Snapshot& s, char* buffer, size_t bufferSize, bool* isNumeric) {
        if (buffer == nullptr || bufferSize == 0 || isNumeric == nullptr) {
            ESP_LOGE(tag, "Invalid metric format buffer");
            return false;
        }

        *isNumeric = true;
        switch (id) {
            case METRIC_SPEED:
                formatUInt(buffer, bufferSize, roundedMetricValue(s.speed()));
                return true;
            case METRIC_CADENCE:
                formatUInt(buffer, bufferSize, s.cadence);
                return true;
            case METRIC_PAS:
                return formatPasValue(s.pasLevelRequested, buffer, bufferSize, isNumeric);
            case METRIC_MOTOR_PWR:
                formatUInt(buffer, bufferSize, roundedMetricValue(s.motorPower()));
                return true;
            case METRIC_HUMAN_PWR:
                formatUInt(buffer, bufferSize, roundedMetricValue(s.humanPower()));
                return true;
            case METRIC_VOLTAGE:
                formatUInt(buffer, bufferSize, roundedMetricValue((float)s.batteryVoltage_x100 / 100.0f));
                return true;
            case METRIC_SOC:
                formatUInt(buffer, bufferSize, 0);
                return true;
            case METRIC_MOTOR_TEMP:
                formatUInt(buffer, bufferSize, s.motorTemp);
                return true;
            case METRIC_TRIP:
                formatUInt(buffer, bufferSize, cappedMetricValue(s.trip_mx10 / 10000));
                return true;
            case METRIC_ODO:
                formatUInt(buffer, bufferSize, cappedMetricValue(s.odo_mx10 / 10000));
                return true;
            case METRIC_RANGE:
            case METRIC_HEART_RATE:
            case METRIC_BODY_TEMP:
                formatUInt(buffer, bufferSize, 0);
                return true;
            default:
                ESP_LOGW(tag, "Ignoring invalid metric id: %d", id);
                return false;
        }
    }

    bool formatPasValue(int8_t pas, char* buffer, size_t bufferSize, bool* isNumeric) {
        if (pas < 0) {
            *isNumeric = false;
            snprintf(buffer, bufferSize, "WLK");
            return true;
        }
        if (pas == 0) {
            *isNumeric = false;
            snprintf(buffer, bufferSize, "OFF");
            return true;
        }
        formatUInt(buffer, bufferSize, cappedMetricValue((uint32_t)pas));
        return true;
    }

    uint16_t roundedMetricValue(float value) const {
        if (value <= 0.0f) return 0;
        if (value >= 999.0f) return 999;
        return (uint16_t)(value + 0.5f);
    }

    uint16_t cappedMetricValue(uint32_t value) const {
        return value > 999 ? 999 : (uint16_t)value;
    }

    void formatUInt(char* buffer, size_t bufferSize, uint16_t value) {
        snprintf(buffer, bufferSize, "%u", cappedMetricValue(value));
    }

    void drawMenu() {
        canvasMenu->setFont(labelFont);
        canvasMenu->setTextSize(1);
        canvasMenu->setTextColor(WHITE, BLACK);
        canvasMenu->fillScreen(BLACK);

        drawMenuLine("MENU", 52, false);
        for (uint8_t i = 0; i < MENU_ITEM_COUNT; i++) {
            drawMenuLine(menuItems[i], 118 + (i * 54), i == selectedMenuItem);
        }

        canvasMenu->flush();
        menuDirty = false;
    }

    void drawMenuLine(const char* text, int16_t baseline, bool selected) {
        if (text == nullptr) return;

        uint16_t textColor = selected ? BLACK : WHITE;
        uint16_t backgroundColor = selected ? WHITE : BLACK;
        if (selected) {
            canvasMenu->fillRect(0, baseline - 42, canvasMenu->width(), 50, WHITE);
        }

        canvasMenu->setTextColor(textColor, backgroundColor);
        int16_t x1 = 0;
        int16_t y1 = 0;
        uint16_t textWidth = 0;
        uint16_t textHeight = 0;
        canvasMenu->getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
        int16_t cursorX = (canvasMenu->width() - textWidth) / 2 - x1;
        canvasMenu->setCursor(cursorX, baseline);
        canvasMenu->print(text);
    }

    void selectMenuItem() {
        switch (selectedMenuItem) {
            case 0:
                clear();
                nextPage();
                return;
            case 1:
            default:
                clear();
                startPageTransition(currentPage);
                return;
        }
    }
};

#endif  // ST7789_240x240_H
