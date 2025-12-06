/*
 * SD Card Storage Management
 * Handles SD card initialization and file operations
 */

#ifndef SD_CARD_H
#define SD_CARD_H

#include <SD.h>
#include <SPI.h>
#include "../core/config.h"

// SD card state
bool sdCardAvailable = false;
uint64_t sdCardSize = 0;
uint64_t sdCardUsed = 0;

// Initialize SD card
bool initSDCard() {
  Serial.println("Initializing SD card...");

  // Set CS pin high (inactive) before initializing to avoid conflicts
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  delay(10);

  // Initialize SD card with CS pin and explicit SPI settings
  // Use SPI2_HOST (same as display) at 4 MHz (safe speed for SD cards)
  // The display's bus_shared=true setting allows this to work
  if (!SD.begin(SD_CS, SPI, 4000000, "/sd", 5, false)) {
    Serial.println("SD card initialization failed (or no card inserted)");
    sdCardAvailable = false;
    return false;
  }

  Serial.println("SD card initialized successfully");
  sdCardAvailable = true;

  // Get card info
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    sdCardAvailable = false;
    return false;
  }

  // Print card type
  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  // Get card size
  sdCardSize = SD.cardSize() / (1024 * 1024); // Convert to MB
  sdCardUsed = SD.usedBytes() / (1024 * 1024); // Convert to MB

  Serial.printf("SD Card Size: %llu MB\n", sdCardSize);
  Serial.printf("SD Card Used: %llu MB\n", sdCardUsed);

  return true;
}

// Update SD card usage stats
void updateSDCardStats() {
  if (!sdCardAvailable) return;

  sdCardSize = SD.cardSize() / (1024 * 1024);
  sdCardUsed = SD.usedBytes() / (1024 * 1024);
}

// List files in a directory (recursive option)
String listSDFiles(const char* dirname, bool recursive = false, int depth = 0) {
  if (!sdCardAvailable) {
    return "[]";
  }

  String fileList = "[";
  File root = SD.open(dirname);

  if (!root) {
    Serial.println("Failed to open directory");
    return "[]";
  }

  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return "[]";
  }

  File file = root.openNextFile();
  bool firstFile = true;

  while (file) {
    if (!firstFile) {
      fileList += ",";
    }
    firstFile = false;

    fileList += "{";
    fileList += "\"name\":\"" + String(file.name()) + "\",";
    fileList += "\"size\":" + String(file.size()) + ",";
    fileList += "\"isDir\":" + String(file.isDirectory() ? "true" : "false");
    fileList += "}";

    // Recursive listing if enabled
    if (recursive && file.isDirectory() && depth < 3) {
      // Note: Recursive listing can be added here if needed
    }

    file = root.openNextFile();
  }

  fileList += "]";
  return fileList;
}

// Delete a file
bool deleteSDFile(const char* path) {
  if (!sdCardAvailable) return false;

  return SD.remove(path);
}

// Check if file exists
bool fileExists(const char* path) {
  if (!sdCardAvailable) return false;

  return SD.exists(path);
}

// Get file size
size_t getFileSize(const char* path) {
  if (!sdCardAvailable) return 0;

  File file = SD.open(path);
  if (!file) return 0;

  size_t size = file.size();
  file.close();
  return size;
}

// Read file contents (for small files)
String readSDFile(const char* path) {
  if (!sdCardAvailable) return "";

  File file = SD.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "";
  }

  String contents = "";
  while (file.available()) {
    contents += (char)file.read();
  }

  file.close();
  return contents;
}

// Write file contents
bool writeSDFile(const char* path, const char* data) {
  if (!sdCardAvailable) return false;

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return false;
  }

  size_t bytesWritten = file.print(data);
  file.close();

  return bytesWritten > 0;
}

// Append to file
bool appendSDFile(const char* path, const char* data) {
  if (!sdCardAvailable) return false;

  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return false;
  }

  size_t bytesWritten = file.print(data);
  file.close();

  return bytesWritten > 0;
}

// Create a directory on SD card
bool createSDDirectory(const char* path) {
  if (!sdCardAvailable) return false;

  if (SD.exists(path)) {
    return true;  // Directory already exists
  }

  if (SD.mkdir(path)) {
    Serial.print("Created directory: ");
    Serial.println(path);
    return true;
  }

  Serial.print("Failed to create directory: ");
  Serial.println(path);
  return false;
}

#endif // SD_CARD_H
