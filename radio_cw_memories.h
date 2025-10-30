/*
 * CW Memories Module (Placeholder)
 * Future implementation: Store and playback CW message memories
 */

#ifndef RADIO_CW_MEMORIES_H
#define RADIO_CW_MEMORIES_H

#include "config.h"

// Draw CW Memories placeholder UI
void drawCWMemoriesUI(Adafruit_ST7789 &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Modern card container
  int cardX = 40;
  int cardY = 80;
  int cardW = SCREEN_WIDTH - 80;
  int cardH = 100;

  display.fillRoundRect(cardX, cardY, cardW, cardH, 12, 0x1082); // Dark blue fill
  display.drawRoundRect(cardX, cardY, cardW, cardH, 12, 0x34BF); // Light blue outline

  // Title
  display.setTextSize(2);
  display.setTextColor(ST77XX_CYAN);
  int16_t x1, y1;
  uint16_t w, h;
  String title = "CW Memories";
  display.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, cardY + 25);
  display.print(title);

  // Coming soon message
  display.setTextSize(1);
  display.setTextColor(0x7BEF); // Light gray
  String message = "Coming Soon...";
  display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
  centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, cardY + 55);
  display.print(message);

  // Footer with instructions
  display.setTextSize(1);
  display.setTextColor(COLOR_WARNING);
  String helpText = "ESC Back";
  display.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  centerX = (SCREEN_WIDTH - w) / 2;
  display.setCursor(centerX, SCREEN_HEIGHT - 12);
  display.print(helpText);
}

#endif // RADIO_CW_MEMORIES_H
