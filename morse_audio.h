#ifndef MORSE_AUDIO_H
#define MORSE_AUDIO_H

#include <Arduino.h>
#include "adapter.h"

// Forward declarations
class VailAdapter;

// Morse code playback using adapter settings (user's current WPM and tone)
void playMorseDit();
void playMorseDah();
void playMorseChar(char c);
void playMorseWord(const char* word);

// Startup sequence
void playDot(uint8_t noteNumber);
void playDash(uint8_t noteNumber);
void playVAIL(uint8_t noteNumber);

// Audio feedback tones
void playAdjustmentBeep(bool isIncrease);
void playErrorTone();
void playDescendingTones();
void playRecordingCountdown();
void playMemoryClearedAnnouncement(uint8_t slotNumber);

// Keyer type announcement
void playKeyerTypeCode(uint8_t keyerType);
const char* getKeyerTypeName(uint8_t keyerType);

// Initialize with adapter reference
void initMorseAudio(VailAdapter* adapterRef, int piezoPinRef);

#endif // MORSE_AUDIO_H
