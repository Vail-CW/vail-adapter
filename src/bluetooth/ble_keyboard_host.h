/*
 * BLE Keyboard Host Module
 * Implements BLE Central (Client) mode to receive input from external Bluetooth keyboards
 * Uses NimBLE-Arduino library for BLE Central functionality
 */

#ifndef BLE_KEYBOARD_HOST_H
#define BLE_KEYBOARD_HOST_H

#include <NimBLEDevice.h>
#include <Preferences.h>
#include "../core/config.h"
#include "hid_keycodes.h"

// BLE HID Service and Characteristic UUIDs
#define HID_SERVICE_UUID        "1812"
#define HID_REPORT_CHAR_UUID    "2A4D"
#define HID_REPORT_MAP_UUID     "2A4B"
#define HID_INFO_UUID           "2A4A"

// Key buffer size (circular buffer for incoming keys)
#define BLE_KEY_BUFFER_SIZE     16

// Scan and connection timeouts
#define BLE_SCAN_DURATION_SEC   10
#define BLE_RECONNECT_INTERVAL  30000  // 30 seconds between reconnect attempts
#define BLE_MAX_FOUND_DEVICES   10

// BLE Keyboard Host States
enum BLEKBHostState {
  BLEKB_STATE_IDLE,           // Not active, BLE not initialized
  BLEKB_STATE_READY,          // BLE initialized, waiting for action
  BLEKB_STATE_SCANNING,       // Actively scanning for keyboards
  BLEKB_STATE_SCAN_COMPLETE,  // Scan finished, results available
  BLEKB_STATE_CONNECTING,     // Attempting to connect
  BLEKB_STATE_CONNECTED,      // Connected and receiving input
  BLEKB_STATE_DISCONNECTED,   // Was connected, now disconnected
  BLEKB_STATE_ERROR           // Error occurred
};

// Paired device info (persisted to Preferences)
struct PairedKeyboard {
  char name[32];
  char address[18];  // "XX:XX:XX:XX:XX:XX"
  bool valid;
};

// Scan result entry
struct ScanResult {
  String name;
  String address;
  int rssi;
};

// BLE Keyboard Host context
struct BLEKBHostContext {
  BLEKBHostState state;
  NimBLEClient* pClient;
  NimBLERemoteCharacteristic* pReportChar;
  PairedKeyboard pairedDevice;

  // Circular key buffer (thread-safe via volatile)
  volatile char keyBuffer[BLE_KEY_BUFFER_SIZE];
  volatile uint8_t keyHead;
  volatile uint8_t keyTail;

  // Track previous HID report to detect key changes
  uint8_t prevKeys[6];

  // Scan results
  ScanResult foundDevices[BLE_MAX_FOUND_DEVICES];
  int foundCount;
  int selectedDevice;

  // Connection management
  bool autoReconnect;
  unsigned long lastReconnectAttempt;
  String lastError;

  // Activity tracking
  unsigned long lastKeyTime;
};

// Global context
BLEKBHostContext bleKBHost;

// Forward declarations
void initBLEKeyboardHost();
void deinitBLEKeyboardHost();
void startBLEKeyboardScan();
void stopBLEKeyboardScan();
bool connectToBLEKeyboard(int deviceIndex);
bool connectToBLEKeyboardByAddress(const char* address);
void disconnectBLEKeyboard();
void updateBLEKeyboardHost();
bool isBLEKeyboardConnected();
bool hasBLEKeyboardInput();
char getBLEKeyboardKey();
void saveBLEKeyboardSettings();
void loadBLEKeyboardSettings();
void forgetBLEKeyboardPairing();
const char* getBLEKBStateString();

// NimBLE Client callbacks for connection state
class BLEKBClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {
    Serial.println("BLEKB: Connected to keyboard");
    bleKBHost.state = BLEKB_STATE_CONNECTED;
    bleKBHost.lastKeyTime = millis();
  }

  void onDisconnect(NimBLEClient* pClient, int reason) override {
    Serial.print("BLEKB: Disconnected, reason: ");
    Serial.println(reason);
    bleKBHost.state = BLEKB_STATE_DISCONNECTED;
    bleKBHost.pReportChar = nullptr;

    // Schedule reconnect attempt if auto-reconnect is enabled
    if (bleKBHost.autoReconnect && bleKBHost.pairedDevice.valid) {
      bleKBHost.lastReconnectAttempt = millis();
    }
  }
};

// Scan callbacks for device discovery
class BLEKBScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    // Check if device advertises HID service
    if (advertisedDevice->haveServiceUUID()) {
      if (advertisedDevice->isAdvertisingService(NimBLEUUID(HID_SERVICE_UUID))) {
        // Found a HID device
        if (bleKBHost.foundCount < BLE_MAX_FOUND_DEVICES) {
          String name = advertisedDevice->getName().c_str();
          if (name.length() == 0) {
            name = "Unknown Device";
          }

          // Check for duplicates
          String addr = advertisedDevice->getAddress().toString().c_str();
          for (int i = 0; i < bleKBHost.foundCount; i++) {
            if (bleKBHost.foundDevices[i].address == addr) {
              return;  // Already found this device
            }
          }

          bleKBHost.foundDevices[bleKBHost.foundCount].name = name;
          bleKBHost.foundDevices[bleKBHost.foundCount].address = addr;
          bleKBHost.foundDevices[bleKBHost.foundCount].rssi = advertisedDevice->getRSSI();
          bleKBHost.foundCount++;

          Serial.print("BLEKB: Found HID device: ");
          Serial.print(name);
          Serial.print(" [");
          Serial.print(addr);
          Serial.print("] RSSI: ");
          Serial.println(advertisedDevice->getRSSI());
        }
      }
    }
  }

  void onScanEnd(const NimBLEScanResults& results, int reason) override {
    Serial.print("BLEKB: Scan complete, found ");
    Serial.print(bleKBHost.foundCount);
    Serial.println(" HID devices");
    bleKBHost.state = BLEKB_STATE_SCAN_COMPLETE;
  }
};

// HID Report notification callback
void hidReportNotifyCallback(NimBLERemoteCharacteristic* pChar,
                              uint8_t* pData, size_t length, bool isNotify) {
  // HID Boot Protocol keyboard report is 8 bytes:
  // [0] = Modifier keys (Ctrl, Shift, Alt, GUI)
  // [1] = Reserved (always 0)
  // [2-7] = Up to 6 key codes

  if (length < 8) {
    // Non-standard report, try to handle anyway
    if (length >= 3) {
      // At least modifier + reserved + 1 key
    } else {
      return;
    }
  }

  uint8_t modifiers = pData[0];
  // pData[1] is reserved

  // Process each key code (bytes 2-7)
  int maxKeys = min((int)(length - 2), 6);
  for (int i = 0; i < maxKeys; i++) {
    uint8_t keyCode = pData[i + 2];

    if (keyCode == 0) continue;  // No key

    // Check if this is a new key (not in previous report)
    bool isNewKey = true;
    for (int j = 0; j < 6; j++) {
      if (keyCode == bleKBHost.prevKeys[j]) {
        isNewKey = false;
        break;
      }
    }

    if (isNewKey) {
      // Convert HID key code to ASCII/CardKB format
      char ascii = hidKeyCodeToChar(keyCode, modifiers);
      if (ascii != 0) {
        // Add to circular buffer
        uint8_t nextHead = (bleKBHost.keyHead + 1) % BLE_KEY_BUFFER_SIZE;
        if (nextHead != bleKBHost.keyTail) {
          bleKBHost.keyBuffer[bleKBHost.keyHead] = ascii;
          bleKBHost.keyHead = nextHead;
          bleKBHost.lastKeyTime = millis();

          Serial.print("BLEKB: Key 0x");
          Serial.print(keyCode, HEX);
          Serial.print(" -> '");
          if (ascii >= 32 && ascii < 127) {
            Serial.print(ascii);
          } else {
            Serial.print("0x");
            Serial.print((uint8_t)ascii, HEX);
          }
          Serial.println("'");
        }
      }
    }
  }

  // Save current keys for next comparison
  memset(bleKBHost.prevKeys, 0, sizeof(bleKBHost.prevKeys));
  for (int i = 0; i < maxKeys && i < 6; i++) {
    bleKBHost.prevKeys[i] = pData[i + 2];
  }
}

// Initialize BLE keyboard host system
void initBLEKeyboardHost() {
  if (bleKBHost.state != BLEKB_STATE_IDLE) {
    Serial.println("BLEKB: Already initialized");
    return;
  }

  Serial.println("BLEKB: Initializing BLE Keyboard Host...");

  // Initialize context
  bleKBHost.pClient = nullptr;
  bleKBHost.pReportChar = nullptr;
  bleKBHost.keyHead = 0;
  bleKBHost.keyTail = 0;
  bleKBHost.foundCount = 0;
  bleKBHost.selectedDevice = 0;
  bleKBHost.lastReconnectAttempt = 0;
  bleKBHost.lastKeyTime = 0;
  memset(bleKBHost.prevKeys, 0, sizeof(bleKBHost.prevKeys));

  // Initialize NimBLE in Central mode
  NimBLEDevice::init("VAIL-SUMMIT-KB");

  // Set power level for better range
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  bleKBHost.state = BLEKB_STATE_READY;
  Serial.println("BLEKB: Initialized successfully");
}

// Deinitialize BLE keyboard host
void deinitBLEKeyboardHost() {
  if (bleKBHost.state == BLEKB_STATE_IDLE) {
    return;
  }

  Serial.println("BLEKB: Deinitializing...");

  // Disconnect if connected
  if (bleKBHost.pClient != nullptr && bleKBHost.pClient->isConnected()) {
    bleKBHost.pClient->disconnect();
    delay(100);
  }

  // Stop any ongoing scan
  NimBLEDevice::getScan()->stop();

  // Clean up client
  if (bleKBHost.pClient != nullptr) {
    NimBLEDevice::deleteClient(bleKBHost.pClient);
    bleKBHost.pClient = nullptr;
  }

  // Deinit NimBLE
  NimBLEDevice::deinit(true);
  delay(100);

  bleKBHost.state = BLEKB_STATE_IDLE;
  bleKBHost.pReportChar = nullptr;

  Serial.println("BLEKB: Deinitialized");
}

// Start scanning for BLE keyboards
void startBLEKeyboardScan() {
  if (bleKBHost.state == BLEKB_STATE_IDLE) {
    initBLEKeyboardHost();
  }

  if (bleKBHost.state == BLEKB_STATE_SCANNING) {
    Serial.println("BLEKB: Already scanning");
    return;
  }

  // Disconnect if connected
  if (bleKBHost.pClient != nullptr && bleKBHost.pClient->isConnected()) {
    bleKBHost.pClient->disconnect();
  }

  Serial.println("BLEKB: Starting scan for HID keyboards...");

  // Clear previous results
  bleKBHost.foundCount = 0;
  bleKBHost.selectedDevice = 0;

  // Configure and start scan
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new BLEKBScanCallbacks(), false);
  pScan->setActiveScan(true);  // Active scan for device names
  pScan->setInterval(100);
  pScan->setWindow(99);

  bleKBHost.state = BLEKB_STATE_SCANNING;
  pScan->start(BLE_SCAN_DURATION_SEC, false);  // Non-blocking
}

// Stop scanning
void stopBLEKeyboardScan() {
  if (bleKBHost.state == BLEKB_STATE_SCANNING) {
    NimBLEDevice::getScan()->stop();
    bleKBHost.state = BLEKB_STATE_SCAN_COMPLETE;
    Serial.println("BLEKB: Scan stopped");
  }
}

// Connect to a keyboard from scan results
bool connectToBLEKeyboard(int deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= bleKBHost.foundCount) {
    Serial.println("BLEKB: Invalid device index");
    return false;
  }

  return connectToBLEKeyboardByAddress(
    bleKBHost.foundDevices[deviceIndex].address.c_str());
}

// Connect to a keyboard by address
bool connectToBLEKeyboardByAddress(const char* address) {
  if (bleKBHost.state == BLEKB_STATE_IDLE) {
    initBLEKeyboardHost();
  }

  Serial.print("BLEKB: Connecting to ");
  Serial.println(address);

  bleKBHost.state = BLEKB_STATE_CONNECTING;
  bleKBHost.lastError = "";

  // Create client if needed
  if (bleKBHost.pClient == nullptr) {
    bleKBHost.pClient = NimBLEDevice::createClient();
    bleKBHost.pClient->setClientCallbacks(new BLEKBClientCallbacks(), false);
  }

  // Set connection parameters for low latency
  bleKBHost.pClient->setConnectionParams(12, 12, 0, 200);

  // Connect to the device
  NimBLEAddress bleAddr(std::string(address));
  if (!bleKBHost.pClient->connect(bleAddr)) {
    Serial.println("BLEKB: Connection failed");
    bleKBHost.state = BLEKB_STATE_ERROR;
    bleKBHost.lastError = "Connection failed";
    return false;
  }

  Serial.println("BLEKB: Connected, discovering services...");

  // Get HID service
  NimBLERemoteService* pService = bleKBHost.pClient->getService(NimBLEUUID(HID_SERVICE_UUID));
  if (pService == nullptr) {
    Serial.println("BLEKB: HID service not found");
    bleKBHost.pClient->disconnect();
    bleKBHost.state = BLEKB_STATE_ERROR;
    bleKBHost.lastError = "HID service not found";
    return false;
  }

  Serial.println("BLEKB: Found HID service, looking for report characteristic...");

  // Get Report characteristic (may be multiple, we need the input report)
  // The characteristic with notify property is the input report
  const std::vector<NimBLERemoteCharacteristic*>& chars = pService->getCharacteristics(true);
  bleKBHost.pReportChar = nullptr;

  for (auto pChar : chars) {
    if (pChar->getUUID() == NimBLEUUID(HID_REPORT_CHAR_UUID)) {
      if (pChar->canNotify()) {
        bleKBHost.pReportChar = pChar;
        Serial.print("BLEKB: Found input report characteristic: ");
        Serial.println(pChar->getUUID().toString().c_str());
        break;
      }
    }
  }

  if (bleKBHost.pReportChar == nullptr) {
    Serial.println("BLEKB: Input report characteristic not found");
    bleKBHost.pClient->disconnect();
    bleKBHost.state = BLEKB_STATE_ERROR;
    bleKBHost.lastError = "Report char not found";
    return false;
  }

  // Subscribe to notifications
  if (!bleKBHost.pReportChar->subscribe(true, hidReportNotifyCallback)) {
    Serial.println("BLEKB: Failed to subscribe to notifications");
    bleKBHost.pClient->disconnect();
    bleKBHost.state = BLEKB_STATE_ERROR;
    bleKBHost.lastError = "Subscribe failed";
    return false;
  }

  Serial.println("BLEKB: Subscribed to keyboard input notifications");

  // Save pairing info
  strncpy(bleKBHost.pairedDevice.address, address, sizeof(bleKBHost.pairedDevice.address) - 1);

  // Get device name
  String deviceName = bleKBHost.pClient->getPeerAddress().toString().c_str();
  for (int i = 0; i < bleKBHost.foundCount; i++) {
    if (bleKBHost.foundDevices[i].address == address) {
      deviceName = bleKBHost.foundDevices[i].name;
      break;
    }
  }
  strncpy(bleKBHost.pairedDevice.name, deviceName.c_str(),
          sizeof(bleKBHost.pairedDevice.name) - 1);
  bleKBHost.pairedDevice.valid = true;

  // Save to preferences
  saveBLEKeyboardSettings();

  bleKBHost.state = BLEKB_STATE_CONNECTED;
  bleKBHost.lastKeyTime = millis();

  Serial.print("BLEKB: Successfully connected to ");
  Serial.println(bleKBHost.pairedDevice.name);

  return true;
}

// Disconnect from keyboard
void disconnectBLEKeyboard() {
  if (bleKBHost.pClient != nullptr && bleKBHost.pClient->isConnected()) {
    Serial.println("BLEKB: Disconnecting...");
    bleKBHost.pClient->disconnect();
  }
  bleKBHost.state = BLEKB_STATE_READY;
  bleKBHost.pReportChar = nullptr;
}

// Update function (call from main loop)
void updateBLEKeyboardHost() {
  // Only run if BLE keyboard host is active
  if (bleKBHost.state == BLEKB_STATE_IDLE) {
    return;
  }

  // Handle auto-reconnect
  if (bleKBHost.state == BLEKB_STATE_DISCONNECTED ||
      bleKBHost.state == BLEKB_STATE_READY) {
    if (bleKBHost.autoReconnect && bleKBHost.pairedDevice.valid) {
      unsigned long now = millis();
      if (now - bleKBHost.lastReconnectAttempt >= BLE_RECONNECT_INTERVAL) {
        bleKBHost.lastReconnectAttempt = now;
        Serial.println("BLEKB: Attempting auto-reconnect...");
        connectToBLEKeyboardByAddress(bleKBHost.pairedDevice.address);
      }
    }
  }
}

// Check if BLE keyboard is connected
bool isBLEKeyboardConnected() {
  return bleKBHost.state == BLEKB_STATE_CONNECTED &&
         bleKBHost.pClient != nullptr &&
         bleKBHost.pClient->isConnected();
}

// Check if there's keyboard input waiting
bool hasBLEKeyboardInput() {
  return bleKBHost.keyHead != bleKBHost.keyTail;
}

// Get next key from buffer (returns 0 if empty)
char getBLEKeyboardKey() {
  if (bleKBHost.keyHead == bleKBHost.keyTail) {
    return 0;  // Buffer empty
  }

  char key = bleKBHost.keyBuffer[bleKBHost.keyTail];
  bleKBHost.keyTail = (bleKBHost.keyTail + 1) % BLE_KEY_BUFFER_SIZE;
  return key;
}

// Save settings to Preferences
void saveBLEKeyboardSettings() {
  Preferences prefs;
  prefs.begin("btkeyboard", false);

  prefs.putBool("valid", bleKBHost.pairedDevice.valid);
  if (bleKBHost.pairedDevice.valid) {
    prefs.putString("name", bleKBHost.pairedDevice.name);
    prefs.putString("addr", bleKBHost.pairedDevice.address);
  }
  prefs.putBool("autoRecon", bleKBHost.autoReconnect);

  prefs.end();
  Serial.println("BLEKB: Settings saved");
}

// Load settings from Preferences
void loadBLEKeyboardSettings() {
  Preferences prefs;
  prefs.begin("btkeyboard", true);

  bleKBHost.pairedDevice.valid = prefs.getBool("valid", false);
  if (bleKBHost.pairedDevice.valid) {
    String name = prefs.getString("name", "");
    String addr = prefs.getString("addr", "");
    strncpy(bleKBHost.pairedDevice.name, name.c_str(),
            sizeof(bleKBHost.pairedDevice.name) - 1);
    strncpy(bleKBHost.pairedDevice.address, addr.c_str(),
            sizeof(bleKBHost.pairedDevice.address) - 1);
  }
  bleKBHost.autoReconnect = prefs.getBool("autoRecon", true);

  prefs.end();

  Serial.print("BLEKB: Settings loaded, paired device: ");
  if (bleKBHost.pairedDevice.valid) {
    Serial.println(bleKBHost.pairedDevice.name);
  } else {
    Serial.println("None");
  }
}

// Forget paired keyboard
void forgetBLEKeyboardPairing() {
  // Disconnect if connected
  if (bleKBHost.pClient != nullptr && bleKBHost.pClient->isConnected()) {
    bleKBHost.pClient->disconnect();
  }

  // Clear pairing info
  bleKBHost.pairedDevice.valid = false;
  memset(bleKBHost.pairedDevice.name, 0, sizeof(bleKBHost.pairedDevice.name));
  memset(bleKBHost.pairedDevice.address, 0, sizeof(bleKBHost.pairedDevice.address));

  // Save to preferences
  saveBLEKeyboardSettings();

  bleKBHost.state = BLEKB_STATE_READY;
  Serial.println("BLEKB: Pairing forgotten");
}

// Get human-readable state string
const char* getBLEKBStateString() {
  switch (bleKBHost.state) {
    case BLEKB_STATE_IDLE:
      return "Idle";
    case BLEKB_STATE_READY:
      return "Ready";
    case BLEKB_STATE_SCANNING:
      return "Scanning...";
    case BLEKB_STATE_SCAN_COMPLETE:
      return "Scan Complete";
    case BLEKB_STATE_CONNECTING:
      return "Connecting...";
    case BLEKB_STATE_CONNECTED:
      return "Connected";
    case BLEKB_STATE_DISCONNECTED:
      return "Disconnected";
    case BLEKB_STATE_ERROR:
      return "Error";
    default:
      return "Unknown";
  }
}

#endif // BLE_KEYBOARD_HOST_H
