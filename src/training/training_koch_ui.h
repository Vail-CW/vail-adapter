/*
 * Koch Method - UI Drawing Functions
 * All visual rendering: main UI, help, settings, character selector
 */

#ifndef TRAINING_KOCH_UI_H
#define TRAINING_KOCH_UI_H

#include "training_koch_core.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Forward declaration
extern void drawHeader();

// ============================================
// Help Screen
// ============================================

void drawKochHelp(Adafruit_ST7789& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  // Title
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(50, 50);
  tft.print("KOCH METHOD");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  if (kochHelpPage == 0) {
    // Page 1: What is Koch Method
    tft.setCursor(10, 80);
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("What is the Koch Method?");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 95);
    tft.print("The Koch Method is a proven");
    tft.setCursor(10, 107);
    tft.print("way to learn morse code:");

    tft.setCursor(10, 125);
    tft.print("1. Start with 2 characters");
    tft.setCursor(15, 137);
    tft.print("(K and M) at 20 WPM");

    tft.setCursor(10, 155);
    tft.print("2. Practice until 90%");
    tft.setCursor(15, 167);
    tft.print("accuracy (10+ attempts)");

    tft.setCursor(10, 185);
    tft.print("3. Add one more character");

    tft.setCursor(10, 203);
    tft.print("4. Repeat through all 44");
    tft.setCursor(15, 215);
    tft.print("characters!");

  } else if (kochHelpPage == 1) {
    // Page 2: Why High Speed?
    tft.setCursor(10, 80);
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("Why start at high speed?");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 95);
    tft.print("Learning morse at high speed");
    tft.setCursor(10, 107);
    tft.print("from the start prevents bad");
    tft.setCursor(10, 119);
    tft.print("habits like 'counting dits");
    tft.setCursor(10, 131);
    tft.print("and dahs'.");

    tft.setCursor(10, 149);
    tft.print("You learn to recognize each");
    tft.setCursor(10, 161);
    tft.print("character by its sound");
    tft.setCursor(10, 173);
    tft.print("pattern, not by memorizing");
    tft.setCursor(10, 185);
    tft.print("dit/dah sequences.");

    tft.setCursor(10, 203);
    tft.print("Speed: 15-30 WPM");
    tft.setCursor(10, 215);
    tft.print("Default: 20 WPM");

  } else {
    // Page 3: Controls
    tft.setCursor(10, 80);
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("How to use:");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 95);
    tft.print("SPACE: Play morse group");

    tft.setCursor(10, 110);
    tft.print("Type answer, press ENTER");

    tft.setCursor(10, 125);
    tft.print("P: Toggle Practice/Test mode");

    tft.setCursor(10, 140);
    tft.print("C: Select practice chars");

    tft.setCursor(10, 155);
    tft.print("+/-: Advance/go back lesson");

    tft.setCursor(10, 170);
    tft.print("S: Settings (WPM, length)");

    tft.setCursor(10, 185);
    tft.print("H: This help screen");

    tft.setCursor(10, 200);
    tft.print("ESC: Return to menu");
  }

  // Footer with page indicator
  tft.setTextColor(0x7BEF);
  tft.setCursor(10, 225);
  tft.print("H=Next page  ESC=Back  ");
  tft.print(kochHelpPage + 1);
  tft.print("/3");
}

// ============================================
// Character Selection (Practice Mode)
// ============================================

void drawCharacterSelector(Adafruit_ST7789& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  // Title
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(30, 50);
  tft.print("SELECT CHARS");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 75);
  tft.print("Choose 1-5 chars to practice:");

  // Show available characters from current lesson
  String available = getKochCharacterSet();

  // Draw character grid
  int startY = 95;
  int charX = 15;
  int charY = startY;
  int charSpacing = 35;
  int charsPerRow = 8;

  tft.setTextSize(2);
  for (int i = 0; i < available.length(); i++) {
    char c = available[i];
    bool isSelected = (kochPracticeChars.indexOf(c) >= 0);

    // Draw box around character
    int boxX = charX - 3;
    int boxY = charY - 3;

    if (isSelected) {
      tft.fillRoundRect(boxX, boxY, 28, 28, 4, ST77XX_GREEN);
      tft.setTextColor(ST77XX_BLACK);
    } else {
      tft.drawRoundRect(boxX, boxY, 28, 28, 4, 0x4A49);
      tft.setTextColor(ST77XX_WHITE);
    }

    tft.setCursor(charX, charY);
    tft.print(c);

    charX += charSpacing;
    if ((i + 1) % charsPerRow == 0) {
      charX = 15;
      charY += 35;
    }
  }

  // Show current selection
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 200);
  tft.print("Selected (");
  tft.print(kochPracticeChars.length());
  tft.print("/5): ");

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_GREEN);
  if (kochPracticeChars.length() > 0) {
    tft.print(kochPracticeChars);
  } else {
    tft.setTextColor(0x7BEF);
    tft.print("(none)");
  }

  // Instructions
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  tft.setCursor(10, 227);
  tft.print("Type char to toggle  ENTER=Done");
}

// ============================================
// Settings Overlay
// ============================================

void drawKochSettings(Adafruit_ST7789& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  // Title
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(80, 55);
  tft.print("SETTINGS");

  // WPM Setting
  int yPos = 100;
  tft.setTextSize(2);
  if (kochSettingsSelection == 0) {
    tft.fillRoundRect(10, yPos - 5, 300, 35, 6, COLOR_HIGHLIGHT_BG);
    tft.setTextColor(COLOR_HIGHLIGHT_FG);
  } else {
    tft.setTextColor(ST77XX_WHITE);
  }
  tft.setCursor(20, yPos);
  tft.print("Speed: ");
  tft.print(kochProgress.wpm);
  tft.print(" WPM");

  // Group Length Setting
  yPos += 50;
  if (kochSettingsSelection == 1) {
    tft.fillRoundRect(10, yPos - 5, 300, 35, 6, COLOR_HIGHLIGHT_BG);
    tft.setTextColor(COLOR_HIGHLIGHT_FG);
  } else {
    tft.setTextColor(ST77XX_WHITE);
  }
  tft.setCursor(20, yPos);
  tft.print("Length: ");
  tft.print(kochProgress.groupLength);
  tft.print(" chars");

  // Instructions
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  tft.setCursor(10, 210);
  tft.print("UP/DN Navigate  L/R Adjust");
  tft.setCursor(10, 225);
  tft.print("ENTER Save  ESC Cancel");
}

// ============================================
// Main UI Drawing
// ============================================

void drawKochUI(Adafruit_ST7789& tft) {
  // If in character selection mode, draw selector and return
  if (kochInCharSelectMode) {
    drawCharacterSelector(tft);
    return;
  }

  // If in help mode, draw help and return
  if (kochInHelpMode) {
    drawKochHelp(tft);
    return;
  }

  // If in settings mode, draw settings and return
  if (kochInSettingsMode) {
    drawKochSettings(tft);
    return;
  }

  // Clear screen but preserve header
  tft.fillRect(0, 40, SCREEN_WIDTH, SCREEN_HEIGHT - 40, COLOR_BACKGROUND);

  // Mode indicator badge (top right, below header)
  uint16_t modeBadgeColor = (kochCurrentMode == KOCH_MODE_PRACTICE) ? ST77XX_MAGENTA : ST77XX_GREEN;
  String modeText = (kochCurrentMode == KOCH_MODE_PRACTICE) ? "PRACTICE" : "TEST";

  tft.fillRoundRect(SCREEN_WIDTH - 90, 43, 85, 14, 4, modeBadgeColor);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(SCREEN_WIDTH - 85, 45);
  tft.print(modeText);

  // Modern card-style info display (3 cards similar to Practice mode)
  const int CARD_Y = 55;
  const int CARD_HEIGHT = 50;
  const int CARD_SPACING = 4;
  const int CARD_WIDTH = (SCREEN_WIDTH - (4 * CARD_SPACING)) / 3;

  int16_t x1, y1;
  uint16_t w, h;

  // Card 1: Lesson Number
  int card1X = CARD_SPACING;

  // Card background
  tft.fillRoundRect(card1X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x2104);
  tft.drawRoundRect(card1X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x4A49);

  // Title badge (hovering over card)
  tft.fillRoundRect(card1X + 5, CARD_Y - 7, 55, 14, 4, ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(card1X + 10, CARD_Y - 5);
  tft.print("LESSON");

  // Lesson value (centered)
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  String lessonStr = String(kochProgress.currentLesson) + "/" + String(KOCH_TOTAL_LESSONS);
  tft.getTextBounds(lessonStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(card1X + (CARD_WIDTH - w) / 2, CARD_Y + 20);
  tft.print(lessonStr);

  // Card 2: Accuracy
  int card2X = card1X + CARD_WIDTH + CARD_SPACING;

  // Clear card area
  tft.fillRect(card2X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, COLOR_BACKGROUND);

  // Card background
  tft.fillRoundRect(card2X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x2104);
  tft.drawRoundRect(card2X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x4A49);

  // Title badge
  int accuracy = getSessionAccuracy();
  bool ready = canAdvanceLesson();
  uint16_t badgeColor = ready ? ST77XX_GREEN : ST77XX_YELLOW;

  tft.fillRoundRect(card2X + 5, CARD_Y - 7, 72, 14, 4, badgeColor);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(card2X + 10, CARD_Y - 5);
  tft.print("ACCURACY");

  // Accuracy value (centered, color-coded)
  tft.setTextSize(2);
  tft.setTextColor(ready ? ST77XX_GREEN : (accuracy > 50 ? ST77XX_YELLOW : ST77XX_RED));
  String accStr = String(accuracy) + "%";
  tft.getTextBounds(accStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(card2X + (CARD_WIDTH - w) / 2, CARD_Y + 20);
  tft.print(accStr);

  // Card 3: Score
  int card3X = card2X + CARD_WIDTH + CARD_SPACING;

  // Card background
  tft.fillRoundRect(card3X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x2104);
  tft.drawRoundRect(card3X, CARD_Y, CARD_WIDTH, CARD_HEIGHT, 6, 0x4A49);

  // Title badge
  tft.fillRoundRect(card3X + 5, CARD_Y - 7, 47, 14, 4, ST77XX_MAGENTA);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(card3X + 10, CARD_Y - 5);
  tft.print("SCORE");

  // Score value (centered) - use current mode's stats
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_MAGENTA);
  String scoreStr = String(getCurrentCorrect()) + "/" + String(getCurrentTotal());
  tft.getTextBounds(scoreStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(card3X + (CARD_WIDTH - w) / 2, CARD_Y + 20);
  tft.print(scoreStr);

  // Progress bar (below cards) - different display for Practice vs Test mode
  const int PROG_Y = 115;
  const int PROG_H = 20;
  const int PROG_W = SCREEN_WIDTH - 20;

  // Background
  tft.fillRoundRect(10, PROG_Y, PROG_W, PROG_H, 6, 0x2104);
  tft.drawRoundRect(10, PROG_Y, PROG_W, PROG_H, 6, 0x4A49);

  String progText;

  if (kochCurrentMode == KOCH_MODE_PRACTICE) {
    // Practice mode - show character selection status
    if (kochPracticeChars.length() > 0) {
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_MAGENTA);
      progText = "Practicing: " + kochPracticeChars + " (Press C to change)";
    } else {
      tft.setTextSize(1);
      tft.setTextColor(0x7BEF);
      String lessonChars = getKochCharacterSet();
      progText = "All lesson chars: " + lessonChars + " (C=Select)";
    }
  } else {
    // Test mode - show progress toward 90%
    if (getCurrentTotal() >= KOCH_MIN_ATTEMPTS) {
      int fillWidth = (accuracy * PROG_W) / 100;
      if (fillWidth > PROG_W - 4) fillWidth = PROG_W - 4;
      if (fillWidth > 0) {
        uint16_t fillColor = ready ? ST77XX_GREEN : (accuracy > 50 ? ST77XX_YELLOW : ST77XX_RED);
        tft.fillRoundRect(12, PROG_Y + 2, fillWidth, PROG_H - 4, 4, fillColor);
      }
    }

    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    progText = String(getCurrentTotal() < KOCH_MIN_ATTEMPTS ? getCurrentTotal() : KOCH_MIN_ATTEMPTS) +
                      "/" + String(KOCH_MIN_ATTEMPTS) + " attempts, " + String(accuracy) + "% → 90%";
  }

  tft.getTextBounds(progText, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, PROG_Y + 6);
  tft.print(progText);

  // Main content area (larger text, cleaner layout)
  const int CONTENT_Y = 145;

  if (kochShowingFeedback) {
    // Show feedback with cleaner layout
    tft.fillRoundRect(10, CONTENT_Y, SCREEN_WIDTH - 20, 65, 8, 0x1082);
    tft.drawRoundRect(10, CONTENT_Y, SCREEN_WIDTH - 20, 65, 8, 0x4A49);

    // "Sent" label
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(20, CONTENT_Y + 8);
    tft.print("Sent:");

    // Sent text
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(80, CONTENT_Y + 3);
    tft.print(kochCurrentGroup);

    // "You typed" label
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(20, CONTENT_Y + 35);
    tft.print("You:");

    // User input text
    tft.setTextSize(3);
    tft.setTextColor(kochCorrectAnswer ? ST77XX_GREEN : ST77XX_RED);
    tft.setCursor(80, CONTENT_Y + 30);
    tft.print(kochUserInput);

    // Result message (below box, bigger and centered)
    tft.setTextSize(3);
    tft.setTextColor(kochCorrectAnswer ? ST77XX_GREEN : ST77XX_RED);
    String resultMsg = kochCorrectAnswer ? "CORRECT!" : "TRY AGAIN";
    tft.getTextBounds(resultMsg, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, CONTENT_Y + 72);
    tft.print(resultMsg);

  } else if (kochWaitingForInput) {
    // Input box (larger and cleaner)
    tft.fillRoundRect(10, CONTENT_Y, SCREEN_WIDTH - 20, 55, 8, 0x1082);
    tft.drawRoundRect(10, CONTENT_Y, SCREEN_WIDTH - 20, 55, 8, 0x34BF);

    // Prompt text above box
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(15, CONTENT_Y + 8);
    tft.print("Type what you heard:");

    // User input (larger text)
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(20, CONTENT_Y + 25);
    tft.print(kochUserInput);

    // Blinking cursor
    int cursorX = 20 + (kochUserInput.length() * 18);
    if (cursorX < SCREEN_WIDTH - 30) {
      tft.fillRect(cursorX, CONTENT_Y + 25, 3, 24, ST77XX_WHITE);
    }

  } else {
    // Ready state (centered, bigger)
    tft.setTextSize(3);
    tft.setTextColor(ST77XX_GREEN);
    String readyMsg = "READY";
    tft.getTextBounds(readyMsg, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, CONTENT_Y + 10);
    tft.print(readyMsg);

    tft.setTextSize(2);
    tft.setTextColor(0x7BEF);
    String startMsg = "Press SPACE";
    tft.getTextBounds(startMsg, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, CONTENT_Y + 45);
    tft.print(startMsg);
  }

  // Footer instructions (non-overlapping, clean)
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);

  if (kochResetHoldActive) {
    // Show reset progress
    unsigned long elapsed = millis() - kochResetHoldStartTime;
    int progress = (elapsed * 100) / 3000;
    tft.setTextColor(ST77XX_RED);
    String resetMsg = "Resetting... " + String(progress) + "%";
    tft.getTextBounds(resetMsg, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 222);
    tft.print(resetMsg);
  } else {
    tft.setCursor(5, 215);
    if (kochCurrentMode == KOCH_MODE_PRACTICE) {
      tft.print("SPACE=Play P=Test C=Chars H=Help");
    } else {
      tft.print("SPACE=Play P=Practice +/-=Lvl H=Help");
    }
    tft.setCursor(5, 227);
    tft.print("S=Settings  ESC=Menu");
  }
}

#endif // TRAINING_KOCH_UI_H
