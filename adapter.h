#pragma once

#include <MIDIUSB.h>
#include "keyers.h"
#include "polybuzzer.h"
#include "config.h" // Include config.h
#include "memory.h" // Include memory.h for recording state

// Paddle swap scope (CC3). Values are stored in EEPROM as-is, so keep them
// stable: legacy firmware stored 0=off / 1=swapped, which maps onto
// OFF / ALL below unchanged.
#define PADDLE_SWAP_OFF   0  // normal mapping
#define PADDLE_SWAP_ALL   1  // swap physical paddle pins AND capacitive touch pads
#define PADDLE_SWAP_TOUCH 2  // swap capacitive touch pads only

class VailAdapter: public Transmitter {
private:
    unsigned int txNote = DEFAULT_TONE_NOTE;
    unsigned int ditDuration = DEFAULT_ADAPTER_DIT_DURATION_MS;
    bool keyboardMode = true;
    Keyer *keyer = NULL;
    PolyBuzzer *buzzer = NULL;

    unsigned long keyPressStartTime = 0;
    bool keyIsPressed = false;

    unsigned long ditHoldStartTime = 0;
    bool ditIsHeld = false;
    bool buzzerEnabled = true;

    bool radioModeActive = false;
    bool radioKeyerMode = false;
    uint8_t paddleSwapMode = PADDLE_SWAP_OFF;
    unsigned long lastCapDahTime = 0;
    unsigned int capDahPressCount = 0;
    unsigned long dahHoldStartTime = 0;
    bool dahIsHeld = false;
    bool radioDitState = false;
    bool radioDahState = false;

    // Track which relays are active for proper key mapping
    bool txRelays[2] = {false, false}; // [dit, dah]
    int lastPaddlePressed = PADDLE_DIT; // Track last paddle for keyer transmission

    // Track which keyboard keys are currently pressed
    bool ditKeyPressed = false;
    bool dahKeyPressed = false;

    // CW memory recording
    RecordingState* recordingState = nullptr;

    void midiKey(uint8_t key, bool down);
    void keyboardKey(uint8_t key, bool down);

    void setRadioDit(bool active);
    void setRadioDah(bool active);

public:
    VailAdapter(unsigned int PiezoPin);
    bool KeyboardMode();

    void ProcessPaddleInput(Paddle paddle, bool pressed, bool isCapacitive);
    void HandleMIDI(midiEventPacket_t event);

    void BeginTx() override;
    void EndTx() override;
    void BeginTx(int relay) override;
    void EndTx(int relay) override;
    void Tx(int relay, bool closed); // Add Tx method for keyer relay control

    void Tick(unsigned long millis);
    
    void ResetDitCounter(); 
    void DisableBuzzer(); 
    bool isBuzzerEnabled() const;

    void ToggleRadioMode();
    bool isRadioModeActive() const;
    void ToggleRadioKeyerMode();
    bool isRadioKeyerMode() const;
    void SetRadioKeyerMode(bool enabled);
    void ResetDahCounter();
    void ResetDahHoldCounter();

    uint8_t getPaddleSwapMode() const;
    void SetPaddleSwapMode(uint8_t mode, bool announce);

    uint8_t getCurrentKeyerType() const;
    uint16_t getDitDuration() const;
    uint8_t getTxNote() const;

    // CW memory recording support
    void setRecordingState(RecordingState* state);

    // Cleanup method to release all keys
    void ReleaseAllKeys();
};
