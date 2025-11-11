/*
 * Web Password Settings Module
 * Allows user to set/change password for web interface access
 * Password is stored and can be changed without knowing current password
 */

#ifndef SETTINGS_WEB_PASSWORD_H
#define SETTINGS_WEB_PASSWORD_H

#include <Preferences.h>
#include "config.h"

// Password settings state
enum PasswordState {
  PASSWORD_STATE_NORMAL,
  PASSWORD_STATE_CONFIRM_DISABLE
};

// Password settings globals
String webPasswordInput = "";
unsigned long passwordLastBlink = 0;
bool passwordCursorVisible = true;
Preferences webPasswordPrefs;
PasswordState passwordState = PASSWORD_STATE_NORMAL;

// Global password state (used by web_server.h)
String webPassword = "";
bool webAuthEnabled = false;

// Forward declarations
void startWebPasswordSettings(Adafruit_ST7789 &display);
void drawWebPasswordUI(Adafruit_ST7789 &display);
int handleWebPasswordInput(char key, Adafruit_ST7789 &display);
void saveWebPassword(String password);
bool loadWebPassword(String &password);
void clearWebPassword();

// Start web password settings mode
void startWebPasswordSettings(Adafruit_ST7789 &display) {
  // Start with empty input (no need to show current password)
  webPasswordInput = "";
  passwordState = PASSWORD_STATE_NORMAL;

  passwordCursorVisible = true;
  passwordLastBlink = millis();

  drawWebPasswordUI(display);
}

// Draw password input UI
void drawWebPasswordUI(Adafruit_ST7789 &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(COLOR_TITLE);
  display.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  String title = "Web Password";
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 65);
  display.print(title);
  display.setFont(); // Reset font

  // Current status
  display.setTextSize(1);
  if (webAuthEnabled && webPassword.length() > 0) {
    display.setTextColor(ST77XX_GREEN);
    String status = "Status: ENABLED";
    display.setCursor((SCREEN_WIDTH - status.length() * 6) / 2, 85);
    display.print(status);
  } else {
    display.setTextColor(COLOR_WARNING);
    String status = "Status: DISABLED";
    display.setCursor((SCREEN_WIDTH - status.length() * 6) / 2, 85);
    display.print(status);
  }

  // Instructions
  display.setTextSize(1);
  display.setTextColor(0x7BEF); // Light gray
  String prompt = "Enter new password (8-16 chars)";
  display.setCursor((SCREEN_WIDTH - prompt.length() * 6) / 2, 105);
  display.print(prompt);

  // Input box
  int boxX = 30;
  int boxY = 120;
  int boxW = SCREEN_WIDTH - 60;
  int boxH = 50;

  display.fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x1082); // Dark blue fill
  display.drawRoundRect(boxX, boxY, boxW, boxH, 8, 0x34BF); // Light blue outline

  // Display password input as asterisks
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(1);

  String maskedPassword = "";
  for (unsigned int i = 0; i < webPasswordInput.length(); i++) {
    maskedPassword += "*";
  }

  display.getTextBounds(maskedPassword.c_str(), 0, 0, &x1, &y1, &w, &h);
  int textX = boxX + 15;
  int textY = boxY + (boxH / 2) + (h / 2) + 5;
  display.setCursor(textX, textY);
  display.print(maskedPassword);

  // Blinking cursor
  if (passwordCursorVisible) {
    int cursorX = textX + w + 5;
    if (cursorX < boxX + boxW - 10) {
      display.fillRect(cursorX, textY - h, 3, h + 5, COLOR_WARNING);
    }
  }

  display.setFont(); // Reset font

  // Footer with controls
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String footerText = "ENTER Save";
  display.setCursor(40, SCREEN_HEIGHT - 30);
  display.print(footerText);

  display.setTextColor(ST77XX_RED);
  String clearText = "DEL Disable";
  display.setCursor(130, SCREEN_HEIGHT - 30);
  display.print(clearText);

  display.setTextColor(0x7BEF);
  String escText = "ESC Cancel";
  display.setCursor(230, SCREEN_HEIGHT - 30);
  display.print(escText);

  // Password strength indicator
  if (webPasswordInput.length() > 0) {
    int strength = 0;
    if (webPasswordInput.length() >= 8) strength++;
    if (webPasswordInput.length() >= 12) strength++;

    bool hasUpper = false, hasLower = false, hasDigit = false;
    for (unsigned int i = 0; i < webPasswordInput.length(); i++) {
      if (isupper(webPasswordInput[i])) hasUpper = true;
      if (islower(webPasswordInput[i])) hasLower = true;
      if (isdigit(webPasswordInput[i])) hasDigit = true;
    }
    if (hasUpper && hasLower) strength++;
    if (hasDigit) strength++;

    uint16_t strengthColor;
    String strengthText;
    if (strength <= 1) {
      strengthColor = ST77XX_RED;
      strengthText = "Weak";
    } else if (strength <= 2) {
      strengthColor = COLOR_WARNING;
      strengthText = "Fair";
    } else if (strength <= 3) {
      strengthColor = ST77XX_YELLOW;
      strengthText = "Good";
    } else {
      strengthColor = ST77XX_GREEN;
      strengthText = "Strong";
    }

    display.setTextSize(1);
    display.setTextColor(strengthColor);
    display.setCursor((SCREEN_WIDTH - strengthText.length() * 6) / 2, 180);
    display.print(strengthText);
  }
}

// Handle web password input
int handleWebPasswordInput(char key, Adafruit_ST7789 &display) {
  // Handle confirmation state first
  if (passwordState == PASSWORD_STATE_CONFIRM_DISABLE) {
    if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
      // Confirm disable
      clearWebPassword();
      webPassword = "";
      webAuthEnabled = false;
      beep(TONE_SELECT, BEEP_MEDIUM);

      display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);
      display.setTextSize(2);
      display.setTextColor(ST77XX_YELLOW);
      display.setCursor(45, 110);
      display.print("Password Disabled");
      display.setTextSize(1);
      display.setTextColor(0x7BEF);
      display.setCursor(55, 135);
      display.print("Web access now open");
      delay(2000);

      return -1;  // Exit settings
    } else if (key == KEY_ESC) {
      // Cancel disable, return to normal state
      beep(TONE_MENU_NAV, BEEP_SHORT);
      passwordState = PASSWORD_STATE_NORMAL;
      drawWebPasswordUI(display);
      return 0;
    }
    return 0;  // Ignore other keys in confirmation state
  }

  // Normal state handling
  // Update cursor blink
  if (millis() - passwordLastBlink > 500) {
    passwordCursorVisible = !passwordCursorVisible;
    passwordLastBlink = millis();
    drawWebPasswordUI(display);
  }

  if (key == KEY_BACKSPACE) {
    if (webPasswordInput.length() > 0) {
      webPasswordInput.remove(webPasswordInput.length() - 1);
      passwordCursorVisible = true;
      passwordLastBlink = millis();
      drawWebPasswordUI(display);
    }
    return 1;
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Save password (must be 8-16 characters)
    if (webPasswordInput.length() >= 8 && webPasswordInput.length() <= 16) {
      saveWebPassword(webPasswordInput);
      webPassword = webPasswordInput;
      webAuthEnabled = true;
      beep(TONE_SELECT, BEEP_MEDIUM);

      // Show success message
      display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);
      display.setTextSize(2);
      display.setTextColor(ST77XX_GREEN);
      display.setCursor(55, 110);
      display.print("Password Set!");
      display.setTextSize(1);
      display.setTextColor(ST77XX_WHITE);
      display.setCursor(45, 135);
      display.print("Web access now protected");
      delay(2000);

      return -1;  // Exit settings
    } else if (webPasswordInput.length() > 0) {
      // Too short or too long
      beep(TONE_ERROR, BEEP_MEDIUM);
      display.fillRect(0, 185, SCREEN_WIDTH, 20, COLOR_BACKGROUND);
      display.setTextSize(1);
      display.setTextColor(ST77XX_RED);
      String errorMsg = "Must be 8-16 characters";
      display.setCursor((SCREEN_WIDTH - errorMsg.length() * 6) / 2, 195);
      display.print(errorMsg);
      delay(1500);
      drawWebPasswordUI(display);
    }
    return 0;
  }
  else if (key == 0x7F || key == 'd' || key == 'D') {
    // DEL key - show disable confirmation
    passwordState = PASSWORD_STATE_CONFIRM_DISABLE;
    beep(TONE_MENU_NAV, BEEP_SHORT);

    display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);
    display.setTextSize(2);
    display.setTextColor(COLOR_WARNING);
    display.setCursor(65, 100);
    display.print("Disable Web");
    display.setCursor(75, 125);
    display.print("Password?");

    display.setTextSize(1);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(70, 155);
    display.print("ENTER Confirm  ESC Cancel");

    return 0;
  }
  else if (key == KEY_ESC) {
    // Cancel without saving
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return -1;  // Exit settings
  }
  else if (key >= 32 && key <= 126 && webPasswordInput.length() < 16) {
    // Add printable character (max 16 chars for password)
    // Accept alphanumeric and special characters
    webPasswordInput += key;
    passwordCursorVisible = true;
    passwordLastBlink = millis();
    beep(TONE_MENU_NAV, BEEP_SHORT);
    drawWebPasswordUI(display);
    return 1;
  }

  return 0;
}

// Save web password to flash memory
void saveWebPassword(String password) {
  webPasswordPrefs.begin("webpw", false);
  webPasswordPrefs.putString("pw", password);
  webPasswordPrefs.putBool("enabled", true);
  webPasswordPrefs.end();

  Serial.println("Web password saved and enabled");
}

// Clear/disable web password
void clearWebPassword() {
  webPasswordPrefs.begin("webpw", false);
  webPasswordPrefs.remove("pw");
  webPasswordPrefs.putBool("enabled", false);
  webPasswordPrefs.end();

  Serial.println("Web password disabled");
}

// Load web password from flash memory
bool loadWebPassword(String &password) {
  webPasswordPrefs.begin("webpw", true);
  bool enabled = webPasswordPrefs.getBool("enabled", false);
  password = webPasswordPrefs.getString("pw", "");
  webPasswordPrefs.end();

  return (enabled && password.length() > 0);
}

// Load web password on startup (call from setup())
void loadSavedWebPassword() {
  String savedPassword;
  if (loadWebPassword(savedPassword)) {
    webPassword = savedPassword;
    webAuthEnabled = true;
    Serial.println("Web password protection enabled");
  } else {
    webPassword = "";
    webAuthEnabled = false;
    Serial.println("Web password protection disabled");
  }
}

#endif // SETTINGS_WEB_PASSWORD_H
