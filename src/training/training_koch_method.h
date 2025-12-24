/*
 * Training Mode: Koch Method
 * Progressive character introduction method for learning morse code
 * Main entry point - includes core logic and UI modules
 */

#ifndef TRAINING_KOCH_METHOD_H
#define TRAINING_KOCH_METHOD_H

#include "training_koch_core.h"  // Same folder
#include "training_koch_ui.h"  // Same folder

// ============================================
// Input Handling
// ============================================

int handleKochInput(char key, LGFX& tft) {
  // Handle tutorial mode input (mandatory on first launch)
  if (kochInTutorialMode) {
    if (key == ' ') {
      // Advance to next tutorial step
      if (kochTutorialStep < 2) {
        kochTutorialStep++;
        beep(TONE_SELECT, BEEP_SHORT);
        return 2;  // Full redraw
      } else {
        // Tutorial complete - mark user as no longer first-time
        kochInTutorialMode = false;
        kochFirstTimeUser = false;
        saveKochProgress();
        beep(TONE_SUCCESS, BEEP_LONG);
        Serial.println("Tutorial complete - starting training");
        return 2;  // Full redraw
      }
    }
    return 0;  // Ignore all other keys during tutorial
  }

  // Handle new character introduction screen
  if (kochShowingNewChar) {
    if (key == ' ') {
      // Play the new character 3 times, then show grid
      if (kochNewCharPlayCount < 3) {
        String charStr;
        charStr += kochNewCharacter;
        playMorseString(charStr.c_str(), kochProgress.wpm);
        kochNewCharPlayCount++;
        return 0;  // Keep showing intro
      } else {
        // Done playing, show character grid
        kochShowingNewChar = false;
        kochShowingGrid = true;
        return 2;  // Full redraw to grid
      }
    }
    return 0;  // Ignore other keys
  }

  // Handle character grid display
  if (kochShowingGrid) {
    // Any key exits grid
    kochShowingGrid = false;
    beep(TONE_SELECT, BEEP_SHORT);
    return 2;  // Full redraw back to main UI
  }

  // Handle character selection mode input (Practice mode only)
  if (kochInCharSelectMode) {
    if (key == KEY_ESC) {
      kochInCharSelectMode = false;
      return 2;
    }

    if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      kochInCharSelectMode = false;
      // Reset practice stats when changing characters
      kochPracticeCorrect = 0;
      kochPracticeTotal = 0;
      beep(TONE_SELECT, BEEP_SHORT);
      return 2;
    }

    // Toggle character selection
    if (isalnum(key) || key == ',' || key == '?' || key == '/') {
      char upperKey = toupper(key);
      String available = getKochCharacterSet();

      // Check if character is in available set
      if (available.indexOf(upperKey) >= 0) {
        int idx = kochPracticeChars.indexOf(upperKey);
        if (idx >= 0) {
          // Deselect
          kochPracticeChars.remove(idx, 1);
          beep(TONE_MENU_NAV, BEEP_SHORT);
        } else if (kochPracticeChars.length() < 5) {
          // Select (max 5)
          kochPracticeChars += upperKey;
          beep(TONE_SELECT, BEEP_SHORT);
        } else {
          // Max reached
          beep(TONE_ERROR, BEEP_SHORT);
        }
        return 2;
      }
    }
    return 0;
  }

  // Handle help mode input
  if (kochInHelpMode) {
    if (key == KEY_ESC) {
      kochInHelpMode = false;
      return 2;  // Full redraw
    }
    if (key == 'H' || key == 'h') {
      // Next help page
      kochHelpPage = (kochHelpPage + 1) % 3;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
    return 0;
  }

  // Handle settings mode input
  if (kochInSettingsMode) {
    if (key == KEY_ESC) {
      kochInSettingsMode = false;
      loadKochProgress();  // Reload to discard changes
      return 2;
    }

    if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      kochInSettingsMode = false;
      saveKochProgress();
      beep(TONE_SELECT, BEEP_SHORT);
      return 2;
    }

    // Quick preset keys
    if (key == '1') {
      kochProgress.wpm = 15;  // Beginner
      beep(TONE_SELECT, BEEP_SHORT);
      return 2;
    }
    if (key == '2') {
      kochProgress.wpm = 20;  // Standard
      beep(TONE_SELECT, BEEP_SHORT);
      return 2;
    }
    if (key == '3') {
      kochProgress.wpm = 25;  // Advanced
      beep(TONE_SELECT, BEEP_SHORT);
      return 2;
    }
    if (key == '4') {
      kochProgress.wpm = 30;  // Expert
      beep(TONE_SELECT, BEEP_SHORT);
      return 2;
    }

    if (key == KEY_UP) {
      kochSettingsSelection = (kochSettingsSelection > 0) ? kochSettingsSelection - 1 : 1;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }

    if (key == KEY_DOWN) {
      kochSettingsSelection = (kochSettingsSelection < 1) ? kochSettingsSelection + 1 : 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }

    if (key == KEY_LEFT) {
      if (kochSettingsSelection == 0 && kochProgress.wpm > KOCH_MIN_WPM) {
        kochProgress.wpm--;
        beep(TONE_MENU_NAV, BEEP_SHORT);
      } else if (kochSettingsSelection == 1 && kochProgress.groupLength > KOCH_MIN_GROUP_LENGTH) {
        kochProgress.groupLength--;
        beep(TONE_MENU_NAV, BEEP_SHORT);
      }
      return 2;
    }

    if (key == KEY_RIGHT) {
      if (kochSettingsSelection == 0 && kochProgress.wpm < KOCH_MAX_WPM) {
        kochProgress.wpm++;
        beep(TONE_MENU_NAV, BEEP_SHORT);
      } else if (kochSettingsSelection == 1 && kochProgress.groupLength < KOCH_MAX_GROUP_LENGTH) {
        kochProgress.groupLength++;
        beep(TONE_MENU_NAV, BEEP_SHORT);
      }
      return 2;
    }

    return 0;
  }

  // Handle reset hold detection
  if (key == 'R' || key == 'r') {
    if (!kochResetHoldActive) {
      kochResetHoldStartTime = millis();
      kochResetHoldActive = true;
      return 2;
    } else {
      if (millis() - kochResetHoldStartTime >= 3000) {
        resetKochProgress();
        kochResetHoldActive = false;
        startNewKochGroup();
        return 2;
      }
    }
    return 2;
  } else {
    if (kochResetHoldActive) {
      kochResetHoldActive = false;
      return 2;
    }
  }

  // Exit to training menu
  if (key == KEY_ESC) {
    saveKochProgress();
    return -1;
  }

  // Open help
  if (key == 'H' || key == 'h') {
    kochInHelpMode = true;
    kochHelpPage = 0;
    beep(TONE_SELECT, BEEP_SHORT);
    return 2;
  }

  // Open settings
  if (key == 'S' || key == 's') {
    kochInSettingsMode = true;
    kochSettingsSelection = 0;
    beep(TONE_SELECT, BEEP_SHORT);
    return 2;
  }

  // Show character grid
  if (key == 'G' || key == 'g') {
    kochShowingGrid = true;
    beep(TONE_SELECT, BEEP_SHORT);
    return 2;
  }

  // Toggle Practice/Test mode
  if (key == KEY_TAB) {
    if (kochCurrentMode == KOCH_MODE_PRACTICE) {
      // Switch to test mode
      kochCurrentMode = KOCH_MODE_TEST;
      // Don't reset test stats - keep current progress
    } else {
      // Switch to practice mode
      kochCurrentMode = KOCH_MODE_PRACTICE;
      // Reset practice stats
      kochPracticeCorrect = 0;
      kochPracticeTotal = 0;
      // Default to all chars if none selected
      if (kochPracticeChars.length() == 0) {
        kochPracticeChars = "";  // Empty means all chars
      }
    }
    beep(TONE_SELECT, BEEP_MEDIUM);
    return 2;
  }

  // Open character selector (Practice mode only)
  if ((key == 'C' || key == 'c') && kochCurrentMode == KOCH_MODE_PRACTICE) {
    kochInCharSelectMode = true;
    beep(TONE_SELECT, BEEP_SHORT);
    return 2;
  }

  // Manual lesson progression (Test mode only)
  if (key == '+' || key == '=') {
    if (kochCurrentMode == KOCH_MODE_TEST) {
      advanceLesson();
      startNewKochGroup();
    }
    return 2;
  }

  if (key == '-' || key == '_') {
    if (kochCurrentMode == KOCH_MODE_TEST) {
      regressLesson();
      startNewKochGroup();
    }
    return 2;
  }

  // Handle different states
  if (kochShowingFeedback) {
    // After feedback, any key starts next round
    startNewKochGroup();
    playKochGroup();
    return 2;
  }

  if (kochWaitingForInput) {
    // Handle text input
    if (key == KEY_BACKSPACE) {
      if (kochUserInput.length() > 0) {
        kochUserInput.remove(kochUserInput.length() - 1);
        return 3;
      }
    } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      if (kochUserInput.length() > 0) {
        checkKochAnswer(tft);
        return 2;
      }
    } else if (key == ' ') {
      // Replay current group
      playKochGroup();
      return 0;
    } else if (isalnum(key) || key == ',' || key == '?' || key == '/') {
      if (kochUserInput.length() < 15) {
        kochUserInput += (char)toupper(key);
        return 3;
      }
    }
    return 0;
  }

  // Ready state - space to start
  if (key == ' ') {
    startNewKochGroup();
    playKochGroup();
    return 2;
  }

  return 0;
}

// ============================================
// Mode Entry Point
// ============================================

void startKochMethod(LGFX& tft) {
  Serial.println("=== Starting Koch Method Training ===");

  // Load progress
  loadKochProgress();

  // Reset state
  kochWaitingForInput = false;
  kochShowingFeedback = false;
  kochInSettingsMode = false;
  kochInHelpMode = false;
  kochInCharSelectMode = false;
  kochResetHoldActive = false;
  kochUserInput = "";
  kochCurrentStreak = 0;  // Reset streak on mode entry

  // Default to test mode on startup
  kochCurrentMode = KOCH_MODE_TEST;
  kochPracticeChars = "";
  kochPracticeCorrect = 0;
  kochPracticeTotal = 0;

  // Check if first-time user - trigger mandatory tutorial
  if (kochFirstTimeUser) {
    kochInTutorialMode = true;
    kochTutorialStep = 0;
    Serial.println("First-time user - starting mandatory tutorial");
  } else {
    kochInTutorialMode = false;
    // Set welcome message for returning users
    kochCurrentMessage = "Ready to practice? Press SPACE to begin!";
    kochMessageColor = ST77XX_GREEN;
  }

  // Draw initial UI (will show tutorial if first-time)
  drawKochUI(tft);

  // Ready to start
  beep(TONE_STARTUP, BEEP_SHORT);

  Serial.print("Starting at lesson ");
  Serial.print(kochProgress.currentLesson);
  Serial.print(" with characters: ");
  Serial.println(getKochCharacterSet());
  Serial.println("Mode: TEST (Press P to switch to Practice mode)");
}

#endif // TRAINING_KOCH_METHOD_H
