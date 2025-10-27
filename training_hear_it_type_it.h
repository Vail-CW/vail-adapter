/*
 * Training Mode: Hear It Type It
 * Listen to morse code callsigns and type what you hear
 */

#ifndef TRAINING_HEAR_IT_TYPE_IT_H
#define TRAINING_HEAR_IT_TYPE_IT_H

#include "morse_code.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Training state
String currentCallsign = "";
String userInput = "";
int currentWPM = 15;
bool waitingForInput = false;
int attemptsOnCurrentCallsign = 0;

// Generate a random ham radio callsign
// US Format: ^[AKNW][A-Z]?[0-9][A-Z]{1,3}$
// Examples: W1ABC, K4XY, N2Q, KA1ABC, WB4XYZ, etc.
String generateCallsign() {
  String callsign = "";

  // First character: A, K, N, or W
  char firstLetters[] = {'A', 'K', 'N', 'W'};
  callsign += firstLetters[random(0, 4)];

  // Optional: 0 or 1 additional prefix letter (total prefix: 1 or 2 letters)
  // Common second letters: A-L for older calls, B-W for newer/club stations
  if (random(0, 2) == 1) {  // 50% chance of 2-letter prefix
    callsign += char('A' + random(0, 26));  // Any letter A-Z
  }

  // Required: Single digit (0-9)
  callsign += String(random(0, 10));

  // Required: 1-3 suffix letters
  int suffixLength = random(1, 4); // 1, 2, or 3 letters
  for (int i = 0; i < suffixLength; i++) {
    callsign += char('A' + random(0, 26));
  }

  return callsign;
}

// Start a new callsign challenge
void startNewCallsign() {
  currentCallsign = generateCallsign();
  userInput = "";
  currentWPM = random(12, 21); // Random speed between 12-20 WPM
  attemptsOnCurrentCallsign = 0;

  Serial.print("New callsign: ");
  Serial.print(currentCallsign);
  Serial.print(" at ");
  Serial.print(currentWPM);
  Serial.println(" WPM");
}

// Play the current callsign
void playCurrentCallsign() {
  waitingForInput = false;

  // Debug output to serial (for troubleshooting/cheating)
  Serial.print(">>> PLAYING CALLSIGN: ");
  Serial.print(currentCallsign);
  Serial.print(" @ ");
  Serial.print(currentWPM);
  Serial.println(" WPM");

  playMorseString(currentCallsign.c_str(), currentWPM);
  waitingForInput = true;
}

// Check user's answer
bool checkAnswer() {
  userInput.toUpperCase();
  return userInput.equals(currentCallsign);
}

// Forward declaration - defined in main .ino file
void drawHeader();

// Draw just the input box (for fast updates while typing)
void drawInputBox(Adafruit_ST7789& tft) {
  int boxX = 30;
  int boxY = 125;
  int boxW = SCREEN_WIDTH - 60;
  int boxH = 50;

  // Clear and redraw the input box
  tft.fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x1082); // Dark blue fill
  tft.drawRoundRect(boxX, boxY, boxW, boxH, 8, 0x34BF); // Light blue outline

  // Show user input with modern font
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  // Center the text vertically in the box
  tft.getTextBounds(userInput.c_str(), 0, 0, &x1, &y1, &w, &h);
  int textX = boxX + 15;
  int textY = boxY + (boxH / 2) + (h / 2) + 5;
  tft.setCursor(textX, textY);
  tft.print(userInput);

  // Show blinking cursor
  if ((millis() / 500) % 2 == 0) { // Blink every 500ms
    int cursorX = textX + w + 5;
    tft.fillRect(cursorX, textY - h, 3, h + 5, COLOR_WARNING);
  }
  tft.setFont(); // Reset font
}

// Draw the Hear It Type It UI
void drawHearItTypeItUI(Adafruit_ST7789& tft) {
  // Draw header to ensure it's properly sized
  drawHeader();

  // Clear content area (keep header intact)
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title area with modern font
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds("HEAR IT TYPE IT", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 75);
  tft.print("HEAR IT TYPE IT");
  tft.setFont(); // Reset font

  // Speed indicator with modern styling
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_WARNING);
  String speedText = String(currentWPM) + " WPM";
  tft.getTextBounds(speedText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 100);
  tft.print(speedText);
  tft.setFont(); // Reset font

  // Main content area
  if (waitingForInput) {
    // Instructions
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF); // Light gray
    String prompt = "Type what you heard:";
    tft.setCursor((SCREEN_WIDTH - prompt.length() * 6) / 2, 115);
    tft.print(prompt);

    // Draw input box
    drawInputBox(tft);

  } else {
    // Playing status
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(0x7BEF);
    String playMsg = "Playing callsign...";
    tft.getTextBounds(playMsg.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 145);
    tft.print(playMsg);
    tft.setFont(); // Reset font
  }

  // Attempt counter if multiple attempts
  if (attemptsOnCurrentCallsign > 0) {
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WARNING);
    String attemptText = "Attempt " + String(attemptsOnCurrentCallsign + 1);
    tft.setCursor((SCREEN_WIDTH - attemptText.length() * 6) / 2, 190);
    tft.print(attemptText);
  }

  // Help text at bottom with modern styling
  tft.setTextColor(0x7BEF);
  tft.setTextSize(1);
  String helpText = "ENTER Submit  \x1B Replay  TAB Skip  ESC Exit";
  tft.setCursor((SCREEN_WIDTH - helpText.length() * 6) / 2, SCREEN_HEIGHT - 10);
  tft.print(helpText);
}

// Handle keyboard input for this mode
// Returns: 0 = continue, -1 = exit mode, 2 = full redraw needed, 3 = input box redraw only
int handleHearItTypeItInput(char key, Adafruit_ST7789& tft) {
  if (!waitingForInput && key != KEY_ESC && key != KEY_LEFT && key != KEY_TAB) {
    return 0; // Ignore input while playing
  }

  if (key == KEY_ESC) {
    // Exit back to training menu
    return -1;

  } else if (key == KEY_LEFT) {
    // Replay the callsign
    beep(TONE_MENU_NAV, BEEP_SHORT);
    drawHearItTypeItUI(tft);
    delay(500);
    playCurrentCallsign();
    drawHearItTypeItUI(tft);
    return 2;

  } else if (key == KEY_TAB) {
    // Skip to next callsign
    beep(TONE_MENU_NAV, BEEP_SHORT);
    startNewCallsign();
    drawHearItTypeItUI(tft);
    delay(500);
    playCurrentCallsign();
    drawHearItTypeItUI(tft);
    return 2;

  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Check answer
    if (userInput.length() == 0) {
      return 0; // Ignore empty input
    }

    attemptsOnCurrentCallsign++;

    if (checkAnswer()) {
      // Correct!
      beep(TONE_SELECT, BEEP_LONG);

      tft.fillRect(0, 140, SCREEN_WIDTH, 60, COLOR_BACKGROUND);
      tft.setFont(&FreeSansBold12pt7b);
      tft.setTextColor(COLOR_SUCCESS);
      tft.setTextSize(1);

      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds("CORRECT!", 0, 0, &x1, &y1, &w, &h);
      tft.setCursor((SCREEN_WIDTH - w) / 2, 175);
      tft.print("CORRECT!");

      tft.setFont();
      tft.setTextSize(1);
      tft.setTextColor(COLOR_TEXT);
      String msg = "The answer was: " + currentCallsign;
      tft.setCursor((SCREEN_WIDTH - msg.length() * 6) / 2, 190);
      tft.print(msg);

      delay(2000);

      // Move to next callsign
      startNewCallsign();
      drawHearItTypeItUI(tft);
      delay(500);
      playCurrentCallsign();
      drawHearItTypeItUI(tft);
      return 2;

    } else {
      // Wrong!
      beep(TONE_ERROR, BEEP_LONG);

      tft.fillRect(0, 140, SCREEN_WIDTH, 60, COLOR_BACKGROUND);
      tft.setFont(&FreeSansBold12pt7b);
      tft.setTextColor(COLOR_ERROR);
      tft.setTextSize(1);

      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds("INCORRECT", 0, 0, &x1, &y1, &w, &h);
      tft.setCursor((SCREEN_WIDTH - w) / 2, 175);
      tft.print("INCORRECT");

      tft.setFont();
      tft.setTextSize(1);
      tft.setTextColor(COLOR_TEXT);
      tft.setCursor(75, 190);
      tft.print("Try again...");

      delay(2000);

      // Clear user input and replay
      userInput = "";
      drawHearItTypeItUI(tft);
      delay(500);
      playCurrentCallsign();
      drawHearItTypeItUI(tft);
      return 2;
    }

  } else if (key == KEY_BACKSPACE) {
    // Remove last character
    if (userInput.length() > 0) {
      userInput.remove(userInput.length() - 1);
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 3; // Input box redraw only
    }

  } else if (key >= 32 && key < 127) {
    // Regular character input
    if (userInput.length() < 10) { // Limit input length
      char c = toupper(key);
      // Only accept alphanumeric
      if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        userInput += c;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 3; // Input box redraw only
      }
    }
  }

  return 0;
}

#endif // TRAINING_HEAR_IT_TYPE_IT_H
