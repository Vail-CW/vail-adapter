/*
 * Training Mode: Hear It Type It
 * Listen to morse code callsigns and type what you hear
 */

#ifndef TRAINING_HEAR_IT_TYPE_IT_H
#define TRAINING_HEAR_IT_TYPE_IT_H

#include "morse_code.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Preferences.h>

// Training mode types
enum HearItMode {
  MODE_CALLSIGNS,
  MODE_RANDOM_LETTERS,
  MODE_RANDOM_NUMBERS,
  MODE_LETTERS_NUMBERS,
  MODE_CUSTOM_CHARS
};

// Training settings
struct HearItSettings {
  HearItMode mode;
  int groupLength;        // 3-10 characters
  String customChars;     // For MODE_CUSTOM_CHARS
};

// Default settings
HearItSettings hearItSettings = {
  MODE_CALLSIGNS,  // Default to callsigns
  5,               // Default group length
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"  // Default custom chars (all)
};

// Training state
String currentCallsign = "";
String userInput = "";
int currentWPM = 15;
bool waitingForInput = false;
int attemptsOnCurrentCallsign = 0;
bool inSettingsMode = false;
HearItSettings tempSettings;  // Temporary settings while in settings mode

// Load settings from preferences
void loadHearItSettings() {
  Preferences prefs;
  prefs.begin("hear_it", true);  // Read-only
  hearItSettings.mode = (HearItMode)prefs.getInt("mode", MODE_CALLSIGNS);
  hearItSettings.groupLength = prefs.getInt("length", 5);
  hearItSettings.customChars = prefs.getString("custom", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
  prefs.end();
}

// Save settings to preferences
void saveHearItSettings() {
  Preferences prefs;
  prefs.begin("hear_it", false);  // Read-write
  prefs.putInt("mode", hearItSettings.mode);
  prefs.putInt("length", hearItSettings.groupLength);
  prefs.putString("custom", hearItSettings.customChars);
  prefs.end();
}

// Generate random character group based on settings
String generateCharacterGroup() {
  String result = "";

  switch (hearItSettings.mode) {
    case MODE_CALLSIGNS: {
      // Generate a random ham radio callsign
      // US Format: ^[AKNW][A-Z]?[0-9][A-Z]{1,3}$
      // Examples: W1ABC, K4XY, N2Q, KA1ABC, WB4XYZ, etc.

      // First character: A, K, N, or W
      char firstLetters[] = {'A', 'K', 'N', 'W'};
      result += firstLetters[random(0, 4)];

      // Optional: 0 or 1 additional prefix letter (total prefix: 1 or 2 letters)
      if (random(0, 2) == 1) {  // 50% chance of 2-letter prefix
        result += char('A' + random(0, 26));
      }

      // Required: Single digit (0-9)
      result += String(random(0, 10));

      // Required: 1-3 suffix letters
      int suffixLength = random(1, 4);
      for (int i = 0; i < suffixLength; i++) {
        result += char('A' + random(0, 26));
      }
      break;
    }

    case MODE_RANDOM_LETTERS:
      // Generate random letters only
      for (int i = 0; i < hearItSettings.groupLength; i++) {
        result += char('A' + random(0, 26));
      }
      break;

    case MODE_RANDOM_NUMBERS:
      // Generate random numbers only
      for (int i = 0; i < hearItSettings.groupLength; i++) {
        result += char('0' + random(0, 10));
      }
      break;

    case MODE_LETTERS_NUMBERS:
      // Generate random mix of letters and numbers
      for (int i = 0; i < hearItSettings.groupLength; i++) {
        if (random(0, 2) == 0) {
          result += char('A' + random(0, 26));  // Letter
        } else {
          result += char('0' + random(0, 10));  // Number
        }
      }
      break;

    case MODE_CUSTOM_CHARS:
      // Generate from custom character set
      if (hearItSettings.customChars.length() > 0) {
        for (int i = 0; i < hearItSettings.groupLength; i++) {
          int idx = random(0, hearItSettings.customChars.length());
          result += hearItSettings.customChars[idx];
        }
      } else {
        // Fallback if custom chars is empty
        result = "ERROR";
      }
      break;
  }

  return result;
}

// Legacy function name for compatibility
String generateCallsign() {
  return generateCharacterGroup();
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
  String helpText = "ENTER Submit  \x1B Replay  TAB Skip  S Settings  ESC Exit";
  tft.setCursor(10, SCREEN_HEIGHT - 10);
  tft.print(helpText);
}

// Draw settings overlay
void drawSettingsOverlay(Adafruit_ST7789& tft) {
  // Semi-transparent overlay effect (draw dark rectangle)
  tft.fillRect(20, 60, SCREEN_WIDTH - 40, 140, 0x18C3);
  tft.drawRect(20, 60, SCREEN_WIDTH - 40, 140, COLOR_WARNING);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  // Title
  tft.setFont(&FreeSansBold12pt7b);
  tft.setCursor(70, 85);
  tft.print("SETTINGS");
  tft.setFont();

  // Current mode
  const char* modeNames[] = {"Callsigns", "Letters", "Numbers", "Let+Num", "Custom"};
  tft.setCursor(30, 100);
  tft.print("Mode: ");
  tft.setTextColor(COLOR_WARNING);
  tft.print(modeNames[hearItSettings.mode]);

  // Group length (only for non-callsign modes)
  if (hearItSettings.mode != MODE_CALLSIGNS) {
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(30, 115);
    tft.print("Length: ");
    tft.setTextColor(COLOR_WARNING);
    tft.print(hearItSettings.groupLength);
  }

  // Custom chars preview (only for custom mode)
  if (hearItSettings.mode == MODE_CUSTOM_CHARS) {
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(30, 130);
    tft.print("Chars: ");
    tft.setTextColor(COLOR_WARNING);
    String preview = hearItSettings.customChars.substring(0, 15);
    if (hearItSettings.customChars.length() > 15) preview += "...";
    tft.print(preview);
  }

  // Instructions
  tft.setTextColor(0x7BEF);
  tft.setCursor(30, 160);
  tft.print("M:Mode  +:Len+  -:Len-");
  tft.setCursor(30, 175);
  tft.print("C:Custom  ENTER:Save  ESC:Cancel");
}

// Handle settings mode input
int handleSettingsInput(char key, Adafruit_ST7789& tft) {
  if (key == KEY_ESC) {
    // Cancel - restore original settings
    inSettingsMode = false;
    return 2;  // Redraw main UI
  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Save settings
    hearItSettings = tempSettings;
    saveHearItSettings();
    inSettingsMode = false;
    beep(TONE_SELECT, BEEP_LONG);
    return 2;  // Redraw main UI
  } else if (key == 'm' || key == 'M') {
    // Cycle mode
    tempSettings.mode = (HearItMode)((tempSettings.mode + 1) % 5);
    beep(TONE_MENU_NAV, BEEP_SHORT);
    drawHearItTypeItUI(tft);
    drawSettingsOverlay(tft);
    return 0;
  } else if (key == '+' || key == '=') {
    // Increase length
    if (tempSettings.groupLength < 10) {
      tempSettings.groupLength++;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawHearItTypeItUI(tft);
      drawSettingsOverlay(tft);
    }
    return 0;
  } else if (key == '-' || key == '_') {
    // Decrease length
    if (tempSettings.groupLength > 3) {
      tempSettings.groupLength--;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawHearItTypeItUI(tft);
      drawSettingsOverlay(tft);
    }
    return 0;
  }
  // Ignore other keys in settings mode
  return 0;
}

// Handle keyboard input for this mode
// Returns: 0 = continue, -1 = exit mode, 2 = full redraw needed, 3 = input box redraw only
int handleHearItTypeItInput(char key, Adafruit_ST7789& tft) {
  // Handle settings mode
  if (inSettingsMode) {
    return handleSettingsInput(key, tft);
  }

  if (!waitingForInput && key != KEY_ESC && key != KEY_LEFT && key != KEY_TAB && key != 's' && key != 'S') {
    return 0; // Ignore input while playing
  }

  if (key == KEY_ESC) {
    // Exit back to training menu
    return -1;

  } else if (key == 's' || key == 'S') {
    // Open settings
    tempSettings = hearItSettings;  // Copy current settings to temp
    inSettingsMode = true;
    beep(TONE_MENU_NAV, BEEP_SHORT);
    drawHearItTypeItUI(tft);
    drawSettingsOverlay(tft);
    return 0;

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
