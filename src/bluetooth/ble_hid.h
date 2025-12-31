/*
 * BLE HID Mode
 * Emulates a BLE keyboard sending Left/Right Ctrl keys for paddle input
 * Compatible with MorseRunner and other CW tools expecting keyboard input
 * Uses NimBLE-Arduino library with NimBLEHIDDevice helper class
 *
 * Keyer Modes:
 * - Passthrough: Raw paddle → immediate key press/release (host handles timing)
 * - Straight Key: Either paddle → single Left Ctrl key
 * - Iambic A/B: Full timed sequences with proper dit/dah timing
 */

#ifndef BLE_HID_H
#define BLE_HID_H

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <Preferences.h>
#include "ble_core.h"
#include "../core/config.h"
#include "../audio/i2s_audio.h"

// HID constants
#define HID_KEYBOARD_APPEARANCE    0x03C1
#define KEYBOARD_REPORT_ID         0x01

// HID Report Descriptor for keyboard
// Standard keyboard with Report ID 1
static const uint8_t hidReportDescriptor[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x06,        // Usage (Keyboard)
  0xa1, 0x01,        // Collection (Application)
  0x85, KEYBOARD_REPORT_ID, //   Report ID (1)
  0x05, 0x07,        //   Usage Page (Key Codes)
  0x19, 0xe0,        //   Usage Minimum (224) - Left Ctrl
  0x29, 0xe7,        //   Usage Maximum (231) - Right GUI
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x08,        //   Report Count (8)
  0x81, 0x02,        //   Input (Data, Variable, Absolute) - Modifier byte
  0x95, 0x01,        //   Report Count (1)
  0x75, 0x08,        //   Report Size (8)
  0x81, 0x01,        //   Input (Constant) - Reserved byte
  0x95, 0x06,        //   Report Count (6)
  0x75, 0x08,        //   Report Size (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x65,        //   Logical Maximum (101)
  0x05, 0x07,        //   Usage Page (Key Codes)
  0x19, 0x00,        //   Usage Minimum (0)
  0x29, 0x65,        //   Usage Maximum (101)
  0x81, 0x00,        //   Input (Data, Array) - Key array
  0xc0               // End Collection
};

// Keyboard report structure (8 bytes, no Report ID prefix)
// Report ID is handled by the characteristic descriptor
struct KeyboardReport {
  uint8_t modifiers;   // Modifier keys (bit 0=Left Ctrl, bit 4=Right Ctrl)
  uint8_t reserved;    // Reserved byte (always 0)
  uint8_t keys[6];     // Key array (up to 6 simultaneous keys)
};

// HID modifier key bits
#define KEY_MOD_LCTRL  0x01  // Left Control
#define KEY_MOD_RCTRL  0x10  // Right Control

// BT HID Keyer Modes
enum BTHIDKeyerMode {
  BT_HID_PASSTHROUGH = 0,  // Raw paddle → immediate key press/release
  BT_HID_STRAIGHT,         // Either paddle → single Left Ctrl
  BT_HID_IAMBIC_A,         // Full iambic A keying
  BT_HID_IAMBIC_B          // Full iambic B keying (with squeeze alternation)
};

// Number of keyer modes
#define BT_HID_KEYER_MODE_COUNT 4

// Keyer mode names for display
static const char* btHIDKeyerModeNames[] = {
  "Passthrough",
  "Straight Key",
  "Iambic A",
  "Iambic B"
};

// BLE HID state
struct BLEHIDState {
  bool active = false;
  bool lastDitPressed = false;
  bool lastDahPressed = false;
  NimBLEHIDDevice* hid = nullptr;          // NimBLEHIDDevice helper class
  NimBLECharacteristic* inputReport = nullptr;  // Input report characteristic
  unsigned long lastUpdateTime = 0;

  // Keyer mode
  BTHIDKeyerMode keyerMode = BT_HID_PASSTHROUGH;

  // Iambic keyer state machine
  bool keyerActive = false;      // Currently sending an element
  bool inSpacing = false;        // In inter-element spacing
  bool sendingDit = false;       // Current element is dit (vs dah)
  bool ditMemory = false;        // Dit paddle was pressed during element
  bool dahMemory = false;        // Dah paddle was pressed during element
  unsigned long elementTimer = 0; // When current element/spacing ends
  unsigned long ditDuration = 60; // Dit duration in ms (calculated from cwSpeed)

  // Current key state (for proper key up/down)
  bool isKeying = false;         // Currently holding a key down
  uint8_t currentModifier = 0;   // Which modifier key is held
};

BLEHIDState btHID;

// Preferences for BT HID settings
static Preferences btHIDPrefs;

// Track previous connection state for UI updates
static BLEConnectionState lastBTHIDState = BLE_STATE_OFF;

// External CW speed setting
extern int cwSpeed;

// Forward declarations
void startBTHID(LGFX& display);
void drawBTHIDUI(LGFX& display);
int handleBTHIDInput(char key, LGFX& display);
void updateBTHID();
void sendHIDReport(uint8_t modifiers);
void stopBTHID();
void cycleBTHIDKeyerMode(int direction);
const char* getBTHIDKeyerModeName();
void loadBTHIDSettings();
void saveBTHIDSettings();

// Forward declarations for LVGL UI updates (defined in lv_mode_screens.h)
extern void updateBTHIDStatus(const char* status, bool connected);
extern void updateBTHIDDeviceName(const char* name);
extern void updateBTHIDPaddleIndicators(bool ditPressed, bool dahPressed);
extern void updateBTHIDKeyerMode(const char* mode);
extern void cleanupBTHIDScreen();

// ============================================
// Settings Persistence
// ============================================

void loadBTHIDSettings() {
  btHIDPrefs.begin("bthid", true);  // Read-only
  btHID.keyerMode = (BTHIDKeyerMode)btHIDPrefs.getInt("keyermode", BT_HID_PASSTHROUGH);
  btHIDPrefs.end();
  Serial.printf("[BT HID] Loaded keyer mode: %s\n", btHIDKeyerModeNames[btHID.keyerMode]);
}

void saveBTHIDSettings() {
  btHIDPrefs.begin("bthid", false);  // Read-write
  btHIDPrefs.putInt("keyermode", (int)btHID.keyerMode);
  btHIDPrefs.end();
  Serial.printf("[BT HID] Saved keyer mode: %s\n", btHIDKeyerModeNames[btHID.keyerMode]);
}

// ============================================
// Keyer Mode Functions
// ============================================

const char* getBTHIDKeyerModeName() {
  return btHIDKeyerModeNames[btHID.keyerMode];
}

void cycleBTHIDKeyerMode(int direction) {
  int mode = (int)btHID.keyerMode;
  if (direction > 0) {
    mode = (mode + 1) % BT_HID_KEYER_MODE_COUNT;
  } else {
    mode = (mode - 1 + BT_HID_KEYER_MODE_COUNT) % BT_HID_KEYER_MODE_COUNT;
  }
  btHID.keyerMode = (BTHIDKeyerMode)mode;

  // Reset keyer state when changing modes
  btHID.keyerActive = false;
  btHID.inSpacing = false;
  btHID.ditMemory = false;
  btHID.dahMemory = false;
  if (btHID.isKeying) {
    sendHIDReport(0x00);  // Release any held key
    btHID.isKeying = false;
    stopTone();
  }

  // Update UI and save
  updateBTHIDKeyerMode(getBTHIDKeyerModeName());
  saveBTHIDSettings();

  Serial.printf("[BT HID] Keyer mode changed to: %s\n", getBTHIDKeyerModeName());
}

// Send HID keyboard report with modifiers
void sendHIDReport(uint8_t modifiers) {
  if (!btHID.active || btHID.inputReport == nullptr) return;
  if (!isBLEConnected()) return;

  // Build HID keyboard report (8 bytes, no Report ID prefix)
  // The inputReport characteristic handles Report ID via descriptor
  KeyboardReport report;
  memset(&report, 0, sizeof(report));
  report.modifiers = modifiers;  // Modifier keys (Left Ctrl=0x01, Right Ctrl=0x10)
  report.reserved = 0x00;
  // report.keys[] = all zeros (no regular keys pressed)

  btHID.inputReport->setValue((uint8_t*)&report, sizeof(report));
  btHID.inputReport->notify();

  Serial.print("[BT HID] Sent report: Modifiers=0x");
  Serial.println(modifiers, HEX);
}

// Start BT HID mode
void startBTHID(LGFX& display) {
  Serial.println("Starting BT HID mode");

  // Load saved keyer mode
  loadBTHIDSettings();

  btHID.active = true;
  btHID.lastDitPressed = false;
  btHID.lastDahPressed = false;
  btHID.lastUpdateTime = millis();
  lastBTHIDState = BLE_STATE_OFF;  // Reset state tracking

  // Reset iambic keyer state
  btHID.keyerActive = false;
  btHID.inSpacing = false;
  btHID.sendingDit = false;
  btHID.ditMemory = false;
  btHID.dahMemory = false;
  btHID.isKeying = false;
  btHID.currentModifier = 0;

  // Calculate dit duration from CW speed (PARIS standard)
  btHID.ditDuration = 1200 / cwSpeed;
  Serial.printf("[BT HID] Dit duration: %lu ms (at %d WPM)\n", btHID.ditDuration, cwSpeed);

  // Initialize BLE core if not already done
  initBLECore();
  bleCore.activeMode = BLE_MODE_HID;

  // Use NimBLEHIDDevice helper class for proper HID setup
  // This correctly sets up all required services and characteristics
  btHID.hid = new NimBLEHIDDevice(bleCore.pServer);

  // Set manufacturer name (optional but nice)
  btHID.hid->setManufacturer("VAIL SUMMIT");

  // Set PnP ID (vendor, product, version)
  // Using 0x02 = USB vendor ID source, Apple VID for HID compatibility
  btHID.hid->setPnp(0x02, 0x05ac, 0x820a, 0x0001);

  // Set HID information (country = 0, flags = 0x01 = normally connectable)
  btHID.hid->setHidInfo(0x00, 0x01);

  // Set report map (HID descriptor)
  btHID.hid->setReportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));

  // Get input report characteristic for keyboard (Report ID 1)
  // This also creates the Report Reference descriptor automatically
  btHID.inputReport = btHID.hid->getInputReport(KEYBOARD_REPORT_ID);

  // Set security: bonding + MITM + secure connections
  // This is critical for HID to work properly
  NimBLEDevice::setSecurityAuth(true, true, true);

  // Start HID services (this starts HID, Device Info, and Battery services)
  btHID.hid->startServices();

  // Set up advertising with explicit configuration for Android/Linux compatibility
  // NimBLE 2.0+ defaults have scan response disabled and no name/TX power advertised
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();

  // Set HID keyboard appearance
  advertising->setAppearance(HID_KEYBOARD_APPEARANCE);

  // Add HID Service UUID to advertising data
  advertising->addServiceUUID(btHID.hid->getHidService()->getUUID());

  // Enable scan response (OFF by default in NimBLE 2.0+)
  // Android and Linux rely on scan response data to identify devices
  advertising->enableScanResponse(true);

  // Set device name explicitly (not advertised by default in NimBLE 2.0+)
  advertising->setName(getBLEDeviceName().c_str());

  // Set fast advertising intervals for better discovery on mobile platforms
  // Values in units of 0.625ms (32 = 20ms, 160 = 100ms)
  advertising->setMinInterval(32);   // 20ms minimum
  advertising->setMaxInterval(160);  // 100ms maximum

  // Start advertising
  startBLEAdvertising("HID Keyboard");

  // Set initial battery level
  btHID.hid->setBatteryLevel(100);

  Serial.println("[BT HID] NimBLEHIDDevice initialized successfully");

  // Initialize LVGL UI with device name and status
  updateBTHIDDeviceName(getBLEDeviceName().c_str());
  updateBTHIDStatus("Advertising...", false);
  updateBTHIDPaddleIndicators(false, false);
  updateBTHIDKeyerMode(getBTHIDKeyerModeName());
}

// Stop BT HID mode
void stopBTHID() {
  Serial.println("Stopping BT HID mode");

  // Send release report before disconnecting
  if (btHID.active && isBLEConnected()) {
    sendHIDReport(0x00);  // Release all keys
  }

  // Stop any sidetone that might be playing
  stopTone();

  btHID.active = false;
  btHID.inputReport = nullptr;

  // Note: NimBLEHIDDevice cleanup is handled by deinitBLECore()
  // The hid object is owned by the server and will be deleted when server is deinitialized
  btHID.hid = nullptr;

  // Clean up LVGL widget pointers
  cleanupBTHIDScreen();

  // Deinit BLE
  deinitBLECore();
}

// Draw BT HID UI
void drawBTHIDUI(LGFX& display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Status card
  int cardX = 20;
  int cardY = 55;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 100;

  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  // Connection status
  display.setFont(&FreeSansBold12pt7b);
  display.setTextSize(1);

  int yPos = cardY + 30;
  display.setCursor(cardX + 15, yPos);

  if (isBLEConnected()) {
    display.setTextColor(ST77XX_GREEN);
    display.print("Connected");
  } else if (isBLEAdvertising()) {
    display.setTextColor(ST77XX_YELLOW);
    display.print("Advertising...");
  } else {
    display.setTextColor(ST77XX_RED);
    display.print("Disconnected");
  }

  // Device name
  display.setFont(nullptr);
  display.setTextSize(2);
  display.setTextColor(ST77XX_CYAN);
  yPos += 35;
  display.setCursor(cardX + 15, yPos);
  display.print(getBLEDeviceName());

  // Key mapping info card
  cardY = 170;
  cardH = 80;
  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082);
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF);

  display.setTextSize(1);
  display.setTextColor(0x7BEF);  // Light gray
  yPos = cardY + 12;
  display.setCursor(cardX + 15, yPos);
  display.print("Key Mapping:");

  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);
  yPos += 20;
  display.setCursor(cardX + 15, yPos);
  display.print("DIT -> Left Ctrl");
  yPos += 22;
  display.setCursor(cardX + 15, yPos);
  display.print("DAH -> Right Ctrl");

  // Instructions
  display.setTextSize(1);
  display.setTextColor(ST77XX_YELLOW);
  display.setCursor(cardX + 15, SCREEN_HEIGHT - 35);
  display.print("Pair device in system Bluetooth settings");

  display.setFont(nullptr);
}

// Handle BT HID input
int handleBTHIDInput(char key, LGFX& display) {
  if (key == KEY_ESC) {
    stopBTHID();
    return -1;  // Exit mode
  }

  return 0;  // Normal input
}

// Helper to start keying (key down + tone)
static void btHIDKeyDown(uint8_t modifier) {
  if (!btHID.isKeying || btHID.currentModifier != modifier) {
    btHID.isKeying = true;
    btHID.currentModifier = modifier;
    sendHIDReport(modifier);
    startTone(TONE_SIDETONE);
  } else {
    // Keep audio buffer filled
    continueTone(TONE_SIDETONE);
  }
}

// Helper to stop keying (key up + stop tone)
static void btHIDKeyUp() {
  if (btHID.isKeying) {
    btHID.isKeying = false;
    btHID.currentModifier = 0;
    sendHIDReport(0x00);
    stopTone();
  }
}

// Update BT HID (called from main loop)
void updateBTHID() {
  if (!btHID.active) return;

  // Check for connection state changes and update LVGL UI
  BLEConnectionState currentBLEState = bleCore.connectionState;
  if (currentBLEState != lastBTHIDState) {
    lastBTHIDState = currentBLEState;

    switch (currentBLEState) {
      case BLE_STATE_CONNECTED:
        updateBTHIDStatus("Connected", true);
        Serial.println("[BT HID] Connection state: Connected");
        break;
      case BLE_STATE_ADVERTISING:
        updateBTHIDStatus("Advertising...", false);
        Serial.println("[BT HID] Connection state: Advertising");
        break;
      case BLE_STATE_OFF:
        updateBTHIDStatus("Off", false);
        Serial.println("[BT HID] Connection state: Off");
        break;
      case BLE_STATE_ERROR:
        updateBTHIDStatus("Error", false);
        Serial.println("[BT HID] Connection state: Error");
        break;
    }
  }

  // Read paddle inputs
  bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) ||
                    (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  bool dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) ||
                    (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  // Update visual indicators if paddle state changed
  if (ditPressed != btHID.lastDitPressed || dahPressed != btHID.lastDahPressed) {
    updateBTHIDPaddleIndicators(ditPressed, dahPressed);
    btHID.lastDitPressed = ditPressed;
    btHID.lastDahPressed = dahPressed;
  }

  unsigned long currentTime = millis();

  // Handle based on keyer mode
  switch (btHID.keyerMode) {

    case BT_HID_PASSTHROUGH:
      // Passthrough: Raw paddle → immediate key press/release
      // Host software (MorseRunner) handles timing
      {
        uint8_t modifiers = 0;
        if (ditPressed) modifiers |= KEY_MOD_LCTRL;
        if (dahPressed) modifiers |= KEY_MOD_RCTRL;

        if (modifiers != btHID.currentModifier) {
          if (modifiers != 0) {
            btHIDKeyDown(modifiers);
          } else {
            btHIDKeyUp();
          }
        } else if (modifiers != 0) {
          continueTone(TONE_SIDETONE);
        }
      }
      break;

    case BT_HID_STRAIGHT:
      // Straight key: Either paddle → single Left Ctrl key
      {
        bool anyPressed = ditPressed || dahPressed;
        if (anyPressed && !btHID.isKeying) {
          btHIDKeyDown(KEY_MOD_LCTRL);
        } else if (!anyPressed && btHID.isKeying) {
          btHIDKeyUp();
        } else if (anyPressed) {
          continueTone(TONE_SIDETONE);
        }
      }
      break;

    case BT_HID_IAMBIC_A:
    case BT_HID_IAMBIC_B:
      // Iambic keyer: Full timed sequences
      // State machine: IDLE → SENDING → SPACING → (repeat or IDLE)
      {
        if (!btHID.keyerActive && !btHID.inSpacing) {
          // IDLE state - check for paddle presses
          if (ditPressed || dahPressed) {
            // Start sending element (dit has priority if both pressed)
            btHID.sendingDit = ditPressed;
            btHID.keyerActive = true;
            btHID.elementTimer = currentTime + (btHID.sendingDit ? btHID.ditDuration : btHID.ditDuration * 3);

            // Key down with appropriate modifier
            btHIDKeyDown(btHID.sendingDit ? KEY_MOD_LCTRL : KEY_MOD_RCTRL);

            Serial.printf("[BT HID] Keyer: Starting %s (%lu ms)\n",
              btHID.sendingDit ? "DIT" : "DAH",
              btHID.sendingDit ? btHID.ditDuration : btHID.ditDuration * 3);
          }
        }
        else if (btHID.keyerActive) {
          // SENDING state - outputting dit or dah

          // Keep audio buffer filled
          continueTone(TONE_SIDETONE);

          // Check for memory paddle presses (opposite of what we're sending)
          if (ditPressed && !btHID.sendingDit) btHID.ditMemory = true;
          if (dahPressed && btHID.sendingDit) btHID.dahMemory = true;

          // Check if element duration completed
          if (currentTime >= btHID.elementTimer) {
            // Key up
            btHIDKeyUp();

            // Enter spacing state (inter-element gap = 1 dit)
            btHID.keyerActive = false;
            btHID.inSpacing = true;
            btHID.elementTimer = currentTime + btHID.ditDuration;
          }
        }
        else if (btHID.inSpacing) {
          // SPACING state - inter-element gap

          // Check for memory paddle presses
          if (ditPressed && !btHID.sendingDit) btHID.ditMemory = true;
          if (dahPressed && btHID.sendingDit) btHID.dahMemory = true;

          // Check if spacing completed
          if (currentTime >= btHID.elementTimer) {
            btHID.inSpacing = false;

            // Determine next element
            bool sendNextElement = false;
            bool nextIsDit = false;

            if (btHID.keyerMode == BT_HID_IAMBIC_B) {
              // Iambic B: alternate on squeeze (both paddles pressed)
              if (btHID.ditMemory && btHID.dahMemory) {
                // Both paddles - send opposite of what we just sent
                nextIsDit = !btHID.sendingDit;
                sendNextElement = true;
              } else if (btHID.ditMemory) {
                nextIsDit = true;
                sendNextElement = true;
              } else if (btHID.dahMemory) {
                nextIsDit = false;
                sendNextElement = true;
              } else if (ditPressed && dahPressed) {
                // Still squeezing - continue alternating
                nextIsDit = !btHID.sendingDit;
                sendNextElement = true;
              } else if (ditPressed) {
                nextIsDit = true;
                sendNextElement = true;
              } else if (dahPressed) {
                nextIsDit = false;
                sendNextElement = true;
              }
            } else {
              // Iambic A: memory only, no auto-alternate on squeeze release
              if (btHID.ditMemory) {
                nextIsDit = true;
                sendNextElement = true;
              } else if (btHID.dahMemory) {
                nextIsDit = false;
                sendNextElement = true;
              } else if (ditPressed) {
                nextIsDit = true;
                sendNextElement = true;
              } else if (dahPressed) {
                nextIsDit = false;
                sendNextElement = true;
              }
            }

            if (sendNextElement) {
              // Start next element
              btHID.sendingDit = nextIsDit;
              btHID.keyerActive = true;
              btHID.elementTimer = currentTime + (nextIsDit ? btHID.ditDuration : btHID.ditDuration * 3);

              // Clear used memory
              if (nextIsDit) btHID.ditMemory = false;
              else btHID.dahMemory = false;

              // Key down with appropriate modifier
              btHIDKeyDown(nextIsDit ? KEY_MOD_LCTRL : KEY_MOD_RCTRL);

              Serial.printf("[BT HID] Keyer: Next %s\n", nextIsDit ? "DIT" : "DAH");
            } else {
              // No queued element - return to idle
              btHID.ditMemory = false;
              btHID.dahMemory = false;
            }
          }
        }
      }
      break;
  }

  btHID.lastUpdateTime = currentTime;
}

#endif // BLE_HID_H
