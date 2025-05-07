#include <Arduino.h>
#include <Keyboard.h>
#include <MIDIUSB.h>
#include <cstddef>
#include "keyers.h"
#include "adapter.h"
#include "polybuzzer.h"

// Do NOT include FlashStorage_SAMD.h here - include it only in the main sketch

#define MILLISECOND 1
#define SECOND (1000 * MILLISECOND)

// Duration threshold for key hold to disable buzzer (6 seconds)
#define KEY_HOLD_DISABLE_THRESHOLD 6000

// Time window for consecutive dit presses (500ms)
#define DIT_DISABLE_WINDOW 500
// Number of consecutive dit presses required to disable buzzer
#define DIT_DISABLE_COUNT 10

// Forward declaration of the save settings function from the main sketch
extern void saveSettingsToEEPROM(uint8_t keyerType, uint16_t ditDuration, uint8_t txNote);

VailAdapter::VailAdapter(unsigned int PiezoPin) {
    this->buzzer = new PolyBuzzer(PiezoPin);
    this->buzzerEnabled = true;
    this->keyPressStartTime = 0;
    this->keyIsPressed = false;
    this->lastDitTime = 0;
    this->ditPressCount = 0;
}

bool VailAdapter::KeyboardMode() {
    return this->keyboardMode;
}

uint8_t VailAdapter::getCurrentKeyerType() const {
    return getKeyerNumber(this->keyer);
}

uint16_t VailAdapter::getDitDuration() const {
    return this->ditDuration;
}

uint8_t VailAdapter::getTxNote() const {
    return this->txNote;
}

bool VailAdapter::isBuzzerEnabled() const {
    return this->buzzerEnabled;
}

void VailAdapter::ResetDitCounter() {
    this->ditPressCount = 0;
}

// Send a MIDI Key Event
void VailAdapter::midiKey(uint8_t key, bool down) {
    midiEventPacket_t event = {uint8_t(down?9:8), uint8_t(down?0x90:0x80), key, 0x7f};
    MidiUSB.sendMIDI(event);
    MidiUSB.flush();
}

// Send a keyboard key event
void VailAdapter::keyboardKey(uint8_t key, bool down) {
    if (down) {
        Keyboard.press(key);
    } else {
        Keyboard.release(key);
    }
}

// Begin transmitting
void VailAdapter::BeginTx() {
    // Track key press start time
    if (!this->keyIsPressed) {
        this->keyIsPressed = true;
        this->keyPressStartTime = millis();
    }
    
    // Only play the sidetone if buzzer is enabled
    if (this->buzzerEnabled) {
        this->buzzer->Note(0, this->txNote);
    }
    
    if (this->keyboardMode) {
        this->keyboardKey(KEY_LEFT_CTRL, true);
    } else {
        this->midiKey(0, true);
    }
}

// Stop transmitting
void VailAdapter::EndTx() {
    // Reset key press tracking
    this->keyIsPressed = false;
    this->keyPressStartTime = 0;
    
    // Always stop any tones when the key is released, regardless of buzzer enabled state
    this->buzzer->NoTone(0);
    
    if (this->keyboardMode) {
        this->keyboardKey(KEY_LEFT_CTRL, false);
    } else {
        this->midiKey(0, false);
    }
}

// Disable the buzzer with notification tones
void VailAdapter::DisableBuzzer() {
    // Stop any current tone
    this->buzzer->NoTone(0);
    
    // Play a quick descending tone pattern to indicate buzzer disabled
    this->buzzer->Note(1, 70);
    delay(100);
    this->buzzer->Note(1, 65);
    delay(100);
    this->buzzer->Note(1, 60);
    delay(100);
    this->buzzer->NoTone(1);
    
    // Now disable the buzzer
    this->buzzerEnabled = false;
}

// Handle a paddle being pressed.
//
// The caller needs to debounce keys and deal with keys wired in parallel.
void VailAdapter::HandlePaddle(Paddle paddle, bool pressed) {
    unsigned long currentTime = millis();
    
    switch (paddle) {
    case PADDLE_STRAIGHT:
        if (pressed) {
            this->BeginTx();
        } else {
            this->EndTx();
        }
        return;
        
    case PADDLE_DIT:
        // Track dit presses for buzzer disable feature
        if (pressed && this->buzzerEnabled) {
            // Check if this press is within the time window
            if (currentTime - this->lastDitTime < DIT_DISABLE_WINDOW) {
                this->ditPressCount++;
                
                // If we've reached the threshold, disable the buzzer
                if (this->ditPressCount >= DIT_DISABLE_COUNT) {
                    this->DisableBuzzer();
                }
            } else {
                // If too much time has passed, reset the counter
                this->ditPressCount = 1;
            }
            
            // Update the timestamp
            this->lastDitTime = currentTime;
        }
        
        // Handle the dit paddle normally
        if (this->keyer) {
            this->keyer->Key(paddle, pressed);
        } else if (this->keyboardMode) {
            this->keyboardKey(KEY_LEFT_CTRL, pressed);
        } else {
            this->midiKey(1, pressed);
        }
        break;
        
    case PADDLE_DAH:
        // Reset dit counter if dah paddle is used
        if (pressed) {
            this->ResetDitCounter();
        }
        
        if (this->keyer) {
            this->keyer->Key(paddle, pressed);
        } else if (this->keyboardMode) {
            this->keyboardKey(KEY_RIGHT_CTRL, pressed);
        } else {
            this->midiKey(2, pressed);
        }
        break;
    }
}

// Handle a MIDI event.
void VailAdapter::HandleMIDI(midiEventPacket_t event) {
    uint16_t msg = (event.byte1 << 8) | (event.byte2 << 0);
    
    switch (event.byte1) {
    case 0xB0: // Controller Change
        switch (event.byte2) {
            case 0: // turn keyboard mode on/off
                this->keyboardMode = (event.byte3 > 0x3f);
                MidiUSB.sendMIDI(event); // Send it back to acknowledge
                break;
                
            case 1: // set dit duration (0-254) *2ms
                this->ditDuration = event.byte3 * 2 * MILLISECOND;
                if (this->keyer) {
                    this->keyer->SetDitDuration(this->ditDuration);
                }
                // Save settings to EEPROM
                saveSettingsToEEPROM(getCurrentKeyerType(), this->ditDuration, this->txNote);
                break;
                
            case 2: // set tx note
                this->txNote = event.byte3;
                
                // Test the new tone by playing it briefly, only if buzzer is enabled
                if (this->buzzerEnabled) {
                    this->buzzer->Note(1, this->txNote);
                    delay(100);
                    this->buzzer->NoTone(1);
                }
                
                // Save settings to EEPROM
                saveSettingsToEEPROM(getCurrentKeyerType(), this->ditDuration, this->txNote);
                break;
                
            default:
                break;
        }
        break;
        
    case 0xC0: // Program Change
        if (this->keyer) {
            this->keyer->Release();
        }
        this->keyer = GetKeyerByNumber(event.byte2, this);
        this->keyer->SetDitDuration(this->ditDuration);
        
        // Save settings to EEPROM
        saveSettingsToEEPROM(event.byte2, this->ditDuration, this->txNote);
        break;
        
    case 0x80: // Note off
        if (this->buzzerEnabled) {
            this->buzzer->NoTone(1);
        }
        break;
        
    case 0x90: // Note on
        if (this->buzzerEnabled) {
            this->buzzer->Note(1, event.byte2);
        }
        break;
    }
}

void VailAdapter::Tick(unsigned int currentMillis) {
    // Check for key hold duration to disable buzzer (for straight key)
    if (this->keyIsPressed && this->buzzerEnabled && this->keyPressStartTime > 0) {
        // Calculate how long the key has been held
        unsigned long holdDuration = currentMillis - this->keyPressStartTime;
        
        // If key held long enough, disable the buzzer
        if (holdDuration >= KEY_HOLD_DISABLE_THRESHOLD) {
            this->DisableBuzzer();
            
            // Reset timer to prevent repeated triggers
            this->keyPressStartTime = 0;
            
            // If key is still down, restart transmission if needed
            if (this->keyboardMode) {
                this->keyboardKey(KEY_LEFT_CTRL, true);
            } else {
                this->midiKey(0, true);
            }
        }
    }
    
    if (this->keyer) {
        this->keyer->Tick(currentMillis);
    }
}
