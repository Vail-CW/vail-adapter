/*
 * Web File Downloader
 * Downloads web interface files from GitHub to SD card
 */

#ifndef WEB_FILE_DOWNLOADER_H
#define WEB_FILE_DOWNLOADER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include "../../core/config.h"
#include "../../storage/sd_card.h"

// ============================================
// Download State
// ============================================

enum WebDownloadState {
  DOWNLOAD_IDLE,
  DOWNLOAD_FETCHING_MANIFEST,
  DOWNLOAD_IN_PROGRESS,
  DOWNLOAD_COMPLETE,
  DOWNLOAD_ERROR
};

// Download progress tracking
struct WebDownloadProgress {
  WebDownloadState state;
  int totalFiles;
  int currentFile;
  int currentFileBytes;
  int currentFileTotal;
  String currentFileName;
  String errorMessage;
  bool cancelled;
};

WebDownloadProgress webDownloadProgress = {
  DOWNLOAD_IDLE, 0, 0, 0, 0, "", "", false
};

// ============================================
// Helper Functions
// ============================================

/**
 * Create directories recursively for a path
 * @param path Full path (e.g., "/www/css/styles.css")
 */
void createDirectoriesForPath(const char* path) {
  String pathStr = path;
  int lastSlash = pathStr.lastIndexOf('/');
  if (lastSlash <= 0) return;  // No subdirectories needed

  String dirPath = pathStr.substring(0, lastSlash);

  // Create each directory level
  int start = 1;  // Skip leading slash
  while (true) {
    int nextSlash = dirPath.indexOf('/', start);
    if (nextSlash == -1) {
      // Create final directory
      if (!SD.exists(dirPath.c_str())) {
        SD.mkdir(dirPath.c_str());
        Serial.printf("Created directory: %s\n", dirPath.c_str());
      }
      break;
    }

    String subDir = dirPath.substring(0, nextSlash);
    if (!SD.exists(subDir.c_str())) {
      SD.mkdir(subDir.c_str());
      Serial.printf("Created directory: %s\n", subDir.c_str());
    }
    start = nextSlash + 1;
  }
}

/**
 * Download a single file from URL to SD card
 * @param url Full URL to download
 * @param sdPath Path on SD card to save file
 * @return true if successful
 */
bool downloadFileToSD(const char* url, const char* sdPath) {
  Serial.printf("Downloading: %s -> %s\n", url, sdPath);

  // Create directories if needed
  createDirectoriesForPath(sdPath);

  HTTPClient http;
  http.begin(url);
  http.setTimeout(30000);  // 30 second timeout for larger files

  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.printf("HTTP error %d for %s\n", httpCode, url);
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  webDownloadProgress.currentFileTotal = contentLength;
  webDownloadProgress.currentFileBytes = 0;

  // Open file for writing
  File file = SD.open(sdPath, FILE_WRITE);
  if (!file) {
    Serial.printf("Failed to create file: %s\n", sdPath);
    http.end();
    return false;
  }

  // Get stream and download in chunks
  WiFiClient* stream = http.getStreamPtr();
  uint8_t buffer[512];
  int bytesRead = 0;

  while (http.connected() && (contentLength > 0 || contentLength == -1)) {
    size_t available = stream->available();
    if (available) {
      int toRead = min((int)available, (int)sizeof(buffer));
      int c = stream->readBytes(buffer, toRead);
      file.write(buffer, c);

      bytesRead += c;
      webDownloadProgress.currentFileBytes = bytesRead;

      if (contentLength > 0) {
        contentLength -= c;
      }
    }

    // Check for cancellation
    if (webDownloadProgress.cancelled) {
      Serial.println("Download cancelled by user");
      file.close();
      SD.remove(sdPath);  // Clean up partial file
      http.end();
      return false;
    }

    yield();  // Allow other tasks to run
  }

  file.close();
  http.end();

  Serial.printf("Downloaded %d bytes to %s\n", bytesRead, sdPath);
  return true;
}

// ============================================
// Main Download Functions
// ============================================

/**
 * Download web files from GitHub using manifest
 * @return true if all files downloaded successfully
 */
bool downloadWebFilesFromGitHub() {
  // Reset progress
  webDownloadProgress.state = DOWNLOAD_FETCHING_MANIFEST;
  webDownloadProgress.totalFiles = 0;
  webDownloadProgress.currentFile = 0;
  webDownloadProgress.currentFileName = "manifest.json";
  webDownloadProgress.errorMessage = "";
  webDownloadProgress.cancelled = false;

  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    webDownloadProgress.state = DOWNLOAD_ERROR;
    webDownloadProgress.errorMessage = "No WiFi connection";
    return false;
  }

  // Check SD card
  if (!sdCardAvailable) {
    if (!initSDCard()) {
      webDownloadProgress.state = DOWNLOAD_ERROR;
      webDownloadProgress.errorMessage = "SD card not available";
      return false;
    }
  }

  // Create /www directory if it doesn't exist
  if (!SD.exists(WEB_FILES_PATH)) {
    SD.mkdir(WEB_FILES_PATH);
    Serial.printf("Created directory: %s\n", WEB_FILES_PATH);
  }

  // Build manifest URL
  String manifestUrl = String(WEB_FILES_BASE_URL) + WEB_FILES_MANIFEST;
  Serial.printf("Fetching manifest: %s\n", manifestUrl.c_str());

  HTTPClient http;
  http.begin(manifestUrl);
  http.setTimeout(10000);

  int httpCode = http.GET();

  if (httpCode != 200) {
    webDownloadProgress.state = DOWNLOAD_ERROR;
    webDownloadProgress.errorMessage = "Failed to fetch manifest (HTTP " + String(httpCode) + ")";
    http.end();
    return false;
  }

  String manifestJson = http.getString();
  http.end();

  // Parse manifest
  StaticJsonDocument<4096> doc;
  DeserializationError error = deserializeJson(doc, manifestJson);

  if (error) {
    webDownloadProgress.state = DOWNLOAD_ERROR;
    webDownloadProgress.errorMessage = "Failed to parse manifest: " + String(error.c_str());
    return false;
  }

  // Get file list from manifest
  JsonArray files = doc["files"].as<JsonArray>();
  webDownloadProgress.totalFiles = files.size();
  webDownloadProgress.state = DOWNLOAD_IN_PROGRESS;

  Serial.printf("Manifest contains %d files\n", webDownloadProgress.totalFiles);

  // Download each file
  int fileIndex = 0;
  for (JsonObject fileObj : files) {
    if (webDownloadProgress.cancelled) {
      webDownloadProgress.state = DOWNLOAD_ERROR;
      webDownloadProgress.errorMessage = "Download cancelled";
      return false;
    }

    const char* fileName = fileObj["name"];
    webDownloadProgress.currentFile = fileIndex + 1;
    webDownloadProgress.currentFileName = fileName;

    // Build URLs/paths
    String fileUrl = String(WEB_FILES_BASE_URL) + fileName;
    String sdPath = String(WEB_FILES_PATH) + fileName;

    if (!downloadFileToSD(fileUrl.c_str(), sdPath.c_str())) {
      webDownloadProgress.state = DOWNLOAD_ERROR;
      webDownloadProgress.errorMessage = "Failed to download: " + String(fileName);
      return false;
    }

    fileIndex++;
  }

  // Save manifest version info
  String versionPath = String(WEB_FILES_PATH) + "version.txt";
  const char* version = doc["version"] | "unknown";
  writeSDFile(versionPath.c_str(), version);

  webDownloadProgress.state = DOWNLOAD_COMPLETE;
  Serial.println("Web files download complete!");

  // Update SD card stats
  updateSDCardStats();

  return true;
}

/**
 * Cancel ongoing download
 */
void cancelWebFileDownload() {
  webDownloadProgress.cancelled = true;
}

/**
 * Get current download progress as JSON
 */
String getWebDownloadProgressJson() {
  StaticJsonDocument<512> doc;

  doc["state"] = (int)webDownloadProgress.state;
  doc["totalFiles"] = webDownloadProgress.totalFiles;
  doc["currentFile"] = webDownloadProgress.currentFile;
  doc["currentFileName"] = webDownloadProgress.currentFileName;
  doc["currentFileBytes"] = webDownloadProgress.currentFileBytes;
  doc["currentFileTotal"] = webDownloadProgress.currentFileTotal;
  doc["error"] = webDownloadProgress.errorMessage;

  // Calculate overall progress percentage
  int overallProgress = 0;
  if (webDownloadProgress.totalFiles > 0) {
    overallProgress = (webDownloadProgress.currentFile * 100) / webDownloadProgress.totalFiles;
  }
  doc["overallProgress"] = overallProgress;

  String output;
  serializeJson(doc, output);
  return output;
}

/**
 * Check if web files exist on SD card
 * @return true if /www/index.html exists
 */
bool webFilesExist() {
  if (!sdCardAvailable) {
    // Try to init SD card first
    if (!initSDCard()) {
      return false;
    }
  }

  String indexPath = String(WEB_FILES_PATH) + "index.html";
  return SD.exists(indexPath.c_str());
}

/**
 * Get web files version from SD card
 * @return Version string or empty if not found
 */
String getWebFilesVersion() {
  if (!sdCardAvailable) return "";

  String versionPath = String(WEB_FILES_PATH) + "version.txt";
  if (!SD.exists(versionPath.c_str())) return "";

  return readSDFile(versionPath.c_str());
}

/**
 * Delete all web files from SD card
 * @return true if successful
 */
bool deleteWebFiles() {
  if (!sdCardAvailable) return false;

  // List and delete all files in /www/
  File root = SD.open(WEB_FILES_PATH);
  if (!root || !root.isDirectory()) {
    return false;
  }

  File file = root.openNextFile();
  while (file) {
    String path = String(WEB_FILES_PATH) + file.name();
    if (file.isDirectory()) {
      // Recursive delete for subdirectories
      File subdir = SD.open(path.c_str());
      if (subdir && subdir.isDirectory()) {
        File subfile = subdir.openNextFile();
        while (subfile) {
          String subpath = path + "/" + subfile.name();
          SD.remove(subpath.c_str());
          subfile = subdir.openNextFile();
        }
        subdir.close();
        SD.rmdir(path.c_str());
      }
    } else {
      SD.remove(path.c_str());
    }
    file = root.openNextFile();
  }
  root.close();

  // Remove the /www/ directory itself
  SD.rmdir(WEB_FILES_PATH);

  return true;
}

#endif // WEB_FILE_DOWNLOADER_H
