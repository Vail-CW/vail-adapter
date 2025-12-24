/*
 * Practice Oscillator Mode
 * Allows free-form morse code practice with paddle/key
 * Includes real-time morse decoding with adaptive speed tracking
 */

#ifndef TRAINING_PRACTICE_H
#define TRAINING_PRACTICE_H

#include "../core/config.h"
#include "../settings/settings_cw.h"
#include "../audio/morse_decoder_adaptive.h"


// Practice mode state
bool practiceActive = false;
bool ditPressed = false;
bool dahPressed = false;
bool lastDitPressed = false;
bool lastDahPressed = false;
unsigned long practiceStartupTime = 0;  // Track startup time for input delay

// Deferred save state - debounces rapid setting changes when holding keys
unsigned long lastSettingSaveTime = 0;
bool settingSavePending = false;
#define SETTING_SAVE_DEBOUNCE_MS 500  // Save 500ms after last change

// Iambic keyer state
unsigned long ditDahTimer = 0;
bool keyerActive = false;
bool sendingDit = false;
bool sendingDah = false;
bool inSpacing = false;  // True when in inter-element gap
bool ditMemory = false;
bool dahMemory = false;
unsigned long elementStartTime = 0;
int ditDuration = 0;

// Statistics
unsigned long practiceStartTime = 0;

// Decoder state
MorseDecoderAdaptive decoder(20, 20, 30);  // Initial 20 WPM, buffer size 30
String decodedText = "";
String decodedMorse = "";
bool showDecoding = true;
bool needsUIUpdate = false;

// Timing capture for decoder
unsigned long lastStateChangeTime = 0;
bool lastToneState = false;
unsigned long lastElementTime = 0;  // Track last element for timeout flush

// Forward declarations
void startPracticeMode(LGFX &display);
void updatePracticeOscillator();
void straightKeyHandler();
void iambicKeyerHandler();

// LVGL-callable action functions
void practiceHandleEsc();
void practiceHandleClear();
void practiceAdjustSpeed(int delta);
void practiceCycleKeyType(int direction);
void practiceToggleDecoding();
void practiceCheckDeferredSave();

// Start practice mode
void startPracticeMode(LGFX &display) {
  practiceActive = true;
  ditPressed = false;
  dahPressed = false;
  keyerActive = false;
  inSpacing = false;
  ditMemory = false;
  dahMemory = false;
  practiceStartupTime = millis();  // Record startup time for input delay

  // Clear any lingering touch sensor state
  touchRead(TOUCH_DIT_PIN);
  touchRead(TOUCH_DAH_PIN);
  delay(50);

  // Reinitialize I2S to ensure clean state
  Serial.println("Reinitializing I2S for practice mode...");
  i2s_zero_dma_buffer(I2S_NUM_0);
  delay(50);

  // Calculate dit duration from current speed setting
  ditDuration = DIT_DURATION(cwSpeed);

  // Reset statistics
  practiceStartTime = millis();

  // Reset decoder and clear any buffered state
  decoder.reset();
  decoder.flush();  // Clear any pending timings
  decoder.setWPM(cwSpeed);
  decodedText = "";
  decodedMorse = "";
  lastStateChangeTime = 0;  // Don't initialize until first key press
  lastToneState = false;
  lastElementTime = 0;  // Reset element timeout tracker
  showDecoding = true;
  needsUIUpdate = false;

  // Setup decoder callbacks
  decoder.messageCallback = [](String morse, String text) {
    // Process each character in the decoded text individually
    for (int i = 0; i < text.length(); i++) {
      // Check if adding this character would exceed our 32-char limit
      if (decodedText.length() >= 32) {
        // Clear everything and start fresh
        decodedText = "";
        decodedMorse = "";
      }

      // Add the character
      decodedText += text[i];
    }

    // Also track morse pattern
    if (decodedMorse.length() + morse.length() > 100) {
      decodedMorse = "";  // Clear morse if it gets too long
    }
    decodedMorse += morse;

    needsUIUpdate = true;

    Serial.print("Decoded: ");
    Serial.print(text);
    Serial.print(" (");
    Serial.print(morse);
    Serial.print(") -> Total length: ");
    Serial.println(decodedText.length());
  };

  decoder.speedCallback = [](float wpm, float fwpm) {
    Serial.print("Speed detected: ");
    Serial.print(wpm);
    Serial.println(" WPM");
  };

  Serial.println("Practice mode started with decoding enabled");
  Serial.print("Speed: ");
  Serial.print(cwSpeed);
  Serial.print(" WPM, Tone: ");
  Serial.print(cwTone);
  Serial.print(" Hz, Key type: ");
  if (cwKeyType == KEY_STRAIGHT) {
    Serial.println("Straight");
  } else if (cwKeyType == KEY_IAMBIC_A) {
    Serial.println("Iambic A");
  } else {
    Serial.println("Iambic B");
  }
}


// Update practice oscillator (called in main loop)
void updatePracticeOscillator() {
  if (!practiceActive) return;

  // Check for deferred settings save
  practiceCheckDeferredSave();

  // Ignore all input for first 1000ms to prevent startup glitches
  if (millis() - practiceStartupTime < 1000) {
    return;
  }

  // Check for decoder timeout (flush if no activity for word gap duration)
  // This is a backup to flush any buffered data if user stops keying mid-character
  if (showDecoding && lastElementTime > 0 && !ditPressed && !dahPressed) {
    unsigned long timeSinceLastElement = millis() - lastElementTime;
    float wordGapDuration = MorseWPM::wordGap(decoder.getWPM());

    // Flush buffered data after word gap silence (backup timeout)
    if (timeSinceLastElement > wordGapDuration) {
      decoder.flush();
      lastElementTime = 0;  // Reset timeout
    }
  }

  // Read paddle/key inputs (physical + capacitive touch)
  // ESP32-S3: Use GPIO numbers directly, check > threshold (values rise when touched)
  ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  // Handle based on key type
  if (cwKeyType == KEY_STRAIGHT) {
    straightKeyHandler();
  } else {
    iambicKeyerHandler();
  }

  // Update visual feedback if state changed
  if (ditPressed != lastDitPressed || dahPressed != lastDahPressed) {
    // Will be redrawn in main loop
    lastDitPressed = ditPressed;
    lastDahPressed = dahPressed;
  }
}

// Straight key handler (simple on/off)
void straightKeyHandler() {
  unsigned long currentTime = millis();
  bool toneOn = isTonePlaying();

  // Use DIT pin as straight key
  if (ditPressed && !toneOn) {
    // Tone starting
    if (showDecoding && lastToneState == false) {
      // Send silence duration to decoder (negative)
      // Only if we have a valid previous state change time
      if (lastStateChangeTime > 0) {
        float silenceDuration = currentTime - lastStateChangeTime;
        if (silenceDuration > 0) {
          decoder.addTiming(-silenceDuration);
        }
      }
      lastStateChangeTime = currentTime;
      lastToneState = true;
    }

    startTone(cwTone);
  }
  else if (ditPressed && toneOn) {
    // Tone continuing
    continueTone(cwTone);
  }
  else if (!ditPressed && toneOn) {
    // Tone stopping
    if (showDecoding && lastToneState == true) {
      // Send tone duration to decoder (positive)
      float toneDuration = currentTime - lastStateChangeTime;
      if (toneDuration > 0) {
        decoder.addTiming(toneDuration);
        lastElementTime = currentTime;  // Update timeout tracker
      }
      lastStateChangeTime = currentTime;
      lastToneState = false;
    }

    stopTone();
  }
}

// Iambic keyer handler (Mode A or B)
void iambicKeyerHandler() {
  unsigned long currentTime = millis();

  // If not actively sending or spacing, check for new input
  if (!keyerActive && !inSpacing) {
    if (ditPressed || ditMemory) {
      // Start sending dit
      if (showDecoding && lastToneState == false) {
        // Send silence duration to decoder - let decoder filter inter-element gaps
        // Only if we have a valid previous state change time
        if (lastStateChangeTime > 0) {
          float silenceDuration = currentTime - lastStateChangeTime;
          if (silenceDuration > 0) {
            decoder.addTiming(-silenceDuration);
          }
        }
        lastStateChangeTime = currentTime;
        lastToneState = true;
      }

      keyerActive = true;
      sendingDit = true;
      sendingDah = false;
      inSpacing = false;
      elementStartTime = currentTime;
      startTone(cwTone);

      // Clear dit memory
      ditMemory = false;
    }
    else if (dahPressed || dahMemory) {
      // Start sending dah
      if (showDecoding && lastToneState == false) {
        // Send silence duration to decoder - let decoder filter inter-element gaps
        // Only if we have a valid previous state change time
        if (lastStateChangeTime > 0) {
          float silenceDuration = currentTime - lastStateChangeTime;
          if (silenceDuration > 0) {
            decoder.addTiming(-silenceDuration);
          }
        }
        lastStateChangeTime = currentTime;
        lastToneState = true;
      }

      keyerActive = true;
      sendingDit = false;
      sendingDah = true;
      inSpacing = false;
      elementStartTime = currentTime;
      startTone(cwTone);

      // Clear dah memory
      dahMemory = false;
    }
  }
  // Currently sending an element
  else if (keyerActive && !inSpacing) {
    unsigned long elementDuration = sendingDit ? ditDuration : (ditDuration * 3);

    // Keep tone playing
    continueTone(cwTone);

    // Continuously check for paddle input during element send
    if (ditPressed && dahPressed) {
      // Both pressed (squeeze) - remember opposite paddle
      if (sendingDit) {
        dahMemory = true;
      } else {
        ditMemory = true;
      }
    }
    else if (sendingDit && dahPressed) {
      // Sending dit, dah pressed
      dahMemory = true;
    }
    else if (sendingDah && ditPressed) {
      // Sending dah, dit pressed
      ditMemory = true;
    }

    // Check if element is complete
    if (currentTime - elementStartTime >= elementDuration) {
      // Element complete, turn off tone and start spacing
      if (showDecoding && lastToneState == true) {
        // Send tone duration to decoder
        float toneDuration = currentTime - lastStateChangeTime;
        if (toneDuration > 0) {
          decoder.addTiming(toneDuration);
          lastElementTime = currentTime;  // Update timeout tracker
        }
        lastStateChangeTime = currentTime;
        lastToneState = false;
      }

      stopTone();
      keyerActive = false;
      sendingDit = false;
      sendingDah = false;
      inSpacing = true;
      elementStartTime = currentTime;  // Reset timer for spacing
    }
  }
  // In inter-element spacing
  else if (inSpacing) {
    // Continue checking paddles during spacing to catch input
    if (ditPressed && dahPressed) {
      // Both pressed - if we just sent dit, remember dah (and vice versa)
      // Can't determine what we just sent, so set both memories
      ditMemory = true;
      dahMemory = true;
    }
    else if (ditPressed && !ditMemory) {
      ditMemory = true;
    }
    else if (dahPressed && !dahMemory) {
      dahMemory = true;
    }

    // Wait for 1 dit duration (inter-element gap)
    if (currentTime - elementStartTime >= ditDuration) {
      inSpacing = false;
      // Now ready to send next element if memory is set or paddle still pressed
    }
  }
}

// ============================================
// LVGL-Callable Action Functions
// ============================================

// Handle ESC key - stop practice and prepare for exit
void practiceHandleEsc() {
  practiceActive = false;
  stopTone();
  decoder.flush();  // Decode any remaining buffered timings

  // Save any pending settings before exit
  if (settingSavePending) {
    saveCWSettings();
    settingSavePending = false;
    Serial.println("[Practice] Saved pending settings on exit");
  }

  Serial.println("[Practice] ESC - exiting practice mode");
}

// Clear decoder text
void practiceHandleClear() {
  decodedText = "";
  decodedMorse = "";
  decoder.reset();
  decoder.flush();
  needsUIUpdate = true;  // Signal LVGL to update display
  beep(TONE_MENU_NAV, BEEP_SHORT);
  Serial.println("[Practice] Cleared decoder text");
}

// Adjust WPM speed (delta can be any value, e.g., 1, 2, 4 based on acceleration)
void practiceAdjustSpeed(int delta) {
  int newSpeed = cwSpeed + delta;
  if (newSpeed >= WPM_MIN && newSpeed <= WPM_MAX) {
    cwSpeed = newSpeed;
    ditDuration = DIT_DURATION(cwSpeed);
    decoder.setWPM(cwSpeed);

    // Mark save as pending instead of immediate save (debounces rapid changes)
    settingSavePending = true;
    lastSettingSaveTime = millis();

    beep(TONE_MENU_NAV, BEEP_SHORT);
    Serial.printf("[Practice] Speed changed to %d WPM (save pending)\n", cwSpeed);
  }
}

// Check and perform deferred save of CW settings
// Call this from updatePracticeOscillator() to save after debounce period
void practiceCheckDeferredSave() {
  if (settingSavePending && (millis() - lastSettingSaveTime > SETTING_SAVE_DEBOUNCE_MS)) {
    saveCWSettings();
    settingSavePending = false;
    Serial.println("[Practice] Deferred CW settings save completed");
  }
}

// Cycle key type (+1 forward, -1 backward)
void practiceCycleKeyType(int direction) {
  if (direction > 0) {
    // Cycle forward: Straight -> Iambic A -> Iambic B -> Straight
    if (cwKeyType == KEY_STRAIGHT) {
      cwKeyType = KEY_IAMBIC_A;
    } else if (cwKeyType == KEY_IAMBIC_A) {
      cwKeyType = KEY_IAMBIC_B;
    } else {
      cwKeyType = KEY_STRAIGHT;
    }
  } else {
    // Cycle backward: Iambic B -> Iambic A -> Straight -> Iambic B
    if (cwKeyType == KEY_IAMBIC_B) {
      cwKeyType = KEY_IAMBIC_A;
    } else if (cwKeyType == KEY_IAMBIC_A) {
      cwKeyType = KEY_STRAIGHT;
    } else {
      cwKeyType = KEY_IAMBIC_B;
    }
  }

  // Mark save as pending (use same debounce as speed changes)
  settingSavePending = true;
  lastSettingSaveTime = millis();

  beep(TONE_MENU_NAV, BEEP_SHORT);

  const char* keyTypeStr = (cwKeyType == KEY_STRAIGHT) ? "Straight" :
                           (cwKeyType == KEY_IAMBIC_A) ? "Iambic A" : "Iambic B";
  Serial.printf("[Practice] Key type changed to %s (save pending)\n", keyTypeStr);
}

// Toggle decoding display
void practiceToggleDecoding() {
  showDecoding = !showDecoding;
  beep(TONE_MENU_NAV, BEEP_SHORT);
  Serial.printf("[Practice] Decoding %s\n", showDecoding ? "enabled" : "disabled");
}

#endif // TRAINING_PRACTICE_H
