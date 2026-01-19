/*
 * LICW Training Core
 *
 * Core structures, enums, state management, and preferences functions
 * for the Long Island CW Club (LICW) training implementation.
 *
 * LICW Principles:
 * - Characters are learned as SOUNDS, not visual dot/dash patterns
 * - NO morse pattern visuals (.-) anywhere in the UI
 * - Time-To-Recognize (TTR) is the key metric
 * - Instant Flow Recovery (IFR) - skip misses, keep going
 * - Progressive speed: 12/8 -> 12/10 -> 12/12 -> 16/14 -> 20/18 -> 20+
 *
 * Copyright (c) 2025 VAIL SUMMIT Contributors
 */

#ifndef TRAINING_LICW_CORE_H
#define TRAINING_LICW_CORE_H

#include <Arduino.h>
#include <Preferences.h>
#include "../core/config.h"

// ============================================
// LICW Curriculum Carousels
// ============================================

enum LICWCarousel {
    LICW_BC1 = 0,      // Basic CW 1: 18 characters in 6 lessons (12/8 WPM)
    LICW_BC2 = 1,      // Basic CW 2: 26 characters in 9 lessons (12/10 WPM)
    LICW_BC3 = 2,      // Basic CW 3: 5 on-air prep lessons (12/12 WPM)
    LICW_INT1 = 3,     // Intermediate 1: Flow Skills Development (12 WPM)
    LICW_INT2 = 4,     // Intermediate 2: Increasing Effective Speed (16 WPM)
    LICW_INT3 = 5,     // Intermediate 3: Word Discovery (20 WPM)
    LICW_ADV1 = 6,     // Advanced 1: Conversational 20-25 WPM
    LICW_ADV2 = 7,     // Advanced 2: QRQ 25-35 WPM
    LICW_ADV3 = 8      // Advanced 3: Mastery 35-45+ WPM
};
const int LICW_TOTAL_CAROUSELS = 9;

// Carousel names for display
const char* licwCarouselNames[] = {
    "BC1: Basic CW 1",
    "BC2: Basic CW 2",
    "BC3: On-Air Prep",
    "INT1: Flow Skills",
    "INT2: Speed Building",
    "INT3: Word Discovery",
    "ADV1: Conversational",
    "ADV2: QRQ Fluency",
    "ADV3: QRQ Mastery"
};

// Carousel short names for compact display
const char* licwCarouselShortNames[] = {
    "BC1", "BC2", "BC3",
    "INT1", "INT2", "INT3",
    "ADV1", "ADV2", "ADV3"
};

// Carousel descriptions
const char* licwCarouselDescriptions[] = {
    "18 chars, 6 lessons, 12/8 WPM",
    "26 more chars, 9 lessons, 12/10 WPM",
    "QSO Protocol, 5 lessons, 12/12 WPM",
    "Flow development, 12 WPM",
    "Speed increase, 16 WPM",
    "Word discovery, 20 WPM",
    "Head copy, 20-25 WPM",
    "High speed, 25-35 WPM",
    "Mastery, 35-45+ WPM"
};

// Number of lessons per carousel
const int licwLessonCounts[] = {6, 9, 5, 10, 10, 10, 10, 10, 10};

// ============================================
// Practice Types
// ============================================

enum LICWPracticeType {
    LICW_PRACTICE_CSF = 0,           // Character Sound Familiarity (new char intro)
    LICW_PRACTICE_COPY = 1,          // Copy Practice (TTR-tracked receive)
    LICW_PRACTICE_SENDING = 2,       // Sending Practice (key what you see)
    LICW_PRACTICE_IFR = 3,           // Instant Flow Recovery
    LICW_PRACTICE_CFP = 4,           // Character Flow Proficiency
    LICW_PRACTICE_WORD_DISCOVERY = 5, // Word Discovery training
    LICW_PRACTICE_QSO = 6,           // QSO Protocol practice
    LICW_PRACTICE_ADVERSE = 7        // Adverse Copy (noise, QSB, varied fists)
};
const int LICW_TOTAL_PRACTICE_TYPES = 8;

// Practice type names
const char* licwPracticeTypeNames[] = {
    "New Character",
    "Copy Practice",
    "Sending Practice",
    "IFR Training",
    "Character Flow",
    "Word Discovery",
    "QSO Practice",
    "Adverse Copy"
};

// Practice type descriptions
const char* licwPracticeTypeDescriptions[] = {
    "Learn new character sounds",
    "Listen and type with TTR",
    "Key what you see",
    "Skip misses, keep going",
    "Continuous stream copy",
    "Intuitive word recognition",
    "QSO exchange practice",
    "Copy under noise/QSB"
};

// ============================================
// Content Types for Practice
// ============================================

enum LICWContentType {
    LICW_CONTENT_CHARACTERS = 0,     // Individual characters
    LICW_CONTENT_GROUPS = 1,         // Random character groups (2-5 chars)
    LICW_CONTENT_WORDS = 2,          // Common words from lesson vocabulary
    LICW_CONTENT_CALLSIGNS = 3,      // Callsign patterns
    LICW_CONTENT_QSO_EXCHANGE = 4,   // QSO exchange elements
    LICW_CONTENT_PHRASES = 5         // Full phrases/sentences
};

// ============================================
// Lesson Structure
// ============================================

struct LICWLesson {
    int lessonNum;              // Lesson number within carousel (1-based)
    int characterWPM;           // Character speed (e.g., 12, 16, 20)
    int effectiveWPM;           // Effective/Farnsworth speed
    const char* newChars;       // Characters introduced this lesson
    const char* cumulativeChars; // All characters learned through this lesson
    const char* description;    // Lesson description/focus
    const char** words;         // Common words for this lesson (null-terminated)
    const char** phrases;       // Practice phrases (null-terminated)
};

// ============================================
// Carousel Definition
// ============================================

struct LICWCarouselDef {
    LICWCarousel id;
    const char* name;           // Full name
    const char* shortName;      // Short name (e.g., "BC1")
    const char* description;    // Description
    int totalLessons;           // Number of lessons
    int targetCharWPM;          // Target character WPM for this carousel
    int startingFWPM;           // Starting effective WPM
    int endingFWPM;             // Target effective WPM at completion
    const LICWLesson* lessons;  // Array of lessons
};

// ============================================
// TTR (Time-To-Recognize) Structures
// ============================================

// Single TTR measurement
struct TTRMeasurement {
    char character;             // Character that was played
    unsigned long playEndTime;  // When character finished playing (millis)
    unsigned long recognitionTime; // When user pressed key (millis)
    unsigned long ttr;          // Recognition time - playEndTime
    bool correct;               // Was the response correct?
};

// TTR session statistics
struct TTRSession {
    TTRMeasurement measurements[100];  // Recent measurements (circular buffer)
    int measurementCount;              // Number of measurements taken
    int measurementIndex;              // Current index in circular buffer
    unsigned long totalTTR;            // Sum of all TTR values
    unsigned long bestTTR;             // Best (lowest) TTR achieved
    unsigned long worstTTR;            // Worst (highest) TTR achieved
    int correctCount;                  // Number of correct responses
    int totalCount;                    // Total attempts
};

// ============================================
// Progress Tracking Structure
// ============================================

struct LICWProgress {
    // Current position
    LICWCarousel currentCarousel;
    int currentLesson;

    // Current session statistics
    int sessionCorrect;
    int sessionTotal;
    unsigned long sessionTTRTotal;  // Total TTR in milliseconds
    int sessionTTRCount;            // Number of TTR measurements

    // Per-carousel completion (stored separately in preferences)
    // These are loaded/saved individually

    // Per-character metrics (44 total characters)
    // Index mapping: A=0, B=1, ... Z=25, 0=26, 1=27, ... 9=35
    // Punctuation: ?=36, /=37, .=38, ,=39
    // Prosigns: AR=40, SK=41, BT=42, BK=43
    int charCorrect[44];        // Per-character correct count
    int charTotal[44];          // Per-character attempt count
    unsigned long charTTR[44];  // Cumulative TTR per character (ms)
    int charTTRCount[44];       // Number of TTR measurements per character

    // Settings
    int preferredCharWPM;       // User's preferred character WPM
    int preferredFWPM;          // User's preferred Farnsworth WPM
    bool ttrTrackingEnabled;    // Enable/disable TTR tracking
    bool ifrModeEnabled;        // Auto-advance in IFR mode

    // Achievement flags (bitmask)
    uint32_t achievements;
};

// Achievement bit definitions
#define LICW_ACH_BC1_COMPLETE     (1 << 0)
#define LICW_ACH_BC2_COMPLETE     (1 << 1)
#define LICW_ACH_BC3_COMPLETE     (1 << 2)
#define LICW_ACH_INT1_COMPLETE    (1 << 3)
#define LICW_ACH_INT2_COMPLETE    (1 << 4)
#define LICW_ACH_INT3_COMPLETE    (1 << 5)
#define LICW_ACH_ADV1_COMPLETE    (1 << 6)
#define LICW_ACH_ADV2_COMPLETE    (1 << 7)
#define LICW_ACH_ADV3_COMPLETE    (1 << 8)
#define LICW_ACH_FIRST_QSO        (1 << 9)
#define LICW_ACH_TTR_UNDER_500MS  (1 << 10)
#define LICW_ACH_100_CHARS        (1 << 11)
#define LICW_ACH_1000_CHARS       (1 << 12)
#define LICW_ACH_PERFECT_SESSION  (1 << 13)

// ============================================
// Global State Variables
// ============================================

// Current progress (loaded from preferences on init)
LICWProgress licwProgress = {
    .currentCarousel = LICW_BC1,
    .currentLesson = 1,
    .sessionCorrect = 0,
    .sessionTotal = 0,
    .sessionTTRTotal = 0,
    .sessionTTRCount = 0,
    .preferredCharWPM = 12,
    .preferredFWPM = 8,
    .ttrTrackingEnabled = true,
    .ifrModeEnabled = true,
    .achievements = 0
};

// Current TTR session
TTRSession licwTTRSession = {
    .measurementCount = 0,
    .measurementIndex = 0,
    .totalTTR = 0,
    .bestTTR = ULONG_MAX,
    .worstTTR = 0,
    .correctCount = 0,
    .totalCount = 0
};

// Selected carousel, lesson, practice type (for navigation)
LICWCarousel licwSelectedCarousel = LICW_BC1;
int licwSelectedLesson = 1;
LICWPracticeType licwSelectedPracticeType = LICW_PRACTICE_COPY;
LICWContentType licwSelectedContentType = LICW_CONTENT_CHARACTERS;

// Preferences object
Preferences licwPrefs;

// ============================================
// Character Index Mapping
// ============================================

// Get index for a character (0-43)
// Returns -1 if character not in set
int getLICWCharIndex(char c) {
    c = toupper(c);
    if (c >= 'A' && c <= 'Z') return c - 'A';           // 0-25
    if (c >= '0' && c <= '9') return 26 + (c - '0');    // 26-35
    switch (c) {
        case '?': return 36;
        case '/': return 37;
        case '.': return 38;
        case ',': return 39;
        // Prosigns would need special handling
        default: return -1;
    }
}

// Get character from index
char getLICWCharFromIndex(int idx) {
    if (idx < 0 || idx >= 44) return '?';
    if (idx < 26) return 'A' + idx;
    if (idx < 36) return '0' + (idx - 26);
    switch (idx) {
        case 36: return '?';
        case 37: return '/';
        case 38: return '.';
        case 39: return ',';
        default: return '?';
    }
}

// ============================================
// Preferences Functions
// ============================================

// Load LICW progress from preferences
void loadLICWProgress() {
    licwPrefs.begin("licw", true);  // Read-only

    // Current position
    licwProgress.currentCarousel = (LICWCarousel)licwPrefs.getInt("carousel", LICW_BC1);
    licwProgress.currentLesson = licwPrefs.getInt("lesson", 1);

    // Settings
    licwProgress.preferredCharWPM = licwPrefs.getInt("charWPM", 12);
    licwProgress.preferredFWPM = licwPrefs.getInt("fwpm", 8);
    licwProgress.ttrTrackingEnabled = licwPrefs.getBool("ttrOn", true);
    licwProgress.ifrModeEnabled = licwPrefs.getBool("ifrOn", true);

    // Achievements
    licwProgress.achievements = licwPrefs.getUInt("achieve", 0);

    licwPrefs.end();

    // Set selected values to current progress
    licwSelectedCarousel = licwProgress.currentCarousel;
    licwSelectedLesson = licwProgress.currentLesson;

    Serial.printf("[LICW] Progress loaded: Carousel %d, Lesson %d\n",
                  (int)licwProgress.currentCarousel, licwProgress.currentLesson);
}

// Save LICW progress to preferences
void saveLICWProgress() {
    licwPrefs.begin("licw", false);  // Read-write

    // Current position
    licwPrefs.putInt("carousel", (int)licwProgress.currentCarousel);
    licwPrefs.putInt("lesson", licwProgress.currentLesson);

    // Settings
    licwPrefs.putInt("charWPM", licwProgress.preferredCharWPM);
    licwPrefs.putInt("fwpm", licwProgress.preferredFWPM);
    licwPrefs.putBool("ttrOn", licwProgress.ttrTrackingEnabled);
    licwPrefs.putBool("ifrOn", licwProgress.ifrModeEnabled);

    // Achievements
    licwPrefs.putUInt("achieve", licwProgress.achievements);

    licwPrefs.end();

    Serial.printf("[LICW] Progress saved: Carousel %d, Lesson %d\n",
                  (int)licwProgress.currentCarousel, licwProgress.currentLesson);
}

// Load per-character statistics
void loadLICWCharStats() {
    licwPrefs.begin("licwstats", true);  // Read-only

    for (int i = 0; i < 44; i++) {
        char key[12];
        snprintf(key, sizeof(key), "cc%d", i);
        licwProgress.charCorrect[i] = licwPrefs.getInt(key, 0);

        snprintf(key, sizeof(key), "ct%d", i);
        licwProgress.charTotal[i] = licwPrefs.getInt(key, 0);

        snprintf(key, sizeof(key), "ttr%d", i);
        licwProgress.charTTR[i] = licwPrefs.getULong(key, 0);

        snprintf(key, sizeof(key), "ttrc%d", i);
        licwProgress.charTTRCount[i] = licwPrefs.getInt(key, 0);
    }

    licwPrefs.end();
    Serial.println("[LICW] Character statistics loaded");
}

// Save per-character statistics
void saveLICWCharStats() {
    licwPrefs.begin("licwstats", false);  // Read-write

    for (int i = 0; i < 44; i++) {
        char key[12];
        snprintf(key, sizeof(key), "cc%d", i);
        licwPrefs.putInt(key, licwProgress.charCorrect[i]);

        snprintf(key, sizeof(key), "ct%d", i);
        licwPrefs.putInt(key, licwProgress.charTotal[i]);

        snprintf(key, sizeof(key), "ttr%d", i);
        licwPrefs.putULong(key, licwProgress.charTTR[i]);

        snprintf(key, sizeof(key), "ttrc%d", i);
        licwPrefs.putInt(key, licwProgress.charTTRCount[i]);
    }

    licwPrefs.end();
    Serial.println("[LICW] Character statistics saved");
}

// ============================================
// TTR Measurement Functions
// ============================================

// Record a TTR measurement
void recordLICWTTR(char character, unsigned long playEndTime,
                   unsigned long recognitionTime, bool correct) {
    unsigned long ttr = recognitionTime - playEndTime;

    // Add to session
    int idx = licwTTRSession.measurementIndex;
    licwTTRSession.measurements[idx].character = character;
    licwTTRSession.measurements[idx].playEndTime = playEndTime;
    licwTTRSession.measurements[idx].recognitionTime = recognitionTime;
    licwTTRSession.measurements[idx].ttr = ttr;
    licwTTRSession.measurements[idx].correct = correct;

    licwTTRSession.measurementIndex = (idx + 1) % 100;
    if (licwTTRSession.measurementCount < 100) {
        licwTTRSession.measurementCount++;
    }

    // Update session stats
    licwTTRSession.totalTTR += ttr;
    licwTTRSession.totalCount++;
    if (correct) licwTTRSession.correctCount++;
    if (ttr < licwTTRSession.bestTTR) licwTTRSession.bestTTR = ttr;
    if (ttr > licwTTRSession.worstTTR) licwTTRSession.worstTTR = ttr;

    // Update per-character stats
    int charIdx = getLICWCharIndex(character);
    if (charIdx >= 0) {
        licwProgress.charTotal[charIdx]++;
        licwProgress.charTTR[charIdx] += ttr;
        licwProgress.charTTRCount[charIdx]++;
        if (correct) {
            licwProgress.charCorrect[charIdx]++;
        }
    }

    // Update session totals
    licwProgress.sessionTotal++;
    licwProgress.sessionTTRTotal += ttr;
    licwProgress.sessionTTRCount++;
    if (correct) {
        licwProgress.sessionCorrect++;
    }

    Serial.printf("[LICW] TTR recorded: '%c' = %lu ms, correct=%d\n",
                  character, ttr, correct);
}

// Get average TTR for current session
unsigned long getLICWSessionAvgTTR() {
    if (licwTTRSession.totalCount == 0) return 0;
    return licwTTRSession.totalTTR / licwTTRSession.totalCount;
}

// Get average TTR for a specific character
unsigned long getLICWCharAvgTTR(char c) {
    int idx = getLICWCharIndex(c);
    if (idx < 0 || licwProgress.charTTRCount[idx] == 0) return 0;
    return licwProgress.charTTR[idx] / licwProgress.charTTRCount[idx];
}

// Find the weakest character (highest avg TTR)
char getLICWWeakestChar(const char* charSet) {
    char weakest = charSet[0];
    unsigned long highestTTR = 0;

    for (int i = 0; charSet[i] != '\0'; i++) {
        unsigned long avgTTR = getLICWCharAvgTTR(charSet[i]);
        if (avgTTR > highestTTR) {
            highestTTR = avgTTR;
            weakest = charSet[i];
        }
    }

    return weakest;
}

// Find the strongest character (lowest avg TTR)
char getLICWStrongestChar(const char* charSet) {
    char strongest = charSet[0];
    unsigned long lowestTTR = ULONG_MAX;

    for (int i = 0; charSet[i] != '\0'; i++) {
        unsigned long avgTTR = getLICWCharAvgTTR(charSet[i]);
        if (avgTTR > 0 && avgTTR < lowestTTR) {
            lowestTTR = avgTTR;
            strongest = charSet[i];
        }
    }

    return strongest;
}

// Reset session statistics
void resetLICWSession() {
    licwProgress.sessionCorrect = 0;
    licwProgress.sessionTotal = 0;
    licwProgress.sessionTTRTotal = 0;
    licwProgress.sessionTTRCount = 0;

    licwTTRSession.measurementCount = 0;
    licwTTRSession.measurementIndex = 0;
    licwTTRSession.totalTTR = 0;
    licwTTRSession.bestTTR = ULONG_MAX;
    licwTTRSession.worstTTR = 0;
    licwTTRSession.correctCount = 0;
    licwTTRSession.totalCount = 0;

    Serial.println("[LICW] Session reset");
}

// ============================================
// Speed Configuration
// ============================================

// Get character WPM for a carousel
int getLICWCarouselCharWPM(LICWCarousel carousel) {
    switch (carousel) {
        case LICW_BC1:
        case LICW_BC2:
        case LICW_BC3:
        case LICW_INT1:
            return 12;
        case LICW_INT2:
            return 16;
        case LICW_INT3:
        case LICW_ADV1:
            return 20;
        case LICW_ADV2:
            return 30;
        case LICW_ADV3:
            return 40;
        default:
            return 12;
    }
}

// Get effective (Farnsworth) WPM for a carousel
int getLICWCarouselEffectiveWPM(LICWCarousel carousel) {
    switch (carousel) {
        case LICW_BC1:
            return 8;    // 12/8
        case LICW_BC2:
            return 10;   // 12/10
        case LICW_BC3:
        case LICW_INT1:
            return 12;   // 12/12
        case LICW_INT2:
            return 14;   // 16/14
        case LICW_INT3:
        case LICW_ADV1:
            return 18;   // 20/18
        case LICW_ADV2:
            return 28;   // 30/28
        case LICW_ADV3:
            return 38;   // 40/38
        default:
            return 8;
    }
}

// ============================================
// Utility Functions
// ============================================

// Get accuracy percentage for session
int getLICWSessionAccuracy() {
    if (licwProgress.sessionTotal == 0) return 0;
    return (licwProgress.sessionCorrect * 100) / licwProgress.sessionTotal;
}

// Get accuracy percentage for a character
int getLICWCharAccuracy(char c) {
    int idx = getLICWCharIndex(c);
    if (idx < 0 || licwProgress.charTotal[idx] == 0) return 0;
    return (licwProgress.charCorrect[idx] * 100) / licwProgress.charTotal[idx];
}

// Check if a carousel is considered "complete" (>90% accuracy, 50+ attempts)
bool isLICWCarouselComplete(LICWCarousel carousel) {
    return (licwProgress.achievements & (1 << carousel)) != 0;
}

// Mark a carousel as complete
void markLICWCarouselComplete(LICWCarousel carousel) {
    licwProgress.achievements |= (1 << carousel);
    saveLICWProgress();
}

// Format TTR for display (e.g., "0.52s" or "1.2s")
void formatTTR(unsigned long ttrMs, char* buf, size_t bufSize) {
    if (ttrMs < 1000) {
        snprintf(buf, bufSize, "0.%02lus", ttrMs / 10);
    } else {
        snprintf(buf, bufSize, "%lu.%lus", ttrMs / 1000, (ttrMs % 1000) / 100);
    }
}

// Get TTR rating text
const char* getTTRRating(unsigned long ttrMs) {
    if (ttrMs < 300) return "Excellent!";
    if (ttrMs < 500) return "Great!";
    if (ttrMs < 700) return "Good";
    if (ttrMs < 1000) return "OK";
    return "Keep practicing";
}

#endif // TRAINING_LICW_CORE_H
