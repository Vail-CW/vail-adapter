/*
 * QSO Logger UI Module
 * All UI rendering for QSO Logger modes
 */

#ifndef QSO_LOGGER_UI_H
#define QSO_LOGGER_UI_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "../core/config.h"
#include "qso_logger.h"  // Same folder

// Forward declarations from main file
extern Adafruit_ST7789 tft;
void drawHeader();

// ============================================
// Tools Menu
// ============================================

/*
 * Draw Tools submenu
 * Currently only has QSO Logger, but designed for future expansion
 */
void drawToolsMenu(Adafruit_ST7789& tft) {
  // Tools menu is drawn by main menu system using drawMenuItems()
  // This function is a placeholder for future custom rendering
}

// ============================================
// QSO Logger Menu
// ============================================

/*
 * Draw QSO Logger submenu
 */
void drawQSOLoggerMenu(Adafruit_ST7789& tft) {
  // QSO Logger menu is drawn by main menu system using drawMenuItems()
  // This function is a placeholder for future custom rendering
}

// ============================================
// Log Entry Screen
// ============================================

/*
 * Draw QSO Log Entry UI
 * Card-based form matching existing VAIL SUMMIT style
 */
void drawQSOLogEntryUI(Adafruit_ST7789& tft) {
  // Draw header
  drawHeader();

  // Clear content area
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42 - 20, COLOR_BACKGROUND);

  // Get field labels and values
  String fieldLabels[11] = {
    "Callsign",
    "Frequency",
    "Mode",
    "RST Sent",
    "RST Rcvd",
    "Date/Time",
    "My Grid",
    "My POTA",
    "Their Grid",
    "Their POTA",
    "Notes"
  };

  String fieldValues[11] = {
    String(logEntryState.callsign),
    String(logEntryState.frequency),
    String(QSO_MODES[logEntryState.modeIndex]),
    String(logEntryState.rstSent),
    String(logEntryState.rstRcvd),
    String(logEntryState.date) + " " + String(logEntryState.time),
    String(logEntryState.myGrid),
    String(logEntryState.myPOTA),
    String(logEntryState.theirGrid),
    String(logEntryState.theirPOTA),
    String(logEntryState.notes)
  };

  // Draw current field (large card)
  int currentField = logEntryState.currentField;
  int cardY = 55;
  int cardHeight = 50;

  // Draw main field card
  tft.fillRoundRect(10, cardY, 300, cardHeight, 8, 0x1082); // Dark blue
  tft.drawRoundRect(10, cardY, 300, cardHeight, 8, ST77XX_CYAN); // Cyan outline for active

  // Field label
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING); // Yellow label
  tft.setCursor(18, cardY + 12);
  tft.print(fieldLabels[currentField]);

  // Field value (larger text)
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(18, cardY + 28);

  // Show value or placeholder
  if (fieldValues[currentField].length() > 0) {
    // Truncate if too long
    String displayValue = fieldValues[currentField];
    if (displayValue.length() > 18) {
      displayValue = displayValue.substring(0, 18);
    }
    tft.print(displayValue);

    // Cursor blink if editing
    if (logEntryState.isEditing && (millis() / 500) % 2 == 0) {
      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(displayValue, 0, 0, &x1, &y1, &w, &h);
      tft.fillRect(18 + w + 2, cardY + 28, 3, 16, COLOR_WARNING);
    }
  } else {
    // Placeholder
    tft.setTextColor(0x7BEF); // Dimmed
    tft.print("(empty)");
  }

  // Draw mini preview of other fields (3 visible)
  int previewY = cardY + cardHeight + 8;
  int previewCount = 0;
  for (int i = 1; i <= 3 && currentField + i < 11; i++) {
    int idx = currentField + i;
    int py = previewY + (previewCount * 22);

    // Small card
    tft.fillRoundRect(15, py, 290, 18, 4, 0x2104); // Very dark
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF); // Dimmed
    tft.setCursor(20, py + 6);
    tft.print(fieldLabels[idx]);

    // Value preview (truncated)
    if (fieldValues[idx].length() > 0) {
      String preview = ": " + fieldValues[idx];
      if (preview.length() > 22) {
        preview = preview.substring(0, 22);
      }
      tft.print(preview);
    }

    previewCount++;
  }

  // Progress indicator
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, cardY + cardHeight + 85);
  String progress = "Field " + String(currentField + 1) + " of 11";
  tft.print(progress);

  // Footer with context-sensitive help
  int footerY = SCREEN_HEIGHT - 12;
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING); // Yellow

  // Clear footer area first
  tft.fillRect(0, footerY - 4, SCREEN_WIDTH, 16, COLOR_BACKGROUND);

  String helpText;

  if (currentField == FIELD_MODE) {
    helpText = "< > Mode  TAB Next  ENT Save";
  } else if (currentField == FIELD_DATE_TIME) {
    helpText = "Type Date/Time or N=Now  ENT=Save";
  } else if (currentField == FIELD_NOTES) {
    helpText = "Type  TAB Next  ENT Save";
  } else {
    helpText = "Type  TAB Next  ESC Back";
  }

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, footerY);
  tft.print(helpText);
}

// ============================================
// View Logs Screen
// ============================================

/*
 * Draw View Logs UI
 * Scrollable list of logged contacts
 */
void drawQSOViewLogsUI(Adafruit_ST7789& tft) {
  // Draw header
  drawHeader();

  // Clear content area
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42 - 20, COLOR_BACKGROUND);

  // Title
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(1);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds("VIEW LOGS", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 70);
  tft.print("VIEW LOGS");
  tft.setFont(); // Reset font

  // Placeholder message
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(40, 110);
  tft.print("No Logs Yet");

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF); // Dimmed white
  tft.setCursor(30, 135);
  tft.print("Log viewer coming in M4");

  // Footer
  int footerY = SCREEN_HEIGHT - 12;
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING); // Yellow
  String helpText = "ESC Back";
  tft.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, footerY);
  tft.print(helpText);
}

// ============================================
// Statistics Screen
// ============================================

/*
 * Draw Statistics UI
 * Summary statistics and band/mode breakdown
 */
void drawQSOStatisticsUI(Adafruit_ST7789& tft) {
  // Draw header
  drawHeader();

  // Clear content area
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42 - 20, COLOR_BACKGROUND);

  // Title
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(1);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds("STATISTICS", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 70);
  tft.print("STATISTICS");
  tft.setFont(); // Reset font

  // Placeholder message
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(40, 110);
  tft.print("Total: 0");

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF); // Dimmed white
  tft.setCursor(30, 135);
  tft.print("Stats coming in Milestone 5");

  // Footer
  int footerY = SCREEN_HEIGHT - 12;
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING); // Yellow
  String helpText = "ESC Back";
  tft.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, footerY);
  tft.print(helpText);
}

// ============================================
// Export Screen
// ============================================

/*
 * Draw Export UI
 * Export logs to ADIF format
 */
void drawQSOExportUI(Adafruit_ST7789& tft) {
  // Draw header
  drawHeader();

  // Clear content area
  tft.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42 - 20, COLOR_BACKGROUND);

  // Title
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TITLE);
  tft.setTextSize(1);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds("EXPORT LOGS", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 70);
  tft.print("EXPORT LOGS");
  tft.setFont(); // Reset font

  // Placeholder message
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(40, 110);
  tft.print("ADIF Export");

  tft.setTextSize(1);
  tft.setTextColor(0x7BEF); // Dimmed white
  tft.setCursor(30, 135);
  tft.print("Export coming in M6");

  // Footer
  int footerY = SCREEN_HEIGHT - 12;
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING); // Yellow
  String helpText = "ESC Back";
  tft.getTextBounds(helpText, 0, 0, &x1, &y1, &w, &h);
  int centerX = (SCREEN_WIDTH - w) / 2;
  tft.setCursor(centerX, footerY);
  tft.print(helpText);
}

#endif // QSO_LOGGER_UI_H
