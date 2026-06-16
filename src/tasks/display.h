#ifndef DISPLAY_H
#define DISPLAY_H

#include "task.h"
#include "model/state.h"
#include "config.h"

#if ORD_DISPLAY == st7789_240x240
#include "drivers/st7789_240x240.h"
#else
#error "Unsupported display. Please define ORD_DISPLAY in platformio.ini"
#endif

extern State state;

class Display : public Task {
   public:
    virtual const char* taskName() override {
        return "Display";
    }

    virtual void setup() {
        output.setup();
        brightnessPercent = 50;
        output.setBrightnessPercent(brightnessPercent);
        output.splash();
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

    bool keyUpClick() {
        return output.menuPrevious();
    }
    bool keyDownClick() {
        return output.menuNext();
    }
    void keyPowerClick() {
        output.keyPowerClick = true;
    }
    void enterMenu() {
        output.enterMenu();
    }
    bool menuActive() {
        return output.menuActive();
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
