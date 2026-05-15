#include "task.h"
#include "pins.h"

class Blink : public Task {
   public:
    virtual const char* taskName() override {
        return "Blink";
    }

    virtual void setup() override {
        pinMode(LED, OUTPUT);
    }

    virtual void run() override {
        digitalWrite(LED, LOW);
        delay(30);
        digitalWrite(LED, HIGH);
    }
};