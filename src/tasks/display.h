#ifndef DISPLAY_H
#define DISPLAY_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "task.h"
#include "config.h"
#include "api.h"
#include "has_preferences.h"
#include "ui/menu.h"
#include "ui/events.h"
// #include "tasks/wifi.h"

class State;

#if ORD_DISPLAY == st7789_240x240
#include "drivers/st7789_240x240.h"
#else
#error "Unsupported display. Please define ORD_DISPLAY in platformio.ini"
#endif

class Display : public Task, public ApiClient, public HasPreferences {
   public:
    virtual const char* taskName() const override;
    virtual void setup();
    virtual void taskRun() override;

    bool queueUiEvent(UiEvent event);

    bool increaseBrightness();
    bool decreaseBrightness();
    bool saveBrightness();

    Api::Reply nextPageCommand(const char* args);

    void receiveReply(const Api::Reply& reply) override;

    Menu menu;

    // TOOD: Query Wifi task for current AP status and SSID
    // instead of storing them in the display task.
    bool wifiApMode = false;   // Set by Wifi task
    char wifiApSsid[64] = {};  // Set by Wifi task

   protected:
    static constexpr UBaseType_t UI_EVENT_QUEUE_LENGTH = 16;

    ST7789_240x240 output{
        PIN_TFT_CS,
        PIN_TFT_DC,
        PIN_SPI_MOSI,
        PIN_SPI_SCK,
        PIN_TFT_RST,
        PIN_TFT_BL,
        SPI2_HOST,
        TFT_ROTATION};

    QueueHandle_t uiEventQueue = nullptr;
    char brightnessPrefKey[NVS_KEY_NAME_MAX_SIZE] = "brightness";
    uint8_t savedBrightnessPercent = 100;  // default to 100% brightness
    uint8_t brightnessPercent = 100;       // current brightness level, 0-100%
    uint32_t lastBrightnessChange = 0;
    static constexpr uint8_t brightnessChangeDelay = 50;
    bool menuShown = false;
    bool passkeyActive = false;
    bool passkeyShown = false;
    bool apSsidShown = false;

    bool loadPreferences();
    void processUiEvents();
    void handleUiEvent(UiEvent event);
    void syncPasskeyDisplay();
    void syncApDisplay();
    void syncMenuDisplay();
    void adjustPasLevel(int8_t delta);
    void handleUpLongPress();
    void handleDownLongPress();
    bool shouldLogLongPress();
};

#endif  // DISPLAY_H
