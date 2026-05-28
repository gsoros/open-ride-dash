#include "task.h"
#include "model/state.h"
#include "pins.h"
#include "config.h"
#include "drivers/st7789.h"

extern State state;

class Display : public Task {
   public:
    virtual const char* taskName() override {
        return "Display";
    }

    virtual void setup() {
        output.setup();
        output.clear();
        output.setBrightnessPercent(5);
        Task::taskSetup();
    }

    virtual void taskRun() override {
        static uint8_t bl = 0;
        static bool blDir = true;
        if (blDir) {
            bl += 2;
            if (bl > 99) blDir = false;

        } else {
            bl -= 2;
            if (bl < 2) blDir = true;
        }
        output.setBrightnessPercent(bl);

        ulong t = millis();
        static ulong last = 0;
        if (t - last < 150) return;
        last = t;

        output.drawMajor(state.getSpeed());
        output.drawMinor1(state.getMotorPower());
        output.drawMinor2(state.getMotorPower());

        // ESP_LOGD(taskName(), "Update took %d ms, bl=%d", millis() - t, bl);

        /*
        // Fade counter
        if (output.hasBrightnessControl()) {
            for (uint8_t p = 80; p > 0; p -= 1) {
                output.setBrightnessPercent(p);
                delay(1);
            }
        }
        static uint8_t counter = 0;
        if (counter > 99) counter = 0;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", counter++);
        output.clear();
        output.drawText(buf);
        delay(100);
        if (output.hasBrightnessControl()) {
            for (uint8_t p = 0; p <= 80; p += 1) {
                output.setBrightnessPercent(p);
                delay(3);
            }
        }
        */
    }

   protected:
    ST7789 output{
        TFT_CS,
        TFT_DC,
        SPI_MOSI,
        SPI_SCK,
        TFT_RST,
        TFT_BL,
        SPI2_HOST,
        TFT_ROTATION,
        TFT_WIDTH,
        TFT_HEIGHT};
};
