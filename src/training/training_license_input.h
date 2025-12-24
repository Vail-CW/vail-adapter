/*
 * Ham Radio License Study - Input Handlers
 * Keyboard input routing for license selection, quiz, and stats modes
 */

#ifndef TRAINING_LICENSE_INPUT_H
#define TRAINING_LICENSE_INPUT_H

#include "../core/config.h"
#include "../audio/i2s_audio.h"
#include "training_license_core.h"
#include "training_license_data.h"
#include "training_license_ui.h"
#include "training_license_stats.h"
#include "training_license_downloader.h"

// Forward declarations
extern MenuMode currentMode;
extern int currentSelection;

void startLicenseQuiz(LGFX& tft, int licenseType);
void startLicenseStats(LGFX& tft);

// ============================================
// License Quiz Input Handler
// ============================================

/**
 * Handle input for quiz mode
 * Returns: -1 to exit, 0 for no action, 2 for redraw
 */
int handleLicenseQuizInput(char key, LGFX& tft) {
  // ESC - exit quiz
  if (key == KEY_ESC) {
    return -1;
  }

  // If showing feedback, any key advances to next question
  if (licenseSession.showingFeedback) {
    advanceToNextQuestion();
    return 2;  // Redraw
  }

  // Direct answer selection (A/B/C/D keys)
  if (key >= 'A' && key <= 'D') {
    int answerIdx = key - 'A';
    licenseSession.selectedAnswerIndex = answerIdx;
    submitAnswer(answerIdx);
    return 2;  // Redraw with feedback
  }

  if (key >= 'a' && key <= 'd') {
    int answerIdx = key - 'a';
    licenseSession.selectedAnswerIndex = answerIdx;
    submitAnswer(answerIdx);
    return 2;  // Redraw with feedback
  }

  // Arrow navigation (up/down/left/right all cycle answers)
  if (key == KEY_UP || key == KEY_LEFT) {
    if (licenseSession.selectedAnswerIndex > 0) {
      licenseSession.selectedAnswerIndex--;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;  // Redraw
    }
  } else if (key == KEY_DOWN || key == KEY_RIGHT) {
    if (licenseSession.selectedAnswerIndex < 3) {
      licenseSession.selectedAnswerIndex++;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;  // Redraw
    }
  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Submit selected answer
    if (licenseSession.selectedAnswerIndex >= 0 && licenseSession.selectedAnswerIndex <= 3) {
      submitAnswer(licenseSession.selectedAnswerIndex);
      return 2;  // Redraw with feedback
    }
  }

  return 0;  // No action
}

// ============================================
// Statistics Screen Input Handler
// ============================================

/**
 * Handle input for statistics screen
 * Returns: -1 to exit, 0 for no action
 */
int handleLicenseStatsInput(char key, LGFX& tft) {
  // ESC - return to license select
  if (key == KEY_ESC) {
    return -1;
  }

  return 0;  // No action
}

// ============================================
// Mode Start Functions
// ============================================

/**
 * Start quiz mode for selected license
 * NOTE: LVGL version is now used - this legacy function is kept for compatibility
 * but no longer draws UI (LVGL handles all rendering)
 */
void startLicenseQuiz(LGFX& tft, int licenseType) {
  Serial.println("[LicenseQuiz] Legacy startLicenseQuiz called - UI handled by LVGL");

  // Unload previous pool if different
  if (activePool && activePool->loaded && licenseSession.selectedLicense != licenseType) {
    unloadLicenseProgress(activePool);
    unloadQuestionPool(activePool);
    activePool = nullptr;
  }

  // Get question pool for selected license
  QuestionPool* pool = getQuestionPool(licenseType);
  if (!pool) {
    Serial.println("ERROR: Invalid license type");
    return;
  }

  // NOTE: File downloading is now handled by LVGL in initializeModeInt()
  // Files should already exist when this function is called

  // Load question pool from SD card
  if (!pool->loaded) {
    if (!loadQuestionPool(pool)) {
      Serial.println("ERROR: Failed to load question pool");
      // NOTE: Error UI is handled by LVGL - don't draw legacy UI
      return;
    }
  }

  // Load progress from Preferences
  if (!pool->progress) {
    loadLicenseProgress(licenseType);
  }

  // Set active pool
  activePool = pool;

  // Start session
  startLicenseSession(licenseType);

  // NOTE: Don't set currentMode here - LVGL mode integration handles this
  // NOTE: Don't draw UI here - LVGL handles all rendering

  Serial.print("Started quiz for ");
  Serial.print(getLicenseName(licenseType));
  Serial.print(" (");
  Serial.print(pool->totalQuestions);
  Serial.println(" questions)");
}

/**
 * Start statistics view
 * NOTE: LVGL version is now used - this legacy function is kept for compatibility
 * but no longer draws UI (LVGL handles all rendering)
 */
void startLicenseStats(LGFX& tft) {
  Serial.println("[LicenseStats] Legacy startLicenseStats called - UI handled by LVGL");

  // Make sure we have an active pool
  if (!activePool || !activePool->loaded) {
    // NOTE: File downloading is now handled by LVGL in initializeModeInt()

    // Try to load the selected license pool
    QuestionPool* pool = getQuestionPool(licenseSession.selectedLicense);
    if (pool && !pool->loaded) {
      if (!loadQuestionPool(pool)) {
        Serial.println("ERROR: Failed to load question pool");
        // NOTE: Error UI is handled by LVGL - don't draw legacy UI
        return;
      }
      activePool = pool;
    }

    // Load progress if not already loaded
    if (activePool && !activePool->progress) {
      loadLicenseProgress(licenseSession.selectedLicense);
    }
  }

  // NOTE: Don't set currentMode here - LVGL mode integration handles this

  // Calculate statistics (but don't draw - LVGL handles rendering)
  updateCurrentStatistics();
  // NOTE: Don't draw UI here - LVGL handles all rendering

  Serial.print("Showing statistics for ");
  Serial.println(getLicenseName(licenseSession.selectedLicense));
}

#endif // TRAINING_LICENSE_INPUT_H
