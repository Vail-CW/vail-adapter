/*
 * VAIL SUMMIT - Morse Code Training Device
 * Main program file (refactored for modularity)
 */

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

// Settings modes
#include "settings_wifi.h"
#include "settings_cw.h"
#include "settings_volume.h"
#include "settings_general.h"

// Connectivity
#include "vail_repeater.h"

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
  Serial.println("Starting setup...");

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
  autoConnectWiFi();
  Serial.println("WiFi initialized");
  // NOTE: OTA server starts on-demand when entering firmware update menu

  // Load CW settings from preferences
  Serial.println("Loading CW settings...");
  loadCWSettings();

  // Load saved callsign
  Serial.println("Loading callsign...");
  loadSavedCallsign();

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
}

// ============================================
// Main Loop - Event Processing
// ============================================

void loop() {

  // Update status periodically (NEVER during practice/games mode to avoid audio interference)
  static unsigned long lastStatusUpdate = 0;
  if (currentMode != MODE_PRACTICE &&
      currentMode != MODE_HEAR_IT_TYPE_IT &&
      currentMode != MODE_MORSE_SHOOTER &&
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

  // Check for keyboard input (reduce I2C polling frequency during practice/game modes)
  static unsigned long lastKeyCheck = 0;
  unsigned long keyCheckInterval = (currentMode == MODE_PRACTICE || currentMode == MODE_MORSE_SHOOTER) ? 50 : 10; // Slower polling in practice/game

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

  // Minimal delay in practice, game, and vail modes for better audio/graphics performance
  delay((currentMode == MODE_PRACTICE || currentMode == MODE_MORSE_SHOOTER || currentMode == MODE_VAIL_REPEATER) ? 1 : 10);
}
