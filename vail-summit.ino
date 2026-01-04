/*
 * VAIL SUMMIT - Morse Code Training Device
 * Main program file (refactored for modularity)
 */

// Note: PSRAM is enabled via Arduino IDE board settings (PSRAM: "OPI PSRAM")
// No explicit PSRAM initialization code needed for Arduino ESP32 core 2.0.14

// LovyanGFX for ST7796S 4.0" display (requires Arduino ESP32 core 2.0.14)
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// LVGL Graphics Library
// Uses lv_conf.h from Arduino/libraries folder (next to lvgl folder)
#include <lvgl.h>

// Use LovyanGFX built-in fonts (in lgfx::v1::fonts namespace)
// Available: FreeSansBold9pt7b, FreeSansBold12pt7b, FreeSansBold18pt7b, etc.
using namespace lgfx::v1::fonts;

// Standard libraries
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <WiFi.h>

// Configuration and battery monitoring
#include "src/core/config.h"
#include <Adafruit_LC709203F.h>
#include <Adafruit_MAX1704X.h>

// Core modules
#include "src/audio/i2s_audio.h"
#include "src/core/morse_code.h"
#include "src/core/task_manager.h"

// Hardware initialization (defines LGFX class, must come before LVGL modules)
#include "src/core/hardware_init.h"

// LVGL modules (must come after hardware_init.h for LGFX type)
#include "src/lvgl/lv_init.h"
#include "src/lvgl/lv_screen_manager.h"
#include "src/lvgl/lv_theme_summit.h"
#include "src/lvgl/lv_widgets_summit.h"
#include "src/lvgl/lv_menu_screens.h"

// LVGL is the only UI system - no legacy rendering

// Boot splash screen (LVGL version)
#include "src/lvgl/lv_splash_screen.h"

// Status bar
#include "src/ui/status_bar.h"

// Menu system
#include "src/ui/menu_ui.h"

// Training modes
#include "src/training/training_hear_it_type_it.h"
#include "src/training/training_practice.h"
#include "src/training/training_koch_method.h"
#include "src/training/training_cwa.h"
#include "src/training/training_license_ui.h"
#include "src/training/training_license_input.h"
#include "src/training/training_vail_master.h"

// Games
#include "src/games/game_morse_shooter.h"
#include "src/games/game_morse_memory.h"

// Settings modes
#include "src/settings/settings_wifi.h"
#include "src/settings/settings_cw.h"
#include "src/settings/settings_volume.h"
#include "src/settings/settings_brightness.h"
#include "src/settings/settings_general.h"
#include "src/settings/settings_web_password.h"

// Connectivity
#include "src/network/vail_repeater.h"

// Radio modes (CW memories must be included before radio_output)
#include "src/radio/radio_cw_memories.h"
#include "src/radio/radio_output.h"

// NTP Time
#include "src/network/ntp_time.h"

// QSO Logger (order matters - validation and API must come first)
#include "src/qso/qso_logger_validation.h"
#include "src/network/pota_api.h"
#include "src/qso/qso_logger.h"
#include "src/qso/qso_logger_storage.h"
#include "src/qso/qso_logger_input.h"
#include "src/qso/qso_logger_ui.h"
#include "src/qso/qso_logger_view.h"
#include "src/qso/qso_logger_statistics.h"
#include "src/qso/qso_logger_settings.h"

// Web Server (must come after QSO Logger to access storage)
#include "src/web/server/web_server.h"

// Web Practice Mode
#include "src/web/modes/web_practice_mode.h"

// Web Memory Chain Mode
#include "src/web/modes/web_memory_chain_mode.h"

// Web Hear It Type It Mode
#include "src/web/modes/web_hear_it_mode.h"

// SD Card Storage
#include "src/storage/sd_card.h"

// Web First Boot (must come after sd_card.h)
#include "src/web/server/web_first_boot.h"

// Bluetooth modes
#include "src/bluetooth/ble_hid.h"
#include "src/bluetooth/ble_midi.h"

// Bluetooth keyboard host (external keyboard input)
#include "src/settings/settings_bt_keyboard.h"

// Menu navigation (must come after all mode headers that define input handlers)
#include "src/ui/menu_navigation.h"

// Additional LVGL screen modules (must come after settings modules for type access)
#include "src/lvgl/lv_settings_screens.h"
#include "src/lvgl/lv_training_screens.h"
#include "src/lvgl/lv_game_screens.h"
#include "src/lvgl/lv_mode_screens.h"
#include "src/lvgl/lv_mode_integration.h"

// ============================================
// cwKeyType accessor functions for LVGL (needed because cwKeyType is KeyType enum)
// ============================================
int getCwKeyTypeAsInt() { return (int)cwKeyType; }
void setCwKeyTypeFromInt(int keyType) { cwKeyType = (KeyType)keyType; }

// ============================================
// Global Hardware Objects
// ============================================

// Battery monitor (one of these will be present)
Adafruit_LC709203F lc;
Adafruit_MAX17048 maxlipo;
bool hasLC709203 = false;
bool hasMAX17048 = false;
bool hasBatteryMonitor = false;

// Create display object (LovyanGFX configured in hardware_init.h)
LGFX tft;

// ============================================
// Menu System State
// ============================================

MenuMode currentMode = MODE_MAIN_MENU;
int currentSelection = 0;
bool menuActive = true;

// Getter/setter for currentMode to allow LVGL integration without circular includes
int getCurrentModeAsInt() { return (int)currentMode; }
void setCurrentModeFromInt(int mode) { currentMode = (MenuMode)mode; }

// ============================================
// Setup - Hardware Initialization
// ============================================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500); // Brief wait for serial monitor (reduced from 3000ms for faster boot)
  Serial.println("\n\n=== VAIL SUMMIT STARTING ===");
  Serial.printf("Firmware: %s (Build: %s)\n", FIRMWARE_VERSION, FIRMWARE_DATE);
  Serial.println("Starting setup...");

  // ============================================
  // PSRAM Diagnostic - Run First
  // ============================================
  Serial.println("\n--- PSRAM Diagnostic ---");
  Serial.printf("ESP32 Chip Model: %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());

  Serial.println("\nChecking PSRAM (before manual init)...");
  Serial.printf("psramFound(): %s\n", psramFound() ? "true" : "false");
  Serial.printf("ESP.getPsramSize(): %d bytes\n", ESP.getPsramSize());

  // PSRAM is auto-initialized when "OPI PSRAM" is selected in Arduino IDE
  // Manual initialization (esp_psram_init) is not available in Arduino ESP32 core 2.0.14
  if (psramFound() && ESP.getPsramSize() == 0) {
    Serial.println("\nWARNING: PSRAM detected but reports 0 bytes size.");
    Serial.println("This may indicate a board configuration issue.");
  }

  Serial.println("\nRechecking PSRAM (after manual init)...");
  Serial.printf("psramFound(): %s\n", psramFound() ? "true" : "false");
  Serial.printf("ESP.getPsramSize(): %d bytes\n", ESP.getPsramSize());
  Serial.printf("ESP.getFreePsram(): %d bytes\n", ESP.getFreePsram());
  Serial.printf("ESP.getMinFreePsram(): %d bytes\n", ESP.getMinFreePsram());

  // Try a test allocation
  if (psramFound()) {
    Serial.println("\nPSRAM found! Testing allocation...");
    void* testPtr = ps_malloc(1024);
    if (testPtr) {
      Serial.println("PSRAM test allocation: SUCCESS");
      free(testPtr);
    } else {
      Serial.println("PSRAM test allocation: FAILED");
    }
  } else {
    Serial.println("\nWARNING: PSRAM NOT DETECTED!");
    Serial.println("Possible causes:");
    Serial.println("  1. PSRAM not enabled in Arduino IDE (Tools > PSRAM)");
    Serial.println("  2. Wrong board selected (should be ESP32-S3 variant)");
    Serial.println("  3. Hardware doesn't have PSRAM");
    Serial.println("  4. ESP32 Arduino core version issue");
  }
  Serial.println("--- End PSRAM Diagnostic ---\n");

  // Initialize backlight control with PWM (GPIO 39)
  Serial.println("Initializing backlight PWM control on GPIO 39...");
  setupBrightnessPWM();
  loadBrightnessSettings();
  applyBrightness(brightnessValue);
  Serial.print("Backlight enabled at ");
  Serial.print(brightnessValue);
  Serial.println("% brightness");

  // Initialize I2C first (before display to avoid conflicts)
  Serial.println("Initializing I2C...");
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  // Initialize I2S Audio BEFORE display (critical for DMA priority)
  Serial.println("\nInitializing I2S audio...");
  initI2SAudio();
  delay(100);

  // Initialize LCD (after I2S to avoid DMA conflicts)
  initDisplay();

  // Initialize LVGL library with display and input drivers
  Serial.println("Initializing LVGL...");
  if (!initLVGL(tft)) {
    Serial.println("CRITICAL ERROR: LVGL initialization failed!");
    // Can't continue without LVGL - display error and halt
    tft.fillScreen(0xF800);  // Red background
    tft.setTextColor(0xFFFF);
    tft.setCursor(10, 100);
    tft.println("LVGL INIT FAILED");
    while (1) delay(1000);
  }
  Serial.println("LVGL initialized successfully");
  // Initialize theme manager (must be before initSummitTheme)
  initThemeManager();
  // Load saved theme preference
  loadThemeSettings();
  // Initialize VAIL SUMMIT theme styles (uses active theme colors)
  initSummitTheme();
  // Initialize mode integration (menu callbacks, back navigation)
  initLVGLModeIntegration();

  // Set up FreeRTOS task manager (audio on Core 0, UI on Core 1)
  Serial.println("Setting up task manager...");
  setupTaskManager();

  // Show LVGL boot splash screen immediately for visual feedback
  showSplashScreen();
  setSplashStage(1);  // "Initializing I2C..."

  // Initialize GPIO pins
  initPins();
  setSplashStage(2);  // "Starting audio..."

  // SD card initialization moved to on-demand (when storage page is accessed)
  // This avoids SPI bus conflicts with display during boot
  Serial.println("SD card will be initialized on first access");

  // Initialize battery monitoring (I2C chip)
  initBatteryMonitor();
  setSplashStage(3);  // "Loading settings..."

  // Initialize WiFi and attempt auto-connect
  Serial.println("Initializing WiFi...");

  // Set up WiFi event handler to auto-start web server
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      Serial.println("WiFi connected! Starting web server...");
      setupWebServer();
      // Check if we should prompt for web files download
      checkAndShowWebFilesPrompt();
    } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      Serial.println("WiFi disconnected. Stopping web server...");
      stopWebServer();
    }
  });

  // Enable auto-reconnect to saved networks
  WiFi.setAutoReconnect(true);

  autoConnectWiFi();
  Serial.println("WiFi initialized");

  // Force initial internet connectivity check after WiFi connects
  if (WiFi.status() == WL_CONNECTED) {
    forceInternetCheck();
    updateInternetStatus();  // Run first check immediately
    Serial.printf("Internet status: %s\n", getInternetStatusString());
  }

  setSplashStage(4);  // "Configuring WiFi..."
  // NOTE: OTA server starts on-demand when entering firmware update menu

  // Load CW settings from preferences
  Serial.println("Loading CW settings...");
  loadCWSettings();

  // Load radio settings from preferences
  Serial.println("Loading radio settings...");
  loadRadioSettings();

  // Load CW memories from preferences
  Serial.println("Loading CW memories...");
  loadCWMemories();

  // Load saved callsign
  Serial.println("Loading callsign...");
  loadSavedCallsign();

  // Load saved web password
  Serial.println("Loading web password...");
  loadSavedWebPassword();
  setSplashStage(5);  // "Starting web server..."

  // Load Koch Method progress
  Serial.println("Loading Koch Method progress...");
  loadKochProgress();

  // Initialize NTP time (if WiFi connected)
  Serial.println("Initializing NTP time...");
  initNTPTime();

  // Initialize SPIFFS for QSO Logger
  Serial.println("Initializing storage...");
  if (!initStorage()) {
    Serial.println("WARNING: Storage initialization failed!");
  }

  // Load QSO Logger operator settings
  Serial.println("Loading QSO Logger settings...");
  loadOperatorSettings();
  setSplashStage(6);  // "Initializing UI..."

  // Load BLE keyboard settings (for auto-reconnect on boot)
  Serial.println("Loading BLE keyboard settings...");
  loadBLEKeyboardSettings();

  // Initial status update
  Serial.println("Updating status...");
  updateStatus();
  setSplashStage(7);  // "Almost ready..."

  // Brief pause to show 100% complete
  setSplashStage(8);  // "Ready!"
  delay(300);  // Brief pause to show completion

  // Clean up splash screen and show initial LVGL menu
  Serial.println("Drawing LVGL menu...");
  cleanupSplashScreen();  // Free splash screen resources
  showInitialLVGLScreen();

  Serial.println("Setup complete!");

  // Startup beep (test I2S audio)
  Serial.println("\nTesting I2S audio output...");
  Serial.println("You should hear a 1000 Hz beep now");
  beep(TONE_STARTUP, BEEP_MEDIUM);
  delay(200);

  // Second test beep
  Serial.println("Second test beep at 700 Hz");
  beep(700, 300);
  Serial.println("Audio test complete\n");

  // Optionally test QSO storage (uncomment to test)
  // testSaveDummyQSO();
}

// Helper function to read keyboard (used by first-boot prompt)
char readKeyboardNonBlocking() {
  Wire.requestFrom(CARDKB_ADDR, 1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}

// ============================================
// Main Loop - Event Processing (LVGL-Only)
// ============================================

void loop() {

  // Process LVGL timer tasks (handles ALL UI, input, animations, redraws)
  // LVGL reads CardKB directly via its input driver
  lv_timer_handler();

  // Process any pending deferred screen operations
  // (screen creation is deferred from event callbacks to avoid stack overflow)
  processQSOViewLogsPending();

  // Poll non-blocking WiFi connection state machine
  // This must be called every loop to check connection progress
  if (updateWiFiConnection()) {
    // State changed - if we're in WiFi settings, update the UI
    if (currentMode == MODE_WIFI_SETTINGS) {
      updateWiFiScreen();
    }
  }

  // Check for pending web server restart (after file upload)
  if (isWebServerRestartPending()) {
    restartWebServer();
  }

  // Update status data periodically (NEVER during audio-critical modes)
  // Note: LVGL screens will read this data when they refresh
  static unsigned long lastStatusUpdate = 0;
  if (currentMode != MODE_PRACTICE &&
      currentMode != MODE_HEAR_IT_TYPE_IT &&
      currentMode != MODE_KOCH_METHOD &&
      currentMode != MODE_CW_ACADEMY_SENDING_PRACTICE &&
      currentMode != MODE_MORSE_SHOOTER &&
      currentMode != MODE_MORSE_MEMORY &&
      currentMode != MODE_RADIO_OUTPUT &&
      currentMode != MODE_WEB_PRACTICE &&
      currentMode != MODE_WEB_MEMORY_CHAIN &&
      currentMode != MODE_BT_HID &&
      currentMode != MODE_BT_MIDI &&
      millis() - lastStatusUpdate > 5000) {
    updateStatus();
    lastStatusUpdate = millis();
  }

  // Update practice oscillator if in practice mode
  if (currentMode == MODE_PRACTICE) {
    updatePracticeOscillator();

    // Update LVGL decoded text display when new text is decoded
    // Only update when not actively playing tone to avoid audio glitches
    if (needsUIUpdate && !isTonePlaying()) {
      updatePracticeDecoderDisplay(decodedText.c_str());
      needsUIUpdate = false;
    }
  }

  // Update CW Academy sending practice (paddle input processing)
  if (currentMode == MODE_CW_ACADEMY_SENDING_PRACTICE) {
    updateCWASendingPractice();

    // Update decoded text display when new text is decoded
    if (cwaSendNeedsUIUpdate && !isTonePlaying()) {
      drawCWASendDecodedOnly(tft);
      cwaSendNeedsUIUpdate = false;
    }
  }

  // Update Vail repeater if in Vail mode
  if (currentMode == MODE_VAIL_REPEATER) {
    updateVailRepeater(tft);
    updateVailScreenLVGL();  // Update LVGL UI elements
  }

  // Update Morse Shooter game if in game mode
  if (currentMode == MODE_MORSE_SHOOTER) {
    updateMorseShooterInput(tft);
    updateMorseShooterVisuals(tft);
  }

  // Update Memory Chain game if in game mode
  if (currentMode == MODE_MORSE_MEMORY) {
    updateMemoryGame();
    bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
    bool dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);
    handleMemoryPaddleInput(ditPressed, dahPressed);
  }

  // Update Vail Master if in practice mode
  // Note: Use LVGL mode constant since that's what's set by mode integration
  if (currentMode == LVGL_MODE_VAIL_MASTER_PRACTICE) {
    vmUpdateKeyer();
  }

  // Update Radio Output if in radio output mode
  if (currentMode == MODE_RADIO_OUTPUT) {
    updateRadioOutput();
  }

  // Update Web Practice mode if active
  if (currentMode == MODE_WEB_PRACTICE) {
    updateWebPracticeMode();
    extern AsyncWebSocket practiceWebSocket;
    practiceWebSocket.cleanupClients();
  }

  // Update Web Memory Chain mode if active
  if (currentMode == MODE_WEB_MEMORY_CHAIN) {
    updateWebMemoryChain();
    extern AsyncWebSocket memoryChainWebSocket;
    memoryChainWebSocket.cleanupClients();
  }

  // Update Web Hear It Type It mode if active
  if (currentMode == MODE_WEB_HEAR_IT) {
    updateWebHearItMode();
    extern AsyncWebSocket hearItWebSocket;
    hearItWebSocket.cleanupClients();
  }

  // Update BT HID mode if active
  if (currentMode == MODE_BT_HID) {
    updateBTHID();
  }

  // Update BT MIDI mode if active
  if (currentMode == MODE_BT_MIDI) {
    updateBTMIDI();
  }

  // Update BLE Keyboard host (for auto-reconnect, etc.)
  if (currentMode != MODE_BT_HID && currentMode != MODE_BT_MIDI) {
    updateBLEKeyboardHost();
  }

  // Minimal delay - faster for audio-critical modes
  delay((currentMode == MODE_PRACTICE || currentMode == MODE_CW_ACADEMY_SENDING_PRACTICE ||
         currentMode == MODE_MORSE_SHOOTER || currentMode == MODE_MORSE_MEMORY ||
         currentMode == MODE_RADIO_OUTPUT || currentMode == MODE_WEB_PRACTICE ||
         currentMode == MODE_VAIL_REPEATER || currentMode == MODE_BT_HID ||
         currentMode == MODE_BT_MIDI || currentMode == LVGL_MODE_VAIL_MASTER_PRACTICE) ? 1 : 10);
}
