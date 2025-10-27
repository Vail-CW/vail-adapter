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
    statusMessage = "No networks found";
  }

  drawWiFiUI(display);
}

// Scan for WiFi networks
void scanNetworks() {
  Serial.println("Scanning for WiFi networks...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();
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
    footerText = "Up/Down: Select  Enter: Connect  ESC: Back";
  } else if (wifiState == WIFI_STATE_PASSWORD_INPUT) {
    footerText = "Type password  Enter: Connect  ESC: Cancel";
  } else if (wifiState == WIFI_STATE_CONNECTED || wifiState == WIFI_STATE_ERROR) {
    footerText = "Press ESC to return";
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
  display.setTextSize(1);
  display.setTextColor(ST77XX_CYAN);
  display.setCursor(10, 55);
  display.print("Available Networks:");

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
    display.setCursor(networks[i].encrypted ? 42 : 32, yPos + 6);

    // Truncate long SSIDs
    String ssid = networks[i].ssid;
    if (ssid.length() > 30) {
      ssid = ssid.substring(0, 27) + "...";
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

// Save WiFi credentials to flash memory
void saveWiFiCredentials(String ssid, String password) {
  wifiPrefs.begin("wifi", false);
  wifiPrefs.putString("ssid", ssid);
  wifiPrefs.putString("password", password);
  wifiPrefs.end();

  Serial.println("WiFi credentials saved");
}

// Load WiFi credentials from flash memory
bool loadWiFiCredentials(String &ssid, String &password) {
  wifiPrefs.begin("wifi", true);
  ssid = wifiPrefs.getString("ssid", "");
  password = wifiPrefs.getString("password", "");
  wifiPrefs.end();

  return (ssid.length() > 0);
}

// Auto-connect to saved WiFi on startup
void autoConnectWiFi() {
  String ssid, password;

  if (loadWiFiCredentials(ssid, password)) {
    Serial.print("Auto-connecting to saved network: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait up to 10 seconds
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
      delay(250);
      Serial.print(".");
      attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Auto-connect successful!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Auto-connect failed");
    }
  } else {
    Serial.println("No saved WiFi credentials");
  }
}

#endif // SETTINGS_WIFI_H
