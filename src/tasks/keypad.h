#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>
#include <OneButton.h>

#include "config.h"
#include "task.h"
#include "ui/events.h"
#include "tasks/api.h"

class Display;

class Keypad : public Task {
   public:
    virtual const char* taskName() const override;
    virtual void setup();
    virtual void taskRun() override;

    void keyUpClick();
    void keyDownClick();
    void keyPowerClick();

    void keyUpLongPress();
    void keyDownLongPress();
    void keyPowerLongPress();

   protected:
    OneButton keyUp;
    OneButton keyDown;
    OneButton keyPower;
    bool menuChordHandled = false;
    uint32_t lastLongpressLog = 0;
    static constexpr uint16_t longpressLogInterval = 500;

    bool menuChordPressed();
    bool menuChordInProgress();
    bool handleMenuChord();
    Api::Reply keyCommand(const char* args);
};

#endif  // KEYPAD_H
