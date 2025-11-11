/*
 * CW Academy Training - Core Structures and Utilities
 * Shared definitions, enums, and helper functions for all CWA training modules
 */

#ifndef TRAINING_CWA_CORE_H
#define TRAINING_CWA_CORE_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Preferences.h>
#include "../core/config.h"
#include "../audio/i2s_audio.h"
#include "../core/morse_code.h"
#include "../audio/morse_decoder_adaptive.h"
#include "../settings/settings_cw.h"  // For KeyType enum and cwSpeed/cwTone/cwKeyType

// ============================================
// Track and Session Data Structures
// ============================================

// CW Academy Training Tracks
enum CWATrack {
  TRACK_BEGINNER = 0,
  TRACK_FUNDAMENTAL = 1,
  TRACK_INTERMEDIATE = 2,
  TRACK_ADVANCED = 3
};

const char* cwaTrackNames[] = {
  "Beginner",
  "Fundamental",
  "Intermediate",
  "Advanced"
};

const char* cwaTrackDescriptions[] = {
  "Learn CW from zero",
  "Build solid foundation",
  "Increase speed & skill",
  "Master advanced CW"
};

const int CWA_TOTAL_TRACKS = 4;

// Session structure
struct CWASession {
  int sessionNum;           // Session number (1-16)
  int charCount;            // Total characters learned by this session
  const char* newChars;     // New characters introduced in this session
  const char* description;  // Session description
};

// CW Academy Session Progression (Beginner track)
const CWASession cwaSessionData[] = {
  {1,  4,  "AENT",           "Foundation"},
  {2,  9,  "SIO14",          "Numbers Begin"},
  {3,  15, "RHDL25",         "Building Words"},
  {4,  17, "CU",             "Conversations"},
  {5,  22, "MW36?",          "Questions"},
  {6,  25, "FY,",            "Punctuation"},
  {7,  31, "GPQ79/",         "Complete Numbers"},
  {8,  34, "BV<AR>",         "Pro-signs Start"},
  {9,  39, "JK08<BT>",       "Advanced Signs"},
  {10, 44, "XZ.<BK><SK>",    "Complete!"},
  {11, 44, "",               "QSO Practice 1"},
  {12, 44, "",               "QSO Practice 2"},
  {13, 44, "",               "QSO Practice 3"},
  {14, 44, "",               "On-Air Prep 1"},
  {15, 44, "",               "On-Air Prep 2"},
  {16, 44, "",               "On-Air Prep 3"}
};

const int CWA_TOTAL_SESSIONS = 16;

// ============================================
// Practice Types and Message Types
// ============================================

// Practice types
enum CWAPracticeType {
  PRACTICE_COPY = 0,         // Copy practice (receive, keyboard input)
  PRACTICE_SENDING = 1,      // Sending practice (transmit, physical key input)
  PRACTICE_DAILY_DRILL = 2   // Daily drill (warm-up exercise)
};

const char* cwaPracticeTypeNames[] = {
  "Copy Practice",
  "Sending Practice",
  "Daily Drill"
};

const char* cwaPracticeTypeDescriptions[] = {
  "Listen & type",
  "Send with key",
  "Warm-up drills"
};

const int CWA_TOTAL_PRACTICE_TYPES = 3;

// Message types (content types for practice)
enum CWAMessageType {
  MESSAGE_CHARACTERS = 0,
  MESSAGE_WORDS = 1,
  MESSAGE_ABBREVIATIONS = 2,
  MESSAGE_NUMBERS = 3,
  MESSAGE_CALLSIGNS = 4,
  MESSAGE_PHRASES = 5
};

const char* cwaMessageTypeNames[] = {
  "Characters",
  "Words",
  "CW Abbreviations",
  "Numbers",
  "Callsigns",
  "Phrases"
};

const char* cwaMessageTypeDescriptions[] = {
  "Individual letters",
  "Common words",
  "Ham radio terms",
  "Number practice",
  "Call signs",
  "Sentences"
};

const int CWA_TOTAL_MESSAGE_TYPES = 6;

// ============================================
// Session Definitions (Beginner Track)
// ============================================

// Character sets introduced in each session (cumulative)
const char* cwaSessionCharSets[] = {
  "AENT",                    // Session 1
  "AENTSIO14",               // Session 2
  "AENTSIO14RHDL25",         // Session 3
  "AENTSIO14RHDL25CU",       // Session 4
  "AENTSIO14RHDL25CUMW36",   // Session 5
  "AENTSIO14RHDL25CUMW36FY", // Session 6
  "AENTSIO14RHDL25CUMW36FYGPQ79", // Session 7
  "AENTSIO14RHDL25CUMW36FYGPQ79BV", // Session 8
  "AENTSIO14RHDL25CUMW36FYGPQ79BVJK08", // Session 9
  "AENTSIO14RHDL25CUMW36FYGPQ79BVJK08XZ" // Session 10 (all alphanumeric)
};

const char* cwaSessionDescriptions[] = {
  "A E N T",
  "+ S I O 1 4",
  "+ R H D L 2 5",
  "+ C U",
  "+ M W 3 6 ?",
  "+ F Y ,",
  "+ G P Q 7 9 /",
  "+ B V <AR>",
  "+ J K 0 8 <BT>",
  "+ X Z . <BK> <SK>",
  "QSO Practice 1",
  "QSO Practice 2",
  "QSO Practice 3",
  "On-Air Prep 1",
  "On-Air Prep 2",
  "On-Air Prep 3"
};

// ============================================
// CW Academy State
// ============================================

CWATrack cwaSelectedTrack = TRACK_BEGINNER;  // Currently selected track
int cwaSelectedSession = 1;  // Currently selected session (1-16)
CWAPracticeType cwaSelectedPracticeType = PRACTICE_COPY;  // Currently selected practice type
CWAMessageType cwaSelectedMessageType = MESSAGE_CHARACTERS;  // Currently selected message type

// Preferences for saving progress
Preferences cwaPrefs;

// Settings (declared extern from settings_cw.h)
extern int cwSpeed;
extern int cwTone;
// Note: cwKeyType is KeyType enum, not int - see settings_cw.h

// ============================================
// Helper Functions
// ============================================

/*
 * Count items in a null-terminated string array
 */
int countArrayItems(const char** arr) {
  if (arr == nullptr) return 0;
  int count = 0;
  while (arr[count] != nullptr) {
    count++;
  }
  return count;
}

/*
 * Select random items from array and concatenate with spaces
 */
String selectRandomItems(const char** arr, int numItems) {
  if (arr == nullptr || numItems <= 0) return "";

  int arraySize = countArrayItems(arr);
  if (arraySize == 0) return "";

  String result = "";
  for (int i = 0; i < numItems; i++) {
    if (i > 0) result += " ";
    int index = random(arraySize);
    result += arr[index];
  }
  return result;
}

/*
 * Load saved CW Academy progress
 */
void loadCWAProgress() {
  cwaPrefs.begin("cwa", false); // Read-only
  cwaSelectedTrack = (CWATrack)cwaPrefs.getInt("track", TRACK_BEGINNER);
  cwaSelectedSession = cwaPrefs.getInt("session", 1);
  cwaSelectedPracticeType = (CWAPracticeType)cwaPrefs.getInt("practype", PRACTICE_COPY);
  cwaSelectedMessageType = (CWAMessageType)cwaPrefs.getInt("msgtype", MESSAGE_CHARACTERS);
  cwaPrefs.end();
}

/*
 * Save CW Academy progress
 */
void saveCWAProgress() {
  cwaPrefs.begin("cwa", false); // Read-write
  cwaPrefs.putInt("track", (int)cwaSelectedTrack);
  cwaPrefs.putInt("session", cwaSelectedSession);
  cwaPrefs.putInt("practype", (int)cwaSelectedPracticeType);
  cwaPrefs.putInt("msgtype", (int)cwaSelectedMessageType);
  cwaPrefs.end();
}

#endif // TRAINING_CWA_CORE_H
