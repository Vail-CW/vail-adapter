/*
 * CW Academy Training Module - Main Header
 * Modular implementation of the CW Academy Beginner Curriculum
 *
 * This file coordinates all CWA training modules:
 * - Core: Shared definitions, enums, and utilities
 * - Menus: Track/session/message type selection
 * - Copy Practice: Receive morse and type
 * - Sending Practice: Transmit morse with paddle
 * - Data: Session curriculum content
 *
 * Future tracks (Fundamental, Intermediate, Advanced) can be added
 * by creating new data files and extending the menu system.
 */

#ifndef TRAINING_CWA_H
#define TRAINING_CWA_H

// ============================================
// Include Core Module (Foundation)
// ============================================
#include "training_cwa_core.h"  // Same folder

// ============================================
// Include Practice Modules
// ============================================
#include "training_cwa_copy_practice.h"  // Same folder
#include "training_cwa_send_practice.h"  // Same folder
#include "training_cwa_qso_practice.h"  // Same folder

// ============================================
// Menu and Navigation Functions
// (Inline implementation to avoid circular dependencies)
// ============================================

// Forward declaration for header drawing
extern void drawHeader();

/*
 * Draw track selection screen
 */
void drawCWATrackSelectUI(LGFX& tft) {
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
  getTextBounds_compat(tft, indicator.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 18);
  tft.print(indicator);

  // Track name (large, centered)
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  String trackText = String(cwaTrackNames[cwaSelectedTrack]);
  getTextBounds_compat(tft, trackText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 60);
  tft.print(trackText);

  // Track description
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  String desc = String(cwaTrackDescriptions[cwaSelectedTrack]);
  getTextBounds_compat(tft, desc.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 95);
  tft.print(desc);

  // Navigation hint
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  String navHint = "16 Sessions";
  getTextBounds_compat(tft, navHint.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 125);
  tft.print(navHint);

  // Navigation arrows
  if (cwaSelectedTrack > TRACK_BEGINNER) {
    tft.fillTriangle(
      SCREEN_WIDTH / 2, cardY - 15,
      SCREEN_WIDTH / 2 - 12, cardY - 5,
      SCREEN_WIDTH / 2 + 12, cardY - 5,
      ST77XX_CYAN
    );
  }

  if (cwaSelectedTrack < TRACK_ADVANCED) {
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
  getTextBounds_compat(tft, helpText.c_str(), 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, SCREEN_HEIGHT - 12);
  tft.print(helpText);
}

/*
 * Handle input for CW Academy track selection
 */
int handleCWATrackSelectInput(char key, LGFX& tft) {
  if (key == KEY_UP) {
    if (cwaSelectedTrack > TRACK_BEGINNER) {
      cwaSelectedTrack = (CWATrack)((int)cwaSelectedTrack - 1);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  } else if (key == KEY_DOWN) {
    if (cwaSelectedTrack < TRACK_ADVANCED) {
      cwaSelectedTrack = (CWATrack)((int)cwaSelectedTrack + 1);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    saveCWAProgress();
    beep(TONE_SELECT, BEEP_MEDIUM);
    return 1; // Navigate to session selection
  } else if (key == KEY_ESC) {
    return -1; // Exit to training menu
  }

  return 0;
}

/*
 * Draw session selection screen
 */
void drawCWASessionSelectUI(LGFX& tft) {
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  int cardX = 20;
  int cardY = 60;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 140;

  tft.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  const CWASession& session = cwaSessionData[cwaSelectedSession - 1];

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  int16_t x1, y1;
  uint16_t w, h;
  String trackLabel = String(cwaTrackNames[cwaSelectedTrack]) + " Track";
  getTextBounds_compat(tft, trackLabel.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 18);
  tft.print(trackLabel);

  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  String sessionText = "Session " + String(cwaSelectedSession);
  getTextBounds_compat(tft, sessionText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 60);
  tft.print(sessionText);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  String charInfo = String(session.charCount) + " characters";
  getTextBounds_compat(tft, charInfo.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 90);
  tft.print(charInfo);

  if (strlen(session.newChars) > 0) {
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    String newCharsText = "New: " + String(session.newChars);
    getTextBounds_compat(tft, newCharsText.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(cardX + (cardW - w) / 2, cardY + 115);
    tft.print(newCharsText);
  }

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  String desc = session.description;
  getTextBounds_compat(tft, desc.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 132);
  tft.print(desc);

  // Navigation arrows
  if (cwaSelectedSession > 1) {
    tft.fillTriangle(SCREEN_WIDTH / 2, cardY - 15, SCREEN_WIDTH / 2 - 12, cardY - 5, SCREEN_WIDTH / 2 + 12, cardY - 5, ST77XX_CYAN);
  }
  if (cwaSelectedSession < CWA_TOTAL_SESSIONS) {
    tft.fillTriangle(SCREEN_WIDTH / 2, cardY + cardH + 15, SCREEN_WIDTH / 2 - 12, cardY + cardH + 5, SCREEN_WIDTH / 2 + 12, cardY + cardH + 5, ST77XX_CYAN);
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  String helpText = "\x18\x19 Select  ENTER Continue  ESC Back";
  getTextBounds_compat(tft, helpText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 12);
  tft.print(helpText);
}

/*
 * Handle input for CW Academy session selection
 */
int handleCWASessionSelectInput(char key, LGFX& tft) {
  if (key == KEY_UP) {
    if (cwaSelectedSession > 1) {
      cwaSelectedSession--;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  } else if (key == KEY_DOWN) {
    if (cwaSelectedSession < CWA_TOTAL_SESSIONS) {
      cwaSelectedSession++;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    saveCWAProgress();
    beep(TONE_SELECT, BEEP_MEDIUM);
    return 1; // Navigate to practice type selection
  } else if (key == KEY_ESC) {
    return -1; // Return to track selection
  }

  return 0;
}

/*
 * Draw practice type selection screen
 */
void drawCWAPracticeTypeSelectUI(LGFX& tft) {
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  bool advancedLocked = (cwaSelectedSession <= 10);
  bool currentTypeLocked = advancedLocked && (cwaSelectedPracticeType != PRACTICE_COPY);

  int cardX = 20;
  int cardY = 60;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 140;

  tft.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  int16_t x1, y1;
  uint16_t w, h;
  String context = String(cwaTrackNames[cwaSelectedTrack]) + " - Session " + String(cwaSelectedSession);
  getTextBounds_compat(tft, context.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 18);
  tft.print(context);

  tft.setTextSize(2);
  tft.setTextColor(currentTypeLocked ? 0x4208 : ST77XX_WHITE);
  String typeText = String(cwaPracticeTypeNames[cwaSelectedPracticeType]);
  getTextBounds_compat(tft, typeText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 60);
  tft.print(typeText);

  tft.setTextSize(2);
  if (currentTypeLocked) {
    tft.setTextColor(ST77XX_RED);
    String lockMsg = "LOCKED";
    getTextBounds_compat(tft, lockMsg.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(cardX + (cardW - w) / 2, cardY + 85);
    tft.print(lockMsg);

    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    String hint = "Unlocks at Session 11";
    getTextBounds_compat(tft, hint.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(cardX + (cardW - w) / 2, cardY + 105);
    tft.print(hint);
  } else {
    tft.setTextColor(ST77XX_CYAN);
    String desc = String(cwaPracticeTypeDescriptions[cwaSelectedPracticeType]);
    getTextBounds_compat(tft, desc.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(cardX + (cardW - w) / 2, cardY + 95);
    tft.print(desc);
  }

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  String hint = String(cwaSelectedPracticeType + 1) + " of " + String(CWA_TOTAL_PRACTICE_TYPES);
  getTextBounds_compat(tft, hint.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 125);
  tft.print(hint);

  if (cwaSelectedPracticeType > PRACTICE_COPY) {
    tft.fillTriangle(SCREEN_WIDTH / 2, cardY - 15, SCREEN_WIDTH / 2 - 12, cardY - 5, SCREEN_WIDTH / 2 + 12, cardY - 5, ST77XX_CYAN);
  }
  if (cwaSelectedPracticeType < PRACTICE_DAILY_DRILL) {
    tft.fillTriangle(SCREEN_WIDTH / 2, cardY + cardH + 15, SCREEN_WIDTH / 2 - 12, cardY + cardH + 5, SCREEN_WIDTH / 2 + 12, cardY + cardH + 5, ST77XX_CYAN);
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  String helpText = "\x18\x19 Select  ENTER Continue  ESC Back";
  getTextBounds_compat(tft, helpText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 12);
  tft.print(helpText);
}

/*
 * Handle input for CW Academy practice type selection
 * Return codes:
 *   -1 = Exit to session selection
 *    0 = No action
 *    1 = Navigate to message type selection (for copy/sending with sessions 1-10, 14-16)
 *    2 = Redraw UI
 *    3 = Start QSO practice (sessions 11-13)
 */
int handleCWAPracticeTypeSelectInput(char key, LGFX& tft) {
  bool advancedLocked = (cwaSelectedSession <= 10);
  bool currentTypeLocked = advancedLocked && (cwaSelectedPracticeType != PRACTICE_COPY);

  if (key == KEY_UP) {
    if (cwaSelectedPracticeType > PRACTICE_COPY) {
      cwaSelectedPracticeType = (CWAPracticeType)((int)cwaSelectedPracticeType - 1);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  } else if (key == KEY_DOWN) {
    if (cwaSelectedPracticeType < PRACTICE_DAILY_DRILL) {
      cwaSelectedPracticeType = (CWAPracticeType)((int)cwaSelectedPracticeType + 1);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    if (currentTypeLocked) {
      beep(600, 150);
      tft.fillRect(0, 210, SCREEN_WIDTH, 20, COLOR_BACKGROUND);
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_RED);
      int16_t x1, y1;
      uint16_t w, h;
      String msg = "Available at Session 11+";
      getTextBounds_compat(tft, msg.c_str(), 0, 0, &x1, &y1, &w, &h);
      tft.setCursor((SCREEN_WIDTH - w) / 2, 215);
      tft.print(msg);
      delay(1500);
      return 2;
    } else {
      saveCWAProgress();
      beep(TONE_SELECT, BEEP_MEDIUM);

      // Sessions 11-13 go directly to QSO practice (bypass message type selection)
      if (cwaSelectedSession >= 11 && cwaSelectedSession <= 13) {
        return 3; // Start QSO practice
      } else {
        return 1; // Navigate to message type selection (for copy/sending practice)
      }
    }
  } else if (key == KEY_ESC) {
    return -1; // Return to session selection
  }

  return 0;
}

/*
 * Draw message type selection screen
 */
void drawCWAMessageTypeSelectUI(LGFX& tft) {
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  int cardX = 20;
  int cardY = 60;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 140;

  tft.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  int16_t x1, y1;
  uint16_t w, h;
  String context = String(cwaPracticeTypeNames[cwaSelectedPracticeType]);
  getTextBounds_compat(tft, context.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 18);
  tft.print(context);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  String typeText = String(cwaMessageTypeNames[cwaSelectedMessageType]);
  getTextBounds_compat(tft, typeText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 60);
  tft.print(typeText);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  String desc = String(cwaMessageTypeDescriptions[cwaSelectedMessageType]);
  getTextBounds_compat(tft, desc.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 95);
  tft.print(desc);

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  String hint = String(cwaSelectedMessageType + 1) + " of " + String(CWA_TOTAL_MESSAGE_TYPES);
  getTextBounds_compat(tft, hint.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 125);
  tft.print(hint);

  if (cwaSelectedMessageType > MESSAGE_CHARACTERS) {
    tft.fillTriangle(SCREEN_WIDTH / 2, cardY - 15, SCREEN_WIDTH / 2 - 12, cardY - 5, SCREEN_WIDTH / 2 + 12, cardY - 5, ST77XX_CYAN);
  }
  if (cwaSelectedMessageType < MESSAGE_PHRASES) {
    tft.fillTriangle(SCREEN_WIDTH / 2, cardY + cardH + 15, SCREEN_WIDTH / 2 - 12, cardY + cardH + 5, SCREEN_WIDTH / 2 + 12, cardY + cardH + 5, ST77XX_CYAN);
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  String helpText = "\x18\x19 Select  ENTER Start  ESC Back";
  getTextBounds_compat(tft, helpText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 12);
  tft.print(helpText);
}

/*
 * Handle input for CW Academy message type selection
 * Return codes:
 *   -1 = Exit to practice type selection
 *    0 = No action
 *    1 = Start copy practice
 *    2 = Redraw UI
 *    3 = Start sending practice
 */
int handleCWAMessageTypeSelectInput(char key, LGFX& tft) {
  if (key == KEY_UP) {
    if (cwaSelectedMessageType > MESSAGE_CHARACTERS) {
      cwaSelectedMessageType = (CWAMessageType)((int)cwaSelectedMessageType - 1);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  } else if (key == KEY_DOWN) {
    if (cwaSelectedMessageType < MESSAGE_PHRASES) {
      cwaSelectedMessageType = (CWAMessageType)((int)cwaSelectedMessageType + 1);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;
    }
  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    saveCWAProgress();
    beep(TONE_SELECT, BEEP_MEDIUM);

    // Route to appropriate practice mode based on type
    if (cwaSelectedPracticeType == PRACTICE_COPY) {
      return 1; // Start copy practice
    } else if (cwaSelectedPracticeType == PRACTICE_SENDING) {
      return 3; // Start sending practice
    } else {
      // PRACTICE_DAILY_DRILL - not yet implemented, default to copy
      return 1;
    }
  } else if (key == KEY_ESC) {
    return -1; // Return to practice type selection
  }

  return 0;
}

/*
 * Initialize CW Academy mode (entry point from Training menu)
 */
void startCWAcademy(LGFX& tft) {
  loadCWAProgress();
  drawCWATrackSelectUI(tft);
}

#endif // TRAINING_CWA_H
