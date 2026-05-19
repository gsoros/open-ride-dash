#include "task.h"
#include "pins.h"

class Blink : public Task {
   public:
    virtual const char* taskName() override {
        return "Blink";
    }

    virtual void setup() {
        pinMode(ONBOARD_LED, OUTPUT);
        Task::taskSetup();
    }

    virtual void taskRun() override {
        digitalWrite(ONBOARD_LED, LOW);
        delay(30);
        digitalWrite(ONBOARD_LED, HIGH);
    }
};