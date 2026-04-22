#pragma once

#include <Arduino.h>

#if defined(__AVR__)
  #include <avr/pgmspace.h>
#endif

// MIDI note number → frequency (Hz) lookup, 0..127.
// Stored in PROGMEM on AVR to save ~256 bytes of SRAM.
// pgm_read_word is a no-op pointer deref on ARM cores, so GET_EQUAL_TEMPERAMENT_NOTE is safe on SAMD21.
extern const uint16_t equalTemperamentNote[128] PROGMEM;

#define GET_EQUAL_TEMPERAMENT_NOTE(n) pgm_read_word(&equalTemperamentNote[(n)])
