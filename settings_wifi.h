/*
 * WiFi Settings Module
 * Handles WiFi network scanning, connection, and credential storage
 */

#ifndef SETTINGS_WIFI_H
#define SETTINGS_WIFI_H

#include <WiFi.h>
#include <Preferences.h>
#include "config.h"

// WiFi settings state machine
enum WiFiSettingsState {
  WIFI_STATE_SCANNING,
  WIFI_STATE_NETWORK_LIST,
  WIFI_STATE_PASSWORD_INPUT,
  WIFI_STATE_CONNECTING,
  WIFI_STATE_CONNECTED,
  WIFI_STATE_ERROR
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

// Forward declarations
void startWiFiSettings(Adafruit_ST7789 &display);
void drawWiFiUI(Adafruit_ST7789 &display);
int handleWiFiInput(char key, Adafruit_ST7789 &display);
void scanNetworks();
void drawNetworkList(Adafruit_ST7789 &display);
void drawPasswordInput(Adafruit_ST7789 &display);
void connectToWiFi(String ssid, String password);
void saveWiFiCredentials(String ssid, String password);
int loadAllWiFiCredentials(String ssids[3], String passwords[3]);
bool loadWiFiCredentials(String &ssid, String &password);
void autoConnectWiFi();

// Start WiFi settings mode
void startWiFiSettings(Adafruit_ST7789 &display) {
  wifiState = WIFI_STATE_SCANNING;
  selectedNetwork = 0;
  passwordInput = "";
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

// Draw WiFi UI based on current state
void drawWiFiUI(Adafruit_ST7789 &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  if (wifiState == WIFI_STATE_SCANNING) {
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

  // Draw footer instructions
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String footerText = "";

  if (wifiState == WIFI_STATE_NETWORK_LIST) {
    footerText = "*=Saved  Up/Down:Select  Enter:Connect  ESC:Back";
  } else if (wifiState == WIFI_STATE_PASSWORD_INPUT) {
    footerText = "Type password  Enter: Connect  ESC: Cancel";
  } else if (wifiState == WIFI_STATE_CONNECTED) {
    footerText = "Press ESC to return";
  } else if (wifiState == WIFI_STATE_ERROR) {
    footerText = "Enter: Rescan  ESC: Return";
  }

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(footerText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, SCREEN_HEIGHT - 12);
  display.print(footerText);
}

// Draw network list
void drawNetworkList(Adafruit_ST7789 &display) {
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

// Draw password input screen
void drawPasswordInput(Adafruit_ST7789 &display) {
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
int handleWiFiInput(char key, Adafruit_ST7789 &display) {
  // Update cursor blink
  if (wifiState == WIFI_STATE_PASSWORD_INPUT && millis() - lastBlink > 500) {
    cursorVisible = !cursorVisible;
    lastBlink = millis();
    drawPasswordInput(display);
  }

  if (wifiState == WIFI_STATE_NETWORK_LIST) {
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
      // Check if network requires password
      if (networks[selectedNetwork].encrypted) {
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
    if (key == KEY_ESC) {
      return -1;  // Exit WiFi settings
    }
    else if (wifiState == WIFI_STATE_ERROR && (key == KEY_ENTER || key == KEY_ENTER_ALT)) {
      // Rescan networks on ENTER when in error state
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

  return 0;
}

// Connect to WiFi network
void connectToWiFi(String ssid, String password) {
  Serial.print("Connecting to: ");
  Serial.println(ssid);

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
  } else {
    Serial.println("Connection failed!");
    wifiState = WIFI_STATE_ERROR;
    statusMessage = "Failed to connect";
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

#endif // SETTINGS_WIFI_H
