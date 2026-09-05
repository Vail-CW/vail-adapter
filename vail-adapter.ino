#include "config.h"

#include <MIDIUSB.h>
#include <Keyboard.h>
#ifndef NO_CAPACITIVE_TOUCH
#include <Adafruit_FreeTouch.h>
#endif
#include "bounce2.h"
#include "touchbounce.h"
#include "adapter.h"
#include "buttons.h"
#include "memory.h"
#include "morse_audio.h"
#include "settings_eeprom.h"
#include "menu_handler.h"
#include "equal_temperament.h"
#include "morse_decoder.h"

// ---- Mono (TS) plug detection ----
// A mono straight key plug shorts the ring to the sleeve, so the DAH pin reads
// pressed for as long as the plug is in. In Straight Key mode that would be a
// constant key-down, so the firmware switches to TRS mode and reads the key
// from the DIT pin only. See updateTrsDetection() for the rules.
bool trs = false;
uint8_t ditReleasesWhileDahLow = 0;   // Complete DIT presses seen with DAH grounded throughout
unsigned long lastDitEdgeTime = 0;    // Last DIT press or release
const uint8_t TRS_KEYED_CYCLES = 2;              // DIT presses with DAH grounded that prove a mono plug
const unsigned long TRS_IDLE_THRESHOLD = 3000;   // DAH grounded this long with DIT idle and open
const unsigned long TRS_UNPLUG_THRESHOLD = 1000; // DAH open this long ends TRS mode

Bounce dit = Bounce();
Bounce dah = Bounce();
Bounce key = Bounce();
#ifndef NO_CAPACITIVE_TOUCH
TouchBounce qt_dit = TouchBounce();
TouchBounce qt_dah = TouchBounce();
TouchBounce qt_key = TouchBounce();
#endif

VailAdapter adapter = VailAdapter(PIEZO_PIN);

// ---- Keyboard Sim mode: Morse decoder + USB keyboard output ----
MorseDecoder morseDecoder;

// Decoder timing-capture state
bool lastDecoderKeyState = false;
unsigned long lastDecoderEventTime = 0;

void onDecodedChar(char c) {
  // Detection runs whether or not we're already in sim mode.
  adapter.checkForKSKS(c);
  if (adapter.isKeyboardSimMode()) {
    adapter.outputKeyboardChar(c);
  }
}

void onDecodedBackspace() {
  if (adapter.isKeyboardSimMode()) adapter.outputKeyboardBackspace();
}

void onDecodedEnter() {
  if (adapter.isKeyboardSimMode()) adapter.outputKeyboardEnter();
}

void onDecodedSpace() {
  if (adapter.isKeyboardSimMode()) adapter.outputKeyboardSpace();
}

void onDecodedWordGap() {
  // A word-length silence was measured. This fires whether or not a space
  // character is typed for it, so KSKS can start after any real pause, not
  // only after pauses the decoder chose to turn into a space.
  adapter.notifyWordBoundary();
}

void onDecodedError() {
  if (adapter.isKeyboardSimMode()) playInvalidCodeTone();
}

void onEnterKeyboardSimMode() {
  // Reset the decoder so in-flight elements aren't typed as the first character.
  morseDecoder.reset();
  bool usingKeyer = (adapter.getCurrentKeyerType() > 1);
  bool currentKeyState = false;
  if (usingKeyer) {
    currentKeyState = adapter.isTransmitting();
  } else {
    dit.update();
    dah.update();
    if (!dit.read() || !dah.read()) currentKeyState = true;
  }
  lastDecoderKeyState = currentKeyState;
  lastDecoderEventTime = 0;  // Prevents a bogus timing on the next transition
}

#ifdef BUTTON_PIN
ButtonDebouncer buttonDebouncer;

// CW Memory system
CWMemory memorySlots[MAX_MEMORY_SLOTS];  // 3 memory slots
RecordingState recordingState;           // Current recording state
PlaybackState playbackState;             // Current playback state
#endif

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.print("\n\nVail Adapter starting on: ");
  Serial.println(BOARD_NAME);

#ifndef NO_LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
#endif

#ifdef BUTTON_PIN
  pinMode(BUTTON_PIN, INPUT);
  Serial.println("Button input initialized on pin 3");
#endif

  dit.attach(DIT_PIN, INPUT_PULLUP);
  dah.attach(DAH_PIN, INPUT_PULLUP);
#ifndef TRRS_TRINKEY
  // Trinkey doesn't need separate straight key pin - KEY_PIN same as DIT_PIN
  key.attach(KEY_PIN, INPUT_PULLUP);
#endif

#ifdef TRRS_TRINKEY
  // Configure TRRS jack ground pins - required for switched TRRS jack
  pinMode(SLEEVE_PIN, OUTPUT);
  digitalWrite(SLEEVE_PIN, LOW);
  pinMode(RING2_PIN, OUTPUT);
  digitalWrite(RING2_PIN, LOW);
  Serial.println("TRRS ground pins (SLEEVE, RING2) configured as OUTPUT LOW");

  // Increase debounce interval for TRRS jack pins - they have different electrical characteristics
  dit.interval(25);  // Increased from default 10ms to 25ms
  dah.interval(25);
  key.interval(25);
  Serial.println("Debounce intervals increased to 25ms for TRRS jack stability");
#endif

#ifndef NO_CAPACITIVE_TOUCH
  // Attach capacitive touch with calibrated per-pad thresholds
  qt_dit.attach(QT_DIT_PIN, QT_DIT_THRESHOLD_PRESS, QT_DIT_THRESHOLD_RELEASE);
  qt_dah.attach(QT_DAH_PIN, QT_DAH_THRESHOLD_PRESS, QT_DAH_THRESHOLD_RELEASE);
#ifdef QT_KEY_PIN
  qt_key.attach(QT_KEY_PIN, QT_DIT_THRESHOLD_PRESS, QT_DIT_THRESHOLD_RELEASE); // Use DIT thresholds for KEY
#endif
#endif

#ifdef HAS_RADIO_OUTPUT
  pinMode(RADIO_DIT_PIN, OUTPUT);
  pinMode(RADIO_DAH_PIN, OUTPUT);
  digitalWrite(RADIO_DIT_PIN, RADIO_INACTIVE_LEVEL); // Use configured inactive level
  digitalWrite(RADIO_DAH_PIN, RADIO_INACTIVE_LEVEL); // Use configured inactive level
  Serial.print("Radio Output Pins Initialized. Inactive Level: ");
  Serial.println(RADIO_INACTIVE_LEVEL == LOW ? "LOW" : "HIGH");
#endif

  // Initialize audio module
  initMorseAudio(&adapter, PIEZO_PIN);

  uint8_t startupTone = loadToneFromEEPROM();
  Serial.println("Playing VAIL in Morse code at 20 WPM");
  playVAIL(startupTone);

  loadSettingsFromEEPROM(adapter);
  loadRadioKeyerModeFromEEPROM(adapter);
  loadPaddleSwapModeFromEEPROM(adapter);

#ifdef BUTTON_PIN
  loadMemoriesFromEEPROM(memorySlots);
  // Connect recording state to adapter for key capture
  adapter.setRecordingState(&recordingState);
  // Initialize menu handler with flush callback
  initMenuHandler(&adapter, memorySlots, &recordingState, &playbackState, flushBounceState);
#endif

  Serial.print("Adapter settings loaded - Keyer: "); Serial.print(adapter.getCurrentKeyerType());
  Serial.print(", Dit Duration (ms): "); Serial.print(adapter.getDitDuration());
  Serial.print(", TX Note: "); Serial.println(adapter.getTxNote());
  Serial.print("Buzzer initially: "); Serial.println(adapter.isBuzzerEnabled() ? "ON" : "OFF");
  Serial.print("Radio Mode initially: "); Serial.println(adapter.isRadioModeActive() ? "ON" : "OFF");
  Serial.print("Radio Keyer Mode initially: "); Serial.println(adapter.isRadioKeyerMode() ? "ON" : "OFF");
  Serial.print(F("Paddle Swap mode initially: "));
  Serial.println(adapter.getPaddleSwapMode());

  Keyboard.begin();
  MidiUSB.flush();

  // Keyboard Sim mode: initialise the Morse decoder and wire its callbacks
  morseDecoder.begin(adapter.getDitDuration());
  morseDecoder.setCharacterCallback(onDecodedChar);
  morseDecoder.setBackspaceCallback(onDecodedBackspace);
  morseDecoder.setEnterCallback(onDecodedEnter);
  morseDecoder.setSpaceCallback(onDecodedSpace);
  morseDecoder.setWordGapCallback(onDecodedWordGap);
  morseDecoder.setErrorCallback(onDecodedError);
  adapter.setEnterKeyboardSimModeCallback(onEnterKeyboardSimMode);

  // Ensure clean keyboard state on startup
  adapter.ReleaseAllKeys();

  for (int i = 0; i < 16; i++) {
    delay(20);
    dah.update();
  }
  if (dah.read() == LOW) {
    trs = true;
    Serial.println("TRS plug potentially detected (DAH pin grounded).");
  }
}

void flushBounceState() {
  // Flush the Bounce state by updating multiple times without processing
  // This clears any stale "pressed" states after mode changes
  for (int i = 0; i < 3; i++) {
    dit.update();
    dah.update();
    key.update();
    delay(5);
  }
  Serial.println("Flushed Bounce state for dit/dah/key inputs");
}

// Record a debounced DIT edge for TRS detection. A release that happens while
// DAH is still grounded counts toward the keyed rule.
void noteDitEdge(unsigned long now) {
  lastDitEdgeTime = now;
  if (dit.read() == HIGH && dah.read() == LOW) {
    if (ditReleasesWhileDahLow < 255) ditReleasesWhileDahLow++;
  }
}

void setTrsMode(bool enabled, const __FlashStringHelper* why) {
  trs = enabled;
  ditReleasesWhileDahLow = 0;
  // Whatever the keyer thought was closed on the old input path is now
  // meaningless. Drop it so the next press starts clean.
  adapter.ResetInputState();
  Serial.print(enabled ? F("TRS mode on: ") : F("TRS mode off: "));
  Serial.println(why);
  if (enabled) Serial.println(F("Straight key input via DIT pin, DAH pin ignored."));
}

// Decide whether a mono plug is present. Only runs in Straight Key mode, and
// only on the physical pins. Two rules can turn TRS mode on, and neither can
// be satisfied by a person working a paddle or a key cable with tip and ring
// tied together:
//   1. DAH stays grounded across TRS_KEYED_CYCLES complete DIT presses. A real
//      DAH lever, or a tied cable, opens the pin between presses.
//   2. DAH stays grounded for TRS_IDLE_THRESHOLD with no DIT activity at all
//      while the DIT pin is open. Someone holding a DAH lever that long in
//      Straight Key mode is already sending a constant tone, so cutting it is
//      harmless, and TRS mode drops again once the lever is released.
// Both rules also require the DIT pin to be open at the moment of the switch,
// so the input path never changes in the middle of an element. TRS mode ends
// when DAH has read open for TRS_UNPLUG_THRESHOLD, again with DIT open.
void updateTrsDetection(unsigned long now) {
  bool dahLow = (dah.read() == LOW);
  bool ditOpen = (dit.read() == HIGH);

  if (!dahLow) ditReleasesWhileDahLow = 0;

  if (!trs) {
    if (adapter.getCurrentKeyerType() != 1) {
      ditReleasesWhileDahLow = 0;
      return;
    }
    if (!dahLow || !ditOpen) return;
    if (ditReleasesWhileDahLow >= TRS_KEYED_CYCLES) {
      setTrsMode(true, F("DAH stayed grounded across DIT presses"));
    } else if (dah.duration() >= TRS_IDLE_THRESHOLD &&
               (now - lastDitEdgeTime) >= TRS_IDLE_THRESHOLD) {
      setTrsMode(true, F("DAH grounded with no DIT activity"));
    }
  } else if (!dahLow && ditOpen && dah.duration() >= TRS_UNPLUG_THRESHOLD) {
    setTrsMode(false, F("DAH no longer grounded"));
  }
}

void setLED() {
#ifndef NO_LED
  bool finalLedState = false;

  if (adapter.isRadioModeActive()) {
    finalLedState = (millis() % 400 < 200);
  } else if (!adapter.isBuzzerEnabled()) {
    finalLedState = (millis() % 2000 < 1000);
  } else {
    finalLedState = adapter.KeyboardMode();
  }
  digitalWrite(LED_BUILTIN, finalLedState ? LED_ON : LED_OFF);
#endif
}

void loop() {
  unsigned long currentTime = millis();
  midiEventPacket_t event = MidiUSB.read();

  setLED();
  adapter.Tick(currentTime);

#ifdef BUTTON_PIN
  MenuHandlerState& menuState = getMenuState();

  // Update memory playback state machine
  updatePlayback(playbackState);

  // Control output during playback
  static bool lastPlaybackKeyState = false;
  static bool wasPlaying = false;

  if (playbackState.isPlaying) {
    wasPlaying = true;
    if (playbackState.keyCurrentlyDown != lastPlaybackKeyState) {
      if (playbackState.keyCurrentlyDown) {
        // Key down
        if (menuState.currentMode == MODE_PLAYING_MEMORY) {
          // Normal mode playback: use BeginTx(relay) to pass paddle info for radio mode
          // Convert paddle flag (0=DIT, 1=DAH) to PADDLE enum (PADDLE_DIT=0, PADDLE_DAH=1)
          int relay = (playbackState.currentPaddle == 0) ? PADDLE_DIT : PADDLE_DAH;
          adapter.BeginTx(relay);
        } else {
          // Memory management mode: piezo only (bypass buzzer enable check)
          // Convert MIDI note to frequency
          uint8_t midiNote = adapter.getTxNote();
          int frequency = GET_EQUAL_TEMPERAMENT_NOTE(midiNote);
          tone(PIEZO_PIN, frequency);
        }
      } else {
        // Key up
        if (menuState.currentMode == MODE_PLAYING_MEMORY) {
          // Normal mode playback: use EndTx(relay) to pass paddle info for radio mode
          // Convert paddle flag (0=DIT, 1=DAH) to PADDLE enum (PADDLE_DIT=0, PADDLE_DAH=1)
          int relay = (playbackState.currentPaddle == 0) ? PADDLE_DIT : PADDLE_DAH;
          adapter.EndTx(relay);
        } else {
          // Memory management mode: piezo only
          noTone(PIEZO_PIN);
        }
      }
      lastPlaybackKeyState = playbackState.keyCurrentlyDown;
    }
  } else if (wasPlaying) {
    // Playback just finished
    // The playback state machine ensures key is already released before stopping,
    // so we don't need to call EndTx here (it would be a duplicate)
    if (menuState.currentMode != MODE_PLAYING_MEMORY) {
      // Memory management mode: ensure piezo is off
      noTone(PIEZO_PIN);
    }
    lastPlaybackKeyState = false;
    wasPlaying = false;

    // Return to appropriate mode
    if (menuState.currentMode == MODE_PLAYING_MEMORY) {
      menuState.currentMode = MODE_NORMAL;
      Serial.println("Playback finished - returned to normal mode");
    }
  }

  // Check for recording timeout (25 seconds) or max transitions
  if (recordingState.isRecording) {
    if (recordingState.hasReachedMaxDuration() || recordingState.hasReachedMaxTransitions()) {
      uint8_t activeSlot = recordingState.slotNumber;
      Serial.println("Recording auto-stopped (timeout or max transitions reached)");
      stopRecording(recordingState, memorySlots[activeSlot]);
      saveMemoryToEEPROM(activeSlot, memorySlots[activeSlot]);

      // Play completion tone
      playAdjustmentBeep(false);
      delay(100);
      playAdjustmentBeep(true);
      delay(100);
      playAdjustmentBeep(true);

      menuState.currentMode = MODE_MEMORY_MANAGEMENT;
      Serial.println("Returned to memory management mode");
    }
  }

  // Update menu handler (handles all button logic)
  updateMenuHandler(currentTime, buttonDebouncer);
#endif

  if (event.header) {
    adapter.HandleMIDI(event);
  }

#ifndef TRRS_TRINKEY
  // Trinkey doesn't process separate straight key input
  if (key.update()) {
    adapter.ProcessPaddleInput(PADDLE_STRAIGHT, !key.read(), false);
#ifdef BUTTON_PIN
    // Reset activity timer on CW key activity in setting modes
    if (menuState.currentMode != MODE_NORMAL) {
      menuState.lastActivityTime = currentTime;
    }
#endif
  }
#endif

  if (trs) {
    // TRS mode: DAH pin is grounded (ring shorted to sleeve)
    // DIT pin (tip) is the actual straight key input
    // Only process DIT as straight key, ignore DAH completely
    if (dit.update()) {
      adapter.ProcessPaddleInput(PADDLE_STRAIGHT, !dit.read(), false);
#ifdef BUTTON_PIN
      // Reset activity timer on CW key activity in setting modes
      if (menuState.currentMode != MODE_NORMAL) {
        menuState.lastActivityTime = currentTime;
      }
#endif
    }
    // Update DAH to keep Bounce state current, but don't process it
    dah.update();
  } else {
    // Normal paddle mode: process both DIT and DAH separately
    if (dit.update()) {
      noteDitEdge(currentTime);
      adapter.ProcessPaddleInput(PADDLE_DIT, !dit.read(), false);
#ifdef BUTTON_PIN
      // Reset activity timer on CW key activity in setting modes
      if (menuState.currentMode != MODE_NORMAL) {
        menuState.lastActivityTime = currentTime;
      }
#endif
    }
    if (dah.update()) {
      adapter.ProcessPaddleInput(PADDLE_DAH, !dah.read(), false);
#ifdef BUTTON_PIN
      // Reset activity timer on CW key activity in setting modes
      if (menuState.currentMode != MODE_NORMAL) {
        menuState.lastActivityTime = currentTime;
      }
#endif
    }
  }

  // Mono plug hot-plug detection, using the debounced pin states from above
  updateTrsDetection(currentTime);

#ifndef NO_CAPACITIVE_TOUCH
#ifdef QT_KEY_PIN
  if (qt_key.update()) {
    adapter.ProcessPaddleInput(PADDLE_STRAIGHT, qt_key.read(), true);
#ifdef BUTTON_PIN
    if (menuState.currentMode != MODE_NORMAL) {
      menuState.lastActivityTime = currentTime;
    }
#endif
  }
#endif
  if (qt_dit.update()) {
    adapter.ProcessPaddleInput(PADDLE_DIT, qt_dit.read(), true);
#ifdef BUTTON_PIN
    if (menuState.currentMode != MODE_NORMAL) {
      menuState.lastActivityTime = currentTime;
    }
#endif
  }
  if (qt_dah.update()) {
    adapter.ProcessPaddleInput(PADDLE_DAH, qt_dah.read(), true);
#ifdef BUTTON_PIN
    if (menuState.currentMode != MODE_NORMAL) {
      menuState.lastActivityTime = currentTime;
    }
#endif
  }
#endif

  // ========================================================================
  // Keyboard Sim mode: feed keyed Morse timing into the decoder.
  // For iambic keyers we use the keyer's output timing (the user holds paddles
  // while the keyer times the elements); for straight key we use raw input.
  // Skip radio mode and memory record/playback so they aren't decoded/typed.
  // ========================================================================
  bool decoderActive = !adapter.isRadioModeActive();
#ifdef BUTTON_PIN
  decoderActive = decoderActive && !recordingState.isRecording && !playbackState.isPlaying;
#endif
  if (decoderActive) {
    bool keyIsDown = false;
    bool usingKeyer = (adapter.getCurrentKeyerType() > 1);
    if (usingKeyer) {
      keyIsDown = adapter.isTransmitting();
    } else {
      if (!dit.read() || !dah.read()) keyIsDown = true;
#ifndef TRRS_TRINKEY
      if (!key.read()) keyIsDown = true;
#endif
#ifndef NO_CAPACITIVE_TOUCH
      if (qt_dit.read() || qt_dah.read()) keyIsDown = true;
#ifdef QT_KEY_PIN
      if (qt_key.read()) keyIsDown = true;
#endif
#endif
    }

    morseDecoder.setKeyDown(keyIsDown);

    if (keyIsDown != lastDecoderKeyState) {
      unsigned long now = millis();
      if (lastDecoderEventTime > 0) {
        unsigned long elapsed = now - lastDecoderEventTime;
        // Cap the gap so a long pause cannot wrap the 16-bit timing value and
        // reach the decoder looking like a very long tone. That was putting a
        // phantom dah in front of the first character after a pause.
        if (elapsed > 30000) elapsed = 30000;
        int16_t duration = (int16_t)elapsed;
        if (keyIsDown) {
          morseDecoder.addTiming(-duration);  // Silence that just ended
        } else {
          morseDecoder.addTiming(duration);   // Tone that just ended
        }
      }
      lastDecoderEventTime = now;
      lastDecoderKeyState = keyIsDown;
    }

    morseDecoder.tick(currentTime);
  } else {
    // Keep edge state clean so we don't emit a bogus timing when we resume.
    lastDecoderKeyState = false;
    lastDecoderEventTime = 0;
  }
}