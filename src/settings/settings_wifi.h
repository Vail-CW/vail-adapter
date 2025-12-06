/*
 * WiFi Settings Module
 * Handles WiFi network scanning, connection, and credential storage
 */

#ifndef SETTINGS_WIFI_H
#define SETTINGS_WIFI_H

#include <WiFi.h>
#include <Preferences.h>
#include "../core/config.h"

// WiFi settings state machine
enum WiFiSettingsState {
  WIFI_STATE_CURRENT_CONNECTION,  // Show current connection status
  WIFI_STATE_SCANNING,
  WIFI_STATE_NETWORK_LIST,
  WIFI_STATE_PASSWORD_INPUT,
  WIFI_STATE_CONNECTING,
  WIFI_STATE_CONNECTED,
  WIFI_STATE_ERROR,
  WIFI_STATE_RESET_CONFIRM,
  WIFI_STATE_AP_MODE
};

// WiFi network info
struct WiFiNetwork {
  String ssid;
  int rssi;
  bool encrypted;
};

// WiFi settings globals
WiFiSettingsState wifiState = WIFI_STATE_SCANNING;
WiFiNetwork networks[20];  // Store up to 20 networks
int networkCount = 0;
int selectedNetwork = 0;
String passwordInput = "";
bool passwordVisible = false;
unsigned long lastBlink = 0;
bool cursorVisible = true;
String statusMessage = "";
Preferences wifiPrefs;
bool isAPMode = false;  // Track if device is in AP mode
String apPassword = "vailsummit";  // Default AP password
bool connectedFromAPMode = false;  // Track if connection was made from AP mode
unsigned long connectionSuccessTime = 0;  // Time when connection succeeded
String failedSSID = "";  // Track SSID that failed to connect (for password retry)

// Forward declarations
void startWiFiSettings(LGFX &display);
void drawWiFiUI(LGFX &display);
int handleWiFiInput(char key, LGFX &display);
void scanNetworks();
void drawCurrentConnection(LGFX &display);
void drawNetworkList(LGFX &display);
void drawPasswordInput(LGFX &display);
void drawResetConfirmation(LGFX &display);
void drawAPModeScreen(LGFX &display);
void connectToWiFi(String ssid, String password);
void saveWiFiCredentials(String ssid, String password);
int loadAllWiFiCredentials(String ssids[3], String passwords[3]);
bool loadWiFiCredentials(String &ssid, String &password);
void autoConnectWiFi();
void resetWiFiSettings();
void startAPMode();
void stopAPMode();

// Start WiFi settings mode
void startWiFiSettings(LGFX &display) {
  selectedNetwork = 0;
  passwordInput = "";

  // Check if already connected to WiFi
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already connected to WiFi - showing current connection");
    wifiState = WIFI_STATE_CURRENT_CONNECTION;
    drawWiFiUI(display);
  } else {
    // Not connected, scan for networks
    wifiState = WIFI_STATE_SCANNING;
    statusMessage = "Scanning for networks...";
    drawWiFiUI(display);
    scanNetworks();

    if (networkCount > 0) {
      wifiState = WIFI_STATE_NETWORK_LIST;
    } else {
      wifiState = WIFI_STATE_ERROR;
      statusMessage = "No networks found. Try again?";
    }

    drawWiFiUI(display);
  }
}

// Scan for WiFi networks
void scanNetworks() {
  Serial.println("Scanning for WiFi networks...");

  // Ensure clean WiFi state before scanning
  WiFi.disconnect(true);  // Disconnect and clear saved credentials from WiFi hardware
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  delay(100);

  int n = WiFi.scanNetworks();

  Serial.print("Scan result: ");
  Serial.println(n);

  if (n < 0) {
    // Scan failed
    Serial.println("WiFi scan failed!");
    networkCount = 0;
    return;
  }

  networkCount = min(n, 20);

  Serial.print("Found ");
  Serial.print(n);
  Serial.println(" networks");

  for (int i = 0; i < networkCount; i++) {
    networks[i].ssid = WiFi.SSID(i);
    networks[i].rssi = WiFi.RSSI(i);
    networks[i].encrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

    Serial.print(i);
    Serial.print(": ");
    Serial.print(networks[i].ssid);
    Serial.print(" (");
    Serial.print(networks[i].rssi);
    Serial.print(" dBm) ");
    Serial.println(networks[i].encrypted ? "[Encrypted]" : "[Open]");
  }
}

// Draw current connection status
void drawCurrentConnection(LGFX &display) {
  display.setTextSize(2);
  display.setTextColor(ST77XX_GREEN);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, "WiFi Connected", 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, 60);
  display.print("WiFi Connected");

  // Info box
  display.drawRect(10, 90, SCREEN_WIDTH - 20, 110, ST77XX_CYAN);
  display.fillRect(12, 92, SCREEN_WIDTH - 24, 106, 0x0841);  // Dark blue background

  // Network name
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(20, 100);
  display.print("Network:");

  display.setTextSize(2);
  display.setTextColor(ST77XX_CYAN);
  display.setCursor(20, 115);
  String ssid = WiFi.SSID();
  if (ssid.length() > 28) {  // Increased for 480px width (was 18 for 320px)
    ssid = ssid.substring(0, 25) + "...";
  }
  display.print(ssid);

  // IP Address
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(20, 145);
  display.print("IP Address:");

  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);
  display.setCursor(20, 160);
  display.print(WiFi.localIP().toString());

  // Signal strength
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(20, 180);
  display.print("Signal: ");

  int rssi = WiFi.RSSI();
  int bars = map(rssi, -100, -40, 1, 4);
  bars = constrain(bars, 1, 4);

  // Draw signal bars
  for (int b = 0; b < 4; b++) {
    int barHeight = (b + 1) * 3;
    int barX = 70 + b * 5;
    if (b < bars) {
      display.fillRect(barX, 185 - barHeight, 4, barHeight, ST77XX_GREEN);
    } else {
      display.drawRect(barX, 185 - barHeight, 4, barHeight, 0x4208);
    }
  }

  display.setCursor(95, 180);
  display.print(rssi);
  display.print(" dBm");
}

// Draw WiFi UI based on current state
void drawWiFiUI(LGFX &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  if (wifiState == WIFI_STATE_CURRENT_CONNECTION) {
    drawCurrentConnection(display);
  }
  else if (wifiState == WIFI_STATE_SCANNING) {
    display.setTextSize(2);
    display.setTextColor(ST77XX_CYAN);
    display.setCursor(40, 100);
    display.print("Scanning...");
  }
  else if (wifiState == WIFI_STATE_NETWORK_LIST) {
    drawNetworkList(display);
  }
  else if (wifiState == WIFI_STATE_PASSWORD_INPUT) {
    drawPasswordInput(display);
  }
  else if (wifiState == WIFI_STATE_CONNECTING) {
    display.setTextSize(2);
    display.setTextColor(ST77XX_YELLOW);
    display.setCursor(40, 100);
    display.print("Connecting...");
  }
  else if (wifiState == WIFI_STATE_CONNECTED) {
    display.setTextSize(2);
    display.setTextColor(ST77XX_GREEN);
    display.setCursor(60, 90);
    display.print("Connected!");

    display.setTextSize(1);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(40, 130);
    display.print("IP: ");
    display.print(WiFi.localIP().toString());
  }
  else if (wifiState == WIFI_STATE_ERROR) {
    display.setTextSize(2);
    display.setTextColor(ST77XX_RED);
    display.setCursor(70, 100);
    display.print("Error");

    display.setTextSize(1);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(40, 130);
    display.print(statusMessage);
  }
  else if (wifiState == WIFI_STATE_RESET_CONFIRM) {
    drawResetConfirmation(display);
  }
  else if (wifiState == WIFI_STATE_AP_MODE) {
    drawAPModeScreen(display);
  }

  // Draw footer instructions
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String footerText = "";

  if (wifiState == WIFI_STATE_CURRENT_CONNECTION) {
    footerText = "C: Change Networks  ESC: Return";
  } else if (wifiState == WIFI_STATE_NETWORK_LIST) {
    footerText = "Up/Down  Enter:Connect  A:AP Mode  R:Reset";
  } else if (wifiState == WIFI_STATE_PASSWORD_INPUT) {
    footerText = "Type password  Enter: Connect  ESC: Cancel";
  } else if (wifiState == WIFI_STATE_CONNECTED) {
    footerText = "Press ESC to return";
  } else if (wifiState == WIFI_STATE_ERROR) {
    // Check if we can offer password retry
    if (failedSSID.length() > 0 && statusMessage.indexOf("password") >= 0) {
      footerText = "P: Retry Password  Enter: Rescan  ESC: Return";
    } else {
      footerText = "Enter: Rescan  ESC: Return";
    }
  } else if (wifiState == WIFI_STATE_RESET_CONFIRM) {
    footerText = "Y: Yes, erase all  N: Cancel";
  } else if (wifiState == WIFI_STATE_AP_MODE) {
    footerText = "A: Disable AP Mode  ESC: Return";
  }

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, footerText.c_str(), 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, SCREEN_HEIGHT - 12);
  display.print(footerText);
}

// Draw network list
void drawNetworkList(LGFX &display) {
  // Clear the network list area (preserve header and footer)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 60, COLOR_BACKGROUND);

  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);
  display.setCursor(10, 55);
  display.print("Available Networks:");

  // Load saved networks to mark them
  String savedSSIDs[3];
  String savedPasswords[3];
  int savedCount = loadAllWiFiCredentials(savedSSIDs, savedPasswords);

  // Calculate visible range (show 5 networks at a time)
  int startIdx = max(0, selectedNetwork - 2);
  int endIdx = min(networkCount, startIdx + 5);

  // Adjust if at end of list
  if (endIdx - startIdx < 5 && networkCount >= 5) {
    startIdx = max(0, endIdx - 5);
  }

  int yPos = 75;
  for (int i = startIdx; i < endIdx; i++) {
    bool isSelected = (i == selectedNetwork);

    // Check if this network is saved
    bool isSaved = false;
    for (int j = 0; j < savedCount; j++) {
      if (networks[i].ssid == savedSSIDs[j]) {
        isSaved = true;
        break;
      }
    }

    // Draw selection background
    if (isSelected) {
      display.fillRect(5, yPos - 2, SCREEN_WIDTH - 10, 22, 0x249F);
    }

    // Draw signal strength bars
    int bars = map(networks[i].rssi, -100, -40, 1, 4);
    bars = constrain(bars, 1, 4);
    uint16_t barColor = isSelected ? ST77XX_WHITE : ST77XX_GREEN;

    for (int b = 0; b < 4; b++) {
      int barHeight = (b + 1) * 3;
      if (b < bars) {
        display.fillRect(10 + b * 4, yPos + 12 - barHeight, 3, barHeight, barColor);
      } else {
        display.drawRect(10 + b * 4, yPos + 12 - barHeight, 3, barHeight, 0x4208);
      }
    }

    // Draw lock icon if encrypted
    if (networks[i].encrypted) {
      uint16_t lockColor = isSelected ? ST77XX_WHITE : ST77XX_YELLOW;
      display.drawRect(30, yPos + 4, 6, 8, lockColor);
      display.fillRect(31, yPos + 7, 4, 5, lockColor);
      display.drawCircle(33, yPos + 6, 2, lockColor);
    }

    // Draw SSID
    display.setTextColor(isSelected ? ST77XX_WHITE : ST77XX_CYAN);
    int ssidX = networks[i].encrypted ? 42 : 32;

    // If saved, draw star icon before SSID
    if (isSaved) {
      uint16_t starColor = isSelected ? ST77XX_WHITE : ST77XX_YELLOW;
      // Draw simple star (asterisk-style)
      display.setTextColor(starColor);
      display.setCursor(ssidX, yPos + 6);
      display.print("*");
      ssidX += 6;
      display.setTextColor(isSelected ? ST77XX_WHITE : ST77XX_CYAN);
    }

    display.setCursor(ssidX, yPos + 6);

    // Truncate long SSIDs (adjust for star icon)
    String ssid = networks[i].ssid;
    int maxLen = isSaved ? 28 : 30;
    if (ssid.length() > maxLen) {
      ssid = ssid.substring(0, maxLen - 3) + "...";
    }
    display.print(ssid);

    yPos += 24;
  }

  // Draw scrollbar if needed
  if (networkCount > 5) {
    int scrollbarHeight = (SCREEN_HEIGHT - 100) * 5 / networkCount;
    int scrollbarY = 75 + (SCREEN_HEIGHT - 100 - scrollbarHeight) * selectedNetwork / (networkCount - 1);
    display.fillRect(SCREEN_WIDTH - 5, scrollbarY, 3, scrollbarHeight, ST77XX_WHITE);
  }
}

// Draw reset confirmation screen
void drawResetConfirmation(LGFX &display) {
  display.setTextSize(2);
  display.setTextColor(ST77XX_RED);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, "Reset WiFi?", 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, 70);
  display.print("Reset WiFi?");

  // Warning box
  display.drawRect(20, 100, SCREEN_WIDTH - 40, 80, ST77XX_YELLOW);
  display.fillRect(22, 102, SCREEN_WIDTH - 44, 76, 0x1800);  // Dark red background

  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(30, 110);
  display.print("This will erase ALL saved");
  display.setCursor(30, 125);
  display.print("WiFi network credentials.");
  display.setCursor(30, 145);
  display.print("This action cannot be");
  display.setCursor(30, 160);
  display.print("undone.");
}

// Draw AP mode screen
void drawAPModeScreen(LGFX &display) {
  display.setTextSize(2);
  display.setTextColor(ST77XX_GREEN);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, "AP Mode Active", 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, 60);
  display.print("AP Mode Active");

  // Info box
  display.drawRect(10, 90, SCREEN_WIDTH - 20, 110, ST77XX_CYAN);
  display.fillRect(12, 92, SCREEN_WIDTH - 24, 106, 0x0841);  // Dark blue background

  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(20, 100);
  display.print("Network Name (SSID):");

  display.setTextSize(2);
  display.setTextColor(ST77XX_CYAN);
  display.setCursor(20, 115);
  display.print(WiFi.softAPSSID());

  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(20, 145);
  display.print("Password:");

  display.setTextSize(2);
  display.setTextColor(ST77XX_CYAN);
  display.setCursor(20, 160);
  display.print(apPassword);

  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(20, 185);
  display.print("Connect and browse to:");
  display.setCursor(20, 198);
  display.print("http://192.168.4.1");
}

// Draw password input screen
void drawPasswordInput(LGFX &display) {
  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);
  display.setCursor(10, 55);
  display.print("Connect to:");

  // Draw selected network name
  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(10, 75);
  String ssid = networks[selectedNetwork].ssid;
  if (ssid.length() > 20) {
    ssid = ssid.substring(0, 17) + "...";
  }
  display.print(ssid);

  // Draw password input box
  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);
  display.setCursor(10, 110);
  display.print("Password:");

  // Input box
  display.drawRect(10, 125, SCREEN_WIDTH - 20, 30, ST77XX_WHITE);
  display.fillRect(12, 127, SCREEN_WIDTH - 24, 26, COLOR_BACKGROUND);

  // Display password (masked or visible)
  display.setTextSize(2);
  display.setTextColor(ST77XX_WHITE);
  display.setCursor(15, 135);

  if (passwordVisible) {
    display.print(passwordInput);
  } else {
    // Show asterisks
    for (int i = 0; i < passwordInput.length(); i++) {
      display.print("*");
    }
  }

  // Blinking cursor
  if (cursorVisible) {
    int cursorX = 15 + (passwordInput.length() * 12);
    if (cursorX < SCREEN_WIDTH - 25) {
      display.fillRect(cursorX, 135, 2, 16, ST77XX_WHITE);
    }
  }

  // Toggle visibility hint
  display.setTextSize(1);
  display.setTextColor(0x7BEF);
  display.setCursor(10, 170);
  display.print("TAB: ");
  display.print(passwordVisible ? "Hide" : "Show");
  display.print(" password");
}

// Handle WiFi settings input
int handleWiFiInput(char key, LGFX &display) {
  // Update cursor blink
  if (wifiState == WIFI_STATE_PASSWORD_INPUT && millis() - lastBlink > 500) {
    cursorVisible = !cursorVisible;
    lastBlink = millis();
    drawPasswordInput(display);
  }

  if (wifiState == WIFI_STATE_CURRENT_CONNECTION) {
    if (key == 'c' || key == 'C') {
      // User wants to change networks - scan and show network list
      wifiState = WIFI_STATE_SCANNING;
      statusMessage = "Scanning for networks...";
      beep(TONE_SELECT, BEEP_MEDIUM);
      drawWiFiUI(display);
      scanNetworks();

      if (networkCount > 0) {
        wifiState = WIFI_STATE_NETWORK_LIST;
      } else {
        wifiState = WIFI_STATE_ERROR;
        statusMessage = "No networks found. Try again?";
      }

      drawWiFiUI(display);
      return 2;
    }
    else if (key == KEY_ESC) {
      return -1;  // Exit WiFi settings
    }
  }
  else if (wifiState == WIFI_STATE_NETWORK_LIST) {
    if (key == KEY_UP) {
      if (selectedNetwork > 0) {
        selectedNetwork--;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawNetworkList(display);
        return 1;
      }
    }
    else if (key == KEY_DOWN) {
      if (selectedNetwork < networkCount - 1) {
        selectedNetwork++;
        beep(TONE_MENU_NAV, BEEP_SHORT);
        drawNetworkList(display);
        return 1;
      }
    }
    else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      // Check if this network is already saved
      String savedSSIDs[3];
      String savedPasswords[3];
      int savedCount = loadAllWiFiCredentials(savedSSIDs, savedPasswords);

      bool isSaved = false;
      String savedPassword = "";
      for (int j = 0; j < savedCount; j++) {
        if (networks[selectedNetwork].ssid == savedSSIDs[j]) {
          isSaved = true;
          savedPassword = savedPasswords[j];
          break;
        }
      }

      if (isSaved) {
        // Network is saved, connect using saved password
        Serial.println("Network is saved - connecting with saved credentials");
        wifiState = WIFI_STATE_CONNECTING;
        beep(TONE_SELECT, BEEP_MEDIUM);
        drawWiFiUI(display);
        connectToWiFi(networks[selectedNetwork].ssid, savedPassword);
        return 2;
      } else if (networks[selectedNetwork].encrypted) {
        // Not saved and encrypted - prompt for password
        wifiState = WIFI_STATE_PASSWORD_INPUT;
        passwordInput = "";
        cursorVisible = true;
        lastBlink = millis();
        beep(TONE_SELECT, BEEP_MEDIUM);
        drawWiFiUI(display);
      } else {
        // Open network, connect immediately
        wifiState = WIFI_STATE_CONNECTING;
        drawWiFiUI(display);
        connectToWiFi(networks[selectedNetwork].ssid, "");
        return 2;
      }
      return 1;
    }
    else if (key == 'r' || key == 'R') {
      // Show reset confirmation
      wifiState = WIFI_STATE_RESET_CONFIRM;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawWiFiUI(display);
      return 1;
    }
    else if (key == 'a' || key == 'A') {
      // Enable AP mode
      startAPMode();
      wifiState = WIFI_STATE_AP_MODE;
      beep(TONE_SELECT, BEEP_MEDIUM);
      drawWiFiUI(display);
      return 2;
    }
    else if (key == KEY_ESC) {
      return -1;  // Exit WiFi settings
    }
  }
  else if (wifiState == WIFI_STATE_PASSWORD_INPUT) {
    if (key == KEY_BACKSPACE) {
      if (passwordInput.length() > 0) {
        passwordInput.remove(passwordInput.length() - 1);
        cursorVisible = true;
        lastBlink = millis();
        drawPasswordInput(display);
      }
      return 1;
    }
    else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      // Connect with password
      wifiState = WIFI_STATE_CONNECTING;
      beep(TONE_SELECT, BEEP_MEDIUM);
      drawWiFiUI(display);
      connectToWiFi(networks[selectedNetwork].ssid, passwordInput);
      return 2;
    }
    else if (key == KEY_ESC) {
      // Cancel password input, back to network list
      wifiState = WIFI_STATE_NETWORK_LIST;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawWiFiUI(display);
      return 1;
    }
    else if (key == KEY_TAB) {
      // Toggle password visibility
      passwordVisible = !passwordVisible;
      drawPasswordInput(display);
      return 1;
    }
    else if (key >= 32 && key <= 126 && passwordInput.length() < 63) {
      // Add printable character (max 63 chars for WiFi password)
      passwordInput += key;
      cursorVisible = true;
      lastBlink = millis();
      drawPasswordInput(display);
      return 1;
    }
  }
  else if (wifiState == WIFI_STATE_CONNECTED || wifiState == WIFI_STATE_ERROR) {
    // Auto-exit after 2 seconds if connected from AP mode
    if (wifiState == WIFI_STATE_CONNECTED && connectedFromAPMode) {
      if (millis() - connectionSuccessTime >= 2000) {
        Serial.println("Auto-exiting WiFi settings after successful AP mode connection");
        connectedFromAPMode = false;  // Reset flag
        return -1;  // Exit WiFi settings to main menu
      }
    }

    if (key == KEY_ESC) {
      failedSSID = "";  // Clear failed SSID
      return -1;  // Exit WiFi settings
    }
    else if (wifiState == WIFI_STATE_ERROR && (key == 'p' || key == 'P')) {
      // Retry password if available
      if (failedSSID.length() > 0 && statusMessage.indexOf("password") >= 0) {
        Serial.println("Retrying password entry for failed network");

        // Find the failed network in the network list
        int failedNetworkIndex = -1;
        for (int i = 0; i < networkCount; i++) {
          if (networks[i].ssid == failedSSID) {
            failedNetworkIndex = i;
            break;
          }
        }

        if (failedNetworkIndex >= 0) {
          selectedNetwork = failedNetworkIndex;
          wifiState = WIFI_STATE_PASSWORD_INPUT;
          passwordInput = "";
          cursorVisible = true;
          lastBlink = millis();
          beep(TONE_SELECT, BEEP_MEDIUM);
          failedSSID = "";  // Clear failed SSID
          drawWiFiUI(display);
          return 2;
        }
      }
    }
    else if (wifiState == WIFI_STATE_ERROR && (key == KEY_ENTER || key == KEY_ENTER_ALT)) {
      // Rescan networks on ENTER when in error state
      failedSSID = "";  // Clear failed SSID
      wifiState = WIFI_STATE_SCANNING;
      statusMessage = "Scanning for networks...";
      drawWiFiUI(display);
      scanNetworks();

      if (networkCount > 0) {
        wifiState = WIFI_STATE_NETWORK_LIST;
      } else {
        wifiState = WIFI_STATE_ERROR;
        statusMessage = "No networks found. Try again?";
      }

      drawWiFiUI(display);
      return 2;
    }
  }
  else if (wifiState == WIFI_STATE_RESET_CONFIRM) {
    if (key == 'y' || key == 'Y') {
      // Confirm reset - erase all WiFi credentials
      resetWiFiSettings();
      beep(TONE_ERROR, BEEP_LONG);

      // Show confirmation message
      wifiState = WIFI_STATE_ERROR;
      statusMessage = "WiFi settings erased";
      drawWiFiUI(display);
      delay(2000);

      // Rescan networks
      wifiState = WIFI_STATE_SCANNING;
      drawWiFiUI(display);
      scanNetworks();

      if (networkCount > 0) {
        wifiState = WIFI_STATE_NETWORK_LIST;
      } else {
        wifiState = WIFI_STATE_ERROR;
        statusMessage = "No networks found. Try again?";
      }

      drawWiFiUI(display);
      return 2;
    }
    else if (key == 'n' || key == 'N' || key == KEY_ESC) {
      // Cancel reset, back to network list
      wifiState = WIFI_STATE_NETWORK_LIST;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawWiFiUI(display);
      return 1;
    }
  }
  else if (wifiState == WIFI_STATE_AP_MODE) {
    if (key == 'a' || key == 'A') {
      // Disable AP mode and return to network list
      stopAPMode();
      beep(TONE_MENU_NAV, BEEP_SHORT);

      // Rescan networks
      wifiState = WIFI_STATE_SCANNING;
      drawWiFiUI(display);
      scanNetworks();

      if (networkCount > 0) {
        wifiState = WIFI_STATE_NETWORK_LIST;
      } else {
        wifiState = WIFI_STATE_ERROR;
        statusMessage = "No networks found. Try again?";
      }

      drawWiFiUI(display);
      return 2;
    }
    else if (key == KEY_ESC) {
      // Exit WiFi settings but keep AP mode active
      return -1;
    }
  }

  return 0;
}

// Connect to WiFi network
void connectToWiFi(String ssid, String password) {
  Serial.print("Connecting to: ");
  Serial.println(ssid);

  // If we were in AP mode, stop it first
  bool wasInAPMode = isAPMode;
  if (isAPMode) {
    Serial.println("Stopping AP mode before connecting to WiFi...");
    // Stop web server if running
    extern bool webServerRunning;
    extern void stopWebServer();
    if (webServerRunning) {
      stopWebServer();
    }

    WiFi.softAPdisconnect(true);
    isAPMode = false;
    delay(100);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait up to 10 seconds for connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(250);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    wifiState = WIFI_STATE_CONNECTED;

    // Save credentials
    saveWiFiCredentials(ssid, password);

    // If we were in AP mode, set flag to auto-exit after showing success
    if (wasInAPMode) {
      Serial.println("Connection successful from AP mode - will return to main menu");
      connectedFromAPMode = true;
      connectionSuccessTime = millis();
      // The WiFi event handler will start the web server automatically
    }
  } else {
    Serial.println("Connection failed!");
    wifiState = WIFI_STATE_ERROR;

    // Track failed SSID for potential password retry
    failedSSID = ssid;

    // Check if this was a saved network (might have wrong password)
    String savedSSIDs[3];
    String savedPasswords[3];
    int savedCount = loadAllWiFiCredentials(savedSSIDs, savedPasswords);
    bool wasSaved = false;
    for (int j = 0; j < savedCount; j++) {
      if (ssid == savedSSIDs[j]) {
        wasSaved = true;
        break;
      }
    }

    if (wasSaved) {
      statusMessage = "Connection failed. Wrong password?";
    } else {
      statusMessage = "Failed to connect";
    }

    // If we were in AP mode and connection failed, restart AP mode
    if (wasInAPMode) {
      Serial.println("Connection failed - restarting AP mode...");
      startAPMode();
    }
  }
}

// Save WiFi credentials to flash memory (up to 3 networks)
void saveWiFiCredentials(String ssid, String password) {
  wifiPrefs.begin("wifi", false);

  // Load existing saved networks
  String ssid1 = wifiPrefs.getString("ssid1", "");
  String pass1 = wifiPrefs.getString("pass1", "");
  String ssid2 = wifiPrefs.getString("ssid2", "");
  String pass2 = wifiPrefs.getString("pass2", "");
  String ssid3 = wifiPrefs.getString("ssid3", "");
  String pass3 = wifiPrefs.getString("pass3", "");

  // Check if this SSID already exists in the list
  if (ssid == ssid1) {
    // Update slot 1
    wifiPrefs.putString("pass1", password);
    Serial.println("Updated existing network in slot 1");
  }
  else if (ssid == ssid2) {
    // Update slot 2
    wifiPrefs.putString("pass2", password);
    Serial.println("Updated existing network in slot 2");
  }
  else if (ssid == ssid3) {
    // Update slot 3
    wifiPrefs.putString("pass3", password);
    Serial.println("Updated existing network in slot 3");
  }
  else {
    // Add new network - shift existing ones down
    if (ssid1.length() == 0) {
      // Slot 1 is empty
      wifiPrefs.putString("ssid1", ssid);
      wifiPrefs.putString("pass1", password);
      Serial.println("Saved to slot 1");
    }
    else if (ssid2.length() == 0) {
      // Slot 2 is empty
      wifiPrefs.putString("ssid2", ssid);
      wifiPrefs.putString("pass2", password);
      Serial.println("Saved to slot 2");
    }
    else if (ssid3.length() == 0) {
      // Slot 3 is empty
      wifiPrefs.putString("ssid3", ssid);
      wifiPrefs.putString("pass3", password);
      Serial.println("Saved to slot 3");
    }
    else {
      // All slots full, shift down and add to slot 1 (most recent)
      wifiPrefs.putString("ssid3", ssid2);
      wifiPrefs.putString("pass3", pass2);
      wifiPrefs.putString("ssid2", ssid1);
      wifiPrefs.putString("pass2", pass1);
      wifiPrefs.putString("ssid1", ssid);
      wifiPrefs.putString("pass1", password);
      Serial.println("Saved to slot 1 (shifted others down, slot 3 dropped)");
    }
  }

  wifiPrefs.end();
  Serial.println("WiFi credentials saved");
}

// Load WiFi credentials from flash memory (loads all saved networks)
int loadAllWiFiCredentials(String ssids[3], String passwords[3]) {
  wifiPrefs.begin("wifi", true);

  int count = 0;

  ssids[0] = wifiPrefs.getString("ssid1", "");
  passwords[0] = wifiPrefs.getString("pass1", "");
  if (ssids[0].length() > 0) count++;

  ssids[1] = wifiPrefs.getString("ssid2", "");
  passwords[1] = wifiPrefs.getString("pass2", "");
  if (ssids[1].length() > 0) count++;

  ssids[2] = wifiPrefs.getString("ssid3", "");
  passwords[2] = wifiPrefs.getString("pass3", "");
  if (ssids[2].length() > 0) count++;

  wifiPrefs.end();

  return count;
}

// Load WiFi credentials from flash memory (legacy function for compatibility)
bool loadWiFiCredentials(String &ssid, String &password) {
  String ssids[3];
  String passwords[3];
  int count = loadAllWiFiCredentials(ssids, passwords);

  if (count > 0) {
    ssid = ssids[0];
    password = passwords[0];
    return true;
  }

  return false;
}

// Auto-connect to saved WiFi on startup (tries all 3 saved networks)
void autoConnectWiFi() {
  String ssids[3];
  String passwords[3];

  int count = loadAllWiFiCredentials(ssids, passwords);

  if (count == 0) {
    Serial.println("No saved WiFi credentials");
    return;
  }

  Serial.print("Found ");
  Serial.print(count);
  Serial.println(" saved network(s)");

  WiFi.mode(WIFI_STA);

  // Try each saved network in order
  for (int i = 0; i < count; i++) {
    Serial.print("Attempting to connect to: ");
    Serial.println(ssids[i]);

    WiFi.begin(ssids[i].c_str(), passwords[i].c_str());

    // Wait up to 10 seconds for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
      delay(250);
      Serial.print(".");
      attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Auto-connect successful!");
      Serial.print("Connected to: ");
      Serial.println(ssids[i]);
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      return;  // Successfully connected, exit
    } else {
      Serial.print("Failed to connect to: ");
      Serial.println(ssids[i]);
      WiFi.disconnect();
    }
  }

  Serial.println("Could not connect to any saved network");
}

// Reset WiFi settings - erase all saved credentials
void resetWiFiSettings() {
  Serial.println("Resetting WiFi settings...");

  wifiPrefs.begin("wifi", false);

  // Clear all saved network credentials
  wifiPrefs.putString("ssid1", "");
  wifiPrefs.putString("pass1", "");
  wifiPrefs.putString("ssid2", "");
  wifiPrefs.putString("pass2", "");
  wifiPrefs.putString("ssid3", "");
  wifiPrefs.putString("pass3", "");

  wifiPrefs.end();

  // Disconnect from current network
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  Serial.println("All WiFi credentials erased");
}

// Start AP mode - create access point for direct connection
void startAPMode() {
  Serial.println("Starting AP mode...");

  // Disconnect from any existing network
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);

  // Generate unique SSID based on chip ID
  uint32_t chipId = 0;
  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  String apSSID = "VAIL-SUMMIT-" + String(chipId, HEX);
  apSSID.toUpperCase();

  // Start AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID.c_str(), apPassword.c_str());

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP Mode started. SSID: ");
  Serial.println(apSSID);
  Serial.print("Password: ");
  Serial.println(apPassword);
  Serial.print("AP IP address: ");
  Serial.println(IP);

  isAPMode = true;

  // Start web server for AP mode
  extern bool webServerRunning;
  extern void setupWebServer();
  if (!webServerRunning) {
    Serial.println("Starting web server for AP mode...");
    setupWebServer();
  }
}

// Stop AP mode and switch back to station mode
void stopAPMode() {
  Serial.println("Stopping AP mode...");

  // Stop web server if running in AP mode
  extern bool webServerRunning;
  extern void stopWebServer();
  if (webServerRunning) {
    Serial.println("Stopping web server for AP mode...");
    stopWebServer();
  }

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);

  isAPMode = false;

  Serial.println("AP mode stopped");
}

// Check if web server should be running in AP mode
void updateAPModeWebServer() {
  extern bool webServerRunning;
  extern void setupWebServer();
  extern void stopWebServer();

  if (isAPMode && !webServerRunning) {
    Serial.println("Starting web server for AP mode...");
    setupWebServer();
  }
}

#endif // SETTINGS_WIFI_H
