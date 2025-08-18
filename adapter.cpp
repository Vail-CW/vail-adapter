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
this->keyIsPressed = false;
this->keyPressStartTime = 0;
this->lastDitTime = 0;
this->ditPressCount = 0;
this->lastCapDahTime = 0;
this->capDahPressCount = 0;
this->radioDitState = false;
this->radioDahState = false;
this->keyboardMode = true;
this->keyer = NULL;
this->txNote = DEFAULT_TONE_NOTE;
this->ditDuration = DEFAULT_ADAPTER_DIT_DURATION_MS;
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

bool VailAdapter::isRadioModeActive() const {
return this->radioModeActive;
}

void VailAdapter::ResetDitCounter() {
this->ditPressCount = 0;
}

void VailAdapter::ResetDahCounter() {
this->capDahPressCount = 0;
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
} else {
Keyboard.release(key);
}
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

void VailAdapter::BeginTx() {
if (!keyIsPressed) {
keyIsPressed = true;
if (!this->radioModeActive) {
if (this->keyPressStartTime == 0) {
this->keyPressStartTime = millis();
}
}
}

if (this->buzzerEnabled && !this->radioModeActive) { 
    this->buzzer->Note(0, this->txNote);
}

if (!this->radioModeActive) { 
    if (this->keyboardMode) {
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

this->buzzer->NoTone(0); 

if (!this->radioModeActive) { 
    if (this->keyboardMode) {
        this->keyboardKey(KEY_LEFT_CTRL, false);
    } else {
        this->midiKey(0, false);
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

setRadioDit(false); 
setRadioDah(false);
radioDitState = false;
radioDahState = false;
keyIsPressed = false; 

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

void VailAdapter::ProcessPaddleInput(Paddle paddle, bool pressed, bool isCapacitive) {
unsigned long currentTime = millis();

if (paddle == PADDLE_DIT && pressed && this->buzzerEnabled) {
    if (currentTime - this->lastDitTime < SPAM_DISABLE_WINDOW) {
        this->ditPressCount++;
        if (this->ditPressCount >= DIT_SPAM_COUNT_BUZZER_DISABLE) {
            this->DisableBuzzer();
            this->ditPressCount = 0; 
        }
    } else {
        this->ditPressCount = 1;
    }
    this->lastDitTime = currentTime;
}
#ifdef HAS_RADIO_OUTPUT
if (paddle == PADDLE_DAH && isCapacitive && pressed) {
if (currentTime - this->lastCapDahTime < SPAM_DISABLE_WINDOW) {
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
bool radioKeyIsActiveBefore = radioDitState || radioDahState;

    if (paddle == PADDLE_DIT) {
        radioDitState = pressed;
        setRadioDit(radioDitState);
    } else if (paddle == PADDLE_DAH) {
        radioDahState = pressed;
        setRadioDah(radioDahState);
    } else if (paddle == PADDLE_STRAIGHT) { 
        radioDitState = pressed; 
        setRadioDit(pressed);
    }

    keyIsPressed = radioDitState || radioDahState; 

    if (!keyIsPressed && radioKeyIsActiveBefore) { 
         this->buzzer->NoTone(0);
    }
#endif
} else {
if (paddle == PADDLE_STRAIGHT) {
if (pressed) BeginTx(); else EndTx();
} else {
if (this->keyer) {
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
this->keyer->Release();
this->keyer = NULL;
}
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
if (!radioModeActive && keyIsPressed && this->buzzerEnabled && this->keyPressStartTime > 0) {
if (currentMillis - this->keyPressStartTime >= KEY_HOLD_DISABLE_THRESHOLD) {
this->DisableBuzzer();
}
}

if (this->keyer && !this->radioModeActive) { 
    this->keyer->Tick(currentMillis);
}
}

