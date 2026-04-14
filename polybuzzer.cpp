#include "polybuzzer.h"
#include "equal_temperament.h"

PolyBuzzer::PolyBuzzer(uint8_t pin) {
  for (int i = 0; i < POLYBUZZER_MAX_TONES; i++) {
    this->tones[i] = 0;
  }
  this->playing = 0;
  this->pin = pin;
  pinMode(pin, OUTPUT);
}

void PolyBuzzer::update() {
  // Micro-safe: only use slot 0
  if (this->tones[0] != 0) {
    if (this->playing != this->tones[0]) {
      this->playing = this->tones[0];
      Serial.print(F("Buzzer playing frequency: "));
      Serial.println(this->playing);
      tone(this->pin, this->playing);
    }
    return;
  }

  this->playing = 0;
  Serial.println(F("Buzzer stopped"));
  noTone(this->pin);
}

void PolyBuzzer::Tone(int slot, unsigned int frequency) {
  if (slot != 0) return;  // ignore extra slots on Micro

  Serial.print(F("Setting tone in slot "));
  Serial.print(slot);
  Serial.print(F(" to frequency: "));
  Serial.println(frequency);

  this->tones[0] = frequency;
  this->update();
}

void PolyBuzzer::Note(int slot, uint8_t note) {
  if (slot != 0) return;  // ignore extra slots on Micro

  if (note > 127) {
    note = 127;
  }

  uint16_t frequency = GET_EQUAL_TEMPERAMENT_NOTE(note);

  Serial.print(F("Setting note in slot "));
  Serial.print(slot);
  Serial.print(F(" to MIDI note #"));
  Serial.print(note);
  Serial.print(F(" (frequency: "));
  Serial.print(frequency);
  Serial.println(F("Hz)"));

  this->Tone(0, frequency);
}

void PolyBuzzer::NoTone(int slot) {
  if (slot != 0) return;  // ignore extra slots on Micro

  Serial.print(F("Clearing tone in slot "));
  Serial.println(slot);

  this->tones[0] = 0;
  this->update();
}