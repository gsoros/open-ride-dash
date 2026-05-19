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
        output.setBrightness(128);
    }

    virtual void taskRun() override {
        static uint8_t counter = 0;
        char buf[32];
        snprintf(buf, sizeof(buf), "Counter: %d", counter++);
        output.clear();
        output.drawText(buf);
    }

   protected:
    ST7789 output{-1, TFT_DC, SPI_MOSI, SPI_SCK, TFT_RST, TFT_BL};
};