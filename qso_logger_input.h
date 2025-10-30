/*
 * QSO Logger Input Handler
 * Handles keyboard input for log entry form
 */

#ifndef QSO_LOGGER_INPUT_H
#define QSO_LOGGER_INPUT_H

#include <Adafruit_ST7789.h>
#include "config.h"
#include "i2s_audio.h"
#include "qso_logger.h"
#include "qso_logger_validation.h"
#include "qso_logger_storage.h"

/*
 * Handle input for log entry form
 * Returns: -1 to exit, 0 for normal input, 2 for redraw
 */
int handleQSOLogEntryInput(char key, Adafruit_ST7789& tft) {
  int currentField = logEntryState.currentField;

  // Debug output
  Serial.print("Key pressed: 0x");
  Serial.print(key, HEX);
  Serial.print(" (");
  Serial.print((int)key);
  Serial.println(")");

  // Handle ESC - back to menu
  if (key == KEY_ESC) {
    return -1;
  }

  // Handle TAB - next field
  if (key == KEY_TAB || key == KEY_DOWN) {
    logEntryState.currentField = (logEntryState.currentField + 1) % 11;
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 2;  // Redraw
  }

  // Handle UP - previous field
  if (key == KEY_UP) {
    logEntryState.currentField = (logEntryState.currentField + 10) % 11;  // -1 mod 11
    beep(TONE_MENU_NAV, BEEP_SHORT);
    return 2;  // Redraw
  }

  // Handle ENTER - save if on last field or all required fields filled
  if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    Serial.println("ENTER pressed - attempting to save...");

    // Validate required fields
    Serial.print("Validating callsign: ");
    Serial.println(logEntryState.callsign);

    if (strlen(logEntryState.callsign) == 0) {
      beep(TONE_ERROR, BEEP_MEDIUM);
      Serial.println("ERROR: Callsign required");
      return 0;
    }

    if (!validateCallsign(logEntryState.callsign)) {
      beep(TONE_ERROR, BEEP_MEDIUM);
      Serial.println("ERROR: Invalid callsign format");
      return 0;
    }

    float freq = atof(logEntryState.frequency);
    if (!validateFrequency(freq)) {
      beep(TONE_ERROR, BEEP_MEDIUM);
      Serial.println("ERROR: Invalid frequency");
      return 0;
    }

    // Create QSO struct
    QSO qso;
    memset(&qso, 0, sizeof(QSO));

    qso.id = millis();  // Use millis as unique ID
    strcpy(qso.callsign, logEntryState.callsign);
    qso.frequency = freq;
    strcpy(qso.mode, QSO_MODES[logEntryState.modeIndex]);
    strcpy(qso.band, frequencyToBand(freq).c_str());
    strcpy(qso.rst_sent, logEntryState.rstSent);
    strcpy(qso.rst_rcvd, logEntryState.rstRcvd);
    strcpy(qso.date, logEntryState.date);
    strcpy(qso.time_on, logEntryState.time);
    strcpy(qso.time_off, logEntryState.time);  // Same as time_on for now
    strcpy(qso.notes, logEntryState.notes);
    strcpy(qso.operator_call, operatorCallsign);
    strcpy(qso.station_call, operatorCallsign);

    // Location fields
    strcpy(qso.my_gridsquare, logEntryState.myGrid);
    strcpy(qso.my_pota_ref, logEntryState.myPOTA);
    strcpy(qso.gridsquare, logEntryState.theirGrid);  // Their grid
    strcpy(qso.their_pota_ref, logEntryState.theirPOTA);

    // Debug: Print what we're about to save
    Serial.println("=== Saving QSO Debug ===");
    Serial.print("my_gridsquare: [");
    Serial.print(qso.my_gridsquare);
    Serial.print("] len=");
    Serial.println(strlen(qso.my_gridsquare));
    Serial.print("my_pota_ref: [");
    Serial.print(qso.my_pota_ref);
    Serial.print("] len=");
    Serial.println(strlen(qso.my_pota_ref));
    Serial.print("gridsquare (their): [");
    Serial.print(qso.gridsquare);
    Serial.print("] len=");
    Serial.println(strlen(qso.gridsquare));
    Serial.print("their_pota_ref: [");
    Serial.print(qso.their_pota_ref);
    Serial.print("] len=");
    Serial.println(strlen(qso.their_pota_ref));

    // Save to storage
    if (saveQSO(qso)) {
      beep(TONE_SELECT, BEEP_LONG);
      Serial.println("QSO saved successfully!");

      // Show success message
      tft.fillScreen(COLOR_BACKGROUND);
      tft.setTextSize(2);
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(60, 100);
      tft.print("QSO SAVED!");
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(40, 130);
      tft.print("Total logs: ");
      tft.print(getTotalLogs());
      delay(1500);

      // Clear form and reset for next entry
      initLogEntry();
      return 2;  // Redraw
    } else {
      beep(TONE_ERROR, BEEP_LONG);
      Serial.println("ERROR: Failed to save QSO");
      return 0;
    }
  }

  // Field-specific input handling
  switch (currentField) {
    case FIELD_CALLSIGN: {
      // Alphanumeric input only
      if (isalnum(key) || key == '/') {
        int len = strlen(logEntryState.callsign);
        if (len < 10) {
          logEntryState.callsign[len] = toupper(key);
          logEntryState.callsign[len + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_BACKSPACE) {
        int len = strlen(logEntryState.callsign);
        if (len > 0) {
          logEntryState.callsign[len - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      }
      break;
    }

    case FIELD_FREQUENCY: {
      // Numeric input with decimal point
      if (isdigit(key) || key == '.') {
        int len = strlen(logEntryState.frequency);
        if (len < 9) {
          logEntryState.frequency[len] = key;
          logEntryState.frequency[len + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_BACKSPACE) {
        int len = strlen(logEntryState.frequency);
        if (len > 0) {
          logEntryState.frequency[len - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      }
      break;
    }

    case FIELD_MODE: {
      // Cycle through modes with UP/DOWN (already handled above)
      // Or use LEFT/RIGHT
      if (key == KEY_LEFT) {
        logEntryState.modeIndex = (logEntryState.modeIndex + NUM_MODES - 1) % NUM_MODES;
        // Update default RST for new mode
        String defaultRST = getDefaultRST(QSO_MODES[logEntryState.modeIndex]);
        strcpy(logEntryState.rstSent, defaultRST.c_str());
        strcpy(logEntryState.rstRcvd, defaultRST.c_str());
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;
      } else if (key == KEY_RIGHT) {
        logEntryState.modeIndex = (logEntryState.modeIndex + 1) % NUM_MODES;
        // Update default RST for new mode
        String defaultRST = getDefaultRST(QSO_MODES[logEntryState.modeIndex]);
        strcpy(logEntryState.rstSent, defaultRST.c_str());
        strcpy(logEntryState.rstRcvd, defaultRST.c_str());
        beep(TONE_MENU_NAV, BEEP_SHORT);
        return 2;
      }
      break;
    }

    case FIELD_RST_SENT: {
      // Numeric input only (RST format)
      if (isdigit(key)) {
        int len = strlen(logEntryState.rstSent);
        if (len < 3) {
          logEntryState.rstSent[len] = key;
          logEntryState.rstSent[len + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_BACKSPACE) {
        int len = strlen(logEntryState.rstSent);
        if (len > 0) {
          logEntryState.rstSent[len - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      }
      break;
    }

    case FIELD_RST_RCVD: {
      // Numeric input only (RST format)
      if (isdigit(key)) {
        int len = strlen(logEntryState.rstRcvd);
        if (len < 3) {
          logEntryState.rstRcvd[len] = key;
          logEntryState.rstRcvd[len + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_BACKSPACE) {
        int len = strlen(logEntryState.rstRcvd);
        if (len > 0) {
          logEntryState.rstRcvd[len - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      }
      break;
    }

    case FIELD_DATE_TIME: {
      // Combined date/time editing
      // Format: YYYYMMDD HHMM (13 chars total with space)
      // We'll edit both date and time as one field

      if (isdigit(key)) {
        // Count current digits (skip space at position 8)
        int dateLen = strlen(logEntryState.date);
        int timeLen = strlen(logEntryState.time);

        // Fill date first (8 digits), then time (4 digits)
        if (dateLen < 8) {
          logEntryState.date[dateLen] = key;
          logEntryState.date[dateLen + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        } else if (timeLen < 4) {
          logEntryState.time[timeLen] = key;
          logEntryState.time[timeLen + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_BACKSPACE) {
        int timeLen = strlen(logEntryState.time);
        int dateLen = strlen(logEntryState.date);

        // Delete from time first, then date
        if (timeLen > 0) {
          logEntryState.time[timeLen - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        } else if (dateLen > 0) {
          logEntryState.date[dateLen - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == 'n' || key == 'N') {
        // Press 'N' for "now" - auto-fill current time
        String dateTime = formatCurrentDateTime();
        dateTime.substring(0, 8).toCharArray(logEntryState.date, 9);
        dateTime.substring(9, 13).toCharArray(logEntryState.time, 5);
        beep(TONE_SELECT, BEEP_SHORT);
        return 2;
      }
      break;
    }

    case FIELD_MY_GRID: {
      // Grid square input (alphanumeric, uppercase, max 6 chars)
      if (isalnum(key)) {
        int len = strlen(logEntryState.myGrid);
        if (len < 6) {
          logEntryState.myGrid[len] = toupper(key);
          logEntryState.myGrid[len + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_BACKSPACE) {
        int len = strlen(logEntryState.myGrid);
        if (len > 0) {
          logEntryState.myGrid[len - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      }
      break;
    }

    case FIELD_MY_POTA: {
      // POTA reference input (alphanumeric + dash, max 10 chars)
      if (isalnum(key) || key == '-') {
        int len = strlen(logEntryState.myPOTA);
        if (len < 10) {
          logEntryState.myPOTA[len] = toupper(key);
          logEntryState.myPOTA[len + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_BACKSPACE) {
        int len = strlen(logEntryState.myPOTA);
        if (len > 0) {
          logEntryState.myPOTA[len - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      }
      break;
    }

    case FIELD_THEIR_GRID: {
      // Their grid square input (alphanumeric, uppercase, max 6 chars)
      if (isalnum(key)) {
        int len = strlen(logEntryState.theirGrid);
        if (len < 6) {
          logEntryState.theirGrid[len] = toupper(key);
          logEntryState.theirGrid[len + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_BACKSPACE) {
        int len = strlen(logEntryState.theirGrid);
        if (len > 0) {
          logEntryState.theirGrid[len - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      }
      break;
    }

    case FIELD_THEIR_POTA: {
      // Their POTA reference input (alphanumeric + dash, max 10 chars)
      if (isalnum(key) || key == '-') {
        int len = strlen(logEntryState.theirPOTA);
        if (len < 10) {
          logEntryState.theirPOTA[len] = toupper(key);
          logEntryState.theirPOTA[len + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_BACKSPACE) {
        int len = strlen(logEntryState.theirPOTA);
        if (len > 0) {
          logEntryState.theirPOTA[len - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_RIGHT || key == 0xB7) { // RIGHT arrow - Lookup POTA park
        if (strlen(logEntryState.theirPOTA) > 0 && validatePOTAReference(logEntryState.theirPOTA)) {
          Serial.print("Looking up POTA park: ");
          Serial.println(logEntryState.theirPOTA);

          POTAPark park;
          if (lookupPOTAPark(logEntryState.theirPOTA, park) && park.valid) {
            // Success - auto-fill their grid from park data
            strcpy(logEntryState.theirGrid, park.grid6);
            beep(1000, 100); // Success beep

            // Show brief confirmation
            tft.fillRect(10, 200, 300, 20, 0x0320); // Dark green bar
            tft.setTextSize(1);
            tft.setTextColor(ST77XX_WHITE);
            tft.setCursor(15, 207);
            tft.print("Found: ");
            tft.print(park.name);
            delay(1000);

            Serial.print("Auto-filled their grid: ");
            Serial.println(logEntryState.theirGrid);

            return 2; // Redraw
          } else {
            // Failed lookup
            beep(600, 100); // Error beep
            Serial.println("POTA lookup failed");

            // Show brief error
            tft.fillRect(10, 200, 300, 20, 0x2800); // Dark red bar
            tft.setTextSize(1);
            tft.setTextColor(ST77XX_WHITE);
            tft.setCursor(15, 207);
            tft.print("Park not found");
            delay(1000);

            return 2; // Redraw
          }
        } else {
          beep(600, 50); // Invalid format beep
        }
      }
      break;
    }

    case FIELD_NOTES: {
      // Free text input
      if (isprint(key) && key != KEY_TAB && key != KEY_ESC) {
        int len = strlen(logEntryState.notes);
        if (len < 60) {
          logEntryState.notes[len] = key;
          logEntryState.notes[len + 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      } else if (key == KEY_BACKSPACE) {
        int len = strlen(logEntryState.notes);
        if (len > 0) {
          logEntryState.notes[len - 1] = '\0';
          beep(TONE_MENU_NAV, 20);
          return 2;
        }
      }
      break;
    }
  }

  return 0;  // No redraw needed
}

#endif // QSO_LOGGER_INPUT_H
