#include <Arduino.h>
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
    for (int i = 0; i < POLYBUZZER_MAX_TONES; i++) {
        if (this->tones[i]) {
            if (this->playing != this->tones[i]) {
                this->playing = this->tones[i];
                Serial.print("Buzzer playing frequency: ");
                Serial.println(this->playing);
                tone(this->pin, this->playing);
            }
            return;
        }
    }
    this->playing = 0;
    Serial.println("Buzzer stopped");
    noTone(this->pin);
}

// On single-slot builds (AVR), remap any higher-priority slot request to
// slot 0 so overlay tones still play audibly — just without the priority
// stacking behavior the SAMD build supports.
static inline int clampSlot(int slot) {
    if (slot >= POLYBUZZER_MAX_TONES) return POLYBUZZER_MAX_TONES - 1;
    if (slot < 0) return 0;
    return slot;
}

void PolyBuzzer::Tone(int slot, unsigned int frequency) {
    slot = clampSlot(slot);
    Serial.print("Setting tone in slot ");
    Serial.print(slot);
    Serial.print(" to frequency: ");
    Serial.println(frequency);

    this->tones[slot] = frequency;
    this->update();
}

void PolyBuzzer::Note(int slot, uint8_t note) {
    if (note > 127) {
        note = 127;
    }
    slot = clampSlot(slot);

    Serial.print("Setting note in slot ");
    Serial.print(slot);
    Serial.print(" to MIDI note #");
    Serial.print(note);
    Serial.print(" (frequency: ");
    Serial.print(GET_EQUAL_TEMPERAMENT_NOTE(note));
    Serial.println("Hz)");

    this->Tone(slot, GET_EQUAL_TEMPERAMENT_NOTE(note));
}

void PolyBuzzer::NoTone(int slot) {
    slot = clampSlot(slot);
    Serial.print("Clearing tone in slot ");
    Serial.println(slot);

    tones[slot] = 0;
    this->update();
}