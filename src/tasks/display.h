#include "task.h"
#include "model/state.h"
#include "pins.h"
#include "drivers/st7789.h"

extern State state;

class Display : public Task {
   public:
    virtual const char* taskName() override {
        return "Display";
    }

    virtual void setup() {
        output.setup();
        output.setBrightnessPercent(50);
    }

    virtual void taskRun() override {
        // Display speed
        ulong t = millis();
        float speed = state.getSpeed();
        char buf[3];
        snprintf(buf, sizeof(buf), "%.0f", speed);
        output.clear();
        output.drawText(buf);
        ESP_LOGD(taskName(), "Update took %d ms", millis() - t);

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
    ST7789 output{TFT_CS, TFT_DC, SPI_MOSI, SPI_SCK, TFT_RST, TFT_BL};
};