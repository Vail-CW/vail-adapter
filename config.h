#ifndef CONFIG_H
#define CONFIG_H

// --- SELECT YOUR HARDWARE CONFIGURATION ---
// Uncomment only one of the following lines based on your build:
// #define V1_Basic_PCB
// #define V2_Basic_PCB
// #define Advanced_PCB
#define NO_PCB_GITHUB_SPECS
// #define TRRS_TRINKEY
// #define ARDUINO_MICRO_BOARD

// --- PIN DEFINITIONS BASED ON SELECTION ---

#ifdef V1_Basic_PCB
  #define DIT_PIN 1
  #define DAH_PIN 0
  #define KEY_PIN 9
  #define QT_DIT_PIN A6
  #define QT_DAH_PIN A7
  #define QT_KEY_PIN A8
  #define PIEZO_PIN 10
  #define BUTTON_PIN 3  // R2R button ladder input (pin 3 on QT Py)
  #define LED_ON true
  #define LED_OFF (!LED_ON)
  #define BOARD_NAME "V1 Basic PCB"
  // Radio pins not defined, so HAS_RADIO_OUTPUT will not be defined
#endif

#ifdef V2_Basic_PCB
  #define DIT_PIN 2
  #define DAH_PIN 1
  #define KEY_PIN 0
  #define QT_DIT_PIN A6
  #define QT_DAH_PIN A7
  // #define QT_KEY_PIN A8  // Not used on V2 Basic PCB with button hat
  #define PIEZO_PIN 10
  #define BUTTON_PIN 8  // R2R button ladder input (A8, requires button hat wire mod from A4)
  #define LED_ON true
  #define LED_OFF (!LED_ON)
  #define BOARD_NAME "V2 Basic PCB"
  // Radio pins not defined, so HAS_RADIO_OUTPUT will not be defined
#endif

#ifdef Advanced_PCB
  #define DIT_PIN 1
  #define DAH_PIN 0
  #define KEY_PIN 9
  #define QT_DIT_PIN A7
  #define QT_DAH_PIN A6
  // #define QT_KEY_PIN A8  // Not used on Advanced PCB
  #define PIEZO_PIN 10
  #define BUTTON_PIN 8  // R2R button ladder input (A8 to avoid conflict with radio pins A2/A3)
  #define LED_ON true
  #define LED_OFF (!LED_ON)
  #define BOARD_NAME "Advanced PCB"
  // Radio Output Pins for Advanced PCB
  #define RADIO_DIT_PIN A3
  #define RADIO_DAH_PIN A2
  #define HAS_RADIO_OUTPUT // Signal that radio output pins are configured
#endif

#ifdef NO_PCB_GITHUB_SPECS
  #define DIT_PIN 2
  #define DAH_PIN 1
  #define KEY_PIN 0
  #define QT_DIT_PIN A6
  #define QT_DAH_PIN A7
  #define QT_KEY_PIN A8
  #define PIEZO_PIN 10
  #define BUTTON_PIN 3  // R2R button ladder input (pin 3 on QT Py)
  #define LED_ON false // Xiao inverts this logic
  #define LED_OFF (!LED_ON)
  #define BOARD_NAME "No PCB (GitHub Specs)"
  // Radio pins not defined, so HAS_RADIO_OUTPUT will not be defined
#endif

#ifdef ARDUINO_MICRO_BOARD
  // Arduino Micro (ATmega32U4) — AVR/5V board.
  // This target is memory-constrained and plug-and-play only:
  //   - No capacitive touch hardware (ATmega32U4 lacks FreeTouch)
  //   - No button menu (no resistor ladder supported in current design)
  //   - No built-in LED state changes
  //   - 1024 bytes EEPROM (vs 16KB Flash-emulated on SAMD21)
  #define DIT_PIN 2
  #define DAH_PIN 1
  #define KEY_PIN 0
  #define PIEZO_PIN 10
  // BUTTON_PIN intentionally not defined — no menu on Micro
  #define NO_CAPACITIVE_TOUCH
  #define NO_LED
  #define LED_ON true
  #define LED_OFF (!LED_ON)
  #define BOARD_NAME "Arduino Micro"
  // Optional radio outputs on Micro (A2/A3 are 5V — verify radio tolerance):
  // #define RADIO_DIT_PIN A3
  // #define RADIO_DAH_PIN A2
  // #define HAS_RADIO_OUTPUT

  // EEPROM/RAM-constrained: ATmega32U4 has 1024 bytes EEPROM (vs 16KB on SAMD21)
  // and only 2560 bytes RAM. Shrink CW memory slot dimensions to fit.
  //   3 slots × (100 transitions × 2 bytes + 2 length bytes) = 606 bytes EEPROM
  //   Plus 6 bytes settings = 612 bytes EEPROM used / 1024 available.
  //   Each in-RAM CWMemory is 202 bytes; 3 slots + RecordingState ≈ ~800 bytes RAM.
  #define MAX_MEMORY_SLOTS 3
  #define MAX_TRANSITIONS_PER_MEMORY 100
  #define MAX_RECORDING_DURATION_MS 12000
#endif

#ifdef TRRS_TRINKEY
  // TRRS Jack Pins (using TIP and RING1 for dit/dah)
  #define DIT_PIN 0       // PIN_TIP - connected to tip of TRRS jack
  #define DAH_PIN 2       // PIN_RING1 - connected to ring 1 of TRRS jack
  #define KEY_PIN 0       // Same as DIT_PIN for straight key mode (TRS cable uses tip only)
  // TRRS Jack ground pins - must be driven LOW for proper operation
  #define SLEEVE_PIN 5    // PIN_SLEEVE - ground for TRS/TRRS cable
  #define RING2_PIN 4     // PIN_RING2 - additional ground (optional)
  // Trinkey doesn't have capacitive touch hardware - disable it
  #define NO_CAPACITIVE_TOUCH
  #define PIEZO_PIN 7     // SDA/PA08 on STEMMA QT connector (D7) - wire buzzer here
  // Trinkey uses NeoPixel (requires special library) - disable LED control
  #define NO_LED
  #define LED_BUILTIN 1   // Define but don't use - Trinkey has NeoPixel on pin 1
  #define LED_ON true
  #define LED_OFF (!LED_ON)
  #define BOARD_NAME "TRRS Trinkey"
  // No button ladder on Trinkey - menu functionality not available
  // BUTTON_PIN intentionally not defined
  // Radio pins not defined, so HAS_RADIO_OUTPUT will not be defined
#endif

// --- RADIO KEYING POLARITY ---
// Set to true if your radio PTT/KEY line activates when pulled LOW.
// Set to false if your radio PTT/KEY line activates when pulled HIGH.
#define RADIO_KEYING_ACTIVE_LOW false // <<<< ADJUST THIS FOR YOUR RADIO

#ifdef HAS_RADIO_OUTPUT
  #if RADIO_KEYING_ACTIVE_LOW
    #define RADIO_ACTIVE_LEVEL LOW
    #define RADIO_INACTIVE_LEVEL HIGH
  #else
    #define RADIO_ACTIVE_LEVEL HIGH
    #define RADIO_INACTIVE_LEVEL LOW
  #endif
#endif


// --- COMMON DEFINITIONS ---
#define DIT_KEYBOARD_KEY KEY_LEFT_CTRL
#define DAH_KEYBOARD_KEY KEY_RIGHT_CTRL
#define DEFAULT_TONE_NOTE 69
#define DEFAULT_ADAPTER_DIT_DURATION_MS 100

#define MILLISECOND 1
// NOTE: a `#define SECOND (1000 * MILLISECOND)` used to live here but was
// never referenced in the codebase. Recent Adafruit FreeTouch / ASF headers
// now define SECOND as an enum/constant, which collides with a macro
// redefinition and breaks every SAMD21 compile that includes FreeTouch.
// (Trinkey and Micro skip FreeTouch via NO_CAPACITIVE_TOUCH and were fine.)

// Morse code timing at 20 WPM (for startup sound, not for keyer logic directly)
#define DOT_DURATION 60
#define DASH_DURATION (DOT_DURATION * 3)
#define ELEMENT_SPACE (DOT_DURATION)
#define CHAR_SPACE (DOT_DURATION * 3)
#define WORD_SPACE (DOT_DURATION * 7)

// EEPROM definitions
#define EEPROM_KEYER_TYPE_ADDR 0
#define EEPROM_DIT_DURATION_ADDR 1
#define EEPROM_TX_NOTE_ADDR 3
#define EEPROM_VALID_FLAG_ADDR 4
#define EEPROM_RADIO_KEYER_MODE_ADDR 5
#define EEPROM_PADDLES_SWAPPED_ADDR 6
#define EEPROM_VALID_VALUE 0x42

// Feature activation thresholds
#define DIT_HOLD_BUZZER_DISABLE_THRESHOLD 5000   // 5 seconds
#define DAH_SPAM_COUNT_RADIO_MODE 10
#define DAH_SPAM_WINDOW 500
#define DAH_HOLD_RADIO_KEYER_TOGGLE_THRESHOLD 5000  // 5 seconds in radio mode
#define KEY_HOLD_DISABLE_THRESHOLD 6000

#endif // CONFIG_H
