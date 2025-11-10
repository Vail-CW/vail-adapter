/*
 * Menu UI Module
 * Handles all menu rendering (header, footer, menu items, status icons)
 */

#ifndef MENU_UI_H
#define MENU_UI_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "config.h"
#include <Fonts/FreeSansBold12pt7b.h>

// Forward declarations from main file
extern Adafruit_ST7789 tft;
extern int currentSelection;

// Menu mode enum - must match main file
enum MenuMode {
  MODE_MAIN_MENU,
  MODE_TRAINING_MENU,
  MODE_HEAR_IT_TYPE_IT,
  MODE_PRACTICE,
  MODE_CW_ACADEMY_TRACK_SELECT,
  MODE_CW_ACADEMY_SESSION_SELECT,
  MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT,
  MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT,
  MODE_CW_ACADEMY_COPY_PRACTICE,
  MODE_CW_ACADEMY_SENDING_PRACTICE,
  MODE_CW_ACADEMY_QSO_PRACTICE,
  MODE_GAMES_MENU,
  MODE_MORSE_SHOOTER,
  MODE_MORSE_MEMORY,
  MODE_RADIO_MENU,
  MODE_RADIO_OUTPUT,
  MODE_CW_MEMORIES,
  MODE_SETTINGS_MENU,
  MODE_WIFI_SETTINGS,
  MODE_CW_SETTINGS,
  MODE_VOLUME_SETTINGS,
  MODE_CALLSIGN_SETTINGS,
  MODE_VAIL_REPEATER,
  MODE_BLUETOOTH,
  MODE_TOOLS_MENU,
  MODE_QSO_LOGGER_MENU,
  MODE_QSO_LOG_ENTRY,
  MODE_QSO_VIEW_LOGS,
  MODE_QSO_STATISTICS,
  MODE_QSO_LOGGER_SETTINGS,
  MODE_WEB_PRACTICE
};

extern MenuMode currentMode;

// Forward declaration for CW Memories helper function (from radio_cw_memories.h)
bool shouldDrawCWMemoriesList();

// Forward declarations for status bar functions
void drawStatusIcons();

// Forward declarations for mode-specific UI functions
void drawHearItTypeItUI(Adafruit_ST7789& tft);
void drawPracticeUI(Adafruit_ST7789& tft);
void drawCWATrackSelectUI(Adafruit_ST7789& tft);
void drawCWASessionSelectUI(Adafruit_ST7789& tft);
void drawCWAPracticeTypeSelectUI(Adafruit_ST7789& tft);
void drawCWAMessageTypeSelectUI(Adafruit_ST7789& tft);
void drawCWACopyPracticeUI(Adafruit_ST7789& tft);
void drawCWASendingPracticeUI(Adafruit_ST7789& tft);
void drawCWAQSOPracticeUI(Adafruit_ST7789& tft);
void drawMorseShooterUI(Adafruit_ST7789& tft);
void drawMemoryUI(Adafruit_ST7789& tft);
void drawWiFiUI(Adafruit_ST7789& tft);
void drawCWSettingsUI(Adafruit_ST7789& tft);
void drawVolumeDisplay(Adafruit_ST7789& tft);
void drawCallsignUI(Adafruit_ST7789& tft);
void drawVailUI(Adafruit_ST7789& tft);
void drawToolsMenu(Adafruit_ST7789& tft);
void drawQSOLoggerMenu(Adafruit_ST7789& tft);
void drawQSOLogEntryUI(Adafruit_ST7789& tft);
void drawQSOViewLogsUI(Adafruit_ST7789& tft);
void drawQSOStatisticsUI(Adafruit_ST7789& tft);
void drawRadioOutputUI(Adafruit_ST7789& tft);
void drawCWMemoriesUI(Adafruit_ST7789& tft);
void drawWebPracticeUI(Adafruit_ST7789& tft);

// Menu Options and Icons
String mainMenuOptions[MENU_ITEMS] = {
  "Training",
  "Games",
  "Radio",
  "Tools",
  "Settings",
  "WiFi"
};

String mainMenuIcons[MENU_ITEMS] = {
  "T",  // Training
  "G",  // Games
  "R",  // Radio
  "L",  // Tools (Logger)
  "S",  // Settings
  "W"   // WiFi
};

// Training submenu
#define TRAINING_MENU_ITEMS 3
String trainingMenuOptions[TRAINING_MENU_ITEMS] = {
  "Hear It Type It",
  "Practice",
  "CW Academy"
};

String trainingMenuIcons[TRAINING_MENU_ITEMS] = {
  "H",  // Hear It Type It
  "P",  // Practice
  "A"   // CW Academy
};

// Games submenu
#define GAMES_MENU_ITEMS 2
String gamesMenuOptions[GAMES_MENU_ITEMS] = {
  "Morse Shooter",
  "Memory Chain"
};

String gamesMenuIcons[GAMES_MENU_ITEMS] = {
  "M",  // Morse Shooter
  "C"   // Memory Chain
};

// Settings submenu
#define SETTINGS_MENU_ITEMS 4
String settingsMenuOptions[SETTINGS_MENU_ITEMS] = {
  "WiFi Setup",
  "CW Settings",
  "Volume",
  "General"
};

String settingsMenuIcons[SETTINGS_MENU_ITEMS] = {
  "W",  // WiFi Setup
  "C",  // CW Settings
  "V",  // Volume
  "G"   // General
};

// Tools submenu
#define TOOLS_MENU_ITEMS 1
String toolsMenuOptions[TOOLS_MENU_ITEMS] = {
  "QSO Logger"
};

String toolsMenuIcons[TOOLS_MENU_ITEMS] = {
  "Q"   // QSO Logger
};

// QSO Logger submenu
#define QSO_LOGGER_MENU_ITEMS 4
String qsoLoggerMenuOptions[QSO_LOGGER_MENU_ITEMS] = {
  "New Log Entry",
  "View Logs",
  "Statistics",
  "Logger Settings"
};

String qsoLoggerMenuIcons[QSO_LOGGER_MENU_ITEMS] = {
  "N",  // New Log Entry
  "V",  // View Logs
  "S",  // Statistics
  "L"   // Logger Settings
};

// Radio submenu
#define RADIO_MENU_ITEMS 2
String radioMenuOptions[RADIO_MENU_ITEMS] = {
  "Radio Output",
  "CW Memories"
};

String radioMenuIcons[RADIO_MENU_ITEMS] = {
  "O",  // Radio Output
  "M"   // CW Memories
};

/*
 * Draw header bar with title and status icons
 */
void drawHeader() {
  // Draw modern header bar
  tft.fillRect(0, 0, SCREEN_WIDTH, 40, 0x1082); // Dark blue header

  // Draw title based on current mode
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  String title = "VAIL SUMMIT";

  if (currentMode == MODE_TRAINING_MENU) {
    title = "TRAINING";
  } else if (currentMode == MODE_HEAR_IT_TYPE_IT) {
    title = "TRAINING";
  } else if (currentMode == MODE_PRACTICE) {
    title = "PRACTICE";
  } else if (currentMode == MODE_CW_ACADEMY_TRACK_SELECT) {
    title = "CW ACADEMY";
  } else if (currentMode == MODE_CW_ACADEMY_SESSION_SELECT) {
    title = "CW ACADEMY";
  } else if (currentMode == MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT) {
    title = "CW ACADEMY";
  } else if (currentMode == MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT) {
    title = "CW ACADEMY";
  } else if (currentMode == MODE_CW_ACADEMY_COPY_PRACTICE) {
    title = "CW ACADEMY";
  } else if (currentMode == MODE_CW_ACADEMY_SENDING_PRACTICE) {
    title = "CW ACADEMY";
  } else if (currentMode == MODE_CW_ACADEMY_QSO_PRACTICE) {
    title = "CW ACADEMY";
  } else if (currentMode == MODE_GAMES_MENU) {
    title = "GAMES";
  } else if (currentMode == MODE_MORSE_SHOOTER) {
    title = "MORSE SHOOTER";
  } else if (currentMode == MODE_RADIO_MENU) {
    title = "RADIO";
  } else if (currentMode == MODE_RADIO_OUTPUT) {
    title = "RADIO OUTPUT";
  } else if (currentMode == MODE_CW_MEMORIES) {
    title = "CW MEMORIES";
  } else if (currentMode == MODE_SETTINGS_MENU) {
    title = "SETTINGS";
  } else if (currentMode == MODE_WIFI_SETTINGS) {
    title = "WIFI SETUP";
  } else if (currentMode == MODE_CW_SETTINGS) {
    title = "CW SETTINGS";
  } else if (currentMode == MODE_VOLUME_SETTINGS) {
    title = "VOLUME";
  } else if (currentMode == MODE_CALLSIGN_SETTINGS) {
    title = "GENERAL";
  } else if (currentMode == MODE_VAIL_REPEATER) {
    title = "VAIL CHAT";
  } else if (currentMode == MODE_TOOLS_MENU) {
    title = "TOOLS";
  } else if (currentMode == MODE_QSO_LOGGER_MENU) {
    title = "QSO LOGGER";
  } else if (currentMode == MODE_QSO_LOG_ENTRY) {
    title = "NEW LOG";
  } else if (currentMode == MODE_QSO_VIEW_LOGS) {
    title = "VIEW LOGS";
  } else if (currentMode == MODE_QSO_STATISTICS) {
    title = "STATISTICS";
  } else if (currentMode == MODE_QSO_LOGGER_SETTINGS) {
    title = "LOGGER SETTINGS";
  }

  tft.setCursor(10, 27); // Left-justified
  tft.print(title);
  tft.setFont(); // Reset to default font

  // Draw status icons
  drawStatusIcons();

  // Draw subtle shadow line under header
  tft.drawLine(0, 40, SCREEN_WIDTH, 40, 0x2104);
  tft.drawLine(0, 41, SCREEN_WIDTH, 41, 0x0861);
}

/*
 * Draw footer with help text
 */
void drawFooter() {
  // Draw modern footer with instructions (single line centered in yellow)
  int footerY = SCREEN_HEIGHT - 12;
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING); // Yellow

  String helpText;
  if (currentMode == MODE_MAIN_MENU) {
    helpText = "\x18\x19 Navigate  ENTER Select  ESC x3 Sleep";
  } else {
    helpText = "\x18\x19 Navigate  ENTER Select  ESC Back";
  }

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, footerY);
  tft.print(helpText);
}

/*
 * Draw menu items in carousel/stack card design
 */
void drawMenuItems(String options[], String icons[], int numItems) {
  // Clear only the menu area (between header and footer)
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42 - 20, COLOR_BACKGROUND);

  // Draw menu items with carousel/stack design
  // Main selected card (larger, using more screen space)
  int mainCardWidth = 300;
  int mainCardHeight = 60;
  int mainCardX = (SCREEN_WIDTH - mainCardWidth) / 2;
  int mainCardY = 85; // Moved down to avoid clipping with header

  // Draw the selected card (large and prominent)
  tft.fillRoundRect(mainCardX, mainCardY, mainCardWidth, mainCardHeight, 8, 0x249F); // Blue accent
  tft.drawRoundRect(mainCardX, mainCardY, mainCardWidth, mainCardHeight, 8, 0x34BF); // Lighter outline

  // Draw icon circle for selected
  tft.fillCircle(mainCardX + 30, mainCardY + 30, 20, 0x34BF);
  tft.drawCircle(mainCardX + 30, mainCardY + 30, 20, ST77XX_WHITE); // White outline
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(mainCardX + 23, mainCardY + 20); // Letter centered in circle
  tft.print(icons[currentSelection]);

  // Draw menu text for selected (slightly larger)
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(mainCardX + 65, mainCardY + 22);
  tft.print(options[currentSelection]);

  // Draw selection arrow
  tft.fillTriangle(mainCardX + mainCardWidth - 20, mainCardY + 25,
                   mainCardX + mainCardWidth - 20, mainCardY + 35,
                   mainCardX + mainCardWidth - 10, mainCardY + 30, ST77XX_WHITE);

  // Draw stacked cards underneath (previous items)
  int stackCardWidth = 270;
  int stackCardHeight = 24;
  int stackCardX = (SCREEN_WIDTH - stackCardWidth) / 2;
  int stackOffset = 10;

  // Draw card below (next item in list)
  if (currentSelection < numItems - 1) {
    int stackY1 = mainCardY + mainCardHeight + stackOffset;
    tft.fillRoundRect(stackCardX, stackY1, stackCardWidth, stackCardHeight, 6, 0x2104);

    // Draw small circle for icon
    tft.drawCircle(stackCardX + 12, stackY1 + 12, 8, 0x4208);
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(stackCardX + 10, stackY1 + 9); // Letter centered in circle
    tft.print(icons[currentSelection + 1]);

    // Draw text
    tft.setCursor(stackCardX + 28, stackY1 + 8);
    tft.print(options[currentSelection + 1]);
  }

  // Draw card further below (next+1 item)
  if (currentSelection < numItems - 2) {
    int stackY2 = mainCardY + mainCardHeight + stackOffset + stackCardHeight + 6;
    int stackCardWidth2 = 250;
    int stackCardX2 = (SCREEN_WIDTH - stackCardWidth2) / 2;
    tft.fillRoundRect(stackCardX2, stackY2, stackCardWidth2, 18, 4, 0x1082);

    // Draw small circle for icon
    tft.drawCircle(stackCardX2 + 10, stackY2 + 9, 6, 0x3186);
    tft.setTextSize(1);
    tft.setTextColor(0x5AEB);
    tft.setCursor(stackCardX2 + 8, stackY2 + 6); // Letter centered in circle
    tft.print(icons[currentSelection + 2]);

    // Draw text
    tft.setCursor(stackCardX2 + 22, stackY2 + 5);
    tft.print(options[currentSelection + 2]);
  }

  // Draw card above (previous item in list)
  if (currentSelection > 0) {
    int stackY0 = mainCardY - stackCardHeight - stackOffset;
    tft.fillRoundRect(stackCardX, stackY0, stackCardWidth, stackCardHeight, 6, 0x2104);

    // Draw small circle for icon
    tft.drawCircle(stackCardX + 12, stackY0 + 12, 8, 0x4208);
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(stackCardX + 10, stackY0 + 9); // Letter centered in circle
    tft.print(icons[currentSelection - 1]);

    // Draw text
    tft.setCursor(stackCardX + 28, stackY0 + 8);
    tft.print(options[currentSelection - 1]);
  }
}

/*
 * Main menu draw dispatcher
 */
void drawMenu() {
  tft.fillScreen(COLOR_BACKGROUND);

  drawHeader();

  // Draw footer (only for menu modes)
  if (currentMode == MODE_MAIN_MENU || currentMode == MODE_TRAINING_MENU ||
      currentMode == MODE_GAMES_MENU || currentMode == MODE_RADIO_MENU ||
      currentMode == MODE_SETTINGS_MENU || currentMode == MODE_TOOLS_MENU ||
      currentMode == MODE_QSO_LOGGER_MENU) {
    drawFooter();
  }

  // Draw menu items or mode-specific UI
  if (currentMode == MODE_MAIN_MENU) {
    drawMenuItems(mainMenuOptions, mainMenuIcons, MENU_ITEMS);
  } else if (currentMode == MODE_TRAINING_MENU) {
    drawMenuItems(trainingMenuOptions, trainingMenuIcons, TRAINING_MENU_ITEMS);
  } else if (currentMode == MODE_GAMES_MENU) {
    drawMenuItems(gamesMenuOptions, gamesMenuIcons, GAMES_MENU_ITEMS);
  } else if (currentMode == MODE_RADIO_MENU) {
    drawMenuItems(radioMenuOptions, radioMenuIcons, RADIO_MENU_ITEMS);
  } else if (currentMode == MODE_SETTINGS_MENU) {
    drawMenuItems(settingsMenuOptions, settingsMenuIcons, SETTINGS_MENU_ITEMS);
  } else if (currentMode == MODE_TOOLS_MENU) {
    drawMenuItems(toolsMenuOptions, toolsMenuIcons, TOOLS_MENU_ITEMS);
  } else if (currentMode == MODE_QSO_LOGGER_MENU) {
    drawMenuItems(qsoLoggerMenuOptions, qsoLoggerMenuIcons, QSO_LOGGER_MENU_ITEMS);
  } else if (currentMode == MODE_QSO_LOG_ENTRY) {
    drawQSOLogEntryUI(tft);
  } else if (currentMode == MODE_QSO_VIEW_LOGS) {
    drawQSOViewLogsUI(tft);
  } else if (currentMode == MODE_QSO_STATISTICS) {
    drawQSOStatisticsUI(tft);
  } else if (currentMode == MODE_HEAR_IT_TYPE_IT) {
    drawHearItTypeItUI(tft);
  } else if (currentMode == MODE_PRACTICE) {
    drawPracticeUI(tft);
  } else if (currentMode == MODE_CW_ACADEMY_TRACK_SELECT) {
    drawCWATrackSelectUI(tft);
  } else if (currentMode == MODE_CW_ACADEMY_SESSION_SELECT) {
    drawCWASessionSelectUI(tft);
  } else if (currentMode == MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT) {
    drawCWAPracticeTypeSelectUI(tft);
  } else if (currentMode == MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT) {
    drawCWAMessageTypeSelectUI(tft);
  } else if (currentMode == MODE_CW_ACADEMY_COPY_PRACTICE) {
    drawCWACopyPracticeUI(tft);
  } else if (currentMode == MODE_CW_ACADEMY_SENDING_PRACTICE) {
    drawCWASendingPracticeUI(tft);
  } else if (currentMode == MODE_CW_ACADEMY_QSO_PRACTICE) {
    drawCWAQSOPracticeUI(tft);
  } else if (currentMode == MODE_MORSE_SHOOTER) {
    drawMorseShooterUI(tft);
  } else if (currentMode == MODE_MORSE_MEMORY) {
    drawMemoryUI(tft);
  } else if (currentMode == MODE_RADIO_OUTPUT) {
    drawRadioOutputUI(tft);
  } else if (currentMode == MODE_CW_MEMORIES) {
    // CW Memories has multiple UI states - only redraw main list if not in submenu
    if (shouldDrawCWMemoriesList()) {
      drawCWMemoriesUI(tft);
    }
    // Otherwise, the active state (context menu, edit screen, delete confirm) is already showing
  } else if (currentMode == MODE_WIFI_SETTINGS) {
    drawWiFiUI(tft);
  } else if (currentMode == MODE_CW_SETTINGS) {
    drawCWSettingsUI(tft);
  } else if (currentMode == MODE_VOLUME_SETTINGS) {
    drawVolumeDisplay(tft);
  } else if (currentMode == MODE_CALLSIGN_SETTINGS) {
    drawCallsignUI(tft);
  } else if (currentMode == MODE_VAIL_REPEATER) {
    drawVailUI(tft);
  }
}

#endif // MENU_UI_H
