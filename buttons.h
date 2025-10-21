#pragma once

// Button states detected by R2R ladder
typedef enum {
    BTN_NONE = 0,
    BTN_1,
    BTN_2,
    BTN_3,
    BTN_1_2,
    BTN_1_3,
    BTN_2_3
} ButtonState;

// Button debouncer class
// Handles: 2-reading debouncing, max state tracking, release detection, timing
class ButtonDebouncer {
private:
    ButtonState previousReading;
    ButtonState debouncedState;
    ButtonState maxStateDuringPress;
    bool isPressed;
    unsigned long pressStartTime;
    unsigned long lastPressDuration;  // Duration of the last completed press
    bool longPressNotified;  // Track if we've already notified about long press
    bool comboPressNotified;  // Track if we've already notified about combo press (0.5s)

    // Double-click detection
    unsigned long lastReleaseTime;    // Time of last button release
    ButtonState lastReleasedButton;   // Which button was last released
    bool doubleClickDetected;         // Flag to prevent multiple detections
    #define DOUBLE_CLICK_WINDOW 400   // Max time between clicks (ms)

public:
    ButtonDebouncer();

    // Update with new reading, returns true if a complete gesture detected (press & release)
    bool update(ButtonState newReading, unsigned long currentTime);

    // Get the maximum button state seen during the last press
    ButtonState getMaxState();

    // Check if current press has crossed 2-second threshold (only returns true once per press)
    bool isLongPress(unsigned long currentTime);

    // Check if current combo press has crossed 0.5-second threshold (only returns true once per press)
    bool isComboPress(unsigned long currentTime);

    // Get the duration of the last completed press (call after update() returns true)
    unsigned long getLastPressDuration();

    // Check if currently pressed
    bool isPressActive();

    // Check if a double-click was detected (only single buttons, not combos)
    bool isDoubleClick();
};

// Read analog pin and average samples
int readButtonAnalog();

// Map analog value to button state
ButtonState getButtonState(int analogValue);
