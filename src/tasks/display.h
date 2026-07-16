#ifndef DISPLAY_H
#define DISPLAY_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <memory>

#include "task.h"
#include "config.h"
#include "api.h"
#include "has_preferences.h"
#include "ui/menu.h"
#include "ui/events.h"
#include "drivers/display_driver.h"

#if ORD_DISPLAY == st7789_240x240
#include "drivers/st7789_240x240.h"
#elif ORD_DISPLAY == ili9341_240x320
#include "drivers/ili9341_240x320.h"
#else
#error "Unsupported display. Please define ORD_DISPLAY in platformio.ini"
#endif

// Factory that constructs the display driver selected by ORD_DISPLAY at compile time.
std::unique_ptr<DisplayDriver> makeDisplayDriver();

class State;

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

   protected:
    static constexpr UBaseType_t UI_EVENT_QUEUE_LENGTH = 16;

    std::unique_ptr<DisplayDriver> output = makeDisplayDriver();

    QueueHandle_t uiEventQueue = nullptr;
    char brightnessPrefKey[NVS_KEY_NAME_MAX_SIZE] = "brightness";
    uint8_t savedBrightnessPercent = 100;  // default to 100% brightness
    uint8_t brightnessPercent = 100;       // current brightness level, 0-100%
    uint32_t lastBrightnessChange = 0;
    static constexpr uint8_t brightnessChangeDelay = 50;
    bool passkeyActive = false;  // logical: a BLE passkey is currently required
    bool apActive = false;       // logical: WiFi AP is currently enabled

    // Display state machine: which screen is currently shown.
    enum class Screen : uint8_t { Metrics,   // Displays the actual metrics on pages
                                  Menu,      // Shows the menu
                                  Passkey,   // Shows a BLE passkey
                                  ApSsid };  // Shows the Wi-Fi AP SSID
    Screen currentScreen = Screen::Metrics;
    bool screenEntered = false;  // whether enterScreen() has succeeded for currentScreen

    bool loadPreferences();
    void processUiEvents();
    void handleUiEvent(UiEvent event);
    Screen resolveScreen() const;  // desired screen from logical inputs (priority order)
    bool enterScreen(Screen s);    // draw screen on entry; returns true on success (retried until then)
    void exitScreen(Screen s);     // tear down screen on exit
    void runScreen(Screen s);      // per-frame update for the active screen
    void adjustPasLevel(int8_t delta);
    void handleUpLongPress();
    void handleDownLongPress();
    bool shouldLogLongPress();
};

#endif  // DISPLAY_H
