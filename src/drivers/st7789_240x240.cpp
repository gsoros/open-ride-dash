/* ST7789 240x240 Display Driver - Implementation */

#include "st7789_240x240.h"

#include <Arduino.h>

#include "config.h"
#include "util.h"

extern State state;

ST7789_240x240::ST7789_240x240(
    int8_t cs,
    int8_t dc,
    int8_t mosi,
    int8_t sck,
    int8_t rst,
    int8_t bl,
    uint8_t spi,
    uint8_t rot,
    uint16_t w,
    uint16_t h)
    : bl_pin(bl), width(w), height(h) {
    //
    // Initialize the data bus
    bus = new Arduino_ESP32SPIDMA(dc, cs, sck, mosi, GFX_NOT_DEFINED, spi);

    // Initialize the display (use module-specific offsets from config.h)
    tft = new Arduino_ST7789(bus, rst, rot, true, w, h,
                             ST7789_COL_OFFSET1, ST7789_ROW_OFFSET1,
                             ST7789_COL_OFFSET2, ST7789_ROW_OFFSET2);

    // Main canvases
    canvasMajor = new Canvas(w - 8, 140, tft, 4, 0);
    canvasMinor1 = new Canvas((w - 16) / 2, h - 144, tft, 4, 144);
    canvasMinor2 = new Canvas((w - 16) / 2, h - 144, tft, (w - 16) / 2 + 12, 144);
    canvasFullScreen = new Canvas(w, h, tft, 0, 0);

    // Transition canvases without output
    transitionLabelMajor = new Canvas(canvasMajor->w(), canvasMajor->h());
    transitionValueMajor = new Canvas(canvasMajor->w(), canvasMajor->h());
    transitionLabelMinor1 = new Canvas(canvasMinor1->w(), canvasMinor1->h());
    transitionValueMinor1 = new Canvas(canvasMinor1->w(), canvasMinor1->h());
    transitionLabelMinor2 = new Canvas(canvasMinor2->w(), canvasMinor2->h());
    transitionValueMinor2 = new Canvas(canvasMinor2->w(), canvasMinor2->h());
}

void ST7789_240x240::setup() {
    if (bl_pin >= 0) {
        // gpio_deep_sleep_hold_dis();
        gpio_hold_dis((gpio_num_t)bl_pin);
        pinMode(bl_pin, OUTPUT);
        digitalWrite(bl_pin, LOW);
    }

    if (!tft->begin()) {
        ESP_LOGE(tag, "begin() failed");
        while (true) delay(1000);
    }

    clear();

    if (!canvasMajor->begin(GFX_SKIP_OUTPUT_BEGIN) ||
        !canvasMinor1->begin(GFX_SKIP_OUTPUT_BEGIN) ||
        !canvasMinor2->begin(GFX_SKIP_OUTPUT_BEGIN) ||
        !canvasFullScreen->begin(GFX_SKIP_OUTPUT_BEGIN) ||
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
    canvasFullScreen->setTextColor(WHITE, BLACK);

    // Wrapping may interfere with getTextBounds()
    canvasMajor->setTextWrap(false);
    canvasMinor1->setTextWrap(false);
    canvasMinor2->setTextWrap(false);
    canvasFullScreen->setTextWrap(false);
}

void ST7789_240x240::splash() {
    static uint32_t f = 0;  // frame

    Canvas* c = canvasFullScreen;
    using i = int16_t;

    if (f == 0) {
        c->setFont(mediumFont);
        c->fillScreen(BLACK);
        c->setTextColor(WHITE);
        c->setTextSize(1);
    }

    const uint8_t animationLength = 10;  // frames
    const i targetY = 100;
    const i centerX = 110;
    const i letterSpacing = 30;
    i oTargetX = centerX - letterSpacing;
    i rTargetX = centerX;
    i dTargetX = centerX + letterSpacing;

    if (f <= animationLength) {
        if (f > 0) {
            i prev_f = f - 1;
            i prev_oX = 0 + (prev_f * (oTargetX) / animationLength);
            i prev_rY = 0 + (prev_f * targetY / animationLength);
            i prev_dX = 200 - (prev_f * (200 - dTargetX) / animationLength);
            c->setTextColor(BLACK);
            c->setCursor(prev_oX, targetY);
            c->print("O");
            c->setCursor(rTargetX, prev_rY);
            c->print("R");
            c->setCursor(prev_dX, targetY);
            c->print("D");
        }
        i oX = 0 + (f * (oTargetX) / animationLength);
        i rY = 0 + (f * targetY / animationLength);
        i dX = 200 - (f * (200 - dTargetX) / animationLength);
        c->setTextColor(WHITE);
        c->setCursor(oX, targetY);
        c->print("O");
        c->setCursor(rTargetX, rY);
        c->print("R");
        c->setCursor(dX, targetY);
        c->print("D");
    } else if (f == animationLength + 1) {
        c->setFont(smallFont);
        c->setTextSize(1);
        c->setTextColor(WHITE);
        c->setCursor(centerX - 70, targetY + 60);
        c->print("OpenRideDash");
    }

    if (f <= animationLength + 1) {
        c->flush();
        f++;
    }
}

void ST7789_240x240::update() {
    ulong t = millis();

    static DisplayMode lastDisplayMode = _displayMode;
    if (lastDisplayMode != _displayMode) {
        lastDisplayMode = _displayMode;
        ESP_LOGD(tag, "Display mode: %s", displayModeToStr(_displayMode).c_str());
    }

    if (_displayMode == MODE_SPLASH) {
        if (t < 4000) {  // show splash for ~4 seconds
            splash();
            return;
        }
        clear();
        startPageTransition(_currentPage);
        return;
    }

    if (_displayMode == MODE_PASSKEY) {
        // TODO: Timeout and exit passkey mode
        return;
    }

    if (_displayMode == MODE_WIFI_AP) {
        return;
    }

    if (_displayMode == MODE_MENU) {
        return;
    }

    if (_displayMode == MODE_LABEL_TRANSITION) {
        if (t - _lastTransitionUpdate < TRANSITION_UPDATE_MS) return;
        _lastTransitionUpdate = t;
        drawTransitionFrame(t);
        return;
    }

    State::Snapshot s = state.getSnapshot();

    if (_displayMode == MODE_FEEDBACK) {
        if (startPasFeedbackIfNeeded(t, s)) {
            ESP_LOGW(tag, "Starting feedback in MODE_FEEDBACK");
            return;
        }
        if (t - _lastPasFeedbackUpdate < TRANSITION_UPDATE_MS) return;
        _lastPasFeedbackUpdate = t;
        drawFeedbackFrame(t, s);
        return;
    }

    if (_displayMode != MODE_PAGE) {
        ESP_LOGW(tag, "Expected _displayMode == %u (MODE_PAGE), got %u", MODE_PAGE, _displayMode);
    }

    if (startPasFeedbackIfNeeded(t, s)) {
        ESP_LOGD(tag, "Starting PAS feedback");
        return;
    }

    if (t - _lastPageUpdate < PAGE_UPDATE_MS) return;
    _lastPageUpdate = t;

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
        analogWrite((uint8_t)bl_pin, 0);
        return;
    }

    // tunable gamma-squared curve
    constexpr float GAMMA = 3.0f;
    uint8_t val = (uint8_t)(254.0f * powf(p / 100.0f, GAMMA)) + 1;

    ESP_LOGD(tag, "setBrightnessPercent(%d) -> %d", p, val);
    setBacklight(val);
}

bool ST7789_240x240::setBacklight(uint8_t level) {
    if (!hasBacklight()) return false;
    analogWrite((uint8_t)bl_pin, level);
    return true;
}

void ST7789_240x240::onSleep() {
    ESP_LOGD(tag, "onSleep()");
    canvasFullScreen->fillScreen(BLACK);
    renderTextToCanvas(canvasFullScreen, "SLEEPING", labelFont, 1, 0);
    canvasFullScreen->flush();
    setBrightnessPercent(0);

    // Switch pin from PWM mode to GPIO mode and hold it LOW during deep sleep.
    // Although setBrightnessPercent(0) sets PWM to 0%, the PWM peripheral remains active.
    // We must explicitly disable PWM by setting GPIO_MODE_OUTPUT, then drive the pin LOW,
    // and enable hold. This ensures the pin stays LOW (backlight off) throughout sleep,
    // even if there's an external pull-up resistor on the pin.
    gpio_set_direction((gpio_num_t)bl_pin, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)bl_pin, 0);
    gpio_hold_en((gpio_num_t)bl_pin);
    gpio_deep_sleep_hold_en();
}

bool ST7789_240x240::showPasskey(uint32_t passkey) {
    if (passkey == 0) {
        ESP_LOGE(tag, "Invalid passkey");
        return false;
    }
    _displayMode = MODE_PASSKEY;
    char buf[32] = {};
    snprintf(buf, sizeof(buf), "%06u", passkey);
    renderTextToCanvas(canvasFullScreen, buf, labelFont, 1, 20);
    canvasFullScreen->flush();
    return true;
}

void ST7789_240x240::exitPasskey() {
    canvasFullScreen->fillScreen(BLACK);
    canvasFullScreen->flush();
    startPageTransition(_currentPage);
}

bool ST7789_240x240::showApSsid(const char* ssid) {
    if (ssid == nullptr || strlen(ssid) == 0) {
        ESP_LOGE(tag, "Invalid AP SSID");
        return false;
    }
    _displayMode = MODE_WIFI_AP;
    canvasFullScreen->fillScreen(BLACK);
    canvasFullScreen->setFont(smallFont);
    canvasFullScreen->setTextColor(WHITE);
    canvasFullScreen->setTextSize(1);
    // Draw "AP Active" label near the top
    canvasFullScreen->setCursor(10, 20);
    canvasFullScreen->print("AP Active");
    // Draw the SSID centered vertically
    int16_t x1, y1;
    uint16_t w, h;
    canvasFullScreen->getTextBounds(ssid, 0, 0, &x1, &y1, &w, &h);
    int16_t cx = (canvasFullScreen->w() - w) / 2;
    int16_t cy = (canvasFullScreen->h() - h) / 2;
    // canvasFullScreen->setFont(labelFont); //too large without wrapping
    canvasFullScreen->setCursor(cx, cy);
    canvasFullScreen->print(ssid);
    canvasFullScreen->flush();
    return true;
}

void ST7789_240x240::exitApSsid() {
    canvasFullScreen->fillScreen(BLACK);
    canvasFullScreen->flush();
    startPageTransition(_currentPage);
}

bool ST7789_240x240::showMenu(const Menu::Snapshot& menu) {
    if (!menu.active || menu.itemCount == 0) {
        static uint32_t lastWarningTime = 0;
        if (!lastWarningTime || millis() - lastWarningTime > 5000) {
            if (!menu.active) ESP_LOGE(tag, "Menu snapshot is not active");
            if (menu.itemCount == 0) ESP_LOGE(tag, "Menu snapshot itemCount is zero");
            lastWarningTime = millis();
        }
        return false;
    }

    bool enteringMenu = _displayMode != MODE_MENU;
    if (enteringMenu) _displayModeBeforeMenu = _displayMode;
    _displayMode = MODE_MENU;
    if (!enteringMenu && !menu.dirty) return false;

    drawMenu(menu);
    return true;
}

void ST7789_240x240::exitMenu() {
    if (_displayMode != MODE_MENU) return;
    canvasFullScreen->fillScreen(BLACK);
    canvasFullScreen->flush();
    // Restore AP mode if it was active before entering the menu
    if (_displayModeBeforeMenu == MODE_WIFI_AP) {
        _displayMode = MODE_WIFI_AP;
        return;
    }
    startPageTransition(_currentPage);
}

void ST7789_240x240::nextPage() {
    uint8_t next = (_currentPage + 1) % PAGE_COUNT;
    startPageTransition(next);
}

uint8_t ST7789_240x240::currentPage() {
    return _currentPage;
}

/*
    Starts a page transition to the given page by first clearing the metric slots, then drawing the labels and starting the crossfade to the values.
    page: index of the page to transition to
*/
void ST7789_240x240::startPageTransition(uint8_t page) {
    if (page >= PAGE_COUNT) {
        ESP_LOGW(tag, "Ignoring invalid page index: %u", page);
        return;
    }

    _currentPage = page;
    _displayMode = MODE_LABEL_TRANSITION;
    _transitionStart = millis();
    _lastTransitionUpdate = 0;
    _lastPasFeedbackUpdate = 0;
    invalidateRenderedMetrics();
    clearMetricSlots();
    drawPageLabels();
    ESP_LOGD(tag, "Transitioning to page %u", _currentPage);
}

void ST7789_240x240::finishPageTransition() {
    _displayMode = MODE_PAGE;
    invalidateRenderedMetrics();
    State::Snapshot s = state.getSnapshot();
    drawPageValues(s, true);
    _lastPageUpdate = millis();
    ESP_LOGD(tag, "Finished transitioning to page %u", _currentPage);
}

void ST7789_240x240::drawTransitionFrame(uint32_t now) {
    uint32_t elapsed = now - _transitionStart;
    if (elapsed >= TRANSITION_MS) {
        finishPageTransition();
        return;
    }

    // Show only labels for the first TRANSITION_STATIC_MS (labels
    // were already drawn by startPageTransition), then begin the crossfade.
    if (elapsed < TRANSITION_STATIC_MS) return;

    uint32_t crossfadeElapsed = elapsed - TRANSITION_STATIC_MS;
    uint32_t crossfadeDuration = TRANSITION_MS - TRANSITION_STATIC_MS;
    State::Snapshot s = state.getSnapshot();
    uint8_t blendStep = (uint8_t)((crossfadeElapsed * 17U) / crossfadeDuration);
    if (blendStep > 16) blendStep = 16;

    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        drawTransitionSlot(i, pageMetric(i), s, blendStep);
    }
}

void ST7789_240x240::drawTransitionSlot(uint8_t slotIndex, MetricID id, State::Snapshot& s, uint8_t blendStep) {
    MetricSlot slot = metricSlot(slotIndex);
    const MetricDefinition* metric = metricDefinition(id);
    if (metric == nullptr || slot.canvas == nullptr || slot.labelCanvas == nullptr || slot.valueCanvas == nullptr) return;

    char value[sizeof(_renderedMetrics[slotIndex].value)] = {};
    bool isNumeric = true;
    if (!formatMetricValue(id, s, value, sizeof(value), &isNumeric)) return;

    const uint8_t* valueFont = selectValueFont(value, isNumeric);
    uint8_t valueTextSize = isNumeric ? slot.valueTextSize : slot.labelTextSize;
    uint8_t valueVerticalOffset = isNumeric ? slot.valueVerticalOffset : slot.labelVerticalOffset;

    renderTextToCanvas(slot.labelCanvas, metric->label, labelFont, slot.labelTextSize, slot.labelVerticalOffset);
    renderTextToCanvas(slot.valueCanvas, value, valueFont, valueTextSize, valueVerticalOffset);
    blendMonoCanvases(slot.labelCanvas, slot.valueCanvas, slot.canvas, blendStep);
    slot.canvas->flush();
}

bool ST7789_240x240::startPasFeedbackIfNeeded(uint32_t now, State::Snapshot& s) {
    if (!_pasFeedbackInitialized) {
        _lastPasLevel = s.pasLevel;
        _pasFeedbackInitialized = true;
        return false;
    }

    if (s.pasLevel == _lastPasLevel) return false;
    _lastPasLevel = s.pasLevel;

    if (pageIncludesMetric(METRIC_PAS)) return false;

    startFeedback(s, now);
    return true;
}

void ST7789_240x240::startFeedback(State::Snapshot& s, uint32_t now) {
    ESP_LOGD(tag, "Starting PAS feedback");
    _displayMode = MODE_FEEDBACK;
    _feedbackStart = now;
    _lastPasFeedbackUpdate = now;
    drawPasFeedbackValue(s);
}

void ST7789_240x240::finishFeedback(State::Snapshot& s) {
    ESP_LOGD(tag, "Finishing PAS feedback");
    _displayMode = MODE_PAGE;
    drawMetricValue(SLOT_MAJOR, pageMetric(SLOT_MAJOR), s, true, true);
    _lastPageUpdate = millis();
}

void ST7789_240x240::drawFeedbackFrame(uint32_t now, State::Snapshot& s) {
    uint32_t elapsed = now - _feedbackStart;
    if (elapsed >= TRANSITION_STATIC_MS + FEEDBACK_FADE_MS) {
        finishFeedback(s);
        return;
    }

    if (elapsed < TRANSITION_STATIC_MS) return;

    uint32_t fadeElapsed = elapsed - TRANSITION_STATIC_MS;
    uint8_t blendStep = (uint8_t)((fadeElapsed * 17U) / FEEDBACK_FADE_MS);
    if (blendStep > 16) blendStep = 16;
    drawPasFeedbackBlend(s, blendStep);
}

void ST7789_240x240::drawPasFeedbackValue(State::Snapshot& s) {
    MetricSlot slot = metricSlot(SLOT_MAJOR);
    if (slot.canvas == nullptr) return;

    char value[sizeof(_renderedMetrics[SLOT_MAJOR].value)] = {};
    bool isNumeric = true;
    if (!formatPasValue(s.pasLevel, value, sizeof(value), &isNumeric)) return;

    const uint8_t* font = selectValueFont(value, isNumeric);
    uint8_t textSize = isNumeric ? slot.valueTextSize : slot.labelTextSize;
    uint8_t verticalOffset = isNumeric ? slot.valueVerticalOffset : slot.labelVerticalOffset;
    drawSlotText(slot, value, font, textSize, verticalOffset);
}

void ST7789_240x240::drawPasFeedbackBlend(State::Snapshot& s, uint8_t blendStep) {
    MetricSlot slot = metricSlot(SLOT_MAJOR);
    if (slot.canvas == nullptr || slot.labelCanvas == nullptr || slot.valueCanvas == nullptr) return;

    char pasValue[sizeof(_renderedMetrics[SLOT_MAJOR].value)] = {};
    bool pasIsNumeric = true;
    if (!formatPasValue(s.pasLevel, pasValue, sizeof(pasValue), &pasIsNumeric)) return;

    char targetValue[sizeof(_renderedMetrics[SLOT_MAJOR].value)] = {};
    bool targetIsNumeric = true;
    if (!formatMetricValue(pageMetric(SLOT_MAJOR), s, targetValue, sizeof(targetValue), &targetIsNumeric)) return;

    const uint8_t* pasFont = selectValueFont(pasValue, pasIsNumeric);
    uint8_t pasTextSize = pasIsNumeric ? slot.valueTextSize : slot.labelTextSize;
    uint8_t pasVerticalOffset = pasIsNumeric ? slot.valueVerticalOffset : slot.labelVerticalOffset;

    const uint8_t* targetFont = selectValueFont(targetValue, targetIsNumeric);
    uint8_t targetTextSize = targetIsNumeric ? slot.valueTextSize : slot.labelTextSize;
    uint8_t targetVerticalOffset = targetIsNumeric ? slot.valueVerticalOffset : slot.labelVerticalOffset;

    renderTextToCanvas(slot.labelCanvas, pasValue, pasFont, pasTextSize, pasVerticalOffset);
    renderTextToCanvas(slot.valueCanvas, targetValue, targetFont, targetTextSize, targetVerticalOffset);
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
        _renderedMetrics[i].value[0] = '\0';
        _renderedMetrics[i].valid = false;
    }
}

const ST7789_240x240::PageLayout& ST7789_240x240::currentPageLayout() const {
    return defaultPages[_currentPage];
}

ST7789_240x240::MetricSlot ST7789_240x240::metricSlot(uint8_t slotIndex) const {
    // canvas, labelCanvas, valueCanvas, valueTextSize, labelTextSize, valueVOffset, labelVOffset
    switch (slotIndex) {
        case SLOT_MAJOR:
            return {canvasMajor, transitionLabelMajor, transitionValueMajor, 2, 2, 4, 8};
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

bool ST7789_240x240::pageIncludesMetric(MetricID id) const {
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        if (pageMetric(i) == id) return true;
    }
    return false;
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

/*
    Draws metric values to all slots.
    s: snapshot of the current state
    force: if true, always draw the value, even if it hasn't changed
    remember: if true, remember the value and don't redraw it if it hasn't changed
*/
void ST7789_240x240::drawPageValues(State::Snapshot& s, bool force, bool remember) {
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        drawMetricValue(i, pageMetric(i), s, force, remember);
    }
}

/*
    Draws a single metric value to a slot.
    - slotIndex: index of the slot to draw to
    - id: metric to draw
    - s: snapshot of the current state
    - force: if true, always draw the value, even if it hasn't changed
    - remember: if true, remember the value and don't redraw it if it hasn't changed
    If the controller is not alive, "ZZZ" is shown in the major slot.
*/
void ST7789_240x240::drawMetricValue(uint8_t slotIndex, MetricID id, State::Snapshot& s, bool force, bool remember) {
    MetricSlot slot = metricSlot(slotIndex);
    if (slot.canvas == nullptr) return;

    RenderedMetric& rendered = _renderedMetrics[slotIndex];
    char value[sizeof(rendered.value)] = {};
    bool isNumeric = true;

    // If the controller is not alive, indicate it in the major
    // slot by showing "ZZZ" instead of the metric value.
    // Otherwise, format the metric value.
    if (!s.controllerAlive && slotIndex == SLOT_MAJOR) {
        strlcpy(value, "ZZZ", sizeof(value));
        isNumeric = false;
    } else {
        if (!formatMetricValue(id, s, value, sizeof(value), &isNumeric)) return;
    }

    if (remember && !force && rendered.valid && strcmp(rendered.value, value) == 0) return;

    const uint8_t* font = selectValueFont(value, isNumeric);
    uint8_t textSize = isNumeric ? slot.valueTextSize : slot.labelTextSize;
    uint8_t verticalOffset = isNumeric ? slot.valueVerticalOffset : slot.labelVerticalOffset;
    drawSlotText(slot, value, font, textSize, verticalOffset);

    if (!remember) return;
    Util::copyString(rendered.value, sizeof(rendered.value), value);
    rendered.valid = true;
}

const uint8_t* ST7789_240x240::selectValueFont(const char* value, bool isNumeric) const {
    if (!isNumeric) return labelFont;
    return strlen(value) >= 3 ? mediumFont : largeFont;
}

void ST7789_240x240::drawSlotText(
    MetricSlot& slot,
    const char* text,
    const uint8_t* font,
    uint8_t textSize,
    uint8_t verticalOffset,
    bool invertColors) {
    if (slot.canvas == nullptr || text == nullptr || font == nullptr) return;

    renderTextToCanvas(slot.canvas, text, font, textSize, verticalOffset, invertColors);
    slot.canvas->flush();
}

void ST7789_240x240::renderTextToCanvas(
    Canvas* canvas,
    const char* text,
    const uint8_t* font,
    uint8_t textSize,
    uint8_t verticalOffset,
    bool invertColors) {
    if (canvas == nullptr || text == nullptr || font == nullptr) return;

    uint16_t textColor = invertColors ? BLACK : WHITE;
    uint16_t backgroundColor = invertColors ? WHITE : BLACK;
    canvas->setFont(font);
    canvas->setTextSize(textSize);
    canvas->setTextColor(textColor, backgroundColor);
    canvas->fillRect(0, 0, canvas->w(), canvas->h(), backgroundColor);

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t textWidth = 0;
    uint16_t textHeight = 0;
    canvas->getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);

    /*
    int16_t cursorX = -x1;
    if (textWidth < canvas->w()) {
        cursorX = (canvas->w() - textWidth) / 2 - x1;
    }
    */

    int16_t cursorX = canvas->w() - textWidth - 8;  // align right

    if ((canvas == canvasMajor || canvas == transitionLabelMajor || canvas == transitionValueMajor) &&
        font == largeFont) {
        cursorX -= 6;
        // ESP_LOGD(tag, "Extra offset for '%s'", text);
    }

    if (cursorX < 0) {
        // ESP_LOGW(tag, "Text overflow for '%s' (%dpx), aligning to left", text, textWidth);
        cursorX = 0;
    }

    canvas->setCursor(cursorX, canvas->h() - verticalOffset);
    canvas->print(text);
}

void ST7789_240x240::blendMonoCanvases(
    Canvas* fromCanvas,
    Canvas* toCanvas,
    Canvas* outputCanvas,
    uint8_t blendStep) {
    if (fromCanvas == nullptr || toCanvas == nullptr || outputCanvas == nullptr) return;

    uint8_t* from = fromCanvas->getFramebuffer();
    uint8_t* to = toCanvas->getFramebuffer();
    uint8_t* output = outputCanvas->getFramebuffer();
    if (from == nullptr || to == nullptr || output == nullptr) return;

    uint16_t width = outputCanvas->w();
    uint16_t height = outputCanvas->h();
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
    if (buffer == nullptr || isNumeric == nullptr || bufferSize < 4) {
        ESP_LOGE(tag, "Invalid metric format buffer");
        return false;
    }

    *isNumeric = true;
    switch (id) {
        case METRIC_SPEED:
            formatUInt(buffer, bufferSize, roundedMetricValue(s.speed()));
            return true;
        case METRIC_CADENCE:
            // formatUInt(buffer, bufferSize, 87);
            formatUInt(buffer, bufferSize, s.cadence);
            return true;
        case METRIC_PAS:
            return formatPasValue(s.pasLevel, buffer, bufferSize, isNumeric);
        case METRIC_MOTOR_PWR:
            // abbreviatedMetricValue(buffer, bufferSize, (uint32_t)789, isNumeric);
            abbreviatedMetricValue(buffer, bufferSize, (uint32_t)s.motorPower(), isNumeric);
            return true;
        case METRIC_HUMAN_PWR:
            // abbreviatedMetricValue(buffer, bufferSize, (uint32_t)234, isNumeric);
            abbreviatedMetricValue(buffer, bufferSize, (uint32_t)s.humanPower(), isNumeric);
            return true;
        case METRIC_VOLTAGE:  // 49.2 → "492"
            formatUInt(buffer, bufferSize, roundedMetricValue((float)s.batteryVoltage_x100 / 10.0f));
            return true;
        case METRIC_SOC:
            formatUInt(buffer, bufferSize, s.soc());
            return true;
        case METRIC_MOTOR_TEMP:
            formatUInt(buffer, bufferSize, s.motorTemp);
            return true;
        case METRIC_TRIP: {
            uint32_t km = s.trip_mx10 / 100;
            abbreviatedMetricValue(buffer, bufferSize, km, isNumeric);
            return true;
        }
        case METRIC_ODO: {
            uint32_t km = s.odo_mx10 / 100;
            abbreviatedMetricValue(buffer, bufferSize, km, isNumeric);
            return true;
        }
        case METRIC_RANGE:
            formatUInt(buffer, bufferSize, roundedMetricValue((float)s.range()));
            return true;
        case METRIC_HEART_RATE:  // TODO: Implement live heart rate
            formatUInt(buffer, bufferSize, 123);
            return true;
        case METRIC_BODY_TEMP:  // TODO: Implement live body temperature
            snprintf(buffer, bufferSize, "%.1f", 36.5f);
            return true;
        default:
            ESP_LOGW(tag, "Ignoring invalid metric id: %d", id);
            return false;
    }
}

// writes WLK, OFF, or P1-P99 to the buffer
bool ST7789_240x240::formatPasValue(int8_t pas, char* buffer, size_t bufferSize, bool* isNumeric) {
    *isNumeric = false;
    if (pas < 0) {
        snprintf(buffer, bufferSize, "WLK");
        return true;
    }
    if (pas == 0) {
        snprintf(buffer, bufferSize, "OFF");
        return true;
    }
    // Capital P glyph is included in the large font
    snprintf(buffer, bufferSize, "P%u", cappedMetricValue((uint32_t)pas, 99));
    *isNumeric = true;
    return true;
}

// returns the rounded integer value between 0 and 999
uint16_t ST7789_240x240::roundedMetricValue(float value) const {
    if (value <= 0.0f) return 0;
    if (value >= 999.0f) return 999;
    return (uint16_t)(value + 0.5f);
}

// returns value between 0 and cap
uint16_t ST7789_240x240::cappedMetricValue(uint32_t value, uint16_t cap) const {
    return value > cap ? cap : (uint16_t)value;
}

/**
 * Writes the given value into the buffer, abbreviated so that the result fits.
 *
 * @param buffer      Destination buffer (must be at least 2 bytes for minimal output).
 * @param bufferSize  Size of the buffer (including the null terminator).
 * @param value       Unsigned integer to display.
 * @param isNumeric   Will be set to true if the buffer contains only digits (no suffix).
 *
 * The function tries to show as many significant digits as possible:
 *   - If the plain number fits, it is written directly.
 *   - Otherwise, an abbreviated form with a suffix (k, M, G) is used.
 *   - When the suffix alone doesn't fit, the scaled integer may be shown without
 *     any suffix (isNumeric is set to true).
 */
void ST7789_240x240::abbreviatedMetricValue(char* buffer, size_t bufferSize, uint32_t value, bool* isNumeric) {
    if (buffer == nullptr || isNumeric == nullptr || bufferSize < 2) {
        // Need at least 1 character + null terminator
        if (buffer && bufferSize > 0) buffer[0] = '\0';
        return;
    }

    const size_t maxChars = bufferSize - 1;  // usable width

    // Helper: count decimal digits
    auto numDigits = [](uint32_t v) {
        if (v == 0) return (uint32_t)1;
        uint32_t count = 0;
        while (v) {
            v /= 10;
            ++count;
        }
        return count;
    };

    // 1. Plain number fits?
    uint32_t d = numDigits(value);
    if (d <= maxChars) {
        if (d <= 3)
            formatUInt(buffer, bufferSize, value);
        else
            snprintf(buffer, bufferSize, "%u", value);
        *isNumeric = true;
        return;
    }

    // Available suffixes – largest first (order doesn’t matter for final selection)
    static constexpr struct {
        uint32_t divisor;
        char suffix;
    } suffixes[] = {
        {1000000000, 'G'},  // billion
        {1000000, 'M'},     // million
        {1000, 'k'}         // thousand
    };

    struct Candidate {
        uint32_t int_part;
        uint32_t extra_digit;
        char suffix;
        bool use_suffix;
        bool use_extra;
        int digits_shown;  // how many original significant digits are visible
        size_t length;     // total characters (excluding null)
        bool is_numeric;
    };

    Candidate best = {0, 0, 0, false, false, -1, 0, false};

    auto consider = [&](const Candidate& c) {
        if (c.length <= maxChars && c.digits_shown > best.digits_shown) {
            best = c;
        }
    };

    // 2. Try all suffix levels
    for (const auto& s : suffixes) {
        uint32_t int_part = value / s.divisor;
        if (int_part == 0) continue;  // suffix too large, skip

        int len_int = numDigits(int_part);

        // Scaled integer without suffix (fallback when suffix doesn't fit)
        if (len_int <= (int)maxChars) {
            Candidate c{};
            c.int_part = int_part;
            c.suffix = s.suffix;
            c.use_suffix = false;
            c.use_extra = false;
            c.digits_shown = len_int;
            c.length = len_int;
            c.is_numeric = true;
            consider(c);
        }

        // With suffix, no extra digit: "<int_part><suffix>" (e.g. "12k")
        if (len_int + 1 <= maxChars) {
            Candidate c{};
            c.int_part = int_part;
            c.suffix = s.suffix;
            c.use_suffix = true;
            c.use_extra = false;
            c.digits_shown = len_int;
            c.length = len_int + 1;
            c.is_numeric = false;
            consider(c);
        }

        // With suffix and one extra digit: "<int_part><suffix><digit>" (e.g. "1k2")
        if (len_int + 2 <= maxChars) {
            uint32_t extra = (value / (s.divisor / 10)) % 10;
            Candidate c{};
            c.int_part = int_part;
            c.extra_digit = extra;
            c.suffix = s.suffix;
            c.use_suffix = true;
            c.use_extra = true;
            c.digits_shown = len_int + 1;
            c.length = len_int + 2;
            c.is_numeric = false;
            consider(c);
        }
    }

    // 3. Write the best representation found
    if (best.digits_shown >= 0) {
        if (best.use_suffix) {
            if (best.use_extra)
                snprintf(buffer, bufferSize, "%u%c%u", best.int_part, best.suffix, best.extra_digit);
            else
                snprintf(buffer, bufferSize, "%u%c", best.int_part, best.suffix);
            *isNumeric = false;
        } else {
            // Pure digits (scaled, no suffix)
            snprintf(buffer, bufferSize, "%u", best.int_part);
            *isNumeric = true;
        }
    } else {
        // Extremely narrow buffer – show a placeholder
        buffer[0] = '?';
        buffer[1] = '\0';
        *isNumeric = false;
    }
}

/**
 * Writes the given value into the buffer, capped to 999.
 * - doesn't use snprintf, for speed
 * - if the buffer is too small, it will be empty
 */
void ST7789_240x240::formatUInt(char* buffer, size_t bufferSize, uint16_t value) {
    uint16_t capped = cappedMetricValue(value);

    // Temporary buffer for the reversed number string
    char temp[MAX_METRIC_VALUE_LEN + 1];
    int i = 0;

    // Extract the digits from the number
    do {
        temp[i++] = (capped % 10) + '0';
        capped /= 10;
    } while (capped > 0 && i < MAX_METRIC_VALUE_LEN);

    // If the buffer is too small, terminate it and return
    if ((size_t)i >= bufferSize) {
        if (bufferSize > 0) buffer[0] = '\0';
        return;
    }

    // Reverse the string and copy it to the target buffer
    int j = 0;
    while (i > 0) {
        buffer[j++] = temp[--i];
    }
    buffer[j] = '\0';  // Guarantee null termination
}

void ST7789_240x240::drawMenu(const Menu::Snapshot& menu) {
    canvasFullScreen->setFont(smallFont);
    canvasFullScreen->setTextSize(1);
    canvasFullScreen->setTextColor(WHITE, BLACK);
    canvasFullScreen->fillScreen(BLACK);

    constexpr uint8_t MAX_VISIBLE_MENU_ITEMS = 7;
    static uint8_t scrollOffset = 0;
    if (menu.selectedItem < scrollOffset) {
        scrollOffset = menu.selectedItem;
    } else if (menu.selectedItem >= scrollOffset + MAX_VISIBLE_MENU_ITEMS - 1) {  // -1 because the last item may not be fully visible
        scrollOffset = menu.selectedItem - MAX_VISIBLE_MENU_ITEMS + 2;            // +2 to make sure the selected item is fully visible
    }
    if (scrollOffset >= menu.itemCount) {
        scrollOffset = 0;
    }

    uint8_t end = scrollOffset + MAX_VISIBLE_MENU_ITEMS;
    if (end > menu.itemCount) {
        end = menu.itemCount;
    }
    for (uint8_t i = scrollOffset; i < end; i++) {
        if (i > menu.itemCount) {
            ESP_LOGW(tag, "Menu item index %u exceeds itemCount %u", i, menu.itemCount);
            break;
        }
        // ESP_LOGD(tag, "Drawing menu item %u: '%s'%s", i, menu.items[i], i == menu.selectedItem ? " (selected)" : "");
        drawMenuLine(menu.items[i], 34 + (i - scrollOffset) * 36, i == menu.selectedItem);
    }

    canvasFullScreen->flush();
}

void ST7789_240x240::drawMenuLine(const char* text, int16_t baseline, bool selected) {
    if (text == nullptr) return;

    uint16_t textColor = selected ? BLACK : WHITE;
    uint16_t backgroundColor = selected ? WHITE : BLACK;

    canvasFullScreen->fillRect(0, baseline - 30, canvasFullScreen->w(), 34, backgroundColor);
    canvasFullScreen->setTextColor(textColor);
    /* // centered
    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t textWidth = 0;
    uint16_t textHeight = 0;
    canvasFullScreen->getTextBounds(text, 0, 0, &x1, &y1, &textWidth, &textHeight);
    int16_t cursorX = (canvasFullScreen->w() - textWidth) / 2 - x1;
    canvasFullScreen->setCursor(cursorX, baseline);
    */
    // left aligned
    canvasFullScreen->setCursor(5, baseline);
    canvasFullScreen->print(text);
}
