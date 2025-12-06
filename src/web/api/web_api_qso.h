/*
 * web_api_qso.h
 *
 * QSO Logger API Endpoints for VAIL SUMMIT
 *
 * Provides REST API endpoints for:
 * - Station settings (callsign, grid square, POTA)
 * - QSO CRUD operations (create, update, delete)
 *
 * Extracted from web_server.h for modularity
 */

#ifndef WEB_API_QSO_H
#define WEB_API_QSO_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SD.h>
#include "../../qso/qso_logger.h"
#include "../../storage/sd_card.h"

// External declarations for functions from other modules
extern bool saveQSO(const QSO& qso);                    // From qso_logger_storage.h
extern bool isQSOStorageReady();                        // From qso_logger_storage.h
extern void regenerateADIFFiles(const char* date);      // From qso_logger_storage.h
extern String frequencyToBand(float freq);              // From qso_logger_validation.h
extern String formatCurrentDateTime();                  // From qso_logger_validation.h
extern bool checkWebAuth(AsyncWebServerRequest *request); // From web_server.h

// QSO directory on SD card
#define QSO_DIR "/qso"

/*
 * Setup all QSO-related API endpoints
 * Call this from setupWebServer() in web_server.h
 */
void setupQSOAPI(AsyncWebServer &webServer) {

  // ============================================
  // Station Settings Endpoints
  // ============================================

  // Get station settings
  webServer.on("/api/settings/station", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    JsonDocument doc;

    Preferences prefs;
    prefs.begin("qso_operator", true);

    char callsign[11] = "";
    char gridsquare[9] = "";
    char pota[11] = "";

    prefs.getString("callsign", callsign, sizeof(callsign));
    prefs.getString("gridsquare", gridsquare, sizeof(gridsquare));
    prefs.getString("pota", pota, sizeof(pota));

    prefs.end();

    doc["callsign"] = callsign;
    doc["gridsquare"] = gridsquare;
    doc["pota"] = pota;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Save station settings
  webServer.on("/api/settings/station", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (!checkWebAuth(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!checkWebAuth(request)) return;

      // Parse JSON body
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Save to Preferences
      Preferences prefs;
      prefs.begin("qso_operator", false);

      if (doc.containsKey("callsign")) {
        prefs.putString("callsign", doc["callsign"].as<String>());
      }
      if (doc.containsKey("gridsquare")) {
        prefs.putString("gridsquare", doc["gridsquare"].as<String>());
      }
      if (doc.containsKey("pota")) {
        prefs.putString("pota", doc["pota"].as<String>());
      }

      prefs.end();

      Serial.println("Station settings saved via web interface");

      request->send(200, "application/json", "{\"success\":true}");
    });

  // ============================================
  // QSO CRUD Endpoints
  // ============================================

  // Create new QSO
  webServer.on("/api/qsos/create", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (!checkWebAuth(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!checkWebAuth(request)) return;

      // Check SD card availability
      if (!sdCardAvailable) {
        request->send(503, "application/json", "{\"success\":false,\"error\":\"SD card required for QSO logging\"}");
        return;
      }

      // Parse JSON body
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Extract QSO data - initialize struct to zero first!
      QSO newQSO;
      memset(&newQSO, 0, sizeof(QSO));

      newQSO.id = doc["id"] | (unsigned long)millis();
      strlcpy(newQSO.callsign, doc["callsign"] | "", sizeof(newQSO.callsign));
      newQSO.frequency = doc["frequency"] | 0.0f;
      strlcpy(newQSO.mode, doc["mode"] | "", sizeof(newQSO.mode));

      // Calculate band from frequency
      String band = frequencyToBand(newQSO.frequency);
      strlcpy(newQSO.band, band.c_str(), sizeof(newQSO.band));

      strlcpy(newQSO.rst_sent, doc["rst_sent"] | "", sizeof(newQSO.rst_sent));
      strlcpy(newQSO.rst_rcvd, doc["rst_rcvd"] | "", sizeof(newQSO.rst_rcvd));
      strlcpy(newQSO.date, doc["date"] | "", sizeof(newQSO.date));
      strlcpy(newQSO.time_on, doc["time_on"] | "", sizeof(newQSO.time_on));
      strlcpy(newQSO.gridsquare, doc["gridsquare"] | "", sizeof(newQSO.gridsquare));
      strlcpy(newQSO.my_gridsquare, doc["my_gridsquare"] | "", sizeof(newQSO.my_gridsquare));
      strlcpy(newQSO.my_pota_ref, doc["my_pota_ref"] | "", sizeof(newQSO.my_pota_ref));
      strlcpy(newQSO.their_pota_ref, doc["their_pota_ref"] | "", sizeof(newQSO.their_pota_ref));
      strlcpy(newQSO.notes, doc["notes"] | "", sizeof(newQSO.notes));

      // If no date provided, use current date
      if (strlen(newQSO.date) == 0) {
        String dateTime = formatCurrentDateTime();
        strlcpy(newQSO.date, dateTime.substring(0, 8).c_str(), sizeof(newQSO.date));
        strlcpy(newQSO.time_on, dateTime.substring(9, 13).c_str(), sizeof(newQSO.time_on));
      }

      // Save QSO using existing function (handles ADIF regeneration)
      if (saveQSO(newQSO)) {
        Serial.println("QSO created via web interface");
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save QSO\"}");
      }
    });

  // Update existing QSO
  webServer.on("/api/qsos/update", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (!checkWebAuth(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!checkWebAuth(request)) return;

      // Check SD card availability
      if (!sdCardAvailable) {
        request->send(503, "application/json", "{\"success\":false,\"error\":\"SD card required for QSO logging\"}");
        return;
      }

      // Parse JSON body
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Extract QSO data
      const char* date = doc["date"] | "";
      unsigned long id = doc["id"] | 0;

      if (strlen(date) == 0 || id == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing date or id\"}");
        return;
      }

      // Load the day's log file from SD card
      String filename = String(QSO_DIR) + "/qso_" + String(date) + ".json";
      File file = SD.open(filename, "r");
      if (!file) {
        request->send(404, "application/json", "{\"success\":false,\"error\":\"Log file not found\"}");
        return;
      }

      String content = file.readString();
      file.close();

      // Parse log file
      JsonDocument logDoc;
      error = deserializeJson(logDoc, content);
      if (error) {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to parse log file\"}");
        return;
      }

      // Find and update the QSO
      bool found = false;
      JsonArray logs = logDoc["logs"].as<JsonArray>();
      for (JsonObject qso : logs) {
        if (qso["id"].as<unsigned long>() == id) {
          // Update all fields
          qso["callsign"] = doc["callsign"];
          float frequency = doc["frequency"];
          qso["frequency"] = frequency;
          qso["mode"] = doc["mode"];

          // Calculate band from frequency
          String band = frequencyToBand(frequency);
          qso["band"] = band;

          qso["rst_sent"] = doc["rst_sent"];
          qso["rst_rcvd"] = doc["rst_rcvd"];
          qso["gridsquare"] = doc["gridsquare"];
          qso["my_gridsquare"] = doc["my_gridsquare"];
          qso["my_pota_ref"] = doc["my_pota_ref"];
          qso["their_pota_ref"] = doc["their_pota_ref"];
          qso["notes"] = doc["notes"];
          found = true;
          break;
        }
      }

      if (!found) {
        request->send(404, "application/json", "{\"success\":false,\"error\":\"QSO not found\"}");
        return;
      }

      // Save updated log file to SD card
      file = SD.open(filename, FILE_WRITE);
      if (!file) {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to open file for writing\"}");
        return;
      }

      serializeJson(logDoc, file);
      file.close();

      // Regenerate ADIF files
      regenerateADIFFiles(date);

      Serial.println("QSO updated via web interface");
      request->send(200, "application/json", "{\"success\":true}");
    });

  // Delete QSO
  webServer.on("/api/qsos/delete", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    // Check SD card availability
    if (!sdCardAvailable) {
      request->send(503, "application/json", "{\"success\":false,\"error\":\"SD card required for QSO logging\"}");
      return;
    }

    if (!request->hasParam("date") || !request->hasParam("id")) {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing date or id\"}");
      return;
    }

    String date = request->getParam("date")->value();
    unsigned long id = request->getParam("id")->value().toInt();

    // Load the day's log file from SD card
    String filename = String(QSO_DIR) + "/qso_" + date + ".json";
    File file = SD.open(filename, "r");
    if (!file) {
      request->send(404, "application/json", "{\"success\":false,\"error\":\"Log file not found\"}");
      return;
    }

    String content = file.readString();
    file.close();

    // Parse log file
    JsonDocument logDoc;
    DeserializationError error = deserializeJson(logDoc, content);
    if (error) {
      request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to parse log file\"}");
      return;
    }

    // Find and remove the QSO
    bool found = false;
    JsonArray logs = logDoc["logs"].as<JsonArray>();
    for (size_t i = 0; i < logs.size(); i++) {
      if (logs[i]["id"].as<unsigned long>() == id) {
        logs.remove(i);
        found = true;
        break;
      }
    }

    if (!found) {
      request->send(404, "application/json", "{\"success\":false,\"error\":\"QSO not found\"}");
      return;
    }

    // Update count
    int newCount = logs.size();
    logDoc["count"] = newCount;

    // If no QSOs left, delete the JSON file and its ADIF counterpart
    if (newCount == 0) {
      SD.remove(filename);
      String adifFile = String(QSO_DIR) + "/qso_" + date + ".adi";
      SD.remove(adifFile);
      Serial.println("Log files deleted (no QSOs remaining)");
    } else {
      // Save updated log file to SD card
      file = SD.open(filename, FILE_WRITE);
      if (!file) {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to open file for writing\"}");
        return;
      }

      serializeJson(logDoc, file);
      file.close();
    }

    // Regenerate ADIF files
    regenerateADIFFiles(date.c_str());

    Serial.println("QSO deleted via web interface");
    request->send(200, "application/json", "{\"success\":true}");
  });
}

#endif // WEB_API_QSO_H
