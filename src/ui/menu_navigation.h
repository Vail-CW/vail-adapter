/*
 * Menu Navigation Module
 * Handles keyboard input routing and menu selection logic
 */

#ifndef MENU_NAVIGATION_H
#define MENU_NAVIGATION_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <esp_sleep.h>
#include "../core/config.h"
#include "../audio/i2s_audio.h"
#include "menu_ui.h"  // Same folder - Get MenuMode definition and menu UI functions
#include "../storage/sd_card.h"  // SD card functions for QSO logger
#include "../settings/settings_brightness.h"  // Brightness settings

// Forward declarations from main file
extern int currentSelection;
extern MenuMode currentMode;

// Menu options arrays are defined in menu_ui.h (already included above)
// No forward declarations needed

// Forward declarations for mode-specific handlers
int handleHearItTypeItInput(char key, LGFX& tft);
int handleHearItConfigureInput(char key, LGFX& tft);  // New: Settings configuration input
int handleWiFiInput(char key, LGFX& tft);
int handleCWSettingsInput(char key, LGFX& tft);
int handleVolumeInput(char key, LGFX& tft);
int handleCallsignInput(char key, LGFX& tft);
int handleWebPasswordInput(char key, LGFX& tft);
int handlePracticeInput(char key, LGFX& tft);
int handleVailInput(char key, LGFX& tft);
int handleCWATrackSelectInput(char key, LGFX& tft);
int handleCWASessionSelectInput(char key, LGFX& tft);
int handleCWAPracticeTypeSelectInput(char key, LGFX& tft);
int handleCWAMessageTypeSelectInput(char key, LGFX& tft);
int handleCWACopyPracticeInput(char key, LGFX& tft);
int handleCWASendingPracticeInput(char key, LGFX& tft);
int handleCWAQSOPracticeInput(char key, LGFX& tft);
int handleMorseShooterInput(char key, LGFX& tft);
int handleMemoryGameInput(char key, LGFX& tft);
int handleWebPracticeInput(char key, LGFX& tft);
int handleKochInput(char key, LGFX& tft);
int handleLicenseSelectInput(char key, LGFX& tft);
int handleLicenseQuizInput(char key, LGFX& tft);
int handleLicenseStatsInput(char key, LGFX& tft);
void startLicenseQuiz(LGFX& tft, int licenseType);
void startLicenseStats(LGFX& tft);

void drawHearItTypeItUI(LGFX& tft);
void drawInputBox(LGFX& tft);
void drawWiFiUI(LGFX& tft);
void drawCWSettingsUI(LGFX& tft);
void drawVolumeDisplay(LGFX& tft);
void drawCallsignUI(LGFX& tft);
void drawWebPasswordUI(LGFX& tft);
void drawVailUI(LGFX& tft);

void startNewCallsign();
void playCurrentCallsign();
void startPracticeMode(LGFX& tft);
void startCWAcademy(LGFX& tft);
void startCWACopyPractice(LGFX& tft);
void startCWACopyRound(LGFX& tft);
void startCWASendingPractice(LGFX& tft);
void startCWAQSOPractice(LGFX& tft);
void startWiFiSettings(LGFX& tft);
void startCWSettings(LGFX& tft);
void initVolumeSettings(LGFX& tft);
void startCallsignSettings(LGFX& tft);
void startWebPasswordSettings(LGFX& tft);
void startVailRepeater(LGFX& tft);
void connectToVail(String channel);
void startMorseShooter(LGFX& tft);
void startMemoryGame(LGFX& tft);
void startKochMethod(LGFX& tft);  // Koch Method initialization
void startRadioOutput(LGFX& tft);  // Radio Output initialization
int handleRadioOutputInput(char key, LGFX& tft);  // Radio Output input handler
void startCWMemoriesMode(LGFX& tft);  // CW Memories initialization
int handleCWMemoriesInput(char key, LGFX& tft);  // CW Memories input handler
void drawCWMemoriesUI(LGFX& tft);  // CW Memories UI
void initLogEntry();  // QSO Logger initialization
int handleQSOLogEntryInput(char key, LGFX& tft);  // QSO log entry input handler
void startViewLogs(LGFX& tft);  // QSO view logs initialization
int handleViewLogsInput(char key, LGFX& tft);  // QSO view logs input handler
void startStatistics(LGFX& tft);  // QSO statistics initialization
int handleStatisticsInput(char key, LGFX& tft);  // QSO statistics input handler
void startLoggerSettings(LGFX& tft);  // QSO logger settings initialization
int handleLoggerSettingsInput(char key, LGFX& tft);  // QSO logger settings input handler
void drawLoggerSettingsUI(LGFX& tft);  // QSO logger settings UI
void startBTHID(LGFX& tft);  // BT HID mode initialization
int handleBTHIDInput(char key, LGFX& tft);  // BT HID input handler
void startBTMIDI(LGFX& tft);  // BT MIDI mode initialization
int handleBTMIDIInput(char key, LGFX& tft);  // BT MIDI input handler
void startBTKeyboardSettings(LGFX& tft);  // BT Keyboard settings initialization
int handleBTKeyboardSettingsInput(char key, LGFX& tft);  // BT Keyboard settings input handler
void updateBTKeyboardSettingsUI(LGFX& tft);  // BT Keyboard settings UI update

extern String vailChannel;

// Deep sleep tracking (triple ESC press)
int escPressCount = 0;
unsigned long lastEscPressTime = 0;
#define TRIPLE_ESC_TIMEOUT 2000  // 2 seconds window for 3 presses

/*
 * Show SD card required error screen
 * Returns true if SD card becomes available after user presses key
 */
bool showSDCardRequiredScreen() {
  tft.fillScreen(COLOR_BACKGROUND);

  // Draw error icon (SD card with X)
  tft.setTextColor(ST77XX_RED);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setCursor(140, 100);
  tft.print("SD Card Required");

  tft.setFont(nullptr);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(80, 150);
  tft.print("Insert SD card to use");
  tft.setCursor(120, 180);
  tft.print("QSO Logger");

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);  // Gray
  tft.setCursor(100, 250);
  tft.print("Press any key to retry");
  tft.setCursor(120, 270);
  tft.print("ESC to go back");

  beep(TONE_ERROR, BEEP_MEDIUM);
  return false;
}

/*
 * Check if QSO storage is ready, show error if not
 * Returns true if storage is ready
 */
bool checkQSOStorageReady() {
  // Try to initialize SD card if not already done
  if (!sdCardAvailable) {
    initSDCard();
  }

  return sdCardAvailable;
}

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

  tft.setFont(nullptr);
  tft.setTextSize(1);
  tft.setTextColor(0x7BEF);
  tft.setCursor(30, 180);
  tft.print("Press DIT paddle to wake");

  delay(2000);

  // Turn off display
  tft.fillScreen(ST77XX_BLACK);

  // Turn off backlight (GPIO 39)
  digitalWrite(TFT_BL, LOW);  // LOW = backlight OFF (active HIGH logic)
  Serial.println("Backlight turned off for deep sleep");

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

    // Handle main menu selections (4 items: CW, Games, Ham Tools, Settings)
    if (currentSelection == 0) {
      // CW
      currentMode = MODE_CW_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 1) {
      // Games
      currentMode = MODE_GAMES_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 2) {
      // Ham Tools
      currentMode = MODE_HAM_TOOLS_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 3) {
      // Settings
      currentMode = MODE_SETTINGS_MENU;
      currentSelection = 0;
      drawMenu();
    }

  } else if (currentMode == MODE_CW_MENU) {
    selectedItem = cwMenuOptions[currentSelection];

    // Handle CW menu selections (6 items: Training, Practice, Vail Repeater, Bluetooth, Radio Output, CW Memories)
    if (currentSelection == 0) {
      // Training submenu
      currentMode = MODE_TRAINING_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 1) {
      // Practice (direct link)
      currentMode = MODE_PRACTICE;
      startPracticeMode(tft);

    } else if (currentSelection == 2) {
      // Vail Repeater
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

    } else if (currentSelection == 3) {
      // Bluetooth submenu
      currentMode = MODE_BLUETOOTH_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 4) {
      // Radio Output
      currentMode = MODE_RADIO_OUTPUT;
      startRadioOutput(tft);

    } else if (currentSelection == 5) {
      // CW Memories
      currentMode = MODE_CW_MEMORIES;
      startCWMemoriesMode(tft);
    }

  } else if (currentMode == MODE_TRAINING_MENU) {
    selectedItem = trainingMenuOptions[currentSelection];

    // Handle training menu selections
    if (currentSelection == 0) {
      // Hear It Type It - Open submenu
      currentMode = MODE_HEAR_IT_MENU;
      currentSelection = 0;
      drawMenu();

    } else if (currentSelection == 1) {
      // Koch Method
      currentMode = MODE_KOCH_METHOD;
      startKochMethod(tft);
    } else if (currentSelection == 2) {
      // CW Academy
      currentMode = MODE_CW_ACADEMY_TRACK_SELECT;
      startCWAcademy(tft);
    }

  } else if (currentMode == MODE_HEAR_IT_MENU) {
    selectedItem = hearItMenuOptions[currentSelection];

    // Handle Hear It submenu selections
    if (currentSelection == 0) {
      // Configure
      currentMode = MODE_HEAR_IT_CONFIGURE;
      loadHearItSettings();

      // Initialize settings state from saved settings
      settingsState.currentScreen = SETTINGS_SCREEN_MENU;
      settingsState.menuSelection = 0;
      settingsState.modeSelection = hearItSettings.mode;
      settingsState.speedValue = hearItSettings.wpm;
      settingsState.groupLength = hearItSettings.groupLength;
      settingsState.gridCursor = 0;
      settingsState.inPresetSelector = false;

      // Initialize character selection grid from customChars
      for (int i = 0; i < 36; i++) settingsState.charSelected[i] = false;
      for (int i = 0; i < hearItSettings.customChars.length(); i++) {
        char ch = hearItSettings.customChars[i];
        if (ch >= 'A' && ch <= 'Z') {
          settingsState.charSelected[ch - 'A'] = true;
        } else if (ch >= '0' && ch <= '9') {
          settingsState.charSelected[26 + ch - '0'] = true;
        }
      }

      // Draw settings configuration screen
      drawHearItConfigureUI(tft);

    } else if (currentSelection == 1) {
      // Start Training (Quick-start with saved settings)
      currentMode = MODE_HEAR_IT_START;
      loadHearItSettings();
      randomSeed(analogRead(0));

      // Initialize session stats
      sessionStats.totalAttempts = 0;
      sessionStats.totalCorrect = 0;
      sessionStats.sessionStartTime = millis();

      // Start directly in training state (no settings overlay)
      currentHearItState = HEAR_IT_STATE_TRAINING;
      inSettingsMode = false;

      // Start first challenge
      startNewCallsign();
      drawHearItTypeItUI(tft);
      delay(500);
      playCurrentCallsign();
      drawHearItTypeItUI(tft);
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
      // Device Settings submenu
      currentMode = MODE_DEVICE_SETTINGS_MENU;
      currentSelection = 0;
      drawMenu();
    } else if (currentSelection == 1) {
      // CW Settings
      currentMode = MODE_CW_SETTINGS;
      startCWSettings(tft);
    }

  } else if (currentMode == MODE_DEVICE_SETTINGS_MENU) {
    selectedItem = deviceSettingsMenuOptions[currentSelection];

    // Handle device settings menu selections (3 items: WiFi, General, Bluetooth)
    if (currentSelection == 0) {
      // WiFi submenu
      currentMode = MODE_WIFI_SUBMENU;
      currentSelection = 0;
      drawMenu();
    } else if (currentSelection == 1) {
      // General submenu
      currentMode = MODE_GENERAL_SUBMENU;
      currentSelection = 0;
      drawMenu();
    } else if (currentSelection == 2) {
      // Bluetooth submenu
      currentMode = MODE_DEVICE_BT_SUBMENU;
      currentSelection = 0;
      drawMenu();
    }

  } else if (currentMode == MODE_DEVICE_BT_SUBMENU) {
    selectedItem = deviceBTSubmenuOptions[currentSelection];

    // Handle Device Bluetooth submenu selections (1 item: External Keyboard)
    if (currentSelection == 0) {
      // External Keyboard
      currentMode = MODE_BT_KEYBOARD_SETTINGS;
      startBTKeyboardSettings(tft);
    }

  } else if (currentMode == MODE_WIFI_SUBMENU) {
    selectedItem = wifiSubmenuOptions[currentSelection];

    // Handle WiFi submenu selections
    if (currentSelection == 0) {
      // WiFi Setup
      currentMode = MODE_WIFI_SETTINGS;
      startWiFiSettings(tft);
    } else if (currentSelection == 1) {
      // Web Password
      currentMode = MODE_WEB_PASSWORD_SETTINGS;
      startWebPasswordSettings(tft);
    }

  } else if (currentMode == MODE_GENERAL_SUBMENU) {
    selectedItem = generalSubmenuOptions[currentSelection];

    // Handle General submenu selections
    if (currentSelection == 0) {
      // Callsign
      currentMode = MODE_CALLSIGN_SETTINGS;
      startCallsignSettings(tft);
    } else if (currentSelection == 1) {
      // Volume
      currentMode = MODE_VOLUME_SETTINGS;
      initVolumeSettings(tft);
    } else if (currentSelection == 2) {
      // Brightness
      currentMode = MODE_BRIGHTNESS_SETTINGS;
      initBrightnessSettings(tft);
    }

  } else if (currentMode == MODE_HAM_TOOLS_MENU) {
    selectedItem = hamToolsMenuOptions[currentSelection];

    // Handle Ham Tools menu selections (6 items)
    if (currentSelection == 0) {
      // QSO Logger
      currentMode = MODE_QSO_LOGGER_MENU;
      currentSelection = 0;
      drawMenu();
    } else if (currentSelection == 1) {
      // Band Plans - Coming Soon
      currentMode = MODE_BAND_PLANS;
      drawComingSoon("Band Plans");
    } else if (currentSelection == 2) {
      // Propagation - Coming Soon
      currentMode = MODE_PROPAGATION;
      drawComingSoon("Propagation");
    } else if (currentSelection == 3) {
      // Antennas - Coming Soon
      currentMode = MODE_ANTENNAS;
      drawComingSoon("Antennas");
    } else if (currentSelection == 4) {
      // License Study - Enter license selection
      currentMode = MODE_LICENSE_SELECT;
      currentSelection = 0;
      drawMenu();
    } else if (currentSelection == 5) {
      // Summit Chat - Coming Soon
      currentMode = MODE_SUMMIT_CHAT;
      drawComingSoon("Summit Chat");
    }

  } else if (currentMode == MODE_LICENSE_SELECT) {
    selectedItem = licenseSelectOptions[currentSelection];

    // Handle License Select menu (4 items: Technician, General, Extra, View Statistics)
    if (currentSelection >= 0 && currentSelection <= 2) {
      // Start quiz for selected license (0=Tech, 1=Gen, 2=Extra)
      startLicenseQuiz(tft, currentSelection);
    } else if (currentSelection == 3) {
      // View Statistics
      if (licenseSession.selectedLicense < 0 || licenseSession.selectedLicense > 2) {
        licenseSession.selectedLicense = 0;  // Default to Technician
      }
      startLicenseStats(tft);
    }

  } else if (currentMode == MODE_QSO_LOGGER_MENU) {
    selectedItem = qsoLoggerMenuOptions[currentSelection];

    // Handle QSO Logger menu selections
    // All QSO logger functions require SD card
    if (currentSelection == 0) {
      // New Log Entry - requires SD card
      if (!checkQSOStorageReady()) {
        showSDCardRequiredScreen();
        return;
      }
      currentMode = MODE_QSO_LOG_ENTRY;
      initLogEntry();  // Initialize form with defaults
      drawMenu();
    } else if (currentSelection == 1) {
      // View Logs - requires SD card
      if (!checkQSOStorageReady()) {
        showSDCardRequiredScreen();
        return;
      }
      currentMode = MODE_QSO_VIEW_LOGS;
      startViewLogs(tft);
    } else if (currentSelection == 2) {
      // Statistics - can work without SD card (uses cached metadata)
      currentMode = MODE_QSO_STATISTICS;
      startStatistics(tft);
    } else if (currentSelection == 3) {
      // Logger Settings - doesn't require SD card
      currentMode = MODE_QSO_LOGGER_SETTINGS;
      startLoggerSettings(tft);
    }

  } else if (currentMode == MODE_BLUETOOTH_MENU) {
    selectedItem = bluetoothMenuOptions[currentSelection];

    // Handle Bluetooth menu selections (2 items: HID (Keyboard), MIDI)
    if (currentSelection == 0) {
      // HID (Keyboard)
      currentMode = MODE_BT_HID;
      startBTHID(tft);
    } else if (currentSelection == 1) {
      // MIDI
      currentMode = MODE_BT_MIDI;
      startBTMIDI(tft);
    }
  }
}

/*
 * Handle keyboard input and route to appropriate mode handler
 */
void handleKeyPress(char key) {
  bool redraw = false;

  // Handle different modes
  if (currentMode == MODE_HEAR_IT_TYPE_IT || currentMode == MODE_HEAR_IT_START) {
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

  // Handle Hear It Configure mode (settings configuration)
  if (currentMode == MODE_HEAR_IT_CONFIGURE) {
    int result = handleHearItConfigureInput(key, tft);
    if (result == -1) {
      // Exit to Hear It menu
      currentMode = MODE_HEAR_IT_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Full redraw requested
      drawHearItConfigureUI(tft);
    } else if (result == 3) {
      // Transition to training mode after saving settings
      currentMode = MODE_HEAR_IT_START;
      currentHearItState = HEAR_IT_STATE_TRAINING;
      inSettingsMode = false;

      // Initialize session stats
      sessionStats.totalAttempts = 0;
      sessionStats.totalCorrect = 0;
      sessionStats.sessionStartTime = millis();

      // Start first challenge
      startNewCallsign();
      drawHearItTypeItUI(tft);
      delay(500);
      playCurrentCallsign();
      drawHearItTypeItUI(tft);
    }
    return;
  }

  // Handle WiFi settings mode
  if (currentMode == MODE_WIFI_SETTINGS) {
    int result = handleWiFiInput(key, tft);
    if (result == -1) {
      // Exit WiFi settings, back to WiFi submenu
      currentMode = MODE_WIFI_SUBMENU;
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
      currentSelection = 1;  // Stay on CW Settings position
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Volume settings mode
  if (currentMode == MODE_VOLUME_SETTINGS) {
    int result = handleVolumeInput(key, tft);
    if (result == -1) {
      // Exit volume settings, back to General submenu
      currentMode = MODE_GENERAL_SUBMENU;
      currentSelection = 1;  // Stay on Volume position
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Brightness settings mode
  if (currentMode == MODE_BRIGHTNESS_SETTINGS) {
    int result = handleBrightnessInput(key, tft);
    if (result == -1) {
      // Exit brightness settings, back to General submenu
      currentMode = MODE_GENERAL_SUBMENU;
      currentSelection = 2;  // Stay on Brightness position
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Web Password settings mode
  if (currentMode == MODE_WEB_PASSWORD_SETTINGS) {
    int result = handleWebPasswordInput(key, tft);
    if (result == -1) {
      // Exit web password settings, back to WiFi submenu
      currentMode = MODE_WIFI_SUBMENU;
      currentSelection = 1;  // Stay on Web Password position
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Callsign settings mode
  if (currentMode == MODE_CALLSIGN_SETTINGS) {
    int result = handleCallsignInput(key, tft);
    if (result == -1) {
      // Exit callsign settings, back to General submenu
      currentMode = MODE_GENERAL_SUBMENU;
      currentSelection = 0;  // Stay on Callsign position
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
      // Exit practice mode, back to CW menu
      currentMode = MODE_CW_MENU;
      currentSelection = 1;  // Practice position in CW menu
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle Vail repeater mode
  if (currentMode == MODE_VAIL_REPEATER) {
    int result = handleVailInput(key, tft);
    if (result == -1) {
      // Exit Vail mode, back to CW menu
      currentMode = MODE_CW_MENU;
      currentSelection = 2;  // Vail Repeater position in CW menu
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
      // Exit to CW menu
      currentMode = MODE_CW_MENU;
      currentSelection = 3;  // Radio Output position in CW menu
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
      // Exit to CW menu
      currentMode = MODE_CW_MENU;
      currentSelection = 4;  // CW Memories position in CW menu
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

  // Handle BT HID mode
  if (currentMode == MODE_BT_HID) {
    int result = handleBTHIDInput(key, tft);
    if (result == -1) {
      // Exit to Bluetooth menu
      currentMode = MODE_BLUETOOTH_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Redraw requested
      drawBTHIDUI(tft);
    }
    return;
  }

  // Handle BT MIDI mode
  if (currentMode == MODE_BT_MIDI) {
    int result = handleBTMIDIInput(key, tft);
    if (result == -1) {
      // Exit to Bluetooth menu
      currentMode = MODE_BLUETOOTH_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Redraw requested
      drawBTMIDIUI(tft);
    }
    return;
  }

  // Handle BT Keyboard settings mode
  if (currentMode == MODE_BT_KEYBOARD_SETTINGS) {
    int result = handleBTKeyboardSettingsInput(key, tft);
    if (result == -1) {
      // Exit to Device Bluetooth submenu
      currentMode = MODE_DEVICE_BT_SUBMENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Redraw requested
      drawBTKeyboardSettingsUI(tft);
    }
    return;
  }

  // Handle License Quiz mode
  if (currentMode == MODE_LICENSE_QUIZ) {
    int result = handleLicenseQuizInput(key, tft);
    if (result == -1) {
      currentMode = MODE_LICENSE_SELECT;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    } else if (result == 2) {
      // Redraw requested
      drawLicenseQuizUI(tft);
    }
    return;
  }

  // Handle License Stats mode
  if (currentMode == MODE_LICENSE_STATS) {
    int result = handleLicenseStatsInput(key, tft);
    if (result == -1) {
      currentMode = MODE_LICENSE_SELECT;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Handle placeholder "Coming Soon" modes - ESC returns to Ham Tools menu
  if (currentMode == MODE_BAND_PLANS || currentMode == MODE_PROPAGATION ||
      currentMode == MODE_ANTENNAS || currentMode == MODE_SUMMIT_CHAT) {
    if (key == KEY_ESC) {
      currentMode = MODE_HAM_TOOLS_MENU;
      currentSelection = 0;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawMenu();
    }
    return;
  }

  // Menu navigation (for all menu modes)
  if (currentMode == MODE_MAIN_MENU || currentMode == MODE_TRAINING_MENU ||
      currentMode == MODE_GAMES_MENU || currentMode == MODE_CW_MENU ||
      currentMode == MODE_SETTINGS_MENU || currentMode == MODE_DEVICE_SETTINGS_MENU ||
      currentMode == MODE_WIFI_SUBMENU || currentMode == MODE_GENERAL_SUBMENU ||
      currentMode == MODE_HAM_TOOLS_MENU || currentMode == MODE_QSO_LOGGER_MENU ||
      currentMode == MODE_BLUETOOTH_MENU || currentMode == MODE_DEVICE_BT_SUBMENU ||
      currentMode == MODE_LICENSE_SELECT || currentMode == MODE_HEAR_IT_MENU) {
    int maxItems = MAIN_MENU_ITEMS;
    if (currentMode == MODE_CW_MENU) maxItems = CW_MENU_ITEMS;
    if (currentMode == MODE_TRAINING_MENU) maxItems = TRAINING_MENU_ITEMS;
    if (currentMode == MODE_GAMES_MENU) maxItems = GAMES_MENU_ITEMS;
    if (currentMode == MODE_SETTINGS_MENU) maxItems = SETTINGS_MENU_ITEMS;
    if (currentMode == MODE_DEVICE_SETTINGS_MENU) maxItems = DEVICE_SETTINGS_MENU_ITEMS;
    if (currentMode == MODE_WIFI_SUBMENU) maxItems = WIFI_SUBMENU_ITEMS;
    if (currentMode == MODE_GENERAL_SUBMENU) maxItems = GENERAL_SUBMENU_ITEMS;
    if (currentMode == MODE_HAM_TOOLS_MENU) maxItems = HAM_TOOLS_MENU_ITEMS;
    if (currentMode == MODE_QSO_LOGGER_MENU) maxItems = QSO_LOGGER_MENU_ITEMS;
    if (currentMode == MODE_BLUETOOTH_MENU) maxItems = BLUETOOTH_MENU_ITEMS;
    if (currentMode == MODE_DEVICE_BT_SUBMENU) maxItems = DEVICE_BT_SUBMENU_ITEMS;
    if (currentMode == MODE_LICENSE_SELECT) maxItems = LICENSE_SELECT_ITEMS;
    if (currentMode == MODE_HEAR_IT_MENU) maxItems = HEAR_IT_MENU_ITEMS;

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
      if (currentMode == MODE_CW_MENU || currentMode == MODE_BLUETOOTH_MENU ||
          currentMode == MODE_GAMES_MENU ||
          currentMode == MODE_HAM_TOOLS_MENU || currentMode == MODE_SETTINGS_MENU) {
        // Back to main menu
        currentMode = MODE_MAIN_MENU;
        currentSelection = 0;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawMenu();
        return;
      } else if (currentMode == MODE_TRAINING_MENU) {
        // Back to CW menu
        currentMode = MODE_CW_MENU;
        currentSelection = 0;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawMenu();
        return;
      } else if (currentMode == MODE_LICENSE_SELECT) {
        // Back to Ham Tools menu
        currentMode = MODE_HAM_TOOLS_MENU;
        currentSelection = 4;  // License Study position
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawMenu();
        return;
      } else if (currentMode == MODE_DEVICE_SETTINGS_MENU) {
        // Back to settings menu
        currentMode = MODE_SETTINGS_MENU;
        currentSelection = 0;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawMenu();
        return;
      } else if (currentMode == MODE_WIFI_SUBMENU) {
        // Back to device settings menu
        currentMode = MODE_DEVICE_SETTINGS_MENU;
        currentSelection = 0;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawMenu();
        return;
      } else if (currentMode == MODE_GENERAL_SUBMENU) {
        // Back to device settings menu
        currentMode = MODE_DEVICE_SETTINGS_MENU;
        currentSelection = 1;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawMenu();
        return;
      } else if (currentMode == MODE_DEVICE_BT_SUBMENU) {
        // Back to device settings menu
        currentMode = MODE_DEVICE_SETTINGS_MENU;
        currentSelection = 2;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawMenu();
        return;
      } else if (currentMode == MODE_QSO_LOGGER_MENU) {
        // Back to Ham Tools menu
        currentMode = MODE_HAM_TOOLS_MENU;
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
      } else if (currentMode == MODE_BLUETOOTH_MENU) {
        drawMenuItems(bluetoothMenuOptions, bluetoothMenuIcons, BLUETOOTH_MENU_ITEMS);
      } else if (currentMode == MODE_DEVICE_BT_SUBMENU) {
        drawMenuItems(deviceBTSubmenuOptions, deviceBTSubmenuIcons, DEVICE_BT_SUBMENU_ITEMS);
      } else if (currentMode == MODE_LICENSE_SELECT) {
        drawMenuItems(licenseSelectOptions, licenseSelectIcons, LICENSE_SELECT_ITEMS);
      } else if (currentMode == MODE_HEAR_IT_MENU) {
        drawMenuItems(hearItMenuOptions, hearItMenuIcons, HEAR_IT_MENU_ITEMS);
      }
    }
  }
}

#endif // MENU_NAVIGATION_H
