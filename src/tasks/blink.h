#include "task.h"
#include "pins.h"

// Blink the onboard LED for testing
class Blink : public Task {
   public:
    virtual const char* taskName() const override {
        return "Blink";
    }

    virtual void setup() {
        pinMode(PIN_ONBOARD_LED, OUTPUT);
    }

    virtual void taskRun() override {
        digitalWrite(PIN_ONBOARD_LED, LOW);
        delay(30);
        digitalWrite(PIN_ONBOARD_LED, HIGH);
    }
};