/* ILI9341 240x320 Display Driver - Stub (no-op) */

#ifndef ILI9341_240x320_H
#define ILI9341_240x320_H

#include <Arduino_GFX_Library.h>

#include "display_driver.h"
#include "model/state.h"
#include "ui/menu.h"

class ILI9341_240x320 : public DisplayDriver {
   public:
    ILI9341_240x320(
        int8_t cs,
        int8_t dc,
        int8_t mosi,
        int8_t sck,
        int8_t rst = -1,
        int8_t bl = -1,
        uint8_t spi = SPI2_HOST,
        uint8_t rot = 0,
        uint16_t w = 240,
        uint16_t h = 320);

    void setup() override;
    void splash() override;
    void update() override;
    void clear() override;
    void fillScreen(uint16_t color) override;
    void setRotation(uint8_t rotation) override;
    void setBrightnessPercent(uint8_t p) override;
    bool hasBacklight() override { return bl_pin >= 0; }
    bool setBacklight(uint8_t level) override;
    void onSleep() override;
    void onRestart() override;
    void onWifiChange() override;
    void onBleChange() override;
    bool showPasskey(uint32_t passkey) override;  // returns true if passkey was shown
    void exitPasskey() override;

    bool showApSsid(const char* ssid) override;  // returns true if AP SSID was shown
    void exitApSsid() override;

    void nextPage() override;
    uint8_t currentPage() override;

    bool showMenu(const Menu::Snapshot& menu) override;  // returns true if menu was shown
    void exitMenu() override;

   protected:
    const char* tag = "ILI9341_240x320";
    int8_t bl_pin;    // backlight pin, -1 if not used
    uint16_t width;   // display width
    uint16_t height;  // display height
};

#endif  // ILI9341_240x320_H
