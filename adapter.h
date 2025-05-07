#pragma once

#include <MIDIUSB.h>
#include "keyers.h"
#include "polybuzzer.h"

class VailAdapter: public Transmitter {
private:
    unsigned int txNote = 69;
    unsigned int ditDuration = 100;
    bool keyboardMode = true;
    Keyer *keyer = NULL;
    PolyBuzzer *buzzer = NULL;
    
    // Variables for tracking key press duration (for straight key)
    unsigned long keyPressStartTime = 0;
    bool keyIsPressed = false;
    
    // Variables for tracking consecutive dit presses (for paddle)
    unsigned long lastDitTime = 0;
    unsigned int ditPressCount = 0;
    
    bool buzzerEnabled = true;
    
    void midiKey(uint8_t key, bool down);
    void keyboardKey(uint8_t key, bool down);
    void DisableBuzzer();

public:
    VailAdapter(unsigned int PiezoPin);
    bool KeyboardMode();
    void HandlePaddle(Paddle key, bool pressed);
    void HandleMIDI(midiEventPacket_t event);
    void BeginTx();
    void EndTx();
    void Tick(unsigned int millis);
    void ResetDitCounter();
    uint8_t getCurrentKeyerType() const;
    uint16_t getDitDuration() const;
    uint8_t getTxNote() const;
    bool isBuzzerEnabled() const;
};
