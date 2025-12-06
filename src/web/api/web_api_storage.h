/*
 * Web API - Storage Management Endpoints
 * REST API for SD card file operations
 */

#ifndef WEB_API_STORAGE_H
#define WEB_API_STORAGE_H

#include <ESPAsyncWebServer.h>
#include <SD.h>
#include "../../storage/sd_card.h"

// Get SD card status
void handleGetStorageStatus(AsyncWebServerRequest *request) {
  // Initialize SD card on first access if not already done
  if (!sdCardAvailable) {
    initSDCard();
  }

  updateSDCardStats();

  String json = "{";
  json += "\"available\":" + String(sdCardAvailable ? "true" : "false") + ",";
  json += "\"totalMB\":" + String((unsigned long)sdCardSize) + ",";
  json += "\"usedMB\":" + String((unsigned long)sdCardUsed) + ",";
  json += "\"freeMB\":" + String((unsigned long)(sdCardSize - sdCardUsed));
  json += "}";

  request->send(200, "application/json", json);
}

// List files in SD card root or specific directory
void handleListFiles(AsyncWebServerRequest *request) {
  if (!sdCardAvailable) {
    request->send(503, "application/json", "{\"error\":\"SD card not available\"}");
    return;
  }

  String path = "/";
  if (request->hasParam("path")) {
    path = request->getParam("path")->value();
  }

  String fileList = listSDFiles(path.c_str());
  request->send(200, "application/json", fileList);
}

// Download file from SD card
void handleDownloadFile(AsyncWebServerRequest *request) {
  if (!sdCardAvailable) {
    request->send(503, "text/plain", "SD card not available");
    return;
  }

  if (!request->hasParam("file")) {
    request->send(400, "text/plain", "Missing file parameter");
    return;
  }

  String filepath = request->getParam("file")->value();

  if (!fileExists(filepath.c_str())) {
    request->send(404, "text/plain", "File not found");
    return;
  }

  // Send file for download
  request->send(SD, filepath, "application/octet-stream", true);
}

// Delete file from SD card
void handleDeleteFile(AsyncWebServerRequest *request) {
  if (!sdCardAvailable) {
    request->send(503, "application/json", "{\"success\":false,\"error\":\"SD card not available\"}");
    return;
  }

  if (!request->hasParam("file")) {
    request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing file parameter\"}");
    return;
  }

  String filepath = request->getParam("file")->value();

  if (deleteSDFile(filepath.c_str())) {
    updateSDCardStats();
    request->send(200, "application/json", "{\"success\":true}");
  } else {
    request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to delete file\"}");
  }
}

// Upload file to SD card
void handleUploadFile(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  static File uploadFile;

  if (!sdCardAvailable) {
    request->send(503, "application/json", "{\"success\":false,\"error\":\"SD card not available\"}");
    return;
  }

  // Start of upload
  if (index == 0) {
    Serial.printf("Upload Start: %s\n", filename.c_str());

    // Ensure filename starts with /
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }

    // Open file for writing
    uploadFile = SD.open(filename, FILE_WRITE);
    if (!uploadFile) {
      Serial.println("Failed to open file for upload");
      request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to create file\"}");
      return;
    }
  }

  // Write chunk to file
  if (uploadFile) {
    uploadFile.write(data, len);
  }

  // End of upload
  if (final) {
    if (uploadFile) {
      uploadFile.close();
    }
    Serial.printf("Upload Complete: %s (%u bytes)\n", filename.c_str(), index + len);
    updateSDCardStats();
    request->send(200, "application/json", "{\"success\":true,\"file\":\"" + filename + "\"}");
  }
}

// Register all storage API endpoints
void registerStorageAPI(AsyncWebServer* server) {
  // Get storage status
  server->on("/api/storage/status", HTTP_GET, handleGetStorageStatus);

  // List files
  server->on("/api/storage/files", HTTP_GET, handleListFiles);

  // Download file
  server->on("/api/storage/download", HTTP_GET, handleDownloadFile);

  // Delete file
  server->on("/api/storage/delete", HTTP_DELETE, handleDeleteFile);

  // Upload file
  server->on("/api/storage/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      // This is called after upload completes
    },
    handleUploadFile
  );
}

#endif // WEB_API_STORAGE_H
