/*
 * VAIL SUMMIT - LVGL Mode Integration
 * Connects LVGL screens to the existing mode state machine
 *
 * This module provides the bridge between:
 * - The existing MenuMode enum and currentMode state
 * - The new LVGL-based screen rendering
 * - Input handling delegation
 *
 * NOTE: This file uses int for mode values to avoid circular include dependency
 * with menu_ui.h. The mode values match the MenuMode enum defined there.
 */

#ifndef LV_MODE_INTEGRATION_H
#define LV_MODE_INTEGRATION_H

#include <lvgl.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "lv_screen_manager.h"
#include "lv_menu_screens.h"
#include "lv_settings_screens.h"
#include "lv_training_screens.h"
#include "lv_game_screens.h"
#include "lv_mode_screens.h"
#include "lv_band_conditions.h"
#include "../core/config.h"
#include "../core/hardware_init.h"
#include "../storage/sd_card.h"

// ============================================
// Mode Constants (MUST match MenuMode enum in menu_ui.h)
// ============================================

// We define these as constants instead of using the enum
// to avoid circular include with menu_ui.h
// CRITICAL: These values MUST match the MenuMode enum order exactly!
#define LVGL_MODE_MAIN_MENU              0
#define LVGL_MODE_TRAINING_MENU          1
#define LVGL_MODE_HEAR_IT_MENU           2
#define LVGL_MODE_HEAR_IT_TYPE_IT        3
#define LVGL_MODE_HEAR_IT_CONFIGURE      4
#define LVGL_MODE_HEAR_IT_START          5
#define LVGL_MODE_PRACTICE               6
#define LVGL_MODE_KOCH_METHOD            7
#define LVGL_MODE_CW_ACADEMY_TRACK_SELECT      8
#define LVGL_MODE_CW_ACADEMY_SESSION_SELECT    9
#define LVGL_MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT  10
#define LVGL_MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT   11
#define LVGL_MODE_CW_ACADEMY_COPY_PRACTICE     12
#define LVGL_MODE_CW_ACADEMY_SENDING_PRACTICE  13
#define LVGL_MODE_CW_ACADEMY_QSO_PRACTICE      14
#define LVGL_MODE_GAMES_MENU             15
#define LVGL_MODE_MORSE_SHOOTER          16
#define LVGL_MODE_MORSE_MEMORY           17
#define LVGL_MODE_RADIO_MENU             18
#define LVGL_MODE_RADIO_OUTPUT           19
#define LVGL_MODE_CW_MEMORIES            20
#define LVGL_MODE_SETTINGS_MENU          21
#define LVGL_MODE_DEVICE_SETTINGS_MENU   22
#define LVGL_MODE_WIFI_SUBMENU           23
#define LVGL_MODE_GENERAL_SUBMENU        24
#define LVGL_MODE_WIFI_SETTINGS          25
#define LVGL_MODE_CW_SETTINGS            26
#define LVGL_MODE_VOLUME_SETTINGS        27
#define LVGL_MODE_BRIGHTNESS_SETTINGS    28
#define LVGL_MODE_CALLSIGN_SETTINGS      29
#define LVGL_MODE_WEB_PASSWORD_SETTINGS  30
#define LVGL_MODE_VAIL_REPEATER          31
#define LVGL_MODE_BLUETOOTH_MENU         32
#define LVGL_MODE_BT_HID                 33
#define LVGL_MODE_BT_MIDI                34
#define LVGL_MODE_TOOLS_MENU             35
#define LVGL_MODE_QSO_LOGGER_MENU        36
#define LVGL_MODE_QSO_LOG_ENTRY          37
#define LVGL_MODE_QSO_VIEW_LOGS          38
#define LVGL_MODE_QSO_STATISTICS         39
#define LVGL_MODE_QSO_LOGGER_SETTINGS    40
#define LVGL_MODE_WEB_PRACTICE           41
#define LVGL_MODE_WEB_MEMORY_CHAIN       42
#define LVGL_MODE_WEB_HEAR_IT            43
#define LVGL_MODE_CW_MENU                44
#define LVGL_MODE_HAM_TOOLS_MENU         45
#define LVGL_MODE_BAND_PLANS             46
#define LVGL_MODE_PROPAGATION            47
#define LVGL_MODE_ANTENNAS               48
#define LVGL_MODE_LICENSE_STUDY          49
#define LVGL_MODE_LICENSE_SELECT         50
#define LVGL_MODE_LICENSE_QUIZ           51
#define LVGL_MODE_LICENSE_STATS          52
#define LVGL_MODE_SUMMIT_CHAT            53
#define LVGL_MODE_DEVICE_BT_SUBMENU      54
#define LVGL_MODE_BT_KEYBOARD_SETTINGS   55
#define LVGL_MODE_LICENSE_DOWNLOAD       56
#define LVGL_MODE_LICENSE_WIFI_ERROR     57
#define LVGL_MODE_LICENSE_SD_ERROR       58
#define LVGL_MODE_THEME_SETTINGS         59
#define LVGL_MODE_LICENSE_ALL_STATS      60
#define LVGL_MODE_SYSTEM_INFO            61

// ============================================
// Forward declarations from main file
// ============================================

extern int currentSelection;
extern LGFX tft;

// Note: We access currentMode via extern int
// The actual type is MenuMode but we cast it

// Forward declarations for mode start functions (kept for modes with initialization logic)
extern void startPracticeMode(LGFX& tft);
extern void startVailRepeater(LGFX& tft);
extern void startKochMethod(LGFX& tft);
extern void startCWAcademy(LGFX& tft);
extern void startMorseShooter(LGFX& tft);
extern void loadShooterPrefs();  // Load shooter settings before showing settings screen
extern void startMemoryGame(LGFX& tft);
extern void startRadioOutput(LGFX& tft);
extern void startCWMemoriesMode(LGFX& tft);
extern void startBTHID(LGFX& tft);
extern void startBTMIDI(LGFX& tft);
extern void startWiFiSettings(LGFX& tft);
extern void startCWSettings(LGFX& tft);
extern void initVolumeSettings(LGFX& tft);
extern void startCallsignSettings(LGFX& tft);
extern void startWebPasswordSettings(LGFX& tft);
extern void initBrightnessSettings(LGFX& tft);
extern void startBTKeyboardSettings(LGFX& tft);
extern void startViewLogs(LGFX& tft);
extern void startStatistics(LGFX& tft);
extern void startLoggerSettings(LGFX& tft);
extern void startCWACopyPractice(LGFX& tft);
extern void startCWASendingPractice(LGFX& tft);
extern void startCWAQSOPractice(LGFX& tft);
extern void startWebPracticeMode(LGFX& tft);
extern void startWebMemoryChainMode(LGFX& tft, int difficulty, int mode, int wpm, bool sound, bool hints);
extern void startWebHearItMode(LGFX& tft);
extern void startHearItTypeItMode(LGFX& tft);
extern void startLicenseQuiz(LGFX& tft, int licenseType);
extern void startLicenseStats(LGFX& tft);
extern void updateLicenseQuizDisplay();

// Forward declaration for license session
extern struct LicenseStudySession licenseSession;

// ============================================
// LVGL Configuration
// ============================================

// LVGL is the only UI system - no legacy rendering available

// ============================================
// Mode Category Detection
// ============================================

/*
 * Check if a mode is a menu mode (not an active feature)
 */
bool isMenuModeInt(int mode) {
    switch (mode) {
        case LVGL_MODE_MAIN_MENU:
        case LVGL_MODE_CW_MENU:
        case LVGL_MODE_TRAINING_MENU:
        case LVGL_MODE_GAMES_MENU:
        case LVGL_MODE_SETTINGS_MENU:
        case LVGL_MODE_DEVICE_SETTINGS_MENU:
        case LVGL_MODE_WIFI_SUBMENU:
        case LVGL_MODE_GENERAL_SUBMENU:
        case LVGL_MODE_HAM_TOOLS_MENU:
        case LVGL_MODE_BLUETOOTH_MENU:
        case LVGL_MODE_QSO_LOGGER_MENU:
        case LVGL_MODE_HEAR_IT_MENU:
        case LVGL_MODE_DEVICE_BT_SUBMENU:
        case LVGL_MODE_LICENSE_SELECT:
            return true;
        default:
            return false;
    }
}

/*
 * Check if a mode is a settings mode
 */
bool isSettingsModeInt(int mode) {
    switch (mode) {
        case LVGL_MODE_VOLUME_SETTINGS:
        case LVGL_MODE_BRIGHTNESS_SETTINGS:
        case LVGL_MODE_CW_SETTINGS:
        case LVGL_MODE_CALLSIGN_SETTINGS:
        case LVGL_MODE_WEB_PASSWORD_SETTINGS:
        case LVGL_MODE_WIFI_SETTINGS:
        case LVGL_MODE_BT_KEYBOARD_SETTINGS:
        case LVGL_MODE_THEME_SETTINGS:
        case LVGL_MODE_SYSTEM_INFO:
            return true;
        default:
            return false;
    }
}

/*
 * Check if a mode has special handling
 * NOTE: LVGL handles ALL modes - no legacy rendering
 */
bool useLegacyRenderingInt(int mode) {
    // LVGL handles everything - no legacy rendering
    return false;
}

// ============================================
// Mode-to-Screen Mapping
// ============================================

/*
 * Create the appropriate LVGL screen for a given mode
 * LVGL handles ALL modes - no legacy rendering
 */
lv_obj_t* createScreenForModeInt(int mode) {
    // Menu screens
    if (isMenuModeInt(mode)) {
        switch (mode) {
            case LVGL_MODE_MAIN_MENU:
                return createMainMenuScreen();
            case LVGL_MODE_CW_MENU:
                return createCWMenuScreen();
            case LVGL_MODE_TRAINING_MENU:
                return createTrainingMenuScreen();
            case LVGL_MODE_GAMES_MENU:
                return createGamesMenuScreen();
            case LVGL_MODE_SETTINGS_MENU:
                return createSettingsMenuScreen();
            case LVGL_MODE_DEVICE_SETTINGS_MENU:
                return createDeviceSettingsMenuScreen();
            case LVGL_MODE_WIFI_SUBMENU:
                return createWiFiSubmenuScreen();
            case LVGL_MODE_GENERAL_SUBMENU:
                return createGeneralSubmenuScreen();
            case LVGL_MODE_HAM_TOOLS_MENU:
                return createHamToolsMenuScreen();
            case LVGL_MODE_BLUETOOTH_MENU:
                return createBluetoothMenuScreen();
            case LVGL_MODE_QSO_LOGGER_MENU:
                return createQSOLoggerMenuScreen();
            default:
                break;
        }
    }

    // Settings screens
    if (isSettingsModeInt(mode)) {
        return createSettingsScreenForMode(mode);
    }

    // Training screens - delegate to training screen selector
    lv_obj_t* trainingScreen = createTrainingScreenForMode(mode);
    if (trainingScreen != NULL) return trainingScreen;

    // Game screens - delegate to game screen selector
    lv_obj_t* gameScreen = createGameScreenForMode(mode);
    if (gameScreen != NULL) return gameScreen;

    // Mode screens (network, radio, etc) - delegate to mode screen selector
    lv_obj_t* modeScreen = createModeScreenForMode(mode);
    if (modeScreen != NULL) return modeScreen;

    // Placeholder screens for unimplemented features
    switch (mode) {
        case LVGL_MODE_BAND_PLANS:
            return createComingSoonScreen("BAND PLANS");
        case LVGL_MODE_PROPAGATION:
            return createBandConditionsScreen();
        case LVGL_MODE_ANTENNAS:
            return createComingSoonScreen("ANTENNAS");
        case LVGL_MODE_SUMMIT_CHAT:
            return createComingSoonScreen("SUMMIT CHAT");
        default:
            break;
    }

    // If we get here, create a placeholder screen with mode number
    Serial.printf("[ModeIntegration] No LVGL screen for mode %d, creating placeholder\n", mode);
    char placeholder_title[32];
    snprintf(placeholder_title, sizeof(placeholder_title), "MODE %d", mode);
    return createComingSoonScreen(placeholder_title);
}

// ============================================
// Menu Selection Handler
// ============================================

// External currentMode (declared in menu_navigation.h as MenuMode)
// We access it through a getter/setter approach

// Getter/setter functions to access currentMode
// These will be defined after menu_ui.h is included
extern int getCurrentModeAsInt();
extern void setCurrentModeFromInt(int mode);

/*
 * Initialize mode-specific state after screen is loaded
 * This calls the appropriate start function for modes that need initialization
 * (decoders, audio callbacks, game state, etc.)
 */
void initializeModeInt(int mode) {
    switch (mode) {
        // Training modes
        case LVGL_MODE_PRACTICE:
            Serial.println("[ModeInit] Starting Practice mode");
            startPracticeMode(tft);
            break;
        case LVGL_MODE_KOCH_METHOD:
            Serial.println("[ModeInit] Starting Koch Method");
            startKochMethod(tft);
            break;
        case LVGL_MODE_CW_ACADEMY_TRACK_SELECT:
            Serial.println("[ModeInit] Starting CW Academy");
            startCWAcademy(tft);
            break;
        case LVGL_MODE_CW_ACADEMY_COPY_PRACTICE:
            Serial.println("[ModeInit] Starting CW Academy Copy Practice");
            startCWACopyPractice(tft);
            break;
        case LVGL_MODE_CW_ACADEMY_SENDING_PRACTICE:
            Serial.println("[ModeInit] Starting CW Academy Sending Practice");
            startCWASendingPractice(tft);
            break;
        case LVGL_MODE_CW_ACADEMY_QSO_PRACTICE:
            Serial.println("[ModeInit] Starting CW Academy QSO Practice");
            startCWAQSOPractice(tft);
            break;
        case LVGL_MODE_HEAR_IT_TYPE_IT:
        case LVGL_MODE_HEAR_IT_MENU:
            Serial.println("[ModeInit] Starting Hear It Type It");
            startHearItTypeItMode(tft);
            break;

        // Game modes
        case LVGL_MODE_MORSE_SHOOTER:
            // Just load preferences, game starts when user presses START on settings screen
            Serial.println("[ModeInit] Loading Morse Shooter settings");
            loadShooterPrefs();
            break;
        case LVGL_MODE_MORSE_MEMORY:
            Serial.println("[ModeInit] Starting Memory Game");
            startMemoryGame(tft);
            break;

        // Network/radio modes
        case LVGL_MODE_VAIL_REPEATER:
            Serial.println("[ModeInit] Starting Vail Repeater");
            startVailRepeater(tft);
            break;
        case LVGL_MODE_RADIO_OUTPUT:
            Serial.println("[ModeInit] Starting Radio Output");
            startRadioOutput(tft);
            break;
        case LVGL_MODE_CW_MEMORIES:
            Serial.println("[ModeInit] Starting CW Memories");
            startCWMemoriesMode(tft);
            break;
        case LVGL_MODE_PROPAGATION:
            Serial.println("[ModeInit] Starting Band Conditions");
            startBandConditions(tft);
            break;

        // Bluetooth modes
        case LVGL_MODE_BT_HID:
            Serial.println("[ModeInit] Starting BT HID");
            startBTHID(tft);
            break;
        case LVGL_MODE_BT_MIDI:
            Serial.println("[ModeInit] Starting BT MIDI");
            startBTMIDI(tft);
            break;
        case LVGL_MODE_BT_KEYBOARD_SETTINGS:
            Serial.println("[ModeInit] Starting BT Keyboard Settings");
            startBTKeyboardSettings(tft);
            break;

        // Settings modes
        case LVGL_MODE_WIFI_SETTINGS:
            Serial.println("[ModeInit] Starting WiFi Settings (LVGL)");
            startWiFiSetupLVGL();  // Initialize WiFi setup state
            break;
        case LVGL_MODE_CW_SETTINGS:
            Serial.println("[ModeInit] Starting CW Settings");
            startCWSettings(tft);
            break;
        case LVGL_MODE_VOLUME_SETTINGS:
            Serial.println("[ModeInit] Starting Volume Settings");
            initVolumeSettings(tft);
            break;
        case LVGL_MODE_BRIGHTNESS_SETTINGS:
            Serial.println("[ModeInit] Starting Brightness Settings");
            initBrightnessSettings(tft);
            break;
        case LVGL_MODE_CALLSIGN_SETTINGS:
            Serial.println("[ModeInit] Starting Callsign Settings");
            startCallsignSettings(tft);
            break;
        case LVGL_MODE_WEB_PASSWORD_SETTINGS:
            Serial.println("[ModeInit] Starting Web Password Settings");
            startWebPasswordSettings(tft);
            break;

        // QSO Logger modes
        case LVGL_MODE_QSO_VIEW_LOGS:
            Serial.println("[ModeInit] Starting View Logs");
            startViewLogs(tft);
            break;
        case LVGL_MODE_QSO_STATISTICS:
            Serial.println("[ModeInit] Starting QSO Statistics");
            startStatistics(tft);
            break;
        case LVGL_MODE_QSO_LOGGER_SETTINGS:
            Serial.println("[ModeInit] Starting Logger Settings");
            startLoggerSettings(tft);
            break;

        // Web modes
        case LVGL_MODE_WEB_PRACTICE:
            Serial.println("[ModeInit] Starting Web Practice Mode");
            startWebPracticeMode(tft);
            break;
        case LVGL_MODE_WEB_HEAR_IT:
            Serial.println("[ModeInit] Starting Web Hear It Mode");
            startWebHearItMode(tft);
            break;

        // License study modes
        case LVGL_MODE_LICENSE_SELECT:
            Serial.println("[ModeInit] Starting License Select");
            // Focus first license card for keyboard navigation
            if (license_select_cards[0]) {
                lv_group_focus_obj(license_select_cards[0]);
            }
            break;
        case LVGL_MODE_LICENSE_QUIZ:
            Serial.println("[ModeInit] Starting License Quiz");
            // NOTE: File existence is checked in license_type_select_handler before navigating here
            // If we reach this point, files should already exist on SD card

            // Load questions and start session
            startLicenseQuizLVGL(licenseSession.selectedLicense);
            // Update the LVGL display after loading questions
            updateLicenseQuizDisplay();
            // Focus first answer button for keyboard navigation
            if (license_answer_btns[0]) {
                lv_group_focus_obj(license_answer_btns[0]);
            }
            break;
        case LVGL_MODE_LICENSE_STATS:
            Serial.println("[ModeInit] Starting License Stats");
            // Use LVGL version if available, otherwise call legacy
            startLicenseQuizLVGL(licenseSession.selectedLicense);  // Ensure pool is loaded
            break;
        case LVGL_MODE_LICENSE_DOWNLOAD:
            Serial.println("[ModeInit] Starting License Download");
            // Perform downloads and show progress
            if (performLicenseDownloadsLVGL()) {
                // Downloads succeeded - transition to quiz
                Serial.println("[ModeInit] Downloads complete, transitioning to quiz");
                clearNavigationGroup();
                lv_obj_t* quiz_screen = createLicenseQuizScreen();
                loadScreen(quiz_screen, SCREEN_ANIM_FADE);
                setCurrentModeFromInt(LVGL_MODE_LICENSE_QUIZ);
                startLicenseQuizLVGL(licenseSession.selectedLicense);
                updateLicenseQuizDisplay();
            } else {
                // Downloads failed - add focus widget for ESC navigation
                Serial.println("[ModeInit] Downloads failed, user can press ESC to go back");
            }
            break;
        case LVGL_MODE_LICENSE_WIFI_ERROR:
        case LVGL_MODE_LICENSE_SD_ERROR:
            // Error screens just show message - ESC handled by focus container
            break;

        // Menu modes and others - no initialization needed
        default:
            if (!isMenuModeInt(mode)) {
                Serial.printf("[ModeInit] No init function for mode %d\n", mode);
            }
            break;
    }
}

/*
 * Handler for menu item selection from LVGL menus
 * Called when user selects a menu item
 * All modes are handled by LVGL - no legacy fallback
 */
void onLVGLMenuSelect(int target_mode) {
    Serial.printf("[ModeIntegration] Menu selected mode: %d\n", target_mode);

    // Play selection beep
    beep(TONE_SELECT, BEEP_MEDIUM);

    // Update selection
    currentSelection = 0;

    // Clear navigation group before creating new screen's widgets
    clearNavigationGroup();

    // Create and load LVGL screen for the target mode
    lv_obj_t* screen = createScreenForModeInt(target_mode);

    // Update mode and load screen
    setCurrentModeFromInt(target_mode);
    if (screen != NULL) {
        loadScreen(screen, SCREEN_ANIM_SLIDE_LEFT);

        // Initialize mode-specific state (decoders, audio callbacks, game state)
        initializeModeInt(target_mode);

        // Debug: verify navigation group has widgets
        lv_group_t* group = getLVGLInputGroup();
        Serial.printf("[ModeIntegration] Screen loaded, nav group has %d objects\n",
                      group ? lv_group_get_obj_count(group) : -1);
    } else {
        Serial.printf("[ModeIntegration] WARNING: No screen for mode %d\n", target_mode);
    }
}

// ============================================
// Back Navigation Handler
// ============================================

/*
 * Get the parent mode for a given mode (for back navigation)
 */
int getParentModeInt(int mode) {
    switch (mode) {
        // Main menu has no parent
        case LVGL_MODE_MAIN_MENU:
            return LVGL_MODE_MAIN_MENU;

        // Top-level submenus return to main
        case LVGL_MODE_CW_MENU:
        case LVGL_MODE_GAMES_MENU:
        case LVGL_MODE_HAM_TOOLS_MENU:
        case LVGL_MODE_SETTINGS_MENU:
            return LVGL_MODE_MAIN_MENU;

        // CW submenu items
        case LVGL_MODE_TRAINING_MENU:
        case LVGL_MODE_PRACTICE:
        case LVGL_MODE_VAIL_REPEATER:
        case LVGL_MODE_BLUETOOTH_MENU:
        case LVGL_MODE_RADIO_OUTPUT:
        case LVGL_MODE_CW_MEMORIES:
            return LVGL_MODE_CW_MENU;

        // Training submenu items
        case LVGL_MODE_HEAR_IT_MENU:
        case LVGL_MODE_HEAR_IT_TYPE_IT:
        case LVGL_MODE_HEAR_IT_START:
        case LVGL_MODE_KOCH_METHOD:
        case LVGL_MODE_CW_ACADEMY_TRACK_SELECT:
            return LVGL_MODE_TRAINING_MENU;

        // Hear It submenu items
        case LVGL_MODE_HEAR_IT_CONFIGURE:
            return LVGL_MODE_HEAR_IT_MENU;

        // Games submenu items
        case LVGL_MODE_MORSE_SHOOTER:
        case LVGL_MODE_MORSE_MEMORY:
            return LVGL_MODE_GAMES_MENU;

        // Settings submenu items
        case LVGL_MODE_DEVICE_SETTINGS_MENU:
        case LVGL_MODE_CW_SETTINGS:
            return LVGL_MODE_SETTINGS_MENU;

        // Device settings submenu items
        case LVGL_MODE_WIFI_SUBMENU:
        case LVGL_MODE_GENERAL_SUBMENU:
        case LVGL_MODE_DEVICE_BT_SUBMENU:
        case LVGL_MODE_SYSTEM_INFO:
            return LVGL_MODE_DEVICE_SETTINGS_MENU;

        // WiFi submenu items
        case LVGL_MODE_WIFI_SETTINGS:
        case LVGL_MODE_WEB_PASSWORD_SETTINGS:
            return LVGL_MODE_WIFI_SUBMENU;

        // General submenu items
        case LVGL_MODE_CALLSIGN_SETTINGS:
        case LVGL_MODE_VOLUME_SETTINGS:
        case LVGL_MODE_BRIGHTNESS_SETTINGS:
        case LVGL_MODE_THEME_SETTINGS:
            return LVGL_MODE_GENERAL_SUBMENU;

        // Device BT submenu items
        case LVGL_MODE_BT_KEYBOARD_SETTINGS:
            return LVGL_MODE_DEVICE_BT_SUBMENU;

        // Bluetooth submenu items
        case LVGL_MODE_BT_HID:
        case LVGL_MODE_BT_MIDI:
            return LVGL_MODE_BLUETOOTH_MENU;

        // Ham Tools submenu items
        case LVGL_MODE_QSO_LOGGER_MENU:
        case LVGL_MODE_BAND_PLANS:
        case LVGL_MODE_PROPAGATION:
        case LVGL_MODE_ANTENNAS:
        case LVGL_MODE_LICENSE_SELECT:
        case LVGL_MODE_SUMMIT_CHAT:
            return LVGL_MODE_HAM_TOOLS_MENU;

        // QSO Logger submenu items
        case LVGL_MODE_QSO_LOG_ENTRY:
        case LVGL_MODE_QSO_VIEW_LOGS:
        case LVGL_MODE_QSO_STATISTICS:
        case LVGL_MODE_QSO_LOGGER_SETTINGS:
            return LVGL_MODE_QSO_LOGGER_MENU;

        // CW Academy hierarchy
        case LVGL_MODE_CW_ACADEMY_SESSION_SELECT:
            return LVGL_MODE_CW_ACADEMY_TRACK_SELECT;
        case LVGL_MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT:
            return LVGL_MODE_CW_ACADEMY_SESSION_SELECT;
        case LVGL_MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT:
            return LVGL_MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT;
        case LVGL_MODE_CW_ACADEMY_COPY_PRACTICE:
        case LVGL_MODE_CW_ACADEMY_SENDING_PRACTICE:
            return LVGL_MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT;
        case LVGL_MODE_CW_ACADEMY_QSO_PRACTICE:
            return LVGL_MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT;

        // License submenu items
        case LVGL_MODE_LICENSE_QUIZ:
        case LVGL_MODE_LICENSE_STATS:
        case LVGL_MODE_LICENSE_DOWNLOAD:
        case LVGL_MODE_LICENSE_WIFI_ERROR:
        case LVGL_MODE_LICENSE_SD_ERROR:
        case LVGL_MODE_LICENSE_ALL_STATS:
            return LVGL_MODE_LICENSE_SELECT;

        default:
            return LVGL_MODE_MAIN_MENU;
    }
}

/*
 * Handle back navigation from LVGL screens
 * All navigation is handled by LVGL - no legacy fallback
 */
void onLVGLBackNavigation() {
    int currentModeInt = getCurrentModeAsInt();
    Serial.printf("[ModeIntegration] Back navigation from mode: %d\n", currentModeInt);

    // Play navigation beep
    beep(TONE_MENU_NAV, BEEP_SHORT);

    // Mode-specific cleanup before leaving
    if (currentModeInt == LVGL_MODE_PROPAGATION) {
        cleanupBandConditions();
    }
    if (currentModeInt == LVGL_MODE_WIFI_SETTINGS) {
        cleanupWiFiScreen();
    }
    if (currentModeInt == LVGL_MODE_BT_HID) {
        cleanupBTHIDScreen();
    }
    if (currentModeInt == LVGL_MODE_HEAR_IT_TYPE_IT ||
        currentModeInt == LVGL_MODE_HEAR_IT_MENU) {
        cleanupHearItTypeItScreen();
    }

    // Get parent mode
    int parentMode = getParentModeInt(currentModeInt);

    if (parentMode == currentModeInt) {
        // Already at top level, ignore or handle deep sleep triple-ESC
        return;
    }

    // Update mode and selection
    setCurrentModeFromInt(parentMode);
    currentSelection = 0;

    // Clear navigation group before creating new screen's widgets
    clearNavigationGroup();

    // Create and load parent screen
    lv_obj_t* screen = createScreenForModeInt(parentMode);
    if (screen != NULL) {
        loadScreen(screen, SCREEN_ANIM_SLIDE_RIGHT);

        // Debug: verify navigation group has widgets
        lv_group_t* group = getLVGLInputGroup();
        Serial.printf("[ModeIntegration] Parent screen loaded, nav group has %d objects\n",
                      group ? lv_group_get_obj_count(group) : -1);
    } else {
        Serial.printf("[ModeIntegration] WARNING: No parent screen for mode %d\n", parentMode);
    }
}

// ============================================
// Initialization
// ============================================

/*
 * Initialize LVGL mode integration
 * Call this after LVGL and theme are initialized
 */
void initLVGLModeIntegration() {
    Serial.println("[ModeIntegration] Initializing LVGL mode integration");

    // Set up menu selection callback
    setMenuSelectCallback(onLVGLMenuSelect);

    // Set up back navigation callback
    setBackCallback(onLVGLBackNavigation);

    Serial.println("[ModeIntegration] Mode integration initialized");
}

/*
 * Show the initial LVGL screen (main menu)
 */
void showInitialLVGLScreen() {
    Serial.println("[ModeIntegration] Loading initial LVGL screen (main menu)");

    // Clear any widgets from splash screen before creating menu
    clearNavigationGroup();

    lv_obj_t* main_menu = createMainMenuScreen();
    if (main_menu != NULL) {
        loadScreen(main_menu, SCREEN_ANIM_NONE);
        setCurrentModeFromInt(LVGL_MODE_MAIN_MENU);
        currentSelection = 0;

        // Debug: verify navigation group has widgets
        lv_group_t* group = getLVGLInputGroup();
        Serial.printf("[ModeIntegration] Main menu loaded, nav group has %d objects\n",
                      group ? lv_group_get_obj_count(group) : -1);
    } else {
        Serial.println("[ModeIntegration] CRITICAL: Failed to create main menu screen!");
    }
}

/*
 * Check if LVGL mode is enabled
 * Note: LVGL is now the only UI system - always returns true
 */
bool isLVGLModeEnabled() {
    return true;  // LVGL is always enabled - no legacy UI
}

// ============================================
// Dynamic Screen Updates
// ============================================

/*
 * Refresh the current LVGL screen based on mode
 * Call this when mode state changes need to be reflected in UI
 */
void refreshCurrentLVGLScreen() {
    int currentModeInt = getCurrentModeAsInt();
    lv_obj_t* screen = createScreenForModeInt(currentModeInt);
    if (screen != NULL) {
        loadScreen(screen, SCREEN_ANIM_NONE);
    }
}

/*
 * Update specific UI elements without full screen reload
 * Used for real-time updates in training/game modes
 */
void updateLVGLModeUI() {
    // The individual screen modules provide update functions
    // This function can be extended to call those based on mode
    // For now, specific updates are called directly from the mode handlers
}

#endif // LV_MODE_INTEGRATION_H
