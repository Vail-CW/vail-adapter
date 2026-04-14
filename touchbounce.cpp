#include "config.h"

#ifndef NO_CAPACITIVE_TOUCH
#include "touchbounce.h"

void TouchBounce::attach(int pin, int pressThreshold, int releaseThreshold) {
  this->qt = Adafruit_FreeTouch(pin, OVERSAMPLE_2, RESISTOR_50K, FREQ_MODE_SPREAD);
  this->qt.begin();
  this->lastState = false;
  this->pressThreshold = pressThreshold;
  this->releaseThreshold = releaseThreshold;
}

bool TouchBounce::readCurrentState() {
  int val = this->qt.measure();

  if (lastState) {
    if (val < this->releaseThreshold) {
      lastState = false;
    }
  } else {
    if (val > this->pressThreshold) {
      lastState = true;
    }
  }

  return lastState;
}
#endif