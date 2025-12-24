/*
 * Ham Radio License Study - UI Rendering
 * Modern, clean interface using VAIL SUMMIT's design language
 */

#ifndef TRAINING_LICENSE_UI_H
#define TRAINING_LICENSE_UI_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "../core/config.h"
#include "training_license_core.h"
#include "training_license_data.h"
#include "training_license_stats.h"

// Forward declarations
extern LGFX tft;
extern MenuMode currentMode;
extern int currentSelection;

// ============================================
// Helper Functions
// ============================================

/**
 * Word-wrap text to fit within width
 */
void wrapText(LGFX& display, String text, int maxWidth, String* lines, int& lineCount, int maxLines) {
  lineCount = 0;
  int startIdx = 0;

  while (startIdx < text.length() && lineCount < maxLines) {
    int endIdx = startIdx;
    int lastSpace = -1;

    // Find end of line that fits
    while (endIdx < text.length()) {
      char c = text[endIdx];
      if (c == ' ') lastSpace = endIdx;

      // Measure width so far
      String substr = text.substring(startIdx, endIdx + 1);
      int16_t x1, y1;
      uint16_t w, h;
      getTextBounds_compat(display, substr.c_str(), 0, 0, &x1, &y1, &w, &h);

      if (w > maxWidth) {
        // Line is too wide - break at last space
        if (lastSpace > startIdx) {
          lines[lineCount++] = text.substring(startIdx, lastSpace);
          startIdx = lastSpace + 1;
        } else {
          // No space found - hard break
          lines[lineCount++] = text.substring(startIdx, endIdx);
          startIdx = endIdx;
        }
        break;
      }

      endIdx++;
    }

    // Reached end of text
    if (endIdx >= text.length() && startIdx < text.length()) {
      lines[lineCount++] = text.substring(startIdx);
      break;
    }
  }
}

/**
 * Draw an answer box with symbol, letter, and text
 * @param tft Display reference
 * @param y Y position for box
 * @param h Height of box
 * @param answerIdx Index of answer (0-3)
 * @param q Question containing answers
 * @param color Background color for box
 * @param symbol Symbol to display ("√", "X", or "")
 * @param showRef Whether to append reference text
 */
void drawAnswerBox(LGFX& tft, int y, int h, int answerIdx,
                   const LicenseQuestion* q, uint16_t color,
                   const char* symbol, bool showRef = false) {
  const int answerX = 10;
  const int answerWidth = 460;

  // Draw rounded rect box
  tft.fillRoundRect(answerX, y, answerWidth, h, 6, color);
  tft.drawRoundRect(answerX, y, answerWidth, h, 6, color);

  // Draw symbol and letter (e.g., "√ A." or "X B." or "A.")
  tft.setTextColor(ST77XX_WHITE);
  tft.setFont(&FreeSansBold12pt7b);

  int xPos = answerX + 15;
  if (symbol && strlen(symbol) > 0) {
    tft.setCursor(xPos, y + 18);
    tft.print(symbol);
    xPos += 25;
  }

  char letter = 'A' + answerIdx;
  tft.setCursor(xPos, y + 18);
  tft.print(letter);
  tft.print(".");

  // Build answer text with optional reference
  String answerText = String(q->answers[answerIdx]);
  if (showRef && strlen(q->refs) > 0) {
    answerText += " ";
    answerText += String(q->refs);
  }

  // Draw wrapped answer text
  tft.setFont(&FreeSansBold9pt7b);
  int maxLines = (h == 50) ? 3 : 5;  // 3 lines for 50px, 5 lines for 70/82px
  String answerLines[5];
  int answerLineCount = 0;
  wrapText(tft, answerText, answerWidth - (symbol && strlen(symbol) > 0 ? 100 : 80),
           answerLines, answerLineCount, maxLines);

  int textY = y + 13;  // Start slightly higher
  int lineSpacing = (h == 50) ? 15 : 16;  // Tighter spacing for 50px boxes
  for (int j = 0; j < answerLineCount && j < maxLines; j++) {
    tft.setCursor(answerX + (symbol && strlen(symbol) > 0 ? 75 : 50), textY);
    tft.print(answerLines[j]);
    textY += lineSpacing;
  }
}

// ============================================
// Quiz Screen UI
// ============================================

/**
 * Draw quiz screen with question and answers
 */
void drawLicenseQuizUI(LGFX& tft) {
  if (!activePool || !activePool->questions || licenseSession.currentQuestionIndex >= activePool->totalQuestions) {
    return;
  }

  LicenseQuestion* q = &activePool->questions[licenseSession.currentQuestionIndex];

  // Clear screen with deep background
  tft.fillScreen(COLOR_BG_DEEP);

  // Draw compact header
  tft.fillRect(0, 0, SCREEN_WIDTH, 30, COLOR_BG_LAYER2);
  tft.drawLine(0, 30, SCREEN_WIDTH, 30, COLOR_BORDER_SUBTLE);

  // Header title with mastery percentage, license type, and progress
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);

  String header = "";
  // Calculate overall mastery percentage (mastered questions / total questions)
  if (activePool && activePool->progress) {
    int masteredCount = 0;
    for (int i = 0; i < activePool->totalQuestions; i++) {
      if (activePool->progress[i].correct >= 5) {
        masteredCount++;
      }
    }
    int masteryPct = (masteredCount * 100) / activePool->totalQuestions;
    header += String(masteryPct);
    header += "% | ";
  }
  header += getLicenseShortName(licenseSession.selectedLicense);
  header += " | Q ";
  header += String(licenseSession.sessionTotal + 1);
  header += "/";
  header += String(activePool->totalQuestions);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, header.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 8);  // Moved up from 12 to 8 (-4px)
  tft.print(header);
  tft.setFont(nullptr);

  // Question area - give it lots of space
  int yPos = 45;

  // Draw question ID
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_ACCENT_CYAN);
  tft.setCursor(10, yPos);
  tft.print(String(q->id) + ":");

  yPos += 20;

  // Word-wrap question text using 9pt font - lots of space available
  String lines[12];
  int lineCount = 0;
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  wrapText(tft, String(q->question), 460, lines, lineCount, 12);

  // Draw question lines
  int lineHeight = 18;
  for (int i = 0; i < lineCount && i < 12; i++) {
    tft.setCursor(10, yPos);
    tft.print(lines[i]);
    yPos += lineHeight;
  }

  // Answer area - give it more space, positioned in middle/lower section
  int answerAreaY = SCREEN_HEIGHT - 144;  // Reserve bottom 144px for answer display (+4px)

  // Draw separator line
  tft.drawLine(0, answerAreaY - 5, SCREEN_WIDTH, answerAreaY - 5, COLOR_BORDER_SUBTLE);

  // Show current answer indicator (A/B/C/D navigation)
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);

  String answerNav = "";
  for (int i = 0; i < 4; i++) {
    if (i > 0) answerNav += "  ";
    if (i == licenseSession.selectedAnswerIndex) {
      answerNav += "[";
      answerNav += char('A' + i);
      answerNav += "]";
    } else {
      answerNav += char('A' + i);
    }
  }

  getTextBounds_compat(tft, answerNav.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, answerAreaY + 7);  // Moved up from +15 to +7 (-8px)
  tft.print(answerNav);

  // Draw answer box(es) based on feedback state
  if (licenseSession.showingFeedback &&
      licenseSession.selectedAnswerIndex != q->correctAnswer) {

    // WRONG ANSWER - Show both red (wrong) and green (correct) boxes
    int wrongIdx = licenseSession.selectedAnswerIndex;
    int correctIdx = q->correctAnswer;

    // Red box with X symbol (top, 50px)
    drawAnswerBox(tft, answerAreaY + 21, 50, wrongIdx, q,
                  COLOR_ERROR_PASTEL, "X", false);

    // Green box with √ symbol and reference (bottom, 50px)
    drawAnswerBox(tft, answerAreaY + 79, 50, correctIdx, q,
                  COLOR_SUCCESS_PASTEL, "√", true);

  } else if (licenseSession.showingFeedback) {

    // CORRECT ANSWER - Single green box with √ symbol
    drawAnswerBox(tft, answerAreaY + 26, 70,
                  licenseSession.selectedAnswerIndex, q,
                  COLOR_SUCCESS_PASTEL, "√", false);

  } else {

    // NORMAL MODE - Blue selection box, no symbol (82px tall, positioned lower)
    drawAnswerBox(tft, answerAreaY + 38, 82,
                  licenseSession.selectedAnswerIndex, q,
                  COLOR_ACCENT_BLUE, "", false);
  }

  // Footer with instructions - compact
  int footerY = SCREEN_HEIGHT - 21;  // Moved up from -18 to -21 (+3px higher)
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_WARNING_PASTEL);  // Changed to yellow/orange

  String instructions = licenseSession.showingFeedback ?
    "Any key: Next  |  ESC: Exit" :
    "Arrows: Cycle  |  A-D/Enter: Submit  |  ESC: Exit";

  getTextBounds_compat(tft, instructions.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, footerY);
  tft.print(instructions);

  tft.setFont(nullptr);
}

// ============================================
// Statistics Screen UI
// ============================================

/**
 * Draw statistics screen
 */
void drawLicenseStatsUI(LGFX& tft) {
  // Clear screen
  tft.fillScreen(COLOR_BG_DEEP);

  // Draw header
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_LAYER2);
  tft.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BORDER_SUBTLE);

  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);

  String title = "STATS: ";
  String licenseName = String(getLicenseName(licenseSession.selectedLicense));
  licenseName.toUpperCase();
  title += licenseName;

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 30);
  tft.print(title);

  // Get statistics
  LicenseStatistics* stats = getStatistics(licenseSession.selectedLicense);
  if (!stats) {
    tft.setFont(nullptr);
    return;
  }

  // Update statistics
  updateCurrentStatistics();

  int yPos = HEADER_HEIGHT + 20;  // Start closer to header
  const int labelX = 30;
  const int valueX = 280;

  // Pool coverage
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(labelX, yPos);
  tft.print("Pool Coverage:");

  tft.setTextColor(COLOR_ACCENT_CYAN);
  tft.setCursor(valueX, yPos);
  tft.print(String(stats->questionsAttempted));
  tft.print("/");
  tft.print(String(stats->totalQuestions));
  tft.print(" (");
  tft.print(String((int)stats->poolCoverage));
  tft.print("%)");

  yPos += 30;  // Reduced from 35

  // Overall aptitude
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(labelX, yPos);
  tft.print("Overall Aptitude:");

  tft.setTextColor(COLOR_SUCCESS_PASTEL);
  tft.setCursor(valueX, yPos);
  if (stats->questionsAttempted > 0) {
    tft.print(String((int)stats->averageAptitude));
    tft.print("%");
  } else {
    tft.print("--");
  }

  yPos += 32;  // Reduced from 40

  // Breakdown section
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(labelX, yPos);
  tft.print("Mastered:");
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setCursor(valueX, yPos);
  tft.print(String(stats->questionsMastered));
  tft.print(" questions");

  yPos += 22;  // Reduced from 25

  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(labelX, yPos);
  tft.print("Improving:");
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setCursor(valueX, yPos);
  tft.print(String(stats->questionsImproving));
  tft.print(" questions");

  yPos += 22;  // Reduced from 25

  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(labelX, yPos);
  tft.print("Never Seen:");
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setCursor(valueX, yPos);
  tft.print(String(stats->questionsNeverSeen));
  tft.print(" questions");

  yPos += 22;  // Reduced from 25

  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(labelX, yPos);
  tft.print("Weak (< 40%):");
  tft.setTextColor(COLOR_WARNING_PASTEL);
  tft.setCursor(valueX, yPos);
  tft.print(String(stats->questionsWeak));
  tft.print(" questions");

  yPos += 30;  // Reduced from 35

  // Session stats
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(labelX, yPos);
  tft.print("Session Stats:");

  yPos += 26;  // Reduced from 30

  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(labelX + 20, yPos);
  tft.print("Questions:");
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setCursor(valueX, yPos);
  tft.print(String(licenseSession.sessionTotal));

  yPos += 22;  // Reduced from 25

  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(labelX + 20, yPos);
  tft.print("Correct:");
  tft.setTextColor(COLOR_SUCCESS_PASTEL);
  tft.setCursor(valueX, yPos);
  tft.print(String(licenseSession.sessionCorrect));
  if (licenseSession.sessionTotal > 0) {
    float accuracy = getSessionAccuracy();
    tft.print(" (");
    tft.print(String((int)accuracy));
    tft.print("%)");
  }

  // Footer
  int footerY = SCREEN_HEIGHT - 25;
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  String footer = "ESC: Back to License Select";
  getTextBounds_compat(tft, footer.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, footerY);
  tft.print(footer);

  tft.setFont(nullptr);
}

/**
 * Draw SD card required error screen
 */
void drawLicenseSDCardError(LGFX& tft) {
  tft.fillScreen(COLOR_BG_DEEP);

  // Header
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_LAYER2);
  tft.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BORDER_SUBTLE);

  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, "LICENSE STUDY", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 30);
  tft.print("LICENSE STUDY");

  // Error message
  tft.setTextColor(COLOR_ERROR_PASTEL);
  getTextBounds_compat(tft, "SD Card Required", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 130);
  tft.print("SD Card Required");

  // Instructions
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);

  String line1 = "Insert SD card with question files:";
  getTextBounds_compat(tft, line1.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 170);
  tft.print(line1);

  String line2 = "/license/technician.json";
  getTextBounds_compat(tft, line2.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 195);
  tft.print(line2);

  String line3 = "/license/general.json";
  getTextBounds_compat(tft, line3.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 220);
  tft.print(line3);

  String line4 = "/license/extra.json";
  getTextBounds_compat(tft, line4.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 245);
  tft.print(line4);

  // Footer
  String footer = "ESC: Back";
  getTextBounds_compat(tft, footer.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 25);
  tft.print(footer);

  tft.setFont(nullptr);

  // Wait for ESC key
  while (true) {
    char key = 0;
    Wire.requestFrom(CARDKB_ADDR, 1);
    if (Wire.available()) {
      key = Wire.read();
    }
    if (key == KEY_ESC) {
      beep(TONE_MENU_NAV, BEEP_SHORT);
      break;
    }
    delay(50);
  }
}

#endif // TRAINING_LICENSE_UI_H
