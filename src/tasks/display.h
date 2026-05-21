#include "task.h"
#include "pins.h"
#include "drivers/st7789.h"

class Display : public Task {
   public:
    virtual const char* taskName() override {
        return "Display";
    }

    virtual void setup() {
        output.setup();
        output.setBrightnessPercent(100);
    }

    virtual void taskRun() override {
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
    }

   protected:
    ST7789 output{TFT_CS, TFT_DC, SPI_MOSI, SPI_SCK, TFT_RST, TFT_BL};
};