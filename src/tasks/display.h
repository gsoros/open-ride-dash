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

        State::Snapshot s = state.getSnapshot();

        output.drawMajor(s.speed());
        output.drawMinor1(s.humanPower());
        output.drawMinor2(s.motorPower());

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

/*

enum MetricID {
    METRIC_SPEED,
    METRIC_CADENCE,
    METRIC_PAS,
    METRIC_MOTOR_PWR,
    METRIC_HUMAN_PWR,
    METRIC_VOLTAGE,
    METRIC_SOC,
    METRIC_RANGE,
    METRIC_HEART_RATE,
    METRIC_BODY_TEMP,
    METRIC_COUNT // Keeps track of total metrics
};

// Global struct to hold live data
struct TelemetryData {
    float values[METRIC_COUNT];
    const char* units[METRIC_COUNT];
};
TelemetryData liveData = {
    .units = {"km/h", "rpm", "PAS", "W", "W", "V", "%", "km", "bpm", "°C"}
};

// Define what goes on each page
struct PageLayout {
    MetricID major;
    MetricID minor1;
    MetricID minor2;
};

PageLayout pages[] = {
    { METRIC_SPEED,     METRIC_PAS,       METRIC_SOC },       // Page 1: Standard Cruise
    { METRIC_HUMAN_PWR, METRIC_MOTOR_PWR, METRIC_CADENCE },   // Page 2: Power & Performance
    { METRIC_SPEED,     METRIC_HEART_RATE,METRIC_RANGE }       // Page 3: Fitness & Range
};
uint8_t currentPage = 0;
uint8_t totalPages = sizeof(pages) / sizeof(PageLayout);


*/