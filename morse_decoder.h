/*
 * Morse Code Decoder for Keyboard Sim Mode
 *
 * Ported from morse-pro library by Stephen C. Phillips
 * Original: https://github.com/scp93ch/morse-pro
 * Licensed under EUPL
 *
 * Decodes real-time Morse code input into characters for USB keyboard output.
 * Supports adaptive speed detection for varying sending speeds.
 */

#ifndef MORSE_DECODER_H
#define MORSE_DECODER_H

#include <Arduino.h>

// Morse code lookup table entry
// Pattern is stored as bits: 0 = dit, 1 = dah
// Bits are stored LSB first (first element in bit 0)
struct MorseEntry {
  uint8_t pattern;   // Bit pattern (0=dit, 1=dah), LSB first
  uint8_t length;    // Number of elements (1-8)
  char character;    // ASCII character to output
};

// Special character codes (not ASCII)
#define MORSE_BACKSPACE 0x08  // 8 dits
#define MORSE_ENTER     0x0A  // BT prosign (-...-)
#define MORSE_SPACE     0x20  // Word space

// Morse lookup table - pattern to character mapping
// Patterns: bit 0 = first element, 0 = dit, 1 = dah
// Example: A = ".-" = 0b10 (dit=0 in bit 0, dah=1 in bit 1), length 2
static const MorseEntry morseTable[] PROGMEM = {
  // Letters A-Z
  // A = .- : dit(0), dah(1) -> 0b10
  {0b10, 2, 'A'},
  // B = -... : dah(1), dit(0), dit(0), dit(0) -> 0b0001
  {0b0001, 4, 'B'},
  // C = -.-. : dah(1), dit(0), dah(1), dit(0) -> 0b0101
  {0b0101, 4, 'C'},
  // D = -.. : dah(1), dit(0), dit(0) -> 0b001
  {0b001, 3, 'D'},
  // E = . : dit(0) -> 0b0
  {0b0, 1, 'E'},
  // F = ..-. : dit(0), dit(0), dah(1), dit(0) -> 0b0100
  {0b0100, 4, 'F'},
  // G = --. : dah(1), dah(1), dit(0) -> 0b011
  {0b011, 3, 'G'},
  // H = .... : dit(0), dit(0), dit(0), dit(0) -> 0b0000
  {0b0000, 4, 'H'},
  // I = .. : dit(0), dit(0) -> 0b00
  {0b00, 2, 'I'},
  // J = .--- : dit(0), dah(1), dah(1), dah(1) -> 0b1110
  {0b1110, 4, 'J'},
  // K = -.- : dah(1), dit(0), dah(1) -> 0b101
  {0b101, 3, 'K'},
  // L = .-.. : dit(0), dah(1), dit(0), dit(0) -> 0b0010
  {0b0010, 4, 'L'},
  // M = -- : dah(1), dah(1) -> 0b11
  {0b11, 2, 'M'},
  // N = -. : dah(1), dit(0) -> 0b01
  {0b01, 2, 'N'},
  // O = --- : dah(1), dah(1), dah(1) -> 0b111
  {0b111, 3, 'O'},
  // P = .--. : dit(0), dah(1), dah(1), dit(0) -> 0b0110
  {0b0110, 4, 'P'},
  // Q = --.- : dah(1), dah(1), dit(0), dah(1) -> 0b1011
  {0b1011, 4, 'Q'},
  // R = .-. : dit(0), dah(1), dit(0) -> 0b010
  {0b010, 3, 'R'},
  // S = ... : dit(0), dit(0), dit(0) -> 0b000
  {0b000, 3, 'S'},
  // T = - : dah(1) -> 0b1
  {0b1, 1, 'T'},
  // U = ..- : dit(0), dit(0), dah(1) -> 0b100
  {0b100, 3, 'U'},
  // V = ...- : dit(0), dit(0), dit(0), dah(1) -> 0b1000
  {0b1000, 4, 'V'},
  // W = .-- : dit(0), dah(1), dah(1) -> 0b110
  {0b110, 3, 'W'},
  // X = -..- : dah(1), dit(0), dit(0), dah(1) -> 0b1001
  {0b1001, 4, 'X'},
  // Y = -.-- : dah(1), dit(0), dah(1), dah(1) -> 0b1101
  {0b1101, 4, 'Y'},
  // Z = --.. : dah(1), dah(1), dit(0), dit(0) -> 0b0011
  {0b0011, 4, 'Z'},

  // Numbers 0-9
  // 0 = ----- : all dahs -> 0b11111
  {0b11111, 5, '0'},
  // 1 = .---- : dit, dah, dah, dah, dah -> 0b11110
  {0b11110, 5, '1'},
  // 2 = ..--- : dit, dit, dah, dah, dah -> 0b11100
  {0b11100, 5, '2'},
  // 3 = ...-- : dit, dit, dit, dah, dah -> 0b11000
  {0b11000, 5, '3'},
  // 4 = ....- : dit, dit, dit, dit, dah -> 0b10000
  {0b10000, 5, '4'},
  // 5 = ..... : all dits -> 0b00000
  {0b00000, 5, '5'},
  // 6 = -.... : dah, dit, dit, dit, dit -> 0b00001
  {0b00001, 5, '6'},
  // 7 = --... : dah, dah, dit, dit, dit -> 0b00011
  {0b00011, 5, '7'},
  // 8 = ---.. : dah, dah, dah, dit, dit -> 0b00111
  {0b00111, 5, '8'},
  // 9 = ----. : dah, dah, dah, dah, dit -> 0b01111
  {0b01111, 5, '9'},

  // Punctuation
  // . = .-.-.- : dit, dah, dit, dah, dit, dah -> 0b101010
  {0b101010, 6, '.'},
  // , = --..-- : dah, dah, dit, dit, dah, dah -> 0b110011
  {0b110011, 6, ','},
  // : = ---... : dah, dah, dah, dit, dit, dit -> 0b000111
  {0b000111, 6, ':'},
  // ? = ..--.. : dit, dit, dah, dah, dit, dit -> 0b001100
  {0b001100, 6, '?'},
  // ' = .----. : dit, dah, dah, dah, dah, dit -> 0b011110
  {0b011110, 6, '\''},
  // - = -....- : dah, dit, dit, dit, dit, dah -> 0b100001
  {0b100001, 6, '-'},
  // / = -..-. : dah, dit, dit, dah, dit -> 0b01001
  {0b01001, 5, '/'},
  // ( = -.--. : dah, dit, dah, dah, dit -> 0b01101
  {0b01101, 5, '('},
  // ) = -.--.- : dah, dit, dah, dah, dit, dah -> 0b101101
  {0b101101, 6, ')'},
  // " = .-..-. : dit, dah, dit, dit, dah, dit -> 0b010010
  {0b010010, 6, '"'},
  // @ = .--.-. : dit, dah, dah, dit, dah, dit -> 0b010110
  {0b010110, 6, '@'},
  // = = -...- : same as BT prosign, handled as SPACE in decodePattern()
  // {0b10001, 5, '='},  // Commented out - BT prosign takes priority for keyboard sim
  // & = .-... : dit, dah, dit, dit, dit -> 0b00010
  {0b00010, 5, '&'},
  // + = .-.-. : dit, dah, dit, dah, dit -> 0b01010
  {0b01010, 5, '+'},
  // ! = -.-.-- : dah, dit, dah, dit, dah, dah -> 0b110101
  {0b110101, 6, '!'},

  // Special: 8 dits for backspace
  {0b00000000, 8, MORSE_BACKSPACE},
};

#define MORSE_TABLE_SIZE (sizeof(morseTable) / sizeof(morseTable[0]))

// Adaptive timing buffer size
#define TIMING_BUFFER_SIZE 16

// Callback function types
typedef void (*CharacterCallback)(char c);
typedef void (*BackspaceCallback)();
typedef void (*EnterCallback)();
typedef void (*SpaceCallback)();
typedef void (*ErrorCallback)();
typedef void (*WordGapCallback)();

class MorseDecoder {
public:
  MorseDecoder();

  // Initialize decoder with starting dit length in ms
  void begin(uint16_t initialDitLen);

  // Reset decoder state (but keep timing calibration)
  void reset();

  // Add a timing event
  // Positive duration = key down (tone)
  // Negative duration = key up (silence)
  void addTiming(int16_t duration);

  // Called periodically to check for flush timeout
  void tick(unsigned long currentTime);

  // Force flush any pending pattern
  void flush();

  // Set callback functions
  void setCharacterCallback(CharacterCallback cb) { onCharacter = cb; }
  void setBackspaceCallback(BackspaceCallback cb) { onBackspace = cb; }
  void setEnterCallback(EnterCallback cb) { onEnter = cb; }
  void setSpaceCallback(SpaceCallback cb) { onSpace = cb; }
  void setErrorCallback(ErrorCallback cb) { onError = cb; }
  // Fires every time a word-length silence is measured, whether or not a
  // space character ends up being emitted for it.
  void setWordGapCallback(WordGapCallback cb) { onWordGap = cb; }

  // Get current estimated WPM
  uint16_t getWPM() const { return 1200 / ditLen; }

  // Set whether to adapt timing (false = use fixed WPM)
  void setAdaptive(bool adapt) { adaptive = adapt; }

  // Manually set dit length (disables adaptive for this value)
  void setDitLength(uint16_t len);

  // Notify decoder of current key state (for tick timeout logic)
  void setKeyDown(bool down) { keyIsDown = down; }

private:
  // Timing thresholds
  uint16_t ditLen;            // Expected dit length in ms
  uint16_t ditDahThreshold;   // Threshold between dit and dah
  uint16_t charSpaceThreshold; // Threshold for character boundary (2x dit)
  uint16_t dahSpaceThreshold; // Threshold between char space and word space

  // Bounds for the adaptive dit length. 25 ms is about 48 WPM, 400 ms is
  // about 3 WPM. Anything outside this is noise, not sending.
  static const uint16_t MIN_DIT_LEN = 25;
  static const uint16_t MAX_DIT_LEN = 400;

  // Key state tracking (for tick timeout)
  bool keyIsDown;

  // Pattern accumulator
  uint8_t currentPattern;     // Bits accumulated (LSB first)
  uint8_t patternLength;      // Number of elements accumulated

  // Adaptive timing buffer
  uint16_t ditBuffer[TIMING_BUFFER_SIZE];
  uint8_t bufferIndex;
  uint8_t bufferCount;
  bool adaptive;

  // State tracking
  unsigned long lastEventTime;
  bool pendingFlush;
  int16_t lastTiming;         // Last timing value (for combining)
  bool suppressNextSpace;     // Don't output space after backspace
  bool pendingWordSpace;      // Word space detected but held until pattern resolves

  // Noise threshold (ignore timings <= this)
  static const uint8_t NOISE_THRESHOLD = 5;

  // Flush timeout in ms (force decode after this silence)
  static const uint16_t FLUSH_TIMEOUT = 2000;

  // Callbacks
  CharacterCallback onCharacter;
  BackspaceCallback onBackspace;
  EnterCallback onEnter;
  SpaceCallback onSpace;
  ErrorCallback onError;
  WordGapCallback onWordGap;

  // Internal methods
  void updateThresholds();
  void updateAdaptive(uint16_t duration, bool isDah);
  void processTiming(int16_t duration, unsigned long now);
  void decodePattern(bool forceOutput = false);
  char lookupPattern(uint8_t pattern, uint8_t length);
  void addElement(bool isDah);
};

#endif // MORSE_DECODER_H
