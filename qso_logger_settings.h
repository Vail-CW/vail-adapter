// QSO Logger Settings Module
// Configure location (grid square or POTA park) for logging

#ifndef QSO_LOGGER_SETTINGS_H
#define QSO_LOGGER_SETTINGS_H

#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include <Preferences.h>
#include "config.h"
#include "qso_logger.h"
#include "pota_api.h"

// ============================================
// Logger Settings State
// ============================================

enum LocationInputMode {
  LOC_MODE_GRID = 0,
  LOC_MODE_POTA = 1
};

struct LoggerSettingsState {
  LocationInputMode inputMode;
  int currentField;           // 0=mode select, 1=input field, 2=qth
  bool isEditing;

  // Grid mode
  char gridInput[9];
  char qthInput[41];

  // POTA mode
  char potaInput[11];
  POTAPark potaPark;          // Looked up park data
  bool potaLookupDone;
  bool potaLookupSuccess;
};

LoggerSettingsState loggerSettings;

// Field indices
#define FIELD_MODE_SELECT 0
#define FIELD_LOCATION_INPUT 1
#define FIELD_QTH 2

// ============================================
// Settings Persistence
// ============================================

void saveLoggerLocation() {
  extern Preferences qsoPrefs;

  qsoPrefs.begin("qso_operator", false);

  if (loggerSettings.inputMode == LOC_MODE_GRID) {
    // Save grid square mode
    qsoPrefs.putString("grid", loggerSettings.gridInput);
    qsoPrefs.putString("qth", loggerSettings.qthInput);
    qsoPrefs.putString("pota_ref", "");  // Clear POTA
    qsoPrefs.putString("pota_name", "");

    Serial.print("Saved grid location: ");
    Serial.print(loggerSettings.gridInput);
    Serial.print(" (");
    Serial.print(loggerSettings.qthInput);
    Serial.println(")");

  } else {
    // Save POTA mode
    if (loggerSettings.potaLookupSuccess && loggerSettings.potaPark.valid) {
      qsoPrefs.putString("pota_ref", loggerSettings.potaPark.reference);
      qsoPrefs.putString("pota_name", loggerSettings.potaPark.name);
      qsoPrefs.putString("grid", loggerSettings.potaPark.grid6);  // Use park's grid
      qsoPrefs.putString("qth", loggerSettings.potaPark.locationDesc);

      Serial.print("Saved POTA location: ");
      Serial.print(loggerSettings.potaPark.reference);
      Serial.print(" - ");
      Serial.print(loggerSettings.potaPark.name);
      Serial.print(" @ ");
      Serial.println(loggerSettings.potaPark.grid6);
    }
  }

  qsoPrefs.end();
}

void loadLoggerLocation() {
  extern Preferences qsoPrefs;

  qsoPrefs.begin("qso_operator", true);  // Read-only

  qsoPrefs.getString("grid", loggerSettings.gridInput, sizeof(loggerSettings.gridInput));
  qsoPrefs.getString("qth", loggerSettings.qthInput, sizeof(loggerSettings.qthInput));
  qsoPrefs.getString("pota_ref", loggerSettings.potaInput, sizeof(loggerSettings.potaInput));

  // If POTA ref exists, start in POTA mode
  if (strlen(loggerSettings.potaInput) > 0) {
    loggerSettings.inputMode = LOC_MODE_POTA;
  } else {
    loggerSettings.inputMode = LOC_MODE_GRID;
  }

  qsoPrefs.end();

  Serial.print("Loaded location - Grid: ");
  Serial.print(loggerSettings.gridInput);
  Serial.print(", POTA: ");
  Serial.println(loggerSettings.potaInput);
}

// ============================================
// Logger Settings UI
// ============================================

void drawLoggerSettingsUI(Adafruit_ST7789& tft) {
  tft.fillScreen(COLOR_BACKGROUND);

  // Header (drawn by menu system)

  int y = 50;
  tft.setTextSize(1);

  // Mode selection card
  uint16_t modeCardColor = (loggerSettings.currentField == FIELD_MODE_SELECT) ? 0x1082 : 0x2104;
  uint16_t modeBorderColor = (loggerSettings.currentField == FIELD_MODE_SELECT) ? ST77XX_CYAN : 0x39C7;

  tft.fillRoundRect(10, y, 300, 35, 8, modeCardColor);
  tft.drawRoundRect(10, y, 300, 35, 8, modeBorderColor);

  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(15, y + 5);
  tft.print("Location Input Mode");

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, y + 17);
  if (loggerSettings.inputMode == LOC_MODE_GRID) {
    tft.print("Grid Square");
  } else {
    tft.print("POTA Park");
  }
  tft.setTextSize(1);

  y += 45;

  // Location input card
  uint16_t inputCardColor = (loggerSettings.currentField == FIELD_LOCATION_INPUT) ? 0x1082 : 0x2104;
  uint16_t inputBorderColor = (loggerSettings.currentField == FIELD_LOCATION_INPUT) ? ST77XX_CYAN : 0x39C7;

  tft.fillRoundRect(10, y, 300, 50, 8, inputCardColor);
  tft.drawRoundRect(10, y, 300, 50, 8, inputBorderColor);

  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(15, y + 5);
  if (loggerSettings.inputMode == LOC_MODE_GRID) {
    tft.print("Grid Square (e.g., EN52wa)");
  } else {
    tft.print("POTA Ref (e.g., US-2256, K-0817)");
  }

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, y + 20);

  if (loggerSettings.inputMode == LOC_MODE_GRID) {
    tft.print(loggerSettings.gridInput);

    // Blinking cursor
    if (loggerSettings.currentField == FIELD_LOCATION_INPUT && (millis() / 500) % 2 == 0) {
      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(loggerSettings.gridInput, 15, y + 20, &x1, &y1, &w, &h);
      tft.fillRect(15 + w, y + 20, 3, 16, COLOR_WARNING);
    }
  } else {
    tft.print(loggerSettings.potaInput);

    // Blinking cursor
    if (loggerSettings.currentField == FIELD_LOCATION_INPUT && (millis() / 500) % 2 == 0) {
      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(loggerSettings.potaInput, 15, y + 20, &x1, &y1, &w, &h);
      tft.fillRect(15 + w, y + 20, 3, 16, COLOR_WARNING);
    }
  }

  tft.setTextSize(1);

  y += 60;

  // POTA lookup status or QTH input
  if (loggerSettings.inputMode == LOC_MODE_POTA) {
    // Show POTA lookup status
    if (loggerSettings.potaLookupDone) {
      if (loggerSettings.potaLookupSuccess && loggerSettings.potaPark.valid) {
        // Success - show park info
        tft.fillRoundRect(10, y, 300, 60, 8, 0x0320);  // Dark green
        tft.drawRoundRect(10, y, 300, 60, 8, ST77XX_GREEN);

        tft.setTextColor(ST77XX_GREEN);
        tft.setCursor(15, y + 5);
        tft.print("Park Found:");

        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(15, y + 18);
        tft.print(loggerSettings.potaPark.name);

        tft.setTextColor(0x7BEF);  // Gray
        tft.setCursor(15, y + 30);
        tft.print("Location: ");
        tft.print(loggerSettings.potaPark.locationDesc);

        tft.setCursor(15, y + 42);
        tft.print("Grid: ");
        tft.print(loggerSettings.potaPark.grid6);

      } else {
        // Failed lookup
        tft.fillRoundRect(10, y, 300, 40, 8, 0x2800);  // Dark red
        tft.drawRoundRect(10, y, 300, 40, 8, ST77XX_RED);

        tft.setTextColor(ST77XX_RED);
        tft.setCursor(15, y + 5);
        tft.print("Park Not Found");

        tft.setTextColor(0x7BEF);
        tft.setCursor(15, y + 18);
        tft.print("Check reference or try again");
      }
    }
  } else {
    // QTH input (grid mode only)
    uint16_t qthCardColor = (loggerSettings.currentField == FIELD_QTH) ? 0x1082 : 0x2104;
    uint16_t qthBorderColor = (loggerSettings.currentField == FIELD_QTH) ? ST77XX_CYAN : 0x39C7;

    tft.fillRoundRect(10, y, 300, 40, 8, qthCardColor);
    tft.drawRoundRect(10, y, 300, 40, 8, qthBorderColor);

    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(15, y + 5);
    tft.print("QTH (Optional)");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(15, y + 20);
    tft.print(loggerSettings.qthInput);

    // Blinking cursor
    if (loggerSettings.currentField == FIELD_QTH && (millis() / 500) % 2 == 0) {
      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(loggerSettings.qthInput, 15, y + 20, &x1, &y1, &w, &h);
      tft.fillRect(15 + w, y + 20, 3, 10, COLOR_WARNING);
    }
  }

  // Footer
  tft.fillRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, COLOR_BACKGROUND);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(10, SCREEN_HEIGHT - 16);

  if (loggerSettings.currentField == FIELD_MODE_SELECT) {
    tft.print("< > Change  TAB Next  ESC Back");
  } else if (loggerSettings.currentField == FIELD_LOCATION_INPUT) {
    if (loggerSettings.inputMode == LOC_MODE_POTA) {
      tft.print("Type & ENT Lookup  TAB Next");
    } else {
      tft.print("Type Grid  TAB Next  ESC Back");
    }
  } else if (loggerSettings.currentField == FIELD_QTH) {
    tft.print("Type QTH  ENT Save  ESC Back");
  }
}

// ============================================
// Input Handler
// ============================================

int handleLoggerSettingsInput(char key, Adafruit_ST7789& tft) {
  Serial.print("Logger Settings key: 0x");
  Serial.println(key, HEX);

  // ESC: exit without saving (except on mode select field - that saves automatically)
  if (key == 0x1B) {
    if (loggerSettings.currentField == FIELD_MODE_SELECT) {
      saveLoggerLocation();  // Save mode change
    }
    return -1;
  }

  // TAB / DOWN: next field
  if (key == '\t' || key == 0xB6) {
    if (loggerSettings.inputMode == LOC_MODE_GRID) {
      loggerSettings.currentField = (loggerSettings.currentField + 1) % 3;
    } else {
      // POTA mode: only 2 fields
      if (loggerSettings.currentField == FIELD_MODE_SELECT) {
        loggerSettings.currentField = FIELD_LOCATION_INPUT;
      } else {
        loggerSettings.currentField = FIELD_MODE_SELECT;
      }
    }
    return 2;  // Redraw
  }

  // UP: previous field
  if (key == 0xB5) {
    if (loggerSettings.inputMode == LOC_MODE_GRID) {
      loggerSettings.currentField = (loggerSettings.currentField - 1 + 3) % 3;
    } else {
      // POTA mode: toggle between 2 fields
      if (loggerSettings.currentField == FIELD_MODE_SELECT) {
        loggerSettings.currentField = FIELD_LOCATION_INPUT;
      } else {
        loggerSettings.currentField = FIELD_MODE_SELECT;
      }
    }
    return 2;  // Redraw
  }

  // Field-specific input
  if (loggerSettings.currentField == FIELD_MODE_SELECT) {
    // LEFT/RIGHT: change mode
    if (key == 0xB4 || key == 0xB7) {  // LEFT or RIGHT
      loggerSettings.inputMode = (loggerSettings.inputMode == LOC_MODE_GRID) ? LOC_MODE_POTA : LOC_MODE_GRID;
      loggerSettings.potaLookupDone = false;  // Reset POTA lookup status
      return 2;  // Redraw
    }

  } else if (loggerSettings.currentField == FIELD_LOCATION_INPUT) {
    // Location input field
    if (loggerSettings.inputMode == LOC_MODE_GRID) {
      // Grid square input
      if (isalnum(key)) {
        int len = strlen(loggerSettings.gridInput);
        if (len < 6) {  // Max 6 characters for grid
          loggerSettings.gridInput[len] = toupper(key);
          loggerSettings.gridInput[len + 1] = '\0';
          return 2;  // Redraw
        }
      } else if (key == '\b' || key == 0x7F) {  // Backspace
        int len = strlen(loggerSettings.gridInput);
        if (len > 0) {
          loggerSettings.gridInput[len - 1] = '\0';
          return 2;  // Redraw
        }
      } else if (key == '\r' || key == '\n') {  // ENTER
        // Validate and move to QTH field
        if (validateGridSquare(loggerSettings.gridInput)) {
          loggerSettings.currentField = FIELD_QTH;
          return 2;  // Redraw
        } else {
          beep(600, 100);  // Error beep
        }
      }

    } else {
      // POTA park input
      if (isalnum(key) || key == '-') {
        int len = strlen(loggerSettings.potaInput);
        if (len < 10) {  // Max 10 characters for POTA ref
          loggerSettings.potaInput[len] = toupper(key);
          loggerSettings.potaInput[len + 1] = '\0';
          return 2;  // Redraw
        }
      } else if (key == '\b' || key == 0x7F) {  // Backspace
        int len = strlen(loggerSettings.potaInput);
        if (len > 0) {
          loggerSettings.potaInput[len - 1] = '\0';
          loggerSettings.potaLookupDone = false;  // Clear lookup status
          return 2;  // Redraw
        }
      } else if (key == '\r' || key == '\n') {  // ENTER - Lookup park
        if (validatePOTAReference(loggerSettings.potaInput)) {
          Serial.println("Looking up POTA park...");
          loggerSettings.potaLookupSuccess = lookupPOTAPark(loggerSettings.potaInput, loggerSettings.potaPark);
          loggerSettings.potaLookupDone = true;

          if (loggerSettings.potaLookupSuccess) {
            beep(1000, 100);  // Success beep
            saveLoggerLocation();  // Auto-save on successful lookup
          } else {
            beep(600, 100);  // Error beep
          }

          return 2;  // Redraw
        } else {
          beep(600, 100);  // Invalid format beep
        }
      }
    }

  } else if (loggerSettings.currentField == FIELD_QTH) {
    // QTH input (grid mode only)
    if (isprint(key) && key != '\r' && key != '\n' && key != '\t') {
      int len = strlen(loggerSettings.qthInput);
      if (len < 40) {
        loggerSettings.qthInput[len] = key;
        loggerSettings.qthInput[len + 1] = '\0';
        return 2;  // Redraw
      }
    } else if (key == '\b' || key == 0x7F) {  // Backspace
      int len = strlen(loggerSettings.qthInput);
      if (len > 0) {
        loggerSettings.qthInput[len - 1] = '\0';
        return 2;  // Redraw
      }
    } else if (key == '\r' || key == '\n') {  // ENTER - Save
      saveLoggerLocation();
      beep(1000, 100);  // Success beep
      return -1;  // Exit
    }
  }

  return 0;  // No action
}

// ============================================
// Initialization
// ============================================

void startLoggerSettings(Adafruit_ST7789& tft) {
  Serial.println("Starting Logger Settings mode");

  // Initialize state
  loggerSettings.currentField = FIELD_MODE_SELECT;
  loggerSettings.isEditing = false;
  loggerSettings.potaLookupDone = false;
  loggerSettings.potaLookupSuccess = false;

  // Load saved location
  loadLoggerLocation();

  // Draw UI
  tft.fillScreen(COLOR_BACKGROUND);
  drawLoggerSettingsUI(tft);
}

#endif // QSO_LOGGER_SETTINGS_H
