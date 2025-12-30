// QSO Logger Settings Module
// Configure location (grid square or POTA park) for logging
// UI is now handled by LVGL in lv_mode_screens.h

#ifndef QSO_LOGGER_SETTINGS_H
#define QSO_LOGGER_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>
#include "../core/config.h"
#include "qso_logger.h"  // Same folder
#include "../network/pota_api.h"

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

#endif // QSO_LOGGER_SETTINGS_H
