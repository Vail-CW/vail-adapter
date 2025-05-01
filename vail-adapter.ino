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
#include "equal_temperament.h"

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
#define DEFAULT_TONE_NOTE 69  // Default MIDI note (A4 = 440Hz)

#define MILLISECOND 1
#define SECOND (1 * MILLISECOND)

// Morse code timing at 20 WPM
#define DOT_DURATION 60
#define DASH_DURATION (DOT_DURATION * 3)
#define ELEMENT_SPACE (DOT_DURATION)
#define CHAR_SPACE (DOT_DURATION * 3)
#define WORD_SPACE (DOT_DURATION * 7)

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

// Pre-declare functions we'll need early
uint8_t loadToneFromEEPROM();

// Function to play Morse code dot with a specific MIDI note
void playDot(uint8_t noteNumber) {
  digitalWrite(LED_BUILTIN, LED_ON);
  tone(PIEZO, equalTemperamentNote[noteNumber]);
  delay(DOT_DURATION);
  digitalWrite(LED_BUILTIN, LED_OFF);
  noTone(PIEZO);
  delay(ELEMENT_SPACE);
}

// Function to play Morse code dash with a specific MIDI note
void playDash(uint8_t noteNumber) {
  digitalWrite(LED_BUILTIN, LED_ON);
  tone(PIEZO, equalTemperamentNote[noteNumber]);
  delay(DASH_DURATION);
  digitalWrite(LED_BUILTIN, LED_OFF);
  noTone(PIEZO);
  delay(ELEMENT_SPACE);
}

// Function to play "VAIL" in Morse code at 20 WPM with a specific MIDI note
void playVAIL(uint8_t noteNumber) {
  // V: ···−
  playDot(noteNumber);
  playDot(noteNumber);
  playDot(noteNumber);
  playDash(noteNumber);
  delay(CHAR_SPACE - ELEMENT_SPACE);  // Subtract the element space already added
  
  // A: ·−
  playDot(noteNumber);
  playDash(noteNumber);
  delay(CHAR_SPACE - ELEMENT_SPACE);  // Subtract the element space already added
  
  // I: ··
  playDot(noteNumber);
  playDot(noteNumber);
  delay(CHAR_SPACE - ELEMENT_SPACE);  // Subtract the element space already added
  
  // L: ·−··
  playDot(noteNumber);
  playDash(noteNumber);
  playDot(noteNumber);
  playDot(noteNumber);
  // Make sure all tones are off
  noTone(PIEZO);
}

// Function to load just the tone note from EEPROM
uint8_t loadToneFromEEPROM() {
  // Check if EEPROM has been initialized with our flag
  if (EEPROM.read(EEPROM_VALID_FLAG_ADDR) == EEPROM_VALID_VALUE) {
    // Read TX note
    uint8_t txNote = EEPROM.read(EEPROM_TX_NOTE_ADDR);
    Serial.print("EEPROM tone note: ");
    Serial.println(txNote);
    return txNote;
  } else {
    // Return default note if EEPROM not initialized
    Serial.println("EEPROM not initialized, using default tone");
    return DEFAULT_TONE_NOTE;
  }
}

// Function to save settings to EEPROM - this is only in the main file
void saveSettingsToEEPROM(uint8_t keyerType, uint16_t ditDuration, uint8_t txNote) {
  EEPROM.write(EEPROM_KEYER_TYPE_ADDR, keyerType);
  EEPROM.put(EEPROM_DIT_DURATION_ADDR, ditDuration);
  EEPROM.write(EEPROM_TX_NOTE_ADDR, txNote);
  EEPROM.write(EEPROM_VALID_FLAG_ADDR, EEPROM_VALID_VALUE);
  // Make sure data is committed to EEPROM
  EEPROM.commit();
  
  // Debug output
  Serial.print("Saved to EEPROM - Keyer: ");
  Serial.print(keyerType);
  Serial.print(", Dit Duration: ");
  Serial.print(ditDuration);
  Serial.print(", TX Note: ");
  Serial.println(txNote);
}

// Function to load all settings from EEPROM - this is only in the main file
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
    
    Serial.print("EEPROM values - Keyer: ");
    Serial.print(keyerType);
    Serial.print(", Dit Duration: ");
    Serial.print(ditDuration);
    Serial.print(", TX Note: ");
    Serial.println(txNote);
    
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
    EEPROM.write(EEPROM_TX_NOTE_ADDR, DEFAULT_TONE_NOTE); // Default note (A4 = 440Hz)
    EEPROM.write(EEPROM_VALID_FLAG_ADDR, EEPROM_VALID_VALUE);
    // Make sure data is committed to EEPROM
    EEPROM.commit();
    
    Serial.println("EEPROM initialized with default values");
  }
}

void setup() {
  // Initialize serial for debugging
  Serial.begin(9600);
  delay(300);
  Serial.println("Vail Adapter starting...");
  
  pinMode(LED_BUILTIN, OUTPUT);
  dit.attach(DIT_PIN, INPUT_PULLUP);
  dah.attach(DAH_PIN, INPUT_PULLUP);
  key.attach(KEY_PIN, INPUT_PULLUP);
  qt_dit.attach(QT_DIT_PIN);
  qt_dah.attach(QT_DAH_PIN);
  qt_key.attach(QT_KEY_PIN);

  // First, load just the tone from EEPROM for our startup sound
  uint8_t startupTone = loadToneFromEEPROM();
  
  // Play VAIL in Morse code with the EEPROM tone
  Serial.println("Playing VAIL in Morse code at 20 WPM");
  playVAIL(startupTone);
  
  // Now load all settings from EEPROM
  loadSettingsFromEEPROM();

  // Print loaded settings for debugging
  Serial.print("Current adapter settings - Keyer: ");
  Serial.print(adapter.getCurrentKeyerType());
  Serial.print(", Dit Duration: ");
  Serial.print(adapter.getDitDuration());
  Serial.print(", TX Note: ");
  Serial.println(adapter.getTxNote());

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
    Serial.println("TRS plug detected, using straight key mode");
  }
  
  // We'll skip the test tone here since we already played the VAIL morse
}

// LED status function - shows keyboard mode or buzzer disabled status
void setLED() {
  bool on = adapter.KeyboardMode(); // Base LED state on keyboard mode
  
  // If buzzer is disabled, blink the LED to indicate
  if (!adapter.isBuzzerEnabled()) {
    // Blink the LED with a pattern (fast double blink every 2 seconds)
    unsigned long currentMillis = millis();
    unsigned long blinkCycle = currentMillis % 2000;
    
    // Double blink pattern (on for 100ms, off for 100ms, on for 100ms, off for 1700ms)
    if (blinkCycle < 100 || (blinkCycle >= 200 && blinkCycle < 300)) {
      on = true;  // LED on during blink periods
    } else {
      on = false; // LED off during non-blink periods
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
    Serial.print("MIDI event received: header=");
    Serial.print(event.header);
    Serial.print(", byte1=");
    Serial.print(event.byte1);
    Serial.print(", byte2=");
    Serial.print(event.byte2);
    Serial.print(", byte3=");
    Serial.println(event.byte3);
    
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
