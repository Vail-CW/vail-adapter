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

// Standard Koch Method character sequence (40 characters)
// Order: K M R S U A P T L O W I N J E F [space] Y , V G 5 / Q 9 Z H 3 8 B ? 4 2 7 C 1 D 6 0 X
// Note: Space is at position 17 (index 16) - this is intentional in the Koch method
const char KOCH_SEQUENCE[] = "KMRSUAPTLOWINJEF Y,VG5/Q9ZH38B?427C1D60X";
const int KOCH_TOTAL_LESSONS = 40;

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
  int currentLesson;        // Current lesson number (1-40)
  int wpm;                  // Speed setting (15-30 WPM)
  int groupLength;          // Characters per group (3-10)
  int sessionCorrect;       // Correct answers this session
  int sessionTotal;         // Total attempts this session
};

// Practice vs Test mode
enum KochMode {
  KOCH_MODE_TEST,      // Stats tracked, full character set, must pass to advance
  KOCH_MODE_PRACTICE   // Stats NOT tracked, can select specific chars (1-5)
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
bool kochInModeSelectionScreen = false;  // Mode selection screen state
uint8_t kochModeSelection = KOCH_MODE_TEST;  // Default selection (TEST mode)
int kochSettingsSelection = 0;  // 0=WPM, 1=Group Length
int kochHelpPage = 0;  // Help screen page number
unsigned long kochResetHoldStartTime = 0;
bool kochResetHoldActive = false;

// Tutorial/Welcome state (mandatory on first launch)
bool kochInTutorialMode = false;
int kochTutorialStep = 0;  // 0=Welcome, 1=How it works, 2=Controls

// New character introduction state
bool kochShowingNewChar = false;
char kochNewCharacter = ' ';
int kochNewCharPlayCount = 0;  // Play 3 times

// Character grid display state
bool kochShowingGrid = false;

KochMode kochCurrentMode = KOCH_MODE_TEST;  // Default to test mode
String kochPracticeChars = "";  // Selected characters for practice mode (1-5 chars)
int kochPracticeCorrect = 0;    // Practice mode stats (not saved)
int kochPracticeTotal = 0;

// LVGL mode flag - when true, skip legacy draw functions (LVGL handles display)
bool kochUseLVGL = true;  // Default to LVGL mode

// ============================================
// Conversational Messaging System
// ============================================

// Message types for context-aware encouragement
enum KochMessageType {
  MSG_WELCOME,
  MSG_ENCOURAGEMENT,
  MSG_SUCCESS,
  MSG_MILESTONE,
  MSG_NEW_CHARACTER,
  MSG_NEED_HELP,
  MSG_CELEBRATION
};

// Enhanced state tracking for game-like features
int kochCurrentStreak = 0;        // Consecutive correct answers
int kochBestStreak = 0;           // Best streak this session
int kochMilestonesHit = 0;        // Milestone flags (bitmask)
bool kochFirstTimeUser = true;    // Show tutorial on first launch
String kochCurrentMessage = "";   // Active message to display
uint16_t kochMessageColor = ST77XX_WHITE;

// Milestone achievement flags (bitmask)
#define MILESTONE_FIRST_CORRECT     (1 << 0)
#define MILESTONE_5_STREAK          (1 << 1)
#define MILESTONE_10_STREAK         (1 << 2)
#define MILESTONE_10_ATTEMPTS       (1 << 3)
#define MILESTONE_FIRST_90          (1 << 4)
#define MILESTONE_LESSON_COMPLETE   (1 << 5)
#define MILESTONE_HALFWAY           (1 << 6)
#define MILESTONE_COMPLETE_ALL      (1 << 7)

// Get conversational message based on context
String getKochMessage(KochMessageType type, int accuracy = 0, int streak = 0) {
  switch (type) {
    case MSG_WELCOME:
      return "Welcome to Koch Method! Let's learn morse code together!";

    case MSG_ENCOURAGEMENT:
      if (accuracy < 50) {
        return "Keep practicing! Every mistake is a step toward mastery.";
      } else if (accuracy < 70) {
        return "You're doing great! Keep going!";
      } else if (accuracy < 85) {
        return "Nice work! You're at " + String(accuracy) + "% accuracy.";
      } else if (accuracy < 90) {
        return "Excellent! You're almost there!";
      } else {
        return "Amazing! You've hit 90%! Ready to level up?";
      }

    case MSG_SUCCESS:
      return "CORRECT!";

    case MSG_MILESTONE:
      if (streak == 5) {
        return "WOW! 5 in a row! You're on fire!";
      } else if (streak == 10) {
        return "Unstoppable! 10 in a row!";
      } else if (streak >= 20) {
        return "INCREDIBLE! " + String(streak) + " in a row!";
      } else {
        return "Great job!";
      }

    case MSG_NEW_CHARACTER:
      return "Congratulations! You've unlocked a new character!";

    case MSG_NEED_HELP:
      if (accuracy < 50) {
        return "Try slowing down to 15 WPM (Press S)";
      } else {
        return "Need a break? Press P to practice specific characters";
      }

    case MSG_CELEBRATION:
      return "Lesson complete! New character unlocked!";

    default:
      return "";
  }
}

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

  // Load new conversational/game-like fields
  kochFirstTimeUser = prefs.getBool("firstTime", true);
  kochMilestonesHit = prefs.getInt("achievements", 0);
  kochBestStreak = prefs.getInt("bestStreak", 0);

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

  // Save new conversational/game-like fields
  prefs.putBool("firstTime", kochFirstTimeUser);
  prefs.putInt("achievements", kochMilestonesHit);
  prefs.putInt("bestStreak", kochBestStreak);

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
int getKochSessionAccuracy() {
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
  return getKochSessionAccuracy() >= KOCH_ACCURACY_THRESHOLD;
}

// Advance to next lesson
void advanceLesson() {
  if (kochProgress.currentLesson < KOCH_TOTAL_LESSONS) {
    kochProgress.currentLesson++;
    kochProgress.sessionCorrect = 0;
    kochProgress.sessionTotal = 0;

    // Trigger new character introduction
    kochNewCharacter = KOCH_SEQUENCE[kochProgress.currentLesson - 1];
    kochShowingNewChar = true;
    kochNewCharPlayCount = 0;

    // Mark milestone
    kochMilestonesHit |= MILESTONE_LESSON_COMPLETE;

    // Special milestone at halfway point
    if (kochProgress.currentLesson == 22) {
      kochMilestonesHit |= MILESTONE_HALFWAY;
    }

    // Special milestone when completing all lessons
    if (kochProgress.currentLesson == KOCH_TOTAL_LESSONS) {
      kochMilestonesHit |= MILESTONE_COMPLETE_ALL;
    }

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
void checkKochAnswer(LGFX& tft) {
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
  }

  // ============================================
  // Streak Tracking & Milestone Celebrations
  // ============================================

  if (kochCorrectAnswer) {
    // Increment streak
    kochCurrentStreak++;
    if (kochCurrentStreak > kochBestStreak) {
      kochBestStreak = kochCurrentStreak;
    }

    // Check for streak milestones
    if (kochCurrentStreak == 5 && !(kochMilestonesHit & MILESTONE_5_STREAK)) {
      kochMilestonesHit |= MILESTONE_5_STREAK;
      kochCurrentMessage = getKochMessage(MSG_MILESTONE, 0, 5);
      kochMessageColor = ST77XX_YELLOW;
      beep(TONE_SUCCESS, BEEP_LONG);
    } else if (kochCurrentStreak == 10 && !(kochMilestonesHit & MILESTONE_10_STREAK)) {
      kochMilestonesHit |= MILESTONE_10_STREAK;
      kochCurrentMessage = getKochMessage(MSG_MILESTONE, 0, 10);
      kochMessageColor = ST77XX_YELLOW;
      beep(TONE_SUCCESS, BEEP_LONG);
      delay(100);
      beep(TONE_SUCCESS, BEEP_LONG);
    } else {
      kochCurrentMessage = getKochMessage(MSG_SUCCESS);
      kochMessageColor = ST77XX_GREEN;
    }

    // First correct milestone
    if (!(kochMilestonesHit & MILESTONE_FIRST_CORRECT)) {
      kochMilestonesHit |= MILESTONE_FIRST_CORRECT;
      kochCurrentMessage = "Your first correct answer! Many more to come!";
      kochMessageColor = ST77XX_GREEN;
    }
  } else {
    // Reset streak on wrong answer
    kochCurrentStreak = 0;
    kochCurrentMessage = "Almost! The correct answer was " + kochCurrentGroup + ". Let's try another one!";
    kochMessageColor = ST77XX_RED;
  }

  // Check for 10 attempts milestone
  if (getCurrentTotal() == 10 && !(kochMilestonesHit & MILESTONE_10_ATTEMPTS)) {
    kochMilestonesHit |= MILESTONE_10_ATTEMPTS;
    kochCurrentMessage = "10 attempts! You're building muscle memory!";
    kochMessageColor = ST77XX_CYAN;
  }

  // Check for first 90% milestone
  int accuracy = getKochSessionAccuracy();
  if (accuracy >= 90 && getCurrentTotal() >= KOCH_MIN_ATTEMPTS && !(kochMilestonesHit & MILESTONE_FIRST_90)) {
    kochMilestonesHit |= MILESTONE_FIRST_90;
    kochCurrentMessage = "You did it! 90% accuracy achieved!";
    kochMessageColor = ST77XX_YELLOW;
  }

  // ============================================
  // Contextual Help Hints (Test mode only)
  // ============================================

  if (kochCurrentMode == KOCH_MODE_TEST) {
    int total = getCurrentTotal();

    // Hint: Struggling badly (<50% after 10+ attempts)
    if (total >= 10 && accuracy < 50) {
      kochCurrentMessage = "Try slowing down to 15 WPM (Press S)";
      kochMessageColor = ST77XX_CYAN;
    }

    // Hint: Stuck just below threshold (85-89% after 20+ attempts)
    else if (total >= 20 && accuracy >= 85 && accuracy < 90) {
      kochCurrentMessage = "Almost there! Just a few more!";
      kochMessageColor = ST77XX_YELLOW;
    }

    // Hint: Extended struggle (<70% after 20+ attempts)
    else if (total >= 20 && accuracy < 70) {
      kochCurrentMessage = "Need a break? Press P to practice specific characters";
      kochMessageColor = ST77XX_CYAN;
    }
  }

  // Save progress (test mode only)
  if (kochCurrentMode == KOCH_MODE_TEST) {
    saveKochProgress();
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
  Serial.print(accuracy);
  Serial.print("%) Streak: ");
  Serial.println(kochCurrentStreak);
}

// ============================================
// LVGL Practice Session Management
// ============================================

// Initialize a new practice session (called when entering practice screen)
void initKochPracticeSession() {
  kochCurrentGroup = "";
  kochUserInput = "";
  kochWaitingForInput = false;
  kochShowingFeedback = false;
  kochCorrectAnswer = false;
  kochCurrentStreak = 0;

  // Load saved progress
  loadKochProgress();

  Serial.println("[Koch] Practice session initialized");
  Serial.printf("[Koch] Level %d, WPM %d, Group Length %d\n",
                kochProgress.currentLesson, kochProgress.wpm, kochProgress.groupLength);
}

// Get current group for display (LVGL callable)
String getCurrentKochGroup() {
  return kochCurrentGroup;
}

// Get user input for display (LVGL callable)
String getKochUserInput() {
  return kochUserInput;
}

// Set user input (LVGL callable)
void setKochUserInput(const String& input) {
  kochUserInput = input;
}

// Append character to user input (LVGL callable)
void appendKochUserInput(char c) {
  kochUserInput += (char)toupper(c);
}

// Delete last character from user input (LVGL callable)
void deleteKochUserInput() {
  if (kochUserInput.length() > 0) {
    kochUserInput.remove(kochUserInput.length() - 1);
  }
}

// Clear user input (LVGL callable)
void clearKochUserInput() {
  kochUserInput = "";
}

// Check if waiting for input
bool isKochWaitingForInput() {
  return kochWaitingForInput;
}

// Check if showing feedback
bool isKochShowingFeedback() {
  return kochShowingFeedback;
}

// Check if last answer was correct
bool wasKochAnswerCorrect() {
  return kochCorrectAnswer;
}

// Get feedback message
String getKochFeedbackMessage() {
  return kochCurrentMessage;
}

// Get current streak
int getKochCurrentStreak() {
  return kochCurrentStreak;
}

// Get best streak
int getKochBestStreak() {
  return kochBestStreak;
}

// Get milestones bitmask
int getKochMilestones() {
  return kochMilestonesHit;
}

// Generate and start new group (LVGL callable)
void startNewKochGroupLVGL() {
  startNewKochGroup();
}

// Play current group (LVGL callable)
void playKochGroupLVGL() {
  playKochGroup();
}

// Submit answer and check result (LVGL callable)
// Returns true if correct, false if wrong
bool submitKochAnswerLVGL() {
  kochUserInput.toUpperCase();
  kochCorrectAnswer = kochUserInput.equals(kochCurrentGroup);

  // Update statistics based on current mode
  if (kochCurrentMode == KOCH_MODE_PRACTICE) {
    kochPracticeTotal++;
    if (kochCorrectAnswer) {
      kochPracticeCorrect++;
    }
  } else {
    kochProgress.sessionTotal++;
    if (kochCorrectAnswer) {
      kochProgress.sessionCorrect++;
    }
  }

  // Streak tracking
  if (kochCorrectAnswer) {
    kochCurrentStreak++;
    if (kochCurrentStreak > kochBestStreak) {
      kochBestStreak = kochCurrentStreak;
    }
    kochCurrentMessage = "CORRECT!";
  } else {
    kochCurrentStreak = 0;
    kochCurrentMessage = "Wrong: " + kochCurrentGroup;
  }

  // Audio feedback
  if (kochCorrectAnswer) {
    beep(TONE_SUCCESS, BEEP_MEDIUM);
  } else {
    beep(TONE_ERROR, BEEP_MEDIUM);
  }

  // Save progress (test mode only)
  if (kochCurrentMode == KOCH_MODE_TEST) {
    saveKochProgress();
  }

  kochShowingFeedback = true;
  kochWaitingForInput = false;

  return kochCorrectAnswer;
}

// Continue after feedback (LVGL callable)
void continueAfterFeedback() {
  kochShowingFeedback = false;
  kochUserInput = "";
}

// Get progress toward next level (0-100)
int getKochLevelProgress() {
  if (kochProgress.sessionTotal < KOCH_MIN_ATTEMPTS) {
    // Show progress toward minimum attempts
    return (kochProgress.sessionTotal * 100) / KOCH_MIN_ATTEMPTS;
  }
  // Show accuracy progress toward 90%
  int accuracy = getKochSessionAccuracy();
  if (accuracy >= KOCH_ACCURACY_THRESHOLD) {
    return 100;
  }
  return (accuracy * 100) / KOCH_ACCURACY_THRESHOLD;
}

// Format characters learned as spaced string (e.g., "K M R S U")
String getKochCharactersSpaced() {
  String charSet = getKochCharacterSet();
  String spaced = "";
  for (int i = 0; i < charSet.length(); i++) {
    if (i > 0) spaced += " ";
    spaced += charSet[i];
  }
  return spaced;
}

#endif // TRAINING_KOCH_CORE_H
