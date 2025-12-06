/*
 * Menu UI Module
 * Handles all menu rendering (header, footer, menu items, status icons)
 */

#ifndef MENU_UI_H
#define MENU_UI_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "../core/config.h"

// Forward declarations from main file
extern LGFX tft;
extern int currentSelection;

// Menu mode enum - must match main file
enum MenuMode {
  MODE_MAIN_MENU,
  MODE_TRAINING_MENU,
  MODE_HEAR_IT_TYPE_IT,
  MODE_PRACTICE,
  MODE_KOCH_METHOD,
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
  MODE_DEVICE_SETTINGS_MENU,
  MODE_WIFI_SUBMENU,
  MODE_GENERAL_SUBMENU,
  MODE_WIFI_SETTINGS,
  MODE_CW_SETTINGS,
  MODE_VOLUME_SETTINGS,
  MODE_BRIGHTNESS_SETTINGS,
  MODE_CALLSIGN_SETTINGS,
  MODE_WEB_PASSWORD_SETTINGS,
  MODE_VAIL_REPEATER,
  MODE_BLUETOOTH_MENU,
  MODE_BT_HID,
  MODE_BT_MIDI,
  MODE_TOOLS_MENU,
  MODE_QSO_LOGGER_MENU,
  MODE_QSO_LOG_ENTRY,
  MODE_QSO_VIEW_LOGS,
  MODE_QSO_STATISTICS,
  MODE_QSO_LOGGER_SETTINGS,
  MODE_WEB_PRACTICE,
  MODE_WEB_MEMORY_CHAIN,
  MODE_WEB_HEAR_IT,
  // New menu structure
  MODE_CW_MENU,
  MODE_HAM_TOOLS_MENU,
  // Placeholder modes (Coming Soon)
  MODE_BAND_PLANS,
  MODE_PROPAGATION,
  MODE_ANTENNAS,
  MODE_LICENSE_STUDY,
  MODE_SUMMIT_CHAT,
  // Device Bluetooth submenu
  MODE_DEVICE_BT_SUBMENU,
  MODE_BT_KEYBOARD_SETTINGS
};

extern MenuMode currentMode;

// Forward declaration for CW Memories helper function (from radio_cw_memories.h)
bool shouldDrawCWMemoriesList();

// Forward declarations for status bar functions
void drawStatusIcons();

// Forward declarations for mode-specific UI functions
void drawHearItTypeItUI(LGFX& tft);
void drawPracticeUI(LGFX& tft);
void drawCWATrackSelectUI(LGFX& tft);
void drawCWASessionSelectUI(LGFX& tft);
void drawCWAPracticeTypeSelectUI(LGFX& tft);
void drawCWAMessageTypeSelectUI(LGFX& tft);
void drawCWACopyPracticeUI(LGFX& tft);
void drawCWASendingPracticeUI(LGFX& tft);
void drawCWAQSOPracticeUI(LGFX& tft);
void drawMorseShooterUI(LGFX& tft);
void drawMemoryUI(LGFX& tft);
void drawWiFiUI(LGFX& tft);
void drawCWSettingsUI(LGFX& tft);
void drawVolumeDisplay(LGFX& tft);
void drawBrightnessDisplay(LGFX& tft);
void drawCallsignUI(LGFX& tft);
void drawWebPasswordUI(LGFX& tft);
void drawVailUI(LGFX& tft);
void drawToolsMenu(LGFX& tft);
void drawQSOLoggerMenu(LGFX& tft);
void drawQSOLogEntryUI(LGFX& tft);
void drawQSOViewLogsUI(LGFX& tft);
void drawQSOStatisticsUI(LGFX& tft);
void drawRadioOutputUI(LGFX& tft);
void drawCWMemoriesUI(LGFX& tft);
void drawWebPracticeUI(LGFX& tft);
void drawKochUI(LGFX& tft);
void drawBTHIDUI(LGFX& tft);
void drawBTMIDIUI(LGFX& tft);
void drawBTKeyboardSettingsUI(LGFX& tft);

// Menu Options and Icons
// Main menu now has 4 items: CW, Games, Ham Tools, Settings
#define MAIN_MENU_ITEMS 4
String mainMenuOptions[MAIN_MENU_ITEMS] = {
  "CW",
  "Games",
  "Ham Tools",
  "Settings"
};

String mainMenuIcons[MAIN_MENU_ITEMS] = {
  "C",  // CW
  "G",  // Games
  "H",  // Ham Tools
  "S"   // Settings
};

// CW submenu - includes Bluetooth above Radio
#define CW_MENU_ITEMS 6
String cwMenuOptions[CW_MENU_ITEMS] = {
  "Training",
  "Practice",
  "Vail Repeater",
  "Bluetooth",
  "Radio Output",
  "CW Memories"
};

String cwMenuIcons[CW_MENU_ITEMS] = {
  "T",  // Training
  "P",  // Practice
  "V",  // Vail Repeater
  "B",  // Bluetooth
  "R",  // Radio Output
  "M"   // CW Memories
};

// Bluetooth submenu
#define BLUETOOTH_MENU_ITEMS 2
String bluetoothMenuOptions[BLUETOOTH_MENU_ITEMS] = {
  "HID (Keyboard)",
  "MIDI"
};

String bluetoothMenuIcons[BLUETOOTH_MENU_ITEMS] = {
  "K",  // Keyboard (HID)
  "M"   // MIDI
};

// Training submenu
#define TRAINING_MENU_ITEMS 4
String trainingMenuOptions[TRAINING_MENU_ITEMS] = {
  "Hear It Type It",
  "Practice",
  "Koch Method",
  "CW Academy"
};

String trainingMenuIcons[TRAINING_MENU_ITEMS] = {
  "H",  // Hear It Type It
  "P",  // Practice
  "K",  // Koch Method
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

// Settings submenu (top level)
#define SETTINGS_MENU_ITEMS 2
String settingsMenuOptions[SETTINGS_MENU_ITEMS] = {
  "Device Settings",
  "CW Settings"
};

String settingsMenuIcons[SETTINGS_MENU_ITEMS] = {
  "D",  // Device Settings
  "C"   // CW Settings
};

// Device Settings submenu
#define DEVICE_SETTINGS_MENU_ITEMS 3
String deviceSettingsMenuOptions[DEVICE_SETTINGS_MENU_ITEMS] = {
  "WiFi",
  "General",
  "Bluetooth"
};

String deviceSettingsMenuIcons[DEVICE_SETTINGS_MENU_ITEMS] = {
  "W",  // WiFi
  "G",  // General
  "B"   // Bluetooth
};

// Device Bluetooth submenu
#define DEVICE_BT_SUBMENU_ITEMS 1
String deviceBTSubmenuOptions[DEVICE_BT_SUBMENU_ITEMS] = {
  "External Keyboard"
};

String deviceBTSubmenuIcons[DEVICE_BT_SUBMENU_ITEMS] = {
  "K"   // Keyboard
};

// WiFi submenu
#define WIFI_SUBMENU_ITEMS 2
String wifiSubmenuOptions[WIFI_SUBMENU_ITEMS] = {
  "WiFi Setup",
  "Web Password"
};

String wifiSubmenuIcons[WIFI_SUBMENU_ITEMS] = {
  "S",  // WiFi Setup
  "P"   // Web Password
};

// General submenu
#define GENERAL_SUBMENU_ITEMS 3
String generalSubmenuOptions[GENERAL_SUBMENU_ITEMS] = {
  "Callsign",
  "Volume",
  "Brightness"
};

String generalSubmenuIcons[GENERAL_SUBMENU_ITEMS] = {
  "C",  // Callsign
  "V",  // Volume
  "B"   // Brightness
};

// Ham Tools submenu (renamed from Tools, expanded)
#define HAM_TOOLS_MENU_ITEMS 6
String hamToolsMenuOptions[HAM_TOOLS_MENU_ITEMS] = {
  "QSO Logger",
  "Band Plans",
  "Propagation",
  "Antennas",
  "License Study",
  "Summit Chat"
};

String hamToolsMenuIcons[HAM_TOOLS_MENU_ITEMS] = {
  "Q",  // QSO Logger
  "B",  // Band Plans
  "P",  // Propagation
  "A",  // Antennas
  "L",  // License Study
  "C"   // Summit Chat
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

// Radio menu removed - items now in CW menu

/*
 * Draw header bar with title and status icons
 */
void drawHeader() {
  // Draw modern header bar
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, 0x1082); // Dark blue header

  // Draw title based on current mode using smooth font
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  String title = "VAIL SUMMIT";

  if (currentMode == MODE_TRAINING_MENU) {
    title = "TRAINING";
  } else if (currentMode == MODE_HEAR_IT_TYPE_IT) {
    title = "TRAINING";
  } else if (currentMode == MODE_PRACTICE) {
    title = "PRACTICE";
  } else if (currentMode == MODE_KOCH_METHOD) {
    title = "KOCH METHOD";
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
  } else if (currentMode == MODE_DEVICE_SETTINGS_MENU) {
    title = "DEVICE SETTINGS";
  } else if (currentMode == MODE_WIFI_SUBMENU) {
    title = "WIFI";
  } else if (currentMode == MODE_GENERAL_SUBMENU) {
    title = "GENERAL";
  } else if (currentMode == MODE_WIFI_SETTINGS) {
    title = "WIFI SETUP";
  } else if (currentMode == MODE_CW_SETTINGS) {
    title = "CW SETTINGS";
  } else if (currentMode == MODE_VOLUME_SETTINGS) {
    title = "VOLUME";
  } else if (currentMode == MODE_BRIGHTNESS_SETTINGS) {
    title = "BRIGHTNESS";
  } else if (currentMode == MODE_CALLSIGN_SETTINGS) {
    title = "CALLSIGN";
  } else if (currentMode == MODE_WEB_PASSWORD_SETTINGS) {
    title = "WEB PASSWORD";
  } else if (currentMode == MODE_VAIL_REPEATER) {
    title = "VAIL CHAT";
  } else if (currentMode == MODE_BLUETOOTH_MENU) {
    title = "BLUETOOTH";
  } else if (currentMode == MODE_BT_HID) {
    title = "BT HID";
  } else if (currentMode == MODE_BT_MIDI) {
    title = "BT MIDI";
  } else if (currentMode == MODE_DEVICE_BT_SUBMENU) {
    title = "BLUETOOTH";
  } else if (currentMode == MODE_BT_KEYBOARD_SETTINGS) {
    title = "BT KEYBOARD";
  } else if (currentMode == MODE_CW_MENU) {
    title = "CW";
  } else if (currentMode == MODE_HAM_TOOLS_MENU) {
    title = "HAM TOOLS";
  } else if (currentMode == MODE_BAND_PLANS) {
    title = "BAND PLANS";
  } else if (currentMode == MODE_PROPAGATION) {
    title = "PROPAGATION";
  } else if (currentMode == MODE_ANTENNAS) {
    title = "ANTENNAS";
  } else if (currentMode == MODE_LICENSE_STUDY) {
    title = "LICENSE STUDY";
  } else if (currentMode == MODE_SUMMIT_CHAT) {
    title = "SUMMIT CHAT";
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

  tft.setCursor(15, 22); // Left-justified, vertically centered in 60px header
  tft.print(title);
  tft.setFont(nullptr); // Reset to default font for status icons

  // Draw status icons
  drawStatusIcons();

  // Draw subtle shadow line under header
  tft.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH, HEADER_HEIGHT, 0x2104);
  tft.drawLine(0, HEADER_HEIGHT + 1, SCREEN_WIDTH, HEADER_HEIGHT + 1, 0x0861);
}

/*
 * Draw footer with help text
 */
void drawFooter() {
  // Draw modern footer with instructions (single line centered in yellow) using smooth font
  int footerY = SCREEN_HEIGHT - 22; // Positioned near bottom
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING); // Yellow

  String helpText;
  if (currentMode == MODE_MAIN_MENU) {
    helpText = "UP/DN Navigate   ENTER Select   ESC x3 Sleep";
  } else {
    helpText = "UP/DN Navigate   ENTER Select   ESC Back";
  }

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, helpText.c_str(), 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, footerY);
  tft.print(helpText);
  tft.setFont(nullptr);
}

/*
 * Draw menu items in carousel/stack card design
 */
void drawMenuItems(String options[], String icons[], int numItems) {
  // Clear only the menu area (between header and footer)
  tft.fillRect(0, HEADER_HEIGHT + 2, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT - 2, COLOR_BACKGROUND);

  // Draw menu items with carousel/stack design
  // Main selected card (larger, using more screen space)
  int mainCardWidth = CARD_MAIN_WIDTH;
  int mainCardHeight = CARD_MAIN_HEIGHT;
  int mainCardX = (SCREEN_WIDTH - mainCardWidth) / 2;
  int mainCardY = 110; // Scaled for larger display

  // Draw the selected card (large and prominent)
  tft.fillRoundRect(mainCardX, mainCardY, mainCardWidth, mainCardHeight, 12, 0x249F); // Blue accent, rounded corners scaled
  tft.drawRoundRect(mainCardX, mainCardY, mainCardWidth, mainCardHeight, 12, 0x34BF); // Lighter outline

  // Draw icon circle for selected (scaled)
  tft.fillCircle(mainCardX + 45, mainCardY + 40, ICON_RADIUS, 0x34BF);
  tft.drawCircle(mainCardX + 45, mainCardY + 40, ICON_RADIUS, ST77XX_WHITE); // White outline
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(mainCardX + 33, mainCardY + 28); // Letter centered in 30px radius circle
  tft.print(icons[currentSelection]);
  tft.setFont(nullptr);

  // Draw menu text for selected (larger for 4" display) using smooth font
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(mainCardX + 95, mainCardY + 28); // Vertically centered in 80px card
  tft.print(options[currentSelection]);
  tft.setFont(nullptr);

  // Draw selection arrow (scaled)
  tft.fillTriangle(mainCardX + mainCardWidth - 30, mainCardY + 32,
                   mainCardX + mainCardWidth - 30, mainCardY + 48,
                   mainCardX + mainCardWidth - 15, mainCardY + 40, ST77XX_WHITE);

  // Draw stacked cards underneath (previous items) - scaled for larger display
  int stackCardWidth = CARD_STACK_WIDTH_1;
  int stackCardHeight = 32; // Scaled from 24
  int stackCardX = (SCREEN_WIDTH - stackCardWidth) / 2;
  int stackOffset = 15; // Scaled from 10

  // Draw card below (next item in list)
  if (currentSelection < numItems - 1) {
    int stackY1 = mainCardY + mainCardHeight + stackOffset;
    tft.fillRoundRect(stackCardX, stackY1, stackCardWidth, stackCardHeight, 8, 0x2104);

    // Draw small circle for icon (scaled)
    tft.drawCircle(stackCardX + 18, stackY1 + 16, 12, 0x4208);
    tft.setFont(&FreeSansBold9pt7b);  // Smaller font for grey cards
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(stackCardX + 13, stackY1 + 8); // Letter centered in 12px radius circle
    tft.print(icons[currentSelection + 1]);

    // Draw text
    tft.setFont(&FreeSansBold12pt7b);  // Keep 12pt for menu text
    tft.setCursor(stackCardX + 38, stackY1 + 6); // Vertically centered in 32px card
    tft.print(options[currentSelection + 1]);
    tft.setFont(nullptr);
  }

  // Draw card further below (next+1 item)
  if (currentSelection < numItems - 2) {
    int stackY2 = mainCardY + mainCardHeight + stackOffset + stackCardHeight + 8;
    int stackCardWidth2 = CARD_STACK_WIDTH_2;
    int stackCardX2 = (SCREEN_WIDTH - stackCardWidth2) / 2;
    tft.fillRoundRect(stackCardX2, stackY2, stackCardWidth2, 24, 6, 0x1082); // 24px tall card

    // Draw small circle for icon (scaled)
    tft.drawCircle(stackCardX2 + 15, stackY2 + 12, 9, 0x3186);
    tft.setFont(&FreeSansBold9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(0x5AEB);
    tft.setCursor(stackCardX2 + 10, stackY2 + 4); // Letter centered in 9px radius circle
    tft.print(icons[currentSelection + 2]);

    // Draw text
    tft.setCursor(stackCardX2 + 30, stackY2 + 4); // Vertically centered in 24px card
    tft.print(options[currentSelection + 2]);
    tft.setFont(nullptr);
  }

  // Draw card above (previous item in list)
  if (currentSelection > 0) {
    int stackY0 = mainCardY - stackCardHeight - stackOffset;
    tft.fillRoundRect(stackCardX, stackY0, stackCardWidth, stackCardHeight, 8, 0x2104);

    // Draw small circle for icon (scaled)
    tft.drawCircle(stackCardX + 18, stackY0 + 16, 12, 0x4208);
    tft.setFont(&FreeSansBold9pt7b);  // Smaller font for grey cards
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF);
    tft.setCursor(stackCardX + 13, stackY0 + 8); // Letter centered in 12px radius circle
    tft.print(icons[currentSelection - 1]);

    // Draw text
    tft.setFont(&FreeSansBold12pt7b);  // Keep 12pt for menu text
    tft.setCursor(stackCardX + 38, stackY0 + 6); // Vertically centered in 32px card
    tft.print(options[currentSelection - 1]);
    tft.setFont(nullptr);
  }
}

/*
 * Show "Coming Soon" placeholder screen for unimplemented features
 */
void drawComingSoon(const char* featureName) {
  tft.fillScreen(COLOR_BACKGROUND);
  drawHeader();

  // Draw feature name using smooth font
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);

  // Center the feature name
  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, featureName, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 102);
  tft.print(featureName);

  // Draw "Coming Soon" message using smooth font
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(COLOR_WARNING);
  const char* comingSoon = "Coming Soon";
  getTextBounds_compat(tft, comingSoon, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 152);
  tft.print(comingSoon);

  // Draw description using smooth font
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(0x7BEF);  // Gray
  const char* desc = "This feature is under development";
  getTextBounds_compat(tft, desc, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 192);
  tft.print(desc);

  // Draw ESC instruction using smooth font
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(ST77XX_WHITE);
  const char* escText = "Press ESC to go back";
  getTextBounds_compat(tft, escText, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 232);
  tft.print(escText);

  tft.setFont(nullptr); // Reset font
}

/*
 * Main menu draw dispatcher
 */
void drawMenu() {
  tft.fillScreen(COLOR_BACKGROUND);

  drawHeader();

  // Draw footer (only for menu modes)
  if (currentMode == MODE_MAIN_MENU || currentMode == MODE_TRAINING_MENU ||
      currentMode == MODE_GAMES_MENU || currentMode == MODE_CW_MENU ||
      currentMode == MODE_SETTINGS_MENU || currentMode == MODE_HAM_TOOLS_MENU ||
      currentMode == MODE_QSO_LOGGER_MENU || currentMode == MODE_DEVICE_SETTINGS_MENU ||
      currentMode == MODE_WIFI_SUBMENU || currentMode == MODE_GENERAL_SUBMENU ||
      currentMode == MODE_BLUETOOTH_MENU || currentMode == MODE_DEVICE_BT_SUBMENU) {
    drawFooter();
  }

  // Draw menu items or mode-specific UI
  if (currentMode == MODE_MAIN_MENU) {
    drawMenuItems(mainMenuOptions, mainMenuIcons, MAIN_MENU_ITEMS);
  } else if (currentMode == MODE_CW_MENU) {
    drawMenuItems(cwMenuOptions, cwMenuIcons, CW_MENU_ITEMS);
  } else if (currentMode == MODE_TRAINING_MENU) {
    drawMenuItems(trainingMenuOptions, trainingMenuIcons, TRAINING_MENU_ITEMS);
  } else if (currentMode == MODE_GAMES_MENU) {
    drawMenuItems(gamesMenuOptions, gamesMenuIcons, GAMES_MENU_ITEMS);
  } else if (currentMode == MODE_SETTINGS_MENU) {
    drawMenuItems(settingsMenuOptions, settingsMenuIcons, SETTINGS_MENU_ITEMS);
  } else if (currentMode == MODE_DEVICE_SETTINGS_MENU) {
    drawMenuItems(deviceSettingsMenuOptions, deviceSettingsMenuIcons, DEVICE_SETTINGS_MENU_ITEMS);
  } else if (currentMode == MODE_WIFI_SUBMENU) {
    drawMenuItems(wifiSubmenuOptions, wifiSubmenuIcons, WIFI_SUBMENU_ITEMS);
  } else if (currentMode == MODE_GENERAL_SUBMENU) {
    drawMenuItems(generalSubmenuOptions, generalSubmenuIcons, GENERAL_SUBMENU_ITEMS);
  } else if (currentMode == MODE_HAM_TOOLS_MENU) {
    drawMenuItems(hamToolsMenuOptions, hamToolsMenuIcons, HAM_TOOLS_MENU_ITEMS);
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
  } else if (currentMode == MODE_BRIGHTNESS_SETTINGS) {
    drawBrightnessDisplay(tft);
  } else if (currentMode == MODE_CALLSIGN_SETTINGS) {
    drawCallsignUI(tft);
  } else if (currentMode == MODE_WEB_PASSWORD_SETTINGS) {
    drawWebPasswordUI(tft);
  } else if (currentMode == MODE_VAIL_REPEATER) {
    drawVailUI(tft);
  } else if (currentMode == MODE_BLUETOOTH_MENU) {
    drawMenuItems(bluetoothMenuOptions, bluetoothMenuIcons, BLUETOOTH_MENU_ITEMS);
  } else if (currentMode == MODE_BT_HID) {
    drawBTHIDUI(tft);
  } else if (currentMode == MODE_BT_MIDI) {
    drawBTMIDIUI(tft);
  } else if (currentMode == MODE_DEVICE_BT_SUBMENU) {
    drawMenuItems(deviceBTSubmenuOptions, deviceBTSubmenuIcons, DEVICE_BT_SUBMENU_ITEMS);
  } else if (currentMode == MODE_BT_KEYBOARD_SETTINGS) {
    drawBTKeyboardSettingsUI(tft);
  }
}

#endif // MENU_UI_H
