#include "config.h"

#include <MIDIUSB.h>
#include <Keyboard.h>
#include <Adafruit_FreeTouch.h>
#include <FlashStorage_SAMD.h>
#include "bounce2.h"
#include "touchbounce.h"
#include "adapter.h"
#include "equal_temperament.h"
#include "buttons.h"

bool trs = false;

Bounce dit = Bounce();
Bounce dah = Bounce();
Bounce key = Bounce(); 
TouchBounce qt_dit = TouchBounce();
TouchBounce qt_dah = TouchBounce();
TouchBounce qt_key = TouchBounce(); 

VailAdapter adapter = VailAdapter(PIEZO_PIN);

#ifdef BUTTON_PIN
ButtonDebouncer buttonDebouncer;

// Operating modes
typedef enum {
  MODE_NORMAL = 0,
  MODE_SPEED_SETTING,
  MODE_TONE_SETTING,
  MODE_KEY_SETTING,
  MODE_MEMORY_MANAGEMENT
} OperatingMode;

OperatingMode currentMode = MODE_NORMAL;
int tempSpeedWPM = 12;  // Temporary value while adjusting (default 12 WPM = 100ms dit)
uint8_t tempToneNote = 69;  // Temporary MIDI note while adjusting (default A4 = 440Hz)
uint8_t tempKeyerType = 8;  // Temporary keyer type while adjusting (1=Straight, 7=Iambic A, 8=Iambic B)
unsigned long lastActivityTime = 0;  // For timeout tracking
#define SETTING_MODE_TIMEOUT 30000  // 30 seconds

// Valid keyer types for cycling
#define KEYER_STRAIGHT 1
#define KEYER_IAMBIC_A 7
#define KEYER_IAMBIC_B 8

// Helper functions for WPM <-> dit duration conversion
int ditDurationToWPM(uint16_t ditDuration) {
  // WPM = 1200 / dit_duration_ms
  if (ditDuration == 0) return 12;  // Safety
  return 1200 / ditDuration;
}

uint16_t wpmToDitDuration(int wpm) {
  // dit_duration_ms = 1200 / WPM
  if (wpm <= 0) return 100;  // Safety
  return 1200 / wpm;
}

void applyTemporarySpeed(int wpm) {
  // Apply speed to adapter without saving to EEPROM
  // This allows testing the speed before committing
  uint16_t newDitDuration = wpmToDitDuration(wpm);
  midiEventPacket_t event;
  event.header = 0x0B;
  event.byte1 = 0xB0;
  event.byte2 = 1;
  event.byte3 = newDitDuration / (2 * MILLISECOND);
  adapter.HandleMIDI(event);
}

void applyTemporaryTone(uint8_t noteNumber) {
  // Apply tone to adapter without saving to EEPROM
  // This allows testing the tone before committing
  midiEventPacket_t event;
  event.header = 0x0B;
  event.byte1 = 0xB0;
  event.byte2 = 2;
  event.byte3 = noteNumber;
  adapter.HandleMIDI(event);
}

void applyTemporaryKeyerType(uint8_t keyerType) {
  // Apply keyer type to adapter without saving to EEPROM
  // This allows testing the keyer type before committing
  midiEventPacket_t event;
  event.header = 0x0C;
  event.byte1 = 0xC0;
  event.byte2 = keyerType;
  event.byte3 = 0;
  adapter.HandleMIDI(event);
}

const char* getKeyerTypeName(uint8_t keyerType) {
  switch (keyerType) {
    case KEYER_STRAIGHT: return "Straight";
    case KEYER_IAMBIC_A: return "Iambic A";
    case KEYER_IAMBIC_B: return "Iambic B";
    default: return "Unknown";
  }
}

void playKeyerTypeCode(uint8_t keyerType) {
  // Play Morse code identifier for the keyer type
  uint16_t ditDur = adapter.getDitDuration();
  uint16_t dahDur = ditDur * 3;
  uint16_t elementSpace = ditDur;
  uint16_t charSpace = ditDur * 3;

  switch (keyerType) {
    case KEYER_STRAIGHT:
      // S = ... (dit dit dit)
      playMorseDit(); playMorseDit(); playMorseDit();
      break;
    case KEYER_IAMBIC_A:
      // IA = .. .- (I then A)
      // I = ..
      playMorseDit(); playMorseDit();
      delay(charSpace - elementSpace);  // Space between letters
      // A = .-
      playMorseDit(); playMorseDah();
      break;
    case KEYER_IAMBIC_B:
      // IB = .. -... (I then B)
      // I = ..
      playMorseDit(); playMorseDit();
      delay(charSpace - elementSpace);  // Space between letters
      // B = -...
      playMorseDah(); playMorseDit(); playMorseDit(); playMorseDit();
      break;
  }
}
#endif

uint8_t loadToneFromEEPROM();

#ifdef BUTTON_PIN
// Morse code playback using adapter settings (user's current WPM and tone)
void playMorseDit() {
  uint8_t note = adapter.getTxNote();
  uint16_t ditDur = adapter.getDitDuration();

  tone(PIEZO_PIN, equalTemperamentNote[note]);
  delay(ditDur);
  noTone(PIEZO_PIN);
  delay(ditDur);  // Inter-element space = 1 dit
}

void playMorseDah() {
  uint8_t note = adapter.getTxNote();
  uint16_t ditDur = adapter.getDitDuration();

  tone(PIEZO_PIN, equalTemperamentNote[note]);
  delay(ditDur * 3);
  noTone(PIEZO_PIN);
  delay(ditDur);  // Inter-element space = 1 dit
}

void playMorseChar(char c) {
  switch(c) {
    case 'A': playMorseDit(); playMorseDah(); break;
    case 'D': playMorseDah(); playMorseDit(); playMorseDit(); break;
    case 'E': playMorseDit(); break;
    case 'K': playMorseDah(); playMorseDit(); playMorseDah(); break;
    case 'M': playMorseDah(); playMorseDah(); break;
    case 'N': playMorseDah(); playMorseDit(); break;
    case 'O': playMorseDah(); playMorseDah(); playMorseDah(); break;
    case 'P': playMorseDit(); playMorseDah(); playMorseDah(); playMorseDit(); break;
    case 'R': playMorseDit(); playMorseDah(); playMorseDit(); break;
    case 'S': playMorseDit(); playMorseDit(); playMorseDit(); break;
    case 'T': playMorseDah(); break;
    case 'Y': playMorseDah(); playMorseDit(); playMorseDah(); playMorseDah(); break;
  }
  // Inter-character space = 3 dits (we already have 1 from last element)
  delay(adapter.getDitDuration() * 2);
}

void playMorseWord(const char* word) {
  while (*word) {
    playMorseChar(*word);
    word++;
  }
  // Inter-word space = 7 dits (we already have 3 from last char)
  delay(adapter.getDitDuration() * 4);
}

// Audio feedback tones
void playAdjustmentBeep(bool isIncrease) {
  uint8_t note = adapter.getTxNote();
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

  tone(PIEZO_PIN, frequency);
  delay(50);  // 50ms beep
  noTone(PIEZO_PIN);
}

void playErrorTone() {
  tone(PIEZO_PIN, 200);  // Low 200 Hz buzz
  delay(200);  // 200ms duration
  noTone(PIEZO_PIN);
}

void playDescendingTones() {
  // Descending tone pattern for timeout/exit without save
  int frequencies[] = {1000, 900, 800, 700, 600, 500, 400};
  for (int i = 0; i < 7; i++) {
    tone(PIEZO_PIN, frequencies[i]);
    delay(100);
    noTone(PIEZO_PIN);
    if (i < 6) delay(20);  // Small gap between tones
  }
}

const char* buttonStateToString(ButtonState state) {
  switch (state) {
    case BTN_NONE: return "NONE";
    case BTN_1: return "B1";
    case BTN_2: return "B2";
    case BTN_3: return "B3";
    case BTN_1_2: return "B1+B2";
    case BTN_1_3: return "B1+B3";
    case BTN_2_3: return "B2+B3";
    default: return "UNKNOWN";
  }
}
#endif

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

void saveRadioKeyerModeToEEPROM(bool radioKeyerMode) {
  EEPROM.write(EEPROM_RADIO_KEYER_MODE_ADDR, radioKeyerMode ? 1 : 0);
  EEPROM.commit();
  Serial.print("Saved Radio Keyer Mode to EEPROM: "); Serial.println(radioKeyerMode ? "ON" : "OFF");
}

void loadRadioKeyerModeFromEEPROM() {
#ifdef HAS_RADIO_OUTPUT
  if (EEPROM.read(EEPROM_VALID_FLAG_ADDR) == EEPROM_VALID_VALUE) {
    uint8_t radioKeyerModeVal = EEPROM.read(EEPROM_RADIO_KEYER_MODE_ADDR);
    bool radioKeyerMode = (radioKeyerModeVal == 1);

    adapter.SetRadioKeyerMode(radioKeyerMode);
    Serial.print("Loaded Radio Keyer Mode from EEPROM: ");
    Serial.println(radioKeyerMode ? "ON" : "OFF");
  }
#endif
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
    EEPROM.write(EEPROM_KEYER_TYPE_ADDR, 8);  // Default to Iambic B
    EEPROM.put(EEPROM_DIT_DURATION_ADDR, (uint16_t)DEFAULT_ADAPTER_DIT_DURATION_MS);
    EEPROM.write(EEPROM_TX_NOTE_ADDR, DEFAULT_TONE_NOTE);
    EEPROM.write(EEPROM_RADIO_KEYER_MODE_ADDR, 0); // Default: Radio Keyer Mode OFF
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

#ifdef BUTTON_PIN
  pinMode(BUTTON_PIN, INPUT);
  Serial.println("Button input initialized on pin 3");
#endif

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
  loadRadioKeyerModeFromEEPROM();

  Serial.print("Adapter settings loaded - Keyer: "); Serial.print(adapter.getCurrentKeyerType());
  Serial.print(", Dit Duration (ms): "); Serial.print(adapter.getDitDuration());
  Serial.print(", TX Note: "); Serial.println(adapter.getTxNote());
  Serial.print("Buzzer initially: "); Serial.println(adapter.isBuzzerEnabled() ? "ON" : "OFF");
  Serial.print("Radio Mode initially: "); Serial.println(adapter.isRadioModeActive() ? "ON" : "OFF");
  Serial.print("Radio Keyer Mode initially: "); Serial.println(adapter.isRadioKeyerMode() ? "ON" : "OFF");

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

#ifdef BUTTON_PIN
  // Read button state with debouncing
  int analogVal = readButtonAnalog();
  ButtonState currentButtonState = getButtonState(analogVal);

  // Reset activity timer on any button press in setting modes
  if (currentButtonState != BTN_NONE && currentMode != MODE_NORMAL) {
    lastActivityTime = currentTime;
  }

  // Update debouncer - returns true when complete gesture detected (press & release)
  if (buttonDebouncer.update(currentButtonState, currentTime)) {
    ButtonState gestureDetected = buttonDebouncer.getMaxState();
    unsigned long duration = buttonDebouncer.getLastPressDuration();

    Serial.print("Button Gesture: ");
    Serial.print(buttonStateToString(gestureDetected));

    if (duration >= 2000) {
      Serial.print(" [LONG PRESS - ");
      Serial.print(duration);
      Serial.println("ms]");
    } else {
      Serial.print(" [quick press - ");
      Serial.print(duration);
      Serial.println("ms]");

      // Handle quick presses based on current mode
      if (currentMode == MODE_SPEED_SETTING) {
        if (gestureDetected == BTN_1) {
          // Increase speed
          tempSpeedWPM++;
          if (tempSpeedWPM > 40) {
            tempSpeedWPM = 40;  // Clamp at max
            playErrorTone();
            Serial.println("  -> At maximum speed (40 WPM)");
          } else {
            applyTemporarySpeed(tempSpeedWPM);  // Apply so user can test
            playAdjustmentBeep(true);  // Higher tone for increase
            Serial.print("  -> Speed increased to ");
            Serial.print(tempSpeedWPM);
            Serial.println(" WPM");
          }
        } else if (gestureDetected == BTN_3) {
          // Decrease speed
          tempSpeedWPM--;
          if (tempSpeedWPM < 5) {
            tempSpeedWPM = 5;  // Clamp at min
            playErrorTone();
            Serial.println("  -> At minimum speed (5 WPM)");
          } else {
            applyTemporarySpeed(tempSpeedWPM);  // Apply so user can test
            playAdjustmentBeep(false);  // Lower tone for decrease
            Serial.print("  -> Speed decreased to ");
            Serial.print(tempSpeedWPM);
            Serial.println(" WPM");
          }
        }
      } else if (currentMode == MODE_TONE_SETTING) {
        if (gestureDetected == BTN_1) {
          // Increase tone (higher pitch)
          tempToneNote++;
          if (tempToneNote > 85) {
            tempToneNote = 85;  // Clamp at middle third max (MIDI 85)
            playErrorTone();
            Serial.println("  -> At maximum tone (MIDI 85)");
          } else {
            applyTemporaryTone(tempToneNote);  // Apply so user can test
            // Play a quick beep at the new tone
            tone(PIEZO_PIN, equalTemperamentNote[tempToneNote]);
            delay(100);
            noTone(PIEZO_PIN);
            Serial.print("  -> Tone increased to MIDI note ");
            Serial.print(tempToneNote);
            Serial.print(" (");
            Serial.print(equalTemperamentNote[tempToneNote]);
            Serial.println(" Hz)");
          }
        } else if (gestureDetected == BTN_3) {
          // Decrease tone (lower pitch)
          tempToneNote--;
          if (tempToneNote < 43) {
            tempToneNote = 43;  // Clamp at middle third min (MIDI 43)
            playErrorTone();
            Serial.println("  -> At minimum tone (MIDI 43)");
          } else {
            applyTemporaryTone(tempToneNote);  // Apply so user can test
            // Play a quick beep at the new tone
            tone(PIEZO_PIN, equalTemperamentNote[tempToneNote]);
            delay(100);
            noTone(PIEZO_PIN);
            Serial.print("  -> Tone decreased to MIDI note ");
            Serial.print(tempToneNote);
            Serial.print(" (");
            Serial.print(equalTemperamentNote[tempToneNote]);
            Serial.println(" Hz)");
          }
        }
      } else if (currentMode == MODE_KEY_SETTING) {
        if (gestureDetected == BTN_1) {
          // Cycle to next keyer type (forward)
          if (tempKeyerType == KEYER_STRAIGHT) {
            tempKeyerType = KEYER_IAMBIC_A;
          } else if (tempKeyerType == KEYER_IAMBIC_A) {
            tempKeyerType = KEYER_IAMBIC_B;
          } else {
            tempKeyerType = KEYER_STRAIGHT;
          }
          applyTemporaryKeyerType(tempKeyerType);  // Apply so user can test
          playKeyerTypeCode(tempKeyerType);  // Play Morse code identifier (S, IA, or IB)
          Serial.print("  -> Keyer type changed to ");
          Serial.println(getKeyerTypeName(tempKeyerType));
        } else if (gestureDetected == BTN_3) {
          // Cycle to previous keyer type (backward)
          if (tempKeyerType == KEYER_STRAIGHT) {
            tempKeyerType = KEYER_IAMBIC_B;
          } else if (tempKeyerType == KEYER_IAMBIC_A) {
            tempKeyerType = KEYER_STRAIGHT;
          } else {
            tempKeyerType = KEYER_IAMBIC_A;
          }
          applyTemporaryKeyerType(tempKeyerType);  // Apply so user can test
          playKeyerTypeCode(tempKeyerType);  // Play Morse code identifier (S, IA, or IB)
          Serial.print("  -> Keyer type changed to ");
          Serial.println(getKeyerTypeName(tempKeyerType));
        }
      }
    }
  }

  // Check for long press threshold (2 seconds) - fires once while button still held
  if (buttonDebouncer.isLongPress(currentTime)) {
    ButtonState currentState = buttonDebouncer.getMaxState();
    Serial.print(">>> LONG PRESS DETECTED: ");
    Serial.print(buttonStateToString(currentState));

    // Handle based on current mode
    if (currentMode == MODE_NORMAL) {
      // In normal mode: long press enters setting modes
      switch(currentState) {
        case BTN_1:
          Serial.println(" - Entering SPEED mode");
          playMorseWord("SPEED");
          currentMode = MODE_SPEED_SETTING;
          tempSpeedWPM = ditDurationToWPM(adapter.getDitDuration());
          applyTemporarySpeed(tempSpeedWPM);  // Apply current speed so user can test
          lastActivityTime = currentTime;  // Reset timeout timer
          Serial.print("Current speed: ");
          Serial.print(tempSpeedWPM);
          Serial.println(" WPM");
          break;
        case BTN_2:
          Serial.println(" - Entering TONE mode");
          playMorseWord("TONE");
          currentMode = MODE_TONE_SETTING;
          tempToneNote = adapter.getTxNote();
          // Clamp to middle third range (MIDI 43-85)
          if (tempToneNote < 43) tempToneNote = 43;
          if (tempToneNote > 85) tempToneNote = 85;
          applyTemporaryTone(tempToneNote);  // Apply current tone so user can test
          lastActivityTime = currentTime;  // Reset timeout timer
          Serial.print("Current tone: MIDI note ");
          Serial.print(tempToneNote);
          Serial.print(" (");
          Serial.print(equalTemperamentNote[tempToneNote]);
          Serial.println(" Hz)");
          break;
        case BTN_3:
          Serial.println(" - Entering KEY TYPE mode");
          playMorseWord("KEY");
          currentMode = MODE_KEY_SETTING;
          tempKeyerType = adapter.getCurrentKeyerType();
          // Ensure we're using a valid keyer type
          if (tempKeyerType != KEYER_STRAIGHT && tempKeyerType != KEYER_IAMBIC_A && tempKeyerType != KEYER_IAMBIC_B) {
            tempKeyerType = KEYER_IAMBIC_B;  // Default to Iambic B
          }
          applyTemporaryKeyerType(tempKeyerType);  // Apply current keyer so user can test
          lastActivityTime = currentTime;  // Reset timeout timer
          Serial.print("Current keyer type: ");
          Serial.println(getKeyerTypeName(tempKeyerType));
          break;
        default:
          Serial.println();
          break;
      }
    } else if (currentMode == MODE_SPEED_SETTING) {
      // In speed mode: B2 long press saves and exits
      if (currentState == BTN_2) {
        Serial.println(" - Saving and exiting SPEED mode");

        // Convert WPM to dit duration and apply to adapter
        uint16_t newDitDuration = wpmToDitDuration(tempSpeedWPM);

        // Update adapter settings via MIDI commands (same as loadSettingsFromEEPROM does)
        midiEventPacket_t event;
        event.header = 0x0B;
        event.byte1 = 0xB0;
        event.byte2 = 1;
        event.byte3 = newDitDuration / (2 * MILLISECOND);
        adapter.HandleMIDI(event);

        // Save to EEPROM
        saveSettingsToEEPROM(adapter.getCurrentKeyerType(), newDitDuration, adapter.getTxNote());

        Serial.print("Saved speed: ");
        Serial.print(tempSpeedWPM);
        Serial.print(" WPM (");
        Serial.print(newDitDuration);
        Serial.println("ms dit duration)");

        // Play confirmation and return to normal mode
        playMorseWord("RR");
        currentMode = MODE_NORMAL;
      }
    } else if (currentMode == MODE_TONE_SETTING) {
      // In tone mode: B2 long press saves and exits
      if (currentState == BTN_2) {
        Serial.println(" - Saving and exiting TONE mode");

        // Update adapter settings via MIDI commands
        midiEventPacket_t event;
        event.header = 0x0B;
        event.byte1 = 0xB0;
        event.byte2 = 2;
        event.byte3 = tempToneNote;
        adapter.HandleMIDI(event);

        // Save to EEPROM
        saveSettingsToEEPROM(adapter.getCurrentKeyerType(), adapter.getDitDuration(), tempToneNote);

        Serial.print("Saved tone: MIDI note ");
        Serial.print(tempToneNote);
        Serial.print(" (");
        Serial.print(equalTemperamentNote[tempToneNote]);
        Serial.println(" Hz)");

        // Play confirmation and return to normal mode
        playMorseWord("RR");
        currentMode = MODE_NORMAL;
      }
    } else if (currentMode == MODE_KEY_SETTING) {
      // In key type mode: B2 long press saves and exits
      if (currentState == BTN_2) {
        Serial.println(" - Saving and exiting KEY TYPE mode");

        // Update adapter settings via MIDI commands
        midiEventPacket_t event;
        event.header = 0x0C;
        event.byte1 = 0xC0;
        event.byte2 = tempKeyerType;
        event.byte3 = 0;
        adapter.HandleMIDI(event);

        // Save to EEPROM
        saveSettingsToEEPROM(tempKeyerType, adapter.getDitDuration(), adapter.getTxNote());

        Serial.print("Saved keyer type: ");
        Serial.println(getKeyerTypeName(tempKeyerType));

        // Play confirmation and return to normal mode
        playMorseWord("RR");
        currentMode = MODE_NORMAL;
      }
    }
  }

  // Check for combo press threshold (0.5 seconds) - fires once while buttons still held
  if (buttonDebouncer.isComboPress(currentTime)) {
    ButtonState currentState = buttonDebouncer.getMaxState();
    Serial.print(">>> COMBO PRESS DETECTED: ");
    Serial.print(buttonStateToString(currentState));

    // Play MEM for B1+B3 combo (memory management mode)
    if (currentState == BTN_1_3) {
      Serial.println(" - Playing 'MEM'");
      playMorseWord("MEM");
    } else {
      Serial.println();
    }
  }

  // Check for timeout in setting modes
  if (currentMode == MODE_SPEED_SETTING) {
    if ((currentTime - lastActivityTime) >= SETTING_MODE_TIMEOUT) {
      Serial.println(">>> TIMEOUT - Auto-saving and exiting SPEED mode");

      // Save current settings
      uint16_t newDitDuration = wpmToDitDuration(tempSpeedWPM);
      midiEventPacket_t event;
      event.header = 0x0B;
      event.byte1 = 0xB0;
      event.byte2 = 1;
      event.byte3 = newDitDuration / (2 * MILLISECOND);
      adapter.HandleMIDI(event);
      saveSettingsToEEPROM(adapter.getCurrentKeyerType(), newDitDuration, adapter.getTxNote());

      Serial.print("Auto-saved speed: ");
      Serial.print(tempSpeedWPM);
      Serial.println(" WPM");

      // Play descending tones and return to normal mode
      playDescendingTones();
      currentMode = MODE_NORMAL;
    }
  } else if (currentMode == MODE_TONE_SETTING) {
    if ((currentTime - lastActivityTime) >= SETTING_MODE_TIMEOUT) {
      Serial.println(">>> TIMEOUT - Auto-saving and exiting TONE mode");

      // Save current settings
      midiEventPacket_t event;
      event.header = 0x0B;
      event.byte1 = 0xB0;
      event.byte2 = 2;
      event.byte3 = tempToneNote;
      adapter.HandleMIDI(event);
      saveSettingsToEEPROM(adapter.getCurrentKeyerType(), adapter.getDitDuration(), tempToneNote);

      Serial.print("Auto-saved tone: MIDI note ");
      Serial.print(tempToneNote);
      Serial.print(" (");
      Serial.print(equalTemperamentNote[tempToneNote]);
      Serial.println(" Hz)");

      // Play descending tones and return to normal mode
      playDescendingTones();
      currentMode = MODE_NORMAL;
    }
  } else if (currentMode == MODE_KEY_SETTING) {
    if ((currentTime - lastActivityTime) >= SETTING_MODE_TIMEOUT) {
      Serial.println(">>> TIMEOUT - Auto-saving and exiting KEY TYPE mode");

      // Save current settings
      midiEventPacket_t event;
      event.header = 0x0C;
      event.byte1 = 0xC0;
      event.byte2 = tempKeyerType;
      event.byte3 = 0;
      adapter.HandleMIDI(event);
      saveSettingsToEEPROM(tempKeyerType, adapter.getDitDuration(), adapter.getTxNote());

      Serial.print("Auto-saved keyer type: ");
      Serial.println(getKeyerTypeName(tempKeyerType));

      // Play descending tones and return to normal mode
      playDescendingTones();
      currentMode = MODE_NORMAL;
    }
  }
#endif

  if (event.header) {
    adapter.HandleMIDI(event);
  }

  if (key.update()) {
    adapter.ProcessPaddleInput(PADDLE_STRAIGHT, !key.read(), false);
    // Reset activity timer on CW key activity in setting modes
    if (currentMode != MODE_NORMAL) {
      lastActivityTime = currentTime;
    }
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
    // Reset activity timer on CW key activity in setting modes
    if (currentMode != MODE_NORMAL) {
      lastActivityTime = currentTime;
    }
  }
  if (dah.update()) {
    adapter.ProcessPaddleInput(PADDLE_DAH, !dah.read(), false);
    // Reset activity timer on CW key activity in setting modes
    if (currentMode != MODE_NORMAL) {
      lastActivityTime = currentTime;
    }
  }

  if (qt_key.update()) {
    adapter.ProcessPaddleInput(PADDLE_STRAIGHT, qt_key.read(), true);
    if (currentMode != MODE_NORMAL) {
      lastActivityTime = currentTime;
    }
  }
  if (qt_dit.update()) {
    adapter.ProcessPaddleInput(PADDLE_DIT, qt_dit.read(), true);
    if (currentMode != MODE_NORMAL) {
      lastActivityTime = currentTime;
    }
  }
  if (qt_dah.update()) {
    adapter.ProcessPaddleInput(PADDLE_DAH, qt_dah.read(), true);
    if (currentMode != MODE_NORMAL) {
      lastActivityTime = currentTime;
    }
  }
}