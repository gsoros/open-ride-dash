/* ILI9341 240x320 Display Driver - Stub (no-op) Implementation */

#include "ili9341_240x320.h"

#include <Arduino.h>

#include "config.h"
#include "util.h"

extern State state;

ILI9341_240x320::ILI9341_240x320(
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
    // TODO: initialize the data bus and display (Arduino_GFX / Arduino_ILI9341).
}

void ILI9341_240x320::setup() {
    // TODO: configure backlight pin and begin the display.
}

void ILI9341_240x320::splash() {
    // TODO: draw the splash screen.
}

void ILI9341_240x320::update() {
    // TODO: render the current page/metrics.
}

void ILI9341_240x320::clear() {
    // TODO: clear the display.
}

void ILI9341_240x320::fillScreen(uint16_t color) {
    // TODO: fill the entire screen with the given color.
}

void ILI9341_240x320::setRotation(uint8_t rotation) {
    // TODO: set the display rotation.
}

void ILI9341_240x320::setBrightnessPercent(uint8_t p) {
    // TODO: map p (0-100) to a backlight level, if supported.
}

bool ILI9341_240x320::setBacklight(uint8_t level) {
    // TODO: drive the backlight pin. Return true on success.
    return false;
}

void ILI9341_240x320::onSleep() {
    // TODO: put the display to sleep.
}

void ILI9341_240x320::onRestart() {
    // TODO: handle restart (e.g. wake from sleep).
}

void ILI9341_240x320::onWifiChange() {
    // TODO: refresh any Wi-Fi status shown on screen.
}

void ILI9341_240x320::onBleChange() {
    // TODO: refresh any BLE status shown on screen.
}

bool ILI9341_240x320::showPasskey(uint32_t passkey) {
    // TODO: render the BLE passkey. Return true once shown.
    return false;
}

void ILI9341_240x320::exitPasskey() {
    // TODO: tear down the passkey screen.
}

bool ILI9341_240x320::showApSsid(const char* ssid) {
    // TODO: render the Wi-Fi AP SSID. Return true once shown.
    return false;
}

void ILI9341_240x320::exitApSsid() {
    // TODO: tear down the AP SSID screen.
}

void ILI9341_240x320::nextPage() {
    // TODO: advance to the next metrics page.
}

uint8_t ILI9341_240x320::currentPage() {
    // TODO: return the active page index.
    return 0;
}

bool ILI9341_240x320::showMenu(const Menu::Snapshot& menu) {
    // TODO: render the menu. Return true once drawn.
    return false;
}

void ILI9341_240x320::exitMenu() {
    // TODO: tear down the menu screen.
}
