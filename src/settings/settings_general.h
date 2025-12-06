/*
 * General Settings Module
 * Allows user to configure general/miscellaneous settings
 * Currently includes: Callsign/Name for Vail repeater
 */

#ifndef SETTINGS_GENERAL_H
#define SETTINGS_GENERAL_H

#include <Preferences.h>
#include "../core/config.h"
#include "../network/vail_repeater.h"

// Callsign settings globals
String callsignInput = "";
unsigned long generalLastBlink = 0;
bool generalCursorVisible = true;
Preferences callsignPrefs;

// Forward declarations
void startCallsignSettings(LGFX &display);
void drawCallsignUI(LGFX &display);
int handleCallsignInput(char key, LGFX &display);
void saveCallsign(String callsign);
bool loadCallsign(String &callsign);

// Start callsign settings mode
void startCallsignSettings(LGFX &display) {
  // Load current callsign
  loadCallsign(callsignInput);
  if (callsignInput.length() == 0) {
    callsignInput = vailCallsign;  // Use current Vail callsign as default
  }

  generalCursorVisible = true;
  generalLastBlink = millis();

  drawCallsignUI(display);
}

// Draw callsign input UI
void drawCallsignUI(LGFX &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  display.setFont(nullptr);  // Use default font
  display.setTextColor(COLOR_TITLE);
  display.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  String title = "Enter Callsign";
  getTextBounds_compat(display, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 75);
  display.print(title);
  display.setFont(nullptr); // Reset font

  // Instructions
  display.setTextSize(1);
  display.setTextColor(0x7BEF); // Light gray
  String prompt = "For use with Vail repeater";
  display.setCursor((SCREEN_WIDTH - prompt.length() * 6) / 2, 95);
  display.print(prompt);

  // Input box
  int boxX = 30;
  int boxY = 115;
  int boxW = SCREEN_WIDTH - 60;
  int boxH = 50;

  display.fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x1082); // Dark blue fill
  display.drawRoundRect(boxX, boxY, boxW, boxH, 8, 0x34BF); // Light blue outline

  // Display callsign input
  display.setFont(nullptr);  // Use default font
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(1);

  getTextBounds_compat(display, callsignInput.c_str(), 0, 0, &x1, &y1, &w, &h);
  int textX = boxX + 15;
  int textY = boxY + (boxH / 2) + (h / 2) + 5;
  display.setCursor(textX, textY);
  display.print(callsignInput);

  // Blinking cursor
  if (generalCursorVisible) {
    int cursorX = textX + w + 5;
    if (cursorX < boxX + boxW - 10) {
      display.fillRect(cursorX, textY - h, 3, h + 5, COLOR_WARNING);
    }
  }

  display.setFont(nullptr); // Reset font

  // Footer with controls
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String footerText = "Type callsign  ENTER Save  ESC Cancel";
  getTextBounds_compat(display, footerText.c_str(), 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, SCREEN_HEIGHT - 12);
  display.print(footerText);
}

// Handle callsign settings input
int handleCallsignInput(char key, LGFX &display) {
  // Update cursor blink
  if (millis() - generalLastBlink > 500) {
    generalCursorVisible = !generalCursorVisible;
    generalLastBlink = millis();
    drawCallsignUI(display);
  }

  if (key == KEY_BACKSPACE) {
    if (callsignInput.length() > 0) {
      callsignInput.remove(callsignInput.length() - 1);
      generalCursorVisible = true;
      generalLastBlink = millis();
      drawCallsignUI(display);
    }
    return 1;
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Save callsign
    if (callsignInput.length() > 0) {
      callsignInput.toUpperCase(); // Convert to uppercase
      saveCallsign(callsignInput);
      vailCallsign = callsignInput;  // Update global Vail callsign
      beep(TONE_SELECT, BEEP_MEDIUM);

      // Show success message
      display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);
      display.setTextSize(2);
      display.setTextColor(ST77XX_GREEN);
      display.setCursor(90, 110);
      display.print("Saved!");
      delay(1000);

      return -1;  // Exit settings
    }
    return 0;
  }
  else if (key == KEY_ESC) {
    // Cancel without saving
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return -1;  // Exit settings
  }
  else if (key >= 32 && key <= 126 && callsignInput.length() < 12) {
    // Add printable character (max 12 chars for callsign)
    // Only accept alphanumeric
    char c = toupper(key);
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
      callsignInput += c;
      generalCursorVisible = true;
      generalLastBlink = millis();
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawCallsignUI(display);
    }
    return 1;
  }

  return 0;
}

// Save callsign to flash memory
void saveCallsign(String callsign) {
  callsignPrefs.begin("callsign", false);
  callsignPrefs.putString("call", callsign);
  callsignPrefs.end();

  Serial.print("Callsign saved: ");
  Serial.println(callsign);
}

// Load callsign from flash memory
bool loadCallsign(String &callsign) {
  callsignPrefs.begin("callsign", true);
  callsign = callsignPrefs.getString("call", "");
  callsignPrefs.end();

  return (callsign.length() > 0);
}

// Load callsign on startup (call from setup())
void loadSavedCallsign() {
  String savedCallsign;
  if (loadCallsign(savedCallsign)) {
    vailCallsign = savedCallsign;
    Serial.print("Loaded callsign: ");
    Serial.println(savedCallsign);
  } else {
    Serial.println("No saved callsign, using default: GUEST");
  }
}

#endif // SETTINGS_GENERAL_H
