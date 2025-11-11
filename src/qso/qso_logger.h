/*
 * QSO Logger Module
 * Core data structures and state management for ham radio contact logging
 */

#ifndef QSO_LOGGER_H
#define QSO_LOGGER_H

#include <Arduino.h>
#include <Preferences.h>

// ============================================
// QSO Data Structure
// ============================================

struct QSO {
  unsigned long id;           // Unix timestamp as unique ID
  char callsign[11];          // Callsign (max 10 chars + null)
  float frequency;            // Frequency in MHz
  char mode[8];               // Mode (CW, SSB, FM, etc.)
  char band[6];               // Band (e.g., "20m", "40m")
  char rst_sent[4];           // RST sent (e.g., "599")
  char rst_rcvd[4];           // RST received
  char date[9];               // Date YYYYMMDD
  char time_on[5];            // Time HHMM (UTC)
  char time_off[5];           // Time HHMM (UTC, optional)
  char name[21];              // Operator name (optional)
  char qth[41];               // Location (optional)
  int power;                  // Power in watts (0 = not specified)
  char gridsquare[9];         // Maidenhead grid (optional)
  char country[31];           // Country (optional)
  char state[3];              // State/Province (optional)
  char iota[11];              // IOTA reference (optional)
  char notes[61];             // Notes (optional)
  char contest[31];           // Contest name (optional)
  int srx;                    // Serial RX (contest)
  int stx;                    // Serial TX (contest)
  char operator_call[11];     // Device operator callsign
  char station_call[11];      // Station callsign (same unless guest op)

  // Operator location (from logger settings, can override per-QSO)
  char my_gridsquare[9];      // My grid square
  char my_pota_ref[11];       // My POTA park reference

  // Contact's POTA info
  char their_pota_ref[11];    // Their POTA park reference (if activating)
};

// ============================================
// Log Entry Form State
// ============================================

struct LogEntryState {
  int currentField;           // Current field being edited (0-10)
  char callsign[11];
  char frequency[10];         // String for editing (converted to float on save)
  int modeIndex;              // Index into modes array
  char rstSent[4];
  char rstRcvd[4];
  char date[9];
  char time[5];
  char notes[61];

  // Location fields
  char myGrid[9];             // My grid square (from logger settings)
  char myPOTA[11];            // My POTA park (from logger settings)
  char theirGrid[9];          // Their grid square
  char theirPOTA[11];         // Their POTA park

  bool isEditing;             // Currently editing a field
};

// ============================================
// Mode Options
// ============================================

#define NUM_MODES 8
const char* QSO_MODES[NUM_MODES] = {
  "CW",
  "SSB",
  "FM",
  "AM",
  "FT8",
  "FT4",
  "RTTY",
  "PSK31"
};

// ============================================
// Log Entry Form Fields
// ============================================

enum LogEntryField {
  FIELD_CALLSIGN = 0,
  FIELD_FREQUENCY = 1,
  FIELD_MODE = 2,
  FIELD_RST_SENT = 3,
  FIELD_RST_RCVD = 4,
  FIELD_DATE_TIME = 5,
  FIELD_MY_GRID = 6,
  FIELD_MY_POTA = 7,
  FIELD_THEIR_GRID = 8,
  FIELD_THEIR_POTA = 9,
  FIELD_NOTES = 10,
  FIELD_COUNT = 11
};

// ============================================
// Global State
// ============================================

LogEntryState logEntryState = {
  .currentField = 0,
  .callsign = "",
  .frequency = "14.025",  // Default to 20m CW
  .modeIndex = 0,         // Default to CW
  .rstSent = "599",
  .rstRcvd = "599",
  .date = "",
  .time = "",
  .notes = "",
  .myGrid = "",
  .myPOTA = "",
  .theirGrid = "",
  .theirPOTA = "",
  .isEditing = false
};

// Operator settings
Preferences qsoPrefs;
char operatorCallsign[11] = "NOCALL";
char operatorName[21] = "";
char operatorQTH[41] = "";
char operatorGrid[9] = "";

// ============================================
// Forward Declarations
// ============================================

void loadOperatorSettings();
void saveOperatorSettings();
void initLogEntry();
void clearLogEntryForm();
String getCurrentDateTime();
String getFieldLabel(int field);

// ============================================
// Initialization Functions
// ============================================

/*
 * Load operator settings from Preferences
 * Uses vailCallsign from main settings if QSO operator callsign is not set
 */
void loadOperatorSettings() {
  // Declare external variable
  extern String vailCallsign;

  qsoPrefs.begin("qso_operator", false);

  qsoPrefs.getString("callsign", operatorCallsign, sizeof(operatorCallsign));
  qsoPrefs.getString("name", operatorName, sizeof(operatorName));
  qsoPrefs.getString("qth", operatorQTH, sizeof(operatorQTH));
  qsoPrefs.getString("grid", operatorGrid, sizeof(operatorGrid));

  qsoPrefs.end();

  // If no callsign set, use the main vailCallsign from general settings
  if (strlen(operatorCallsign) == 0 || strcmp(operatorCallsign, "NOCALL") == 0) {
    vailCallsign.toCharArray(operatorCallsign, sizeof(operatorCallsign));
    Serial.println("Using main callsign for QSO logger");
  }

  Serial.println("Loaded operator settings:");
  Serial.print("  Callsign: ");
  Serial.println(operatorCallsign);
}

/*
 * Save operator settings to Preferences
 */
void saveOperatorSettings() {
  qsoPrefs.begin("qso_operator", false);

  qsoPrefs.putString("callsign", operatorCallsign);
  qsoPrefs.putString("name", operatorName);
  qsoPrefs.putString("qth", operatorQTH);
  qsoPrefs.putString("grid", operatorGrid);

  qsoPrefs.end();

  Serial.println("Saved operator settings");
}

/*
 * Initialize log entry form with defaults
 */
void initLogEntry() {
  clearLogEntryForm();

  // Set default values
  strcpy(logEntryState.frequency, "14.025");  // 20m CW
  logEntryState.modeIndex = 0;  // CW
  strcpy(logEntryState.rstSent, "599");
  strcpy(logEntryState.rstRcvd, "599");

  // Get current date/time (uses NTP if available)
  String dateTime = formatCurrentDateTime();
  // Parse YYYYMMDD HHMM format
  dateTime.substring(0, 8).toCharArray(logEntryState.date, 9);
  dateTime.substring(9, 13).toCharArray(logEntryState.time, 5);

  Serial.print("Auto-filled date/time: ");
  Serial.println(dateTime);

  // Auto-fill operator location from logger settings
  qsoPrefs.begin("qso_operator", true);  // Read-only

  // Debug: Check what Preferences returns
  String gridStr = qsoPrefs.getString("grid", "");
  String potaStr = qsoPrefs.getString("pota_ref", "");
  qsoPrefs.end();

  Serial.print("Prefs grid string: [");
  Serial.print(gridStr);
  Serial.print("] len=");
  Serial.println(gridStr.length());
  Serial.print("Prefs pota string: [");
  Serial.print(potaStr);
  Serial.print("] len=");
  Serial.println(potaStr.length());

  // Copy to char arrays safely
  if (gridStr.length() > 0) {
    gridStr.toCharArray(logEntryState.myGrid, sizeof(logEntryState.myGrid));
  }
  if (potaStr.length() > 0) {
    potaStr.toCharArray(logEntryState.myPOTA, sizeof(logEntryState.myPOTA));
  }

  Serial.print("Auto-filled my grid: [");
  Serial.print(logEntryState.myGrid);
  Serial.print("] len=");
  Serial.println(strlen(logEntryState.myGrid));
  Serial.print("Auto-filled my POTA: [");
  Serial.print(logEntryState.myPOTA);
  Serial.print("] len=");
  Serial.println(strlen(logEntryState.myPOTA));

  logEntryState.currentField = 0;
  logEntryState.isEditing = false;

  Serial.println("Initialized log entry form");
}

/*
 * Clear all form fields
 */
void clearLogEntryForm() {
  memset(logEntryState.callsign, 0, sizeof(logEntryState.callsign));
  memset(logEntryState.frequency, 0, sizeof(logEntryState.frequency));
  memset(logEntryState.rstSent, 0, sizeof(logEntryState.rstSent));
  memset(logEntryState.rstRcvd, 0, sizeof(logEntryState.rstRcvd));
  memset(logEntryState.date, 0, sizeof(logEntryState.date));
  memset(logEntryState.time, 0, sizeof(logEntryState.time));
  memset(logEntryState.notes, 0, sizeof(logEntryState.notes));
  memset(logEntryState.myGrid, 0, sizeof(logEntryState.myGrid));
  memset(logEntryState.myPOTA, 0, sizeof(logEntryState.myPOTA));
  memset(logEntryState.theirGrid, 0, sizeof(logEntryState.theirGrid));
  memset(logEntryState.theirPOTA, 0, sizeof(logEntryState.theirPOTA));
}

/*
 * Get current date/time as string (YYYYMMDD HHMM)
 * Uses millis() approximation until real-time clock available
 */
String getCurrentDateTime() {
  // TODO: Implement real date/time from RTC or NTP
  // For now, return a placeholder
  unsigned long ms = millis();
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = (minutes / 60) % 24;
  minutes = minutes % 60;

  // Fake date: 20250428 (April 28, 2025)
  // Real time from millis
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "20250428 %02lu%02lu", hours, minutes);
  return String(buffer);
}

/*
 * Get label for field
 */
String getFieldLabel(int field) {
  switch (field) {
    case FIELD_CALLSIGN: return "Callsign";
    case FIELD_FREQUENCY: return "Frequency (MHz)";
    case FIELD_MODE: return "Mode";
    case FIELD_RST_SENT: return "RST Sent";
    case FIELD_RST_RCVD: return "RST Rcvd";
    case FIELD_DATE_TIME: return "Date/Time (UTC)";
    case FIELD_MY_GRID: return "My Grid";
    case FIELD_MY_POTA: return "My POTA";
    case FIELD_THEIR_GRID: return "Their Grid";
    case FIELD_THEIR_POTA: return "Their POTA";
    case FIELD_NOTES: return "Notes";
    default: return "Unknown";
  }
}

#endif // QSO_LOGGER_H
