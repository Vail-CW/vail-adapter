/*
 * Ham Radio License Study - Question Pool Downloader
 * Automatically downloads JSON files from GitHub when missing
 */

#ifndef TRAINING_LICENSE_DOWNLOADER_H
#define TRAINING_LICENSE_DOWNLOADER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include "../core/config.h"
#include "../storage/sd_card.h"
#include "../audio/i2s_audio.h"  // For beep() function

// Forward declaration - avoid circular dependency
void drawLicenseSDCardError(LGFX& tft);

// GitHub raw URLs for question pool JSON files
#define QUESTION_POOL_BASE_URL "https://raw.githubusercontent.com/russolsen/ham_radio_question_pool/master"
#define TECHNICIAN_URL QUESTION_POOL_BASE_URL "/technician-2022-2026/technician.json"
#define GENERAL_URL QUESTION_POOL_BASE_URL "/general-2023-2027/general.json"
#define EXTRA_URL QUESTION_POOL_BASE_URL "/extra-2024-2028/extra.json"

// Download status
enum DownloadStatus {
  DOWNLOAD_SUCCESS,
  DOWNLOAD_FAILED_NO_WIFI,
  DOWNLOAD_FAILED_HTTP,
  DOWNLOAD_FAILED_SD_CARD,
  DOWNLOAD_FAILED_WRITE
};

/**
 * Check if a question pool file exists on SD card
 */
bool questionFileExists(const char* filename) {
  if (!sdCardAvailable) return false;
  return SD.exists(filename);
}

/**
 * Check if all question pool files exist
 */
bool allQuestionFilesExist() {
  return questionFileExists("/license/technician.json") &&
         questionFileExists("/license/general.json") &&
         questionFileExists("/license/extra.json");
}

/**
 * Download a file from URL to SD card
 */
DownloadStatus downloadFile(const char* url, const char* filepath) {
  HTTPClient http;

  Serial.print("Downloading: ");
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int len = http.getSize();

    Serial.print("File size: ");
    Serial.print(len);
    Serial.println(" bytes");

    // Open file for writing
    File file = SD.open(filepath, FILE_WRITE);
    if (!file) {
      Serial.println("ERROR: Failed to open file for writing");
      http.end();
      return DOWNLOAD_FAILED_WRITE;
    }

    // Download in chunks
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[1024];
    int bytesRead = 0;
    int totalRead = 0;

    while (http.connected() && (len > 0 || len == -1)) {
      size_t size = stream->available();

      if (size) {
        int c = stream->readBytes(buffer, ((size > sizeof(buffer)) ? sizeof(buffer) : size));
        file.write(buffer, c);
        totalRead += c;

        if (len > 0) {
          len -= c;
        }

        // Print progress every 10KB
        if (totalRead - bytesRead >= 10240) {
          Serial.print("Downloaded: ");
          Serial.print(totalRead / 1024);
          Serial.println(" KB");
          bytesRead = totalRead;
        }
      }
      delay(1);
    }

    file.close();
    http.end();

    Serial.print("Download complete: ");
    Serial.print(totalRead);
    Serial.println(" bytes");

    return DOWNLOAD_SUCCESS;
  } else {
    Serial.print("HTTP error code: ");
    Serial.println(httpCode);
    http.end();
    return DOWNLOAD_FAILED_HTTP;
  }
}

/**
 * Show WiFi required error screen
 */
void drawWiFiRequiredScreen(LGFX& tft) {
  tft.fillScreen(COLOR_BG_DEEP);

  // Header
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_LAYER2);
  tft.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BORDER_SUBTLE);

  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT_PRIMARY);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds_compat(tft, "LICENSE STUDY", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 30);
  tft.print("LICENSE STUDY");

  // Error message
  tft.setTextColor(COLOR_ERROR_PASTEL);
  getTextBounds_compat(tft, "WiFi Required", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 110);
  tft.print("WiFi Required");

  // Instructions
  tft.setFont(&FreeSansBold9pt7b);
  tft.setTextColor(COLOR_TEXT_SECONDARY);

  String line1 = "Question files need to be downloaded.";
  getTextBounds_compat(tft, line1.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 150);
  tft.print(line1);

  String line2 = "Please connect to WiFi first:";
  getTextBounds_compat(tft, line2.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 175);
  tft.print(line2);

  String line3 = "Settings > WiFi Setup";
  getTextBounds_compat(tft, line3.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, 200);
  tft.print(line3);

  // Footer
  String footer = "ESC: Back";
  getTextBounds_compat(tft, footer.c_str(), 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_WIDTH - w) / 2, SCREEN_HEIGHT - 25);
  tft.print(footer);

  tft.setFont(nullptr);

  // Wait for ESC key
  while (true) {
    char key = 0;
    Wire.requestFrom(CARDKB_ADDR, 1);
    if (Wire.available()) {
      key = Wire.read();
    }
    if (key == KEY_ESC) {
      beep(TONE_MENU_NAV, BEEP_SHORT);
      break;
    }
    delay(50);
  }
}

/**
 * Download all missing question pool files
 * Returns true if all files are available (existed or downloaded)
 */
bool ensureQuestionFilesExist(LGFX& tft, bool showProgress = true) {
  // Check if all files already exist
  if (allQuestionFilesExist()) {
    Serial.println("All question pool files found on SD card");
    return true;
  }

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("ERROR: WiFi not connected, cannot download question files");
    if (showProgress) {
      drawWiFiRequiredScreen(tft);
    }
    return false;
  }

  // Check SD card
  if (!sdCardAvailable) {
    Serial.println("ERROR: SD card not available");
    if (showProgress) {
      drawLicenseSDCardError(tft);
    }
    return false;
  }

  // Create /license directory if it doesn't exist
  if (!SD.exists("/license")) {
    Serial.println("Creating /license directory...");
    if (!SD.mkdir("/license")) {
      Serial.println("ERROR: Failed to create directory");
      return false;
    }
  }

  // Show download screen if requested
  if (showProgress) {
    tft.fillScreen(COLOR_BG_DEEP);

    // Header
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG_LAYER2);
    tft.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BORDER_SUBTLE);

    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_TEXT_PRIMARY);

    int16_t x1, y1;
    uint16_t w, h;
    String title = "LICENSE STUDY";
    getTextBounds_compat(tft, title.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 30);
    tft.print(title);

    // Status message
    tft.setTextColor(COLOR_ACCENT_CYAN);
    String msg = "Downloading Question Files...";
    getTextBounds_compat(tft, msg.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 110);
    tft.print(msg);

    tft.setFont(&FreeSansBold9pt7b);
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    String info = "This will take a minute...";
    getTextBounds_compat(tft, info.c_str(), 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((SCREEN_WIDTH - w) / 2, 140);
    tft.print(info);

    tft.setFont(nullptr);
  }

  int yPos = 170;
  bool allSuccess = true;

  // Download Technician pool
  if (!questionFileExists("/license/technician.json")) {
    if (showProgress) {
      tft.setFont(&FreeSansBold9pt7b);
      tft.setTextColor(COLOR_TEXT_PRIMARY);
      tft.setCursor(40, yPos);
      tft.print("Technician... ");
      tft.setFont(nullptr);
    }

    DownloadStatus status = downloadFile(TECHNICIAN_URL, "/license/technician.json");

    if (showProgress) {
      tft.setFont(&FreeSansBold9pt7b);
      if (status == DOWNLOAD_SUCCESS) {
        tft.setTextColor(COLOR_SUCCESS_PASTEL);
        tft.print("OK");
      } else {
        tft.setTextColor(COLOR_ERROR_PASTEL);
        tft.print("FAILED");
        allSuccess = false;
      }
      tft.setFont(nullptr);
    }

    yPos += 25;
  }

  // Download General pool
  if (!questionFileExists("/license/general.json")) {
    if (showProgress) {
      tft.setFont(&FreeSansBold9pt7b);
      tft.setTextColor(COLOR_TEXT_PRIMARY);
      tft.setCursor(40, yPos);
      tft.print("General... ");
      tft.setFont(nullptr);
    }

    DownloadStatus status = downloadFile(GENERAL_URL, "/license/general.json");

    if (showProgress) {
      tft.setFont(&FreeSansBold9pt7b);
      if (status == DOWNLOAD_SUCCESS) {
        tft.setTextColor(COLOR_SUCCESS_PASTEL);
        tft.print("OK");
      } else {
        tft.setTextColor(COLOR_ERROR_PASTEL);
        tft.print("FAILED");
        allSuccess = false;
      }
      tft.setFont(nullptr);
    }

    yPos += 25;
  }

  // Download Extra pool
  if (!questionFileExists("/license/extra.json")) {
    if (showProgress) {
      tft.setFont(&FreeSansBold9pt7b);
      tft.setTextColor(COLOR_TEXT_PRIMARY);
      tft.setCursor(40, yPos);
      tft.print("Extra... ");
      tft.setFont(nullptr);
    }

    DownloadStatus status = downloadFile(EXTRA_URL, "/license/extra.json");

    if (showProgress) {
      tft.setFont(&FreeSansBold9pt7b);
      if (status == DOWNLOAD_SUCCESS) {
        tft.setTextColor(COLOR_SUCCESS_PASTEL);
        tft.print("OK");
      } else {
        tft.setTextColor(COLOR_ERROR_PASTEL);
        tft.print("FAILED");
        allSuccess = false;
      }
      tft.setFont(nullptr);
    }

    yPos += 25;
  }

  // Show completion message
  if (showProgress) {
    yPos += 20;
    tft.setFont(&FreeSansBold9pt7b);

    if (allSuccess) {
      tft.setTextColor(COLOR_SUCCESS_PASTEL);
      String msg = "Download Complete!";
      int16_t x1, y1;
      uint16_t w, h;
      getTextBounds_compat(tft, msg.c_str(), 0, 0, &x1, &y1, &w, &h);
      tft.setCursor((SCREEN_WIDTH - w) / 2, yPos);
      tft.print(msg);

      yPos += 30;
      tft.setTextColor(COLOR_TEXT_SECONDARY);
      String info = "Press any key or wait 3 seconds...";
      getTextBounds_compat(tft, info.c_str(), 0, 0, &x1, &y1, &w, &h);
      tft.setCursor((SCREEN_WIDTH - w) / 2, yPos);
      tft.print(info);

      // Wait for keypress or 3 second timeout
      unsigned long startTime = millis();
      while (millis() - startTime < 3000) {
        char key = 0;
        Wire.requestFrom(CARDKB_ADDR, 1);
        if (Wire.available()) {
          key = Wire.read();
        }
        if (key != 0) {
          beep(TONE_SELECT, BEEP_SHORT);
          break;
        }
        delay(50);
      }
    } else {
      tft.setTextColor(COLOR_ERROR_PASTEL);
      String msg = "Some downloads failed";
      int16_t x1, y1;
      uint16_t w, h;
      getTextBounds_compat(tft, msg.c_str(), 0, 0, &x1, &y1, &w, &h);
      tft.setCursor((SCREEN_WIDTH - w) / 2, yPos);
      tft.print(msg);

      yPos += 30;
      tft.setTextColor(COLOR_TEXT_SECONDARY);
      String info = "Check WiFi and try again";
      getTextBounds_compat(tft, info.c_str(), 0, 0, &x1, &y1, &w, &h);
      tft.setCursor((SCREEN_WIDTH - w) / 2, yPos);
      tft.print(info);

      delay(3000);
    }

    tft.setFont(nullptr);
  }

  return allSuccess;
}

#endif // TRAINING_LICENSE_DOWNLOADER_H
