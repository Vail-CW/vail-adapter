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

    // Keyboard Sim mode — decode keyed Morse into USB keystrokes.
    // Armed by keying "KSKS" as a standalone word (a word-gap before the first
    // K and after the last S). Detection runs on DECODED CHARACTERS, not on the
    // raw dit/dah element stream, so it cannot false-trigger on element runs
    // buried inside other words (e.g. the OLECU run inside "molecule").
    bool keyboardSimMode = false;
    uint8_t ksksMatchLen = 0;          // How many of K-S-K-S matched in order
    unsigned long firstKTime = 0;      // millis() of the first K (sequence timeout)
    bool wordBoundaryPending = true;   // True at boot / after a word-gap; gates the first K
    bool ksksArmed = false;            // KSKS fully matched, awaiting the trailing gap
    unsigned long ksksArmedTime = 0;   // millis() when armed (trailing-gap timer)

    // Callback fired when Keyboard Sim mode activates (used to reset the decoder)
    void (*onEnterKeyboardSimMode)() = nullptr;

    // Track which relays are active for proper key mapping
    bool txRelays[2] = {false, false}; // [dit, dah]
    int lastPaddlePressed = PADDLE_DIT; // Track last paddle for keyer transmission

    // Track which keyboard keys are currently pressed
    bool ditKeyPressed = false;
    bool dahKeyPressed = false;

    // Track which MIDI notes are currently sounding (0=straight, 1=dit, 2=dah)
    bool midiNoteOn[3] = {false, false, false};

    // CW memory recording
    RecordingState* recordingState = nullptr;

    void midiKey(uint8_t key, bool down);
    void keyboardKey(uint8_t key, bool down);
    void releaseOutputKeys();

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

    // Drop all in-flight input state (keyer relays, host keys, hold timers).
    // Call whenever the input path changes underneath the keyer.
    void ResetInputState();

    // Keyboard Sim mode
    bool isKeyboardSimMode() const { return keyboardSimMode; }
    bool isTransmitting() const { return keyIsPressed; }
    void checkForKSKS(char c);                 // Fed decoded characters; arms/activates KSKS
    void notifyWordBoundary();                 // Decoder reports a word-gap (leading guard)
    void enterKeyboardSimMode();
    void setEnterKeyboardSimModeCallback(void (*callback)()) { onEnterKeyboardSimMode = callback; }
    void outputKeyboardChar(char c);
    void outputKeyboardBackspace();
    void outputKeyboardEnter();
    void outputKeyboardSpace();
};
