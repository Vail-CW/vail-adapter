// QSO Logger View Module
// Data loading and management for QSO logs
// UI is now handled by LVGL in lv_mode_screens.h

#ifndef QSO_LOGGER_VIEW_H
#define QSO_LOGGER_VIEW_H

#include <Arduino.h>
#include "../core/config.h"
#include "qso_logger.h"  // Same folder
#include "qso_logger_storage.h"  // Same folder

// View state
enum ViewMode {
  VIEW_MODE_LIST = 0,
  VIEW_MODE_DETAIL = 1,
  VIEW_MODE_DELETE_CONFIRM = 2
};

struct ViewState {
  ViewMode mode;
  int selectedIndex;        // Currently selected QSO in list
  int scrollOffset;         // Top visible item in list
  int totalQSOs;           // Total number of QSOs loaded
  QSO* qsos;               // Array of loaded QSOs (dynamically allocated)
  int detailScrollOffset;  // Scroll position in detail view
  bool deleteConfirm;      // Waiting for delete confirmation
};

ViewState viewState = {VIEW_MODE_LIST, 0, 0, 0, nullptr, 0, false};

// Forward declarations
void loadQSOsForView();
void freeQSOsFromView();

// Load all QSOs from storage into memory
void loadQSOsForView() {
  Serial.println("Loading QSOs for view...");

  // First, count total QSOs
  int totalCount = 0;
  File root = SD.open("/qso");
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open /qso directory");
    return;
  }

  // Count QSOs across all daily log files
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = String(file.name());
      Serial.print("Checking file: ");
      Serial.println(filename);

      // Extract just the filename from full path if needed
      int lastSlash = filename.lastIndexOf('/');
      if (lastSlash >= 0) {
        filename = filename.substring(lastSlash + 1);
      }

      Serial.print("  Basename: ");
      Serial.println(filename);

      if (filename.startsWith("qso_") && filename.endsWith(".json")) {
        Serial.println("  -> Loading this file");
        // Load this file and count entries
        File logFile = SD.open(file.path(), "r");
        if (logFile) {
          String content = logFile.readString();
          logFile.close();

          Serial.print("    File content length: ");
          Serial.println(content.length());
          Serial.print("    First 200 chars: ");
          Serial.println(content.substring(0, min(200, (int)content.length())));

          StaticJsonDocument<8192> doc;
          DeserializationError error = deserializeJson(doc, content);

          Serial.print("    JSON parse result: ");
          Serial.println(error.c_str());
          Serial.print("    Contains 'logs' key: ");
          Serial.println(doc.containsKey("logs") ? "YES" : "NO");

          if (!error && doc.containsKey("logs")) {
            JsonArray qsos = doc["logs"].as<JsonArray>();
            int qsoCount = qsos.size();
            Serial.print("    Found ");
            Serial.print(qsoCount);
            Serial.println(" QSOs in this file");
            totalCount += qsoCount;
          } else {
            Serial.print("    JSON error or no qsos key: ");
            Serial.println(error.c_str());
          }
        }
      }
    }
    file = root.openNextFile();
  }
  root.close();

  Serial.print("Total QSOs found: ");
  Serial.println(totalCount);

  if (totalCount == 0) {
    viewState.totalQSOs = 0;
    viewState.qsos = nullptr;
    return;
  }

  // Free any existing allocation before allocating new
  if (viewState.qsos != nullptr) {
    delete[] viewState.qsos;
    viewState.qsos = nullptr;
  }

  // Allocate memory for QSOs
  viewState.qsos = new QSO[totalCount];
  if (viewState.qsos == nullptr) {
    Serial.println("ERROR: Failed to allocate memory for QSOs");
    viewState.totalQSOs = 0;
    return;
  }
  viewState.totalQSOs = totalCount;
  Serial.printf("Allocated memory for %d QSOs at %p\n", totalCount, viewState.qsos);

  // Initialize all QSOs to zero
  memset(viewState.qsos, 0, sizeof(QSO) * totalCount);

  // Load QSOs into array
  int qsoIndex = 0;
  root = SD.open("/qso");
  if (!root || !root.isDirectory()) {
    Serial.println("ERROR: Failed to reopen /qso directory for loading");
    delete[] viewState.qsos;
    viewState.qsos = nullptr;
    viewState.totalQSOs = 0;
    return;
  }
  file = root.openNextFile();

  while (file && qsoIndex < totalCount) {
    if (!file.isDirectory()) {
      String filename = String(file.name());

      // Extract just the filename from full path if needed
      int lastSlash = filename.lastIndexOf('/');
      if (lastSlash >= 0) {
        filename = filename.substring(lastSlash + 1);
      }

      if (filename.startsWith("qso_") && filename.endsWith(".json")) {
        File logFile = SD.open(file.path(), "r");
        if (logFile) {
          String content = logFile.readString();
          logFile.close();

          StaticJsonDocument<8192> doc;
          DeserializationError error = deserializeJson(doc, content);

          if (!error && doc.containsKey("logs")) {
            JsonArray qsos = doc["logs"].as<JsonArray>();
            for (JsonObject qsoObj : qsos) {
              if (qsoIndex >= totalCount) break;

              // Use the standard jsonToQso function to load all fields
              jsonToQso(qsoObj, viewState.qsos[qsoIndex]);

              qsoIndex++;
            }
          }
        }
      }
    }
    file = root.openNextFile();
  }
  root.close();

  Serial.print("Loaded ");
  Serial.print(qsoIndex);
  Serial.println(" QSOs into memory");
}

// Free QSO memory when exiting view
void freeQSOsFromView() {
  if (viewState.qsos != nullptr) {
    delete[] viewState.qsos;
    viewState.qsos = nullptr;
  }
  viewState.totalQSOs = 0;
}

// Delete the currently viewed QSO from storage
bool deleteCurrentQSO() {
  if (viewState.selectedIndex < 0 || viewState.selectedIndex >= viewState.totalQSOs) {
    return false;
  }

  QSO& qsoToDelete = viewState.qsos[viewState.selectedIndex];

  Serial.print("Deleting QSO: ");
  Serial.print(qsoToDelete.callsign);
  Serial.print(" ID: ");
  Serial.println(qsoToDelete.id);

  // Get the log filename for this QSO's date
  String filename = getLogFilename(qsoToDelete.date);

  if (!SD.exists(filename)) {
    Serial.println("Log file doesn't exist!");
    return false;
  }

  // Read the entire log file
  File file = SD.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open log file for reading");
    return false;
  }

  String content = file.readString();
  file.close();

  // Parse JSON
  StaticJsonDocument<8192> doc;
  DeserializationError error = deserializeJson(doc, content);

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  if (!doc.containsKey("logs")) {
    Serial.println("No logs array found");
    return false;
  }

  // Create new array without the deleted QSO
  JsonArray oldLogs = doc["logs"].as<JsonArray>();
  StaticJsonDocument<8192> newDoc;
  JsonArray newLogs = newDoc.createNestedArray("logs");

  int removedCount = 0;
  for (JsonObject qsoObj : oldLogs) {
    unsigned long id = qsoObj["id"] | 0;
    if (id != qsoToDelete.id) {
      // Keep this QSO
      newLogs.add(qsoObj);
    } else {
      // Skip this QSO (delete it)
      removedCount++;
      Serial.print("Removed QSO with ID: ");
      Serial.println(id);
    }
  }

  if (removedCount == 0) {
    Serial.println("QSO not found in file!");
    return false;
  }

  // Write back to file
  file = SD.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open log file for writing");
    return false;
  }

  serializeJson(newDoc, file);
  file.close();

  // Update metadata to decrement total count
  if (storageStats.totalLogs > 0) {
    storageStats.totalLogs--;
    saveMetadata();
    Serial.print("Updated metadata: totalLogs now = ");
    Serial.println(storageStats.totalLogs);
  }

  Serial.println("QSO deleted successfully");
  return true;
}

#endif // QSO_LOGGER_VIEW_H
