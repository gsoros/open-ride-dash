#ifndef DISPLAY_H
#define DISPLAY_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "task.h"
#include "model/state.h"
#include "config.h"
#include "api.h"
#include "ui/menu_controller.h"
#include "ui/ui_events.h"

#if ORD_DISPLAY == st7789_240x240
#include "drivers/st7789_240x240.h"
#else
#error "Unsupported display. Please define ORD_DISPLAY in platformio.ini"
#endif

extern State state;

class Display : public Task, public ApiClient {
   public:
    virtual const char* taskName() override {
        return "Display";
    }

    virtual void setup() {
        output.setup();
        brightnessPercent = 50;
        output.setBrightnessPercent(brightnessPercent);
        output.splash();
        uiEventQueue = xQueueCreate(UI_EVENT_QUEUE_LENGTH, sizeof(UiEvent));
        if (uiEventQueue == nullptr) {
            ESP_LOGE(taskName(), "Failed to create UI event queue");
        }
        apiClientSetup(taskName());
        api.registerCommand(
            "nextpage",
            [this](const char* args) {
                return nextPageCommand(args);
            },
            "Usage: nextpage\nSwitches to the next page.");
    }

    virtual void taskRun() override {
        processUiEvents();
        syncMenuDisplay();
        output.update();
    }

    void increaseBrightness() {
        brightnessPercent++;
        if (brightnessPercent > 100) {
            brightnessPercent = 100;
            return;
        }
        output.setBrightnessPercent(brightnessPercent);
    }

    void decreaseBrightness() {
        brightnessPercent--;
        if (brightnessPercent < 2) {
            brightnessPercent = 1;
            return;
        }
        output.setBrightnessPercent(brightnessPercent);
    }

    bool queueUiEvent(UiEvent event) {
        if (uiEventQueue == nullptr) {
            ESP_LOGE(taskName(), "UI event queue is null");
            return false;
        }
        BaseType_t res = xQueueSend(uiEventQueue, &event, 0);
        if (res != pdTRUE) {
            ESP_LOGW(taskName(), "UI event queue full, dropping event %u", (uint8_t)event);
            return false;
        }
        return true;
    }

    Api::Reply nextPageCommand(const char* args) {
        ESP_LOGI(taskName(), "Switching to next page");
        output.nextPage();
        Api::Reply reply = {};
        reply.code = Api::ReplyCode::SUCCESS;
        snprintf((char*)reply.data, sizeof(reply.data), "Page %d", output.currentPage());
        return reply;
    }

    void receiveReply(const Api::Reply& reply) override {
        char rtext[32];
        api.replyCodeToString(reply.code, rtext, sizeof(rtext));
        ESP_LOGD(taskName(), "Received API reply: %s: %s", reply.command, rtext);
    }

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
    MenuController menu;
    QueueHandle_t uiEventQueue = nullptr;
    uint8_t brightnessPercent = 0;
    uint32_t lastBrightnessChange = 0;
    static constexpr uint8_t brightnessChangeDelay = 50;
    bool menuShown = false;

    void processUiEvents() {
        if (uiEventQueue == nullptr) return;

        UiEvent event;
        while (xQueueReceive(uiEventQueue, &event, 0) == pdTRUE) {
            handleUiEvent(event);
        }
    }

    void handleUiEvent(UiEvent event) {
        switch (event) {
            case UiEvent::UpClick:
                if (menu.previousMenuItem()) return;
                adjustPasLevel(1);
                return;
            case UiEvent::DownClick:
                if (menu.nextMenuItem()) return;
                adjustPasLevel(-1);
                return;
            case UiEvent::SelectClick:
                if (menu.active()) {
                    menu.selectMenuItem();
                    return;
                }
                output.nextPage();
                return;
            case UiEvent::UpLongPress:
                handleUpLongPress();
                return;
            case UiEvent::DownLongPress:
                handleDownLongPress();
                return;
            case UiEvent::PowerLongPress:
                // ESP_LOGD(taskName(), "Key power long press");
                return;
            case UiEvent::MenuChord:
                if (menu.active())
                    menu.exitMenu();
                else
                    menu.enterMenu();
                return;
            default:
                ESP_LOGW(taskName(), "Unhandled UI event: %u", (uint8_t)event);
                return;
        }
    }

    void syncMenuDisplay() {
        if (menu.active()) {
            MenuSnapshot snapshot = menu.snapshot();
            if (output.showMenu(snapshot)) {
                menu.markRendered();
            }
            menuShown = true;
            return;
        }

        if (!menuShown) return;
        output.exitMenu();
        menu.markRendered();
        menuShown = false;
    }

    void adjustPasLevel(int8_t delta) {
        int8_t pasLevelRequested = state.pasLevelRequested();
        int8_t next = pasLevelRequested + delta;
        if (next < -1 || next > 5) return;
        state.pasLevelRequested(next);
    }

    void handleUpLongPress() {
        if (menu.active()) return;
        if (lastBrightnessChange + brightnessChangeDelay > millis()) return;
        if (shouldLogLongPress()) {
            ESP_LOGD(taskName(), "Key up long press (increase brightness)");
        }
        increaseBrightness();
        lastBrightnessChange = millis();
    }

    void handleDownLongPress() {
        if (menu.active()) return;
        if (lastBrightnessChange + brightnessChangeDelay > millis()) return;
        bool isWalkAssist = state.pasLevel() == -1;
        if (shouldLogLongPress()) {
            ESP_LOGW(taskName(), "Key down long press (%s)",
                     isWalkAssist ? "walk assist" : "decrease brightness");
        }
        if (isWalkAssist) return;
        decreaseBrightness();
        lastBrightnessChange = millis();
    }

    bool shouldLogLongPress() {
        static uint32_t lastLongPressLog = 0;
        uint32_t now = millis();
        if (now - lastLongPressLog <= 500) return false;
        lastLongPressLog = now;
        return true;
    }
};

#endif  // DISPLAY_H
