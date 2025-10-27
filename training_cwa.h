/*
 * CW Academy Training Module
 * Implements the CW Academy Beginner Curriculum with progressive character introduction
 */

#ifndef TRAINING_CWA_H
#define TRAINING_CWA_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Preferences.h>
#include "config.h"
#include "i2s_audio.h"
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

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

struct CWASession {
  int sessionNum;           // Session number (1-16)
  int charCount;            // Total characters learned by this session
  const char* newChars;     // New characters introduced in this session
  const char* description;  // Session description
};

// CW Academy Session Progression
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
// CW Academy State
// ============================================

CWATrack cwaSelectedTrack = TRACK_BEGINNER;  // Currently selected track
int cwaSelectedSession = 1;  // Currently selected session (1-16)
CWAPracticeType cwaSelectedPracticeType = PRACTICE_COPY;  // Currently selected practice type
CWAMessageType cwaSelectedMessageType = MESSAGE_CHARACTERS;  // Currently selected message type

// Preferences for saving progress
Preferences cwaPrefs;

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

/*
 * Draw track selection screen
 */
void drawCWATrackSelectUI(Adafruit_ST7789& tft) {
  // Clear screen (preserve header)
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Modern card container
  int cardX = 20;
  int cardY = 60;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 140;

  tft.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082); // Dark blue fill
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF); // Light blue outline

  // Track indicator at top
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF); // Light gray
  int16_t x1, y1;
  uint16_t w, h;
  String indicator = "Track " + String(cwaSelectedTrack + 1) + " of " + String(CWA_TOTAL_TRACKS);
  tft.getTextBounds(indicator, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 18);
  tft.print(indicator);

  // Track name (large, centered)
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  String trackText = String(cwaTrackNames[cwaSelectedTrack]);
  tft.getTextBounds(trackText, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 60);
  tft.print(trackText);

  // Track description (white for readability)
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  String desc = String(cwaTrackDescriptions[cwaSelectedTrack]);
  tft.getTextBounds(desc, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 95);
  tft.print(desc);

  // Navigation hint
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  String navHint = "16 Sessions";
  tft.getTextBounds(navHint, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 125);
  tft.print(navHint);

  // Navigation arrows
  // Up arrow (if not at first track)
  if (cwaSelectedTrack > TRACK_BEGINNER) {
    tft.fillTriangle(
      SCREEN_WIDTH / 2, cardY - 15,
      SCREEN_WIDTH / 2 - 12, cardY - 5,
      SCREEN_WIDTH / 2 + 12, cardY - 5,
      ST77XX_CYAN
    );
  }

  // Down arrow (if not at last track)
  if (cwaSelectedTrack < TRACK_ADVANCED) {
    tft.fillTriangle(
      SCREEN_WIDTH / 2, cardY + cardH + 15,
      SCREEN_WIDTH / 2 - 12, cardY + cardH + 5,
      SCREEN_WIDTH / 2 + 12, cardY + cardH + 5,
      ST77XX_CYAN
    );
  }

  // Footer with help text
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  String helpText = "\x18\x19 Select  ENTER Continue  ESC Back";
  tft.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, SCREEN_HEIGHT - 12);
  tft.print(helpText);
}

/*
 * Handle input for CW Academy track selection
 * Returns: -1 to exit, 0 for normal input, 2 for redraw needed
 */
int handleCWATrackSelectInput(char key, Adafruit_ST7789& tft) {
  bool needsRedraw = false;

  if (key == KEY_UP) {
    if (cwaSelectedTrack > TRACK_BEGINNER) {
      cwaSelectedTrack = (CWATrack)((int)cwaSelectedTrack - 1);
      needsRedraw = true;
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
  } else if (key == KEY_DOWN) {
    if (cwaSelectedTrack < TRACK_ADVANCED) {
      cwaSelectedTrack = (CWATrack)((int)cwaSelectedTrack + 1);
      needsRedraw = true;
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Save track selection and move to session selection
    saveCWAProgress();
    beep(TONE_SELECT, BEEP_MEDIUM);
    return 1; // Signal to navigate to session selection
  } else if (key == KEY_ESC) {
    // Exit to training menu
    return -1;
  }

  if (needsRedraw) {
    return 2; // Request full redraw
  }

  return 0; // Normal input processed
}

/*
 * Draw session selection screen
 */
void drawCWASessionSelectUI(Adafruit_ST7789& tft) {
  // Clear screen (preserve header)
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Modern card container
  int cardX = 20;
  int cardY = 60;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 140;

  tft.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082); // Dark blue fill
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF); // Light blue outline

  // Get session data
  const CWASession& session = cwaSessionData[cwaSelectedSession - 1];

  // Track name at top
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF); // Light gray
  int16_t x1, y1;
  uint16_t w, h;
  String trackLabel = String(cwaTrackNames[cwaSelectedTrack]) + " Track";
  tft.getTextBounds(trackLabel, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 18);
  tft.print(trackLabel);

  // Session number (large, centered)
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  String sessionText = "Session " + String(cwaSelectedSession);
  tft.getTextBounds(sessionText, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 60);
  tft.print(sessionText);

  // Character count (cyan for visibility)
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  String charInfo = String(session.charCount) + " characters";
  tft.getTextBounds(charInfo, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 90);
  tft.print(charInfo);

  // New characters (white for readability)
  if (strlen(session.newChars) > 0) {
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    String newCharsText = "New: " + String(session.newChars);
    tft.getTextBounds(newCharsText, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(cardX + (cardW - w) / 2, cardY + 115);
    tft.print(newCharsText);
  }

  // Description (light gray, bottom)
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  String desc = session.description;
  tft.getTextBounds(desc, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 132);
  tft.print(desc);

  // Navigation arrows
  // Up arrow (if not at first session)
  if (cwaSelectedSession > 1) {
    tft.fillTriangle(
      SCREEN_WIDTH / 2, cardY - 15,
      SCREEN_WIDTH / 2 - 12, cardY - 5,
      SCREEN_WIDTH / 2 + 12, cardY - 5,
      ST77XX_CYAN
    );
  }

  // Down arrow (if not at last session)
  if (cwaSelectedSession < CWA_TOTAL_SESSIONS) {
    tft.fillTriangle(
      SCREEN_WIDTH / 2, cardY + cardH + 15,
      SCREEN_WIDTH / 2 - 12, cardY + cardH + 5,
      SCREEN_WIDTH / 2 + 12, cardY + cardH + 5,
      ST77XX_CYAN
    );
  }

  // Footer with help text
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  String helpText = "\x18\x19 Select  ENTER Continue  ESC Back";
  tft.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, SCREEN_HEIGHT - 12);
  tft.print(helpText);
}

/*
 * Handle input for CW Academy session selection
 * Returns: -1 to exit to track selection, 0 for normal input, 2 for redraw needed
 */
int handleCWASessionSelectInput(char key, Adafruit_ST7789& tft) {
  bool needsRedraw = false;

  if (key == KEY_UP) {
    if (cwaSelectedSession > 1) {
      cwaSelectedSession--;
      needsRedraw = true;
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
  } else if (key == KEY_DOWN) {
    if (cwaSelectedSession < CWA_TOTAL_SESSIONS) {
      cwaSelectedSession++;
      needsRedraw = true;
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Save progress and move to practice type selection
    saveCWAProgress();
    beep(TONE_SELECT, BEEP_MEDIUM);
    return 1; // Signal to navigate to practice type selection

  } else if (key == KEY_ESC) {
    // Return to track selection
    return -1;
  }

  if (needsRedraw) {
    return 2; // Request full redraw
  }

  return 0; // Normal input processed
}

/*
 * Draw practice type selection screen
 */
void drawCWAPracticeTypeSelectUI(Adafruit_ST7789& tft) {
  // Clear screen (preserve header)
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Check if advanced practice types are locked (Sessions 1-10)
  bool advancedLocked = (cwaSelectedSession <= 10);
  bool currentTypeLocked = advancedLocked && (cwaSelectedPracticeType != PRACTICE_COPY);

  // Modern card container
  int cardX = 20;
  int cardY = 60;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 140;

  tft.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082); // Dark blue fill
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF); // Light blue outline

  // Session context at top
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF); // Light gray
  int16_t x1, y1;
  uint16_t w, h;
  String context = String(cwaTrackNames[cwaSelectedTrack]) + " - Session " + String(cwaSelectedSession);
  tft.getTextBounds(context, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 18);
  tft.print(context);

  // Practice type name (large, centered)
  tft.setTextSize(2);
  tft.setTextColor(currentTypeLocked ? 0x4208 : ST77XX_WHITE); // Dim if locked
  String typeText = String(cwaPracticeTypeNames[cwaSelectedPracticeType]);
  tft.getTextBounds(typeText, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 60);
  tft.print(typeText);

  // Description or lock message
  tft.setTextSize(2);
  if (currentTypeLocked) {
    // Show locked message
    tft.setTextColor(ST77XX_RED);
    String lockMsg = "LOCKED";
    tft.getTextBounds(lockMsg, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(cardX + (cardW - w) / 2, cardY + 85);
    tft.print(lockMsg);

    // Unlock hint
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    String hint = "Unlocks at Session 11";
    tft.getTextBounds(hint, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(cardX + (cardW - w) / 2, cardY + 105);
    tft.print(hint);
  } else {
    tft.setTextColor(ST77XX_CYAN);
    String desc = String(cwaPracticeTypeDescriptions[cwaSelectedPracticeType]);
    tft.getTextBounds(desc, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(cardX + (cardW - w) / 2, cardY + 95);
    tft.print(desc);
  }

  // Navigation hint
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  String hint = String(cwaSelectedPracticeType + 1) + " of " + String(CWA_TOTAL_PRACTICE_TYPES);
  tft.getTextBounds(hint, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 125);
  tft.print(hint);

  // Navigation arrows (always visible)
  if (cwaSelectedPracticeType > PRACTICE_COPY) {
    tft.fillTriangle(
      SCREEN_WIDTH / 2, cardY - 15,
      SCREEN_WIDTH / 2 - 12, cardY - 5,
      SCREEN_WIDTH / 2 + 12, cardY - 5,
      ST77XX_CYAN
    );
  }

  if (cwaSelectedPracticeType < PRACTICE_DAILY_DRILL) {
    tft.fillTriangle(
      SCREEN_WIDTH / 2, cardY + cardH + 15,
      SCREEN_WIDTH / 2 - 12, cardY + cardH + 5,
      SCREEN_WIDTH / 2 + 12, cardY + cardH + 5,
      ST77XX_CYAN
    );
  }

  // Footer
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  String helpText = "\x18\x19 Select  ENTER Continue  ESC Back";
  tft.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, SCREEN_HEIGHT - 12);
  tft.print(helpText);
}

/*
 * Handle input for CW Academy practice type selection
 * Returns: -1 to exit to session selection, 0 for normal input, 1 to navigate to message type, 2 for redraw
 */
int handleCWAPracticeTypeSelectInput(char key, Adafruit_ST7789& tft) {
  bool needsRedraw = false;
  bool advancedLocked = (cwaSelectedSession <= 10);
  bool currentTypeLocked = advancedLocked && (cwaSelectedPracticeType != PRACTICE_COPY);

  if (key == KEY_UP) {
    // Allow navigation through all types
    if (cwaSelectedPracticeType > PRACTICE_COPY) {
      cwaSelectedPracticeType = (CWAPracticeType)((int)cwaSelectedPracticeType - 1);
      needsRedraw = true;
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
  } else if (key == KEY_DOWN) {
    // Allow navigation through all types
    if (cwaSelectedPracticeType < PRACTICE_DAILY_DRILL) {
      cwaSelectedPracticeType = (CWAPracticeType)((int)cwaSelectedPracticeType + 1);
      needsRedraw = true;
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Block ENTER on locked types
    if (currentTypeLocked) {
      // Show error - locked type
      beep(600, 150);

      // Flash locked message
      tft.fillRect(0, 210, SCREEN_WIDTH, 20, COLOR_BACKGROUND);
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_RED);
      int16_t x1, y1;
      uint16_t w, h;
      String msg = "Available at Session 11+";
      tft.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
      tft.setCursor((SCREEN_WIDTH - w) / 2, 215);
      tft.print(msg);
      delay(1500);
      return 2; // Redraw to clear message
    } else {
      // Unlocked - proceed
      saveCWAProgress();
      beep(TONE_SELECT, BEEP_MEDIUM);
      return 1; // Signal to navigate to message type selection
    }
  } else if (key == KEY_ESC) {
    return -1; // Return to session selection
  }

  if (needsRedraw) {
    return 2;
  }

  return 0;
}

/*
 * Draw message type selection screen
 */
void drawCWAMessageTypeSelectUI(Adafruit_ST7789& tft) {
  // Clear screen (preserve header)
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Modern card container
  int cardX = 20;
  int cardY = 60;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 140;

  tft.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082); // Dark blue fill
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF); // Light blue outline

  // Practice type context at top
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF); // Light gray
  int16_t x1, y1;
  uint16_t w, h;
  String context = String(cwaPracticeTypeNames[cwaSelectedPracticeType]);
  tft.getTextBounds(context, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 18);
  tft.print(context);

  // Message type name (large, centered)
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  String typeText = String(cwaMessageTypeNames[cwaSelectedMessageType]);
  tft.getTextBounds(typeText, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 60);
  tft.print(typeText);

  // Description (cyan for visibility)
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  String desc = String(cwaMessageTypeDescriptions[cwaSelectedMessageType]);
  tft.getTextBounds(desc, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 95);
  tft.print(desc);

  // Navigation hint
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  String hint = String(cwaSelectedMessageType + 1) + " of " + String(CWA_TOTAL_MESSAGE_TYPES);
  tft.getTextBounds(hint, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 125);
  tft.print(hint);

  // Navigation arrows
  if (cwaSelectedMessageType > MESSAGE_CHARACTERS) {
    tft.fillTriangle(
      SCREEN_WIDTH / 2, cardY - 15,
      SCREEN_WIDTH / 2 - 12, cardY - 5,
      SCREEN_WIDTH / 2 + 12, cardY - 5,
      ST77XX_CYAN
    );
  }

  if (cwaSelectedMessageType < MESSAGE_PHRASES) {
    tft.fillTriangle(
      SCREEN_WIDTH / 2, cardY + cardH + 15,
      SCREEN_WIDTH / 2 - 12, cardY + cardH + 5,
      SCREEN_WIDTH / 2 + 12, cardY + cardH + 5,
      ST77XX_CYAN
    );
  }

  // Footer
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  String helpText = "\x18\x19 Select  ENTER Start  ESC Back";
  tft.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, SCREEN_HEIGHT - 12);
  tft.print(helpText);
}

/*
 * Handle input for CW Academy message type selection
 * Returns: -1 to exit to practice type selection, 0 for normal input, 2 for redraw
 */
int handleCWAMessageTypeSelectInput(char key, Adafruit_ST7789& tft) {
  bool needsRedraw = false;

  if (key == KEY_UP) {
    if (cwaSelectedMessageType > MESSAGE_CHARACTERS) {
      cwaSelectedMessageType = (CWAMessageType)((int)cwaSelectedMessageType - 1);
      needsRedraw = true;
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
  } else if (key == KEY_DOWN) {
    if (cwaSelectedMessageType < MESSAGE_PHRASES) {
      cwaSelectedMessageType = (CWAMessageType)((int)cwaSelectedMessageType + 1);
      needsRedraw = true;
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Start copy practice mode
    saveCWAProgress();
    beep(TONE_SELECT, BEEP_MEDIUM);
    return 1;  // Signal to start copy practice mode

  } else if (key == KEY_ESC) {
    return -1; // Return to practice type selection
  }

  if (needsRedraw) {
    return 2;
  }

  return 0;
}

/*
 * Initialize CW Academy mode (called when entering from Training menu)
 * Shows track selection screen
 */
void startCWAcademy(Adafruit_ST7789& tft) {
  loadCWAProgress();
  drawCWATrackSelectUI(tft);
}

// ============================================
// CW ACADEMY COPY PRACTICE MODE
// ============================================

/*
 * Character sets for each session (Beginner track)
 * Based on cumulative character introduction
 */
const char* cwaSessionCharSets[] = {
  "AENT",                                  // Session 1: 4 chars
  "AENT SI O14",                          // Session 2: 9 chars
  "AENTSIO14RHDL25",                      // Session 3: 15 chars
  "AENTSIO14RHDL25CU36",                  // Session 4: 18 chars
  "AENTSIO14RHDL25CU36MW7/",              // Session 5: 22 chars
  "AENTSIO14RHDL25CU36MW7/PB8?",          // Session 6: 26 chars
  "AENTSIO14RHDL25CU36MW7/PB8?GY9=",      // Session 7: 30 chars
  "AENTSIO14RHDL25CU36MW7/PB8?GY9=FV0+",  // Session 8: 34 chars
  "AENTSIO14RHDL25CU36MW7/PB8?GY9=FV0+KJ,Q",  // Session 9: 38 chars
  "AENTSIO14RHDL25CU36MW7/PB8?GY9=FV0+KJ,QXZ.<BK><SK>",  // Session 10: 44 chars (complete!)
  "AENTSIO14RHDL25CU36MW7/PB8?GY9=FV0+KJ,QXZ.<BK><SK>",  // Session 11: QSO practice (all chars)
  "AENTSIO14RHDL25CU36MW7/PB8?GY9=FV0+KJ,QXZ.<BK><SK>",  // Session 12: QSO practice
  "AENTSIO14RHDL25CU36MW7/PB8?GY9=FV0+KJ,QXZ.<BK><SK>",  // Session 13: QSO practice
  "AENTSIO14RHDL25CU36MW7/PB8?GY9=FV0+KJ,QXZ.<BK><SK>",  // Session 14: On-air prep
  "AENTSIO14RHDL25CU36MW7/PB8?GY9=FV0+KJ,QXZ.<BK><SK>",  // Session 15: On-air prep
  "AENTSIO14RHDL25CU36MW7/PB8?GY9=FV0+KJ,QXZ.<BK><SK>"   // Session 16: On-air prep
};

// Copy practice state variables
String cwaCopyTarget = "";        // What was sent (correct answer)
String cwaCopyInput = "";         // What user typed
int cwaCopyRound = 0;             // Current round number
int cwaCopyCorrect = 0;           // Number correct this session
int cwaCopyTotal = 0;             // Total attempts this session
int cwaCopyCharCount = 5;         // Number of characters per round (1-10, adjustable with UP/DOWN arrows)
bool cwaCopyWaitingForInput = false;  // Waiting for user to type
bool cwaCopyShowingFeedback = false;  // Showing correct/incorrect feedback

/*
 * Generate random content based on message type and session
 */
String generateCWAContent() {
  String charSet = String(cwaSessionCharSets[cwaSelectedSession - 1]);
  String result = "";

  if (cwaSelectedMessageType == MESSAGE_CHARACTERS) {
    // Generate random characters from session's character set
    for (int i = 0; i < cwaCopyCharCount; i++) {
      int index = random(charSet.length());
      result += charSet[index];
    }
    return result;
  }

  if (cwaSelectedMessageType == MESSAGE_WORDS) {
    // Generate simple "words" (2-5 letter sequences) using only session characters
    int wordsToGenerate = max(1, cwaCopyCharCount / 3);  // Roughly 1 word per 3 chars
    for (int w = 0; w < wordsToGenerate; w++) {
      if (w > 0) result += " ";
      int wordLen = random(2, min(6, (int)charSet.length() + 1));  // 2-5 letters
      for (int i = 0; i < wordLen; i++) {
        int index = random(charSet.length());
        result += charSet[index];
      }
    }
    return result;
  }

  // For other message types, use random characters from session set (placeholder)
  // TODO: Implement proper abbreviations, numbers, callsigns, phrases
  for (int i = 0; i < cwaCopyCharCount; i++) {
    int index = random(charSet.length());
    result += charSet[index];
  }
  return result;
}

/*
 * Start copy practice mode
 */
void startCWACopyPractice(Adafruit_ST7789& tft) {
  cwaCopyRound = 0;
  cwaCopyCorrect = 0;
  cwaCopyTotal = 0;
  cwaCopyInput = "";
  cwaCopyWaitingForInput = false;
  cwaCopyShowingFeedback = false;

  randomSeed(analogRead(0));  // Seed random number generator

  drawCWACopyPracticeUI(tft);
}

/*
 * Start a new round of practice
 */
void startCWACopyRound() {
  cwaCopyRound++;
  cwaCopyInput = "";
  cwaCopyTarget = generateCWAContent();
  cwaCopyWaitingForInput = false;
  cwaCopyShowingFeedback = false;

  // Draw the UI FIRST showing the ready state
  drawCWACopyPracticeUI(tft);

  // Then play the morse code after a brief delay
  delay(1000);
  playMorseString(cwaCopyTarget.c_str(), cwSpeed, cwTone);

  // Now ready for input
  cwaCopyWaitingForInput = true;
  drawCWACopyPracticeUI(tft);  // Redraw to show input prompt
}

/*
 * Draw copy practice UI
 */
void drawCWACopyPracticeUI(Adafruit_ST7789& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  // Score display and settings
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 50);
  tft.print("Round: ");
  tft.print(cwaCopyRound);
  tft.print("/10");

  tft.setCursor(SCREEN_WIDTH - 100, 50);
  tft.print("Score: ");
  tft.print(cwaCopyCorrect);
  tft.print("/");
  tft.print(cwaCopyTotal);

  // Character count setting (in middle)
  tft.setCursor(130, 50);
  tft.print("Chars: ");
  tft.print(cwaCopyCharCount);

  if (cwaCopyShowingFeedback) {
    // Show what was sent and what was typed
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(20, 80);
    tft.print("Sent:");

    tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(20, 100);
    tft.print(cwaCopyTarget);

    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(20, 140);
    tft.print("You typed:");

    tft.setTextSize(3);
    bool correct = (cwaCopyInput.equalsIgnoreCase(cwaCopyTarget));
    tft.setTextColor(correct ? ST77XX_GREEN : ST77XX_RED);
    tft.setCursor(20, 160);
    tft.print(cwaCopyInput);

    // Feedback message
    tft.setTextSize(2);
    tft.setTextColor(correct ? ST77XX_GREEN : ST77XX_RED);
    tft.setCursor(20, 195);
    tft.print(correct ? "CORRECT!" : "INCORRECT");

  } else if (cwaCopyWaitingForInput) {
    // Show input prompt
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(20, 90);
    tft.print("Type what you heard:");

    // Show current input
    tft.fillRect(15, 120, 290, 50, 0x1082);  // Input box
    tft.drawRect(15, 120, 290, 50, 0x34BF);

    tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(25, 135);
    tft.print(cwaCopyInput);

  } else {
    // Show "Get ready" message before playing morse
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(30, 90);
    tft.print("Type what you hear:");

    // Show empty input box ready for typing
    tft.fillRect(15, 120, 290, 50, 0x1082);  // Input box
    tft.drawRect(15, 120, 290, 50, 0x34BF);

    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(80, 180);
    tft.print("Listening...");
  }

  // Footer
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  String helpText;
  if (cwaCopyShowingFeedback) {
    helpText = "Any key: Continue  \x18\x19 Chars  ESC Exit";
  } else if (cwaCopyWaitingForInput) {
    helpText = "SPACE Replay  ENTER Submit  \x18\x19 Chars  ESC";
  } else {
    helpText = "\x18\x19 Adjust chars  ESC Exit";
  }
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 12);
  tft.print(helpText);
}

/*
 * Handle input for copy practice mode
 * Returns: -1 to exit, 0 for normal input, 2 for redraw
 */
int handleCWACopyPracticeInput(char key, Adafruit_ST7789& tft) {
  // ESC always exits
  if (key == KEY_ESC) {
    return -1;  // Return to message type selection
  }

  // Handle character count adjustment with UP/DOWN arrows (1-10)
  if (key == KEY_UP) {
    if (cwaCopyCharCount < 10) {
      cwaCopyCharCount++;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;  // Redraw
    }
  } else if (key == KEY_DOWN) {
    if (cwaCopyCharCount > 1) {
      cwaCopyCharCount--;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;  // Redraw
    }
  }

  // If showing feedback, any key starts next round
  if (cwaCopyShowingFeedback) {
    if (cwaCopyRound >= 10) {
      // Finished all rounds, show final score
      tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);
      tft.setTextSize(2);
      tft.setTextColor(ST77XX_CYAN);
      tft.setCursor(40, 80);
      tft.print("Practice Complete!");

      tft.setTextSize(3);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(60, 120);
      tft.print("Score: ");
      tft.print(cwaCopyCorrect);
      tft.print("/");
      tft.print(cwaCopyTotal);

      int percentage = (cwaCopyTotal > 0) ? (cwaCopyCorrect * 100 / cwaCopyTotal) : 0;
      tft.setTextSize(2);
      tft.setTextColor(percentage >= 70 ? ST77XX_GREEN : ST77XX_YELLOW);
      tft.setCursor(90, 160);
      tft.print(percentage);
      tft.print("%");

      tft.setTextSize(1);
      tft.setTextColor(0x7BEF);
      tft.setCursor(60, 200);
      tft.print("Press any key to exit...");

      delay(3000);
      return -1;  // Exit to message type selection
    } else {
      startCWACopyRound();  // This handles its own UI drawing
      return 0;  // Already handled, no redraw needed
    }
  }

  // Waiting for input
  if (cwaCopyWaitingForInput) {
    if (key == ' ') {  // Space bar for replay
      // Replay the morse code
      playMorseString(cwaCopyTarget.c_str(), cwSpeed, cwTone);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 0;  // No redraw needed

    } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      // Submit answer
      cwaCopyTotal++;
      if (cwaCopyInput.equalsIgnoreCase(cwaCopyTarget)) {
        cwaCopyCorrect++;
        beep(1000, 200);  // Success beep
      } else {
        beep(400, 300);  // Error beep
      }
      cwaCopyShowingFeedback = true;
      cwaCopyWaitingForInput = false;
      return 2;  // Redraw to show feedback

    } else if (key == 0x08 || key == 0x7F) {  // Backspace or DEL
      if (cwaCopyInput.length() > 0) {
        cwaCopyInput.remove(cwaCopyInput.length() - 1);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;  // Redraw
      }
    } else if (key >= 33 && key <= 126) {  // Printable character (excluding space)
      if (cwaCopyInput.length() < 20) {  // Limit input length
        cwaCopyInput += (char)toupper(key);
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;  // Redraw
      }
    }
  }

  return 0;
}

#endif // TRAINING_CWA_H
