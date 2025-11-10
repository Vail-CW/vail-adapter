/*
 * VAIL SUMMIT - Morse Code Training Device
 * Main program file (refactored for modularity)
 */

// Force PSRAM initialization (must be before other includes)
#include "esp_psram.h"
#include "esp_system.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include "config.h"
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_LC709203F.h>
#include <Adafruit_MAX1704X.h>

// Core modules
#include "i2s_audio.h"
#include "morse_code.h"

// Hardware initialization
#include "hardware_init.h"

// Status bar
#include "status_bar.h"

// Menu system
#include "menu_ui.h"
#include "menu_navigation.h"

// Training modes
#include "training_hear_it_type_it.h"
#include "training_practice.h"
#include "training_cwa.h"

// Games
#include "game_morse_shooter.h"
#include "game_morse_memory.h"

// Settings modes
#include "settings_wifi.h"
#include "settings_cw.h"
#include "settings_volume.h"
#include "settings_general.h"

// Connectivity
#include "vail_repeater.h"

// Radio modes (CW memories must be included before radio_output)
#include "radio_cw_memories.h"
#include "radio_output.h"

// NTP Time
#include "ntp_time.h"

// QSO Logger (order matters - validation and API must come first)
#include "qso_logger_validation.h"
#include "pota_api.h"
#include "qso_logger.h"
#include "qso_logger_storage.h"
#include "qso_logger_input.h"
#include "qso_logger_ui.h"
#include "qso_logger_view.h"
#include "qso_logger_statistics.h"
#include "qso_logger_settings.h"

// Web Server (must come after QSO Logger to access storage)
#include "web_server.h"

// Web Practice Mode
#include "web_practice_mode.h"

// ============================================
// Global Hardware Objects
// ============================================

// Battery monitor (one of these will be present)
Adafruit_LC709203F lc;
Adafruit_MAX17048 maxlipo;
bool hasLC709203 = false;
bool hasMAX17048 = false;
bool hasBatteryMonitor = false;

// Create display object
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ============================================
// Menu System State
// ============================================

MenuMode currentMode = MODE_MAIN_MENU;
int currentSelection = 0;
bool menuActive = true;

// ============================================
// Setup - Hardware Initialization
// ============================================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(3000); // Wait for serial monitor to connect
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

  // Manual PSRAM initialization (workaround for ESP32-S3 issue)
  if (psramFound() && ESP.getPsramSize() == 0) {
    Serial.println("\nPSRAM detected but not initialized. Attempting manual init...");
    esp_err_t result = esp_psram_init();
    if (result == ESP_OK) {
      Serial.println("Manual PSRAM init: SUCCESS");
    } else {
      Serial.printf("Manual PSRAM init: FAILED (error code: %d)\n", result);
    }
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

  // Backlight is hardwired to 3.3V (no software control needed)
  Serial.println("Backlight hardwired to 3.3V");

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

  // Initialize GPIO pins
  initPins();

  // Initialize battery monitoring (I2C chip)
  initBatteryMonitor();

  // Initialize WiFi and attempt auto-connect
  Serial.println("Initializing WiFi...");

  // Set up WiFi event handler to auto-start web server
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      Serial.println("WiFi connected! Starting web server...");
      setupWebServer();
    } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      Serial.println("WiFi disconnected. Stopping web server...");
      stopWebServer();
    }
  });

  autoConnectWiFi();
  Serial.println("WiFi initialized");
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

  // Initial status update
  Serial.println("Updating status...");
  updateStatus();

  // Draw initial menu
  Serial.println("Drawing menu...");
  drawMenu();

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

// ============================================
// Main Loop - Event Processing
// ============================================

void loop() {

  // Update status periodically (NEVER during practice/games/radio mode to avoid audio interference)
  static unsigned long lastStatusUpdate = 0;
  if (currentMode != MODE_PRACTICE &&
      currentMode != MODE_HEAR_IT_TYPE_IT &&
      currentMode != MODE_CW_ACADEMY_SENDING_PRACTICE &&
      currentMode != MODE_MORSE_SHOOTER &&
      currentMode != MODE_MORSE_MEMORY &&
      currentMode != MODE_RADIO_OUTPUT &&
      currentMode != MODE_WEB_PRACTICE &&
      millis() - lastStatusUpdate > 5000) { // Update every 5 seconds
    updateStatus();
    // Redraw status icons with new data
    drawStatusIcons();
    lastStatusUpdate = millis();
  }

  // Update practice oscillator if in practice mode
  if (currentMode == MODE_PRACTICE) {
    // Call this frequently to keep audio buffer filled
    // No display updates during practice to maximize audio performance
    updatePracticeOscillator();

    // Update decoded text display when new text is decoded (only if not actively keying)
    if (needsUIUpdate && !isTonePlaying()) {
      drawDecodedTextOnly(tft);
      needsUIUpdate = false;
    }
  }

  // Update CW Academy sending practice (paddle input processing)
  if (currentMode == MODE_CW_ACADEMY_SENDING_PRACTICE) {
    updateCWASendingPractice();
  }

  // Update Vail repeater if in Vail mode
  if (currentMode == MODE_VAIL_REPEATER) {
    updateVailRepeater(tft);
  }

  // Update Morse Shooter game if in game mode
  if (currentMode == MODE_MORSE_SHOOTER) {
    // Input polled every loop for responsiveness
    updateMorseShooterInput(tft);
    // Visuals updated less frequently
    updateMorseShooterVisuals(tft);
  }

  // Update Memory Chain game if in game mode
  if (currentMode == MODE_MORSE_MEMORY) {
    // Update game state machine
    updateMemoryGame();
    // Poll paddle input (same pattern as practice mode)
    bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
    bool dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);
    handleMemoryPaddleInput(ditPressed, dahPressed);

    // Update UI if state changed
    extern bool memoryNeedsUIUpdate;
    if (memoryNeedsUIUpdate) {
      drawMemoryUI(tft);
      memoryNeedsUIUpdate = false;
    }
  }

  // Update Radio Output if in radio output mode
  if (currentMode == MODE_RADIO_OUTPUT) {
    updateRadioOutput();
  }

  // Update Web Practice mode if active
  if (currentMode == MODE_WEB_PRACTICE) {
    updateWebPracticeMode();
    // Also need to process WebSocket events
    extern AsyncWebSocket practiceWebSocket;
    practiceWebSocket.cleanupClients();
  }

  // Check for keyboard input (reduce I2C polling frequency during practice/game/radio modes)
  static unsigned long lastKeyCheck = 0;
  unsigned long keyCheckInterval = (currentMode == MODE_PRACTICE || currentMode == MODE_CW_ACADEMY_SENDING_PRACTICE || currentMode == MODE_MORSE_SHOOTER || currentMode == MODE_MORSE_MEMORY || currentMode == MODE_RADIO_OUTPUT || currentMode == MODE_WEB_PRACTICE) ? 50 : 10; // Slower polling in practice/game/radio/web

  if (millis() - lastKeyCheck >= keyCheckInterval) {
    Wire.requestFrom(CARDKB_ADDR, 1);

    if (Wire.available()) {
      char key = Wire.read();

      if (key != 0) {
        handleKeyPress(key);
      }
    }
    lastKeyCheck = millis();
  }

  // Reset ESC counter if timeout exceeded
  if (escPressCount > 0 && (millis() - lastEscPressTime > TRIPLE_ESC_TIMEOUT)) {
    escPressCount = 0;
  }

  // Minimal delay in practice, game, radio, web, and vail modes for better audio/graphics performance
  delay((currentMode == MODE_PRACTICE || currentMode == MODE_CW_ACADEMY_SENDING_PRACTICE || currentMode == MODE_MORSE_SHOOTER || currentMode == MODE_MORSE_MEMORY || currentMode == MODE_RADIO_OUTPUT || currentMode == MODE_WEB_PRACTICE || currentMode == MODE_VAIL_REPEATER) ? 1 : 10);
}
