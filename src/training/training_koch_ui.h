/*
 * Koch Method - UI Drawing Functions
 * All visual rendering: main UI, help, settings, character selector
 */

#ifndef TRAINING_KOCH_UI_H
#define TRAINING_KOCH_UI_H

#include "training_koch_core.h"

// Forward declaration
extern void drawHeader();

// ============================================
// Modern Layout Constants
// ============================================
#define KOCH_MARGIN_SCREEN      20    // Screen edge margins
#define KOCH_MARGIN_CARD        15    // Card internal padding
#define KOCH_GAP_LARGE          25    // Between major sections
#define KOCH_GAP_MEDIUM         15    // Between cards
#define KOCH_GAP_SMALL          8     // Within cards

#define KOCH_HEADER_Y           45    // Below status bar
#define KOCH_CONTENT_START      80    // Main content area
#define KOCH_FOOTER_Y           290   // Footer position

// Card Dimensions
#define KOCH_CARD_MAIN_W        220   // Two-card layout width
#define KOCH_CARD_MAIN_H        50    // Standard card height
#define KOCH_CARD_RADIUS        8     // Border radius

// Typography Scale (for reference)
// textSize(5) = 80px+ - Hero text (lesson numbers, character intro)
// textSize(4) = 64px - Primary display (READY, main feedback)
// textSize(3) = 48px - Secondary headings (accuracy %, results)
// textSize(2) = 32px - Body text (labels, character sets)
// textSize(1) = 16px - Fine print ONLY (footer hints, tiny labels)

// ============================================
// Modern Helper Functions
// ============================================

// Draw two balanced stat cards (Progress + Accuracy)
void drawKochStatCards(LGFX& tft, int y) {
  const int CARD_H = KOCH_CARD_MAIN_H;
  const int CARD_W = KOCH_CARD_MAIN_W;
  const int CARD_GAP = 20;
  const int CARD1_X = KOCH_MARGIN_SCREEN;
  const int CARD2_X = CARD1_X + CARD_W + CARD_GAP;

  int16_t x1, y1; uint16_t w, h;

  // LEFT CARD - Progress
  tft.fillRoundRect(CARD1_X, y, CARD_W, CARD_H, KOCH_CARD_RADIUS, COLOR_CARD_TEAL);
  tft.drawRoundRect(CARD1_X, y, CARD_W, CARD_H, KOCH_CARD_RADIUS, COLOR_BORDER_SUBTLE);

  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(CARD1_X + 10, y + 18);
  tft.print("PROGRESS");
  tft.setFont(nullptr);

  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  String progressStr = String(kochProgress.sessionTotal) + "/" + String(KOCH_MIN_ATTEMPTS);
  getTextBounds_compat(tft, progressStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(CARD1_X + 120, y + 18);
  tft.print(progressStr);
  tft.setFont(nullptr);

  // RIGHT CARD - Accuracy
  tft.fillRoundRect(CARD2_X, y, CARD_W, CARD_H, KOCH_CARD_RADIUS, COLOR_CARD_CYAN);
  tft.drawRoundRect(CARD2_X, y, CARD_W, CARD_H, KOCH_CARD_RADIUS, COLOR_BORDER_SUBTLE);

  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(CARD2_X + 10, y + 18);
  tft.print("ACCURACY");
  tft.setFont(nullptr);

  int accuracy = (kochProgress.sessionTotal > 0) ?
                 (kochProgress.sessionCorrect * 100 / kochProgress.sessionTotal) : 0;
  uint16_t accColor = accuracy >= 90 ? COLOR_SUCCESS_PASTEL :
                      accuracy >= 70 ? COLOR_WARNING_PASTEL :
                      COLOR_TEXT_PRIMARY;
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(accColor);
  String accStr = String(accuracy) + "%";
  getTextBounds_compat(tft, accStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(CARD2_X + 130, y + 18);
  tft.print(accStr);
  tft.setFont(nullptr);
}

// Draw character set with word-wrapping (NO truncation)
void drawKochCharacterSet(LGFX& tft, int startY) {
  String charSet = getKochCharacterSet();

  int16_t x1, y1; uint16_t w, h;

  // Draw "CHARACTERS:" label
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(KOCH_MARGIN_SCREEN, startY);
  tft.print("CHARACTERS:");

  // Measure label width to position characters after it
  getTextBounds_compat(tft, "CHARACTERS: ", 0, 0, &x1, &y1, &w, &h);
  tft.setFont(nullptr);

  // Draw characters on same line
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_ACCENT_CYAN);

  int x = KOCH_MARGIN_SCREEN + w + 10;  // Start after label with 10px gap
  int y = startY + 2;  // Slight adjustment for baseline alignment
  int maxWidth = SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN);

  // Dynamically measure character width using TrueType font
  getTextBounds_compat(tft, "M ", 0, 0, &x1, &y1, &w, &h);
  int charWidth = w + 2;

  for (int i = 0; i < charSet.length(); i++) {
    if (x + charWidth > maxWidth) {
      x = KOCH_MARGIN_SCREEN;
      y += 22;
    }
    tft.setCursor(x, y);
    tft.print(charSet[i]);
    tft.print(" ");
    x += charWidth + 4;
  }
  tft.setFont(nullptr);
}

// Draw keyboard shortcut card
void drawKochKeyCard(LGFX& tft, int x, int y, int w, int h, String key, String label) {
  tft.fillRoundRect(x, y, w, h, 6, COLOR_CARD_TEAL);
  tft.drawRoundRect(x, y, w, h, 6, COLOR_BORDER_SUBTLE);

  int16_t x1, y1; uint16_t tw, th;

  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  getTextBounds_compat(tft, key.c_str(), 0, 0, &x1, &y1, &tw, &th);
  tft.setCursor(x + (w - tw)/2, y + 20);
  tft.print(key);
  tft.setFont(nullptr);

  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  getTextBounds_compat(tft, label.c_str(), 0, 0, &x1, &y1, &tw, &th);
  tft.setCursor(x + (w - tw)/2, y + h - 5);
  tft.print(label);
  tft.setFont(nullptr);
}

// ============================================
// Help Screen
// ============================================

void drawKochHelp(LGFX& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  int16_t x1, y1;
  uint16_t w, h;

  // Title
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(COLOR_ACCENT_CYAN);
  String title = "HELP & TIPS";
  getTextBounds_compat(tft, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 68);
  tft.print(title);
  tft.setFont(nullptr);

  if (kochHelpPage == 0) {
    // Page 1: Visual keyboard shortcuts
    const int KEY_W = 90;
    const int KEY_H = 35;
    const int KEY_GAP = 15;
    const int ROW1_Y = 80;
    const int ROW2_Y = 125;

    // Row 1: SPACE, P, G
    drawKochKeyCard(tft, 45, ROW1_Y, KEY_W, KEY_H, "SPACE", "Play");
    drawKochKeyCard(tft, 45 + KEY_W + KEY_GAP, ROW1_Y, KEY_W, KEY_H, "P", "Mode");
    drawKochKeyCard(tft, 45 + 2 * (KEY_W + KEY_GAP), ROW1_Y, KEY_W, KEY_H, "G", "Grid");

    // Row 2: S, +/-, C
    drawKochKeyCard(tft, 45, ROW2_Y, KEY_W, KEY_H, "S", "Settings");
    drawKochKeyCard(tft, 45 + KEY_W + KEY_GAP, ROW2_Y, KEY_W, KEY_H, "+/-", "Lesson");
    drawKochKeyCard(tft, 45 + 2 * (KEY_W + KEY_GAP), ROW2_Y, KEY_W, KEY_H, "C", "Chars");

    // Info card
    const int INFO_Y = 175;
    const int INFO_H = 65;

    tft.fillRoundRect(KOCH_MARGIN_SCREEN, INFO_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN), INFO_H, 10, COLOR_BG_LAYER2);
    tft.drawRoundRect(KOCH_MARGIN_SCREEN, INFO_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN), INFO_H, 10, COLOR_BORDER_SUBTLE);

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(KOCH_MARGIN_SCREEN + 10, INFO_Y + 23);
    tft.print("How it works:");

    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(KOCH_MARGIN_SCREEN + 10, INFO_Y + 38);
    tft.print("1. Start with 2 letters (K, M)");
    tft.setCursor(KOCH_MARGIN_SCREEN + 10, INFO_Y + 51);
    tft.print("2. Practice until 90% accuracy");
    tft.setCursor(KOCH_MARGIN_SCREEN + 10, INFO_Y + 64);
    tft.print("3. Unlock characters one by one");
    tft.setFont(nullptr);

  } else if (kochHelpPage == 1) {
    // Page 2: Why full speed?
    const int CARD_Y = 80;
    const int CARD_H = 145;

    tft.fillRoundRect(KOCH_MARGIN_SCREEN + 10, CARD_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN) - 20, CARD_H, 10, COLOR_BG_LAYER2);
    tft.drawRoundRect(KOCH_MARGIN_SCREEN + 10, CARD_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN) - 20, CARD_H, 10, COLOR_BORDER_SUBTLE);

    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_ACCENT_CYAN);
    String heading = "WHY FULL SPEED?";
    getTextBounds_compat(tft, heading.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, CARD_Y + 28);
    tft.print(heading);
    tft.setFont(nullptr);

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD_Y + 55);
    tft.print("Starting at 20 WPM helps you");
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD_Y + 70);
    tft.print("learn by SOUND, not by counting");
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD_Y + 85);
    tft.print("dits and dahs.");

    tft.setTextColor(COLOR_ACCENT_CYAN);
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD_Y + 105);
    tft.print("Think of it like music:");

    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD_Y + 120);
    tft.print("You recognize a song by its");
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD_Y + 133);
    tft.print("rhythm, not by analyzing each");
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD_Y + 146);
    tft.print("note separately!");
    tft.setFont(nullptr);

  } else {
    // Page 3: Test vs Practice modes explained
    const int CARD1_Y = 80;
    const int CARD2_Y = 155;
    const int CARD_H = 65;

    // Practice mode card
    tft.fillRoundRect(KOCH_MARGIN_SCREEN + 10, CARD1_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN) - 20, CARD_H, 10, COLOR_BG_LAYER2);
    tft.drawRoundRect(KOCH_MARGIN_SCREEN + 10, CARD1_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN) - 20, CARD_H, 10, COLOR_BORDER_SUBTLE);

    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(ST77XX_MAGENTA);
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD1_Y + 23);
    tft.print("PRACTICE MODE");
    tft.setFont(nullptr);

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD1_Y + 45);
    tft.print("Focus on specific characters");
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD1_Y + 60);
    tft.print("Stats not tracked");
    tft.setFont(nullptr);

    // Test mode card
    tft.fillRoundRect(KOCH_MARGIN_SCREEN + 10, CARD2_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN) - 20, CARD_H, 10, COLOR_BG_LAYER2);
    tft.drawRoundRect(KOCH_MARGIN_SCREEN + 10, CARD2_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN) - 20, CARD_H, 10, COLOR_BORDER_SUBTLE);

    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_SUCCESS_PASTEL);
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD2_Y + 23);
    tft.print("TEST MODE");
    tft.setFont(nullptr);

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD2_Y + 45);
    tft.print("Full lesson character set");
    tft.setCursor(KOCH_MARGIN_SCREEN + 20, CARD2_Y + 60);
    tft.print("Must reach 90% to advance");
    tft.setFont(nullptr);
  }

  // Footer with page indicator
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_TERTIARY);
  String footer = "H=Next  ESC=Back  Page " + String(kochHelpPage + 1) + "/3";
  getTextBounds_compat(tft, footer.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 270);
  tft.print(footer);
  tft.setFont(nullptr);
}

// ============================================
// Tutorial/Welcome Screen (Mandatory First Launch)
// ============================================

void drawKochTutorial(LGFX& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  int16_t x1, y1;
  uint16_t w, h;

  if (kochTutorialStep == 0) {
    // Step 1: Welcome
    tft.setFont(&FreeSansBold18pt7b);
    tft.setTextColor(ST77XX_CYAN);
    String title = "KOCH METHOD";
    getTextBounds_compat(tft, title.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 70);
    tft.print(title);
    tft.setFont(nullptr);

    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(ST77XX_WHITE);
    String subtitle = "Learn morse code";
    getTextBounds_compat(tft, subtitle.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 100);
    tft.print(subtitle);

    subtitle = "the smart way!";
    getTextBounds_compat(tft, subtitle.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 120);
    tft.print(subtitle);
    tft.setFont(nullptr);

    // Card with bullet points
    tft.fillRoundRect(20, 135, SCREEN_WIDTH - 40, 105, 8, 0x1082);
    tft.drawRoundRect(20, 135, SCREEN_WIDTH - 40, 105, 8, 0x34BF);

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(30, 153);
    tft.print("How it works:");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(30, 173);
    tft.print("1. Start with 2 letters (K, M)");
    tft.setCursor(30, 188);
    tft.print("2. Practice until 90% accuracy");
    tft.setCursor(30, 203);
    tft.print("3. Unlock one new character");
    tft.setCursor(30, 218);
    tft.print("4. Repeat through all 44!");
    tft.setFont(nullptr);

    // Blinking prompt
    if (millis() % 1000 < 500) {
      tft.setFont(&FreeSansBold12pt7b);
      tft.setTextColor(ST77XX_GREEN);
      String prompt = "Press SPACE";
      getTextBounds_compat(tft, prompt.c_str(), 0, 0, &x1, &y1, &w, &h);
      tft.setCursor((SCREEN_WIDTH - w) / 2, 270);
      tft.print(prompt);
      tft.setFont(nullptr);
    }

  } else if (kochTutorialStep == 1) {
    // Step 2: How it works (detailed)
    tft.setFont(&FreeSansBold18pt7b);
    tft.setTextColor(ST77XX_CYAN);
    String title = "HOW IT WORKS";
    getTextBounds_compat(tft, title.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 68);
    tft.print(title);
    tft.setFont(nullptr);

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(15, 100);
    tft.print("Welcome to Koch Method! I'm your");
    tft.setCursor(15, 115);
    tft.print("morse code learning companion.");

    tft.setCursor(15, 140);
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("Why start at high speed?");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(15, 160);
    tft.print("Learning at full speed (20 WPM)");
    tft.setCursor(15, 175);
    tft.print("from the start prevents bad habits");
    tft.setCursor(15, 190);
    tft.print("like counting dits and dahs.");

    tft.setCursor(15, 210);
    tft.print("You'll learn to recognize each");
    tft.setCursor(15, 225);
    tft.print("character by its sound pattern!");

    tft.setTextColor(0x7BEF);
    tft.setCursor(10, 255);
    tft.print("SPACE=Continue  Step 2/3");
    tft.setFont(nullptr);

  } else {
    // Step 3: Controls
    tft.setFont(&FreeSansBold18pt7b);
    tft.setTextColor(ST77XX_CYAN);
    String title = "CONTROLS";
    getTextBounds_compat(tft, title.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 68);
    tft.print(title);
    tft.setFont(nullptr);

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(ST77XX_WHITE);

    // Card with controls
    tft.fillRoundRect(20, 85, SCREEN_WIDTH - 40, 130, 8, 0x1082);
    tft.drawRoundRect(20, 85, SCREEN_WIDTH - 40, 130, 8, 0x34BF);

    tft.setCursor(30, 105);
    tft.setTextColor(ST77XX_CYAN);
    tft.print("SPACE:");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(" Play morse code");

    tft.setCursor(30, 125);
    tft.setTextColor(ST77XX_CYAN);
    tft.print("Type:");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(" Enter what you heard");

    tft.setCursor(30, 145);
    tft.setTextColor(ST77XX_CYAN);
    tft.print("ENTER:");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(" Submit your answer");

    tft.setCursor(30, 165);
    tft.setTextColor(ST77XX_CYAN);
    tft.print("P:");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(" Switch modes");

    tft.setCursor(30, 185);
    tft.setTextColor(ST77XX_CYAN);
    tft.print("H:");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(" Help");

    tft.setCursor(30, 205);
    tft.setTextColor(ST77XX_CYAN);
    tft.print("ESC:");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(" Exit");
    tft.setFont(nullptr);

    // Ready message
    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(ST77XX_GREEN);
    String ready = "Ready to begin!";
    getTextBounds_compat(tft, ready.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 245);
    tft.print(ready);
    tft.setFont(nullptr);

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(0x7BEF);
    tft.setCursor(10, 270);
    tft.print("SPACE=Start Training  Step 3/3");
    tft.setFont(nullptr);
  }
}

// ============================================
// Character Unlock Grid (Progress Visualization)
// ============================================

void drawCharacterGrid(LGFX& tft, int currentLesson, bool isNewUnlock) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  int16_t x1, y1;
  uint16_t w, h;

  // Title
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(ST77XX_CYAN);
  String title = "CHARACTER PROGRESS";
  getTextBounds_compat(tft, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 63);
  tft.print(title);
  tft.setFont(nullptr);

  // Character grid layout (44 characters: 6 rows x 8 cols)
  const int GRID_START_Y = 75;
  const int BOX_SIZE = 30;
  const int BOX_SPACING = 35;
  const int CHARS_PER_ROW = 8;
  const int GRID_START_X = (SCREEN_WIDTH - (CHARS_PER_ROW * BOX_SPACING)) / 2 + 2;

  tft.setFont(&FreeSansBold12pt7b);

  // Draw all 44 characters in grid
  for (int i = 0; i < KOCH_TOTAL_LESSONS; i++) {
    int row = i / CHARS_PER_ROW;
    int col = i % CHARS_PER_ROW;
    int x = GRID_START_X + (col * BOX_SPACING);
    int y = GRID_START_Y + (row * BOX_SPACING);

    char c = KOCH_SEQUENCE[i];
    bool isUnlocked = (i < currentLesson);
    bool isNew = (i == currentLesson - 1) && isNewUnlock;

    // Draw box with appropriate styling
    if (isNew) {
      // Newly unlocked - yellow glow
      tft.fillRoundRect(x - 2, y - 2, BOX_SIZE + 4, BOX_SIZE + 4, 6, ST77XX_YELLOW);
      tft.fillRoundRect(x, y, BOX_SIZE, BOX_SIZE, 4, ST77XX_GREEN);
      tft.setTextColor(ST77XX_BLACK);
    } else if (isUnlocked) {
      // Unlocked - green fill
      tft.fillRoundRect(x, y, BOX_SIZE, BOX_SIZE, 4, 0x07E0);
      tft.setTextColor(ST77XX_BLACK);
    } else {
      // Locked - gray outline
      tft.drawRoundRect(x, y, BOX_SIZE, BOX_SIZE, 4, 0x4208);
      tft.setTextColor(0x4208);
    }

    // Draw character (centered in box with baseline positioning)
    String charStr = String(c);
    getTextBounds_compat(tft, charStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    int baselineY = y + (BOX_SIZE / 2) + (h / 2) - y1;
    tft.setCursor(x + (BOX_SIZE - w) / 2, baselineY);
    tft.print(c);
  }
  tft.setFont(nullptr);

  // Progress text
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(ST77XX_CYAN);
  String progressText = String(currentLesson) + " of " + String(KOCH_TOTAL_LESSONS) + " characters unlocked!";
  getTextBounds_compat(tft, progressText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 283);
  tft.print(progressText);

  // Footer
  tft.setTextColor(0x7BEF);
  tft.setCursor(10, 303);
  tft.print("Press any key to continue...");
  tft.setFont(nullptr);
}

// ============================================
// New Character Introduction Screen
// ============================================

void drawNewCharacterIntro(LGFX& tft, char newChar) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  int16_t x1, y1;
  uint16_t w, h;

  // Celebration title with pulsing effect
  tft.setFont(&FreeSansBold18pt7b);
  // Pulse the title between yellow and cyan (500ms cycle)
  unsigned long pulseTime = millis() % 1000;
  uint16_t titleColor = (pulseTime < 500) ? ST77XX_YELLOW : ST77XX_CYAN;
  tft.setTextColor(titleColor);
  String celebTitle = "NEW CHARACTER!";
  getTextBounds_compat(tft, celebTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 65);
  tft.print(celebTitle);
  tft.setFont(nullptr);

  // Large character display with pulsing glow effect
  const int CHAR_BOX_Y = 85;
  const int CHAR_BOX_SIZE = 110;
  const int CHAR_BOX_X = (SCREEN_WIDTH - CHAR_BOX_SIZE) / 2;

  // Pulsing yellow glow (oscillate glow size between 2-6 pixels)
  int glowSize = 4 + (pulseTime < 500 ? 0 : 2);
  uint16_t glowColor = (pulseTime < 500) ? ST77XX_YELLOW : 0xFFE0;  // Yellow to gold
  tft.fillRoundRect(CHAR_BOX_X - glowSize, CHAR_BOX_Y - glowSize,
                    CHAR_BOX_SIZE + (glowSize * 2), CHAR_BOX_SIZE + (glowSize * 2),
                    12, glowColor);
  tft.fillRoundRect(CHAR_BOX_X, CHAR_BOX_Y, CHAR_BOX_SIZE, CHAR_BOX_SIZE, 8, ST77XX_GREEN);

  // Display the character (huge) - centered with baseline positioning
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(ST77XX_BLACK);
  String charStr = String(newChar);
  getTextBounds_compat(tft, charStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  int baselineY = CHAR_BOX_Y + (CHAR_BOX_SIZE / 2) + (h / 2) - y1;
  tft.setCursor(CHAR_BOX_X + (CHAR_BOX_SIZE - w) / 2, baselineY);
  tft.print(newChar);
  tft.setFont(nullptr);

  // Morse pattern for this character
  const char* morsePattern = getMorseCode(newChar);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(ST77XX_CYAN);
  String patternStr = String(morsePattern);
  getTextBounds_compat(tft, patternStr.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, CHAR_BOX_Y + CHAR_BOX_SIZE + 28);
  tft.print(patternStr);
  tft.setFont(nullptr);

  // Congratulations message
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(ST77XX_WHITE);
  String msg1 = "You've unlocked '";
  msg1 += newChar;
  msg1 += "'! Listen to its sound:";
  getTextBounds_compat(tft, msg1.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 238);
  tft.print(msg1);

  // Playing indicator
  tft.setTextColor(ST77XX_CYAN);
  String playingMsg = "[Playing morse...]";
  getTextBounds_compat(tft, playingMsg.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 258);
  tft.print(playingMsg);

  // Character count
  int totalUnlocked = kochProgress.currentLesson;
  tft.setTextColor(ST77XX_YELLOW);
  String countMsg = "Now you have " + String(totalUnlocked) + " characters to practice!";
  getTextBounds_compat(tft, countMsg.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 278);
  tft.print(countMsg);

  // Footer
  tft.setTextColor(0x7BEF);
  getTextBounds_compat(tft, "Press SPACE to continue", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 303);
  tft.print("Press SPACE to continue");
  tft.setFont(nullptr);
}

// ============================================
// Character Selection (Practice Mode)
// ============================================

void drawCharacterSelector(LGFX& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  int16_t x1, y1; uint16_t w, h;

  // Title
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(30, 68);
  tft.print("SELECT CHARS");
  tft.setFont(nullptr);

  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 90);
  tft.print("Choose 1-5 chars to practice:");
  tft.setFont(nullptr);

  // Show available characters from current lesson
  String available = getKochCharacterSet();

  // Draw character grid
  int startY = 110;
  int charX = 15;
  int charY = startY;
  int charSpacing = 35;
  int charsPerRow = 8;

  tft.setFont(&FreeSansBold12pt7b);
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

    // Center character in box with baseline positioning
    String charStr = String(c);
    getTextBounds_compat(tft, charStr.c_str(), 0, 0, &x1, &y1, &w, &h);
    int baselineY = charY + 14 + (h / 2) - y1;
    tft.setCursor(charX + (22 - w) / 2, baselineY);
    tft.print(c);

    charX += charSpacing;
    if ((i + 1) % charsPerRow == 0) {
      charX = 15;
      charY += 35;
    }
  }
  tft.setFont(nullptr);

  // Show current selection
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 215);
  tft.print("Selected (");
  tft.print(kochPracticeChars.length());
  tft.print("/5): ");
  tft.setFont(nullptr);

  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(ST77XX_GREEN);
  if (kochPracticeChars.length() > 0) {
    tft.print(kochPracticeChars);
  } else {
    tft.setTextColor(0x7BEF);
    tft.print("(none)");
  }
  tft.setFont(nullptr);

  // Instructions
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(0x7BEF);
  tft.setCursor(10, 242);
  tft.print("Type char to toggle  ENTER=Done");
  tft.setFont(nullptr);
}

// ============================================
// Settings Overlay
// ============================================

void drawKochSettings(LGFX& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  int16_t x1, y1;
  uint16_t w, h;

  // Title (centered, larger)
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(COLOR_ACCENT_CYAN);
  String title = "SETTINGS";
  getTextBounds_compat(tft, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 70);
  tft.print(title);
  tft.setFont(nullptr);

  // Quick Presets Label
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(KOCH_MARGIN_SCREEN, 88);
  tft.print("QUICK PRESETS:");
  tft.setFont(nullptr);

  // Larger preset buttons (4 across, 100x35 each)
  const int PRESET_W = 100;
  const int PRESET_H = 35;
  const int PRESET_GAP = 10;
  const int PRESET_Y = 90;
  const int START_X = 30;

  int presetValues[] = {15, 20, 25, 30};

  for (int i = 0; i < 4; i++) {
    int x = START_X + i * (PRESET_W + PRESET_GAP);
    bool isActive = (kochProgress.wpm == presetValues[i]);

    uint16_t bgColor = isActive ? COLOR_CARD_CYAN : COLOR_BG_LAYER2;
    uint16_t borderColor = isActive ? COLOR_BORDER_ACCENT : COLOR_BORDER_SUBTLE;
    uint16_t textColor = isActive ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY;

    tft.fillRoundRect(x, PRESET_Y, PRESET_W, PRESET_H, 6, bgColor);
    tft.drawRoundRect(x, PRESET_Y, PRESET_W, PRESET_H, 6, borderColor);

    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(textColor);
    String label = String(presetValues[i]) + " WPM";
    getTextBounds_compat(tft, label.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(x + (PRESET_W - w) / 2, PRESET_Y + 23);
    tft.print(label);
    tft.setFont(nullptr);
  }

  // Setting Cards (larger, full-width)
  const int SETTING_Y1 = 140;
  const int SETTING_H = 35;
  const int SETTING_W = SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN);

  // Speed setting card
  bool speedSelected = (kochSettingsSelection == 0);
  uint16_t speedBg = speedSelected ? COLOR_CARD_CYAN : COLOR_BG_LAYER2;
  uint16_t speedBorder = speedSelected ? COLOR_BORDER_ACCENT : COLOR_BORDER_SUBTLE;

  tft.fillRoundRect(KOCH_MARGIN_SCREEN, SETTING_Y1, SETTING_W, SETTING_H, 8, speedBg);
  tft.drawRoundRect(KOCH_MARGIN_SCREEN, SETTING_Y1, SETTING_W, SETTING_H, 8, speedBorder);

  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(speedSelected ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY);
  tft.setCursor(KOCH_MARGIN_SCREEN + 15, SETTING_Y1 + 23);
  tft.print("SPEED: ");
  tft.print(kochProgress.wpm);
  tft.print(" WPM");
  tft.setFont(nullptr);

  // Arrows indicator if selected
  if (speedSelected) {
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_ACCENT_CYAN);
    tft.setCursor(SCREEN_WIDTH - 80, SETTING_Y1 + 25);
    tft.print("[< >]");
    tft.setFont(nullptr);
  }

  // Group length setting card
  const int SETTING_Y2 = SETTING_Y1 + SETTING_H + 15;
  bool lengthSelected = (kochSettingsSelection == 1);
  uint16_t lengthBg = lengthSelected ? COLOR_CARD_CYAN : COLOR_BG_LAYER2;
  uint16_t lengthBorder = lengthSelected ? COLOR_BORDER_ACCENT : COLOR_BORDER_SUBTLE;

  tft.fillRoundRect(KOCH_MARGIN_SCREEN, SETTING_Y2, SETTING_W, SETTING_H, 8, lengthBg);
  tft.drawRoundRect(KOCH_MARGIN_SCREEN, SETTING_Y2, SETTING_W, SETTING_H, 8, lengthBorder);

  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(lengthSelected ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY);
  tft.setCursor(KOCH_MARGIN_SCREEN + 15, SETTING_Y2 + 23);
  tft.print("LENGTH: ");
  tft.print(kochProgress.groupLength);
  tft.print(" chars");
  tft.setFont(nullptr);

  // Arrows indicator if selected
  if (lengthSelected) {
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_ACCENT_CYAN);
    tft.setCursor(SCREEN_WIDTH - 80, SETTING_Y2 + 25);
    tft.print("[< >]");
    tft.setFont(nullptr);
  }

  // Instructions (larger, centered)
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_TERTIARY);
  String instr1 = "1-4=Preset  UP/DN=Navigate  LEFT/RIGHT=Adjust";
  getTextBounds_compat(tft, instr1.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 258);
  tft.print(instr1);

  String instr2 = "ENTER=Save  ESC=Cancel";
  getTextBounds_compat(tft, instr2.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 273);
  tft.print(instr2);
  tft.setFont(nullptr);
}

// ============================================
// Mode Selection Screen (NEW)
// ============================================

void drawKochModeSelection(LGFX& tft) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  int16_t x1, y1;
  uint16_t w, h;

  // Title (centered)
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  String title = "CHOOSE YOUR MODE";
  getTextBounds_compat(tft, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 73);
  tft.print(title);
  tft.setFont(nullptr);

  const int CARD_X = 30;
  const int CARD_W = SCREEN_WIDTH - 60;
  const int CARD1_Y = 80;
  const int CARD_H = 90;

  // Practice Mode Card
  bool practiceSelected = (kochModeSelection == KOCH_MODE_PRACTICE);

  tft.fillRoundRect(CARD_X, CARD1_Y, CARD_W, CARD_H, 10,
                    practiceSelected ? COLOR_CARD_CYAN : COLOR_BG_LAYER2);
  tft.drawRoundRect(CARD_X, CARD1_Y, CARD_W, CARD_H, 10,
                    practiceSelected ? COLOR_BORDER_ACCENT : COLOR_BORDER_SUBTLE);

  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(practiceSelected ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY);
  String practiceTitle = "PRACTICE MODE";
  getTextBounds_compat(tft, practiceTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(CARD_X + (CARD_W - w) / 2, CARD1_Y + 26);
  tft.print(practiceTitle);
  tft.setFont(nullptr);

  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(CARD_X + 15, CARD1_Y + 48);
  tft.print("Practice specific characters");
  tft.setCursor(CARD_X + 15, CARD1_Y + 63);
  tft.print("Stats not tracked");
  tft.setCursor(CARD_X + 15, CARD1_Y + 78);
  tft.print("Select 1-5 characters to focus");
  tft.setFont(nullptr);

  // Test Mode Card
  const int CARD2_Y = CARD1_Y + CARD_H + 10;
  bool testSelected = (kochModeSelection == KOCH_MODE_TEST);

  tft.fillRoundRect(CARD_X, CARD2_Y, CARD_W, CARD_H, 10,
                    testSelected ? COLOR_CARD_CYAN : COLOR_BG_LAYER2);
  tft.drawRoundRect(CARD_X, CARD2_Y, CARD_W, CARD_H, 10,
                    testSelected ? COLOR_BORDER_ACCENT : COLOR_BORDER_SUBTLE);

  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(testSelected ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY);
  String testTitle = "TEST MODE";
  getTextBounds_compat(tft, testTitle.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(CARD_X + (CARD_W - w) / 2, CARD2_Y + 26);
  tft.print(testTitle);
  tft.setFont(nullptr);

  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(CARD_X + 15, CARD2_Y + 48);
  tft.print("Full lesson character set");
  tft.setCursor(CARD_X + 15, CARD2_Y + 63);
  tft.print("Stats tracked, must reach 90%");
  tft.setCursor(CARD_X + 15, CARD2_Y + 78);
  tft.print("Unlock new characters");
  tft.setFont(nullptr);

  // Footer
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_TERTIARY);
  String footer = "UP/DOWN=Select  ENTER=Choose  ESC=Back";
  getTextBounds_compat(tft, footer.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 290);
  tft.print(footer);
  tft.setFont(nullptr);
}

// ============================================
// Main UI Drawing
// ============================================

void drawKochUI(LGFX& tft) {
  // If in tutorial mode, draw tutorial and return (mandatory on first launch)
  if (kochInTutorialMode) {
    drawKochTutorial(tft);
    return;
  }

  // If in mode selection screen, draw it and return
  if (kochInModeSelectionScreen) {
    drawKochModeSelection(tft);
    return;
  }

  // If showing new character intro, draw it and return
  if (kochShowingNewChar) {
    drawNewCharacterIntro(tft, kochNewCharacter);
    return;
  }

  // If showing character grid, draw it and return
  if (kochShowingGrid) {
    drawCharacterGrid(tft, kochProgress.currentLesson, false);
    return;
  }

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

  int16_t x1, y1;
  uint16_t w, h;

  // ============================================
  // HEADER SECTION (Y: 45-80)
  // ============================================

  // Lesson number (left side, textSize 2)
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setCursor(KOCH_MARGIN_SCREEN, 68);
  tft.print("Lesson ");
  tft.print(kochProgress.currentLesson);
  tft.print("/");
  tft.print(KOCH_TOTAL_LESSONS);
  tft.setFont(nullptr);

  // Mode badge (right side, compact)
  uint16_t modeBadgeColor = (kochCurrentMode == KOCH_MODE_PRACTICE) ? ST77XX_MAGENTA : COLOR_SUCCESS_PASTEL;
  String modeText = (kochCurrentMode == KOCH_MODE_PRACTICE) ? "PRACTICE" : "TEST";

  int badgeWidth = (kochCurrentMode == KOCH_MODE_PRACTICE) ? 105 : 65;
  int badgeX = SCREEN_WIDTH - badgeWidth - 10;
  tft.fillRoundRect(badgeX, 45, badgeWidth, 22, 8, modeBadgeColor);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(badgeX + 10, 49);
  tft.print(modeText);
  tft.setFont(nullptr);

  // ============================================
  // CHARACTER SET SECTION (Y: 70-110)
  // Large, word-wrapped, NO truncation
  // ============================================

  drawKochCharacterSet(tft, 95);

  // ============================================
  // STATS CARDS (Y: 125-175)
  // Progress + Accuracy in balanced layout
  // ============================================

  drawKochStatCards(tft, 125);

  // ============================================
  // GUIDANCE MESSAGE (Y: 180-210)
  // Clear instructions on what to do
  // ============================================

  const int MSG_Y = 180;
  const int MSG_H = 30;

  // Generate guidance message based on state
  String guidanceMsg = "";
  uint16_t guidanceColor = COLOR_TEXT_PRIMARY;

  if (kochCurrentMode == KOCH_MODE_TEST) {
    int accuracy = getKochSessionAccuracy();
    int total = getCurrentTotal();

    if (total < KOCH_MIN_ATTEMPTS) {
      guidanceMsg = "Complete " + String(KOCH_MIN_ATTEMPTS - total) + " more to advance";
      guidanceColor = COLOR_ACCENT_CYAN;
    } else if (accuracy >= 90) {
      guidanceMsg = "Ready to advance! Press + for next lesson";
      guidanceColor = COLOR_SUCCESS_PASTEL;
    } else {
      guidanceMsg = "Keep practicing - " + String(90 - accuracy) + "% to go!";
      guidanceColor = COLOR_WARNING_PASTEL;
    }
  } else {
    // Practice mode
    if (kochPracticeChars.length() > 0) {
      guidanceMsg = "Practicing: " + kochPracticeChars + " (Press C to change)";
      guidanceColor = ST77XX_MAGENTA;
    } else {
      guidanceMsg = "Press C to select characters to practice";
      guidanceColor = COLOR_ACCENT_CYAN;
    }
  }

  if (guidanceMsg.length() > 0) {
    tft.fillRoundRect(KOCH_MARGIN_SCREEN, MSG_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN), MSG_H, 8, COLOR_BG_LAYER2);
    tft.drawRoundRect(KOCH_MARGIN_SCREEN, MSG_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN), MSG_H, 8, COLOR_BORDER_SUBTLE);

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(guidanceColor);
    getTextBounds_compat(tft, guidanceMsg.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, MSG_Y + 9);
    tft.print(guidanceMsg);
    tft.setFont(nullptr);
  }

  // ============================================
  // INPUT/FEEDBACK AREA (Y: 220-275)
  // State-dependent display (READY/INPUT/FEEDBACK)
  // ============================================

  const int CONTENT_Y = 220;

  if (kochShowingFeedback) {
    // FEEDBACK STATE: Show sent vs typed
    const int FEEDBACK_H = 55;

    uint16_t feedbackBg = kochCorrectAnswer ? COLOR_SUCCESS_PASTEL : COLOR_ERROR_PASTEL;
    tft.fillRoundRect(KOCH_MARGIN_SCREEN, CONTENT_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN), FEEDBACK_H, 8, feedbackBg);
    tft.drawRoundRect(KOCH_MARGIN_SCREEN, CONTENT_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN), FEEDBACK_H, 8, COLOR_BORDER_SUBTLE);

    // "Sent" label and text
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(KOCH_MARGIN_SCREEN + 10, CONTENT_Y + 21);
    tft.print("Sent:");
    tft.setFont(nullptr);

    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(KOCH_MARGIN_SCREEN + 60, CONTENT_Y + 21);
    tft.print(kochCurrentGroup);
    tft.setFont(nullptr);

    // "You" label and text
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(KOCH_MARGIN_SCREEN + 10, CONTENT_Y + 45);
    tft.print("You:");
    tft.setFont(nullptr);

    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(KOCH_MARGIN_SCREEN + 60, CONTENT_Y + 45);
    tft.print(kochUserInput);
    tft.setFont(nullptr);

  } else if (kochWaitingForInput) {
    // INPUT STATE: Typing area
    const int INPUT_H = 55;

    tft.fillRoundRect(KOCH_MARGIN_SCREEN, CONTENT_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN), INPUT_H, 8, COLOR_BG_LAYER2);
    tft.drawRoundRect(KOCH_MARGIN_SCREEN, CONTENT_Y, SCREEN_WIDTH - (2 * KOCH_MARGIN_SCREEN), INPUT_H, 8, COLOR_BORDER_ACCENT);

    // Only show prompt if user hasn't started typing
    if (kochUserInput.length() == 0) {
      tft.setFont(&FreeSans9pt7b);
      tft.setTextColor(COLOR_TEXT_SECONDARY);
      tft.setCursor(KOCH_MARGIN_SCREEN + 10, CONTENT_Y + 21);
      tft.print("Type what you heard:");
      tft.setFont(nullptr);
    }

    // User input - LARGE
    tft.setFont(&FreeSansBold18pt7b);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    getTextBounds_compat(tft, kochUserInput.c_str(), 0, 0, &x1, &y1, &w, &h);
    int inputY = CONTENT_Y + 13;
    tft.setCursor(KOCH_MARGIN_SCREEN + 10, inputY);
    tft.print(kochUserInput);
    tft.setFont(nullptr);

  } else {
    // READY STATE: Waiting to start
    tft.setFont(&FreeSansBold18pt7b);
    tft.setTextColor(COLOR_SUCCESS_PASTEL);
    String readyMsg = "READY";
    getTextBounds_compat(tft, readyMsg.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, CONTENT_Y + 15);
    tft.print(readyMsg);
    tft.setFont(nullptr);

    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    String startMsg = "Press SPACE";
    getTextBounds_compat(tft, startMsg.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, CONTENT_Y + 45);
    tft.print(startMsg);
    tft.setFont(nullptr);
  }

  // ============================================
  // FOOTER (Y: 275-305)
  // Keyboard shortcuts, centered
  // ============================================

  tft.setFont(nullptr);  // Use smaller default font
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);

  String footer;
  if (kochCurrentMode == KOCH_MODE_PRACTICE) {
    footer = "SPACE=Play  TAB=Test  C=Chars  S=Settings  ESC=Exit";
  } else {
    footer = "SPACE=Play  TAB=Practice  +/-=Lesson  S=Settings  ESC=Exit";
  }

  getTextBounds_compat(tft, footer.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 295);
  tft.print(footer);
  tft.setFont(nullptr);
}

#endif // TRAINING_KOCH_UI_H
