// Copyright 2020 Neale Pickett
// Distributed under the MIT license
// Please see https://github.com/nealey/vail-adapter/

// MIDIUSB - Version: Latest 
#include <MIDIUSB.h>
#include <Keyboard.h>
#include <Adafruit_FreeTouch.h>
#include <FlashStorage_SAMD.h>
#include "bounce2.h"
#include "touchbounce.h"
#include "adapter.h"

#define DIT_PIN 2
#define DAH_PIN 1
#define KEY_PIN 0
#define QT_DIT_PIN A6
#define QT_DAH_PIN A7
#define QT_KEY_PIN A8
#define PIEZO 10
#define LED_ON false // Xiao inverts this logic for some reason
#define LED_OFF (!LED_ON)

#define DIT_KEYBOARD_KEY KEY_LEFT_CTRL
#define DAH_KEYBOARD_KEY KEY_RIGHT_CTRL
#define TONE 3000

#define MILLISECOND 1
#define SECOND (1 * MILLISECOND)

// EEPROM definitions
#define EEPROM_KEYER_TYPE_ADDR 0      // Address for keyer type (1 byte)
#define EEPROM_DIT_DURATION_ADDR 1    // Address for dit duration (2 bytes)
#define EEPROM_TX_NOTE_ADDR 3         // Address for TX tone/note (1 byte)
#define EEPROM_VALID_FLAG_ADDR 4      // Address for valid flag (1 byte)
#define EEPROM_VALID_VALUE 0x42       // Magic value to indicate EEPROM is initialized

bool trs = false; // true if a TRS plug is in a TRRS jack
uint16_t iambicDelay = 80 * MILLISECOND;
Bounce dit = Bounce();
Bounce dah = Bounce();
Bounce key = Bounce();
TouchBounce qt_dit = TouchBounce();
TouchBounce qt_dah = TouchBounce();
TouchBounce qt_key = TouchBounce();
VailAdapter adapter = VailAdapter(PIEZO);

// Function to save settings to EEPROM - this is only in the main file
void saveSettingsToEEPROM(uint8_t keyerType, uint16_t ditDuration, uint8_t txNote) {
  EEPROM.write(EEPROM_KEYER_TYPE_ADDR, keyerType);
  EEPROM.put(EEPROM_DIT_DURATION_ADDR, ditDuration);
  EEPROM.write(EEPROM_TX_NOTE_ADDR, txNote);
  EEPROM.write(EEPROM_VALID_FLAG_ADDR, EEPROM_VALID_VALUE);
  // Make sure data is committed to EEPROM
  EEPROM.commit();
}

// Function to load settings from EEPROM - this is only in the main file
void loadSettingsFromEEPROM() {
  // Check if EEPROM has been initialized with our flag
  if (EEPROM.read(EEPROM_VALID_FLAG_ADDR) == EEPROM_VALID_VALUE) {
    // Read keyer type
    uint8_t keyerType = EEPROM.read(EEPROM_KEYER_TYPE_ADDR);
    
    // Read dit duration (stored as 2 bytes)
    uint16_t ditDuration;
    EEPROM.get(EEPROM_DIT_DURATION_ADDR, ditDuration);
    
    // Read TX note
    uint8_t txNote = EEPROM.read(EEPROM_TX_NOTE_ADDR);
    
    // Apply the settings to the adapter
    midiEventPacket_t event;
    
    // Set dit duration
    event.header = 0x0B;
    event.byte1 = 0xB0; // Controller Change
    event.byte2 = 1;    // Dit duration controller
    event.byte3 = ditDuration / 2; // Convert back to MIDI value (0-254)
    adapter.HandleMIDI(event);
    
    // Set TX note
    event.header = 0x0B;
    event.byte1 = 0xB0; // Controller Change
    event.byte2 = 2;    // TX note controller
    event.byte3 = txNote;
    adapter.HandleMIDI(event);
    
    // Set keyer type if it's valid (between 1-9)
    if (keyerType >= 1 && keyerType <= 9) {
      event.header = 0x0C;
      event.byte1 = 0xC0; // Program Change
      event.byte2 = keyerType;
      event.byte3 = 0;
      adapter.HandleMIDI(event);
    }
  } else {
    // Initialize EEPROM with default values
    EEPROM.write(EEPROM_KEYER_TYPE_ADDR, 1);    // Default to straight key
    EEPROM.put(EEPROM_DIT_DURATION_ADDR, (uint16_t)100); // Default dit duration
    EEPROM.write(EEPROM_TX_NOTE_ADDR, 69);      // Default note (A4 = 440Hz)
    EEPROM.write(EEPROM_VALID_FLAG_ADDR, EEPROM_VALID_VALUE);
    // Make sure data is committed to EEPROM
    EEPROM.commit();
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  dit.attach(DIT_PIN, INPUT_PULLUP);
  dah.attach(DAH_PIN, INPUT_PULLUP);
  key.attach(KEY_PIN, INPUT_PULLUP);
  qt_dit.attach(QT_DIT_PIN);
  qt_dah.attach(QT_DAH_PIN);
  qt_key.attach(QT_KEY_PIN);

  // Load settings from EEPROM
  loadSettingsFromEEPROM();

  Keyboard.begin();

  // To auto-sense a straight key in a TRRS jack,
  // we just check to see if DAH is closed. 
  // The sleeve on the straight key's TRS plug
  // will short the second ring to the sleeve.
  for (int i = 0; i < 16; i++) {
    delay(20);
    dah.update();
  }
  if (dah.read() == LOW) {
    trs = true;
    key = dit;
  }
}

// A reentrant doodad to blink out the letter V at startup
// After startup, display the status of the keyboard
#define HELLO_BITS 0b0000101010111000
void setLED() {
  static bool beepin = false;
  int beat = millis() / iambicDelay;
  bool on = adapter.KeyboardMode(); // If we're not in intro, display status of keyboard

  if (beat < 16) {
    on = HELLO_BITS & (1 << (15-beat));
    if (on != beepin) {
      if (on) {
        tone(PIEZO, TONE);
      } else {
        noTone(PIEZO);
      }
      beepin = on;
    }
  }
  
  digitalWrite(LED_BUILTIN, on?LED_ON:LED_OFF);
}

void loop() {
  unsigned now = millis();  
  midiEventPacket_t event = MidiUSB.read();

  setLED();
  adapter.Tick(now);

  if (event.header) {
    adapter.HandleMIDI(event);
  }

  // Monitor straight key pin
  if (key.update() || qt_key.update()) {
    bool pressed = !key.read() || qt_key.read();
    adapter.HandlePaddle(PADDLE_STRAIGHT, pressed);
  }

  // If we made dit = dah, we have a straight key on the dit pin,
  // so we skip other keys polling.
  if (trs) {
    return;
  }

  if (dit.update() || qt_dit.update()) {
    bool pressed = !dit.read() || qt_dit.read();
    adapter.HandlePaddle(PADDLE_DIT, pressed);
  }
  
  if (dah.update() || qt_dah.update()) {
    bool pressed = !dah.read() || qt_dah.read();
    adapter.HandlePaddle(PADDLE_DAH, pressed);
  }
}