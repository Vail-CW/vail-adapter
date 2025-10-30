// QSO Logger Statistics Module
// Calculate and display analytics from saved QSO logs

#ifndef QSO_LOGGER_STATISTICS_H
#define QSO_LOGGER_STATISTICS_H

#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include "config.h"
#include "qso_logger.h"
#include "qso_logger_storage.h"

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
  File root = FileSystem.open("/logs");
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

        File logFile = FileSystem.open(file.path(), "r");
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

// ============================================
// Statistics UI
// ============================================

void drawStatisticsUI(Adafruit_ST7789& tft) {
  tft.fillScreen(COLOR_BACKGROUND);

  // Header
  tft.fillRect(0, 0, SCREEN_WIDTH, 40, 0x1082); // Dark blue header
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 12);
  tft.print("Statistics");

  // Clear content area
  tft.fillRect(0, 40, SCREEN_WIDTH, SCREEN_HEIGHT - 40 - 20, COLOR_BACKGROUND);

  if (stats.totalQSOs == 0) {
    // No data message
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(40, 120);
    tft.print("No QSO data");

    // Footer
    tft.fillRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, COLOR_BACKGROUND);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(10, SCREEN_HEIGHT - 16);
    tft.print("ESC Back");
    return;
  }

  // Statistics cards layout
  int y = 50;
  int cardHeight = 35;
  int cardSpacing = 5;

  tft.setTextSize(1);

  // Card 1: Total QSOs
  tft.fillRoundRect(10, y, 145, cardHeight, 6, 0x1082);
  tft.drawRoundRect(10, y, 145, cardHeight, 6, ST77XX_CYAN);
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(15, y + 5);
  tft.print("Total QSOs");
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, y + 16);
  tft.print(stats.totalQSOs);
  tft.setTextSize(1);
  y += cardHeight + cardSpacing;

  // Card 2: Unique Callsigns
  tft.fillRoundRect(10, y, 145, cardHeight, 6, 0x1082);
  tft.drawRoundRect(10, y, 145, cardHeight, 6, 0x39C7);
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(15, y + 5);
  tft.print("Unique Calls");
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, y + 16);
  tft.print(stats.uniqueCallsigns);
  tft.setTextSize(1);

  // Card 3: Most Active Date (right column)
  y = 50;
  if (strlen(stats.mostActiveDate) > 0) {
    tft.fillRoundRect(165, y, 145, cardHeight, 6, 0x1082);
    tft.drawRoundRect(165, y, 145, cardHeight, 6, 0x39C7);
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(170, y + 5);
    tft.print("Most Active");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(170, y + 16);
    // Format date YYYYMMDD to MM/DD/YY
    if (strlen(stats.mostActiveDate) >= 8) {
      tft.print(stats.mostActiveDate[4]);
      tft.print(stats.mostActiveDate[5]);
      tft.print("/");
      tft.print(stats.mostActiveDate[6]);
      tft.print(stats.mostActiveDate[7]);
      tft.print("/");
      tft.print(stats.mostActiveDate[2]);
      tft.print(stats.mostActiveDate[3]);
    }
    tft.print(" (");
    tft.print(stats.mostActiveDateCount);
    tft.print(")");
  }
  y += cardHeight + cardSpacing;

  // Card 4: Last QSO Date (right column)
  if (strlen(stats.lastQSODate) > 0) {
    tft.fillRoundRect(165, y, 145, cardHeight, 6, 0x1082);
    tft.drawRoundRect(165, y, 145, cardHeight, 6, 0x39C7);
    tft.setTextColor(COLOR_WARNING);
    tft.setCursor(170, y + 5);
    tft.print("Last QSO");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(170, y + 16);
    // Format date YYYYMMDD to MM/DD/YY
    if (strlen(stats.lastQSODate) >= 8) {
      tft.print(stats.lastQSODate[4]);
      tft.print(stats.lastQSODate[5]);
      tft.print("/");
      tft.print(stats.lastQSODate[6]);
      tft.print(stats.lastQSODate[7]);
      tft.print("/");
      tft.print(stats.lastQSODate[2]);
      tft.print(stats.lastQSODate[3]);
    }
  }

  // Band breakdown section
  y = 130;
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, y);
  tft.print("Bands:");
  y += 12;

  tft.setTextColor(ST77XX_WHITE);
  for (int i = 0; i < min(4, stats.bandCount); i++) {
    tft.setCursor(15, y);
    tft.print(stats.bandStats[i].band);
    tft.print(": ");
    tft.print(stats.bandStats[i].count);

    // Draw bar graph
    int barWidth = (stats.bandStats[i].count * 80) / stats.totalQSOs;
    if (barWidth < 2 && stats.bandStats[i].count > 0) barWidth = 2;
    tft.fillRect(80, y + 1, barWidth, 6, ST77XX_CYAN);

    y += 10;
  }

  // Mode breakdown section
  y = 130;
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(170, y);
  tft.print("Modes:");
  y += 12;

  tft.setTextColor(ST77XX_WHITE);
  for (int i = 0; i < min(4, stats.modeCount); i++) {
    tft.setCursor(175, y);
    tft.print(stats.modeStats[i].mode);
    tft.print(": ");
    tft.print(stats.modeStats[i].count);

    // Draw bar graph
    int barWidth = (stats.modeStats[i].count * 50) / stats.totalQSOs;
    if (barWidth < 2 && stats.modeStats[i].count > 0) barWidth = 2;
    tft.fillRect(240, y + 1, barWidth, 6, ST77XX_GREEN);

    y += 10;
  }

  // Footer
  tft.fillRect(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, COLOR_BACKGROUND);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_WARNING);
  tft.setCursor(10, SCREEN_HEIGHT - 16);
  tft.print("ESC Back");
}

// ============================================
// Input Handler
// ============================================

int handleStatisticsInput(char key, Adafruit_ST7789& tft) {
  if (key == 0x1B) { // ESC
    return -1; // Exit to menu
  }

  return 0; // No action
}

// ============================================
// Initialization
// ============================================

void startStatistics(Adafruit_ST7789& tft) {
  Serial.println("Starting Statistics mode");

  // Calculate statistics from saved logs
  calculateStatistics();

  // Draw UI
  drawStatisticsUI(tft);
}

#endif // QSO_LOGGER_STATISTICS_H
