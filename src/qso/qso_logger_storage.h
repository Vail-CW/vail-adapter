/*
 * QSO Logger Storage Module
 * SD card-based storage for contact logs with JSON working format
 * and auto-generated ADIF backups
 */

#ifndef QSO_LOGGER_STORAGE_H
#define QSO_LOGGER_STORAGE_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "qso_logger.h"  // Same folder
#include "../storage/sd_card.h"
#include "../core/config.h"

// ============================================
// Storage Configuration
// ============================================

#define MAX_LOGS 500                    // Maximum logs before warning
#define QSO_DIR "/qso"                  // QSO files directory on SD card
#define METADATA_DIR "/logs"            // Metadata directory on SPIFFS
#define METADATA_FILE "/logs/metadata.json"  // Statistics cache on SPIFFS
#define MASTER_ADIF_FILE "/qso/vail-summit.adi"  // Master ADIF file

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

// QSO storage ready flag
bool qsoStorageReady = false;

// ============================================
// Forward Declarations
// ============================================

void loadMetadata();
void saveMetadata();
void regenerateADIFFiles(const char* date);
void generateMasterADIF();
void generateDailyADIF(const char* date);
String qsoToADIFRecord(const QSO& qso);

// ============================================
// Helper Functions
// ============================================

/*
 * Initialize metadata storage on SPIFFS (for fast stats access)
 */
bool initMetadataStorage() {
  Serial.println("Initializing SPIFFS for metadata...");

  // Try to mount first
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS mount failed, trying to format...");

    // Format and try again
    if (!SPIFFS.begin(true)) {
      Serial.println("ERROR: SPIFFS format and mount failed!");
      return false;
    }
  }

  Serial.println("SPIFFS mounted successfully");

  // Create metadata directory if it doesn't exist
  if (!SPIFFS.exists(METADATA_DIR)) {
    Serial.println("Creating /logs directory on SPIFFS...");
    SPIFFS.mkdir(METADATA_DIR);
  }

  // Load metadata
  loadMetadata();

  return true;
}

/*
 * Initialize QSO storage on SD card
 * Returns true if SD card is available and ready
 */
bool initQSOStorage() {
  Serial.println("Initializing QSO storage...");

  // First ensure metadata storage is ready
  if (!initMetadataStorage()) {
    Serial.println("WARNING: Metadata storage unavailable");
  }

  // Check if SD card is already initialized
  if (!sdCardAvailable) {
    Serial.println("SD card not initialized, attempting init...");
    if (!initSDCard()) {
      Serial.println("ERROR: SD card required for QSO logging");
      qsoStorageReady = false;
      return false;
    }
  }

  // Create QSO directory if it doesn't exist
  if (!createSDDirectory(QSO_DIR)) {
    Serial.println("ERROR: Failed to create /qso directory on SD card");
    qsoStorageReady = false;
    return false;
  }

  Serial.println("QSO storage initialized successfully on SD card");
  qsoStorageReady = true;

  // Print storage info
  Serial.print("Total logs: ");
  Serial.println(storageStats.totalLogs);

  return true;
}

/*
 * Check if QSO storage is ready for use
 */
bool isQSOStorageReady() {
  return qsoStorageReady && sdCardAvailable;
}

/*
 * Legacy init function - now initializes both SPIFFS and SD card
 */
bool initStorage() {
  return initQSOStorage();
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
// Metadata Management (SPIFFS)
// ============================================

/*
 * Load metadata from SPIFFS
 */
void loadMetadata() {
  memset(&storageStats, 0, sizeof(StorageStats));

  if (!SPIFFS.exists(METADATA_FILE)) {
    Serial.println("No metadata file found, starting fresh");
    return;
  }

  File file = SPIFFS.open(METADATA_FILE, "r");
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
 * Save metadata to SPIFFS
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
  File file = SPIFFS.open(METADATA_FILE, "w");
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
  if (strlen(qso.my_gridsquare) > 0) obj["my_gridsquare"] = qso.my_gridsquare;
  if (strlen(qso.my_pota_ref) > 0) obj["my_pota_ref"] = qso.my_pota_ref;
  if (strlen(qso.their_pota_ref) > 0) obj["their_pota_ref"] = qso.their_pota_ref;
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

  // Location fields
  if (obj.containsKey("my_gridsquare")) {
    const char* val = obj["my_gridsquare"];
    if (val != nullptr && strlen(val) > 0) {
      strlcpy(qso.my_gridsquare, val, sizeof(qso.my_gridsquare));
    }
  }
  if (obj.containsKey("my_pota_ref")) {
    const char* val = obj["my_pota_ref"];
    if (val != nullptr && strlen(val) > 0) {
      strlcpy(qso.my_pota_ref, val, sizeof(qso.my_pota_ref));
    }
  }
  if (obj.containsKey("their_pota_ref")) {
    const char* val = obj["their_pota_ref"];
    if (val != nullptr && strlen(val) > 0) {
      strlcpy(qso.their_pota_ref, val, sizeof(qso.their_pota_ref));
    }
  }
}

// ============================================
// ADIF Generation
// ============================================

/*
 * Convert a single QSO to ADIF record format
 */
String qsoToADIFRecord(const QSO& qso) {
  String record = "";

  // Required fields
  record += "<CALL:" + String(strlen(qso.callsign)) + ">" + qso.callsign + " ";

  char freqStr[16];
  snprintf(freqStr, sizeof(freqStr), "%.3f", qso.frequency);
  record += "<FREQ:" + String(strlen(freqStr)) + ">" + freqStr + " ";

  record += "<MODE:" + String(strlen(qso.mode)) + ">" + qso.mode + " ";
  record += "<BAND:" + String(strlen(qso.band)) + ">" + qso.band + " ";
  record += "<QSO_DATE:" + String(strlen(qso.date)) + ">" + qso.date + " ";

  // Convert time from HHMM to HHMMSS
  char timeStr[8];
  snprintf(timeStr, sizeof(timeStr), "%s00", qso.time_on);
  record += "<TIME_ON:" + String(strlen(timeStr)) + ">" + timeStr + " ";

  // RST
  if (strlen(qso.rst_sent) > 0) {
    record += "<RST_SENT:" + String(strlen(qso.rst_sent)) + ">" + qso.rst_sent + " ";
  }
  if (strlen(qso.rst_rcvd) > 0) {
    record += "<RST_RCVD:" + String(strlen(qso.rst_rcvd)) + ">" + qso.rst_rcvd + " ";
  }

  // Optional fields
  if (strlen(qso.name) > 0) {
    record += "<NAME:" + String(strlen(qso.name)) + ">" + qso.name + " ";
  }
  if (strlen(qso.qth) > 0) {
    record += "<QTH:" + String(strlen(qso.qth)) + ">" + qso.qth + " ";
  }
  if (strlen(qso.gridsquare) > 0) {
    record += "<GRIDSQUARE:" + String(strlen(qso.gridsquare)) + ">" + qso.gridsquare + " ";
  }
  if (strlen(qso.country) > 0) {
    record += "<COUNTRY:" + String(strlen(qso.country)) + ">" + qso.country + " ";
  }
  if (strlen(qso.state) > 0) {
    record += "<STATE:" + String(strlen(qso.state)) + ">" + qso.state + " ";
  }
  if (qso.power > 0) {
    char powerStr[8];
    snprintf(powerStr, sizeof(powerStr), "%d", qso.power);
    record += "<TX_PWR:" + String(strlen(powerStr)) + ">" + powerStr + " ";
  }
  if (strlen(qso.notes) > 0) {
    record += "<COMMENT:" + String(strlen(qso.notes)) + ">" + qso.notes + " ";
  }

  // My location fields
  if (strlen(qso.my_gridsquare) > 0) {
    record += "<MY_GRIDSQUARE:" + String(strlen(qso.my_gridsquare)) + ">" + qso.my_gridsquare + " ";
  }

  // POTA fields
  if (strlen(qso.my_pota_ref) > 0) {
    record += "<MY_SIG:4>POTA ";
    record += "<MY_SIG_INFO:" + String(strlen(qso.my_pota_ref)) + ">" + qso.my_pota_ref + " ";
  }
  if (strlen(qso.their_pota_ref) > 0) {
    record += "<SIG:4>POTA ";
    record += "<SIG_INFO:" + String(strlen(qso.their_pota_ref)) + ">" + qso.their_pota_ref + " ";
  }

  // Operator/station
  if (strlen(qso.operator_call) > 0) {
    record += "<OPERATOR:" + String(strlen(qso.operator_call)) + ">" + qso.operator_call + " ";
  }
  if (strlen(qso.station_call) > 0) {
    record += "<STATION_CALLSIGN:" + String(strlen(qso.station_call)) + ">" + qso.station_call + " ";
  }

  record += "<EOR>\n";
  return record;
}

/*
 * Generate ADIF header
 */
String generateADIFHeader() {
  String header = "ADIF Export from VAIL SUMMIT\n";
  header += "Generated by " + String(FIRMWARE_NAME) + " v" + String(FIRMWARE_VERSION) + "\n\n";
  header += "<PROGRAMID:11>VAIL SUMMIT\n";
  header += "<PROGRAMVERSION:" + String(strlen(FIRMWARE_VERSION)) + ">" + FIRMWARE_VERSION + "\n";
  header += "<ADIF_VER:5>3.1.4\n";
  header += "<EOH>\n\n";
  return header;
}

/*
 * Generate daily ADIF file for a specific date
 */
void generateDailyADIF(const char* date) {
  if (!sdCardAvailable) return;

  // Build filenames
  char jsonPath[40];
  char adifPath[40];
  snprintf(jsonPath, sizeof(jsonPath), "%s/qso_%s.json", QSO_DIR, date);
  snprintf(adifPath, sizeof(adifPath), "%s/qso_%s.adi", QSO_DIR, date);

  Serial.print("Generating daily ADIF: ");
  Serial.println(adifPath);

  // Check if JSON file exists
  if (!SD.exists(jsonPath)) {
    Serial.println("No JSON file for this date");
    return;
  }

  // Read JSON file
  File jsonFile = SD.open(jsonPath, "r");
  if (!jsonFile) {
    Serial.println("Failed to open JSON file");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonFile);
  jsonFile.close();

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // Generate ADIF content
  String adifContent = generateADIFHeader();

  JsonArray logs = doc["logs"];
  for (JsonObject logObj : logs) {
    QSO qso;
    jsonToQso(logObj, qso);
    adifContent += qsoToADIFRecord(qso);
  }

  // Write ADIF file
  File adifFile = SD.open(adifPath, FILE_WRITE);
  if (!adifFile) {
    Serial.println("Failed to create ADIF file");
    return;
  }

  adifFile.print(adifContent);
  adifFile.close();

  Serial.print("Daily ADIF generated: ");
  Serial.print(logs.size());
  Serial.println(" QSOs");
}

/*
 * Generate master ADIF file containing all QSOs
 */
void generateMasterADIF() {
  if (!sdCardAvailable) return;

  Serial.println("Generating master ADIF file...");

  // Start with header
  String adifContent = generateADIFHeader();

  // Iterate all JSON files in QSO directory
  File root = SD.open(QSO_DIR);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open QSO directory");
    return;
  }

  int totalQSOs = 0;
  File file = root.openNextFile();
  while (file) {
    String filename = file.name();

    // Only process JSON files
    if (filename.endsWith(".json")) {
      Serial.print("Processing: ");
      Serial.println(filename);

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);

      if (!error) {
        JsonArray logs = doc["logs"];
        for (JsonObject logObj : logs) {
          QSO qso;
          jsonToQso(logObj, qso);
          adifContent += qsoToADIFRecord(qso);
          totalQSOs++;
        }
      }
    }

    file.close();
    file = root.openNextFile();
  }
  root.close();

  // Write master ADIF file
  File adifFile = SD.open(MASTER_ADIF_FILE, FILE_WRITE);
  if (!adifFile) {
    Serial.println("Failed to create master ADIF file");
    return;
  }

  adifFile.print(adifContent);
  adifFile.close();

  Serial.print("Master ADIF generated: ");
  Serial.print(totalQSOs);
  Serial.println(" QSOs");
}

/*
 * Regenerate ADIF files after any QSO change
 */
void regenerateADIFFiles(const char* date) {
  generateDailyADIF(date);
  generateMasterADIF();
}

// ============================================
// QSO Storage Operations (SD Card)
// ============================================

/*
 * Get filename for a QSO log (based on date)
 */
String getLogFilename(const char* date) {
  char filename[40];
  snprintf(filename, sizeof(filename), "%s/qso_%s.json", QSO_DIR, date);
  return String(filename);
}

/*
 * Save a QSO to SD card storage
 */
bool saveQSO(const QSO& qso) {
  if (!isQSOStorageReady()) {
    Serial.println("ERROR: QSO storage not ready (SD card required)");
    return false;
  }

  Serial.print("Saving QSO: ");
  Serial.println(qso.callsign);

  // Get filename based on date
  String filename = getLogFilename(qso.date);
  Serial.print("Filename: ");
  Serial.println(filename);

  // Load existing logs for this day (if any)
  JsonDocument doc;
  JsonArray logs;

  if (SD.exists(filename)) {
    File file = SD.open(filename, "r");
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

  // Write back to file
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open log file for writing");
    return false;
  }

  size_t bytesWritten = serializeJson(doc, file);
  file.close();

  Serial.print("Bytes written: ");
  Serial.println(bytesWritten);

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

  // Regenerate ADIF files
  regenerateADIFFiles(qso.date);

  Serial.println("QSO saved successfully");

  // Check log limit
  if (storageStats.totalLogs > MAX_LOGS) {
    Serial.println("WARNING: Max logs exceeded");
  }

  return true;
}

/*
 * Load all QSOs from SD card storage (for viewing/exporting)
 * Returns number of logs loaded
 */
int loadAllQSOs(QSO* qsos, int maxCount) {
  if (!isQSOStorageReady()) {
    Serial.println("ERROR: QSO storage not ready");
    return 0;
  }

  Serial.println("Loading all QSOs from SD card...");

  int count = 0;
  File root = SD.open(QSO_DIR);
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open QSO directory");
    return 0;
  }

  File file = root.openNextFile();
  while (file && count < maxCount) {
    String filename = file.name();

    // Only process QSO log files (qso_YYYYMMDD.json)
    if (filename.endsWith(".json")) {
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
  if (!isQSOStorageReady()) {
    Serial.println("ERROR: QSO storage not ready");
    return false;
  }

  Serial.print("Deleting QSO ID: ");
  Serial.println(id);

  // Search through all log files
  File root = SD.open(QSO_DIR);
  if (!root || !root.isDirectory()) {
    return false;
  }

  bool found = false;
  String deletedDate = "";
  File file = root.openNextFile();

  while (file && !found) {
    String filename = file.name();

    if (filename.endsWith(".json")) {
      // Load file
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);
      file.close();

      if (!error) {
        JsonArray logs = doc["logs"];

        // Find and remove the QSO
        for (size_t i = 0; i < logs.size(); i++) {
          if (logs[i]["id"] == id) {
            // Extract date for ADIF regeneration
            const char* date = logs[i]["date"];
            if (date) {
              deletedDate = String(date);
            }

            // Remove from array
            logs.remove(i);
            found = true;

            // Write back to file
            String fullPath = String(QSO_DIR) + "/" + filename;
            File outFile = SD.open(fullPath, FILE_WRITE);
            if (outFile) {
              serializeJson(doc, outFile);
              outFile.close();

              // Update metadata
              storageStats.totalLogs--;
              saveMetadata();

              // Regenerate ADIF files
              if (deletedDate.length() > 0) {
                regenerateADIFFiles(deletedDate.c_str());
              }

              Serial.println("QSO deleted successfully");
              root.close();
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
 * Update an existing QSO
 */
bool updateQSO(const QSO& qso) {
  if (!isQSOStorageReady()) {
    Serial.println("ERROR: QSO storage not ready");
    return false;
  }

  Serial.print("Updating QSO ID: ");
  Serial.println(qso.id);

  // Search through all log files
  File root = SD.open(QSO_DIR);
  if (!root || !root.isDirectory()) {
    return false;
  }

  bool found = false;
  File file = root.openNextFile();

  while (file && !found) {
    String filename = file.name();

    if (filename.endsWith(".json")) {
      // Load file
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);
      file.close();

      if (!error) {
        JsonArray logs = doc["logs"];

        // Find and update the QSO
        for (size_t i = 0; i < logs.size(); i++) {
          if (logs[i]["id"] == qso.id) {
            // Update the entry
            JsonObject logObj = logs[i];
            qsoToJson(qso, logObj);
            found = true;

            // Write back to file
            String fullPath = String(QSO_DIR) + "/" + filename;
            File outFile = SD.open(fullPath, FILE_WRITE);
            if (outFile) {
              serializeJson(doc, outFile);
              outFile.close();

              // Regenerate ADIF files
              regenerateADIFFiles(qso.date);

              Serial.println("QSO updated successfully");
              root.close();
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
    Serial.println("QSO not found for update");
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
 * Recalculate metadata by scanning all QSO files
 * Useful if metadata gets out of sync
 */
void recalculateMetadata() {
  if (!isQSOStorageReady()) return;

  Serial.println("Recalculating metadata from SD card...");

  memset(&storageStats, 0, sizeof(StorageStats));

  File root = SD.open(QSO_DIR);
  if (!root || !root.isDirectory()) {
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String filename = file.name();

    if (filename.endsWith(".json")) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);

      if (!error) {
        JsonArray logs = doc["logs"];
        for (JsonObject logObj : logs) {
          storageStats.totalLogs++;

          unsigned long id = logObj["id"] | 0;
          if (storageStats.oldestLogId == 0 || id < storageStats.oldestLogId) {
            storageStats.oldestLogId = id;
          }
          if (id > storageStats.newestLogId) {
            storageStats.newestLogId = id;
          }

          const char* band = logObj["band"] | "";
          int bandIdx = getBandIndex(band);
          if (bandIdx >= 0) {
            storageStats.logsByBand[bandIdx]++;
          }

          const char* mode = logObj["mode"] | "";
          int modeIdx = getModeIndex(mode);
          if (modeIdx >= 0) {
            storageStats.logsByMode[modeIdx]++;
          }
        }
      }
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
  saveMetadata();

  Serial.print("Recalculated: ");
  Serial.print(storageStats.totalLogs);
  Serial.println(" total QSOs");
}

#endif // QSO_LOGGER_STORAGE_H
