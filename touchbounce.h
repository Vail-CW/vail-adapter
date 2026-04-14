#pragma once
#include "config.h"

#ifndef NO_CAPACITIVE_TOUCH

#include <Adafruit_FreeTouch.h>
#include "bounce2.h"

#define QT_DIT_THRESHOLD_PRESS 450
#define QT_DIT_THRESHOLD_RELEASE 360
#define QT_DAH_THRESHOLD_PRESS 450
#define QT_DAH_THRESHOLD_RELEASE 360

class TouchBounce : public Bounce {
public:
  void attach(int pin, int pressThreshold, int releaseThreshold);

protected:
  bool readCurrentState();
  Adafruit_FreeTouch qt;
  bool lastState = false;
  int pressThreshold;
  int releaseThreshold;
};

#endif