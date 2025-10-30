#include <Arduino.h>
#include "buttons.h"
#include "config.h"

// Number of samples to average for noise reduction
#define BUTTON_SAMPLE_COUNT 10

// ButtonDebouncer implementation
ButtonDebouncer::ButtonDebouncer() {
    previousReading = BTN_NONE;
    debouncedState = BTN_NONE;
    maxStateDuringPress = BTN_NONE;
    isPressed = false;
    pressStartTime = 0;
    lastPressDuration = 0;
    longPressNotified = false;
    comboPressNotified = false;
    midiSwitchNotified = false;
    lastReleaseTime = 0;
    lastReleasedButton = BTN_NONE;
    doubleClickDetected = false;
}

bool ButtonDebouncer::update(ButtonState newReading, unsigned long currentTime) {
    // Require 2 consistent readings for debouncing
    if (newReading == previousReading) {
        debouncedState = newReading;

        // Track if we're in a press
        if (debouncedState != BTN_NONE && !isPressed) {
            // Button press started
            isPressed = true;
            maxStateDuringPress = debouncedState;
            pressStartTime = currentTime;
            longPressNotified = false;
            comboPressNotified = false;
            midiSwitchNotified = false;
        }
        else if (isPressed && debouncedState != BTN_NONE) {
            // Update max state if current state is "higher" (more buttons pressed)
            if (debouncedState > maxStateDuringPress) {
                maxStateDuringPress = debouncedState;
            }
        }
        else if (isPressed && debouncedState == BTN_NONE) {
            // Button released - complete gesture detected!
            // Save the duration BEFORE clearing isPressed
            lastPressDuration = currentTime - pressStartTime;

            // Double-click detection (only for single buttons, not combos)
            if (maxStateDuringPress == BTN_1 || maxStateDuringPress == BTN_2 || maxStateDuringPress == BTN_3) {
                // Check if this is the same button released within the double-click window
                if (maxStateDuringPress == lastReleasedButton &&
                    (currentTime - lastReleaseTime) <= DOUBLE_CLICK_WINDOW) {
                    doubleClickDetected = true;
                }

                // Update tracking for next potential double-click
                lastReleasedButton = maxStateDuringPress;
                lastReleaseTime = currentTime;
            }

            isPressed = false;
            previousReading = newReading;
            return true;  // Gesture complete
        }
    }

    previousReading = newReading;
    return false;  // No complete gesture yet
}

ButtonState ButtonDebouncer::getMaxState() {
    return maxStateDuringPress;
}

bool ButtonDebouncer::isLongPress(unsigned long currentTime) {
    if (!isPressed || longPressNotified) {
        return false;  // Not pressed or already notified
    }

    if ((currentTime - pressStartTime) >= 2000) {
        longPressNotified = true;
        return true;  // Crossed 2-second threshold
    }

    return false;
}

bool ButtonDebouncer::isComboPress(unsigned long currentTime) {
    if (!isPressed || comboPressNotified) {
        return false;  // Not pressed or already notified
    }

    // Only trigger for actual combo button states (not single buttons)
    if (maxStateDuringPress != BTN_1_2 &&
        maxStateDuringPress != BTN_1_3 &&
        maxStateDuringPress != BTN_2_3) {
        return false;  // Not a combo press
    }

    if ((currentTime - pressStartTime) >= 500) {
        comboPressNotified = true;
        return true;  // Crossed 0.5-second threshold
    }

    return false;
}

bool ButtonDebouncer::isMidiSwitchPress(unsigned long currentTime) {
    if (!isPressed || midiSwitchNotified) {
        return false;  // Not pressed or already notified
    }

    // Only trigger for B1+B2 combo
    if (maxStateDuringPress != BTN_1_2) {
        return false;  // Not the right combo
    }

    if ((currentTime - pressStartTime) >= 3000) {
        midiSwitchNotified = true;
        return true;  // Crossed 3-second threshold
    }

    return false;
}

unsigned long ButtonDebouncer::getLastPressDuration() {
    return lastPressDuration;
}

bool ButtonDebouncer::isPressActive() {
    return isPressed;
}

bool ButtonDebouncer::isDoubleClick() {
    if (doubleClickDetected) {
        doubleClickDetected = false;  // Clear flag after reading
        return true;
    }
    return false;
}

// Read analog pin and average 10 samples
int readButtonAnalog() {
    long sum = 0;
    for (int i = 0; i < BUTTON_SAMPLE_COUNT; i++) {
        sum += analogRead(BUTTON_PIN);
    }
    return sum / BUTTON_SAMPLE_COUNT;
}

// Map analog value to button state based on calibrated thresholds
// Hardware-specific calibration values
#ifdef V2_Basic_PCB
// V2 Basic PCB calibration: NONE=1, B3=512, B2=614, B1=683, B2+3=769, B1+3=820, B1+2=830
ButtonState getButtonState(int analogValue) {
    // None: 0-100
    if (analogValue >= 0 && analogValue <= 100) {
        return BTN_NONE;
    }
    // Button 3: 490-534 (measured: 512)
    else if (analogValue >= 490 && analogValue <= 534) {
        return BTN_3;
    }
    // Button 2: 592-636 (measured: 614)
    else if (analogValue >= 592 && analogValue <= 636) {
        return BTN_2;
    }
    // Buttons 2+3: 747-791 (measured: 769)
    else if (analogValue >= 747 && analogValue <= 791) {
        return BTN_2_3;
    }
    // Button 1: 661-705 (measured: 683)
    else if (analogValue >= 661 && analogValue <= 705) {
        return BTN_1;
    }
    // Buttons 1+3: 798-842 (measured: 820)
    else if (analogValue >= 798 && analogValue <= 842) {
        return BTN_1_3;
    }
    // Buttons 1+2: 808-900 (measured: 830)
    else if (analogValue >= 808 && analogValue <= 900) {
        return BTN_1_2;
    }

    // If reading falls outside all ranges, return BTN_NONE as safe default
    return BTN_NONE;
}
#else
// Advanced PCB calibration: NONE=0, B3=280, B2=335, B1=414, B2+3=397, B1+3=490, B1+2=516
ButtonState getButtonState(int analogValue) {
    // None: 0-100
    if (analogValue >= 0 && analogValue <= 100) {
        return BTN_NONE;
    }
    // Button 3: 260-300 (measured: 280)
    else if (analogValue >= 260 && analogValue <= 300) {
        return BTN_3;
    }
    // Button 2: 315-355 (measured: 335)
    else if (analogValue >= 315 && analogValue <= 355) {
        return BTN_2;
    }
    // Buttons 2+3: 377-405 (measured: 397)
    else if (analogValue >= 377 && analogValue <= 405) {
        return BTN_2_3;
    }
    // Button 1: 406-435 (measured: 414)
    else if (analogValue >= 406 && analogValue <= 435) {
        return BTN_1;
    }
    // Buttons 1+3: 470-502 (measured: 490)
    else if (analogValue >= 470 && analogValue <= 502) {
        return BTN_1_3;
    }
    // Buttons 1+2: 503-650 (measured: 516)
    else if (analogValue >= 503 && analogValue <= 650) {
        return BTN_1_2;
    }

    // If reading falls outside all ranges, return BTN_NONE as safe default
    return BTN_NONE;
}
#endif
