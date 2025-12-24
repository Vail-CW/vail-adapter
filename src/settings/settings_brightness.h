/*
 * Brightness Settings Module
 * Handles LCD backlight brightness control via PWM
 */

#ifndef SETTINGS_BRIGHTNESS_H
#define SETTINGS_BRIGHTNESS_H

#include <Preferences.h>
#include "../core/config.h"

// Brightness settings state
bool brightnessSettingsActive = false;
int brightnessValue = DEFAULT_BRIGHTNESS;
bool brightnessChanged = false;

// Preferences for persistent storage
Preferences brightnessPrefs;

// Forward declaration
void drawBrightnessDisplay(LGFX &display);

/*
 * Setup PWM for backlight control
 * Call this once during setup() before using brightness functions
 */
void setupBrightnessPWM() {
  ledcSetup(BRIGHTNESS_PWM_CHANNEL, BRIGHTNESS_PWM_FREQ, BRIGHTNESS_PWM_RESOLUTION);
  ledcAttachPin(TFT_BL, BRIGHTNESS_PWM_CHANNEL);
}

/*
 * Apply brightness value (0-100%) to backlight
 */
void applyBrightness(int percent) {
  int pwmValue = map(percent, 0, 100, 0, 255);
  ledcWrite(BRIGHTNESS_PWM_CHANNEL, pwmValue);
}

/*
 * Load brightness from preferences
 */
void loadBrightnessSettings() {
  brightnessPrefs.begin("display", true);  // Read-only
  brightnessValue = brightnessPrefs.getInt("brightness", DEFAULT_BRIGHTNESS);
  brightnessPrefs.end();

  // Ensure value is within bounds
  brightnessValue = constrain(brightnessValue, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
}

/*
 * Save brightness to preferences
 */
void saveBrightnessSettings() {
  brightnessPrefs.begin("display", false);  // Read-write
  brightnessPrefs.putInt("brightness", brightnessValue);
  brightnessPrefs.end();
}

/*
 * Get current brightness value
 */
int getBrightness() {
  return brightnessValue;
}

/*
 * Set brightness value and apply it
 */
void setBrightness(int percent) {
  brightnessValue = constrain(percent, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  applyBrightness(brightnessValue);
  saveBrightnessSettings();
}

/*
 * Initialize brightness settings screen
 */
void initBrightnessSettings(LGFX &display) {
  brightnessSettingsActive = true;
  loadBrightnessSettings();
  brightnessChanged = false;

  // Clear screen
  display.fillScreen(COLOR_BACKGROUND);

  // Draw title
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(COLOR_TITLE);
  display.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, "BRIGHTNESS", 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, 30);
  display.print("BRIGHTNESS");

  // Draw initial brightness display
  drawBrightnessDisplay(display);
}

/*
 * Draw brightness level display
 */
void drawBrightnessDisplay(LGFX &display) {
  // Clear display area
  display.fillRect(0, 50, SCREEN_WIDTH, 140, COLOR_BACKGROUND);

  // Clean brightness card
  int cardX = 30;
  int cardY = 70;
  int cardW = SCREEN_WIDTH - 60;
  int cardH = 100;

  // Solid card background
  display.fillRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BG_LAYER2);

  // Subtle border
  display.drawRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BORDER_SUBTLE);

  // Brightness percentage text (large)
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(COLOR_TEXT_PRIMARY);
  display.setTextSize(2);

  char brightnessText[8];
  sprintf(brightnessText, "%d%%", brightnessValue);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(display, brightnessText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, cardY + 50);
  display.print(brightnessText);

  // Clean brightness bar below percentage
  int barX = cardX + 20;
  int barY = cardY + 70;
  int barW = cardW - 40;
  int barH = 14;

  // Bar container (dark background)
  display.fillRoundRect(barX, barY, barW, barH, 7, COLOR_BG_DEEP);

  // Filled portion with solid color based on brightness
  int fillW = (barW * brightnessValue) / 100;
  if (fillW > 0) {
    // Single solid color based on brightness level
    uint16_t fillColor;
    if (brightnessValue > 70) {
      fillColor = COLOR_WARNING_PASTEL;  // Warm orange/yellow for bright
    } else if (brightnessValue > 30) {
      fillColor = COLOR_ACCENT_CYAN;     // Cyan for medium
    } else {
      fillColor = COLOR_CARD_TEAL;       // Teal for dim
    }

    display.fillRoundRect(barX + 2, barY + 2, fillW - 4, barH - 4, 5, fillColor);
  }

  // Border on bar
  display.drawRoundRect(barX, barY, barW, barH, 7, COLOR_BORDER_LIGHT);

  // Draw footer help text
  display.setFont(nullptr);
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);

  String helpText = "UP/DN Adjust  ENTER Save  ESC Cancel";
  int16_t x1_help, y1_help;
  uint16_t w_help, h_help;
  getTextBounds_compat(display, helpText.c_str(), 0, 0, &x1_help, &y1_help, &w_help, &h_help);
  int helpX = (SCREEN_WIDTH - w_help) / 2;
  display.setCursor(helpX, SCREEN_HEIGHT - 10);
  display.print(helpText);
}

/*
 * Handle brightness settings input
 * Returns: -1 to exit, 0 to continue
 */
int handleBrightnessInput(char key, LGFX &display) {
  if (key == KEY_UP) {
    // Increase brightness
    brightnessValue = constrain(brightnessValue + 5, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    brightnessChanged = true;
    applyBrightness(brightnessValue);  // Live preview
    drawBrightnessDisplay(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
  }
  else if (key == KEY_DOWN) {
    // Decrease brightness
    brightnessValue = constrain(brightnessValue - 5, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    brightnessChanged = true;
    applyBrightness(brightnessValue);  // Live preview
    drawBrightnessDisplay(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Save brightness and exit
    if (brightnessChanged) {
      saveBrightnessSettings();
      beep(TONE_SELECT, BEEP_MEDIUM);
    }
    brightnessSettingsActive = false;
    return -1;  // Exit to settings menu
  }
  else if (key == KEY_ESC) {
    // Cancel without saving - restore previous brightness
    if (brightnessChanged) {
      loadBrightnessSettings();
      applyBrightness(brightnessValue);
    }
    beep(TONE_MENU_NAV, BEEP_SHORT);
    brightnessSettingsActive = false;
    return -1;  // Exit to settings menu
  }

  return 0;  // Continue in brightness settings
}

/*
 * Update brightness settings (called in main loop)
 */
void updateBrightnessSettings(LGFX &display) {
  // Nothing to update in loop for now
}

#endif // SETTINGS_BRIGHTNESS_H
