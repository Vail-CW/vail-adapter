// web_api_settings.h
// Device Settings and System Info API Endpoints for VAIL SUMMIT
// Handles radio control, CW settings, volume, callsign, and system diagnostics

#ifndef WEB_API_SETTINGS_H
#define WEB_API_SETTINGS_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <SPIFFS.h>

// External declarations for global variables
extern MenuMode currentMode;
extern bool radioOutputActive;
extern LGFX tft;
extern int cwSpeed;
extern int cwTone;
extern KeyType cwKeyType;
extern String vailCallsign;
extern StorageStats storageStats;
extern bool hasMAX17048;
extern bool hasLC709203;
extern Adafruit_LC709203F lc;
extern Adafruit_MAX17048 maxlipo;

// External function declarations
extern void startRadioOutput(LGFX &tft);
extern bool queueRadioMessage(const char* message);
extern void saveCWSettings();
extern void setVolume(int volume);
extern int getVolume();
extern void saveCallsign(String callsign);
extern bool checkWebAuth(AsyncWebServerRequest *request);

// ============================================
// Setup Function - Register All Settings API Endpoints
// ============================================

void setupSettingsAPI(AsyncWebServer &webServer) {

  // ============================================
  // Radio Control API Endpoints
  // ============================================

  // Radio status endpoint
  webServer.on("/api/radio/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    JsonDocument doc;
    doc["active"] = (currentMode == MODE_RADIO_OUTPUT && radioOutputActive);
    doc["mode"] = (currentMode == MODE_RADIO_OUTPUT) ? "radio_output" : "other";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Enter radio mode endpoint
  webServer.on("/api/radio/enter", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    // Switch to radio output mode
    currentMode = MODE_RADIO_OUTPUT;
    startRadioOutput(tft);

    Serial.println("Switched to Radio Output mode via web interface");

    request->send(200, "application/json", "{\"success\":true}");
  });

  // Send morse message endpoint
  webServer.on("/api/radio/send", HTTP_POST,
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

      const char* message = doc["message"] | "";
      if (strlen(message) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Message is empty\"}");
        return;
      }

      // Queue message for transmission
      if (queueRadioMessage(message)) {
        Serial.print("Queued radio message: ");
        Serial.println(message);
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Message queue is full\"}");
      }
    });

  // Get WPM speed endpoint
  webServer.on("/api/radio/wpm", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    JsonDocument doc;
    doc["wpm"] = cwSpeed;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Set WPM speed endpoint
  webServer.on("/api/radio/wpm", HTTP_POST,
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

      int wpm = doc["wpm"] | 0;
      if (wpm < 5 || wpm > 40) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"WPM must be between 5 and 40\"}");
        return;
      }

      // Update global CW speed
      cwSpeed = wpm;

      // Save to preferences
      saveCWSettings();

      Serial.print("CW speed updated to ");
      Serial.print(wpm);
      Serial.println(" WPM via web interface");

      request->send(200, "application/json", "{\"success\":true}");
    });

  // ============================================
  // Device Settings API Endpoints
  // ============================================

  // Get CW settings
  webServer.on("/api/settings/cw", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    JsonDocument doc;
    doc["wpm"] = cwSpeed;
    doc["tone"] = cwTone;
    doc["keyType"] = (int)cwKeyType;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Set CW settings
  webServer.on("/api/settings/cw", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (!checkWebAuth(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!checkWebAuth(request)) return;

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Validate and update settings
      if (doc.containsKey("wpm")) {
        int wpm = doc["wpm"];
        if (wpm < 5 || wpm > 40) {
          request->send(400, "application/json", "{\"success\":false,\"error\":\"WPM must be between 5 and 40\"}");
          return;
        }
        cwSpeed = wpm;
      }

      if (doc.containsKey("tone")) {
        int tone = doc["tone"];
        if (tone < 400 || tone > 1200) {
          request->send(400, "application/json", "{\"success\":false,\"error\":\"Tone must be between 400 and 1200 Hz\"}");
          return;
        }
        cwTone = tone;
      }

      if (doc.containsKey("keyType")) {
        int keyType = doc["keyType"];
        if (keyType < 0 || keyType > 2) {
          request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid key type\"}");
          return;
        }
        cwKeyType = (KeyType)keyType;
      }

      // Save to preferences
      saveCWSettings();

      Serial.println("CW settings updated via web interface");

      request->send(200, "application/json", "{\"success\":true}");
    });

  // Get volume
  webServer.on("/api/settings/volume", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    JsonDocument doc;
    doc["volume"] = getVolume();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Set volume
  webServer.on("/api/settings/volume", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (!checkWebAuth(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!checkWebAuth(request)) return;

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      int volume = doc["volume"] | -1;
      if (volume < 0 || volume > 100) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Volume must be between 0 and 100\"}");
        return;
      }

      // Update volume
      setVolume(volume);

      Serial.print("Volume updated to ");
      Serial.print(volume);
      Serial.println("% via web interface");

      request->send(200, "application/json", "{\"success\":true}");
    });

  // Get callsign
  webServer.on("/api/settings/callsign", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    JsonDocument doc;
    doc["callsign"] = vailCallsign;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Set callsign
  webServer.on("/api/settings/callsign", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (!checkWebAuth(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!checkWebAuth(request)) return;

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      String callsign = doc["callsign"] | "";
      callsign.trim();
      callsign.toUpperCase();

      if (callsign.length() == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Callsign cannot be empty\"}");
        return;
      }

      if (callsign.length() > 10) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Callsign too long (max 10 characters)\"}");
        return;
      }

      // Update callsign
      vailCallsign = callsign;
      saveCallsign(callsign);

      Serial.print("Callsign updated to ");
      Serial.print(callsign);
      Serial.println(" via web interface");

      request->send(200, "application/json", "{\"success\":true}");
    });

  // ============================================
  // System Info API Endpoint
  // ============================================

  // Get system info
  webServer.on("/api/system/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    JsonDocument doc;

    // Firmware
    doc["firmware"] = FIRMWARE_VERSION;
    doc["firmwareDate"] = FIRMWARE_DATE;
    doc["firmwareName"] = FIRMWARE_NAME;

    // Chip info
    doc["chipModel"] = ESP.getChipModel();
    doc["chipRevision"] = ESP.getChipRevision();

    // System
    doc["uptime"] = millis();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["flashSize"] = ESP.getFlashChipSize();

    // Memory
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["minFreeHeap"] = ESP.getMinFreeHeap();
    doc["psramFound"] = psramFound();
    if (psramFound()) {
      doc["freePsram"] = ESP.getFreePsram();
      doc["minFreePsram"] = ESP.getMinFreePsram();
      doc["psramSize"] = ESP.getPsramSize();
    }

    // Storage
    doc["spiffsUsed"] = SPIFFS.usedBytes();
    doc["spiffsTotal"] = SPIFFS.totalBytes();
    doc["qsoCount"] = storageStats.totalLogs;

    // WiFi
    doc["wifiConnected"] = WiFi.isConnected();
    if (WiFi.isConnected()) {
      doc["wifiSSID"] = WiFi.SSID();
      doc["wifiIP"] = WiFi.localIP().toString();
      doc["wifiRSSI"] = WiFi.RSSI();
    }

    // Battery
    float batteryVoltage = 0;
    float batteryPercent = 0;
    String batteryMonitor = "None";

    if (hasMAX17048) {
      batteryVoltage = maxlipo.cellVoltage();
      batteryPercent = maxlipo.cellPercent();
      batteryMonitor = "MAX17048";
    } else if (hasLC709203) {
      batteryVoltage = lc.cellVoltage();
      batteryPercent = lc.cellPercent();
      batteryMonitor = "LC709203F";
    }

    doc["batteryVoltage"] = batteryVoltage;
    doc["batteryPercent"] = batteryPercent;
    doc["batteryMonitor"] = batteryMonitor;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  Serial.println("Settings API endpoints registered");
}

#endif // WEB_API_SETTINGS_H
