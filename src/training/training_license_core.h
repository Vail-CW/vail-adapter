/*
 * Ham Radio License Study - Core Logic
 * Progress tracking, adaptive selection algorithm, Preferences storage
 */

#ifndef TRAINING_LICENSE_CORE_H
#define TRAINING_LICENSE_CORE_H

#include <Preferences.h>
#include "../core/config.h"
#include "training_license_data.h"

// ============================================
// Progress Tracking Structure
// ============================================

struct QuestionProgress {
  uint8_t correct;      // Correct answers (0-255, mastery = 5+)
  uint8_t incorrect;    // Incorrect answers (0-255)
  uint8_t aptitude;     // Pre-calculated aptitude % (0-100)
};

// ============================================
// Session State Structure
// ============================================

struct LicenseStudySession {
  int selectedLicense;           // 0=Tech, 1=Gen, 2=Extra
  int currentQuestionIndex;      // Index into active pool
  int sessionCorrect;            // Correct answers this session
  int sessionTotal;              // Total attempts this session
  bool showingFeedback;          // Currently showing answer feedback
  bool correctAnswer;            // Last answer was correct
  int selectedAnswerIndex;       // Currently highlighted answer (0-3, -1=none)
  unsigned long sessionStartTime; // Session start timestamp
  int lastIncorrectIndex;        // Last incorrect question index (-1 = none)
  int boostDecayQuestions;       // Boost countdown timer (0-12)
};

// Global session state
LicenseStudySession licenseSession = {
  0,     // selectedLicense
  0,     // currentQuestionIndex
  0,     // sessionCorrect
  0,     // sessionTotal
  false, // showingFeedback
  false, // correctAnswer
  0,     // selectedAnswerIndex
  0,     // sessionStartTime
  -1,    // lastIncorrectIndex
  0      // boostDecayQuestions
};

// ============================================
// Progress Calculation Functions
// ============================================

/**
 * Calculate aptitude percentage for a question (0-100%)
 * Mastery threshold: 5 correct answers
 */
int calculateAptitude(const QuestionProgress* qp) {
  if (qp->correct == 0 && qp->incorrect == 0) {
    return 0;  // Never attempted
  }

  // Formula: (correct / 5) * 100, capped at 100%
  int aptitude = (qp->correct * 100) / 5;
  if (aptitude > 100) aptitude = 100;

  return aptitude;
}

/**
 * Update question progress after answer
 * Correct: +1 to correct count
 * Incorrect: -2 from correct count (min 0), +1 to incorrect count
 */
void updateQuestionProgress(QuestionProgress* qp, bool correct) {
  if (correct) {
    // Correct answer
    if (qp->correct < 255) qp->correct++;
  } else {
    // Incorrect answer: penalty of -2 correct (minimum 0)
    if (qp->correct >= 2) {
      qp->correct -= 2;
    } else {
      qp->correct = 0;
    }
    if (qp->incorrect < 255) qp->incorrect++;
  }

  // Recalculate aptitude
  qp->aptitude = calculateAptitude(qp);
}

// ============================================
// Adaptive Selection Algorithm
// ============================================

// Priority tiers for weighted random selection
enum QuestionPriority {
  PRIORITY_MASTERED = 0,        // Aptitude 100% (1% weight)
  PRIORITY_PARTIAL = 1,         // Aptitude 80-99% (4% weight)
  PRIORITY_IMPROVING = 2,       // Aptitude 40-79% (15% weight)
  PRIORITY_INCORRECT = 3,       // Aptitude < 40% (30% weight)
  PRIORITY_NEVER_SEEN = 4       // Never attempted (50% weight)
};

/**
 * Get priority tier for a question based on aptitude
 */
int getQuestionPriority(const QuestionProgress* qp) {
  int apt = qp->aptitude;

  if (apt == 0 && qp->correct == 0 && qp->incorrect == 0) {
    return PRIORITY_NEVER_SEEN;
  } else if (apt < 40) {
    return PRIORITY_INCORRECT;
  } else if (apt < 80) {
    return PRIORITY_IMPROVING;
  } else if (apt < 100) {
    return PRIORITY_PARTIAL;
  } else {
    return PRIORITY_MASTERED;
  }
}

/**
 * Select next question using weighted random selection
 * Questions with lower aptitude have higher probability of being selected
 */
int selectNextQuestion(QuestionPool* pool) {
  if (!pool || !pool->progress || pool->totalQuestions == 0) {
    return 0;
  }

  // Check for active boost (80% chance if active and not same question)
  if (licenseSession.boostDecayQuestions > 0 &&
      licenseSession.lastIncorrectIndex >= 0 &&
      licenseSession.lastIncorrectIndex != licenseSession.currentQuestionIndex) {
    if (random(100) < 80) {
      Serial.println("Boost: Re-asking recently missed question");
      return licenseSession.lastIncorrectIndex;
    }
  }

  // Count questions in each priority tier
  int tierCounts[5] = {0};
  for (int i = 0; i < pool->totalQuestions; i++) {
    int priority = getQuestionPriority(&pool->progress[i]);
    tierCounts[priority]++;
  }

  // Weights for each tier (higher priority = more weight)
  // Inverted: mastered has lowest weight, never seen has highest
  int weights[5] = {1, 4, 15, 30, 50};

  // Calculate total weighted questions
  int totalWeight = 0;
  for (int i = 0; i < 5; i++) {
    totalWeight += tierCounts[i] * weights[i];
  }

  if (totalWeight == 0) {
    // All questions mastered or empty pool - random selection
    return random(0, pool->totalQuestions);
  }

  // Select random weighted index
  int randomWeight = random(0, totalWeight);
  int cumulativeWeight = 0;

  // Find question in selected tier (iterate from highest priority to lowest)
  for (int priority = 4; priority >= 0; priority--) {
    int tierWeight = tierCounts[priority] * weights[priority];

    if (randomWeight < cumulativeWeight + tierWeight) {
      // Found target tier - select random question from this tier
      int targetTierIndex = (randomWeight - cumulativeWeight) / weights[priority];
      int currentTierIndex = 0;

      for (int i = 0; i < pool->totalQuestions; i++) {
        if (getQuestionPriority(&pool->progress[i]) == priority) {
          if (currentTierIndex == targetTierIndex) {
            return i;
          }
          currentTierIndex++;
        }
      }
    }

    cumulativeWeight += tierWeight;
  }

  // Fallback (should never reach here)
  return random(0, pool->totalQuestions);
}

// ============================================
// ESP32 Preferences Storage
// ============================================

/**
 * Load progress from Preferences for a license type
 */
void loadLicenseProgress(int licenseType) {
  Preferences prefs;
  const char* namespace_name = nullptr;
  QuestionPool* pool = getQuestionPool(licenseType);

  if (!pool) return;

  switch (licenseType) {
    case 0: namespace_name = "lic_tech"; break;
    case 1: namespace_name = "lic_gen"; break;
    case 2: namespace_name = "lic_extra"; break;
    default: return;
  }

  prefs.begin(namespace_name, true);  // Read-only

  // Allocate progress array
  size_t progressSize = pool->totalQuestions * sizeof(QuestionProgress);
  pool->progress = (QuestionProgress*)malloc(progressSize);

  if (!pool->progress) {
    Serial.println("ERROR: Failed to allocate progress array");
    prefs.end();
    return;
  }

  // Initialize to zeros (never attempted)
  memset(pool->progress, 0, progressSize);

  // Load binary blob if exists
  size_t blobSize = prefs.getBytesLength("progress");
  if (blobSize == progressSize) {
    prefs.getBytes("progress", pool->progress, progressSize);
    Serial.print("Loaded progress for ");
    Serial.print(pool->totalQuestions);
    Serial.println(" questions");
  } else if (blobSize > 0) {
    Serial.print("WARNING: Progress blob size mismatch (expected ");
    Serial.print(progressSize);
    Serial.print(", got ");
    Serial.print(blobSize);
    Serial.println(")");
    // Keep zeros - start fresh
  } else {
    Serial.println("No previous progress found, starting fresh");
  }

  // Load session stats
  licenseSession.sessionCorrect = prefs.getInt("s_correct", 0);
  licenseSession.sessionTotal = prefs.getInt("s_total", 0);

  prefs.end();

  Serial.print("Session stats: ");
  Serial.print(licenseSession.sessionCorrect);
  Serial.print("/");
  Serial.println(licenseSession.sessionTotal);
}

/**
 * Save progress to Preferences for a license type
 */
void saveLicenseProgress(int licenseType) {
  Preferences prefs;
  const char* namespace_name = nullptr;
  QuestionPool* pool = getQuestionPool(licenseType);

  if (!pool || !pool->progress) {
    Serial.println("ERROR: No progress to save");
    return;
  }

  switch (licenseType) {
    case 0: namespace_name = "lic_tech"; break;
    case 1: namespace_name = "lic_gen"; break;
    case 2: namespace_name = "lic_extra"; break;
    default: return;
  }

  prefs.begin(namespace_name, false);  // Read-write

  // Save question count (needed for stats-only loading)
  prefs.putInt("q_count", pool->totalQuestions);

  // Save binary blob
  size_t progressSize = pool->totalQuestions * sizeof(QuestionProgress);
  prefs.putBytes("progress", pool->progress, progressSize);

  // Save session stats
  prefs.putInt("s_correct", licenseSession.sessionCorrect);
  prefs.putInt("s_total", licenseSession.sessionTotal);

  prefs.end();

  Serial.println("License progress saved");
}

/**
 * Free progress array memory
 */
void unloadLicenseProgress(QuestionPool* pool) {
  if (pool && pool->progress) {
    free(pool->progress);
    pool->progress = nullptr;
  }
}

// ============================================
// Session Management
// ============================================

/**
 * Start a new quiz session for selected license
 */
void startLicenseSession(int licenseType) {
  licenseSession.selectedLicense = licenseType;
  licenseSession.sessionCorrect = 0;
  licenseSession.sessionTotal = 0;
  licenseSession.showingFeedback = false;
  licenseSession.correctAnswer = false;
  licenseSession.selectedAnswerIndex = 0;
  licenseSession.sessionStartTime = millis();
  licenseSession.lastIncorrectIndex = -1;
  licenseSession.boostDecayQuestions = 0;

  // Select first question
  if (activePool && activePool->progress) {
    licenseSession.currentQuestionIndex = selectNextQuestion(activePool);
  } else {
    licenseSession.currentQuestionIndex = 0;
  }
}

/**
 * Submit answer and update progress
 */
void submitAnswer(int answerIndex) {
  if (!activePool || licenseSession.currentQuestionIndex >= activePool->totalQuestions) {
    return;
  }

  LicenseQuestion* q = &activePool->questions[licenseSession.currentQuestionIndex];
  bool correct = (answerIndex == q->correctAnswer);

  // Update session stats
  licenseSession.sessionTotal++;
  if (correct) {
    licenseSession.sessionCorrect++;
  }

  // Update question progress
  QuestionProgress* qp = &activePool->progress[licenseSession.currentQuestionIndex];
  updateQuestionProgress(qp, correct);

  // Set boost if answer is incorrect
  if (!correct) {
    licenseSession.lastIncorrectIndex = licenseSession.currentQuestionIndex;
    licenseSession.boostDecayQuestions = 12;
    Serial.print("Boost activated for question ");
    Serial.println(licenseSession.currentQuestionIndex);
  }

  // Show feedback
  licenseSession.showingFeedback = true;
  licenseSession.correctAnswer = correct;

  Serial.print("Answer: ");
  Serial.print(correct ? "CORRECT" : "INCORRECT");
  Serial.print(" (");
  Serial.print(licenseSession.sessionCorrect);
  Serial.print("/");
  Serial.print(licenseSession.sessionTotal);
  Serial.print(", aptitude: ");
  Serial.print(qp->aptitude);
  Serial.println("%)");
}

/**
 * Advance to next question (call after showing feedback)
 */
void advanceToNextQuestion() {
  // Save progress
  saveLicenseProgress(licenseSession.selectedLicense);

  // Decay boost
  if (licenseSession.boostDecayQuestions > 0) {
    licenseSession.boostDecayQuestions--;
    if (licenseSession.boostDecayQuestions == 0) {
      licenseSession.lastIncorrectIndex = -1;
      Serial.println("Boost expired");
    }
  }

  // Select next question using adaptive algorithm
  licenseSession.currentQuestionIndex = selectNextQuestion(activePool);
  licenseSession.showingFeedback = false;
  licenseSession.selectedAnswerIndex = 0;
}

#endif // TRAINING_LICENSE_CORE_H
