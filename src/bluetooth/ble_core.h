/*
 * BLE Core Module
 * Shared Bluetooth Low Energy initialization and state management
 * Supports both BLE MIDI and BLE HID modes
 * Uses NimBLE-Arduino library for improved memory efficiency
 */

#ifndef BLE_CORE_H
#define BLE_CORE_H

#include <NimBLEDevice.h>
#include "../core/config.h"

// BLE connection state
enum BLEConnectionState {
  BLE_STATE_OFF,
  BLE_STATE_ADVERTISING,
  BLE_STATE_CONNECTED,
  BLE_STATE_ERROR
};

// BLE mode type
enum BLEModeType {
  BLE_MODE_NONE,
  BLE_MODE_HID,
  BLE_MODE_MIDI
};

// BLE Core state
struct BLECoreState {
  bool initialized = false;
  BLEConnectionState connectionState = BLE_STATE_OFF;
  BLEModeType activeMode = BLE_MODE_NONE;
  NimBLEServer* pServer = nullptr;
  String connectedDeviceName = "";
  unsigned long lastActivityTime = 0;
};

BLECoreState bleCore;

// Forward declarations
void initBLECore();
void deinitBLECore();
void startBLEAdvertising(const char* serviceName);
void stopBLEAdvertising();
String getBLEDeviceName();
const char* getBLEStateString();

// Server callbacks for connection state tracking
class BLECoreServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    bleCore.connectionState = BLE_STATE_CONNECTED;
    bleCore.lastActivityTime = millis();
    Serial.println("BLE: Client connected");
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    bleCore.connectionState = BLE_STATE_ADVERTISING;
    bleCore.connectedDeviceName = "";
    bleCore.lastActivityTime = millis();
    Serial.println("BLE: Client disconnected");

    // Restart advertising after disconnect
    if (bleCore.activeMode != BLE_MODE_NONE) {
      pServer->startAdvertising();
      Serial.println("BLE: Restarted advertising");
    }
  }
};

// Generate device name with MAC address suffix
String getBLEDeviceName() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);
  char name[32];
  snprintf(name, sizeof(name), "VAIL-SUMMIT-%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(name);
}

// Get human-readable state string
const char* getBLEStateString() {
  switch (bleCore.connectionState) {
    case BLE_STATE_OFF:
      return "Off";
    case BLE_STATE_ADVERTISING:
      return "Advertising";
    case BLE_STATE_CONNECTED:
      return "Connected";
    case BLE_STATE_ERROR:
      return "Error";
    default:
      return "Unknown";
  }
}

// Initialize BLE stack (called once per mode session)
void initBLECore() {
  if (bleCore.initialized) {
    Serial.println("BLE: Already initialized");
    return;
  }

  String deviceName = getBLEDeviceName();
  Serial.print("BLE: Initializing as ");
  Serial.println(deviceName);

  // Small delay before init to ensure clean state
  delay(100);

  NimBLEDevice::init(deviceName.c_str());

  // Create server
  bleCore.pServer = NimBLEDevice::createServer();
  if (bleCore.pServer == nullptr) {
    Serial.println("BLE: ERROR - Failed to create server!");
    return;
  }
  bleCore.pServer->setCallbacks(new BLECoreServerCallbacks());

  bleCore.initialized = true;
  bleCore.connectionState = BLE_STATE_OFF;
  bleCore.lastActivityTime = millis();

  Serial.println("BLE: Core initialized");
}

// Deinitialize BLE stack (called when exiting BLE modes)
void deinitBLECore() {
  if (!bleCore.initialized) {
    return;
  }

  Serial.println("BLE: Deinitializing...");

  // Stop advertising first
  if (bleCore.connectionState == BLE_STATE_ADVERTISING) {
    Serial.println("BLE: Stopping advertising");
    NimBLEDevice::stopAdvertising();
    delay(100);  // Give time for advertising to stop
  }

  // Disconnect any connected clients
  if (bleCore.connectionState == BLE_STATE_CONNECTED && bleCore.pServer != nullptr) {
    Serial.println("BLE: Disconnecting client");
    bleCore.pServer->disconnect(0);  // NimBLE uses connection handle, 0 for first client
    delay(100);  // Give time for disconnect
  }

  // Reset state before deinit
  bleCore.connectionState = BLE_STATE_OFF;
  bleCore.activeMode = BLE_MODE_NONE;
  bleCore.connectedDeviceName = "";
  bleCore.pServer = nullptr;

  // Deinit BLE - this releases all BLE resources
  Serial.println("BLE: Calling NimBLEDevice::deinit()");
  NimBLEDevice::deinit(true);

  // Wait for BLE stack to fully shut down
  delay(500);

  bleCore.initialized = false;

  Serial.println("BLE: Deinitialized successfully");
}

// Start advertising (called after services are set up)
void startBLEAdvertising(const char* serviceName) {
  if (!bleCore.initialized || bleCore.pServer == nullptr) {
    Serial.println("BLE: Cannot advertise - not initialized");
    return;
  }

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  // Note: setScanResponse() and setMinPreferred() removed - deprecated in newer NimBLE API

  NimBLEDevice::startAdvertising();
  bleCore.connectionState = BLE_STATE_ADVERTISING;
  bleCore.lastActivityTime = millis();

  Serial.print("BLE: Started advertising for ");
  Serial.println(serviceName);
}

// Stop advertising
void stopBLEAdvertising() {
  if (!bleCore.initialized) {
    return;
  }

  NimBLEDevice::stopAdvertising();
  bleCore.connectionState = BLE_STATE_OFF;

  Serial.println("BLE: Stopped advertising");
}

// Check if BLE is connected
bool isBLEConnected() {
  return bleCore.connectionState == BLE_STATE_CONNECTED;
}

// Check if BLE is advertising
bool isBLEAdvertising() {
  return bleCore.connectionState == BLE_STATE_ADVERTISING;
}

#endif // BLE_CORE_H
