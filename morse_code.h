/*
 * Morse Code Library
 * Lookup tables and timing functions for morse code generation
 */

#ifndef MORSE_CODE_H
#define MORSE_CODE_H

#include "config.h"

// Morse code representation: . = dit, - = dah
const char* morseTable[] = {
  ".-",    // A
  "-...",  // B
  "-.-.",  // C
  "-..",   // D
  ".",     // E
  "..-.",  // F
  "--.",   // G
  "....",  // H
  "..",    // I
  ".---",  // J
  "-.-",   // K
  ".-..",  // L
  "--",    // M
  "-.",    // N
  "---",   // O
  ".--.",  // P
  "--.-",  // Q
  ".-.",   // R
  "...",   // S
  "-",     // T
  "..-",   // U
  "...-",  // V
  ".--",   // W
  "-..-",  // X
  "-.--",  // Y
  "--..",  // Z
  "-----", // 0
  ".----", // 1
  "..---", // 2
  "...--", // 3
  "....-", // 4
  ".....", // 5
  "-....", // 6
  "--...", // 7
  "---..", // 8
  "----.", // 9
  ".-.-.-", // Period
  "--..--", // Comma
  "..--..", // Question mark
  ".----.", // Apostrophe
  "-.-.--", // Exclamation
  "-..-.",  // Slash
  "-.--.",  // Parenthesis (
  "-.--.-", // Parenthesis )
  ".-...",  // Ampersand
  "---...", // Colon
  "-.-.-.", // Semicolon
  "-...-",  // Equals
  ".-.-.",  // Plus
  "-....-", // Hyphen/Minus
  "..--.-", // Underscore
  ".-..-.", // Quote
  "...-..-", // Dollar
  ".--.-."  // At sign
};

// Get morse code pattern for a character
const char* getMorseCode(char c) {
  c = toupper(c);

  if (c >= 'A' && c <= 'Z') {
    return morseTable[c - 'A'];
  } else if (c >= '0' && c <= '9') {
    return morseTable[26 + (c - '0')];
  } else if (c == '.') {
    return morseTable[36];
  } else if (c == ',') {
    return morseTable[37];
  } else if (c == '?') {
    return morseTable[38];
  } else if (c == '/') {
    return morseTable[41];
  } else if (c == '-') {
    return morseTable[47];
  }

  return nullptr; // Unknown character
}

// Calculate timing based on WPM
struct MorseTiming {
  int ditDuration;
  int dahDuration;
  int elementGap;
  int letterGap;
  int wordGap;

  MorseTiming(int wpm) {
    ditDuration = DIT_DURATION(wpm);
    dahDuration = ditDuration * 3;
    elementGap = ditDuration;      // Gap between dits/dahs in same letter
    letterGap = ditDuration * 3;   // Gap between letters
    wordGap = ditDuration * 7;     // Gap between words
  }
};

// Play a single dit or dah
void playDit(int wpm, int toneFreq = TONE_SIDETONE) {
  int duration = DIT_DURATION(wpm);
  playTone(toneFreq, duration);
}

void playDah(int wpm, int toneFreq = TONE_SIDETONE) {
  int duration = DIT_DURATION(wpm) * 3;
  playTone(toneFreq, duration);
}

// Play morse code pattern for a single character
void playMorseChar(char c, int wpm, int toneFreq = TONE_SIDETONE) {
  const char* pattern = getMorseCode(c);
  if (pattern == nullptr) {
    return; // Skip unknown characters
  }

  MorseTiming timing(wpm);

  // Play each element in the pattern
  for (int i = 0; pattern[i] != '\0'; i++) {
    if (pattern[i] == '.') {
      playDit(wpm, toneFreq);
    } else if (pattern[i] == '-') {
      playDah(wpm, toneFreq);
    }

    // Gap between elements (unless last element)
    if (pattern[i + 1] != '\0') {
      delay(timing.elementGap);
    }
  }
}

// Play morse code for a complete string
void playMorseString(const char* str, int wpm, int toneFreq = TONE_SIDETONE) {
  MorseTiming timing(wpm);

  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == ' ') {
      // Space between words (already have letter gap from previous char)
      delay(timing.wordGap - timing.letterGap);
    } else {
      playMorseChar(str[i], wpm, toneFreq);

      // Gap between letters (unless last character or next is space)
      if (str[i + 1] != '\0' && str[i + 1] != ' ') {
        delay(timing.letterGap);
      }
    }
  }
}

#endif // MORSE_CODE_H
