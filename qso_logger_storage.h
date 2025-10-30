/*
 * QSO Logger Storage Module
 * LittleFS-based storage for contact logs with JSON serialization
 */

#ifndef QSO_LOGGER_STORAGE_H
#define QSO_LOGGER_STORAGE_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "qso_logger.h"

// Use SPIFFS instead of LittleFS for better compatibility
#define FileSystem SPIFFS

// ============================================
// Storage Configuration
// ============================================

#define MAX_LOGS 500                    // Maximum logs before circular buffer deletion
#define LOGS_DIR "/logs"                // Log files directory
#define METADATA_FILE "/logs/metadata.json"  // Statistics cache

// ============================================
// Storage Statistics
// ============================================

struct StorageStats {
  int totalLogs;
  int logsByBand[10];  // 160m, 80m, 40m, 30m, 20m, 17m, 15m, 12m, 10m, 6m
  int logsByMode[8];   // CW, SSB, FM, AM, FT8, FT4, RTTY, PSK31
  unsigned long oldestLogId;
  unsigned long newestLogId;
};

StorageStats storageStats = {0};

// ============================================
// Forward Declarations
// ============================================

void loadMetadata();
void saveMetadata();

// ============================================
// Helper Functions
// ============================================

/*
 * Initialize LittleFS filesystem
 */
bool initStorage() {
  Serial.println("Initializing SPIFFS...");

  // Try to mount first
  if (!FileSystem.begin(false)) {
    Serial.println("SPIFFS mount failed, trying to format...");

    // Format and try again
    if (!FileSystem.begin(true)) {
      Serial.println("ERROR: SPIFFS format and mount failed!");
      Serial.println("This may be a partition table issue.");
      Serial.println("Logger will run without storage.");
      return false;
    }
  }

  Serial.println("SPIFFS mounted successfully");

  // Create logs directory if it doesn't exist
  if (!FileSystem.exists(LOGS_DIR)) {
    Serial.println("Creating /logs directory...");
    FileSystem.mkdir(LOGS_DIR);
  }

  // Load metadata
  loadMetadata();

  // Print storage info
  Serial.print("Total logs: ");
  Serial.println(storageStats.totalLogs);
  Serial.print("Used: ");
  Serial.print(FileSystem.usedBytes());
  Serial.print(" / ");
  Serial.print(FileSystem.totalBytes());
  Serial.println(" bytes");

  return true;
}

/*
 * Get band index from band string
 */
int getBandIndex(const char* band) {
  if (strcmp(band, "160m") == 0) return 0;
  if (strcmp(band, "80m") == 0) return 1;
  if (strcmp(band, "40m") == 0) return 2;
  if (strcmp(band, "30m") == 0) return 3;
  if (strcmp(band, "20m") == 0) return 4;
  if (strcmp(band, "17m") == 0) return 5;
  if (strcmp(band, "15m") == 0) return 6;
  if (strcmp(band, "12m") == 0) return 7;
  if (strcmp(band, "10m") == 0) return 8;
  if (strcmp(band, "6m") == 0) return 9;
  return -1;  // Unknown band
}

/*
 * Get mode index from mode string
 */
int getModeIndex(const char* mode) {
  for (int i = 0; i < NUM_MODES; i++) {
    if (strcmp(mode, QSO_MODES[i]) == 0) {
      return i;
    }
  }
  return -1;  // Unknown mode
}

// ============================================
// Metadata Management
// ============================================

/*
 * Load metadata from file
 */
void loadMetadata() {
  memset(&storageStats, 0, sizeof(StorageStats));

  if (!FileSystem.exists(METADATA_FILE)) {
    Serial.println("No metadata file found, starting fresh");
    return;
  }

  File file = FileSystem.open(METADATA_FILE, "r");
  if (!file) {
    Serial.println("Failed to open metadata file");
    return;
  }

  // Parse JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("Failed to parse metadata: ");
    Serial.println(error.c_str());
    return;
  }

  // Load statistics
  storageStats.totalLogs = doc["totalLogs"] | 0;
  storageStats.oldestLogId = doc["oldestLogId"] | 0;
  storageStats.newestLogId = doc["newestLogId"] | 0;

  JsonArray bands = doc["logsByBand"];
  for (int i = 0; i < 10 && i < bands.size(); i++) {
    storageStats.logsByBand[i] = bands[i];
  }

  JsonArray modes = doc["logsByMode"];
  for (int i = 0; i < 8 && i < modes.size(); i++) {
    storageStats.logsByMode[i] = modes[i];
  }

  Serial.println("Metadata loaded successfully");
}

/*
 * Save metadata to file
 */
void saveMetadata() {
  // Create JSON document
  JsonDocument doc;

  doc["totalLogs"] = storageStats.totalLogs;
  doc["oldestLogId"] = storageStats.oldestLogId;
  doc["newestLogId"] = storageStats.newestLogId;

  JsonArray bands = doc["logsByBand"].to<JsonArray>();
  for (int i = 0; i < 10; i++) {
    bands.add(storageStats.logsByBand[i]);
  }

  JsonArray modes = doc["logsByMode"].to<JsonArray>();
  for (int i = 0; i < 8; i++) {
    modes.add(storageStats.logsByMode[i]);
  }

  // Write to file
  File file = FileSystem.open(METADATA_FILE, "w");
  if (!file) {
    Serial.println("Failed to open metadata file for writing");
    return;
  }

  serializeJson(doc, file);
  file.close();

  Serial.println("Metadata saved");
}

// ============================================
// QSO Serialization
// ============================================

/*
 * Convert QSO struct to JSON object
 */
void qsoToJson(const QSO& qso, JsonObject& obj) {
  obj["id"] = qso.id;
  obj["callsign"] = qso.callsign;
  obj["frequency"] = qso.frequency;
  obj["mode"] = qso.mode;
  obj["band"] = qso.band;
  obj["rst_sent"] = qso.rst_sent;
  obj["rst_rcvd"] = qso.rst_rcvd;
  obj["date"] = qso.date;
  obj["time_on"] = qso.time_on;

  if (strlen(qso.time_off) > 0) obj["time_off"] = qso.time_off;
  if (strlen(qso.name) > 0) obj["name"] = qso.name;
  if (strlen(qso.qth) > 0) obj["qth"] = qso.qth;
  if (qso.power > 0) obj["power"] = qso.power;
  if (strlen(qso.gridsquare) > 0) obj["gridsquare"] = qso.gridsquare;
  if (strlen(qso.country) > 0) obj["country"] = qso.country;
  if (strlen(qso.state) > 0) obj["state"] = qso.state;
  if (strlen(qso.iota) > 0) obj["iota"] = qso.iota;
  if (strlen(qso.notes) > 0) obj["notes"] = qso.notes;
  if (strlen(qso.contest) > 0) obj["contest"] = qso.contest;
  if (qso.srx > 0) obj["srx"] = qso.srx;
  if (qso.stx > 0) obj["stx"] = qso.stx;
  if (strlen(qso.operator_call) > 0) obj["operator_call"] = qso.operator_call;
  if (strlen(qso.station_call) > 0) obj["station_call"] = qso.station_call;

  // Location fields
  Serial.println("=== qsoToJson Location Fields ===");
  Serial.print("my_gridsquare: [");
  Serial.print(qso.my_gridsquare);
  Serial.print("] len=");
  Serial.println(strlen(qso.my_gridsquare));

  Serial.print("my_pota_ref: [");
  Serial.print(qso.my_pota_ref);
  Serial.print("] len=");
  Serial.println(strlen(qso.my_pota_ref));

  Serial.print("their_pota_ref: [");
  Serial.print(qso.their_pota_ref);
  Serial.print("] len=");
  Serial.println(strlen(qso.their_pota_ref));

  if (strlen(qso.my_gridsquare) > 0) {
    Serial.println("Adding my_gridsquare to JSON");
    obj["my_gridsquare"] = qso.my_gridsquare;
  }
  if (strlen(qso.my_pota_ref) > 0) {
    Serial.println("Adding my_pota_ref to JSON");
    obj["my_pota_ref"] = qso.my_pota_ref;
  }
  if (strlen(qso.their_pota_ref) > 0) {
    Serial.println("Adding their_pota_ref to JSON");
    obj["their_pota_ref"] = qso.their_pota_ref;
  }
}

/*
 * Convert JSON object to QSO struct
 */
void jsonToQso(JsonObject& obj, QSO& qso) {
  memset(&qso, 0, sizeof(QSO));

  qso.id = obj["id"] | 0;
  strlcpy(qso.callsign, obj["callsign"] | "", sizeof(qso.callsign));
  qso.frequency = obj["frequency"] | 0.0f;
  strlcpy(qso.mode, obj["mode"] | "", sizeof(qso.mode));
  strlcpy(qso.band, obj["band"] | "", sizeof(qso.band));
  strlcpy(qso.rst_sent, obj["rst_sent"] | "", sizeof(qso.rst_sent));
  strlcpy(qso.rst_rcvd, obj["rst_rcvd"] | "", sizeof(qso.rst_rcvd));
  strlcpy(qso.date, obj["date"] | "", sizeof(qso.date));
  strlcpy(qso.time_on, obj["time_on"] | "", sizeof(qso.time_on));
  strlcpy(qso.time_off, obj["time_off"] | "", sizeof(qso.time_off));
  strlcpy(qso.name, obj["name"] | "", sizeof(qso.name));
  strlcpy(qso.qth, obj["qth"] | "", sizeof(qso.qth));
  qso.power = obj["power"] | 0;
  strlcpy(qso.gridsquare, obj["gridsquare"] | "", sizeof(qso.gridsquare));
  strlcpy(qso.country, obj["country"] | "", sizeof(qso.country));
  strlcpy(qso.state, obj["state"] | "", sizeof(qso.state));
  strlcpy(qso.iota, obj["iota"] | "", sizeof(qso.iota));
  strlcpy(qso.notes, obj["notes"] | "", sizeof(qso.notes));
  strlcpy(qso.contest, obj["contest"] | "", sizeof(qso.contest));
  qso.srx = obj["srx"] | 0;
  qso.stx = obj["stx"] | 0;
  strlcpy(qso.operator_call, obj["operator_call"] | "", sizeof(qso.operator_call));
  strlcpy(qso.station_call, obj["station_call"] | "", sizeof(qso.station_call));

  // Location fields (ensure null termination even if not present in JSON)
  // Use memset to completely zero out the arrays
  memset(qso.my_gridsquare, 0, sizeof(qso.my_gridsquare));
  memset(qso.my_pota_ref, 0, sizeof(qso.my_pota_ref));
  memset(qso.their_pota_ref, 0, sizeof(qso.their_pota_ref));

  Serial.println("=== jsonToQso Loading Location Fields ===");
  Serial.print("JSON has my_gridsquare key: ");
  Serial.println(obj.containsKey("my_gridsquare") ? "YES" : "NO");
  if (obj.containsKey("my_gridsquare")) {
    const char* val = obj["my_gridsquare"];
    Serial.print("  Value from JSON: [");
    Serial.print(val ? val : "NULL");
    Serial.println("]");
    if (val != nullptr && strlen(val) > 0) {
      strlcpy(qso.my_gridsquare, val, sizeof(qso.my_gridsquare));
      Serial.print("  Copied to struct: [");
      Serial.print(qso.my_gridsquare);
      Serial.println("]");
    }
  }

  Serial.print("JSON has my_pota_ref key: ");
  Serial.println(obj.containsKey("my_pota_ref") ? "YES" : "NO");
  if (obj.containsKey("my_pota_ref")) {
    const char* val = obj["my_pota_ref"];
    Serial.print("  Value from JSON: [");
    Serial.print(val ? val : "NULL");
    Serial.println("]");
    if (val != nullptr && strlen(val) > 0) {
      strlcpy(qso.my_pota_ref, val, sizeof(qso.my_pota_ref));
      Serial.print("  Copied to struct: [");
      Serial.print(qso.my_pota_ref);
      Serial.println("]");
    }
  }

  Serial.print("JSON has their_pota_ref key: ");
  Serial.println(obj.containsKey("their_pota_ref") ? "YES" : "NO");
  if (obj.containsKey("their_pota_ref")) {
    const char* val = obj["their_pota_ref"];
    Serial.print("  Value from JSON: [");
    Serial.print(val ? val : "NULL");
    Serial.println("]");
    if (val != nullptr && strlen(val) > 0) {
      strlcpy(qso.their_pota_ref, val, sizeof(qso.their_pota_ref));
      Serial.print("  Copied to struct: [");
      Serial.print(qso.their_pota_ref);
      Serial.println("]");
    }
  }
}

// ============================================
// QSO Storage Operations
// ============================================

/*
 * Get filename for a QSO log (based on date)
 */
String getLogFilename(const char* date) {
  // Format: /logs/qso_YYYYMMDD.json
  char filename[30];
  snprintf(filename, sizeof(filename), "%s/qso_%s.json", LOGS_DIR, date);
  return String(filename);
}

/*
 * Save a QSO to storage
 */
bool saveQSO(const QSO& qso) {
  Serial.print("Saving QSO: ");
  Serial.println(qso.callsign);

  // Get filename based on date
  String filename = getLogFilename(qso.date);
  Serial.print("Filename: ");
  Serial.println(filename);
  Serial.print("Date string: ");
  Serial.println(qso.date);

  // Load existing logs for this day (if any)
  JsonDocument doc;
  JsonArray logs;

  if (FileSystem.exists(filename)) {
    File file = FileSystem.open(filename, "r");
    if (file) {
      DeserializationError error = deserializeJson(doc, file);
      file.close();
      if (!error) {
        logs = doc["logs"].as<JsonArray>();
      }
    }
  }

  // If doc is empty, create new structure
  if (logs.isNull()) {
    logs = doc["logs"].to<JsonArray>();
  }

  // Add new QSO
  JsonObject newLog = logs.add<JsonObject>();
  qsoToJson(qso, newLog);

  // Ensure logs directory exists
  if (!FileSystem.exists(LOGS_DIR)) {
    Serial.println("Creating /logs directory...");
    if (!FileSystem.mkdir(LOGS_DIR)) {
      Serial.println("Failed to create /logs directory");
      return false;
    }
  }

  // Write back to file
  Serial.print("Opening file for writing: ");
  Serial.println(filename);
  File file = FileSystem.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open log file for writing");
    Serial.print("LittleFS total: ");
    Serial.println(FileSystem.totalBytes());
    Serial.print("LittleFS used: ");
    Serial.println(FileSystem.usedBytes());
    return false;
  }

  // Debug: Check JSON size and content before writing
  size_t jsonSize = measureJson(doc);
  Serial.print("JSON size to write: ");
  Serial.print(jsonSize);
  Serial.println(" bytes");

  // Print JSON to serial for inspection
  Serial.println("JSON content:");
  serializeJson(doc, Serial);
  Serial.println();

  size_t bytesWritten = serializeJson(doc, file);
  file.close();

  Serial.print("Bytes actually written: ");
  Serial.println(bytesWritten);

  if (bytesWritten < jsonSize) {
    Serial.println("WARNING: Not all JSON was written to file!");
  }

  // Update metadata
  storageStats.totalLogs++;
  if (storageStats.newestLogId == 0 || qso.id > storageStats.newestLogId) {
    storageStats.newestLogId = qso.id;
  }
  if (storageStats.oldestLogId == 0 || qso.id < storageStats.oldestLogId) {
    storageStats.oldestLogId = qso.id;
  }

  // Update band statistics
  int bandIdx = getBandIndex(qso.band);
  if (bandIdx >= 0) {
    storageStats.logsByBand[bandIdx]++;
  }

  // Update mode statistics
  int modeIdx = getModeIndex(qso.mode);
  if (modeIdx >= 0) {
    storageStats.logsByMode[modeIdx]++;
  }

  saveMetadata();

  Serial.println("QSO saved successfully");

  // Check circular buffer limit
  if (storageStats.totalLogs > MAX_LOGS) {
    Serial.println("WARNING: Max logs exceeded, circular buffer not yet implemented");
    // TODO: Implement oldest log deletion in future milestone
  }

  return true;
}

/*
 * Load all QSOs from storage (for viewing/exporting)
 * Returns number of logs loaded
 */
int loadAllQSOs(QSO* qsos, int maxCount) {
  Serial.println("Loading all QSOs...");

  int count = 0;
  File root = FileSystem.open(LOGS_DIR);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open logs directory");
    return 0;
  }

  File file = root.openNextFile();
  while (file && count < maxCount) {
    String filename = file.name();

    // Only process QSO log files (qso_YYYYMMDD.json)
    if (filename.startsWith("qso_") && filename.endsWith(".json")) {
      Serial.print("Reading: ");
      Serial.println(filename);

      // Parse JSON
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);

      if (!error) {
        JsonArray logs = doc["logs"];
        for (JsonObject logObj : logs) {
          if (count < maxCount) {
            jsonToQso(logObj, qsos[count]);
            count++;
          } else {
            break;
          }
        }
      } else {
        Serial.print("Failed to parse: ");
        Serial.println(error.c_str());
      }
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();

  Serial.print("Loaded ");
  Serial.print(count);
  Serial.println(" QSOs");

  return count;
}

/*
 * Delete a QSO by ID
 */
bool deleteQSO(unsigned long id) {
  Serial.print("Deleting QSO ID: ");
  Serial.println(id);

  // Search through all log files
  File root = FileSystem.open(LOGS_DIR);
  if (!root || !root.isDirectory()) {
    return false;
  }

  bool found = false;
  File file = root.openNextFile();
  while (file && !found) {
    String filename = file.name();

    if (filename.startsWith("qso_") && filename.endsWith(".json")) {
      // Load file
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);
      file.close();

      if (!error) {
        JsonArray logs = doc["logs"];

        // Find and remove the QSO
        for (size_t i = 0; i < logs.size(); i++) {
          if (logs[i]["id"] == id) {
            // Found it! Remove from array
            logs.remove(i);
            found = true;

            // Write back to file
            String fullPath = String(LOGS_DIR) + "/" + filename;
            File outFile = FileSystem.open(fullPath, "w");
            if (outFile) {
              serializeJson(doc, outFile);
              outFile.close();

              // Update metadata
              storageStats.totalLogs--;
              saveMetadata();

              Serial.println("QSO deleted successfully");
              return true;
            }
            break;
          }
        }
      }
    } else {
      file.close();
    }

    file = root.openNextFile();
  }

  root.close();

  if (!found) {
    Serial.println("QSO not found");
  }

  return found;
}

/*
 * Get total number of logs
 */
int getTotalLogs() {
  return storageStats.totalLogs;
}

/*
 * Test function: Save a dummy QSO
 */
void testSaveDummyQSO() {
  Serial.println("\n=== Testing QSO Storage ===");

  QSO testQSO;
  memset(&testQSO, 0, sizeof(QSO));

  testQSO.id = millis();
  strcpy(testQSO.callsign, "W1AW");
  testQSO.frequency = 14.025;
  strcpy(testQSO.mode, "CW");
  strcpy(testQSO.band, "20m");
  strcpy(testQSO.rst_sent, "599");
  strcpy(testQSO.rst_rcvd, "599");
  strcpy(testQSO.date, "20250428");
  strcpy(testQSO.time_on, "1430");
  strcpy(testQSO.name, "Hiram");
  strcpy(testQSO.qth, "Newington, CT");
  testQSO.power = 100;
  strcpy(testQSO.gridsquare, "FN31pr");
  strcpy(testQSO.notes, "Nice fist!");
  strcpy(testQSO.operator_call, operatorCallsign);
  strcpy(testQSO.station_call, operatorCallsign);

  if (saveQSO(testQSO)) {
    Serial.println("✓ Dummy QSO saved");

    // Test loading
    QSO loaded[10];
    int count = loadAllQSOs(loaded, 10);

    if (count > 0) {
      Serial.println("✓ QSO loaded");
      Serial.print("  Callsign: ");
      Serial.println(loaded[0].callsign);
      Serial.print("  Frequency: ");
      Serial.println(loaded[0].frequency);
      Serial.print("  Mode: ");
      Serial.println(loaded[0].mode);
    }
  } else {
    Serial.println("✗ Failed to save dummy QSO");
  }

  Serial.println("=== Test Complete ===\n");
}

#endif // QSO_LOGGER_STORAGE_H
