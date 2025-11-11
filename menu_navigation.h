/*
 * Menu Navigation Module
 * Handles keyboard input routing and menu selection logic
 */

#ifndef MENU_NAVIGATION_H
#define MENU_NAVIGATION_H

#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "config.h"
#include "i2s_audio.h"
#include "menu_ui.h"  // Get MenuMode definition and menu UI functions

// Forward declarations from main file
extern int currentSelection;
extern MenuMode currentMode;

// Menu options arrays are defined in menu_ui.h (already included above)
// No forward declarations needed

// Forward declarations for mode-specific handlers
int handleHearItTypeItInput(char key, Adafruit_ST7789& tft);
int handleWiFiInput(char key, Adafruit_ST7789& tft);
int handleCWSettingsInput(char key, Adafruit_ST7789& tft);
int handleVolumeInput(char key, Adafruit_ST7789& tft);
int handleCallsignInput(char key, Adafruit_ST7789& tft);
int handleWebPasswordInput(char key, Adafruit_ST7789& tft);
int handlePracticeInput(char key, Adafruit_ST7789& tft);
int handleVailInput(char key, Adafruit_ST7789& tft);
int handleCWATrackSelectInput(char key, Adafruit_ST7789& tft);
int handleCWASessionSelectInput(char key, Adafruit_ST7789& tft);
int handleCWAPracticeTypeSelectInput(char key, Adafruit_ST7789& tft);
int handleCWAMessageTypeSelectInput(char key, Adafruit_ST7789& tft);
int handleCWACopyPracticeInput(char key, Adafruit_ST7789& tft);
int handleCWASendingPracticeInput(char key, Adafruit_ST7789& tft);
int handleCWAQSOPracticeInput(char key, Adafruit_ST7789& tft);
int handleMorseShooterInput(char key, Adafruit_ST7789& tft);
int handleMemoryGameInput(char key, Adafruit_ST7789& tft);
int handleWebPracticeInput(char key, Adafruit_ST7789& tft);
int handleKochInput(char key, Adafruit_ST7789& tft);

void drawHearItTypeItUI(Adafruit_ST7789& tft);
void drawInputBox(Adafruit_ST7789& tft);
void drawWiFiUI(Adafruit_ST7789& tft);
void drawCWSettingsUI(Adafruit_ST7789& tft);
void drawVolumeDisplay(Adafruit_ST7789& tft);
void drawCallsignUI(Adafruit_ST7789& tft);
void drawWebPasswordUI(Adafruit_ST7789& tft);
void drawVailUI(Adafruit_ST7789& tft);

void startNewCallsign();
void playCurrentCallsign();
void startPracticeMode(Adafruit_ST7789& tft);
void startCWAcademy(Adafruit_ST7789& tft);
void startCWACopyPractice(Adafruit_ST7789& tft);
void startCWACopyRound(Adafruit_ST7789& tft);
void startCWASendingPractice(Adafruit_ST7789& tft);
void startCWAQSOPractice(Adafruit_ST7789& tft);
void startWiFiSettings(Adafruit_ST7789& tft);
void startCWSettings(Adafruit_ST7789& tft);
void initVolumeSettings(Adafruit_ST7789& tft);
void startCallsignSettings(Adafruit_ST7789& tft);
void startWebPasswordSettings(Adafruit_ST7789& tft);
void startVailRepeater(Adafruit_ST7789& tft);
void connectToVail(String channel);
void startMorseShooter(Adafruit_ST7789& tft);
void startMemoryGame(Adafruit_ST7789& tft);
void startKochMethod(Adafruit_ST7789& tft);  // Koch Method initialization
void startRadioOutput(Adafruit_ST7789& tft);  // Radio Output initialization
int handleRadioOutputInput(char key, Adafruit_ST7789& tft);  // Radio Output input handler
void startCWMemoriesMode(Adafruit_ST7789& tft);  // CW Memories initialization
int handleCWMemoriesInput(char key, Adafruit_ST7789& tft);  // CW Memories input handler
void drawCWMemoriesUI(Adafruit_ST7789& tft);  // CW Memories UI
void initLogEntry();  // QSO Logger initialization
int handleQSOLogEntryInput(char key, Adafruit_ST7789& tft);  // QSO log entry input handler
void startViewLogs(Adafruit_ST7789& tft);  // QSO view logs initialization
int handleViewLogsInput(char key, Adafruit_ST7789& tft);  // QSO view logs input handler
void startStatistics(Adafruit_ST7789& tft);  // QSO statistics initialization
int handleStatisticsInput(char key, Adafruit_ST7789& tft);  // QSO statistics input handler
void startLoggerSettings(Adafruit_ST7789& tft);  // QSO logger settings initialization
int handleLoggerSettingsInput(char key, Adafruit_ST7789& tft);  // QSO logger settings input handler
void drawLoggerSettingsUI(Adafruit_ST7789& tft);  // QSO logger settings UI

extern String vailChannel;

// Deep sleep tracking (triple ESC press)
int escPressCount = 0;
unsigned long lastEscPressTime = 0;
#define TRIPLE_ESC_TIMEOUT 2000  // 2 seconds window for 3 presses

/*
 * Enter deep sleep mode with wake on DIT paddle
 */
void enterDeepSleep() {
  Serial.println("Entering deep sleep...");

  // Disconnect WiFi if connected
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }

  // Show sleep message
  tft.fillScreen(COLOR_BACKGROUND);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);

  tft.setCursor(40, 110);
  tft.print("Going to");
  tft.setCursor(50, 140);
  tft.print("Sleep...");

  tft.setFont();
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  tft.setCursor(30, 180);
  tft.print("Press DIT paddle to wake");

  delay(2000);

  // Turn off display
  tft.fillScreen(ST77XX_BLACK);
  // Note: Backlight is hardwired to 3.3V and cannot be turned off via software

  // Configure wake on DIT paddle press (active LOW)
  esp_sleep_enable_ext0_wakeup((gpio_num_t)DIT_PIN, LOW);

  // Enter deep sleep
  esp_deep_sleep_start();
  // Device will wake here and restart from setup()
}

/*
 * Handle menu item selection
 */
void selectMenuItem() {
  // Play confirmation beep
  beep(TONE_SELECT, BEEP_MEDIUM);

  String selectedItem;

  if (currentMode == MODE_MAIN_MENU) {
    selectedItem = mainMenuOptions[currentSelection];

    // Handle main menu selections
    if (currentSelection == 0) {
      // Training
      currentMode = MODE_TRAINING_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 1) {
      // Games
      currentMode = MODE_GAMES_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 2) {
      // Radio
      currentMode = MODE_RADIO_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 3) {
      // Tools
      currentMode = MODE_TOOLS_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 4) {
      // Settings
      currentMode = MODE_SETTINGS_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 5) {
      // WiFi (Vail Repeater)
      if (WiFi.status() != WL_CONNECTED) {
        // Not connected to WiFi
        tft.fillScreen(COLOR_BACKGROUND);
        tft.setTextSize(2);
        tft.setTextColor(ST77XX_RED);
        tft.setCursor(30, 100);
        tft.print("Connect WiFi");
        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(20, 130);
        tft.print("Settings > WiFi Setup");
        delay(2000);
        drawMenu();
      } else {
        // Connected to WiFi, start Vail repeater
        currentMode = MODE_VAIL_REPEATER;
        startVailRepeater(tft);
        connectToVail(vailChannel);  // Use default channel
      }
    }

  } else if (currentMode == MODE_TRAINING_MENU) {
    selectedItem = trainingMenuOptions[currentSelection];

    // Handle training menu selections
    if (currentSelection == 0) {
      // Hear It Type It
      currentMode = MODE_HEAR_IT_TYPE_IT;
      loadHearItSettings();  // Load settings from preferences
      randomSeed(analogRead(0)); // Seed random number generator
      startNewCallsign();
      drawMenu();
      delay(1000); // Brief pause before starting
      playCurrentCallsign();
      drawHearItTypeItUI(tft);
    } else if (currentSelection == 1) {
      // Practice
      currentMode = MODE_PRACTICE;
      startPracticeMode(tft);
    } else if (currentSelection == 2) {
      // Koch Method
      currentMode = MODE_KOCH_METHOD;
      startKochMethod(tft);
    } else if (currentSelection == 3) {
      // CW Academy
      currentMode = MODE_CW_ACADEMY_TRACK_SELECT;
      startCWAcademy(tft);
    }
  } else if (currentMode == MODE_GAMES_MENU) {
    selectedItem = gamesMenuOptions[currentSelection];

    // Handle games menu selections
    if (currentSelection == 0) {
      // Morse Shooter
      currentMode = MODE_MORSE_SHOOTER;
      startMorseShooter(tft);
    } else if (currentSelection == 1) {
      // Memory Chain
      currentMode = MODE_MORSE_MEMORY;
      startMemoryGame(tft);
    }
  } else if (currentMode == MODE_SETTINGS_MENU) {
    selectedItem = settingsMenuOptions[currentSelection];

    // Handle settings menu selections
    if (currentSelection == 0) {
      // WiFi Setup
      currentMode = MODE_WIFI_SETTINGS;
      startWiFiSettings(tft);
    } else if (currentSelection == 1) {
      // CW Settings
      currentMode = MODE_CW_SETTINGS;
      startCWSettings(tft);
    } else if (currentSelection == 2) {
      // Volume Settings
      currentMode = MODE_VOLUME_SETTINGS;
      initVolumeSettings(tft);
    } else if (currentSelection == 3) {
      // Web Password
      currentMode = MODE_WEB_PASSWORD_SETTINGS;
      startWebPasswordSettings(tft);
    } else if (currentSelection == 4) {
      // General Settings
      currentMode = MODE_CALLSIGN_SETTINGS;
      startCallsignSettings(tft);
    }

  } else if (currentMode == MODE_RADIO_MENU) {
    selectedItem = radioMenuOptions[currentSelection];

    // Handle radio menu selections
    if (currentSelection == 0) {
      // Radio Output
      currentMode = MODE_RADIO_OUTPUT;
      startRadioOutput(tft);
    } else if (currentSelection == 1) {
      // CW Memories
      currentMode = MODE_CW_MEMORIES;
      startCWMemoriesMode(tft);
    }

  } else if (currentMode == MODE_TOOLS_MENU) {
    selectedItem = toolsMenuOptions[currentSelection];

    // Handle tools menu selections
    if (currentSelection == 0) {
      // QSO Logger
      currentMode = MODE_QSO_LOGGER_MENU;
      currentSelection = 0;
      drawMenu();
    }

  } else if (currentMode == MODE_QSO_LOGGER_MENU) {
    selectedItem = qsoLoggerMenuOptions[currentSelection];

    // Handle QSO Logger menu selections
    if (currentSelection == 0) {
      // New Log Entry
      currentMode = MODE_QSO_LOG_ENTRY;
      initLogEntry();  // Initialize form with defaults
      drawMenu();
    } else if (currentSelection == 1) {
      // View Logs
      currentMode = MODE_QSO_VIEW_LOGS;
      startViewLogs(tft);
    } else if (currentSelection == 2) {
      // Statistics
      currentMode = MODE_QSO_STATISTICS;
      startStatistics(tft);
    } else if (currentSelection == 3) {
      // Logger Settings
      currentMode = MODE_QSO_LOGGER_SETTINGS;
      startLoggerSettings(tft);
    }
  }
}

/*
 * Handle keyboard input and route to appropriate mode handler
 */
void handleKeyPress(char key) {
  bool redraw = false;

  // Handle different modes
  if (currentMode == MODE_HEAR_IT_TYPE_IT) {
    int result = handleHearItTypeItInput(key, tft);
    if (result == -1) {
      // Exit to training menu
      currentMode = MODE_TRAINING_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Full redraw requested
      drawHearItTypeItUI(tft);
    } else if (result == 3) {
      // Input box only redraw (faster for typing)
      drawInputBox(tft);
    }
    return;
  }

  // Handle WiFi settings mode
  if (currentMode == MODE_WIFI_SETTINGS) {
    int result = handleWiFiInput(key, tft);
    if (result == -1) {
      // Exit WiFi settings, back to settings menu
      currentMode = MODE_SETTINGS_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Full redraw requested
      drawWiFiUI(tft);
    }
    return;
  }

  // Handle CW settings mode
  if (currentMode == MODE_CW_SETTINGS) {
    int result = handleCWSettingsInput(key, tft);
    if (result == -1) {
      // Exit CW settings, back to settings menu
      currentMode = MODE_SETTINGS_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Volume settings mode
  if (currentMode == MODE_VOLUME_SETTINGS) {
    int result = handleVolumeInput(key, tft);
    if (result == -1) {
      // Exit volume settings, back to settings menu
      currentMode = MODE_SETTINGS_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Web Password settings mode
  if (currentMode == MODE_WEB_PASSWORD_SETTINGS) {
    int result = handleWebPasswordInput(key, tft);
    if (result == -1) {
      // Exit web password settings, back to settings menu
      currentMode = MODE_SETTINGS_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle General settings mode (Callsign)
  if (currentMode == MODE_CALLSIGN_SETTINGS) {
    int result = handleCallsignInput(key, tft);
    if (result == -1) {
      // Exit general settings, back to settings menu
      currentMode = MODE_SETTINGS_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Koch Method mode
  if (currentMode == MODE_KOCH_METHOD) {
    int result = handleKochInput(key, tft);
    if (result == -1) {
      // Exit Koch Method, back to training menu
      currentMode = MODE_TRAINING_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Full redraw requested
      drawKochUI(tft);
    } else if (result == 3) {
      // Partial redraw (input box only)
      drawKochUI(tft);
    }
    return;
  }

  // Handle Practice mode
  if (currentMode == MODE_PRACTICE) {
    int result = handlePracticeInput(key, tft);
    if (result == -1) {
      // Exit practice mode, back to training menu
      currentMode = MODE_TRAINING_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Vail repeater mode
  if (currentMode == MODE_VAIL_REPEATER) {
    int result = handleVailInput(key, tft);
    if (result == -1) {
      // Exit Vail mode, back to main menu
      currentMode = MODE_MAIN_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle CW Academy track selection
  if (currentMode == MODE_CW_ACADEMY_TRACK_SELECT) {
    int result = handleCWATrackSelectInput(key, tft);
    if (result == -1) {
      // Exit CW Academy, back to training menu
      currentMode = MODE_TRAINING_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 1) {
      // Navigate to session selection
      currentMode = MODE_CW_ACADEMY_SESSION_SELECT;
      drawCWASessionSelectUI(tft);
    } else if (result == 2) {
      // Redraw requested
      drawCWATrackSelectUI(tft);
    }
    return;
  }

  // Handle CW Academy session selection
  if (currentMode == MODE_CW_ACADEMY_SESSION_SELECT) {
    int result = handleCWASessionSelectInput(key, tft);
    if (result == -1) {
      // Exit to track selection
      currentMode = MODE_CW_ACADEMY_TRACK_SELECT;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawCWATrackSelectUI(tft);
    } else if (result == 1) {
      // Navigate to practice type selection
      currentMode = MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT;
      drawCWAPracticeTypeSelectUI(tft);
    } else if (result == 2) {
      // Redraw requested
      drawCWASessionSelectUI(tft);
    }
    return;
  }

  // Handle CW Academy practice type selection
  if (currentMode == MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT) {
    int result = handleCWAPracticeTypeSelectInput(key, tft);
    if (result == -1) {
      // Exit to session selection
      currentMode = MODE_CW_ACADEMY_SESSION_SELECT;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawCWASessionSelectUI(tft);
    } else if (result == 1) {
      // Navigate to message type selection
      currentMode = MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT;
      drawCWAMessageTypeSelectUI(tft);
    } else if (result == 2) {
      // Redraw requested
      drawCWAPracticeTypeSelectUI(tft);
    } else if (result == 3) {
      // Start QSO practice (sessions 11-13)
      currentMode = MODE_CW_ACADEMY_QSO_PRACTICE;
      startCWAQSOPractice(tft);
    }
    return;
  }

  // Handle CW Academy message type selection
  if (currentMode == MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT) {
    int result = handleCWAMessageTypeSelectInput(key, tft);
    if (result == -1) {
      // Exit to practice type selection
      currentMode = MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawCWAPracticeTypeSelectUI(tft);
    } else if (result == 1) {
      // Start copy practice mode
      currentMode = MODE_CW_ACADEMY_COPY_PRACTICE;
      startCWACopyPractice(tft);
      delay(1000);  // Brief pause before first round
      startCWACopyRound(tft);
    } else if (result == 2) {
      // Redraw requested
      drawCWAMessageTypeSelectUI(tft);
    } else if (result == 3) {
      // Start sending practice mode
      currentMode = MODE_CW_ACADEMY_SENDING_PRACTICE;
      startCWASendingPractice(tft);
    }
    return;
  }

  // Handle CW Academy copy practice mode
  if (currentMode == MODE_CW_ACADEMY_COPY_PRACTICE) {
    int result = handleCWACopyPracticeInput(key, tft);
    if (result == -1) {
      // Exit to message type selection
      currentMode = MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawCWAMessageTypeSelectUI(tft);
    } else if (result == 2) {
      // Redraw requested
      drawCWACopyPracticeUI(tft);
    }
    return;
  }

  // Handle CW Academy sending practice mode
  if (currentMode == MODE_CW_ACADEMY_SENDING_PRACTICE) {
    int result = handleCWASendingPracticeInput(key, tft);
    if (result == -1) {
      // Exit to message type selection
      currentMode = MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawCWAMessageTypeSelectUI(tft);
    } else if (result == 2) {
      // Redraw requested
      drawCWASendingPracticeUI(tft);
    }
    return;
  }

  // Handle CW Academy QSO practice mode
  if (currentMode == MODE_CW_ACADEMY_QSO_PRACTICE) {
    int result = handleCWAQSOPracticeInput(key, tft);
    if (result == -1) {
      // Exit to practice type selection (since QSO practice bypasses message type)
      currentMode = MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawCWAPracticeTypeSelectUI(tft);
    } else if (result == 2) {
      // Redraw requested
      drawCWAQSOPracticeUI(tft);
    }
    return;
  }

  // Handle Morse Shooter game
  if (currentMode == MODE_MORSE_SHOOTER) {
    int result = handleMorseShooterInput(key, tft);
    if (result == -1) {
      // Exit to games menu
      currentMode = MODE_GAMES_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Memory Chain game
  if (currentMode == MODE_MORSE_MEMORY) {
    int result = handleMemoryGameInput(key, tft);
    if (result == -1) {
      // Exit to games menu
      currentMode = MODE_GAMES_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle QSO Log Entry mode
  if (currentMode == MODE_QSO_LOG_ENTRY) {
    int result = handleQSOLogEntryInput(key, tft);
    if (result == -1) {
      // Exit to QSO Logger menu
      currentMode = MODE_QSO_LOGGER_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Redraw requested
      drawQSOLogEntryUI(tft);
    }
    return;
  }

  // Handle QSO View Logs mode
  if (currentMode == MODE_QSO_VIEW_LOGS) {
    int result = handleViewLogsInput(key, tft);
    if (result == -1) {
      // Exit to QSO Logger menu
      currentMode = MODE_QSO_LOGGER_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle QSO Statistics mode
  if (currentMode == MODE_QSO_STATISTICS) {
    int result = handleStatisticsInput(key, tft);
    if (result == -1) {
      // Exit to QSO Logger menu
      currentMode = MODE_QSO_LOGGER_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle QSO Logger Settings mode
  if (currentMode == MODE_QSO_LOGGER_SETTINGS) {
    int result = handleLoggerSettingsInput(key, tft);
    if (result == -1) {
      // Exit to QSO Logger menu
      currentMode = MODE_QSO_LOGGER_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Redraw requested
      drawLoggerSettingsUI(tft);
    }
    return;
  }

  // Handle Radio Output mode
  if (currentMode == MODE_RADIO_OUTPUT) {
    int result = handleRadioOutputInput(key, tft);
    if (result == -1) {
      // Exit to Radio menu
      currentMode = MODE_RADIO_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Redraw requested
      drawRadioOutputUI(tft);
    }
    return;
  }

  // Handle CW Memories mode
  if (currentMode == MODE_CW_MEMORIES) {
    int result = handleCWMemoriesInput(key, tft);
    if (result == -1) {
      // Exit to Radio menu
      currentMode = MODE_RADIO_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    // Note: result == 2 means redraw was already handled by the input handler
    // Don't force redraw here - the handler draws the appropriate screen (list/menu/edit/delete)
    return;
  }

  // Handle Web Practice mode
  if (currentMode == MODE_WEB_PRACTICE) {
    int result = handleWebPracticeInput(key, tft);
    if (result == -1) {
      // Exit to Main menu
      currentMode = MODE_MAIN_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Web Memory Chain mode
  if (currentMode == MODE_WEB_MEMORY_CHAIN) {
    int result = handleWebMemoryChainInput(key, tft);
    if (result == -1) {
      // Exit to Main menu
      currentMode = MODE_MAIN_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Web Hear It Type It mode
  if (currentMode == MODE_WEB_HEAR_IT) {
    int result = handleWebHearItInput(key, tft);
    if (result == -1) {
      // Exit to Main menu
      currentMode = MODE_MAIN_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Menu navigation (for MAIN_MENU, TRAINING_MENU, GAMES_MENU, RADIO_MENU, SETTINGS_MENU, TOOLS_MENU, QSO_LOGGER_MENU)
  if (currentMode == MODE_MAIN_MENU || currentMode == MODE_TRAINING_MENU ||
      currentMode == MODE_GAMES_MENU || currentMode == MODE_RADIO_MENU ||
      currentMode == MODE_SETTINGS_MENU ||
      currentMode == MODE_TOOLS_MENU || currentMode == MODE_QSO_LOGGER_MENU) {
    int maxItems = MENU_ITEMS;
    if (currentMode == MODE_TRAINING_MENU) maxItems = TRAINING_MENU_ITEMS;
    if (currentMode == MODE_GAMES_MENU) maxItems = GAMES_MENU_ITEMS;
    if (currentMode == MODE_RADIO_MENU) maxItems = RADIO_MENU_ITEMS;
    if (currentMode == MODE_SETTINGS_MENU) maxItems = SETTINGS_MENU_ITEMS;
    if (currentMode == MODE_TOOLS_MENU) maxItems = TOOLS_MENU_ITEMS;
    if (currentMode == MODE_QSO_LOGGER_MENU) maxItems = QSO_LOGGER_MENU_ITEMS;

    // Arrow key navigation
    if (key == KEY_UP) {
      if (currentSelection > 0) {
        currentSelection--;
        redraw = true;
        beep(TONE_MENU_NAV, BEEP_SHORT);
      }
    }
    else if (key == KEY_DOWN) {
      if (currentSelection < maxItems - 1) {
        currentSelection++;
        redraw = true;
        beep(TONE_MENU_NAV, BEEP_SHORT);
      }
    }
    else if (key == KEY_ENTER || key == KEY_ENTER_ALT || key == KEY_RIGHT) {
      selectMenuItem();
    }
    else if (key == KEY_ESC) {
      if (currentMode == MODE_TRAINING_MENU || currentMode == MODE_GAMES_MENU ||
          currentMode == MODE_RADIO_MENU || currentMode == MODE_SETTINGS_MENU ||
          currentMode == MODE_TOOLS_MENU) {
        // Back to main menu
        currentMode = MODE_MAIN_MENU;
        currentSelection = 0;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawMenu();
        return;
      } else if (currentMode == MODE_QSO_LOGGER_MENU) {
        // Back to tools menu
        currentMode = MODE_TOOLS_MENU;
        currentSelection = 0;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawMenu();
        return;
      } else if (currentMode == MODE_MAIN_MENU) {
        // In main menu - count ESC presses for sleep (triple tap)
        escPressCount++;
        lastEscPressTime = millis();

        if (escPressCount >= 3) {
          // Triple ESC pressed - enter sleep
          beep(TONE_STARTUP, 200);
          enterDeepSleep();
        } else {
          // Beep for each press to give feedback
          beep(TONE_MENU_NAV, 50);
        }
      }
    }

    if (redraw) {
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
      }
    }
  }
}

#endif // MENU_NAVIGATION_H
