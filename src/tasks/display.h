#ifndef DISPLAY_H
#define DISPLAY_H

#include "task.h"
#include "model/state.h"
#include "config.h"
#include "api.h"

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
        apiClientSetup(taskName());
        api.registerCommand(
            "nextpage",
            [this](const char* args) {
                return nextPageCommand(args);
            },
            "Usage: nextpage\nSwitches to the next page.");
        Task::taskSetup();
    }

    virtual void taskRun() override {
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

    bool upClicked() {
        return output.menuPrevious();
    }
    bool downClicked() {
        return output.menuNext();
    }
    void selectClicked() {
        output.selectClicked = true;
    }
    void enterMenu() {
        output.enterMenu();
    }
    bool menuActive() {
        return output.menuActive();
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
    ST7789_240x240 output{
        TFT_CS,
        TFT_DC,
        SPI_MOSI,
        SPI_SCK,
        TFT_RST,
        TFT_BL,
        SPI2_HOST,
        TFT_ROTATION};
    uint8_t brightnessPercent = 0;
};

#endif  // DISPLAY_H
