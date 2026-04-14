#pragma once
#include <Arduino.h>

#define POLYBUZZER_MAX_TONES 1

class PolyBuzzer {
public:
  unsigned int tones[POLYBUZZER_MAX_TONES];
  unsigned int playing;
  uint8_t pin;

  PolyBuzzer(uint8_t pin);
  void update();
  void Tone(int slot, unsigned int frequency);
  void Note(int slot, uint8_t note);
  void NoTone(int slot);
};