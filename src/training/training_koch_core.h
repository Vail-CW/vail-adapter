/*
 * Koch Method - Core Logic
 * Character sets, progression, accuracy tracking, and state management
 */

#ifndef TRAINING_KOCH_CORE_H
#define TRAINING_KOCH_CORE_H

#include "../core/morse_code.h"
#include <Preferences.h>

// ============================================
// Koch Method Configuration
// ============================================

// Standard Koch Method character sequence (44 characters)
// Order: K M R S U A P T L O W I N J E F Y , V G 5 / Q 9 Z H 3 8 B ? 4 2 7 C 1 D 6 0 X
const char KOCH_SEQUENCE[] = "KMRSUAPTLOWINJEF Y,VG5/Q9ZH38B?427C1D60X";
const int KOCH_TOTAL_LESSONS = 44;

// Default settings
#define KOCH_DEFAULT_WPM 20
#define KOCH_MIN_WPM 15
#define KOCH_MAX_WPM 30
#define KOCH_DEFAULT_GROUP_LENGTH 5
#define KOCH_MIN_GROUP_LENGTH 3
#define KOCH_MAX_GROUP_LENGTH 10
#define KOCH_ACCURACY_THRESHOLD 90  // 90% accuracy to advance
#define KOCH_MIN_ATTEMPTS 10         // Minimum attempts before allowing progression

// ============================================
// Koch Progress Structure
// ============================================

struct KochProgress {
  int currentLesson;        // Current lesson number (1-44)
  int wpm;                  // Speed setting (15-30 WPM)
  int groupLength;          // Characters per group (3-10)
  int sessionCorrect;       // Correct answers this session
  int sessionTotal;         // Total attempts this session
};

// Global Koch state
KochProgress kochProgress = {
  1,                        // Start with lesson 1 (K and M)
  KOCH_DEFAULT_WPM,
  KOCH_DEFAULT_GROUP_LENGTH,
  0,
  0
};

// Training state
String kochCurrentGroup = "";
String kochUserInput = "";
bool kochWaitingForInput = false;
bool kochShowingFeedback = false;
bool kochCorrectAnswer = false;
bool kochInSettingsMode = false;
bool kochInHelpMode = false;
bool kochInCharSelectMode = false;
int kochSettingsSelection = 0;  // 0=WPM, 1=Group Length
int kochHelpPage = 0;  // Help screen page number
unsigned long kochResetHoldStartTime = 0;
bool kochResetHoldActive = false;

// Practice vs Test mode
enum KochMode {
  KOCH_MODE_TEST,      // Stats tracked, full character set, must pass to advance
  KOCH_MODE_PRACTICE   // Stats NOT tracked, can select specific chars (1-5)
};

KochMode kochCurrentMode = KOCH_MODE_TEST;  // Default to test mode
String kochPracticeChars = "";  // Selected characters for practice mode (1-5 chars)
int kochPracticeCorrect = 0;    // Practice mode stats (not saved)
int kochPracticeTotal = 0;

// ============================================
// Preferences Management
// ============================================

void loadKochProgress() {
  Preferences prefs;
  prefs.begin("koch", true);  // Read-only
  kochProgress.currentLesson = prefs.getInt("lesson", 1);
  kochProgress.wpm = prefs.getInt("wpm", KOCH_DEFAULT_WPM);
  kochProgress.groupLength = prefs.getInt("length", KOCH_DEFAULT_GROUP_LENGTH);
  kochProgress.sessionCorrect = prefs.getInt("correct", 0);
  kochProgress.sessionTotal = prefs.getInt("total", 0);
  prefs.end();

  // Validate loaded values
  if (kochProgress.currentLesson < 1) kochProgress.currentLesson = 1;
  if (kochProgress.currentLesson > KOCH_TOTAL_LESSONS) kochProgress.currentLesson = KOCH_TOTAL_LESSONS;
  if (kochProgress.wpm < KOCH_MIN_WPM) kochProgress.wpm = KOCH_MIN_WPM;
  if (kochProgress.wpm > KOCH_MAX_WPM) kochProgress.wpm = KOCH_MAX_WPM;
  if (kochProgress.groupLength < KOCH_MIN_GROUP_LENGTH) kochProgress.groupLength = KOCH_MIN_GROUP_LENGTH;
  if (kochProgress.groupLength > KOCH_MAX_GROUP_LENGTH) kochProgress.groupLength = KOCH_MAX_GROUP_LENGTH;

  Serial.print("Koch Method - Loaded progress: Lesson ");
  Serial.print(kochProgress.currentLesson);
  Serial.print(", WPM ");
  Serial.print(kochProgress.wpm);
  Serial.print(", Group Length ");
  Serial.println(kochProgress.groupLength);
}

void saveKochProgress() {
  Preferences prefs;
  prefs.begin("koch", false);  // Read-write
  prefs.putInt("lesson", kochProgress.currentLesson);
  prefs.putInt("wpm", kochProgress.wpm);
  prefs.putInt("length", kochProgress.groupLength);
  prefs.putInt("correct", kochProgress.sessionCorrect);
  prefs.putInt("total", kochProgress.sessionTotal);
  prefs.end();

  Serial.print("Koch Method - Saved progress: Lesson ");
  Serial.print(kochProgress.currentLesson);
  Serial.print(", Session ");
  Serial.print(kochProgress.sessionCorrect);
  Serial.print("/");
  Serial.println(kochProgress.sessionTotal);
}

// ============================================
// Character Set Management
// ============================================

// Get the character set for current lesson
String getKochCharacterSet() {
  String charSet = "";
  for (int i = 0; i < kochProgress.currentLesson && i < KOCH_TOTAL_LESSONS; i++) {
    charSet += KOCH_SEQUENCE[i];
  }
  return charSet;
}

// Generate random character group from current lesson's character set
String generateKochGroup() {
  String charSet;

  // Use practice chars if in practice mode, otherwise use full lesson set
  if (kochCurrentMode == KOCH_MODE_PRACTICE && kochPracticeChars.length() > 0) {
    charSet = kochPracticeChars;
  } else {
    charSet = getKochCharacterSet();
  }

  String group = "";

  // Generate random characters from available set
  for (int i = 0; i < kochProgress.groupLength; i++) {
    int idx = random(0, charSet.length());
    group += charSet[idx];
  }

  return group;
}

// ============================================
// Accuracy and Progression
// ============================================

// Calculate current session accuracy percentage
int getSessionAccuracy() {
  if (kochCurrentMode == KOCH_MODE_PRACTICE) {
    if (kochPracticeTotal == 0) return 0;
    return (kochPracticeCorrect * 100) / kochPracticeTotal;
  } else {
    if (kochProgress.sessionTotal == 0) return 0;
    return (kochProgress.sessionCorrect * 100) / kochProgress.sessionTotal;
  }
}

// Get current total attempts
int getCurrentTotal() {
  return (kochCurrentMode == KOCH_MODE_PRACTICE) ? kochPracticeTotal : kochProgress.sessionTotal;
}

// Get current correct count
int getCurrentCorrect() {
  return (kochCurrentMode == KOCH_MODE_PRACTICE) ? kochPracticeCorrect : kochProgress.sessionCorrect;
}

// Check if ready to advance to next lesson (only in test mode)
bool canAdvanceLesson() {
  if (kochCurrentMode == KOCH_MODE_PRACTICE) return false;  // Can't advance in practice mode
  if (kochProgress.sessionTotal < KOCH_MIN_ATTEMPTS) return false;
  if (kochProgress.currentLesson >= KOCH_TOTAL_LESSONS) return false;
  return getSessionAccuracy() >= KOCH_ACCURACY_THRESHOLD;
}

// Advance to next lesson
void advanceLesson() {
  if (kochProgress.currentLesson < KOCH_TOTAL_LESSONS) {
    kochProgress.currentLesson++;
    kochProgress.sessionCorrect = 0;
    kochProgress.sessionTotal = 0;
    saveKochProgress();
    beep(TONE_SUCCESS, BEEP_LONG);
    Serial.print("Koch Method - Advanced to lesson ");
    Serial.println(kochProgress.currentLesson);
  }
}

// Go back to previous lesson
void regressLesson() {
  if (kochProgress.currentLesson > 1) {
    kochProgress.currentLesson--;
    kochProgress.sessionCorrect = 0;
    kochProgress.sessionTotal = 0;
    saveKochProgress();
    beep(TONE_MENU_NAV, BEEP_SHORT);
    Serial.print("Koch Method - Regressed to lesson ");
    Serial.println(kochProgress.currentLesson);
  }
}

// Reset all progress
void resetKochProgress() {
  kochProgress.currentLesson = 1;
  kochProgress.sessionCorrect = 0;
  kochProgress.sessionTotal = 0;
  saveKochProgress();
  beep(TONE_ERROR, BEEP_LONG);
  delay(100);
  beep(TONE_ERROR, BEEP_LONG);
  Serial.println("Koch Method - Progress reset to lesson 1");
}

// ============================================
// Training Flow
// ============================================

// Start new group challenge
void startNewKochGroup() {
  kochCurrentGroup = generateKochGroup();
  kochUserInput = "";
  kochWaitingForInput = false;
  kochShowingFeedback = false;

  Serial.print("Koch Method - New group: ");
  Serial.print(kochCurrentGroup);
  Serial.print(" (");
  Serial.print(getKochCharacterSet());
  Serial.println(")");
}

// Play current group
void playKochGroup() {
  kochWaitingForInput = false;
  Serial.print("Playing: ");
  Serial.println(kochCurrentGroup);
  playMorseString(kochCurrentGroup.c_str(), kochProgress.wpm);
  kochWaitingForInput = true;
}

// Check user's answer
void checkKochAnswer(Adafruit_ST7789& tft) {
  kochUserInput.toUpperCase();
  kochCorrectAnswer = kochUserInput.equals(kochCurrentGroup);

  // Update statistics based on current mode
  if (kochCurrentMode == KOCH_MODE_PRACTICE) {
    // Practice mode - track locally, don't save
    kochPracticeTotal++;
    if (kochCorrectAnswer) {
      kochPracticeCorrect++;
    }
  } else {
    // Test mode - track in progress, save to preferences
    kochProgress.sessionTotal++;
    if (kochCorrectAnswer) {
      kochProgress.sessionCorrect++;
    }
    saveKochProgress();  // Save after each test attempt
  }

  // Show feedback
  kochShowingFeedback = true;
  kochWaitingForInput = false;

  // Audio feedback
  if (kochCorrectAnswer) {
    beep(TONE_SUCCESS, BEEP_MEDIUM);
    Serial.println("CORRECT!");
  } else {
    beep(TONE_ERROR, BEEP_MEDIUM);
    Serial.print("INCORRECT - Expected: ");
    Serial.print(kochCurrentGroup);
    Serial.print(", Got: ");
    Serial.println(kochUserInput);
  }

  // Check if ready to advance (test mode only)
  if (canAdvanceLesson()) {
    Serial.println("*** Ready to advance to next lesson! Press '+' to continue ***");
  }

  Serial.print(kochCurrentMode == KOCH_MODE_PRACTICE ? "Practice: " : "Test: ");
  Serial.print(getCurrentCorrect());
  Serial.print("/");
  Serial.print(getCurrentTotal());
  Serial.print(" (");
  Serial.print(getSessionAccuracy());
  Serial.println("%)");
}

#endif // TRAINING_KOCH_CORE_H
