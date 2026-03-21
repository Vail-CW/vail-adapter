#include <Arduino.h>
#include <Keyboard.h>
#include <MIDIUSB.h>
#include <cstddef>
#include "keyers.h"
#include "adapter.h"
#include "polybuzzer.h"

// For SAMD21 software reset if needed by other parts of code
#if defined(ARDUINO_ARCH_SAMD)
// NVIC_SystemReset() is typically available through Arduino.h / CMSIS includes
#endif

extern void saveSettingsToEEPROM(uint8_t keyerType, uint16_t ditDuration, uint8_t txNote);

VailAdapter::VailAdapter(unsigned int PiezoPin) {
this->buzzer = new PolyBuzzer(PiezoPin);
this->buzzerEnabled = true;
this->radioModeActive = false;
this->radioKeyerMode = false;
this->keyIsPressed = false;
this->keyPressStartTime = 0;
this->ditHoldStartTime = 0;
this->ditIsHeld = false;
this->dahHoldStartTime = 0;
this->dahIsHeld = false;
this->lastCapDahTime = 0;
this->capDahPressCount = 0;
this->radioDitState = false;
this->radioDahState = false;
this->keyboardMode = true;
this->keyer = NULL;
this->txNote = DEFAULT_TONE_NOTE;
this->ditDuration = DEFAULT_ADAPTER_DIT_DURATION_MS;
this->txRelays[0] = false; // dit
this->txRelays[1] = false; // dah
this->lastPaddlePressed = PADDLE_DIT;
this->ditKeyPressed = false;
this->dahKeyPressed = false;
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

void VailAdapter::setRecordingState(RecordingState* state) {
    this->recordingState = state;
}

bool VailAdapter::isBuzzerEnabled() const {
return this->buzzerEnabled;
}

bool VailAdapter::isRadioModeActive() const {
return this->radioModeActive;
}

bool VailAdapter::isRadioKeyerMode() const {
return this->radioKeyerMode;
}

void VailAdapter::SetRadioKeyerMode(bool enabled) {
this->radioKeyerMode = enabled;
}

void VailAdapter::ResetDitCounter() {
this->ditIsHeld = false;
this->ditHoldStartTime = 0;
}

void VailAdapter::ResetDahCounter() {
this->capDahPressCount = 0;
}

void VailAdapter::ResetDahHoldCounter() {
this->dahIsHeld = false;
this->dahHoldStartTime = 0;
}

// Corrected MIDI key event function
void VailAdapter::midiKey(uint8_t key, bool down) {
uint8_t header;
uint8_t status_byte;
uint8_t velocity;

if (down) { // Note On
    header = 0x09;      // CIN = 9 (Note On) for USB MIDI event packet
    status_byte = 0x90; // MIDI Status = 0x90 (Note On, Channel 1)
    velocity = 0x7F;    // Standard velocity for Note On
} else { // Note Off
    header = 0x08;      // CIN = 8 (Note Off) for USB MIDI event packet
    status_byte = 0x80; // MIDI Status = 0x80 (Note Off, Channel 1)
    velocity = 0x00;    // Velocity for Note Off (0x00 is common)
}
// Construct the MIDI event packet for MIDIUSB library
midiEventPacket_t event = {header, status_byte, key, velocity};
MidiUSB.sendMIDI(event);
MidiUSB.flush();
}

void VailAdapter::keyboardKey(uint8_t key, bool down) {
if (down) {
Keyboard.press(key);
// Track which keys we've pressed
if (key == DIT_KEYBOARD_KEY) this->ditKeyPressed = true;
if (key == DAH_KEYBOARD_KEY) this->dahKeyPressed = true;
} else {
Keyboard.release(key);
// Track which keys we've released
if (key == DIT_KEYBOARD_KEY) this->ditKeyPressed = false;
if (key == DAH_KEYBOARD_KEY) this->dahKeyPressed = false;
}
}

void VailAdapter::ReleaseAllKeys() {
// Release all keyboard keys that might be stuck
if (this->keyboardMode) {
if (this->ditKeyPressed) {
Keyboard.release(DIT_KEYBOARD_KEY);
this->ditKeyPressed = false;
}
if (this->dahKeyPressed) {
Keyboard.release(DAH_KEYBOARD_KEY);
this->dahKeyPressed = false;
}
// Also send release for both keys as a safety measure
Keyboard.release(DIT_KEYBOARD_KEY);
Keyboard.release(DAH_KEYBOARD_KEY);
}
// Release MIDI notes if in MIDI mode
if (!this->keyboardMode) {
this->midiKey(0, false);
this->midiKey(1, false);
this->midiKey(2, false);
}
// Reset state tracking
this->ditKeyPressed = false;
this->dahKeyPressed = false;
this->keyIsPressed = false;
this->txRelays[0] = false;
this->txRelays[1] = false;
Serial.println("All keys released");
}

#ifdef HAS_RADIO_OUTPUT
void VailAdapter::setRadioDit(bool active) {
digitalWrite(RADIO_DIT_PIN, active ? RADIO_ACTIVE_LEVEL : RADIO_INACTIVE_LEVEL);
}

void VailAdapter::setRadioDah(bool active) {
digitalWrite(RADIO_DAH_PIN, active ? RADIO_ACTIVE_LEVEL : RADIO_INACTIVE_LEVEL);
}
#else
void VailAdapter::setRadioDit(bool active) {(void)active;}
void VailAdapter::setRadioDah(bool active) {(void)active;}
#endif

void VailAdapter::Tx(int relay, bool closed) {
    // Update relay state
    this->txRelays[relay] = closed;

    // Check if any relay is active
    bool anyRelayActive = this->txRelays[0] || this->txRelays[1];

    if (anyRelayActive && !keyIsPressed) {
        // Start transmission
        keyIsPressed = true;
        if (!this->radioModeActive) {
            if (this->keyPressStartTime == 0) {
                this->keyPressStartTime = millis();
            }
        }

        // Record key event if recording is active - CHECK THIS FIRST before any output
        if (recordingState != nullptr && recordingState->isRecording) {
            // Map relay to paddle: relay 0 = DIT, relay 1 = DAH
            uint8_t paddle = (relay == 0) ? PADDLE_DIT_FLAG : PADDLE_DAH_FLAG;
            recordKeyEvent(*recordingState, true, paddle);  // Key down
            // During recording: always play sidetone for feedback, even in radio mode
            this->buzzer->Note(0, this->txNote);
            return;  // Skip ALL output (radio, MIDI, keyboard) during recording
        }

        if (this->buzzerEnabled && !this->radioModeActive) {
            this->buzzer->Note(0, this->txNote);
        }

#ifdef HAS_RADIO_OUTPUT
        if (this->radioModeActive) {
            // In radio mode, output to hardware pins
            if (this->radioKeyerMode) {
                // Radio Keyer Mode: All keying on DIT pin only
                radioDitState = true;
                setRadioDit(true);
            } else {
                // Normal Radio Mode: Route to appropriate pin
                if (relay == PADDLE_DIT) {
                    radioDitState = true;
                    setRadioDit(true);
                } else if (relay == PADDLE_DAH) {
                    radioDahState = true;
                    setRadioDah(true);
                }
            }
            return;  // Skip keyboard/MIDI output in radio mode
        }
#endif

        if (!this->radioModeActive) {
            // Send the appropriate key based on which relay is active
            if (this->keyboardMode) {
                if (relay == PADDLE_DIT) {
                    this->keyboardKey(DIT_KEYBOARD_KEY, true);
                } else if (relay == PADDLE_DAH) {
                    this->keyboardKey(DAH_KEYBOARD_KEY, true);
                }
            } else {
                if (relay == PADDLE_DIT) {
                    this->midiKey(1, true);
                } else if (relay == PADDLE_DAH) {
                    this->midiKey(2, true);
                }
            }
        }
    } else if (!anyRelayActive && keyIsPressed) {
        // End transmission
        keyIsPressed = false;
        if (!this->radioModeActive) {
            this->keyPressStartTime = 0;
        }

        // Record key event if recording is active
        if (recordingState != nullptr && recordingState->isRecording) {
            // Map relay to paddle: relay 0 = DIT, relay 1 = DAH
            uint8_t paddle = (relay == 0) ? PADDLE_DIT_FLAG : PADDLE_DAH_FLAG;
            recordKeyEvent(*recordingState, false, paddle);  // Key up
            // During recording: only stop sidetone, don't send MIDI/keyboard/radio output
            this->buzzer->NoTone(0);
            return;  // Skip normal output during recording
        }

        this->buzzer->NoTone(0);

#ifdef HAS_RADIO_OUTPUT
        if (this->radioModeActive) {
            // In radio mode, release hardware pins
            if (this->radioKeyerMode) {
                // Radio Keyer Mode: Release DIT pin
                radioDitState = false;
                setRadioDit(false);
            } else {
                // Normal Radio Mode: Release appropriate pin
                if (relay == PADDLE_DIT) {
                    radioDitState = false;
                    setRadioDit(false);
                } else if (relay == PADDLE_DAH) {
                    radioDahState = false;
                    setRadioDah(false);
                }
            }
            return;  // Skip keyboard/MIDI output in radio mode
        }
#endif

        if (!this->radioModeActive) {
            // Release only the keys that were pressed
            if (this->keyboardMode) {
                if (this->ditKeyPressed) this->keyboardKey(DIT_KEYBOARD_KEY, false);
                if (this->dahKeyPressed) this->keyboardKey(DAH_KEYBOARD_KEY, false);
            } else {
                // In MIDI mode, release the notes that were sent
                if (this->txRelays[PADDLE_DIT]) this->midiKey(1, false);
                if (this->txRelays[PADDLE_DAH]) this->midiKey(2, false);
            }
        }
    }
}

void VailAdapter::BeginTx() {
if (!keyIsPressed) {
keyIsPressed = true;
if (!this->radioModeActive) {
if (this->keyPressStartTime == 0) {
this->keyPressStartTime = millis();
}
}
}

// Record key event if recording is active
if (recordingState != nullptr && recordingState->isRecording) {
    // No relay info available, default to DIT paddle
    recordKeyEvent(*recordingState, true, PADDLE_DIT_FLAG);  // Key down
    // During recording: always play sidetone for feedback, even in radio mode
    // Don't send MIDI/keyboard/radio output
    this->buzzer->Note(0, this->txNote);
    return;  // Skip normal output during recording
}

if (this->buzzerEnabled && !this->radioModeActive) {
    this->buzzer->Note(0, this->txNote);
}

// Handle output based on current mode
#ifdef HAS_RADIO_OUTPUT
if (this->radioModeActive) {
    // Memory playback in radio mode: key DIT pin only (like straight key)
    radioDitState = true;
    setRadioDit(true);
    return;  // Skip keyboard/MIDI output in radio mode
}
#endif

if (!this->radioModeActive) {
    if (this->keyboardMode) {
        // For keyer mode, we need to determine which key to send
        // Since keyers don't tell us which paddle, we'll default to left ctrl
        // This is the problem - keyers always call BeginTx() without relay info
        this->keyboardKey(KEY_LEFT_CTRL, true);
    } else {
        this->midiKey(0, true);
    }
}
}

void VailAdapter::EndTx() {
if (keyIsPressed) {
keyIsPressed = false;
if (!this->radioModeActive) {
this->keyPressStartTime = 0;
}
}

// Record key event if recording is active
if (recordingState != nullptr && recordingState->isRecording) {
    // No relay info available, default to DIT paddle
    recordKeyEvent(*recordingState, false, PADDLE_DIT_FLAG);  // Key up
    // During recording: only stop sidetone, don't send MIDI/keyboard/radio output
    this->buzzer->NoTone(0);
    return;  // Skip normal output during recording
}

this->buzzer->NoTone(0);

// Handle output based on current mode
#ifdef HAS_RADIO_OUTPUT
if (this->radioModeActive) {
    // Memory playback in radio mode: release DIT pin
    radioDitState = false;
    setRadioDit(false);
    return;  // Skip keyboard/MIDI output in radio mode
}
#endif

if (!this->radioModeActive) {
    if (this->keyboardMode) {
        this->keyboardKey(KEY_LEFT_CTRL, false);
    } else {
        this->midiKey(0, false);
    }
}
}

void VailAdapter::BeginTx(int relay) {
if (!keyIsPressed) {
keyIsPressed = true;
if (!this->radioModeActive) {
if (this->keyPressStartTime == 0) {
this->keyPressStartTime = millis();
}
}
}

// Record key event if recording is active - CHECK THIS FIRST before any output
if (recordingState != nullptr && recordingState->isRecording) {
    // Map relay to paddle: relay 0 = DIT, relay 1 = DAH
    uint8_t paddle = (relay == PADDLE_DIT) ? PADDLE_DIT_FLAG : PADDLE_DAH_FLAG;
    recordKeyEvent(*recordingState, true, paddle);  // Key down
    // During recording: always play sidetone for feedback, even in radio mode
    // Don't send MIDI/keyboard/radio output
    this->buzzer->Note(0, this->txNote);
    return;  // Skip ALL output (radio, MIDI, keyboard) during recording
}

if (this->buzzerEnabled && !this->radioModeActive) {
    this->buzzer->Note(0, this->txNote);
}

#ifdef HAS_RADIO_OUTPUT
if (this->radioModeActive) {
    // In radio mode, output to hardware pins
    if (this->radioKeyerMode) {
        // Radio Keyer Mode: All keying on DIT pin only
        Serial.println("BeginTx: Radio Keyer Mode - Setting DIT pin ACTIVE");
        radioDitState = true;
        setRadioDit(true);
    } else {
        // Normal Radio Mode: Route to appropriate pin
        if (relay == PADDLE_DIT) {
            Serial.println("BeginTx: Radio Mode - Setting DIT pin ACTIVE (paddle=DIT)");
            radioDitState = true;
            setRadioDit(true);
        } else if (relay == PADDLE_DAH) {
            Serial.println("BeginTx: Radio Mode - Setting DAH pin ACTIVE (paddle=DAH)");
            radioDahState = true;
            setRadioDah(true);
        } else {
            Serial.println("BeginTx: Radio Mode - Setting DIT pin ACTIVE (straight key fallback)");
            radioDitState = true;
            setRadioDit(true); // straight key on DIT
        }
    }
    return;
}
#endif

if (!this->radioModeActive) {
    if (this->keyboardMode) {
        if (relay == PADDLE_DIT) {
            this->keyboardKey(DIT_KEYBOARD_KEY, true);
        } else if (relay == PADDLE_DAH) {
            this->keyboardKey(DAH_KEYBOARD_KEY, true);
        } else {
            this->keyboardKey(KEY_LEFT_CTRL, true); // fallback for straight key
        }
    } else {
        if (relay == PADDLE_DIT) {
            this->midiKey(1, true);
        } else if (relay == PADDLE_DAH) {
            this->midiKey(2, true);
        } else {
            this->midiKey(0, true); // fallback for straight key
        }
    }
}
}

void VailAdapter::EndTx(int relay) {
if (keyIsPressed) {
keyIsPressed = false;
if (!this->radioModeActive) {
this->keyPressStartTime = 0;
}
}

// Record key event if recording is active
if (recordingState != nullptr && recordingState->isRecording) {
    // Map relay to paddle: relay 0 = DIT, relay 1 = DAH
    uint8_t paddle = (relay == PADDLE_DIT) ? PADDLE_DIT_FLAG : PADDLE_DAH_FLAG;
    recordKeyEvent(*recordingState, false, paddle);  // Key up
    // During recording: only stop sidetone, don't send MIDI/keyboard/radio output
    this->buzzer->NoTone(0);
    return;  // Skip normal output during recording
}

this->buzzer->NoTone(0);

#ifdef HAS_RADIO_OUTPUT
if (this->radioModeActive) {
    // In radio mode, output to hardware pins
    if (this->radioKeyerMode) {
        // Radio Keyer Mode: All keying on DIT pin only
        Serial.println("EndTx: Radio Keyer Mode - Setting DIT pin INACTIVE");
        radioDitState = false;
        setRadioDit(false);
    } else {
        // Normal Radio Mode: Route to appropriate pin
        if (relay == PADDLE_DIT) {
            Serial.println("EndTx: Radio Mode - Setting DIT pin INACTIVE (paddle=DIT)");
            radioDitState = false;
            setRadioDit(false);
        } else if (relay == PADDLE_DAH) {
            Serial.println("EndTx: Radio Mode - Setting DAH pin INACTIVE (paddle=DAH)");
            radioDahState = false;
            setRadioDah(false);
        } else {
            Serial.println("EndTx: Radio Mode - Setting DIT pin INACTIVE (straight key fallback)");
            radioDitState = false;
            setRadioDit(false); // straight key on DIT
        }
    }
    return;
}
#endif

if (!this->radioModeActive) {
    if (this->keyboardMode) {
        if (relay == PADDLE_DIT) {
            this->keyboardKey(DIT_KEYBOARD_KEY, false);
        } else if (relay == PADDLE_DAH) {
            this->keyboardKey(DAH_KEYBOARD_KEY, false);
        } else {
            this->keyboardKey(KEY_LEFT_CTRL, false); // fallback for straight key
        }
    } else {
        if (relay == PADDLE_DIT) {
            this->midiKey(1, false);
        } else if (relay == PADDLE_DAH) {
            this->midiKey(2, false);
        } else {
            this->midiKey(0, false); // fallback for straight key
        }
    }
}
}

void VailAdapter::DisableBuzzer() {
this->buzzer->NoTone(0);
this->buzzer->Note(1, 70); delay(100);
this->buzzer->Note(1, 65); delay(100);
this->buzzer->Note(1, 60); delay(100);
this->buzzer->NoTone(1);
this->buzzerEnabled = false;
Serial.println("Buzzer Disabled");
}

void VailAdapter::ToggleRadioMode() {
#ifdef HAS_RADIO_OUTPUT
this->radioModeActive = !this->radioModeActive;

if (keyer) keyer->Release();
if (keyIsPressed) EndTx();

// Ensure all keyboard keys are released
ReleaseAllKeys();

setRadioDit(false);
setRadioDah(false);
radioDitState = false;
radioDahState = false;
keyIsPressed = false;

// Restore the keyer's dit duration after releasing
if (this->keyer) {
    this->keyer->SetDitDuration(this->ditDuration);
    Serial.print("Keyer dit duration restored to: "); Serial.println(this->ditDuration);
}

if (this->radioModeActive) {
    Serial.println("Radio Mode Activated (Sidetone Disabled)");
    this->buzzer->NoTone(0);
    this->buzzer->Note(1, 60); delay(100);
    this->buzzer->Note(1, 65); delay(100);
    this->buzzer->Note(1, 70); delay(100);
    this->buzzer->NoTone(1);
} else {
    Serial.println("Radio Mode Deactivated. Resetting controller...");
    this->buzzer->Note(1, 70); delay(100);
    this->buzzer->Note(1, 65); delay(100);
    this->buzzer->Note(1, 60); delay(100);
    this->buzzer->NoTone(1);
    delay(100);

    NVIC_SystemReset();
}
#else
Serial.println("Radio output not configured. Radio mode unavailable.");
this->buzzer->Tone(1, 100); delay(200); this->buzzer->NoTone(1);
#endif
}

void VailAdapter::ToggleRadioKeyerMode() {
#ifdef HAS_RADIO_OUTPUT
if (!this->radioModeActive) {
    Serial.println("Cannot toggle Radio Keyer Mode: Not in Radio Mode");
    return;
}

this->radioKeyerMode = !this->radioKeyerMode;

if (keyer) keyer->Release();
if (keyIsPressed) EndTx();

// Ensure all keyboard keys are released
ReleaseAllKeys();

setRadioDit(false);
setRadioDah(false);
radioDitState = false;
radioDahState = false;
keyIsPressed = false;

// Restore the keyer's dit duration after releasing
if (this->keyer) {
    this->keyer->SetDitDuration(this->ditDuration);
    Serial.print("Keyer dit duration restored to: "); Serial.println(this->ditDuration);
}

extern void saveSettingsToEEPROM(uint8_t keyerType, uint16_t ditDuration, uint8_t txNote);
extern void saveRadioKeyerModeToEEPROM(bool radioKeyerMode);
saveRadioKeyerModeToEEPROM(this->radioKeyerMode);

if (this->radioKeyerMode) {
    Serial.println("Radio Keyer Mode Activated - Keyer output on DIT pin only");
    // Play "RK" in morse: R = .-. K = -.-
    // R: dit-dah-dit
    this->buzzer->Note(1, this->txNote); delay(60);
    this->buzzer->NoTone(1); delay(60);
    this->buzzer->Note(1, this->txNote); delay(180);
    this->buzzer->NoTone(1); delay(60);
    this->buzzer->Note(1, this->txNote); delay(60);
    this->buzzer->NoTone(1); delay(180); // char space
    // K: dah-dit-dah
    this->buzzer->Note(1, this->txNote); delay(180);
    this->buzzer->NoTone(1); delay(60);
    this->buzzer->Note(1, this->txNote); delay(60);
    this->buzzer->NoTone(1); delay(60);
    this->buzzer->Note(1, this->txNote); delay(180);
    this->buzzer->NoTone(1);
} else {
    Serial.println("Radio Keyer Mode Deactivated - Back to normal Radio Mode");
    // Play "R" in morse: R = .-.
    // R: dit-dah-dit
    this->buzzer->Note(1, this->txNote); delay(60);
    this->buzzer->NoTone(1); delay(60);
    this->buzzer->Note(1, this->txNote); delay(180);
    this->buzzer->NoTone(1); delay(60);
    this->buzzer->Note(1, this->txNote); delay(60);
    this->buzzer->NoTone(1);
}
#else
Serial.println("Radio output not configured. Radio Keyer mode unavailable.");
#endif
}

void VailAdapter::ProcessPaddleInput(Paddle paddle, bool pressed, bool isCapacitive) {
unsigned long currentTime = millis();

// Track dit paddle state for hold detection
if (paddle == PADDLE_DIT) {
    if (pressed && !this->ditIsHeld) {
        // Dit just pressed - start timer
        this->ditHoldStartTime = currentTime;
        this->ditIsHeld = true;
        Serial.println("Dit hold started");
    } else if (!pressed && this->ditIsHeld) {
        // Dit released - reset timer
        unsigned long holdTime = currentTime - this->ditHoldStartTime;
        Serial.print("Dit released after ");
        Serial.print(holdTime);
        Serial.println("ms");
        this->ditIsHeld = false;
    }
}

// Track dah paddle state for hold detection in radio mode
if (paddle == PADDLE_DAH && isCapacitive && this->radioModeActive) {
    if (pressed && !this->dahIsHeld) {
        // Dah just pressed in radio mode - start timer
        this->dahHoldStartTime = currentTime;
        this->dahIsHeld = true;
        Serial.println("Dah hold started (Radio Mode)");
    } else if (!pressed && this->dahIsHeld) {
        // Dah released - reset timer
        unsigned long holdTime = currentTime - this->dahHoldStartTime;
        Serial.print("Dah released after ");
        Serial.print(holdTime);
        Serial.println("ms");
        this->dahIsHeld = false;
    }
}
#ifdef HAS_RADIO_OUTPUT
if (paddle == PADDLE_DAH && isCapacitive && pressed) {
if (currentTime - this->lastCapDahTime < DAH_SPAM_WINDOW) {
this->capDahPressCount++;
if (this->capDahPressCount >= DAH_SPAM_COUNT_RADIO_MODE) {
this->ToggleRadioMode();
this->capDahPressCount = 0;
}
} else {
this->capDahPressCount = 1;
}
this->lastCapDahTime = currentTime;
} else if (paddle == PADDLE_DAH && !isCapacitive && pressed) {
this->capDahPressCount = 0;
} else if (paddle == PADDLE_DIT && pressed) {
this->capDahPressCount = 0;
}
#endif

if (this->radioModeActive) {
#ifdef HAS_RADIO_OUTPUT
    // In radio mode, either use keyer or direct paddle control
    if (paddle == PADDLE_STRAIGHT) {
        // Straight key always goes to DIT pin
        bool radioKeyIsActiveBefore = radioDitState;
        radioDitState = pressed;
        setRadioDit(pressed);
        radioDahState = false;
        keyIsPressed = radioDitState;
        if (!keyIsPressed && radioKeyIsActiveBefore) {
            this->buzzer->NoTone(0);
        }
    } else {
        // DIT/DAH paddles
        bool radioKeyIsActiveBefore = radioDitState || radioDahState;

        if (this->radioKeyerMode) {
            // Radio Keyer Mode: Use adapter's keyer, output all on DIT pin only
            if (this->keyer) {
                // Keyer will call BeginTx(relay)/EndTx(relay) which routes to DIT pin
                this->keyer->Key(paddle, pressed);
            } else {
                // Passthrough mode - directly control DIT pin for both paddles
                radioDitState = pressed;
                setRadioDit(radioDitState);
                radioDahState = false;
            }
        } else {
            // Normal Radio Mode: Passthrough to separate pins (let radio's keyer handle it)
            Serial.print("Normal Radio Mode Passthrough - paddle=");
            Serial.print(paddle == PADDLE_DIT ? "DIT" : "DAH");
            Serial.print(", pressed=");
            Serial.println(pressed);

            if (paddle == PADDLE_DIT) {
                radioDitState = pressed;
                setRadioDit(radioDitState);
                Serial.print("  -> Set DIT pin to ");
                Serial.println(pressed ? "ACTIVE" : "INACTIVE");
            } else if (paddle == PADDLE_DAH) {
                radioDahState = pressed;
                setRadioDah(radioDahState);
                Serial.print("  -> Set DAH pin to ");
                Serial.println(pressed ? "ACTIVE" : "INACTIVE");
            }
        }

        keyIsPressed = radioDitState || radioDahState;
        if (!keyIsPressed && radioKeyIsActiveBefore) {
            this->buzzer->NoTone(0);
        }
    }
#endif
} else {
if (paddle == PADDLE_STRAIGHT) {
if (pressed) BeginTx(); else EndTx();
} else {
if (this->keyer) {
if (pressed) this->lastPaddlePressed = paddle; // Track last paddle pressed
this->keyer->Key(paddle, pressed);
} else {
bool currentPaddleActivity = false;
if (paddle == PADDLE_DIT) {
if (this->keyboardMode) this->keyboardKey(DIT_KEYBOARD_KEY, pressed);
else this->midiKey(1, pressed);
if (pressed) currentPaddleActivity = true;
} else if (paddle == PADDLE_DAH) {
if (this->keyboardMode) this->keyboardKey(DAH_KEYBOARD_KEY, pressed);
else this->midiKey(2, pressed);
if (pressed) currentPaddleActivity = true;
}

            if (currentPaddleActivity) { 
                if(!keyIsPressed) BeginTx(); 
            } else { 
                if(keyIsPressed) EndTx();
            }
        }
    }
  }
}

void VailAdapter::HandleMIDI(midiEventPacket_t event) {
uint16_t msg = (event.byte1 << 8) | (event.byte2 << 0);
switch (event.byte1) {
case 0xB0:
switch (event.byte2) {
case 0:
this->keyboardMode = (event.byte3 > 0x3f);
Serial.print("Keyboard mode: "); Serial.println(this->keyboardMode ? "ON" : "OFF");
MidiUSB.sendMIDI(event);
break;
case 1:
this->ditDuration = event.byte3 * 2 * MILLISECOND;
if (this->keyer) {
this->keyer->SetDitDuration(this->ditDuration);
}
Serial.print("Dit duration set to: "); Serial.println(this->ditDuration);
saveSettingsToEEPROM(getCurrentKeyerType(), this->ditDuration, this->txNote);
break;
case 2:
this->txNote = event.byte3;
Serial.print("TX Note set to: "); Serial.println(this->txNote);

saveSettingsToEEPROM(getCurrentKeyerType(), this->ditDuration, this->txNote);
break;
}
break;
case 0xC0:
if (this->keyer) {
this->keyer->Reset();  // Clear any held key state before switching
this->keyer->Release();
this->keyer = NULL;
}
// Ensure all keyboard keys are released when switching keyers
ReleaseAllKeys();
this->keyer = GetKeyerByNumber(event.byte2, this);
if (this->keyer) {
this->keyer->SetDitDuration(this->ditDuration);
Serial.print("Keyer mode set to: "); Serial.println(event.byte2);
} else {
Serial.print("Keyer mode set to passthrough (or invalid): "); Serial.println(event.byte2);
}
saveSettingsToEEPROM(event.byte2, this->ditDuration, this->txNote);
break;
case 0x80:
if (this->buzzerEnabled && !this->radioModeActive) this->buzzer->NoTone(1);
break;
case 0x90:
if (this->buzzerEnabled && !this->radioModeActive) this->buzzer->Note(1, event.byte2);
break;
}
}

void VailAdapter::Tick(unsigned int currentMillis) {
// Check for dit hold during each tick
if (this->ditIsHeld && this->buzzerEnabled) {
    unsigned long holdTime = currentMillis - this->ditHoldStartTime;
    if (holdTime >= DIT_HOLD_BUZZER_DISABLE_THRESHOLD) {
        Serial.print("Dit held for ");
        Serial.print(holdTime);
        Serial.println("ms - disabling buzzer");
        this->DisableBuzzer();
        this->ditIsHeld = false; // Reset to prevent re-triggering
    } else if (holdTime % 1000 == 0) {
        // Debug: show progress every second
        Serial.print("Dit held for ");
        Serial.print(holdTime);
        Serial.println("ms");
    }
}

// Check for dah hold in radio mode during each tick
#ifdef HAS_RADIO_OUTPUT
if (this->dahIsHeld && this->radioModeActive) {
    unsigned long holdTime = currentMillis - this->dahHoldStartTime;
    if (holdTime >= DAH_HOLD_RADIO_KEYER_TOGGLE_THRESHOLD) {
        Serial.print("Dah held for ");
        Serial.print(holdTime);
        Serial.println("ms - toggling Radio Keyer Mode");
        this->ToggleRadioKeyerMode();
        this->dahIsHeld = false; // Reset to prevent re-triggering
    }
}
#endif

if (!radioModeActive && keyIsPressed && this->buzzerEnabled && this->keyPressStartTime > 0) {
if (currentMillis - this->keyPressStartTime >= KEY_HOLD_DISABLE_THRESHOLD) {
this->DisableBuzzer();
}
}

if (this->keyer) {
    this->keyer->Tick(currentMillis);
}
}

