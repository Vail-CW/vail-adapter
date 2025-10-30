#include "menu_handler.h"
#include "morse_audio.h"
#include "settings_eeprom.h"
#include "config.h"
#include "equal_temperament.h"
#include <MIDIUSB.h>

#define SETTING_MODE_TIMEOUT 30000  // 30 seconds

// Valid keyer types for cycling (1-9, skipping 0=Passthrough)
#define KEYER_MIN 1
#define KEYER_MAX 9

// Module-level state
static MenuHandlerState menuState = {
  MODE_NORMAL,  // currentMode
  12,           // tempSpeedWPM (default 12 WPM = 100ms dit)
  69,           // tempToneNote (default A4 = 440Hz)
  8,            // tempKeyerType (default Iambic B)
  0             // lastActivityTime
};

// Module-level references
static VailAdapter* adapter = nullptr;
static CWMemory* memorySlots = nullptr;
static RecordingState* recordingState = nullptr;
static PlaybackState* playbackState = nullptr;
static FlushBounceCallback flushBounceCallback = nullptr;

// ============================================================================
// Initialization
// ============================================================================

void initMenuHandler(VailAdapter* adapterRef,
                     CWMemory* memoryRef,
                     RecordingState* recordingRef,
                     PlaybackState* playbackRef,
                     FlushBounceCallback flushCallback) {
  adapter = adapterRef;
  memorySlots = memoryRef;
  recordingState = recordingRef;
  playbackState = playbackRef;
  flushBounceCallback = flushCallback;
}

MenuHandlerState& getMenuState() {
  return menuState;
}

// ============================================================================
// Conversion Utilities
// ============================================================================

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

// ============================================================================
// Apply Temporary Settings (for testing before committing)
// ============================================================================

void applyTemporarySpeed(int wpm) {
  if (!adapter) return;
  // Apply speed to adapter without saving to EEPROM
  // This allows testing the speed before committing
  uint16_t newDitDuration = wpmToDitDuration(wpm);
  midiEventPacket_t event;
  event.header = 0x0B;
  event.byte1 = 0xB0;
  event.byte2 = 1;
  event.byte3 = newDitDuration / (2 * MILLISECOND);
  adapter->HandleMIDI(event);
}

void applyTemporaryTone(uint8_t noteNumber) {
  if (!adapter) return;
  // Apply tone to adapter without saving to EEPROM
  // This allows testing the tone before committing
  midiEventPacket_t event;
  event.header = 0x0B;
  event.byte1 = 0xB0;
  event.byte2 = 2;
  event.byte3 = noteNumber;
  adapter->HandleMIDI(event);
}

void applyTemporaryKeyerType(uint8_t keyerType) {
  if (!adapter) return;
  // Apply keyer type to adapter without saving to EEPROM
  // This allows testing the keyer type before committing
  midiEventPacket_t event;
  event.header = 0x0C;
  event.byte1 = 0xC0;
  event.byte2 = keyerType;
  event.byte3 = 0;
  adapter->HandleMIDI(event);

  // Flush bounce state after mode change to prevent stuck keys
  if (flushBounceCallback) {
    flushBounceCallback();
  }
}

// ============================================================================
// Button State to String Conversion
// ============================================================================

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

// ============================================================================
// Quick Press Handlers (by mode)
// ============================================================================

static void handleQuickPressSpeedMode(ButtonState gestureDetected) {
  if (gestureDetected == BTN_1) {
    // Increase speed
    menuState.tempSpeedWPM++;
    if (menuState.tempSpeedWPM > 40) {
      menuState.tempSpeedWPM = 40;  // Clamp at max
      playErrorTone();
      Serial.println("  -> At maximum speed (40 WPM)");
    } else {
      applyTemporarySpeed(menuState.tempSpeedWPM);  // Apply so user can test
      playAdjustmentBeep(true);  // Higher tone for increase
      Serial.print("  -> Speed increased to ");
      Serial.print(menuState.tempSpeedWPM);
      Serial.println(" WPM");
    }
  } else if (gestureDetected == BTN_3) {
    // Decrease speed
    menuState.tempSpeedWPM--;
    if (menuState.tempSpeedWPM < 5) {
      menuState.tempSpeedWPM = 5;  // Clamp at min
      playErrorTone();
      Serial.println("  -> At minimum speed (5 WPM)");
    } else {
      applyTemporarySpeed(menuState.tempSpeedWPM);  // Apply so user can test
      playAdjustmentBeep(false);  // Lower tone for decrease
      Serial.print("  -> Speed decreased to ");
      Serial.print(menuState.tempSpeedWPM);
      Serial.println(" WPM");
    }
  }
}

static void handleQuickPressToneMode(ButtonState gestureDetected) {
  if (gestureDetected == BTN_1) {
    // Increase tone (higher pitch)
    menuState.tempToneNote++;
    if (menuState.tempToneNote > 85) {
      menuState.tempToneNote = 85;  // Clamp at middle third max (MIDI 85)
      playErrorTone();
      Serial.println("  -> At maximum tone (MIDI 85)");
    } else {
      applyTemporaryTone(menuState.tempToneNote);  // Apply so user can test
      // Play a quick beep at the new tone
      tone(PIEZO_PIN, equalTemperamentNote[menuState.tempToneNote]);
      delay(100);
      noTone(PIEZO_PIN);
      Serial.print("  -> Tone increased to MIDI note ");
      Serial.print(menuState.tempToneNote);
      Serial.print(" (");
      Serial.print(equalTemperamentNote[menuState.tempToneNote]);
      Serial.println(" Hz)");
    }
  } else if (gestureDetected == BTN_3) {
    // Decrease tone (lower pitch)
    menuState.tempToneNote--;
    if (menuState.tempToneNote < 43) {
      menuState.tempToneNote = 43;  // Clamp at middle third min (MIDI 43)
      playErrorTone();
      Serial.println("  -> At minimum tone (MIDI 43)");
    } else {
      applyTemporaryTone(menuState.tempToneNote);  // Apply so user can test
      // Play a quick beep at the new tone
      tone(PIEZO_PIN, equalTemperamentNote[menuState.tempToneNote]);
      delay(100);
      noTone(PIEZO_PIN);
      Serial.print("  -> Tone decreased to MIDI note ");
      Serial.print(menuState.tempToneNote);
      Serial.print(" (");
      Serial.print(equalTemperamentNote[menuState.tempToneNote]);
      Serial.println(" Hz)");
    }
  }
}

static void handleQuickPressKeyMode(ButtonState gestureDetected) {
  if (gestureDetected == BTN_1) {
    // Cycle to next keyer type (forward)
    menuState.tempKeyerType++;
    if (menuState.tempKeyerType > KEYER_MAX) {
      menuState.tempKeyerType = KEYER_MIN;  // Wrap around to beginning
    }
    applyTemporaryKeyerType(menuState.tempKeyerType);  // Apply so user can test
    playKeyerTypeCode(menuState.tempKeyerType);  // Play Morse code identifier
    Serial.print("  -> Keyer type changed to ");
    Serial.println(getKeyerTypeName(menuState.tempKeyerType));
  } else if (gestureDetected == BTN_3) {
    // Cycle to previous keyer type (backward)
    menuState.tempKeyerType--;
    if (menuState.tempKeyerType < KEYER_MIN) {
      menuState.tempKeyerType = KEYER_MAX;  // Wrap around to end
    }
    applyTemporaryKeyerType(menuState.tempKeyerType);  // Apply so user can test
    playKeyerTypeCode(menuState.tempKeyerType);  // Play Morse code identifier
    Serial.print("  -> Keyer type changed to ");
    Serial.println(getKeyerTypeName(menuState.tempKeyerType));
  }
}

static void handleQuickPressNormalMode(ButtonState gestureDetected) {
  // In normal mode: quick press plays memory via current output mode
  uint8_t slotNumber = 0;
  if (gestureDetected == BTN_1) slotNumber = 0;
  else if (gestureDetected == BTN_2) slotNumber = 1;
  else if (gestureDetected == BTN_3) slotNumber = 2;
  else return;  // Not a single button press

  if (!memorySlots[slotNumber].isEmpty()) {
    Serial.print("  -> Playing memory slot ");
    Serial.print(slotNumber + 1);
    Serial.println(" via current output mode");
    startPlayback(*playbackState, slotNumber, memorySlots[slotNumber]);
    menuState.currentMode = MODE_PLAYING_MEMORY;
  } else {
    Serial.print("  -> Memory slot ");
    Serial.print(slotNumber + 1);
    Serial.println(" is empty");
  }
}

static void handleQuickPressRecordingMode(ButtonState gestureDetected) {
  // In recording mode: single-click stops and saves recording
  uint8_t activeSlot = (menuState.currentMode == MODE_RECORDING_MEMORY_1) ? 0 :
                      (menuState.currentMode == MODE_RECORDING_MEMORY_2) ? 1 : 2;

  // Check if user clicked the same button that started recording
  uint8_t clickedSlot = 0;
  if (gestureDetected == BTN_1) clickedSlot = 0;
  else if (gestureDetected == BTN_2) clickedSlot = 1;
  else if (gestureDetected == BTN_3) clickedSlot = 2;
  else return;  // Not a single button press

  if (clickedSlot == activeSlot) {
    Serial.println("  -> Stopping recording (user-triggered)");
    stopRecording(*recordingState, memorySlots[activeSlot]);
    saveMemoryToEEPROM(activeSlot, memorySlots[activeSlot]);

    // Play confirmation tone
    playAdjustmentBeep(true);
    delay(100);
    playAdjustmentBeep(true);

    menuState.currentMode = MODE_MEMORY_MANAGEMENT;
    Serial.println("  -> Returned to memory management mode");
  }
}

static void handleQuickPressMemoryManagementMode(ButtonState gestureDetected) {
  // In memory management mode: quick press plays memory via piezo only
  uint8_t slotNumber = 0;
  if (gestureDetected == BTN_1) slotNumber = 0;
  else if (gestureDetected == BTN_2) slotNumber = 1;
  else if (gestureDetected == BTN_3) slotNumber = 2;
  else return;  // Not a single button press

  Serial.print("  -> Attempting playback of slot ");
  Serial.print(slotNumber + 1);
  Serial.print(" - transitions: ");
  Serial.print(memorySlots[slotNumber].transitionCount);
  Serial.print(", duration: ");
  Serial.print(memorySlots[slotNumber].getDurationMs());
  Serial.println("ms");

  if (!memorySlots[slotNumber].isEmpty()) {
    Serial.println("  -> Starting playback (piezo only)");
    startPlayback(*playbackState, slotNumber, memorySlots[slotNumber]);
    // Note: Playback happens in the background via updatePlayback() in loop()
  } else {
    Serial.println("  -> ERROR: Memory slot is empty!");
  }
}

// ============================================================================
// Long Press Handlers (by mode)
// ============================================================================

static void handleLongPressNormalMode(ButtonState currentState, unsigned long currentTime) {
  // In normal mode: long press enters setting modes
  switch(currentState) {
    case BTN_1:
      Serial.println(" - Entering SPEED mode");
      playMorseWord("SPEED");
      menuState.currentMode = MODE_SPEED_SETTING;
      if (adapter) {
        menuState.tempSpeedWPM = ditDurationToWPM(adapter->getDitDuration());
      }
      applyTemporarySpeed(menuState.tempSpeedWPM);  // Apply current speed so user can test
      menuState.lastActivityTime = currentTime;  // Reset timeout timer
      Serial.print("Current speed: ");
      Serial.print(menuState.tempSpeedWPM);
      Serial.println(" WPM");
      break;
    case BTN_2:
      Serial.println(" - Entering TONE mode");
      playMorseWord("TONE");
      menuState.currentMode = MODE_TONE_SETTING;
      if (adapter) {
        menuState.tempToneNote = adapter->getTxNote();
      }
      // Clamp to middle third range (MIDI 43-85)
      if (menuState.tempToneNote < 43) menuState.tempToneNote = 43;
      if (menuState.tempToneNote > 85) menuState.tempToneNote = 85;
      applyTemporaryTone(menuState.tempToneNote);  // Apply current tone so user can test
      menuState.lastActivityTime = currentTime;  // Reset timeout timer
      Serial.print("Current tone: MIDI note ");
      Serial.print(menuState.tempToneNote);
      Serial.print(" (");
      Serial.print(equalTemperamentNote[menuState.tempToneNote]);
      Serial.println(" Hz)");
      break;
    case BTN_3:
      Serial.println(" - Entering KEY TYPE mode");
      playMorseWord("KEY");
      menuState.currentMode = MODE_KEY_SETTING;
      if (adapter) {
        menuState.tempKeyerType = adapter->getCurrentKeyerType();
      }
      // Ensure we're using a valid keyer type (1-9)
      if (menuState.tempKeyerType < KEYER_MIN || menuState.tempKeyerType > KEYER_MAX) {
        menuState.tempKeyerType = 8;  // Default to Iambic B
      }
      applyTemporaryKeyerType(menuState.tempKeyerType);  // Apply current keyer so user can test
      menuState.lastActivityTime = currentTime;  // Reset timeout timer
      Serial.print("Current keyer type: ");
      Serial.println(getKeyerTypeName(menuState.tempKeyerType));
      break;
    default:
      Serial.println();
      break;
  }
}

static void handleLongPressSpeedMode(ButtonState currentState) {
  // In speed mode: B2 long press saves and exits
  if (currentState == BTN_2 && adapter) {
    Serial.println(" - Saving and exiting SPEED mode");

    // Convert WPM to dit duration and apply to adapter
    uint16_t newDitDuration = wpmToDitDuration(menuState.tempSpeedWPM);

    // Update adapter settings via MIDI commands (same as loadSettingsFromEEPROM does)
    midiEventPacket_t event;
    event.header = 0x0B;
    event.byte1 = 0xB0;
    event.byte2 = 1;
    event.byte3 = newDitDuration / (2 * MILLISECOND);
    adapter->HandleMIDI(event);

    // Save to EEPROM
    saveSettingsToEEPROM(adapter->getCurrentKeyerType(), newDitDuration, adapter->getTxNote());

    Serial.print("Saved speed: ");
    Serial.print(menuState.tempSpeedWPM);
    Serial.print(" WPM (");
    Serial.print(newDitDuration);
    Serial.println("ms dit duration)");

    // Play confirmation and return to normal mode
    playMorseWord("RR");
    menuState.currentMode = MODE_NORMAL;
  }
}

static void handleLongPressToneMode(ButtonState currentState) {
  // In tone mode: B2 long press saves and exits
  if (currentState == BTN_2 && adapter) {
    Serial.println(" - Saving and exiting TONE mode");

    // Update adapter settings via MIDI commands
    midiEventPacket_t event;
    event.header = 0x0B;
    event.byte1 = 0xB0;
    event.byte2 = 2;
    event.byte3 = menuState.tempToneNote;
    adapter->HandleMIDI(event);

    // Save to EEPROM
    saveSettingsToEEPROM(adapter->getCurrentKeyerType(), adapter->getDitDuration(), menuState.tempToneNote);

    Serial.print("Saved tone: MIDI note ");
    Serial.print(menuState.tempToneNote);
    Serial.print(" (");
    Serial.print(equalTemperamentNote[menuState.tempToneNote]);
    Serial.println(" Hz)");

    // Play confirmation and return to normal mode
    playMorseWord("RR");
    menuState.currentMode = MODE_NORMAL;
  }
}

static void handleLongPressKeyMode(ButtonState currentState) {
  // In key type mode: B2 long press saves and exits
  if (currentState == BTN_2 && adapter) {
    Serial.println(" - Saving and exiting KEY TYPE mode");

    // Update adapter settings via MIDI commands
    midiEventPacket_t event;
    event.header = 0x0C;
    event.byte1 = 0xC0;
    event.byte2 = menuState.tempKeyerType;
    event.byte3 = 0;
    adapter->HandleMIDI(event);

    // Flush bounce state after final mode switch to prevent stuck keys
    if (flushBounceCallback) {
      flushBounceCallback();
    }

    // Save to EEPROM
    saveSettingsToEEPROM(menuState.tempKeyerType, adapter->getDitDuration(), adapter->getTxNote());

    Serial.print("Saved keyer type: ");
    Serial.println(getKeyerTypeName(menuState.tempKeyerType));

    // Play confirmation and return to normal mode
    playMorseWord("RR");
    menuState.currentMode = MODE_NORMAL;
  }
}

static void handleLongPressMemoryManagementMode(ButtonState currentState) {
  // In memory management mode: long press clears the memory slot
  uint8_t slotNumber = 0;
  if (currentState == BTN_1) slotNumber = 0;
  else if (currentState == BTN_2) slotNumber = 1;
  else if (currentState == BTN_3) slotNumber = 2;
  else {
    Serial.println();
    return;
  }

  Serial.print(" - Clearing memory slot ");
  Serial.println(slotNumber + 1);

  memorySlots[slotNumber].clear();
  clearMemoryInEEPROM(slotNumber);

  playMemoryClearedAnnouncement(slotNumber);
}

// ============================================================================
// Timeout Handlers (by mode)
// ============================================================================

static void handleTimeoutSpeedMode(unsigned long currentTime) {
  if ((currentTime - menuState.lastActivityTime) >= SETTING_MODE_TIMEOUT && adapter) {
    Serial.println(">>> TIMEOUT - Auto-saving and exiting SPEED mode");

    // Save current settings
    uint16_t newDitDuration = wpmToDitDuration(menuState.tempSpeedWPM);
    midiEventPacket_t event;
    event.header = 0x0B;
    event.byte1 = 0xB0;
    event.byte2 = 1;
    event.byte3 = newDitDuration / (2 * MILLISECOND);
    adapter->HandleMIDI(event);
    saveSettingsToEEPROM(adapter->getCurrentKeyerType(), newDitDuration, adapter->getTxNote());

    Serial.print("Auto-saved speed: ");
    Serial.print(menuState.tempSpeedWPM);
    Serial.println(" WPM");

    // Play descending tones and return to normal mode
    playDescendingTones();
    menuState.currentMode = MODE_NORMAL;
  }
}

static void handleTimeoutToneMode(unsigned long currentTime) {
  if ((currentTime - menuState.lastActivityTime) >= SETTING_MODE_TIMEOUT && adapter) {
    Serial.println(">>> TIMEOUT - Auto-saving and exiting TONE mode");

    // Save current settings
    midiEventPacket_t event;
    event.header = 0x0B;
    event.byte1 = 0xB0;
    event.byte2 = 2;
    event.byte3 = menuState.tempToneNote;
    adapter->HandleMIDI(event);
    saveSettingsToEEPROM(adapter->getCurrentKeyerType(), adapter->getDitDuration(), menuState.tempToneNote);

    Serial.print("Auto-saved tone: MIDI note ");
    Serial.print(menuState.tempToneNote);
    Serial.print(" (");
    Serial.print(equalTemperamentNote[menuState.tempToneNote]);
    Serial.println(" Hz)");

    // Play descending tones and return to normal mode
    playDescendingTones();
    menuState.currentMode = MODE_NORMAL;
  }
}

static void handleTimeoutKeyMode(unsigned long currentTime) {
  if ((currentTime - menuState.lastActivityTime) >= SETTING_MODE_TIMEOUT && adapter) {
    Serial.println(">>> TIMEOUT - Auto-saving and exiting KEY TYPE mode");

    // Save current settings
    midiEventPacket_t event;
    event.header = 0x0C;
    event.byte1 = 0xC0;
    event.byte2 = menuState.tempKeyerType;
    event.byte3 = 0;
    adapter->HandleMIDI(event);
    saveSettingsToEEPROM(menuState.tempKeyerType, adapter->getDitDuration(), adapter->getTxNote());

    Serial.print("Auto-saved keyer type: ");
    Serial.println(getKeyerTypeName(menuState.tempKeyerType));

    // Play descending tones and return to normal mode
    playDescendingTones();
    menuState.currentMode = MODE_NORMAL;
  }
}

// ============================================================================
// Main Menu Update Function
// ============================================================================

void updateMenuHandler(unsigned long currentTime, ButtonDebouncer& buttonDebouncer) {
  if (!adapter || !memorySlots || !recordingState || !playbackState) return;

  // Read button state with debouncing
  int analogVal = readButtonAnalog();
  ButtonState currentButtonState = getButtonState(analogVal);

  // Reset activity timer on any button press in setting modes
  if (currentButtonState != BTN_NONE && menuState.currentMode != MODE_NORMAL) {
    menuState.lastActivityTime = currentTime;
  }

  // Update debouncer - returns true when complete gesture detected (press & release)
  if (buttonDebouncer.update(currentButtonState, currentTime)) {
    ButtonState gestureDetected = buttonDebouncer.getMaxState();
    unsigned long duration = buttonDebouncer.getLastPressDuration();

    Serial.print("Button Gesture: ");
    Serial.print(buttonStateToString(gestureDetected));

    // Check for double-click FIRST to trigger recording (before handling quick press)
    bool isDoubleClick = buttonDebouncer.isDoubleClick();

    if (isDoubleClick && menuState.currentMode == MODE_MEMORY_MANAGEMENT) {
      // Only allow double-click recording in memory management mode
      uint8_t slotNumber = 0;
      if (gestureDetected == BTN_1) slotNumber = 0;
      else if (gestureDetected == BTN_2) slotNumber = 1;
      else if (gestureDetected == BTN_3) slotNumber = 2;
      else return;  // Not a single button double-click

      Serial.print(" [DOUBLE-CLICK]");
      Serial.print(">>> DOUBLE-CLICK DETECTED on Button ");
      Serial.print(slotNumber + 1);
      Serial.println(" - Starting recording...");

      // Stop any ongoing playback before starting recording
      if (playbackState->isPlaying) {
        Serial.println("Stopping playback before starting recording");
        playbackState->stopPlayback();
      }

      // Play countdown: "doot, doot, dah" (3 beeps with the last one longer)
      playRecordingCountdown();

      // Start recording
      startRecording(*recordingState, slotNumber);

      // Switch to recording mode
      if (slotNumber == 0) menuState.currentMode = MODE_RECORDING_MEMORY_1;
      else if (slotNumber == 1) menuState.currentMode = MODE_RECORDING_MEMORY_2;
      else if (slotNumber == 2) menuState.currentMode = MODE_RECORDING_MEMORY_3;

      Serial.print("Entered recording mode for memory slot ");
      Serial.println(slotNumber + 1);
      return;  // Exit early - don't process as quick press
    }

    if (duration >= 2000) {
      Serial.print(" [LONG PRESS - ");
      Serial.print(duration);
      Serial.println("ms]");
    } else {
      Serial.print(" [quick press - ");
      Serial.print(duration);
      Serial.println("ms]");

      // Handle quick presses based on current mode (only if NOT a double-click)
      switch (menuState.currentMode) {
        case MODE_SPEED_SETTING:
          handleQuickPressSpeedMode(gestureDetected);
          break;
        case MODE_TONE_SETTING:
          handleQuickPressToneMode(gestureDetected);
          break;
        case MODE_KEY_SETTING:
          handleQuickPressKeyMode(gestureDetected);
          break;
        case MODE_NORMAL:
          handleQuickPressNormalMode(gestureDetected);
          break;
        case MODE_RECORDING_MEMORY_1:
        case MODE_RECORDING_MEMORY_2:
        case MODE_RECORDING_MEMORY_3:
          handleQuickPressRecordingMode(gestureDetected);
          break;
        case MODE_MEMORY_MANAGEMENT:
          handleQuickPressMemoryManagementMode(gestureDetected);
          break;
        default:
          break;
      }
    }
  }

  // Check for long press threshold (2 seconds) - fires once while button still held
  if (buttonDebouncer.isLongPress(currentTime)) {
    ButtonState currentState = buttonDebouncer.getMaxState();
    Serial.print(">>> LONG PRESS DETECTED: ");
    Serial.print(buttonStateToString(currentState));

    // Handle based on current mode
    switch (menuState.currentMode) {
      case MODE_NORMAL:
        handleLongPressNormalMode(currentState, currentTime);
        break;
      case MODE_SPEED_SETTING:
        handleLongPressSpeedMode(currentState);
        break;
      case MODE_TONE_SETTING:
        handleLongPressToneMode(currentState);
        break;
      case MODE_KEY_SETTING:
        handleLongPressKeyMode(currentState);
        break;
      case MODE_MEMORY_MANAGEMENT:
        handleLongPressMemoryManagementMode(currentState);
        break;
      default:
        break;
    }
  }

  // Check for combo press threshold (0.5 seconds) - fires once while buttons still held
  if (buttonDebouncer.isComboPress(currentTime)) {
    ButtonState currentState = buttonDebouncer.getMaxState();
    Serial.print(">>> COMBO PRESS DETECTED: ");
    Serial.print(buttonStateToString(currentState));

    // B1+B3 combo toggles memory management mode
    if (currentState == BTN_1_3) {
      if (menuState.currentMode == MODE_NORMAL) {
        Serial.println(" - Entering MEMORY MANAGEMENT mode");
        playMorseWord("MEM");
        menuState.currentMode = MODE_MEMORY_MANAGEMENT;
      } else if (menuState.currentMode == MODE_MEMORY_MANAGEMENT) {
        Serial.println(" - Exiting MEMORY MANAGEMENT mode");
        playDescendingTones();
        menuState.currentMode = MODE_NORMAL;
      }
    } else {
      Serial.println();
    }
  }

  // Check for MIDI switch press threshold (3 seconds) - fires once while B1+B2 still held
  // ONLY active in MODE_NORMAL to avoid conflicts with other modes
  if (buttonDebouncer.isMidiSwitchPress(currentTime) && menuState.currentMode == MODE_NORMAL) {
    Serial.print(">>> MIDI SWITCH PRESS DETECTED (3s B1+B2): ");

    if (adapter) {
      bool currentMode = adapter->KeyboardMode();

      if (currentMode) {
        // Currently in keyboard mode, switch to MIDI mode
        Serial.println("Switching from Keyboard to MIDI mode");
        midiEventPacket_t event;
        event.header = 0x0B;
        event.byte1 = 0xB0;
        event.byte2 = 0;
        event.byte3 = 0x00;  // 0x00 = MIDI mode (< 0x3f)
        adapter->HandleMIDI(event);
        playMorseChar('M');  // M
        playMorseChar('M');  // M -> "MM" = MIDI Mode
      } else {
        // Currently in MIDI mode, switch to keyboard mode
        Serial.println("Switching from MIDI to Keyboard mode");
        midiEventPacket_t event;
        event.header = 0x0B;
        event.byte1 = 0xB0;
        event.byte2 = 0;
        event.byte3 = 0x7F;  // 0x7F = Keyboard mode (> 0x3f)
        adapter->HandleMIDI(event);
        playMorseChar('K');  // K
        playMorseChar('M');  // M -> "KM" = Keyboard Mode
      }
    }
  }

  // Check for timeout in setting modes
  switch (menuState.currentMode) {
    case MODE_SPEED_SETTING:
      handleTimeoutSpeedMode(currentTime);
      break;
    case MODE_TONE_SETTING:
      handleTimeoutToneMode(currentTime);
      break;
    case MODE_KEY_SETTING:
      handleTimeoutKeyMode(currentTime);
      break;
    default:
      break;
  }
}
