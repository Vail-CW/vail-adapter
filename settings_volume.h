/*
 * Volume Settings Module
 * Handles audio volume control and settings display
 */

#ifndef SETTINGS_VOLUME_H
#define SETTINGS_VOLUME_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "config.h"

// Volume settings state
bool volumeSettingsActive = false;
int volumeValue = DEFAULT_VOLUME;
bool volumeChanged = false;

// Forward declaration
void drawVolumeDisplay(Adafruit_ST7789 &display);

/*
 * Initialize volume settings screen
 */
void initVolumeSettings(Adafruit_ST7789 &display) {
  volumeSettingsActive = true;
  volumeValue = getVolume();  // Get current volume from i2s_audio.h
  volumeChanged = false;

  // Clear screen
  display.fillScreen(COLOR_BACKGROUND);

  // Draw title
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(COLOR_TITLE);
  display.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("VOLUME", 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, 30);
  display.print("VOLUME");

  // Draw initial volume display
  drawVolumeDisplay(display);
}

/*
 * Draw volume level display
 */
void drawVolumeDisplay(Adafruit_ST7789 &display) {
  // Clear display area
  display.fillRect(0, 50, SCREEN_WIDTH, 140, COLOR_BACKGROUND);

  // Draw volume card with rounded corners
  int cardX = 30;
  int cardY = 70;
  int cardW = SCREEN_WIDTH - 60;
  int cardH = 100;
  int radius = 10;

  // Card background
  display.fillRoundRect(cardX, cardY, cardW, cardH, radius, ST77XX_BLUE);
  display.drawRoundRect(cardX, cardY, cardW, cardH, radius, ST77XX_WHITE);

  // Volume percentage text (large)
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(2);

  char volumeText[8];
  sprintf(volumeText, "%d%%", volumeValue);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(volumeText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, cardY + 60);
  display.print(volumeText);

  // Volume bar below percentage
  int barX = cardX + 20;
  int barY = cardY + 75;
  int barW = cardW - 40;
  int barH = 12;

  // Draw bar background (empty)
  display.fillRoundRect(barX, barY, barW, barH, 5, ST77XX_BLACK);

  // Draw filled portion based on volume
  int fillW = (barW * volumeValue) / 100;
  if (fillW > 0) {
    uint16_t barColor = ST77XX_GREEN;
    if (volumeValue < 30) barColor = ST77XX_RED;
    else if (volumeValue < 60) barColor = ST77XX_YELLOW;

    display.fillRoundRect(barX, barY, fillW, barH, 5, barColor);
  }

  // Draw footer help text
  display.setFont();
  display.setTextSize(1);
  display.setTextColor(ST77XX_WHITE);

  String helpText = "UP/DN Adjust  ENTER Save  ESC Cancel";
  int16_t x1_help, y1_help;
  uint16_t w_help, h_help;
  display.getTextBounds(helpText, 0, 0, &x1_help, &y1_help, &w_help, &h_help);
  int helpX = (SCREEN_WIDTH - w_help) / 2;
  display.setCursor(helpX, SCREEN_HEIGHT - 10);
  display.print(helpText);
}

/*
 * Handle volume settings input
 * Returns: -1 to exit, 0 to continue
 */
int handleVolumeInput(char key, Adafruit_ST7789 &display) {
  if (key == KEY_UP) {
    // Increase volume
    volumeValue = constrain(volumeValue + 5, VOLUME_MIN, VOLUME_MAX);
    volumeChanged = true;
    drawVolumeDisplay(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
  }
  else if (key == KEY_DOWN) {
    // Decrease volume
    volumeValue = constrain(volumeValue - 5, VOLUME_MIN, VOLUME_MAX);
    volumeChanged = true;
    drawVolumeDisplay(display);
    beep(TONE_MENU_NAV, BEEP_SHORT);
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Save volume and exit
    if (volumeChanged) {
      setVolume(volumeValue);  // Save to preferences
      beep(TONE_SELECT, BEEP_MEDIUM);
    }
    volumeSettingsActive = false;
    return -1;  // Exit to settings menu
  }
  else if (key == KEY_ESC) {
    // Cancel without saving
    beep(TONE_MENU_NAV, BEEP_SHORT);
    volumeSettingsActive = false;
    return -1;  // Exit to settings menu
  }

  return 0;  // Continue in volume settings
}

/*
 * Update volume settings (called in main loop)
 */
void updateVolumeSettings(Adafruit_ST7789 &display) {
  // Nothing to update in loop for now
  // Future: Could add visual feedback like pulsing animation
}

#endif // SETTINGS_VOLUME_H
