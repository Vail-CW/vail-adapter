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

    unsigned long ditHoldStartTime = 0;
    bool ditIsHeld = false;
    bool buzzerEnabled = true;

    bool radioModeActive = false;
    unsigned long lastCapDahTime = 0;
    unsigned int capDahPressCount = 0;
    bool radioDitState = false;
    bool radioDahState = false;


    void midiKey(uint8_t key, bool down);
    void keyboardKey(uint8_t key, bool down);
    void keyboardDit(bool down);
    void keyboardDah(bool down);

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
};
