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

// Forward declaration of the save settings function from the main sketch
extern void saveSettingsToEEPROM(uint8_t keyerType, uint16_t ditDuration, uint8_t txNote);

VailAdapter::VailAdapter(unsigned int PiezoPin) {
    this->buzzer = new PolyBuzzer(PiezoPin);
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
    this->buzzer->Note(0, this->txNote);
    if (this->keyboardMode) {
        this->keyboardKey(KEY_LEFT_CTRL, true);
    } else {
        this->midiKey(0, true);
    }
}

// Stop transmitting
void VailAdapter::EndTx() {
    this->buzzer->NoTone(0);
    if (this->keyboardMode) {
        this->keyboardKey(KEY_LEFT_CTRL, false);
    } else {
        this->midiKey(0, false);
    }
}

// Handle a paddle being pressed.
//
// The caller needs to debounce keys and deal with keys wired in parallel.
void VailAdapter::HandlePaddle(Paddle paddle, bool pressed) {
    switch (paddle) {
    case PADDLE_STRAIGHT:
        if (pressed) {
            this->BeginTx();
        } else {
            this->EndTx();
        }
        return;
    case PADDLE_DIT:
        if (this->keyer) {
            this->keyer->Key(paddle, pressed);
        } else if (this->keyboardMode) {
            this->keyboardKey(KEY_LEFT_CTRL, pressed);
        } else {
            this->midiKey(1, pressed);
        }
        break;
    case PADDLE_DAH:
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
                // Save settings to EEPROM
                saveSettingsToEEPROM(getCurrentKeyerType(), this->ditDuration, this->txNote);
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
        this->buzzer->NoTone(1);
        break;
    case 0x90: // Note on
        this->buzzer->Note(1, event.byte2);
        break;
    }
}

void VailAdapter::Tick(unsigned millis) {
    if (this->keyer) {
        this->keyer->Tick(millis);
    }
}