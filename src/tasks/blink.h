#include "task.h"
#include "pins.h"

class Blink : public Task {
   public:
    virtual const char* taskName() override {
        return "Blink";
    }

    virtual void setup() {
        pinMode(LED, OUTPUT);
        Task::taskSetup();
    }

    virtual void taskRun() override {
        digitalWrite(LED, LOW);
        delay(30);
        digitalWrite(LED, HIGH);
    }
};