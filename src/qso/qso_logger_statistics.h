// QSO Logger Statistics Module
// Calculate analytics from saved QSO logs
// UI is now handled by LVGL in lv_mode_screens.h

#ifndef QSO_LOGGER_STATISTICS_H
#define QSO_LOGGER_STATISTICS_H

#include <Arduino.h>
#include "../core/config.h"
#include "qso_logger.h"  // Same folder
#include "qso_logger_storage.h"  // Same folder

// ============================================
// Statistics Data Structure
// ============================================

struct QSOStatistics {
  int totalQSOs;

  // Band breakdown
  struct BandStats {
    char band[6];
    int count;
  };
  BandStats bandStats[10];  // Support up to 10 different bands
  int bandCount;

  // Mode breakdown
  struct ModeStats {
    char mode[8];
    int count;
  };
  ModeStats modeStats[8];   // Support up to 8 different modes
  int modeCount;

  // Unique callsigns
  int uniqueCallsigns;

  // Most active date
  char mostActiveDate[9];
  int mostActiveDateCount;

  // Last QSO date
  char lastQSODate[9];
};

QSOStatistics stats;

// ============================================
// Statistics Calculation Functions
// ============================================

// Helper: Find or add band to stats
int findOrAddBand(const char* band) {
  // Search for existing band
  for (int i = 0; i < stats.bandCount; i++) {
    if (strcmp(stats.bandStats[i].band, band) == 0) {
      return i;
    }
  }

  // Add new band if space available
  if (stats.bandCount < 10) {
    strlcpy(stats.bandStats[stats.bandCount].band, band, sizeof(stats.bandStats[stats.bandCount].band));
    stats.bandStats[stats.bandCount].count = 0;
    return stats.bandCount++;
  }

  return -1;  // No space
}

// Helper: Find or add mode to stats
int findOrAddMode(const char* mode) {
  // Search for existing mode
  for (int i = 0; i < stats.modeCount; i++) {
    if (strcmp(stats.modeStats[i].mode, mode) == 0) {
      return i;
    }
  }

  // Add new mode if space available
  if (stats.modeCount < 8) {
    strlcpy(stats.modeStats[stats.modeCount].mode, mode, sizeof(stats.modeStats[stats.modeCount].mode));
    stats.modeStats[stats.modeCount].count = 0;
    return stats.modeCount++;
  }

  return -1;  // No space
}

// Calculate all statistics from saved QSO logs
void calculateStatistics() {
  Serial.println("Calculating QSO statistics...");

  // Reset stats
  memset(&stats, 0, sizeof(stats));

  // Track unique callsigns (simple array, limited to 100 for memory)
  String uniqueCallsignsList[100];
  int uniqueCount = 0;

  // Track dates for most active day
  struct DateCount {
    char date[9];
    int count;
  };
  DateCount dateCounts[50];  // Track up to 50 different dates
  int dateCountsSize = 0;

  // Open logs directory
  File root = SD.open("/logs");
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open /logs directory");
    return;
  }

  // Iterate through all log files
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = String(file.name());

      // Extract basename
      int lastSlash = filename.lastIndexOf('/');
      if (lastSlash >= 0) {
        filename = filename.substring(lastSlash + 1);
      }

      if (filename.startsWith("qso_") && filename.endsWith(".json")) {
        Serial.print("Processing: ");
        Serial.println(filename);

        File logFile = SD.open(file.path(), "r");
        if (logFile) {
          String content = logFile.readString();
          logFile.close();

          StaticJsonDocument<8192> doc;
          DeserializationError error = deserializeJson(doc, content);

          if (!error && doc.containsKey("logs")) {
            JsonArray logs = doc["logs"].as<JsonArray>();

            for (JsonObject qso : logs) {
              stats.totalQSOs++;

              // Band stats
              const char* band = qso["band"] | "";
              if (strlen(band) > 0) {
                int bandIndex = findOrAddBand(band);
                if (bandIndex >= 0) {
                  stats.bandStats[bandIndex].count++;
                }
              }

              // Mode stats
              const char* mode = qso["mode"] | "CW";
              int modeIndex = findOrAddMode(mode);
              if (modeIndex >= 0) {
                stats.modeStats[modeIndex].count++;
              }

              // Unique callsigns
              String callsign = String(qso["callsign"] | "");
              if (callsign.length() > 0) {
                bool found = false;
                for (int i = 0; i < uniqueCount; i++) {
                  if (uniqueCallsignsList[i] == callsign) {
                    found = true;
                    break;
                  }
                }
                if (!found && uniqueCount < 100) {
                  uniqueCallsignsList[uniqueCount++] = callsign;
                }
              }

              // Date tracking
              const char* date = qso["date"] | "";
              if (strlen(date) > 0) {
                // Update last QSO date (assuming files are chronological)
                strlcpy(stats.lastQSODate, date, sizeof(stats.lastQSODate));

                // Find or add date count
                int dateIndex = -1;
                for (int i = 0; i < dateCountsSize; i++) {
                  if (strcmp(dateCounts[i].date, date) == 0) {
                    dateIndex = i;
                    break;
                  }
                }

                if (dateIndex >= 0) {
                  dateCounts[dateIndex].count++;
                } else if (dateCountsSize < 50) {
                  strlcpy(dateCounts[dateCountsSize].date, date, sizeof(dateCounts[dateCountsSize].date));
                  dateCounts[dateCountsSize].count = 1;
                  dateCountsSize++;
                }
              }
            }
          }
        }
      }
    }
    file = root.openNextFile();
  }
  root.close();

  // Set unique callsigns count
  stats.uniqueCallsigns = uniqueCount;

  // Find most active date
  int maxCount = 0;
  for (int i = 0; i < dateCountsSize; i++) {
    if (dateCounts[i].count > maxCount) {
      maxCount = dateCounts[i].count;
      strlcpy(stats.mostActiveDate, dateCounts[i].date, sizeof(stats.mostActiveDate));
      stats.mostActiveDateCount = maxCount;
    }
  }

  Serial.println("Statistics calculated:");
  Serial.print("  Total QSOs: ");
  Serial.println(stats.totalQSOs);
  Serial.print("  Unique callsigns: ");
  Serial.println(stats.uniqueCallsigns);
  Serial.print("  Bands: ");
  Serial.println(stats.bandCount);
  Serial.print("  Modes: ");
  Serial.println(stats.modeCount);
}

#endif // QSO_LOGGER_STATISTICS_H
