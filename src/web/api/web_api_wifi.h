/*
 * Web API - WiFi Endpoints
 * Handles WiFi status, scanning, and connection
 */

#ifndef WEB_API_WIFI_H
#define WEB_API_WIFI_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// External function declarations
extern bool checkWebAuth(AsyncWebServerRequest *request);

// WiFi status endpoint - check if in AP mode and if remote connection
void setupWiFiStatusEndpoint(AsyncWebServer &server) {
  server.on("/api/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    extern bool isAPMode;  // From settings_wifi.h

    JsonDocument doc;
    doc["isAPMode"] = isAPMode;

    // Check if this is a remote connection (client IP != AP IP)
    // If we're in STA mode (connected to a network), any web access is "remote"
    // If we're in AP mode, web access is "local" (direct connection)
    bool isRemoteConnection = !isAPMode && WiFi.status() == WL_CONNECTED;
    doc["isRemoteConnection"] = isRemoteConnection;

    doc["wifiMode"] = isAPMode ? "AP" : "STA";
    doc["connected"] = WiFi.status() == WL_CONNECTED;

    if (isAPMode) {
      doc["ssid"] = WiFi.softAPSSID();
      doc["ip"] = WiFi.softAPIP().toString();
    } else if (WiFi.status() == WL_CONNECTED) {
      doc["ssid"] = WiFi.SSID();
      doc["ip"] = WiFi.localIP().toString();
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
}

// WiFi scan endpoint
void setupWiFiScanEndpoint(AsyncWebServer &server) {
  server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    extern bool isAPMode;

    // Safety check: only allow scanning if in AP mode or not connected
    if (!isAPMode && WiFi.status() == WL_CONNECTED) {
      request->send(403, "application/json", "{\"success\":false,\"error\":\"WiFi changes disabled during remote connection\"}");
      return;
    }

    Serial.println("Web API: Scanning for WiFi networks...");

    // Scan networks
    int n = WiFi.scanNetworks();

    JsonDocument doc;
    if (n < 0) {
      doc["success"] = false;
      doc["error"] = "Scan failed";
    } else {
      doc["success"] = true;
      JsonArray networks = doc["networks"].to<JsonArray>();

      for (int i = 0; i < n; i++) {
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["encrypted"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      }
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);

    Serial.print("Web API: Scan complete, found ");
    Serial.print(n);
    Serial.println(" networks");
  });
}

// WiFi connect endpoint
void setupWiFiConnectEndpoint(AsyncWebServer &server) {
  server.on("/api/wifi/connect", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (!checkWebAuth(request)) {
        request->send(401, "application/json", "{\"success\":false,\"error\":\"Unauthorized\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!checkWebAuth(request)) return;

      extern bool isAPMode;
      extern void connectToWiFi(String ssid, String password);  // From settings_wifi.h

      // Safety check: only allow connection if in AP mode or not connected
      if (!isAPMode && WiFi.status() == WL_CONNECTED) {
        request->send(403, "application/json", "{\"success\":false,\"error\":\"WiFi changes disabled during remote connection\"}");
        return;
      }

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        Serial.print("Web API: JSON parse error: ");
        Serial.println(error.c_str());
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Debug: Print received JSON
      Serial.print("Web API: Received JSON: ");
      String debugJson;
      serializeJson(doc, debugJson);
      Serial.println(debugJson);

      String ssid = doc["ssid"].as<String>();
      String password = doc["password"].as<String>();

      Serial.print("Web API: Parsed SSID: '");
      Serial.print(ssid);
      Serial.print("' (length: ");
      Serial.print(ssid.length());
      Serial.println(")");

      Serial.print("Web API: Password length: ");
      Serial.println(password.length());

      if (ssid.length() == 0) {
        Serial.println("Web API: ERROR - SSID is empty");
        request->send(400, "application/json", "{\"success\":false,\"error\":\"SSID required\"}");
        return;
      }

      Serial.print("Web API: Attempting to connect to ");
      Serial.println(ssid);

      // Connect to WiFi (this will save credentials)
      connectToWiFi(ssid, password);

      // Check connection result
      if (WiFi.status() == WL_CONNECTED) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Connected successfully\"}");
        Serial.println("Web API: Connection successful");
      } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Connection failed\"}");
        Serial.println("Web API: Connection failed");
      }
    });
}

// Setup all WiFi API endpoints
void setupWiFiAPI(AsyncWebServer &server) {
  setupWiFiStatusEndpoint(server);
  setupWiFiScanEndpoint(server);
  setupWiFiConnectEndpoint(server);
}

#endif // WEB_API_WIFI_H
