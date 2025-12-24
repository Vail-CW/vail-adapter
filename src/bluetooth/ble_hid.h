/*
 * BLE HID Mode
 * Emulates a BLE keyboard sending Left/Right Ctrl keys for paddle input
 * Compatible with MorseRunner and other CW tools expecting keyboard input
 * Uses NimBLE-Arduino library for improved memory efficiency
 */

#ifndef BLE_HID_H
#define BLE_HID_H

#include <NimBLEDevice.h>
#include "ble_core.h"
#include "../core/config.h"
#include "../audio/i2s_audio.h"

// HID Service UUIDs
#define HID_SERVICE_UUID_PERIPH    "1812"
#define HID_REPORT_MAP_UUID_PERIPH "2A4B"
#define HID_REPORT_UUID            "2A4D"
#define HID_INFO_UUID_PERIPH       "2A4A"
#define HID_CONTROL_UUID           "2A4C"
#define HID_PROTO_MODE_UUID        "2A4E"

// HID Appearance value for keyboard
#define HID_KEYBOARD_APPEARANCE    0x03C1

// HID Report Descriptor for keyboard
static const uint8_t hidReportDescriptor[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x06,        // Usage (Keyboard)
  0xa1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
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

// HID modifier key bits
#define KEY_MOD_LCTRL  0x01  // Left Control
#define KEY_MOD_RCTRL  0x10  // Right Control

// BLE HID state
struct BLEHIDState {
  bool active = false;
  bool lastDitPressed = false;
  bool lastDahPressed = false;
  NimBLEService* hidService = nullptr;
  NimBLECharacteristic* inputReport = nullptr;
  NimBLECharacteristic* reportMap = nullptr;
  unsigned long lastUpdateTime = 0;
};

BLEHIDState btHID;

// Forward declarations
void startBTHID(LGFX& display);
void drawBTHIDUI(LGFX& display);
int handleBTHIDInput(char key, LGFX& display);
void updateBTHID();
void sendHIDReport(uint8_t modifiers);
void stopBTHID();

// Send HID keyboard report with modifiers
void sendHIDReport(uint8_t modifiers) {
  if (!btHID.active || btHID.inputReport == nullptr) return;
  if (!isBLEConnected()) return;

  // Build HID keyboard report: [Report ID, Modifiers, Reserved, Key1-6]
  uint8_t report[8] = {0};
  report[0] = modifiers;  // Modifier keys
  // Reserved byte and key array remain 0

  btHID.inputReport->setValue(report, sizeof(report));
  btHID.inputReport->notify();

  Serial.print("HID Report: Modifiers=0x");
  Serial.println(modifiers, HEX);
}

// Start BT HID mode
void startBTHID(LGFX& display) {
  Serial.println("Starting BT HID mode");

  btHID.active = true;
  btHID.lastDitPressed = false;
  btHID.lastDahPressed = false;
  btHID.lastUpdateTime = millis();

  // Initialize BLE core if not already done
  initBLECore();
  bleCore.activeMode = BLE_MODE_HID;

  // Create HID service
  btHID.hidService = bleCore.pServer->createService(HID_SERVICE_UUID_PERIPH);

  // Create Report Map characteristic
  btHID.reportMap = btHID.hidService->createCharacteristic(
    HID_REPORT_MAP_UUID_PERIPH,
    NIMBLE_PROPERTY::READ
  );
  btHID.reportMap->setValue((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));

  // Create HID Information characteristic
  NimBLECharacteristic* hidInfo = btHID.hidService->createCharacteristic(
    HID_INFO_UUID_PERIPH,
    NIMBLE_PROPERTY::READ
  );
  uint8_t hidInfoValue[] = {0x11, 0x01, 0x00, 0x01};  // HID version, country, flags
  hidInfo->setValue(hidInfoValue, sizeof(hidInfoValue));

  // Create HID Control Point characteristic
  NimBLECharacteristic* hidControl = btHID.hidService->createCharacteristic(
    HID_CONTROL_UUID,
    NIMBLE_PROPERTY::WRITE_NR
  );

  // Create Protocol Mode characteristic
  NimBLECharacteristic* protoMode = btHID.hidService->createCharacteristic(
    HID_PROTO_MODE_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR
  );
  uint8_t protoValue = 1;  // Report Protocol
  protoMode->setValue(&protoValue, 1);

  // Create Input Report characteristic (for sending keyboard reports)
  btHID.inputReport = btHID.hidService->createCharacteristic(
    HID_REPORT_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  // Add Report Reference descriptor (required for HID)
  NimBLE2904* reportRefDesc = (NimBLE2904*)btHID.inputReport->createDescriptor("2908");
  uint8_t reportRef[] = {0x01, 0x01};  // Report ID 1, Input
  reportRefDesc->setValue(reportRef, sizeof(reportRef));

  // Start HID service
  btHID.hidService->start();

  // Set up advertising
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->setAppearance(HID_KEYBOARD_APPEARANCE);
  advertising->addServiceUUID(btHID.hidService->getUUID());

  // Set security (bonding)
  NimBLEDevice::setSecurityAuth(true, false, true);  // bonding, MITM, SC

  // Start advertising
  startBLEAdvertising("HID Keyboard");

  // UI is now handled by LVGL - see lv_mode_screens.h
}

// Stop BT HID mode
void stopBTHID() {
  Serial.println("Stopping BT HID mode");

  // Send release report before disconnecting
  if (btHID.active && isBLEConnected()) {
    sendHIDReport(0x00);  // Release all keys
  }

  btHID.active = false;
  btHID.hidService = nullptr;
  btHID.inputReport = nullptr;
  btHID.reportMap = nullptr;

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

// Update BT HID (called from main loop)
void updateBTHID() {
  if (!btHID.active) return;

  // Read paddle inputs
  bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) ||
                    (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  bool dahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) ||
                    (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  bool anyPressed = ditPressed || dahPressed;
  bool wasPressed = btHID.lastDitPressed || btHID.lastDahPressed;

  // Check if state changed
  if (ditPressed != btHID.lastDitPressed || dahPressed != btHID.lastDahPressed) {
    // Build modifier byte
    uint8_t modifiers = 0;
    if (ditPressed) modifiers |= KEY_MOD_LCTRL;
    if (dahPressed) modifiers |= KEY_MOD_RCTRL;

    // Send HID report
    sendHIDReport(modifiers);

    // Play local sidetone feedback
    if (anyPressed) {
      startTone(TONE_SIDETONE);
    } else {
      stopTone();
    }

    // Update state
    btHID.lastDitPressed = ditPressed;
    btHID.lastDahPressed = dahPressed;
    btHID.lastUpdateTime = millis();
  }
  else if (anyPressed) {
    // Paddle still pressed - keep filling the audio buffer
    continueTone(TONE_SIDETONE);
  }
}

#endif // BLE_HID_H
