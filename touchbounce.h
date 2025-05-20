#pragma once

#include <Adafruit_FreeTouch.h>
#include "bounce2.h"

#define QT_THRESHOLD 310

class TouchBounce: public Bounce {
public:
    // attach a touch pin
    void attach(int pin);

protected:
    bool readCurrentState();
    Adafruit_FreeTouch qt;
};
