/* ST7789 240x240 Display Driver - Implementation */

#include "st7789_240x240.h"

#include <Arduino.h>

extern State state;

void ST7789_240x240::setup() {
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

void ST7789_240x240::splash() {
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

void ST7789_240x240::update() {
    ulong t = millis();
    if (t < 2000) return;  // show splash for at least 2 seconds

    if (selectClicked) {
        selectClicked = false;
        handleSelectClick();
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

void ST7789_240x240::clear() {
    fillScreen(BLACK);
}

void ST7789_240x240::setRotation(uint8_t rotation) {
    tft->setRotation(rotation);
}

void ST7789_240x240::fillScreen(uint16_t color) {
    tft->fillScreen(color);
}

// exponential curve biased towards the low end
void ST7789_240x240::setBrightnessPercent(uint8_t p) {
    if (!hasBacklight()) return;
    if (p == 0) {
        analogWrite((uint8_t)bl, 0);
        return;
    }

    // tunable gamma-squared curve
    constexpr float GAMMA = 3.0f;
    uint8_t val = (uint8_t)(254.0f * powf(p / 100.0f, GAMMA)) + 1;

    ESP_LOGD(tag, "setBrightnessPercent(%d) -> %d", p, val);
    analogWrite((uint8_t)bl, val);
}

void ST7789_240x240::handleSelectClick() {
    if (displayMode == MODE_MENU) {
        selectMenuItem();
        return;
    }
    nextPage();
}

// returns true if the menu is active and the item was changed
bool ST7789_240x240::menuPrevious() {
    if (!menuActive()) return false;
    selectedMenuItem = selectedMenuItem == 0 ? MENU_ITEM_COUNT - 1 : selectedMenuItem - 1;
    menuDirty = true;
    return true;
}

// returns true if the menu is active and the item was changed
bool ST7789_240x240::menuNext() {
    if (!menuActive()) return false;
    selectedMenuItem = (selectedMenuItem + 1) % MENU_ITEM_COUNT;
    menuDirty = true;
    return true;
}

void ST7789_240x240::enterMenu() {
    if (displayMode == MODE_MENU) return;
    displayMode = MODE_MENU;
    selectedMenuItem = 0;
    menuDirty = true;
    ESP_LOGI(tag, "Entering display menu");
}

void ST7789_240x240::nextPage() {
    uint8_t next = (currentPage + 1) % PAGE_COUNT;
    startPageTransition(next);
}

void ST7789_240x240::startPageTransition(uint8_t page) {
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

void ST7789_240x240::finishPageTransition() {
    displayMode = MODE_PAGE;
    invalidateRenderedMetrics();
    State::Snapshot s = state.getSnapshot();
    drawPageValues(s, true);
    lastPageUpdate = millis();
}

void ST7789_240x240::drawTransitionFrame(uint32_t now) {
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

void ST7789_240x240::drawTransitionSlot(uint8_t slotIndex, MetricID id, State::Snapshot& s, uint8_t blendStep) {
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

void ST7789_240x240::clearMetricSlots() {
    canvasMajor->fillScreen(BLACK);
    canvasMinor1->fillScreen(BLACK);
    canvasMinor2->fillScreen(BLACK);
    canvasMajor->flush();
    canvasMinor1->flush();
    canvasMinor2->flush();
}

void ST7789_240x240::invalidateRenderedMetrics() {
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        renderedMetrics[i].value[0] = '\0';
        renderedMetrics[i].valid = false;
    }
}

const ST7789_240x240::PageLayout& ST7789_240x240::currentPageLayout() const {
    return defaultPages[currentPage];
}

ST7789_240x240::MetricSlot ST7789_240x240::metricSlot(uint8_t slotIndex) const {
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

ST7789_240x240::MetricID ST7789_240x240::pageMetric(uint8_t slotIndex) const {
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

const ST7789_240x240::MetricDefinition* ST7789_240x240::metricDefinition(MetricID id) const {
    if (id < 0 || id >= METRIC_COUNT) return nullptr;
    return &metricDefinitions[id];
}

void ST7789_240x240::drawPageLabels() {
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        const MetricDefinition* metric = metricDefinition(pageMetric(i));
        MetricSlot slot = metricSlot(i);
        if (metric == nullptr || slot.canvas == nullptr) continue;
        drawSlotText(slot, metric->label, labelFont, slot.labelTextSize, slot.labelVerticalOffset);
    }
}

void ST7789_240x240::drawPageValues(State::Snapshot& s, bool force, bool remember) {
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        drawMetricValue(i, pageMetric(i), s, force, remember);
    }
}

void ST7789_240x240::drawMetricValue(uint8_t slotIndex, MetricID id, State::Snapshot& s, bool force, bool remember) {
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

const GFXfont* ST7789_240x240::selectValueFont(const char* value, bool isNumeric) const {
    if (!isNumeric) return labelFont;
    return strlen(value) >= 3 ? mediumFont : largeFont;
}

void ST7789_240x240::drawSlotText(
    MetricSlot& slot,
    const char* text,
    const GFXfont* font,
    uint8_t textSize,
    uint8_t verticalOffset,
    bool invertColors) {
    if (slot.canvas == nullptr || text == nullptr || font == nullptr) return;

    renderTextToCanvas(slot.canvas, text, font, textSize, verticalOffset, invertColors);
    slot.canvas->flush();
}

void ST7789_240x240::renderTextToCanvas(
    Arduino_Canvas_Mono* canvas,
    const char* text,
    const GFXfont* font,
    uint8_t textSize,
    uint8_t verticalOffset,
    bool invertColors) {
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

void ST7789_240x240::blendMonoCanvases(
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

uint8_t ST7789_240x240::bayerThreshold(uint16_t x, uint16_t y) const {
    static constexpr uint8_t bayer4x4[4][4] = {
        {0, 8, 2, 10},
        {12, 4, 14, 6},
        {3, 11, 1, 9},
        {15, 7, 13, 5},
    };
    return bayer4x4[y & 3][x & 3];
}

bool ST7789_240x240::formatMetricValue(MetricID id, State::Snapshot& s, char* buffer, size_t bufferSize, bool* isNumeric) {
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

bool ST7789_240x240::formatPasValue(int8_t pas, char* buffer, size_t bufferSize, bool* isNumeric) {
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

uint16_t ST7789_240x240::roundedMetricValue(float value) const {
    if (value <= 0.0f) return 0;
    if (value >= 999.0f) return 999;
    return (uint16_t)(value + 0.5f);
}

uint16_t ST7789_240x240::cappedMetricValue(uint32_t value) const {
    return value > 999 ? 999 : (uint16_t)value;
}

void ST7789_240x240::formatUInt(char* buffer, size_t bufferSize, uint16_t value) {
    snprintf(buffer, bufferSize, "%u", cappedMetricValue(value));
}

void ST7789_240x240::drawMenu() {
    canvasMenu->setFont(labelFont);
    canvasMenu->setTextSize(1);
    canvasMenu->setTextColor(WHITE, BLACK);
    canvasMenu->fillScreen(BLACK);

    static constexpr uint8_t MAX_VISIBLE_MENU_ITEMS = 4;
    static uint8_t scrollOffset = 0;
    if (selectedMenuItem < scrollOffset) {
        scrollOffset = selectedMenuItem;
    } else if (selectedMenuItem >= scrollOffset + MAX_VISIBLE_MENU_ITEMS) {
        scrollOffset = selectedMenuItem - MAX_VISIBLE_MENU_ITEMS + 1;
    }
    for (uint8_t i = scrollOffset; i < MENU_ITEM_COUNT && i < scrollOffset + MAX_VISIBLE_MENU_ITEMS + 1; i++) {
        drawMenuLine(menuItems[i], 52 + (i - scrollOffset) * 54, i == selectedMenuItem);
    }

    canvasMenu->flush();
    menuDirty = false;
}

void ST7789_240x240::drawMenuLine(const char* text, int16_t baseline, bool selected) {
    if (text == nullptr) return;

    uint16_t textColor = selected ? BLACK : WHITE;
    uint16_t backgroundColor = selected ? WHITE : BLACK;

    canvasMenu->fillRect(0, baseline - 46, canvasMenu->width(), 52, backgroundColor);
    canvasMenu->setTextColor(textColor);
    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t textWidth = 0;
    uint16_t textHeight = 0;
    canvasMenu->getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
    int16_t cursorX = (canvasMenu->width() - textWidth) / 2 - x1;
    canvasMenu->setCursor(cursorX, baseline);
    canvasMenu->print(text);
}

void ST7789_240x240::selectMenuItem() {
    switch (selectedMenuItem) {
        case 0:  // DUMMY A
        case 1:  // DUMMY B
        case 2:  // ANOTHER
        case 3:  // MENU ITEM
        case 4:  // SHORT
        case 5:  // DUMMY F
            ESP_LOGI(tag, "Selected menu item: %s", menuItems[selectedMenuItem]);
            return;
        case MENU_ITEM_COUNT - 1:  // Last item: Exit menu
            ESP_LOGI(tag, "Selected menu item: %s", menuItems[selectedMenuItem]);
            clear();
            displayMode = MODE_PAGE;
            startPageTransition(currentPage);
            return;
        default:
            ESP_LOGW(tag, "Selected invalid menu item: %d", selectedMenuItem);
            clear();
            displayMode = MODE_PAGE;
            startPageTransition(currentPage);
            return;
    }
}