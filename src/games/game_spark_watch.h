/*
 * Spark Watch - Maritime Morse Code Training Game
 *
 * Players act as coastal wireless station operators, listening to distress
 * calls in morse code and transcribing information to save lives.
 *
 * Based on the Spark Watch web app (Telegraph Relay project)
 */

#ifndef GAME_SPARK_WATCH_H
#define GAME_SPARK_WATCH_H

#include "../core/config.h"
#include "../core/morse_code.h"
#include "../audio/i2s_audio.h"
#include <Preferences.h>

// ============================================
// Game Constants
// ============================================

#define SPARK_MAX_SIGNAL_TYPE    16
#define SPARK_MAX_SHIP_NAME      32
#define SPARK_MAX_DISTRESS       48
#define SPARK_MAX_POSITION       8
#define SPARK_MAX_TRANSMISSION   256
#define SPARK_MAX_NARRATIVE      512

// Speed multiplier options
#define SPARK_SPEED_COUNT        6
static const float SPARK_SPEEDS[SPARK_SPEED_COUNT] = {0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f};

// Base WPM by difficulty (before speed multiplier)
static const int SPARK_BASE_WPM[5] = {10, 12, 15, 18, 22};

// Base points by difficulty
static const int SPARK_BASE_POINTS[5] = {25, 50, 100, 150, 250};

// Penalty amounts
#define SPARK_PENALTY_REFERENCE  5
#define SPARK_PENALTY_HINT       2

// ============================================
// Difficulty Levels
// ============================================

enum SparkWatchDifficulty {
    SPARK_EASY = 0,      // 25 pts: Signal + Ship name only
    SPARK_MEDIUM = 1,    // 50 pts: + Nature of distress
    SPARK_HARD = 2,      // 100 pts: + Position coordinates
    SPARK_EXPERT = 3,    // 150 pts: Faster speeds, complex
    SPARK_MASTER = 4     // 250 pts: Multi-part, highest speeds
};

// Minimum speed index allowed per difficulty
static const int SPARK_MIN_SPEED_INDEX[5] = {0, 2, 3, 4, 5};  // 0.5x, 1.0x, 1.25x, 1.5x, 2.0x

// Difficulty display names
static const char* SPARK_DIFFICULTY_NAMES[5] = {
    "Easy", "Medium", "Hard", "Expert", "Master"
};

// ============================================
// Challenge Definition Structure
// ============================================

struct SparkWatchChallenge {
    const char* id;              // Unique identifier (e.g., "rep_1909_01")
    const char* title;           // Display title
    SparkWatchDifficulty difficulty;

    // Transmission content
    const char* morseTransmission;  // Full morse text to play

    // Expected answers (nullptr if not required for difficulty)
    const char* signalType;      // "CQD", "SOS", etc.
    const char* shipName;        // "REPUBLIC", "TITANIC", etc.
    const char* distressNature;  // "SINKING", "COLLISION", etc. (Medium+)
    const char* latDegrees;      // "41" (Hard+)
    const char* latMinutes;      // "46"
    char latDirection;           // 'N' or 'S'
    const char* lonDegrees;      // "50"
    const char* lonMinutes;      // "14"
    char lonDirection;           // 'E' or 'W'

    // Narrative elements
    const char* briefing;        // Pre-challenge historical context
    const char* debriefing;      // Post-challenge explanation
    const char* hint;            // Optional hint text

    // Campaign association (0 = standalone challenge)
    int campaignId;
    int missionNumber;
};

// ============================================
// Campaign Definition Structure
// ============================================

struct SparkWatchCampaign {
    int id;                      // Campaign ID (1-5)
    const char* name;            // "Through the Fog (1909)"
    const char* ship;            // "RMS Republic"
    const char* description;     // Historical description
    int year;                    // Historical year
    int totalMissions;           // Number of missions
    int unlockRequirement;       // Campaign ID to complete first (0 = unlocked)
};

// ============================================
// Player Progress Structure
// ============================================

struct SparkWatchProgress {
    // Overall stats
    int totalScore;
    int challengesCompleted;
    int perfectChallenges;       // No penalties used

    // Per-difficulty stats
    int completedByDifficulty[5];
    int highScoreByDifficulty[5];

    // Campaign progress
    int campaignProgress[6];     // Missions completed per campaign (index 0 unused)
    bool campaignUnlocked[6];    // Whether campaign is available

    // Settings
    float preferredSpeed;        // Last used speed multiplier
    bool showMorseReference;     // Show reference by default
};

// ============================================
// Session State Structure
// ============================================

struct SparkWatchSession {
    const SparkWatchChallenge* currentChallenge;
    int challengeIndex;          // Index in challenge list
    float currentSpeedMult;
    int speedIndex;              // Index in SPARK_SPEEDS array
    int penaltyPoints;
    bool referenceViewed;
    int hintsUsed;
    bool challengeStarted;       // Has played transmission at least once
    bool challengeCompleted;     // Successfully submitted

    // Form inputs
    char inputSignalType[SPARK_MAX_SIGNAL_TYPE];
    char inputShipName[SPARK_MAX_SHIP_NAME];
    char inputDistressNature[SPARK_MAX_DISTRESS];
    char inputLatDegrees[SPARK_MAX_POSITION];
    char inputLatMinutes[SPARK_MAX_POSITION];
    char inputLatDirection;      // 'N' or 'S'
    char inputLonDegrees[SPARK_MAX_POSITION];
    char inputLonMinutes[SPARK_MAX_POSITION];
    char inputLonDirection;      // 'E' or 'W'

    // Current input field
    int currentField;
    int cursorPosition;

    // Playback state
    bool isPlaying;
    bool isPaused;
    int playbackCharIndex;       // For pause/resume
    int playCount;               // Times played
    unsigned long playStartTime;

    // For campaign mode
    int currentCampaignId;
    int currentMissionNumber;
};

// ============================================
// Input Field Enumeration
// ============================================

enum SparkInputField {
    SPARK_FIELD_SIGNAL_TYPE = 0,
    SPARK_FIELD_SHIP_NAME,
    SPARK_FIELD_DISTRESS_NATURE,
    SPARK_FIELD_LAT_DEGREES,
    SPARK_FIELD_LAT_MINUTES,
    SPARK_FIELD_LAT_DIRECTION,
    SPARK_FIELD_LON_DEGREES,
    SPARK_FIELD_LON_MINUTES,
    SPARK_FIELD_LON_DIRECTION,
    SPARK_FIELD_PLAY_BUTTON,
    SPARK_FIELD_SUBMIT_BUTTON,
    SPARK_FIELD_REFERENCE_BUTTON,
    SPARK_FIELD_HINT_BUTTON,
    SPARK_FIELD_COUNT
};

// ============================================
// Validation Result Structure
// ============================================

struct SparkValidationResult {
    bool signalTypeCorrect;
    bool shipNameCorrect;
    bool distressCorrect;
    bool positionCorrect;
    int correctFieldCount;
    int totalFieldCount;
    bool allCorrect;
};

// ============================================
// Global State Variables
// ============================================

static SparkWatchProgress sparkProgress;
static SparkWatchSession sparkSession;
static Preferences sparkPrefs;

// LVGL mode flag
static bool sparkUseLVGL = true;

// ============================================
// Preferences Functions
// ============================================

void loadSparkWatchProgress() {
    sparkPrefs.begin("sparkwatch", true);  // read-only

    // Overall stats
    sparkProgress.totalScore = sparkPrefs.getInt("score", 0);
    sparkProgress.challengesCompleted = sparkPrefs.getInt("completed", 0);
    sparkProgress.perfectChallenges = sparkPrefs.getInt("perfect", 0);

    // Per-difficulty stats
    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "diff_%d_cnt", i);
        sparkProgress.completedByDifficulty[i] = sparkPrefs.getInt(key, 0);
        snprintf(key, sizeof(key), "diff_%d_hs", i);
        sparkProgress.highScoreByDifficulty[i] = sparkPrefs.getInt(key, 0);
    }

    // Campaign progress
    for (int i = 1; i <= 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "camp_%d_prog", i);
        sparkProgress.campaignProgress[i] = sparkPrefs.getInt(key, 0);
        snprintf(key, sizeof(key), "camp_%d_unlk", i);
        // Campaign 1 always unlocked, others default to false
        sparkProgress.campaignUnlocked[i] = sparkPrefs.getBool(key, i == 1);
    }

    // Settings
    sparkProgress.preferredSpeed = sparkPrefs.getFloat("speed", 1.0f);
    sparkProgress.showMorseReference = sparkPrefs.getBool("showref", false);

    sparkPrefs.end();

    Serial.printf("[SparkWatch] Loaded progress: score=%d, completed=%d\n",
                  sparkProgress.totalScore, sparkProgress.challengesCompleted);
}

void saveSparkWatchProgress() {
    sparkPrefs.begin("sparkwatch", false);  // read-write

    // Overall stats
    sparkPrefs.putInt("score", sparkProgress.totalScore);
    sparkPrefs.putInt("completed", sparkProgress.challengesCompleted);
    sparkPrefs.putInt("perfect", sparkProgress.perfectChallenges);

    // Per-difficulty stats
    for (int i = 0; i < 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "diff_%d_cnt", i);
        sparkPrefs.putInt(key, sparkProgress.completedByDifficulty[i]);
        snprintf(key, sizeof(key), "diff_%d_hs", i);
        sparkPrefs.putInt(key, sparkProgress.highScoreByDifficulty[i]);
    }

    // Campaign progress
    for (int i = 1; i <= 5; i++) {
        char key[16];
        snprintf(key, sizeof(key), "camp_%d_prog", i);
        sparkPrefs.putInt(key, sparkProgress.campaignProgress[i]);
        snprintf(key, sizeof(key), "camp_%d_unlk", i);
        sparkPrefs.putBool(key, sparkProgress.campaignUnlocked[i]);
    }

    // Settings
    sparkPrefs.putFloat("speed", sparkProgress.preferredSpeed);
    sparkPrefs.putBool("showref", sparkProgress.showMorseReference);

    sparkPrefs.end();

    Serial.println("[SparkWatch] Progress saved");
}

void saveSparkWatchSettings() {
    sparkPrefs.begin("sparkwatch", false);
    sparkPrefs.putFloat("speed", sparkProgress.preferredSpeed);
    sparkPrefs.putBool("showref", sparkProgress.showMorseReference);
    sparkPrefs.end();
}

// ============================================
// Session Management
// ============================================

void initSparkWatchSession() {
    memset(&sparkSession, 0, sizeof(SparkWatchSession));
    sparkSession.currentSpeedMult = sparkProgress.preferredSpeed;

    // Find speed index
    for (int i = 0; i < SPARK_SPEED_COUNT; i++) {
        if (abs(SPARK_SPEEDS[i] - sparkSession.currentSpeedMult) < 0.01f) {
            sparkSession.speedIndex = i;
            break;
        }
    }

    // Default directions
    sparkSession.inputLatDirection = 'N';
    sparkSession.inputLonDirection = 'W';

    Serial.println("[SparkWatch] Session initialized");
}

void resetSparkWatchSession() {
    // Clear inputs but keep challenge reference
    const SparkWatchChallenge* challenge = sparkSession.currentChallenge;
    int challengeIdx = sparkSession.challengeIndex;
    int campaignId = sparkSession.currentCampaignId;
    int missionNum = sparkSession.currentMissionNumber;

    memset(&sparkSession, 0, sizeof(SparkWatchSession));

    sparkSession.currentChallenge = challenge;
    sparkSession.challengeIndex = challengeIdx;
    sparkSession.currentCampaignId = campaignId;
    sparkSession.currentMissionNumber = missionNum;
    sparkSession.currentSpeedMult = sparkProgress.preferredSpeed;
    sparkSession.inputLatDirection = 'N';
    sparkSession.inputLonDirection = 'W';

    // Find speed index
    for (int i = 0; i < SPARK_SPEED_COUNT; i++) {
        if (abs(SPARK_SPEEDS[i] - sparkSession.currentSpeedMult) < 0.01f) {
            sparkSession.speedIndex = i;
            break;
        }
    }
}

// Initialize a session with a specific challenge
void initSparkSession(const SparkWatchChallenge* challenge) {
    memset(&sparkSession, 0, sizeof(SparkWatchSession));
    sparkSession.currentChallenge = challenge;
    sparkSession.currentSpeedMult = sparkProgress.preferredSpeed;

    // Find speed index
    for (int i = 0; i < SPARK_SPEED_COUNT; i++) {
        if (abs(SPARK_SPEEDS[i] - sparkSession.currentSpeedMult) < 0.01f) {
            sparkSession.speedIndex = i;
            break;
        }
    }

    // Enforce minimum speed for difficulty
    if (challenge) {
        int minIndex = SPARK_MIN_SPEED_INDEX[challenge->difficulty];
        if (sparkSession.speedIndex < minIndex) {
            sparkSession.speedIndex = minIndex;
            sparkSession.currentSpeedMult = SPARK_SPEEDS[minIndex];
        }

        // Set campaign info if this is a campaign challenge
        sparkSession.currentCampaignId = challenge->campaignId;
        sparkSession.currentMissionNumber = challenge->missionNumber;
    }

    // Default directions
    sparkSession.inputLatDirection = 'N';
    sparkSession.inputLonDirection = 'W';

    Serial.printf("[SparkWatch] Session initialized for: %s\n",
                  challenge ? challenge->title : "none");
}

// ============================================
// Field Navigation
// ============================================

int getVisibleFieldCount(SparkWatchDifficulty difficulty) {
    switch (difficulty) {
        case SPARK_EASY:
            return 2;  // Signal + Ship
        case SPARK_MEDIUM:
            return 3;  // + Nature
        default:
            return 9;  // + Position (6 fields)
    }
}

int getFieldMaxLength(SparkInputField field) {
    switch (field) {
        case SPARK_FIELD_SIGNAL_TYPE:
            return SPARK_MAX_SIGNAL_TYPE - 1;
        case SPARK_FIELD_SHIP_NAME:
            return SPARK_MAX_SHIP_NAME - 1;
        case SPARK_FIELD_DISTRESS_NATURE:
            return SPARK_MAX_DISTRESS - 1;
        case SPARK_FIELD_LAT_DEGREES:
        case SPARK_FIELD_LAT_MINUTES:
        case SPARK_FIELD_LON_DEGREES:
        case SPARK_FIELD_LON_MINUTES:
            return 3;  // Up to 3 digits
        case SPARK_FIELD_LAT_DIRECTION:
        case SPARK_FIELD_LON_DIRECTION:
            return 1;  // Single character
        default:
            return 0;
    }
}

char* getFieldBuffer(SparkInputField field) {
    switch (field) {
        case SPARK_FIELD_SIGNAL_TYPE:
            return sparkSession.inputSignalType;
        case SPARK_FIELD_SHIP_NAME:
            return sparkSession.inputShipName;
        case SPARK_FIELD_DISTRESS_NATURE:
            return sparkSession.inputDistressNature;
        case SPARK_FIELD_LAT_DEGREES:
            return sparkSession.inputLatDegrees;
        case SPARK_FIELD_LAT_MINUTES:
            return sparkSession.inputLatMinutes;
        case SPARK_FIELD_LON_DEGREES:
            return sparkSession.inputLonDegrees;
        case SPARK_FIELD_LON_MINUTES:
            return sparkSession.inputLonMinutes;
        default:
            return nullptr;
    }
}

// ============================================
// Scoring Functions
// ============================================

int calculateSparkScore() {
    if (!sparkSession.currentChallenge) return 0;

    SparkWatchDifficulty diff = sparkSession.currentChallenge->difficulty;
    int basePoints = SPARK_BASE_POINTS[diff];

    // Apply speed multiplier
    int score = (int)(basePoints * sparkSession.currentSpeedMult);

    // Deduct penalties
    score -= sparkSession.penaltyPoints;

    // Minimum score is 0
    if (score < 0) score = 0;

    return score;
}

int getPotentialScore() {
    // Score before penalties
    if (!sparkSession.currentChallenge) return 0;

    SparkWatchDifficulty diff = sparkSession.currentChallenge->difficulty;
    int basePoints = SPARK_BASE_POINTS[diff];
    return (int)(basePoints * sparkSession.currentSpeedMult);
}

// ============================================
// Validation Functions
// ============================================

bool compareStringsIgnoreCase(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (toupper(*a) != toupper(*b)) return false;
        a++;
        b++;
    }
    return *a == *b;  // Both should be at null terminator
}

SparkValidationResult validateSparkAnswers() {
    SparkValidationResult result;
    memset(&result, 0, sizeof(result));

    if (!sparkSession.currentChallenge) {
        return result;
    }

    const SparkWatchChallenge* ch = sparkSession.currentChallenge;

    // Signal type (always required)
    result.signalTypeCorrect = compareStringsIgnoreCase(
        sparkSession.inputSignalType, ch->signalType);
    result.totalFieldCount++;
    if (result.signalTypeCorrect) result.correctFieldCount++;

    // Ship name (always required)
    result.shipNameCorrect = compareStringsIgnoreCase(
        sparkSession.inputShipName, ch->shipName);
    result.totalFieldCount++;
    if (result.shipNameCorrect) result.correctFieldCount++;

    // Distress nature (Medium+)
    if (ch->difficulty >= SPARK_MEDIUM && ch->distressNature) {
        result.distressCorrect = compareStringsIgnoreCase(
            sparkSession.inputDistressNature, ch->distressNature);
        result.totalFieldCount++;
        if (result.distressCorrect) result.correctFieldCount++;
    } else {
        result.distressCorrect = true;  // Not required
    }

    // Position (Hard+)
    if (ch->difficulty >= SPARK_HARD && ch->latDegrees) {
        bool latOk = compareStringsIgnoreCase(sparkSession.inputLatDegrees, ch->latDegrees) &&
                     compareStringsIgnoreCase(sparkSession.inputLatMinutes, ch->latMinutes) &&
                     toupper(sparkSession.inputLatDirection) == ch->latDirection;
        bool lonOk = compareStringsIgnoreCase(sparkSession.inputLonDegrees, ch->lonDegrees) &&
                     compareStringsIgnoreCase(sparkSession.inputLonMinutes, ch->lonMinutes) &&
                     toupper(sparkSession.inputLonDirection) == ch->lonDirection;
        result.positionCorrect = latOk && lonOk;
        result.totalFieldCount++;
        if (result.positionCorrect) result.correctFieldCount++;
    } else {
        result.positionCorrect = true;  // Not required
    }

    // All correct?
    result.allCorrect = result.signalTypeCorrect &&
                        result.shipNameCorrect &&
                        result.distressCorrect &&
                        result.positionCorrect;

    return result;
}

// ============================================
// Penalty Functions
// ============================================

void applyReferencePenalty() {
    if (!sparkSession.referenceViewed) {
        sparkSession.referenceViewed = true;
        sparkSession.penaltyPoints += SPARK_PENALTY_REFERENCE;
        Serial.println("[SparkWatch] Reference penalty applied: -5 pts");
    }
}

void applyHintPenalty() {
    sparkSession.hintsUsed++;
    sparkSession.penaltyPoints += SPARK_PENALTY_HINT;
    Serial.printf("[SparkWatch] Hint penalty applied: -2 pts (total hints: %d)\n",
                  sparkSession.hintsUsed);
}

// ============================================
// Progress Update Functions
// ============================================

void recordChallengeCompletion(int score) {
    if (!sparkSession.currentChallenge) return;

    SparkWatchDifficulty diff = sparkSession.currentChallenge->difficulty;

    // Update overall stats
    sparkProgress.totalScore += score;
    sparkProgress.challengesCompleted++;

    // Perfect challenge (no penalties)?
    if (sparkSession.penaltyPoints == 0) {
        sparkProgress.perfectChallenges++;
    }

    // Update per-difficulty stats
    sparkProgress.completedByDifficulty[diff]++;
    if (score > sparkProgress.highScoreByDifficulty[diff]) {
        sparkProgress.highScoreByDifficulty[diff] = score;
    }

    // Update campaign progress
    if (sparkSession.currentCampaignId > 0) {
        int campId = sparkSession.currentCampaignId;
        int mission = sparkSession.currentMissionNumber;

        // Only increment if this is the next mission in sequence
        if (mission == sparkProgress.campaignProgress[campId] + 1) {
            sparkProgress.campaignProgress[campId] = mission;
            Serial.printf("[SparkWatch] Campaign %d progress: %d missions\n",
                          campId, mission);

            // Check for campaign completion and unlock next
            // (Campaign data will define totalMissions per campaign)
        }
    }

    // Save progress immediately
    saveSparkWatchProgress();

    sparkSession.challengeCompleted = true;

    Serial.printf("[SparkWatch] Challenge completed! Score: %d, Total: %d\n",
                  score, sparkProgress.totalScore);
}

// ============================================
// Speed Control Functions
// ============================================

bool canDecreaseSpeed() {
    // Can always decrease (go slower)
    return sparkSession.speedIndex > 0;
}

bool canIncreaseSpeed() {
    // Can only increase if not yet started the challenge
    // Once started, speed is locked (can only go slower)
    if (sparkSession.challengeStarted) {
        return false;
    }

    // Check max speed for difficulty
    if (!sparkSession.currentChallenge) return false;
    int maxIndex = SPARK_SPEED_COUNT - 1;  // 2.0x

    return sparkSession.speedIndex < maxIndex;
}

void setSparkSpeed(int speedIndex) {
    if (speedIndex < 0 || speedIndex >= SPARK_SPEED_COUNT) return;

    // Check minimum speed for difficulty
    if (sparkSession.currentChallenge) {
        int minIndex = SPARK_MIN_SPEED_INDEX[sparkSession.currentChallenge->difficulty];
        if (speedIndex < minIndex) {
            speedIndex = minIndex;
        }
    }

    sparkSession.speedIndex = speedIndex;
    sparkSession.currentSpeedMult = SPARK_SPEEDS[speedIndex];

    // Save as preferred speed
    sparkProgress.preferredSpeed = sparkSession.currentSpeedMult;
}

// ============================================
// Morse Playback Functions
// ============================================

// Forward declaration - actual implementation may be async
void playSparkTransmission();
void pauseSparkTransmission();
void resumeSparkTransmission();
void stopSparkTransmission();

void playSparkTransmission() {
    if (!sparkSession.currentChallenge) return;
    if (sparkSession.isPlaying && !sparkSession.isPaused) return;

    // Mark challenge as started (locks speed increase)
    sparkSession.challengeStarted = true;
    sparkSession.isPlaying = true;
    sparkSession.isPaused = false;
    sparkSession.playCount++;
    sparkSession.playStartTime = millis();

    // Calculate effective WPM
    SparkWatchDifficulty diff = sparkSession.currentChallenge->difficulty;
    int baseWPM = SPARK_BASE_WPM[diff];
    int effectiveWPM = (int)(baseWPM * sparkSession.currentSpeedMult);

    Serial.printf("[SparkWatch] Playing transmission at %d WPM (base %d x %.2f)\n",
                  effectiveWPM, baseWPM, sparkSession.currentSpeedMult);

    // Play the morse string
    // Note: This is blocking - for async, would need to implement character-by-character
    playMorseString(sparkSession.currentChallenge->morseTransmission, effectiveWPM, cwTone);

    sparkSession.isPlaying = false;
    Serial.println("[SparkWatch] Transmission complete");
}

void stopSparkTransmission() {
    stopTone();
    sparkSession.isPlaying = false;
    sparkSession.isPaused = false;
    sparkSession.playbackCharIndex = 0;
}

// ============================================
// Startup Function
// ============================================

void startSparkWatch() {
    loadSparkWatchProgress();
    initSparkWatchSession();
    Serial.println("[SparkWatch] Game started");
}

#endif // GAME_SPARK_WATCH_H
