#include "touchbounce.h"

void TouchBounce::attach(int pin, int pressThreshold, int releaseThreshold) {
    // Use RESISTOR_50K to prevent capacitive crosstalk during squeeze keying
    this->qt = Adafruit_FreeTouch(pin, OVERSAMPLE_2, RESISTOR_50K, FREQ_MODE_SPREAD);
    this->qt.begin();
    this->lastState = false;
    this->pressThreshold = pressThreshold;
    this->releaseThreshold = releaseThreshold;
}

bool TouchBounce::readCurrentState() {
    int val = this->qt.measure();

    // Hysteresis: use different thresholds for press vs release
    // This prevents crosstalk during squeeze keying from causing spurious releases
    if (lastState) {
        // Currently pressed - use lower threshold to release
        if (val < this->releaseThreshold) {
            lastState = false;
        }
    } else {
        // Currently not pressed - use higher threshold to press
        if (val > this->pressThreshold) {
            lastState = true;
        }
    }

    return lastState;
}
