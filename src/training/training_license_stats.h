/*
 * Ham Radio License Study - Statistics Calculations
 * Computes pool coverage, aptitude percentages, mastery counts
 */

#ifndef TRAINING_LICENSE_STATS_H
#define TRAINING_LICENSE_STATS_H

#include "training_license_core.h"
#include "training_license_data.h"

// ============================================
// Statistics Structure
// ============================================

struct LicenseStatistics {
  int totalQuestions;           // Total questions in pool
  int questionsAttempted;       // Questions seen at least once
  int questionsMastered;        // Questions with 5+ correct (100% aptitude)
  int questionsImproving;       // Questions with 1-4 correct
  int questionsWeak;            // Questions with < 40% aptitude
  int questionsNeverSeen;       // Questions never attempted
  float averageAptitude;        // Average aptitude % across attempted questions
  float poolCoverage;           // % of questions attempted (0-100)
};

// Statistics for each license type
LicenseStatistics techStats = {0};
LicenseStatistics genStats = {0};
LicenseStatistics extraStats = {0};

// ============================================
// Statistics Calculation Functions
// ============================================

/**
 * Calculate statistics for a question pool
 */
void calculateStatistics(QuestionPool* pool, LicenseStatistics* stats) {
  if (!pool || !pool->progress) {
    memset(stats, 0, sizeof(LicenseStatistics));
    return;
  }

  stats->totalQuestions = pool->totalQuestions;
  stats->questionsAttempted = 0;
  stats->questionsMastered = 0;
  stats->questionsImproving = 0;
  stats->questionsWeak = 0;
  stats->questionsNeverSeen = 0;

  int totalAptitude = 0;

  for (int i = 0; i < pool->totalQuestions; i++) {
    QuestionProgress* qp = &pool->progress[i];
    int apt = qp->aptitude;

    // Check if attempted
    if (qp->correct > 0 || qp->incorrect > 0) {
      stats->questionsAttempted++;
      totalAptitude += apt;

      // Count mastered (5+ correct, 100% aptitude)
      if (qp->correct >= 5) {
        stats->questionsMastered++;
      }
      // Count improving (1-4 correct)
      else if (qp->correct >= 1) {
        stats->questionsImproving++;
      }

      // Count weak (< 40% aptitude)
      if (apt > 0 && apt < 40) {
        stats->questionsWeak++;
      }
    } else {
      // Never attempted
      stats->questionsNeverSeen++;
    }
  }

  // Calculate averages
  if (stats->questionsAttempted > 0) {
    stats->averageAptitude = (float)totalAptitude / stats->questionsAttempted;
  } else {
    stats->averageAptitude = 0.0;
  }

  stats->poolCoverage = ((float)stats->questionsAttempted / stats->totalQuestions) * 100.0;
}

/**
 * Get statistics for a specific license type
 */
LicenseStatistics* getStatistics(int licenseType) {
  switch (licenseType) {
    case 0: return &techStats;
    case 1: return &genStats;
    case 2: return &extraStats;
    default: return nullptr;
  }
}

/**
 * Update statistics for currently active pool
 */
void updateCurrentStatistics() {
  if (!activePool) return;

  LicenseStatistics* stats = getStatistics(licenseSession.selectedLicense);
  if (stats) {
    calculateStatistics(activePool, stats);
  }
}

/**
 * Get session accuracy percentage
 */
float getSessionAccuracy() {
  if (licenseSession.sessionTotal == 0) return 0.0;
  return ((float)licenseSession.sessionCorrect / licenseSession.sessionTotal) * 100.0;
}

/**
 * Get session duration in minutes
 */
int getSessionDuration() {
  if (licenseSession.sessionStartTime == 0) return 0;
  return (millis() - licenseSession.sessionStartTime) / 60000;
}

// ============================================
// Stats-Only Loading (without full pool)
// ============================================

// Extended statistics with session data
struct LicenseStatsWithSession {
  LicenseStatistics stats;
  int sessionCorrect;
  int sessionTotal;
  bool hasData;  // true if any progress data exists
};

/**
 * Load statistics from Preferences without loading full question pool
 * This allows showing stats for all license types without SD card access
 *
 * @param licenseType 0=Technician, 1=General, 2=Extra
 * @return Statistics with session data, hasData=false if no saved progress
 */
LicenseStatsWithSession loadStatsOnly(int licenseType) {
  LicenseStatsWithSession result;
  memset(&result, 0, sizeof(result));
  result.hasData = false;

  Preferences prefs;
  const char* namespace_name = nullptr;

  switch (licenseType) {
    case 0: namespace_name = "lic_tech"; break;
    case 1: namespace_name = "lic_gen"; break;
    case 2: namespace_name = "lic_extra"; break;
    default: return result;
  }

  prefs.begin(namespace_name, true);  // Read-only

  // Get question count (saved alongside progress)
  int qCount = prefs.getInt("q_count", 0);
  if (qCount <= 0) {
    prefs.end();
    return result;  // No data saved yet
  }

  // Get progress blob size
  size_t expectedSize = qCount * sizeof(QuestionProgress);
  size_t blobSize = prefs.getBytesLength("progress");

  if (blobSize != expectedSize || blobSize == 0) {
    prefs.end();
    return result;  // Size mismatch or no data
  }

  // Allocate temporary progress array
  QuestionProgress* tempProgress = (QuestionProgress*)malloc(blobSize);
  if (!tempProgress) {
    prefs.end();
    return result;  // Memory allocation failed
  }

  // Load progress blob
  prefs.getBytes("progress", tempProgress, blobSize);

  // Load session stats
  result.sessionCorrect = prefs.getInt("s_correct", 0);
  result.sessionTotal = prefs.getInt("s_total", 0);

  prefs.end();

  // Calculate statistics from progress data
  result.stats.totalQuestions = qCount;
  result.stats.questionsAttempted = 0;
  result.stats.questionsMastered = 0;
  result.stats.questionsImproving = 0;
  result.stats.questionsWeak = 0;
  result.stats.questionsNeverSeen = 0;

  int totalAptitude = 0;

  for (int i = 0; i < qCount; i++) {
    QuestionProgress* qp = &tempProgress[i];
    int apt = qp->aptitude;

    if (qp->correct > 0 || qp->incorrect > 0) {
      result.stats.questionsAttempted++;
      totalAptitude += apt;

      if (qp->correct >= 5) {
        result.stats.questionsMastered++;
      } else if (qp->correct >= 1) {
        result.stats.questionsImproving++;
      }

      if (apt > 0 && apt < 40) {
        result.stats.questionsWeak++;
      }
    } else {
      result.stats.questionsNeverSeen++;
    }
  }

  // Free temporary array
  free(tempProgress);

  // Calculate averages
  if (result.stats.questionsAttempted > 0) {
    result.stats.averageAptitude = (float)totalAptitude / result.stats.questionsAttempted;
  } else {
    result.stats.averageAptitude = 0.0;
  }

  result.stats.poolCoverage = ((float)result.stats.questionsAttempted / result.stats.totalQuestions) * 100.0;
  result.hasData = true;

  return result;
}

#endif // TRAINING_LICENSE_STATS_H
