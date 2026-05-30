#ifndef DISPLAY_H
#define DISPLAY_H

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
        brightnessPercent = 20;
        output.setBrightnessPercent(brightnessPercent);
        output.splash();
        Task::taskSetup();
        delay(2000);  // show splash for 2 seconds
    }

    virtual void taskRun() override {
        ulong t = millis();
        static ulong last = 0;
        if (t - last < 150) return;
        last = t;

        State::Snapshot s = state.getSnapshot();

        output.drawMajor(s.humanPower());
        output.drawMinor1((float)s.cadence);
        output.drawMinor2((float)s.pasLevel);

        // ESP_LOGD(taskName(), "Update took %d ms, bl=%d", millis() - t, bl);
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

    void keyUpClick() {
        output.keyUpClick = true;
    }
    void keyDownClick() {
        output.keyDownClick = true;
    }
    void keyPowerClick() {
        output.keyPowerClick = true;
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
    uint8_t brightnessPercent = 0;
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

#endif  // DISPLAY_H