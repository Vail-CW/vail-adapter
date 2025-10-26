#pragma once

#include <Adafruit_FreeTouch.h>
#include "bounce2.h"

// Hysteresis thresholds for RESISTOR_50K configuration
// Relaxed for build variations and reliable squeeze detection
// Calibration: baseline ~300, squeeze ~530, single ~1010
#define QT_DIT_THRESHOLD_PRESS   450  // Well below squeeze value for reliability
#define QT_DIT_THRESHOLD_RELEASE 360  // Well above baseline, avoids false triggers
#define QT_DAH_THRESHOLD_PRESS   450  // Consistent with DIT for symmetry
#define QT_DAH_THRESHOLD_RELEASE 360  // Consistent with DIT for symmetry

class TouchBounce: public Bounce {
public:
    // attach a touch pin with thresholds
    void attach(int pin, int pressThreshold, int releaseThreshold);

protected:
    bool readCurrentState();
    Adafruit_FreeTouch qt;
    bool lastState = false;  // Track previous touch state for hysteresis
    int pressThreshold;
    int releaseThreshold;
};
