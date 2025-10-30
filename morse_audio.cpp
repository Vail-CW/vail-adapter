#include "morse_audio.h"
#include "config.h"
#include "equal_temperament.h"

// Valid keyer types for cycling
#define KEYER_STRAIGHT 1
#define KEYER_IAMBIC_A 7
#define KEYER_IAMBIC_B 8

// Static module-level references (set during init)
static VailAdapter* adapter = nullptr;
static int piezoPin = 0;

void initMorseAudio(VailAdapter* adapterRef, int piezoPinRef) {
  adapter = adapterRef;
  piezoPin = piezoPinRef;
}

// ============================================================================
// Morse Code Playback Functions
// ============================================================================

void playMorseDit() {
  if (!adapter) return;
  uint8_t note = adapter->getTxNote();
  uint16_t ditDur = adapter->getDitDuration();

  tone(piezoPin, equalTemperamentNote[note]);
  delay(ditDur);
  noTone(piezoPin);
  delay(ditDur);  // Inter-element space = 1 dit
}

void playMorseDah() {
  if (!adapter) return;
  uint8_t note = adapter->getTxNote();
  uint16_t ditDur = adapter->getDitDuration();

  tone(piezoPin, equalTemperamentNote[note]);
  delay(ditDur * 3);
  noTone(piezoPin);
  delay(ditDur);  // Inter-element space = 1 dit
}

void playMorseChar(char c) {
  switch(c) {
    case 'A': playMorseDit(); playMorseDah(); break;
    case 'C': playMorseDah(); playMorseDit(); playMorseDah(); playMorseDit(); break;
    case 'D': playMorseDah(); playMorseDit(); playMorseDit(); break;
    case 'E': playMorseDit(); break;
    case 'I': playMorseDit(); playMorseDit(); break;
    case 'K': playMorseDah(); playMorseDit(); playMorseDah(); break;
    case 'L': playMorseDit(); playMorseDah(); playMorseDit(); playMorseDit(); break;
    case 'M': playMorseDah(); playMorseDah(); break;
    case 'N': playMorseDah(); playMorseDit(); break;
    case 'O': playMorseDah(); playMorseDah(); playMorseDah(); break;
    case 'P': playMorseDit(); playMorseDah(); playMorseDah(); playMorseDit(); break;
    case 'R': playMorseDit(); playMorseDah(); playMorseDit(); break;
    case 'S': playMorseDit(); playMorseDit(); playMorseDit(); break;
    case 'T': playMorseDah(); break;
    case 'Y': playMorseDah(); playMorseDit(); playMorseDah(); playMorseDah(); break;
    case '1': playMorseDit(); playMorseDah(); playMorseDah(); playMorseDah(); playMorseDah(); break;
    case '2': playMorseDit(); playMorseDit(); playMorseDah(); playMorseDah(); playMorseDah(); break;
    case '3': playMorseDit(); playMorseDit(); playMorseDit(); playMorseDah(); playMorseDah(); break;
  }
  // Inter-character space = 3 dits (we already have 1 from last element)
  if (adapter) {
    delay(adapter->getDitDuration() * 2);
  }
}

void playMorseWord(const char* word) {
  while (*word) {
    playMorseChar(*word);
    word++;
  }
  // Inter-word space = 7 dits (we already have 3 from last char)
  if (adapter) {
    delay(adapter->getDitDuration() * 4);
  }
}

// ============================================================================
// Startup Sequence Functions
// ============================================================================

void playDot(uint8_t noteNumber) {
  digitalWrite(LED_BUILTIN, LED_ON);
  tone(piezoPin, equalTemperamentNote[noteNumber]);
  delay(DOT_DURATION);
  digitalWrite(LED_BUILTIN, LED_OFF);
  noTone(piezoPin);
  delay(ELEMENT_SPACE);
}

void playDash(uint8_t noteNumber) {
  digitalWrite(LED_BUILTIN, LED_ON);
  tone(piezoPin, equalTemperamentNote[noteNumber]);
  delay(DASH_DURATION);
  digitalWrite(LED_BUILTIN, LED_OFF);
  noTone(piezoPin);
  delay(ELEMENT_SPACE);
}

void playVAIL(uint8_t noteNumber) {
  playDot(noteNumber); playDot(noteNumber); playDot(noteNumber); playDash(noteNumber);
  delay(CHAR_SPACE - ELEMENT_SPACE);
  playDot(noteNumber); playDash(noteNumber);
  delay(CHAR_SPACE - ELEMENT_SPACE);
  playDot(noteNumber); playDot(noteNumber);
  delay(CHAR_SPACE - ELEMENT_SPACE);
  playDot(noteNumber); playDash(noteNumber); playDot(noteNumber); playDot(noteNumber);
  noTone(piezoPin);
}

// ============================================================================
// Audio Feedback Functions
// ============================================================================

void playAdjustmentBeep(bool isIncrease) {
  if (!adapter) return;
  uint8_t note = adapter->getTxNote();
  uint16_t frequency;

  if (isIncrease) {
    // Higher tone for increase (3 semitones up)
    uint8_t highNote = min(note + 3, 127);
    frequency = equalTemperamentNote[highNote];
  } else {
    // Lower tone for decrease (3 semitones down)
    uint8_t lowNote = max((int)note - 3, 0);
    frequency = equalTemperamentNote[lowNote];
  }

  tone(piezoPin, frequency);
  delay(50);  // 50ms beep
  noTone(piezoPin);
}

void playErrorTone() {
  tone(piezoPin, 200);  // Low 200 Hz buzz
  delay(200);  // 200ms duration
  noTone(piezoPin);
}

void playDescendingTones() {
  // Descending tone pattern for timeout/exit without save
  int frequencies[] = {1000, 900, 800, 700, 600, 500, 400};
  for (int i = 0; i < 7; i++) {
    tone(piezoPin, frequencies[i]);
    delay(100);
    noTone(piezoPin);
    if (i < 6) delay(20);  // Small gap between tones
  }
}

void playRecordingCountdown() {
  // "doot, doot, dah" countdown pattern (inspired by Mario Kart)
  // First doot: 800 Hz, 200ms
  tone(piezoPin, 800);
  delay(200);
  noTone(piezoPin);
  delay(200);  // Pause

  // Second doot: 800 Hz, 200ms
  tone(piezoPin, 800);
  delay(200);
  noTone(piezoPin);
  delay(200);  // Pause

  // Dah: 600 Hz, 600ms
  tone(piezoPin, 600);
  delay(600);
  noTone(piezoPin);
  delay(200);  // Pause before recording starts
}

void playMemoryClearedAnnouncement(uint8_t slotNumber) {
  if (!adapter) return;
  // Play "[N] CLR" where N is the slot number (1-3)
  char slotChar = '1' + slotNumber;  // slotNumber is 0-2, we want '1'-'3'
  playMorseChar(slotChar);
  delay(adapter->getDitDuration() * 2);  // Extra space between number and word
  playMorseWord("CLR");
}

// ============================================================================
// Keyer Type Announcement Functions
// ============================================================================

const char* getKeyerTypeName(uint8_t keyerType) {
  switch (keyerType) {
    case 1: return "Straight";
    case 2: return "Bug";
    case 3: return "ElBug";
    case 4: return "SingleDot";
    case 5: return "Ultimatic";
    case 6: return "Plain";
    case 7: return "Iambic A";
    case 8: return "Iambic B";
    case 9: return "Keyahead";
    default: return "Unknown";
  }
}

void playKeyerTypeCode(uint8_t keyerType) {
  if (!adapter) return;
  // Play Morse code identifier for the keyer type
  uint16_t ditDur = adapter->getDitDuration();
  uint16_t charSpace = ditDur * 3;

  switch (keyerType) {
    case 1:  // Straight
      // S = ... (dit dit dit)
      playMorseDit(); playMorseDit(); playMorseDit();
      break;
    case 2:  // Bug
      // B = -... (dah dit dit dit)
      playMorseDah(); playMorseDit(); playMorseDit(); playMorseDit();
      break;
    case 3:  // ElBug
      // EB = . -... (E then B)
      // E = .
      playMorseDit();
      delay(charSpace - ditDur);  // Space between letters
      // B = -...
      playMorseDah(); playMorseDit(); playMorseDit(); playMorseDit();
      break;
    case 4:  // SingleDot
      // SD = ... -.. (S then D)
      // S = ...
      playMorseDit(); playMorseDit(); playMorseDit();
      delay(charSpace - ditDur);  // Space between letters
      // D = -..
      playMorseDah(); playMorseDit(); playMorseDit();
      break;
    case 5:  // Ultimatic
      // U = ..- (dit dit dah)
      playMorseDit(); playMorseDit(); playMorseDah();
      break;
    case 6:  // Plain
      // P = .--. (dit dah dah dit)
      playMorseDit(); playMorseDah(); playMorseDah(); playMorseDit();
      break;
    case 7:  // Iambic A
      // IA = .. .- (I then A)
      // I = ..
      playMorseDit(); playMorseDit();
      delay(charSpace - ditDur);  // Space between letters
      // A = .-
      playMorseDit(); playMorseDah();
      break;
    case 8:  // Iambic B
      // IB = .. -... (I then B)
      // I = ..
      playMorseDit(); playMorseDit();
      delay(charSpace - ditDur);  // Space between letters
      // B = -...
      playMorseDah(); playMorseDit(); playMorseDit(); playMorseDit();
      break;
    case 9:  // Keyahead
      // K = -.- (dah dit dah)
      playMorseDah(); playMorseDit(); playMorseDah();
      break;
  }
}
