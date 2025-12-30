/*
 * QSO Logger Validation Module
 * Input validation functions for QSO log fields
 */

#ifndef QSO_LOGGER_VALIDATION_H
#define QSO_LOGGER_VALIDATION_H

#include <Arduino.h>

// ============================================
// Validation Functions
// ============================================

/*
 * Validate callsign format
 * Basic rules: 3-10 characters, alphanumeric, must contain at least one digit
 */
bool validateCallsign(const char* callsign) {
  int len = strlen(callsign);

  // Check length
  if (len < 3 || len > 10) {
    return false;
  }

  // Check for at least one digit and all alphanumeric
  bool hasDigit = false;
  for (int i = 0; i < len; i++) {
    char c = callsign[i];
    if (isdigit(c)) {
      hasDigit = true;
    } else if (!isalpha(c) && c != '/') {  // Allow slash for portable/mobile suffixes
      return false;
    }
  }

  return hasDigit;
}

/*
 * Validate frequency in MHz
 * Range: 1.8 - 1300 MHz (covers 160m to 23cm)
 */
bool validateFrequency(float freq) {
  return (freq >= 1.8 && freq <= 1300.0);
}

/*
 * Validate RST format
 * RST: 1-5 digits, e.g., "599", "59", "339"
 */
bool validateRST(const char* rst) {
  int len = strlen(rst);

  // Check length (1-5 digits)
  if (len < 1 || len > 5) {
    return false;
  }

  // All characters must be digits
  for (int i = 0; i < len; i++) {
    if (!isdigit(rst[i])) {
      return false;
    }
  }

  // For standard RST format, validate ranges
  if (len >= 2) {
    int readability = rst[0] - '0';  // First digit: readability (1-5)
    int strength = rst[1] - '0';     // Second digit: strength (1-9)

    if (readability < 1 || readability > 5) return false;
    if (strength < 1 || strength > 9) return false;

    // Third digit (tone, CW only): 1-9
    if (len >= 3) {
      int tone = rst[2] - '0';
      if (tone < 1 || tone > 9) return false;
    }
  }

  return true;
}

/*
 * Validate grid square format
 * Format: AA##aa (e.g., FN31pr)
 * AA: Field (A-R)
 * ##: Square (0-9)
 * aa: Subsquare (a-x, optional)
 */
bool validateGridSquare(const char* grid) {
  int len = strlen(grid);

  // Empty is valid (optional field)
  if (len == 0) {
    return true;
  }

  // Check length: 4 or 6 characters
  if (len != 4 && len != 6) {
    return false;
  }

  // First two: uppercase letters A-R
  if (grid[0] < 'A' || grid[0] > 'R') return false;
  if (grid[1] < 'A' || grid[1] > 'R') return false;

  // Next two: digits 0-9
  if (!isdigit(grid[2]) || !isdigit(grid[3])) return false;

  // Optional last two: lowercase letters a-x
  if (len == 6) {
    if (grid[4] < 'a' || grid[4] > 'x') return false;
    if (grid[5] < 'a' || grid[5] > 'x') return false;
  }

  return true;
}

/*
 * Validate date format
 * Format: YYYYMMDD (e.g., 20250428)
 */
bool validateDate(const char* date) {
  int len = strlen(date);

  if (len != 8) {
    return false;
  }

  // All digits
  for (int i = 0; i < 8; i++) {
    if (!isdigit(date[i])) {
      return false;
    }
  }

  // Basic range checks
  int year = (date[0] - '0') * 1000 + (date[1] - '0') * 100 + (date[2] - '0') * 10 + (date[3] - '0');
  int month = (date[4] - '0') * 10 + (date[5] - '0');
  int day = (date[6] - '0') * 10 + (date[7] - '0');

  if (year < 2000 || year > 2100) return false;
  if (month < 1 || month > 12) return false;
  if (day < 1 || day > 31) return false;

  return true;
}

/*
 * Validate time format
 * Format: HHMM (e.g., 1430)
 */
bool validateTime(const char* time) {
  int len = strlen(time);

  if (len != 4) {
    return false;
  }

  // All digits
  for (int i = 0; i < 4; i++) {
    if (!isdigit(time[i])) {
      return false;
    }
  }

  // Range checks
  int hour = (time[0] - '0') * 10 + (time[1] - '0');
  int minute = (time[2] - '0') * 10 + (time[3] - '0');

  if (hour < 0 || hour > 23) return false;
  if (minute < 0 || minute > 59) return false;

  return true;
}

// ============================================
// Conversion Functions
// ============================================

/*
 * Convert frequency to band string
 */
String frequencyToBand(float freq) {
  if (freq >= 1.8 && freq <= 2.0) return "160m";
  if (freq >= 3.5 && freq <= 4.0) return "80m";
  if (freq >= 7.0 && freq <= 7.3) return "40m";
  if (freq >= 10.1 && freq <= 10.15) return "30m";
  if (freq >= 14.0 && freq <= 14.35) return "20m";
  if (freq >= 18.068 && freq <= 18.168) return "17m";
  if (freq >= 21.0 && freq <= 21.45) return "15m";
  if (freq >= 24.89 && freq <= 24.99) return "12m";
  if (freq >= 28.0 && freq <= 29.7) return "10m";
  if (freq >= 50.0 && freq <= 54.0) return "6m";
  if (freq >= 144.0 && freq <= 148.0) return "2m";
  if (freq >= 420.0 && freq <= 450.0) return "70cm";
  if (freq >= 1240.0 && freq <= 1300.0) return "23cm";

  return "??";  // Unknown band
}

/*
 * Get default RST for mode
 */
String getDefaultRST(const char* mode) {
  // CW modes get 599 (readability, strength, tone)
  if (strcmp(mode, "CW") == 0 || strcmp(mode, "RTTY") == 0 || strcmp(mode, "PSK31") == 0) {
    return "599";
  }

  // Phone modes get 59 (readability, strength)
  return "59";
}

/*
 * Check if mode is digital
 */
bool isDigitalMode(const char* mode) {
  return (strcmp(mode, "FT8") == 0 ||
          strcmp(mode, "FT4") == 0 ||
          strcmp(mode, "RTTY") == 0 ||
          strcmp(mode, "PSK31") == 0);
}

/*
 * Format current date/time
 * Uses NTP time if synced, otherwise falls back to millis
 * Returns formatted string "YYYYMMDD HHMM"
 */
String formatCurrentDateTime() {
  // Declare external NTP functions
  extern String getNTPDateTime();
  extern bool ntpSynced;

  // Use NTP time if available
  if (ntpSynced) {
    return getNTPDateTime();
  }

  // Fallback to millis-based time
  unsigned long ms = millis();
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = (minutes / 60) % 24;
  minutes = minutes % 60;

  char buffer[20];
  snprintf(buffer, sizeof(buffer), "20250428 %02lu%02lu", hours, minutes);
  return String(buffer);
}

/*
 * Format current date/time into separate buffers
 * dateOut: buffer for date in YYYYMMDD format (at least 9 chars)
 * timeOut: buffer for time in HHMM format (at least 5 chars)
 */
void formatCurrentDateTime(char* dateOut, char* timeOut) {
  String dt = formatCurrentDateTime();  // Get "YYYYMMDD HHMM" string

  // Parse date (first 8 chars)
  if (dateOut) {
    strncpy(dateOut, dt.c_str(), 8);
    dateOut[8] = '\0';
  }

  // Parse time (chars 9-12, after space)
  if (timeOut) {
    if (dt.length() >= 13) {
      strncpy(timeOut, dt.c_str() + 9, 4);
      timeOut[4] = '\0';
    } else {
      timeOut[0] = '\0';
    }
  }
}

/*
 * Convert string to uppercase
 */
void toUpperCase(char* str) {
  for (int i = 0; str[i]; i++) {
    str[i] = toupper(str[i]);
  }
}

#endif // QSO_LOGGER_VALIDATION_H
