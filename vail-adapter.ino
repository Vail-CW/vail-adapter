#include "config.h"

#include <MIDIUSB.h>
#include <Keyboard.h>
#include <Adafruit_FreeTouch.h>
#include "bounce2.h"
#include "touchbounce.h"
#include "adapter.h"
#include "buttons.h"
#include "memory.h"
#include "morse_audio.h"
#include "settings_eeprom.h"
#include "menu_handler.h"

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

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

#ifdef BUTTON_PIN
  pinMode(BUTTON_PIN, INPUT);
  Serial.println("Button input initialized on pin 3");
#endif

  dit.attach(DIT_PIN, INPUT_PULLUP);
  dah.attach(DAH_PIN, INPUT_PULLUP);
  key.attach(KEY_PIN, INPUT_PULLUP);

  // Attach capacitive touch with calibrated per-pad thresholds
  qt_dit.attach(QT_DIT_PIN, QT_DIT_THRESHOLD_PRESS, QT_DIT_THRESHOLD_RELEASE);
  qt_dah.attach(QT_DAH_PIN, QT_DAH_THRESHOLD_PRESS, QT_DAH_THRESHOLD_RELEASE);
  qt_key.attach(QT_KEY_PIN, QT_DIT_THRESHOLD_PRESS, QT_DIT_THRESHOLD_RELEASE); // Use DIT thresholds for KEY

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

#ifdef BUTTON_PIN
  loadMemoriesFromEEPROM(memorySlots);
  // Connect recording state to adapter for key capture
  adapter.setRecordingState(&recordingState);
  // Initialize menu handler
  initMenuHandler(&adapter, memorySlots, &recordingState, &playbackState);
#endif

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
          // Normal mode: use adapter (outputs to radio/MIDI/keyboard based on current mode)
          adapter.BeginTx();
        } else {
          // Memory management mode: piezo only
          tone(PIEZO_PIN, adapter.getTxNote());
        }
      } else {
        // Key up
        if (menuState.currentMode == MODE_PLAYING_MEMORY) {
          // Normal mode: use adapter
          adapter.EndTx();
        } else {
          // Memory management mode: piezo only
          noTone(PIEZO_PIN);
        }
      }
      lastPlaybackKeyState = playbackState.keyCurrentlyDown;
    }
  } else if (wasPlaying) {
    // Playback just finished
    if (menuState.currentMode == MODE_PLAYING_MEMORY) {
      adapter.EndTx();  // Make sure key is released
    } else {
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

  if (key.update()) {
    adapter.ProcessPaddleInput(PADDLE_STRAIGHT, !key.read(), false);
#ifdef BUTTON_PIN
    // Reset activity timer on CW key activity in setting modes
    if (menuState.currentMode != MODE_NORMAL) {
      menuState.lastActivityTime = currentTime;
    }
#endif
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

  if (qt_key.update()) {
    adapter.ProcessPaddleInput(PADDLE_STRAIGHT, qt_key.read(), true);
#ifdef BUTTON_PIN
    if (menuState.currentMode != MODE_NORMAL) {
      menuState.lastActivityTime = currentTime;
    }
#endif
  }
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
}