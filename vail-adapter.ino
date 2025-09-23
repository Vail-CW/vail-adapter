#include "config.h" 

#include <MIDIUSB.h>
#include <Keyboard.h>
#include <Adafruit_FreeTouch.h>
#include <FlashStorage_SAMD.h>
#include "bounce2.h"
#include "touchbounce.h"
#include "adapter.h"
#include "equal_temperament.h"

bool trs = false;

Bounce dit = Bounce();
Bounce dah = Bounce();
Bounce key = Bounce(); 
TouchBounce qt_dit = TouchBounce();
TouchBounce qt_dah = TouchBounce();
TouchBounce qt_key = TouchBounce(); 

VailAdapter adapter = VailAdapter(PIEZO_PIN);

uint8_t loadToneFromEEPROM(); 

void playDot(uint8_t noteNumber) {
  digitalWrite(LED_BUILTIN, LED_ON);
  tone(PIEZO_PIN, equalTemperamentNote[noteNumber]);
  delay(DOT_DURATION);
  digitalWrite(LED_BUILTIN, LED_OFF);
  noTone(PIEZO_PIN);
  delay(ELEMENT_SPACE);
}

void playDash(uint8_t noteNumber) {
  digitalWrite(LED_BUILTIN, LED_ON);
  tone(PIEZO_PIN, equalTemperamentNote[noteNumber]);
  delay(DASH_DURATION);
  digitalWrite(LED_BUILTIN, LED_OFF);
  noTone(PIEZO_PIN);
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
  noTone(PIEZO_PIN);
}

uint8_t loadToneFromEEPROM() {
  if (EEPROM.read(EEPROM_VALID_FLAG_ADDR) == EEPROM_VALID_VALUE) {
    uint8_t txNote = EEPROM.read(EEPROM_TX_NOTE_ADDR);
    return txNote;
  } else {
    Serial.println("EEPROM not initialized, using default tone");
    return DEFAULT_TONE_NOTE;
  }
}

void saveSettingsToEEPROM(uint8_t keyerType, uint16_t ditDuration, uint8_t txNote) {
  EEPROM.write(EEPROM_KEYER_TYPE_ADDR, keyerType);
  EEPROM.put(EEPROM_DIT_DURATION_ADDR, ditDuration);
  EEPROM.write(EEPROM_TX_NOTE_ADDR, txNote);
  EEPROM.write(EEPROM_VALID_FLAG_ADDR, EEPROM_VALID_VALUE);
  EEPROM.commit();
  Serial.print("Saved to EEPROM - Keyer: "); Serial.print(keyerType);
  Serial.print(", Dit Duration: "); Serial.print(ditDuration);
  Serial.print(", TX Note: "); Serial.println(txNote);
}

void loadSettingsFromEEPROM() {
  if (EEPROM.read(EEPROM_VALID_FLAG_ADDR) == EEPROM_VALID_VALUE) {
    uint8_t keyerType = EEPROM.read(EEPROM_KEYER_TYPE_ADDR);
    uint16_t ditDurationVal; 
    EEPROM.get(EEPROM_DIT_DURATION_ADDR, ditDurationVal);
    uint8_t txNoteVal = EEPROM.read(EEPROM_TX_NOTE_ADDR); 

    Serial.print("EEPROM values - Keyer: "); Serial.print(keyerType);
    Serial.print(", Dit Duration: "); Serial.print(ditDurationVal);
    Serial.print(", TX Note: "); Serial.println(txNoteVal);

    midiEventPacket_t event;
    event.header = 0x0B; event.byte1 = 0xB0; 
    event.byte2 = 1; 
    event.byte3 = ditDurationVal / (2 * MILLISECOND); 
    adapter.HandleMIDI(event);

    event.byte2 = 2; 
    event.byte3 = txNoteVal; 
    adapter.HandleMIDI(event);

    if (keyerType >= 0 && keyerType <= 9) { 
      event.header = 0x0C; event.byte1 = 0xC0; 
      event.byte2 = keyerType; event.byte3 = 0;
      adapter.HandleMIDI(event);
    }
  } else {
    Serial.println("EEPROM initializing with default values...");
    EEPROM.write(EEPROM_KEYER_TYPE_ADDR, 1); 
    EEPROM.put(EEPROM_DIT_DURATION_ADDR, (uint16_t)DEFAULT_ADAPTER_DIT_DURATION_MS);
    EEPROM.write(EEPROM_TX_NOTE_ADDR, DEFAULT_TONE_NOTE);
    EEPROM.write(EEPROM_VALID_FLAG_ADDR, EEPROM_VALID_VALUE);
    EEPROM.commit();
    Serial.println("EEPROM initialized. Loading these defaults now.");
    loadSettingsFromEEPROM(); 
  }
}

void setup() {
  Serial.begin(9600);
  delay(500); 
  Serial.print("\n\nVail Adapter starting on: ");
  Serial.println(BOARD_NAME);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF); 

  dit.attach(DIT_PIN, INPUT_PULLUP);
  dah.attach(DAH_PIN, INPUT_PULLUP);
  key.attach(KEY_PIN, INPUT_PULLUP);
  
  qt_dit.attach(QT_DIT_PIN);
  qt_dah.attach(QT_DAH_PIN);
  qt_key.attach(QT_KEY_PIN);

#ifdef HAS_RADIO_OUTPUT
  pinMode(RADIO_DIT_PIN, OUTPUT);
  pinMode(RADIO_DAH_PIN, OUTPUT);
  digitalWrite(RADIO_DIT_PIN, RADIO_INACTIVE_LEVEL); // Use configured inactive level
  digitalWrite(RADIO_DAH_PIN, RADIO_INACTIVE_LEVEL); // Use configured inactive level
  Serial.print("Radio Output Pins Initialized. Inactive Level: ");
  Serial.println(RADIO_INACTIVE_LEVEL == LOW ? "LOW" : "HIGH");
#endif

  uint8_t startupTone = loadToneFromEEPROM(); 
  Serial.println("Playing VAIL in Morse code at 20 WPM");
  playVAIL(startupTone);
  
  loadSettingsFromEEPROM(); 

  Serial.print("Adapter settings loaded - Keyer: "); Serial.print(adapter.getCurrentKeyerType());
  Serial.print(", Dit Duration (ms): "); Serial.print(adapter.getDitDuration());
  Serial.print(", TX Note: "); Serial.println(adapter.getTxNote());
  Serial.print("Buzzer initially: "); Serial.println(adapter.isBuzzerEnabled() ? "ON" : "OFF");
  Serial.print("Radio Mode initially: "); Serial.println(adapter.isRadioModeActive() ? "ON" : "OFF");

  Keyboard.begin();
  MidiUSB.flush(); 

  for (int i = 0; i < 16; i++) { 
    delay(20); 
    dah.update(); 
  }
  if (dah.read() == LOW) { 
    trs = true;
    Serial.println("TRS plug potentially detected (DAH pin grounded).");
  }
}

void setLED() {
  bool finalLedState = false; 

  if (adapter.isRadioModeActive()) {
    finalLedState = (millis() % 400 < 200); 
  } else if (!adapter.isBuzzerEnabled()) {
    finalLedState = (millis() % 2000 < 1000); 
  } else {
    finalLedState = adapter.KeyboardMode();
  }
  digitalWrite(LED_BUILTIN, finalLedState ? LED_ON : LED_OFF);
}

void loop() {
  unsigned int currentTime = millis(); 
  midiEventPacket_t event = MidiUSB.read();

  setLED();
  adapter.Tick(currentTime);

  if (event.header) {
    adapter.HandleMIDI(event);
  }

  if (key.update()) { 
    adapter.ProcessPaddleInput(PADDLE_STRAIGHT, !key.read(), false); 
  }

  if (trs) {
      // If DAH pin is grounded (TRS), this suggests a straight key might be plugged in
      // where DIT line (tip) is the key and DAH line (ring) is shorted to GND (sleeve).
      // The current Bounce objects 'dit' and 'dah' are still attached to their original pins.
      // If your TRS straight key uses the DIT_PIN for keying and grounds DAH_PIN:
      // You might want to only read the 'dit' object as a straight key when trs is true.
      // if (dit.update()) {
      //    adapter.ProcessPaddleInput(PADDLE_STRAIGHT, !dit.read(), false);
      // }
      // And then skip the separate DIT/DAH paddle processing below if trs == true.
      // This part depends on your exact TRS wiring and desired behavior.
      // For now, assuming all inputs are polled and 'trs' is just an indicator.
  }


  if (dit.update()) {
    adapter.ProcessPaddleInput(PADDLE_DIT, !dit.read(), false);
  }
  if (dah.update()) {
    adapter.ProcessPaddleInput(PADDLE_DAH, !dah.read(), false);
  }

  if (qt_key.update()) {
    adapter.ProcessPaddleInput(PADDLE_STRAIGHT, qt_key.read(), true); 
  }
  if (qt_dit.update()) {
    adapter.ProcessPaddleInput(PADDLE_DIT, qt_dit.read(), true);
  }
  if (qt_dah.update()) {
    adapter.ProcessPaddleInput(PADDLE_DAH, qt_dah.read(), true);
  }
}