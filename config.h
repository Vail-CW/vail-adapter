#ifndef CONFIG_H
#define CONFIG_H

// --- SELECT YOUR HARDWARE CONFIGURATION ---
// Uncomment only one of the following lines based on your build:
// #define V1_PCB
// #define V1_2_PCB
#define V2_ADVANCED_PCB
// #define NO_PCB_GITHUB_SPECS

// --- PIN DEFINITIONS BASED ON SELECTION ---

#ifdef V1_PCB
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
  #define BOARD_NAME "V1 PCB"
  // Radio pins not defined, so HAS_RADIO_OUTPUT will not be defined
#endif

#ifdef V1_2_PCB
  #define DIT_PIN 2
  #define DAH_PIN 1
  #define KEY_PIN 0
  #define QT_DIT_PIN A6
  #define QT_DAH_PIN A7
  #define QT_KEY_PIN A8
  #define PIEZO_PIN 10
  #define BUTTON_PIN 3  // R2R button ladder input (pin 3 on QT Py)
  #define LED_ON true
  #define LED_OFF (!LED_ON)
  #define BOARD_NAME "V1_2 PCB"
  // Radio pins not defined, so HAS_RADIO_OUTPUT will not be defined
#endif

#ifdef V2_ADVANCED_PCB
  #define DIT_PIN 1
  #define DAH_PIN 0
  #define KEY_PIN 9
  #define QT_DIT_PIN A7
  #define QT_DAH_PIN A6
  #define QT_KEY_PIN A8
  #define PIEZO_PIN 10
  #define BUTTON_PIN 3  // R2R button ladder input (pin 3 on QT Py)
  #define LED_ON true
  #define LED_OFF (!LED_ON)
  #define BOARD_NAME "V2 Advanced PCB"
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
#define SECOND (1000 * MILLISECOND)

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
#define EEPROM_VALID_VALUE 0x42

// Feature activation thresholds
#define DIT_HOLD_BUZZER_DISABLE_THRESHOLD 5000   // 5 seconds
#define DAH_SPAM_COUNT_RADIO_MODE 10
#define DAH_SPAM_WINDOW 500
#define DAH_HOLD_RADIO_KEYER_TOGGLE_THRESHOLD 5000  // 5 seconds in radio mode
#define KEY_HOLD_DISABLE_THRESHOLD 6000

#endif // CONFIG_H
