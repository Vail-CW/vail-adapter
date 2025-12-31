/*
 * Training Mode: Hear It Type It
 * Listen to morse code callsigns and type what you hear
 */

#ifndef TRAINING_HEAR_IT_TYPE_IT_H
#define TRAINING_HEAR_IT_TYPE_IT_H

#include "../core/morse_code.h"
#include <Preferences.h>

// Forward declarations for LVGL feedback functions (defined in lv_training_screens.h)
void showHearItFeedback(bool correct, const String& answer);
void updateHearItScore();
void clearHearItInput();
void scheduleHearItNextCallsign(bool wasCorrect);
void cancelHearItTimers();

// Training mode types
enum HearItMode {
  MODE_CALLSIGNS,
  MODE_RANDOM_LETTERS,
  MODE_RANDOM_NUMBERS,
  MODE_LETTERS_NUMBERS,
  MODE_CUSTOM_CHARS
};

// Training state machine
enum HearItState {
  HEAR_IT_STATE_SETTINGS,   // Showing settings overlay (initial state)
  HEAR_IT_STATE_TRAINING,   // Active training (playing/waiting/feedback)
  HEAR_IT_STATE_STATS       // Showing stats overlay (existing)
};

// Settings screen state machine
enum HearItSettingsScreen {
  SETTINGS_SCREEN_MENU,      // Main settings menu
  SETTINGS_SCREEN_MODE,      // Mode selection (5 cards)
  SETTINGS_SCREEN_SPEED,     // Speed selection (10-40 WPM)
  SETTINGS_SCREEN_CHARS,     // Character selection (grid)
  SETTINGS_SCREEN_LENGTH     // Group length (3-10)
};

// Preset type for character selection
enum PresetType {
  PRESET_NONE,     // No preset selected
  PRESET_KOCH,     // Koch Method lesson
  PRESET_CWA       // CW Academy session
};

// Training settings
struct HearItSettings {
  HearItMode mode;
  int wpm;                // 10-40 WPM (fixed speed)
  int groupLength;        // 3-10 characters
  String customChars;     // For MODE_CUSTOM_CHARS
  PresetType presetType;  // Preset source
  int presetLesson;       // Lesson/session number
};

// Settings screen state (for configuration mode)
struct HearItSettingsState {
  HearItSettingsScreen currentScreen;
  int menuSelection;      // 0-3 for settings menu items
  int modeSelection;      // 0-4 (index into HearItMode enum)
  int speedValue;         // 10-40 WPM
  bool charSelected[36];  // A-Z (26) + 0-9 (10)
  int groupLength;        // 3-10
  int gridCursor;         // 0-35 for character grid navigation
  bool inPresetSelector;  // True when preset modal is open
  int presetSelection;    // Current selection in preset modal
};

// Default settings
HearItSettings hearItSettings = {
  MODE_CALLSIGNS,  // Default mode
  15,              // Default WPM
  1,               // Default group length
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",  // Default custom chars (all)
  PRESET_NONE,     // No preset by default
  1                // Default lesson number
};

// Settings screen state (for configuration mode)
HearItSettingsState settingsState = {
  SETTINGS_SCREEN_MENU,  // Start at settings menu
  0,                     // Menu selection (first item)
  0,                     // Default to callsigns (index 0)
  15,                    // Default 15 WPM
  {false},               // No characters selected initially
  1,                     // Default group length 1
  0,                     // Grid cursor at position 0
  false,                 // Preset selector not open
  0                      // Preset selection index 0
};

// Training state
String currentCallsign = "";
String userInput = "";
int currentWPM = 15;
bool waitingForInput = false;
int attemptsOnCurrentCallsign = 0;
bool inSettingsMode = false;
HearItSettings tempSettings;  // Temporary settings while in settings mode
HearItState currentHearItState = HEAR_IT_STATE_SETTINGS;  // Start in settings state
bool hearItUseLVGL = true;  // When true, skip legacy draw functions (LVGL handles display)

// Stats tracking
struct HearItStats {
  int totalAttempts;
  int totalCorrect;
  unsigned long sessionStartTime;
};
HearItStats sessionStats = {0, 0, 0};
bool inStatsMode = false;

// Load settings from preferences
void loadHearItSettings() {
  Preferences prefs;
  prefs.begin("hear_it", true);  // Read-only

  // Load mode with bounds checking to prevent invalid enum values
  int savedMode = prefs.getInt("mode", MODE_CALLSIGNS);
  if (savedMode < 0 || savedMode > MODE_CUSTOM_CHARS) {
    savedMode = MODE_CALLSIGNS;  // Default to safe value if out of range
  }
  hearItSettings.mode = (HearItMode)savedMode;

  hearItSettings.wpm = prefs.getInt("wpm", 15);
  hearItSettings.groupLength = prefs.getInt("length", 1);
  hearItSettings.customChars = prefs.getString("custom", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
  hearItSettings.presetType = (PresetType)prefs.getInt("presetType", PRESET_NONE);
  hearItSettings.presetLesson = prefs.getInt("presetLesson", 1);
  prefs.end();
}

// Save settings to preferences
void saveHearItSettings() {
  Preferences prefs;
  prefs.begin("hear_it", false);  // Read-write
  prefs.putInt("mode", hearItSettings.mode);
  prefs.putInt("wpm", hearItSettings.wpm);
  prefs.putInt("length", hearItSettings.groupLength);
  prefs.putString("custom", hearItSettings.customChars);
  prefs.putInt("presetType", hearItSettings.presetType);
  prefs.putInt("presetLesson", hearItSettings.presetLesson);
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
  currentWPM = hearItSettings.wpm; // Use saved setting instead of random
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

// Initialize and start Hear It Type It mode (called from lv_mode_integration.h)
void startHearItTypeItMode(LGFX& tft) {
  // Load saved settings
  loadHearItSettings();

  // Enable LVGL mode (skip legacy draw functions)
  hearItUseLVGL = true;

  // Start in SETTINGS state (show configuration first)
  currentHearItState = HEAR_IT_STATE_SETTINGS;
  inSettingsMode = true;
  tempSettings = hearItSettings;  // Copy for editing

  currentWPM = hearItSettings.wpm;
  userInput = "";
  sessionStats = {0, 0, millis()};

  Serial.println("[HearIt] Starting in settings mode (LVGL)");
  Serial.printf("[HearIt] Mode: %d, WPM: %d, Length: %d\n",
                hearItSettings.mode, hearItSettings.wpm, hearItSettings.groupLength);

  // Don't auto-start - wait for user to press ENTER after configuring
}

// Check user's answer
bool checkAnswer() {
  userInput.toUpperCase();
  return userInput.equals(currentCallsign);
}

// Get current settings as a display string for LVGL UI
String getHearItSettingsString() {
  const char* modeNames[] = {"Callsigns", "Letters", "Numbers", "Mixed", "Custom"};
  String result = "Mode: ";
  result += modeNames[tempSettings.mode];
  result += "   WPM: ";
  result += String(tempSettings.wpm);
  result += "   Length: ";
  result += String(tempSettings.groupLength);
  return result;
}

// Forward declaration - defined in main .ino file
void drawHeader();

// Draw just the input box (for fast updates while typing)
void drawInputBox(LGFX& tft) {
  if (hearItUseLVGL) return;  // LVGL handles display
  int boxX = 30;
  int boxY = 120;  // Updated from 125 to match new layout
  int boxW = SCREEN_WIDTH - 60;
  int boxH = 90;  // Increased from 70 to accommodate larger text

  // Clear and redraw the input box
  tft.fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x1082); // Dark blue fill
  tft.drawRoundRect(boxX, boxY, boxW, boxH, 8, 0x34BF); // Light blue outline

  // Show user input with larger font
  tft.setFont(&FreeSansBold18pt7b);  // Larger font for better visibility
  tft.setTextColor(ST77XX_WHITE);

  int16_t x1, y1;
  uint16_t w, h;
  // Measure text bounds for accurate centering
  getTextBounds_compat(tft, userInput.c_str(), 0, 0, &x1, &y1, &w, &h);
  int textX = boxX + 15;
  int textY = boxY + 57;  // Properly centered for 18pt font in 90px box
  tft.setCursor(textX, textY);
  tft.print(userInput);

  // Show blinking cursor
  if ((millis() / 500) % 2 == 0) { // Blink every 500ms
    int cursorX = textX + w + 5;
    tft.fillRect(cursorX, textY - h, 3, h + 5, COLOR_WARNING);
  }
  tft.setFont(nullptr); // Reset font
}

// Draw the Hear It Type It UI
void drawHearItTypeItUI(LGFX& tft) {
  // Skip legacy drawing when using LVGL
  if (hearItUseLVGL) return;

  // Draw header to ensure it's properly sized
  drawHeader();

  // Clear content area (keep header intact)
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  int16_t x1, y1;
  uint16_t w, h;

  // Title area with large, smooth font
  tft.setFont(&FreeSansBold18pt7b);  // Large, smooth font
  tft.setTextColor(COLOR_TEXT_PRIMARY);  // Clean white instead of bright cyan
  getTextBounds_compat(tft, "HEAR IT TYPE IT", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 65);  // Updated Y position
  tft.print("HEAR IT TYPE IT");
  tft.setFont(nullptr); // Reset font

  // Speed indicator with modern styling (COMMENTED OUT - moved to stats overlay)
  // tft.setFont(&FreeSans9pt7b);
  // tft.setTextColor(COLOR_WARNING);
  // String speedText = String(currentWPM) + " WPM";
  // getTextBounds_compat(tft, speedText.c_str(), 0, 0, &x1, &y1, &w, &h);
  // tft.setCursor((SCREEN_WIDTH - w) / 2, 100);
  // tft.print(speedText);
  // tft.setFont(nullptr); // Reset font

  // Main content area
  if (waitingForInput) {
    // Instructions with smooth font
    tft.setFont(&FreeSans9pt7b);  // Smooth font instead of default
    tft.setTextColor(COLOR_TEXT_SECONDARY); // Light gray for labels
    String prompt = "Type what you heard:";
    getTextBounds_compat(tft, prompt.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 100);  // Updated Y position
    tft.print(prompt);
    tft.setFont(nullptr); // Reset font

    // Draw input box
    drawInputBox(tft);

  } else {
    // Playing status
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    String playMsg = "Playing...";  // Shortened for cleaner look
    getTextBounds_compat(tft, playMsg.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 150);  // Centered in content area
    tft.print(playMsg);
    tft.setFont(nullptr); // Reset font
  }

  // Attempt counter (COMMENTED OUT - moved to stats overlay)
  // if (attemptsOnCurrentCallsign > 0) {
  //   tft.setFont(&FreeSans9pt7b);
  //   tft.setTextColor(COLOR_WARNING);
  //   String attemptText = "Attempt " + String(attemptsOnCurrentCallsign + 1);
  //   getTextBounds_compat(tft, attemptText.c_str(), 0, 0, &x1, &y1, &w, &h);
  //   tft.setCursor((SCREEN_WIDTH - w) / 2, 190);
  //   tft.print(attemptText);
  //   tft.setFont(nullptr);
  // }

  // Help text at bottom with smooth font - state-aware
  tft.setFont(&FreeSans9pt7b);  // Smooth font instead of default
  tft.setTextColor(COLOR_WARNING);  // Yellow for better visibility

  String helpText;
  if (currentHearItState == HEAR_IT_STATE_SETTINGS) {
    helpText = "M:Mode  +:Len+  -:Len-  ENTER:Start  ESC:Exit";
  } else {
    helpText = "ENTER Submit   LEFT Replay   TAB Skip   UP Stats   ESC Settings";
  }

  getTextBounds_compat(tft, helpText.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 293);  // Moved up 12 pixels from 305
  tft.print(helpText);
  tft.setFont(nullptr); // Reset font
}

// Draw settings overlay
void drawSettingsOverlay(LGFX& tft) {
  // Skip legacy drawing when using LVGL
  if (hearItUseLVGL) return;

  // Semi-transparent overlay effect (draw dark rectangle)
  tft.fillRect(20, 60, SCREEN_WIDTH - 40, 160, 0x18C3);
  tft.drawRect(20, 60, SCREEN_WIDTH - 40, 160, COLOR_WARNING);

  int16_t x1, y1;
  uint16_t w, h;

  // Title with smooth font
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(ST77XX_WHITE);
  getTextBounds_compat(tft, "SETTINGS", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 85);
  tft.print("SETTINGS");
  tft.setFont(nullptr);

  // Current mode with smooth font
  tft.setFont(&FreeSans9pt7b);
  const char* modeNames[] = {"Callsigns", "Letters", "Numbers", "Let+Num", "Custom"};
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(30, 115);
  tft.print("Mode: ");
  tft.setTextColor(COLOR_WARNING);
  tft.print(modeNames[hearItSettings.mode]);

  // Group length (only for non-callsign modes)
  if (hearItSettings.mode != MODE_CALLSIGNS) {
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(30, 135);
    tft.print("Length: ");
    tft.setTextColor(COLOR_WARNING);
    tft.print(hearItSettings.groupLength);
  }

  // Custom chars preview (only for custom mode)
  if (hearItSettings.mode == MODE_CUSTOM_CHARS) {
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(30, 155);
    tft.print("Chars: ");
    tft.setTextColor(COLOR_WARNING);
    String preview = hearItSettings.customChars.substring(0, 15);
    if (hearItSettings.customChars.length() > 15) preview += "...";
    tft.print(preview);
  }

  // Instructions
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(30, 185);
  tft.print("M:Mode  +:Len+  -:Len-");
  tft.setCursor(30, 205);
  tft.print("C:Custom  ENTER:Save  ESC:Cancel");
  tft.setFont(nullptr);
}

// Draw stats card overlay
void drawStatsCard(LGFX& tft) {
  // Skip legacy drawing when using LVGL
  if (hearItUseLVGL) return;

  // Card-based display (not full modal overlay)
  tft.fillRoundRect(40, 80, SCREEN_WIDTH - 80, 160, 8, 0x1082);  // Dark blue fill
  tft.drawRoundRect(40, 80, SCREEN_WIDTH - 80, 160, 8, COLOR_ACCENT_CYAN);  // Cyan outline

  int16_t x1, y1;
  uint16_t w, h;

  // Title
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  getTextBounds_compat(tft, "STATISTICS", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 110);
  tft.print("STATISTICS");

  // Calculate stats
  int accuracy = (sessionStats.totalAttempts > 0)
    ? (sessionStats.totalCorrect * 100) / sessionStats.totalAttempts
    : 0;
  unsigned long sessionTime = (millis() - sessionStats.sessionStartTime) / 1000;  // seconds
  int minutes = sessionTime / 60;
  int seconds = sessionTime % 60;

  // Stats display
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);

  // Attempts
  tft.setCursor(60, 140);
  tft.print("Attempts: ");
  tft.setTextColor(COLOR_WARNING);
  tft.print(sessionStats.totalCorrect);
  tft.print("/");
  tft.print(sessionStats.totalAttempts);

  // Accuracy
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(60, 165);
  tft.print("Accuracy: ");
  tft.setTextColor(COLOR_SUCCESS);
  tft.print(accuracy);
  tft.print("%");

  // Speed
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(60, 190);
  tft.print("Speed: ");
  tft.setTextColor(COLOR_WARNING);
  tft.print(currentWPM);
  tft.print(" WPM");

  // Session time
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  tft.setCursor(60, 215);
  tft.print("Time: ");
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  char timeStr[10];
  sprintf(timeStr, "%dm %02ds", minutes, seconds);
  tft.print(timeStr);

  // Instructions
  tft.setTextColor(COLOR_TEXT_TERTIARY);
  getTextBounds_compat(tft, "Press ESC to close", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 235);
  tft.print("Press ESC to close");

  tft.setFont(nullptr);
}

// ============================================
// NEW SETTINGS CONFIGURATION UI
// ============================================

// Helper text function for settings screens
void drawSettingsHelperText(LGFX& tft, const char* text) {
  if (hearItUseLVGL) return;  // LVGL handles display
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_WARNING);
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, text, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 22);
  tft.print(text);
  tft.setFont(nullptr);
}

// Settings Menu (choose which setting to configure)
void drawSettingsMenuScreen(LGFX& tft) {
  if (hearItUseLVGL) return;  // LVGL handles display
  const char* menuItems[] = {"Training Mode", "Speed (WPM)", "Characters", "Group Length"};
  const char* menuIcons[] = {"M", "S", "C", "L"};

  // Clear content area
  tft.fillRect(0, 40, SCREEN_WIDTH, SCREEN_HEIGHT - 40, COLOR_BACKGROUND);

  // Draw title
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, "Configure Settings", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 50);
  tft.print("Configure Settings");

  // Draw menu cards (4 items vertically)
  int cardHeight = 38;
  int cardWidth = 360;
  int startY = 75;
  int spacing = 6;

  for (int i = 0; i < 4; i++) {
    int y = startY + i * (cardHeight + spacing);
    int x = (SCREEN_WIDTH - cardWidth) / 2;

    // Card background
    uint16_t bgColor = (i == settingsState.menuSelection) ? COLOR_CARD_CYAN : COLOR_BG_LAYER2;
    uint16_t borderColor = (i == settingsState.menuSelection) ? COLOR_BORDER_ACCENT : COLOR_BORDER_SUBTLE;
    tft.fillRoundRect(x, y, cardWidth, cardHeight, 10, bgColor);
    tft.drawRoundRect(x, y, cardWidth, cardHeight, 10, borderColor);

    // Icon circle
    tft.fillCircle(x + 20, y + 19, 15, COLOR_ACCENT_BLUE);
    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(ST77XX_WHITE);
    getTextBounds_compat(tft, menuIcons[i], 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(x + 20 - w / 2, y + 19 + h / 2 - 22);
    tft.print(menuIcons[i]);

    // Item label
    tft.setFont(&FreeSansBold12pt7b);
    uint16_t textColor = (i == settingsState.menuSelection) ? COLOR_TEXT_PRIMARY : COLOR_TEXT_SECONDARY;
    tft.setTextColor(textColor);
    tft.setCursor(x + 45, y + 13);
    tft.print(menuItems[i]);
  }

  tft.setFont(nullptr);
  drawSettingsHelperText(tft, "UP/DN Navigate   ENTER Select   ESC Save & Exit");
}

// Screen 1: Mode Selection (Card-based)
void drawModeSelectionScreen(LGFX& tft) {
  if (hearItUseLVGL) return;  // LVGL handles display
  const char* modeNames[] = {"Callsigns", "Letters", "Numbers", "Mixed", "Custom"};
  const char* modeIcons[] = {"C", "A", "1", "M", "*"};

  // Clear content area
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, "SELECT MODE", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 55);
  tft.print("SELECT MODE");

  // Draw selected mode card (large, centered)
  int cardX = 40, cardY = 80, cardW = 400, cardH = 80;
  tft.fillRoundRect(cardX, cardY, cardW, cardH, 12, COLOR_CARD_CYAN);
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 12, COLOR_BORDER_ACCENT);

  // Icon circle
  tft.fillCircle(cardX + 50, cardY + 40, 26, COLOR_ACCENT_BLUE);
  tft.drawCircle(cardX + 50, cardY + 40, 26, ST77XX_WHITE);
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(ST77XX_WHITE);
  getTextBounds_compat(tft, modeIcons[settingsState.modeSelection], 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + 50 - w/2, cardY + 40 + h/2 - 24);
  tft.print(modeIcons[settingsState.modeSelection]);

  // Mode name
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setCursor(cardX + 100, cardY + 30);
  tft.print(modeNames[settingsState.modeSelection]);

  tft.setFont(nullptr);
  drawSettingsHelperText(tft, "UP/DN Navigate   ENTER Select   ESC Back");
}

// Screen 2: Speed Selection
void drawSpeedSelectionScreen(LGFX& tft) {
  if (hearItUseLVGL) return;  // LVGL handles display
  // Clear content area
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Card container
  int cardX = 20, cardY = 60, cardW = 440, cardH = 140;
  tft.fillRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BG_LAYER2);
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BORDER_SUBTLE);

  // Label
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setCursor(cardX + 20, cardY + 30);
  tft.print("SPEED (WPM)");

  // Large value
  tft.setTextSize(2);
  tft.setTextColor(COLOR_ACCENT_CYAN);
  char speedStr[10];
  sprintf(speedStr, "%d WPM", settingsState.speedValue);
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, speedStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 70);
  tft.print(speedStr);
  tft.setTextSize(1);

  // Progress bar
  int barX = cardX + 30, barY = cardY + 100, barW = cardW - 60, barH = 14;
  tft.fillRoundRect(barX, barY, barW, barH, 7, COLOR_BG_DEEP);

  // Calculate fill (10-40 WPM range = 30 steps)
  int fillW = ((settingsState.speedValue - 10) * barW) / 30;
  uint16_t fillColor = (settingsState.speedValue > 25) ? COLOR_ACCENT_CYAN : COLOR_CARD_CYAN;
  tft.fillRoundRect(barX, barY, fillW, barH, 7, fillColor);
  tft.drawRoundRect(barX, barY, barW, barH, 7, COLOR_BORDER_LIGHT);

  tft.setFont(nullptr);
  drawSettingsHelperText(tft, "LEFT/RIGHT Adjust   ENTER Next   ESC Back");
}

// Screen 3: Character Grid (for custom mode)
void drawCharacterGrid(LGFX& tft) {
  if (hearItUseLVGL) return;  // LVGL handles display
  // Clear content area
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, "SELECT CHARACTERS", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 53);
  tft.print("SELECT CHARACTERS");

  // Grid: 6x6 (A-Z = 26, 0-9 = 10, total 36)
  int gridX = 45, gridY = 80;
  int cellW = 65, cellH = 35;

  for (int i = 0; i < 36; i++) {
    int row = i / 6, col = i % 6;
    int x = gridX + col * cellW, y = gridY + row * cellH;

    // Character (A-Z, 0-9)
    char ch = (i < 26) ? ('A' + i) : ('0' + i - 26);

    // Highlight cursor position
    bool isCursor = (i == settingsState.gridCursor);
    if (isCursor) {
      tft.drawRoundRect(x - 2, y - 2, cellW + 4, cellH + 4, 4, COLOR_WARNING);
    }

    // Draw checkbox circle
    uint16_t circleColor = settingsState.charSelected[i] ? COLOR_ACCENT_CYAN : COLOR_BORDER_SUBTLE;
    tft.fillCircle(x + 12, y + 17, 10, circleColor);
    tft.drawCircle(x + 12, y + 17, 10, COLOR_BORDER_LIGHT);

    // Draw character label
    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    char charStr[2] = {ch, '\0'};
    getTextBounds_compat(tft, charStr, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor(x + 35, y + 17);
    tft.print(ch);
  }

  // Count selected
  int count = 0;
  for (int i = 0; i < 36; i++) if (settingsState.charSelected[i]) count++;

  // Show count
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);
  char countStr[30];
  sprintf(countStr, "%d characters selected", count);
  getTextBounds_compat(tft, countStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 45);
  tft.print(countStr);

  tft.setFont(nullptr);
  drawSettingsHelperText(tft, "Arrows Move   ENTER Toggle   TAB Presets   ESC Back");
}

// Screen 4: Group Length Selection
void drawGroupLengthScreen(LGFX& tft) {
  if (hearItUseLVGL) return;  // LVGL handles display
  // Clear content area
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Card container
  int cardX = 20, cardY = 60, cardW = 440, cardH = 140;
  tft.fillRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BG_LAYER2);
  tft.drawRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BORDER_SUBTLE);

  // Label
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);
  tft.setCursor(cardX + 20, cardY + 30);
  tft.print("GROUP LENGTH");

  // Large value
  tft.setTextSize(2);
  tft.setTextColor(COLOR_ACCENT_CYAN);
  char lenStr[20];
  sprintf(lenStr, "%d chars", settingsState.groupLength);
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, lenStr, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(cardX + (cardW - w) / 2, cardY + 70);
  tft.print(lenStr);
  tft.setTextSize(1);

  // Visual indicator dots (1 to 8)
  int dotY = cardY + 110;
  int dotSpacing = 52;
  int startX = cardX + (cardW - (7 * dotSpacing)) / 2;

  for (int i = 1; i <= 8; i++) {
    uint16_t dotColor = (i <= settingsState.groupLength) ? COLOR_ACCENT_CYAN : COLOR_BG_DEEP;
    tft.fillCircle(startX + (i - 1) * dotSpacing, dotY, 8, dotColor);
    tft.drawCircle(startX + (i - 1) * dotSpacing, dotY, 8, COLOR_BORDER_LIGHT);
  }

  tft.setFont(nullptr);
  drawSettingsHelperText(tft, "LEFT/RIGHT Adjust   ENTER Save   ESC Back");
}

// Main configuration UI dispatcher
void drawHearItConfigureUI(LGFX& tft) {
  if (hearItUseLVGL) return;  // LVGL handles display
  // Draw header
  drawHeader();

  // Dispatch to appropriate screen
  switch (settingsState.currentScreen) {
    case SETTINGS_SCREEN_MENU:
      drawSettingsMenuScreen(tft);
      break;
    case SETTINGS_SCREEN_MODE:
      drawModeSelectionScreen(tft);
      break;
    case SETTINGS_SCREEN_SPEED:
      drawSpeedSelectionScreen(tft);
      break;
    case SETTINGS_SCREEN_CHARS:
      // Always show character grid when "Characters" is selected from menu
      drawCharacterGrid(tft);
      break;
    case SETTINGS_SCREEN_LENGTH:
      // Skip for callsign mode (auto-save and exit)
      if (settingsState.modeSelection == MODE_CALLSIGNS) {
        // Save settings and signal transition to training
        hearItSettings.mode = (HearItMode)settingsState.modeSelection;
        hearItSettings.wpm = settingsState.speedValue;
        hearItSettings.groupLength = settingsState.groupLength;
        saveHearItSettings();
        // Return code 3 will trigger training mode start in menu_navigation.h
      } else {
        drawGroupLengthScreen(tft);
      }
      break;
  }
}

// Input handler for configuration screens
int handleHearItConfigureInput(char key, LGFX& tft) {
  switch (settingsState.currentScreen) {
    case SETTINGS_SCREEN_MENU:
      // Settings menu input
      if (key == KEY_UP) {
        if (settingsState.menuSelection > 0) {
          settingsState.menuSelection--;
          beep(TONE_MENU_NAV, BEEP_SHORT);
          return 2;  // Redraw
        }
      } else if (key == KEY_DOWN) {
        if (settingsState.menuSelection < 3) {
          settingsState.menuSelection++;
          beep(TONE_MENU_NAV, BEEP_SHORT);
          return 2;  // Redraw
        }
      } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
        // Enter selected setting screen
        switch (settingsState.menuSelection) {
          case 0: settingsState.currentScreen = SETTINGS_SCREEN_MODE; break;
          case 1: settingsState.currentScreen = SETTINGS_SCREEN_SPEED; break;
          case 2: settingsState.currentScreen = SETTINGS_SCREEN_CHARS; break;
          case 3: settingsState.currentScreen = SETTINGS_SCREEN_LENGTH; break;
        }
        beep(TONE_SELECT, BEEP_MEDIUM);
        return 2;  // Redraw
      } else if (key == KEY_ESC) {
        // Save settings and exit to Hear It menu
        saveHearItSettings();
        hearItUseLVGL = false;  // Reset LVGL flag on exit
        beep(TONE_SELECT, BEEP_MEDIUM);
        return -1;  // Exit to menu
      }
      break;

    case SETTINGS_SCREEN_MODE:
      // Mode selection input
      if (key == KEY_UP) {
        if (settingsState.modeSelection > 0) {
          settingsState.modeSelection--;
          beep(TONE_MENU_NAV, BEEP_SHORT);
          return 2;  // Redraw
        }
      } else if (key == KEY_DOWN) {
        if (settingsState.modeSelection < 4) {
          settingsState.modeSelection++;
          beep(TONE_MENU_NAV, BEEP_SHORT);
          return 2;  // Redraw
        }
      } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
        // Save mode selection
        hearItSettings.mode = (HearItMode)settingsState.modeSelection;
        settingsState.currentScreen = SETTINGS_SCREEN_MENU;
        beep(TONE_SELECT, BEEP_MEDIUM);
        return 2;  // Back to menu
      } else if (key == KEY_ESC) {
        settingsState.currentScreen = SETTINGS_SCREEN_MENU;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;  // Back to menu
      }
      break;

    case SETTINGS_SCREEN_SPEED:
      // Speed selection input
      if (key == KEY_LEFT) {
        if (settingsState.speedValue > 10) {
          settingsState.speedValue--;
          beep(TONE_MENU_NAV, BEEP_SHORT);
          return 2;  // Redraw
        }
      } else if (key == KEY_RIGHT) {
        if (settingsState.speedValue < 40) {
          settingsState.speedValue++;
          beep(TONE_MENU_NAV, BEEP_SHORT);
          return 2;  // Redraw
        }
      } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
        // Save speed selection
        hearItSettings.wpm = settingsState.speedValue;
        settingsState.currentScreen = SETTINGS_SCREEN_MENU;
        beep(TONE_SELECT, BEEP_MEDIUM);
        return 2;  // Back to menu
      } else if (key == KEY_ESC) {
        settingsState.currentScreen = SETTINGS_SCREEN_MENU;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;  // Back to menu
      }
      break;

    case SETTINGS_SCREEN_CHARS:
      // Character grid input (only for custom mode)
      if (settingsState.modeSelection == MODE_CUSTOM_CHARS) {
        if (key == KEY_UP) {
          if (settingsState.gridCursor >= 6) {
            settingsState.gridCursor -= 6;
            beep(TONE_MENU_NAV, BEEP_SHORT);
            return 2;  // Redraw
          }
        } else if (key == KEY_DOWN) {
          if (settingsState.gridCursor < 30) {
            settingsState.gridCursor += 6;
            beep(TONE_MENU_NAV, BEEP_SHORT);
            return 2;  // Redraw
          }
        } else if (key == KEY_LEFT) {
          if (settingsState.gridCursor > 0) {
            settingsState.gridCursor--;
            beep(TONE_MENU_NAV, BEEP_SHORT);
            return 2;  // Redraw
          }
        } else if (key == KEY_RIGHT) {
          if (settingsState.gridCursor < 35) {
            settingsState.gridCursor++;
            beep(TONE_MENU_NAV, BEEP_SHORT);
            return 2;  // Redraw
          }
        } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
          // Toggle selection
          settingsState.charSelected[settingsState.gridCursor] =
            !settingsState.charSelected[settingsState.gridCursor];
          beep(TONE_SELECT, BEEP_SHORT);
          return 2;  // Redraw
        } else if (key == KEY_ESC) {
          // Save custom characters and go back to menu
          hearItSettings.customChars = "";
          for (int i = 0; i < 36; i++) {
            if (settingsState.charSelected[i]) {
              char ch = (i < 26) ? ('A' + i) : ('0' + i - 26);
              hearItSettings.customChars += ch;
            }
          }
          // Fallback if no chars selected
          if (hearItSettings.customChars.length() == 0) {
            hearItSettings.customChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
          }
          settingsState.currentScreen = SETTINGS_SCREEN_MENU;
          beep(TONE_MENU_NAV, BEEP_SHORT);
          return 2;  // Back to menu
        }
      }
      break;

    case SETTINGS_SCREEN_LENGTH:
      // Group length input
      if (key == KEY_LEFT) {
        if (settingsState.groupLength > 1) {
          settingsState.groupLength--;
          beep(TONE_MENU_NAV, BEEP_SHORT);
          return 2;  // Redraw
        }
      } else if (key == KEY_RIGHT) {
        if (settingsState.groupLength < 8) {
          settingsState.groupLength++;
          beep(TONE_MENU_NAV, BEEP_SHORT);
          return 2;  // Redraw
        }
      } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
        // Save group length
        hearItSettings.groupLength = settingsState.groupLength;
        settingsState.currentScreen = SETTINGS_SCREEN_MENU;
        beep(TONE_SELECT, BEEP_MEDIUM);
        return 2;  // Back to menu
      } else if (key == KEY_ESC) {
        settingsState.currentScreen = SETTINGS_SCREEN_MENU;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;  // Back to menu
      }
      break;
  }

  return 0;  // No action
}

// ============================================
// END NEW SETTINGS CONFIGURATION UI
// ============================================

// Handle settings mode input
int handleSettingsInput(char key, LGFX& tft) {
  if (key == KEY_ESC) {
    // ESC from settings exits mode entirely back to menu
    hearItUseLVGL = false;  // Reset LVGL flag on exit
    return -1;  // Exit mode

  } else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Check if settings changed
    bool settingsChanged = (hearItSettings.mode != tempSettings.mode) ||
                          (hearItSettings.groupLength != tempSettings.groupLength) ||
                          (hearItSettings.customChars != tempSettings.customChars);

    // Save settings
    hearItSettings = tempSettings;
    saveHearItSettings();

    // Reset stats if settings changed or first start from settings state
    if (settingsChanged) {
      sessionStats.totalAttempts = 0;
      sessionStats.totalCorrect = 0;
      sessionStats.sessionStartTime = millis();
    }

    // Transition to training state
    currentHearItState = HEAR_IT_STATE_TRAINING;
    inSettingsMode = false;

    // Start first challenge
    startNewCallsign();
    drawHearItTypeItUI(tft);
    delay(500);
    playCurrentCallsign();
    drawHearItTypeItUI(tft);

    beep(TONE_SELECT, BEEP_LONG);
    return 2;  // Full redraw

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
int handleHearItTypeItInput(char key, LGFX& tft) {
  // Handle stats mode
  if (inStatsMode) {
    if (key == KEY_ESC) {
      inStatsMode = false;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      return 2;  // Redraw main UI
    }
    return 0;  // Ignore other keys in stats mode
  }

  // Route to settings handler if in settings state
  if (currentHearItState == HEAR_IT_STATE_SETTINGS) {
    return handleSettingsInput(key, tft);
  }

  // Training state input handling below

  // Allow alphanumeric input during playback, block only special keys
  if (!waitingForInput) {
    // Block TAB (skip) and UP (stats) during playback
    if (key == KEY_TAB || key == KEY_UP) {
      return 0;
    }
    // ESC, LEFT (replay), and typing are allowed - fall through
  }

  if (key == KEY_ESC) {
    // ESC during training: Return to settings (preserve stats)
    currentHearItState = HEAR_IT_STATE_SETTINGS;
    inSettingsMode = true;
    tempSettings = hearItSettings;  // Load current settings for editing
    beep(TONE_MENU_NAV, BEEP_SHORT);
    drawHearItTypeItUI(tft);
    drawSettingsOverlay(tft);
    return 2;

  } else if (key == KEY_UP) {
    // Show stats overlay (UP arrow to avoid conflict with typing)
    inStatsMode = true;
    beep(TONE_MENU_NAV, BEEP_SHORT);
    drawHearItTypeItUI(tft);
    drawStatsCard(tft);
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
    sessionStats.totalAttempts++;  // Track session stats

    if (checkAnswer()) {
      // Correct!
      sessionStats.totalCorrect++;  // Track session stats
      beep(TONE_SELECT, BEEP_LONG);

      if (hearItUseLVGL) {
        // LVGL mode - update feedback via LVGL widgets
        showHearItFeedback(true, currentCallsign);
        updateHearItScore();
        userInput = "";       // Clear input state
        clearHearItInput();   // Clear the LVGL textarea
        lv_timer_handler();  // Force immediate render so feedback shows now
        scheduleHearItNextCallsign(true);  // Non-blocking timer for next callsign
        return 2;  // Return immediately, timer handles the rest
      } else {
        // Legacy rendering
        int16_t x1, y1;
        uint16_t w, h;

        // Clear feedback area
        tft.fillRect(0, 200, SCREEN_WIDTH, 80, COLOR_BACKGROUND);

        // Large, smooth "CORRECT!" message
        tft.setFont(&FreeSansBold18pt7b);
        tft.setTextColor(COLOR_SUCCESS);
        getTextBounds_compat(tft, "CORRECT!", 0, 0, &x1, &y1, &w, &h);
        tft.setCursor((SCREEN_WIDTH - w) / 2, 230);
        tft.print("CORRECT!");

        // Show answer with smooth font
        tft.setFont(&FreeSans9pt7b);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        String msg = "The answer was: " + currentCallsign;
        getTextBounds_compat(tft, msg.c_str(), 0, 0, &x1, &y1, &w, &h);
        tft.setCursor((SCREEN_WIDTH - w) / 2, 255);
        tft.print(msg);
        tft.setFont(nullptr);

        delay(2000);

        // Move to next callsign (legacy mode only)
        startNewCallsign();
        drawHearItTypeItUI(tft);
        delay(500);
        playCurrentCallsign();
        drawHearItTypeItUI(tft);
        return 2;
      }

    } else {
      // Wrong!
      beep(TONE_ERROR, BEEP_LONG);

      if (hearItUseLVGL) {
        // LVGL mode - update feedback via LVGL widgets
        showHearItFeedback(false, currentCallsign);
        updateHearItScore();
        userInput = "";  // Clear input now
        clearHearItInput();  // Clear the LVGL textarea
        lv_timer_handler();  // Force immediate render so feedback shows now
        scheduleHearItNextCallsign(false);  // Non-blocking timer for replay
        return 2;  // Return immediately, timer handles the rest
      } else {
        // Legacy rendering
        int16_t x1, y1;
        uint16_t w, h;

        // Clear feedback area
        tft.fillRect(0, 200, SCREEN_WIDTH, 80, COLOR_BACKGROUND);

        // Large, smooth "INCORRECT" message
        tft.setFont(&FreeSansBold18pt7b);
        tft.setTextColor(COLOR_ERROR);
        getTextBounds_compat(tft, "INCORRECT", 0, 0, &x1, &y1, &w, &h);
        tft.setCursor((SCREEN_WIDTH - w) / 2, 230);
        tft.print("INCORRECT");

        // Show retry message with smooth font
        tft.setFont(&FreeSans9pt7b);
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        String msg = "Try again...";
        getTextBounds_compat(tft, msg.c_str(), 0, 0, &x1, &y1, &w, &h);
        tft.setCursor((SCREEN_WIDTH - w) / 2, 255);
        tft.print(msg);
        tft.setFont(nullptr);

        delay(2000);

        // Clear user input and replay (legacy mode only)
        userInput = "";
        drawHearItTypeItUI(tft);
        delay(500);
        playCurrentCallsign();
        drawHearItTypeItUI(tft);
        return 2;
      }
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
        // Only beep if not currently playing morse (visual feedback only during playback)
        if (waitingForInput) {
          beep(TONE_MENU_NAV, BEEP_SHORT);
        }
        return 3; // Input box redraw only
      }
    }
  }

  return 0;
}

// ============================================
// CHARACTER PRESET FUNCTIONS
// ============================================

// Apply Koch Method preset (loads characters up to specified lesson)
void applyKochPreset(int lesson) {
  // External reference to Koch sequence from training_koch_core.h
  extern const char KOCH_SEQUENCE[];

  // Clear all selections
  for (int i = 0; i < 36; i++) settingsState.charSelected[i] = false;

  // Select characters up to lesson index
  for (int i = 0; i < lesson && i < 44; i++) {
    char ch = KOCH_SEQUENCE[i];
    if (ch >= 'A' && ch <= 'Z') {
      settingsState.charSelected[ch - 'A'] = true;
    } else if (ch >= '0' && ch <= '9') {
      settingsState.charSelected[26 + ch - '0'] = true;
    }
  }

  hearItSettings.presetType = PRESET_KOCH;
  hearItSettings.presetLesson = lesson;
}

// Extract unique characters from CW Academy session data
String getCWASessionChars(int session) {
  // This would need access to training_cwa_data.h session arrays
  // For now, return a basic character set based on session number
  // Session 1: A E N T
  // Session 2: + S I O 1 4
  // Session 10: Full alphabet + numbers

  String chars = "";
  switch (session) {
    case 1:  chars = "AENT"; break;
    case 2:  chars = "AENTSIO14"; break;
    case 3:  chars = "AENTSIO14RHD"; break;
    case 4:  chars = "AENTSIO14RHDL25CU"; break;
    case 5:  chars = "AENTSIO14RHDL25CUMW36"; break;
    case 6:  chars = "AENTSIO14RHDL25CUMW36FY"; break;
    case 7:  chars = "AENTSIO14RHDL25CUMW36FYGPQ79"; break;
    case 8:  chars = "AENTSIO14RHDL25CUMW36FYGPQ79BV"; break;
    case 9:  chars = "AENTSIO14RHDL25CUMW36FYGPQ79BVJK08"; break;
    case 10: chars = "AENTSIO14RHDL25CUMW36FYGPQ79BVJK08XZ"; break;
    default: chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; break;
  }
  return chars;
}

// Apply CW Academy preset (loads characters from specified session)
void applyCWAPreset(int session) {
  // Extract unique characters from session data
  String chars = getCWASessionChars(session);

  // Clear all selections
  for (int i = 0; i < 36; i++) settingsState.charSelected[i] = false;

  // Select characters from session
  for (int i = 0; i < chars.length(); i++) {
    char ch = chars[i];
    if (ch >= 'A' && ch <= 'Z') {
      settingsState.charSelected[ch - 'A'] = true;
    } else if (ch >= '0' && ch <= '9') {
      settingsState.charSelected[26 + ch - '0'] = true;
    }
  }

  hearItSettings.presetType = PRESET_CWA;
  hearItSettings.presetLesson = session;
}

#endif // TRAINING_HEAR_IT_TYPE_IT_H
