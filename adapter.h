#pragma once

#include <MIDIUSB.h>
#include "keyers.h"
#include "polybuzzer.h"
#include "config.h" // Include config.h

class VailAdapter: public Transmitter {
private:
    unsigned int txNote = DEFAULT_TONE_NOTE;
    unsigned int ditDuration = DEFAULT_ADAPTER_DIT_DURATION_MS;
    bool keyboardMode = true;
    Keyer *keyer = NULL;
    PolyBuzzer *buzzer = NULL;

    unsigned long keyPressStartTime = 0;
    bool keyIsPressed = false; 

    unsigned long lastDitTime = 0;
    unsigned int ditPressCount = 0;
    bool buzzerEnabled = true;

    bool radioModeActive = false;
    unsigned long lastCapDahTime = 0;
    unsigned int capDahPressCount = 0;
    bool radioDitState = false;
    bool radioDahState = false;

    uint8_t ditKey; // New member for dit key
    uint8_t dahKey; // New member for dah key


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

    void Tick(unsigned int millis);
    
    void ResetDitCounter(); 
    void DisableBuzzer(); 
    bool isBuzzerEnabled() const;

    void ToggleRadioMode();
    bool isRadioModeActive() const;
    void ResetDahCounter();

    uint8_t getCurrentKeyerType() const;
    uint16_t getDitDuration() const;
    uint8_t getTxNote() const;

    void setKeybindings(uint8_t newDitKey, uint8_t newDahKey); // New method to set keybindings
    uint8_t getDitKey() const; // New getter for dit key
    uint8_t getDahKey() const; // New getter for dah key
};