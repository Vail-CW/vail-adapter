/*
 * Web Server Module (Modular Version)
 * Provides a comprehensive web interface for device management
 * Features: QSO logging, settings management, device status
 *
 * Access via: http://vail-summit.local or device IP address
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

// Include modular web components
#include "web_pages_dashboard.h"
#include "web_pages_wifi.h"
#include "web_pages_practice.h"
#include "web_pages_memory_chain.h"
#include "web_pages_radio.h"
#include "web_pages_settings.h"
#include "web_pages_system.h"
#include "web_api_wifi.h"
#include "web_api_qso.h"
#include "web_api_settings.h"
#include "web_api_memories.h"
#include "web_server_api.h"
#include "web_logger_enhanced.h"
#include "web_practice_socket.h"
#include "web_memory_chain_socket.h"

// Global web server instance
AsyncWebServer webServer(80);

// WebSocket for practice mode
AsyncWebSocket practiceWebSocket("/ws/practice");

// mDNS hostname
String mdnsHostname = "vail-summit";

// Server state
bool webServerRunning = false;
bool webPracticeModeActive = false;

// Forward declarations
void setupWebServer();
void stopWebServer();
void onPracticeWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void sendPracticeDecoded(String morse, String text);
void sendPracticeWPM(float wpm);
void startWebPracticeMode(Adafruit_ST7789& tft);
void startWebMemoryChainMode(Adafruit_ST7789& tft, int difficulty, int mode, int wpm, bool sound, bool hints);

/*
 * Initialize and start the web server
 * Called automatically when WiFi connects
 */
void setupWebServer() {
  if (webServerRunning) {
    Serial.println("Web server already running");
    return;
  }

  Serial.println("Starting web server...");

  // Check if we're in AP mode (from settings_wifi.h)
  extern bool isAPMode;

  // Set up mDNS responder (only works in Station mode, not AP mode)
  if (!isAPMode) {
    if (MDNS.begin(mdnsHostname.c_str())) {
      Serial.print("mDNS responder started: http://");
      Serial.print(mdnsHostname);
      Serial.println(".local");
      MDNS.addService("http", "tcp", 80);
    } else {
      Serial.println("Error setting up mDNS responder!");
    }
  } else {
    Serial.println("Skipping mDNS setup (not supported in AP mode)");
  }

  // ============================================
  // Main Dashboard Page
  // ============================================
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveDashboard(request, FIRMWARE_VERSION, FIRMWARE_DATE, mdnsHostname);
  });

  // ============================================
  // QSO Logger Page (Enhanced)
  // ============================================
  webServer.on("/logger", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", LOGGER_HTML);
  });

  // ============================================
  // WiFi Setup Page
  // ============================================
  webServer.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveWiFiSetup(request);
  });

  // ============================================
  // Radio Control Page
  // ============================================
  webServer.on("/radio", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveRadioPage(request);
  });

  // ============================================
  // Device Settings Page
  // ============================================
  webServer.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveSettingsPage(request);
  });

  // ============================================
  // System Info Page
  // ============================================
  webServer.on("/system", HTTP_GET, [](AsyncWebServerRequest *request) {
    serveSystemPage(request);
  });

  // ============================================
  // Practice Mode Page
  // ============================================
  webServer.on("/practice", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", PRACTICE_PAGE_HTML);
  });

  // ============================================
  // Memory Chain Game Page
  // ============================================
  webServer.on("/memory-chain", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", MEMORY_CHAIN_HTML);
  });

  // ============================================
  // API Endpoints
  // ============================================

  // Device status endpoint
  webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getDeviceStatusJSON());
  });

  // QSO logs list endpoint
  webServer.on("/api/qsos", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getQSOLogsJSON());
  });

  // ADIF export endpoint
  webServer.on("/api/export/adif", HTTP_GET, [](AsyncWebServerRequest *request) {
    String adif = generateADIF();
    AsyncWebServerResponse *response = request->beginResponse(200, "application/x-adif", adif);
    response->addHeader("Content-Disposition", "attachment; filename=vail-summit-logs.adi");
    request->send(response);
  });

  // CSV export endpoint
  webServer.on("/api/export/csv", HTTP_GET, [](AsyncWebServerRequest *request) {
    String csv = generateCSV();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csv);
    response->addHeader("Content-Disposition", "attachment; filename=vail-summit-logs.csv");
    request->send(response);
  });

  // Practice mode API endpoint
  webServer.on("/api/practice/start", HTTP_POST, [](AsyncWebServerRequest *request) {
    extern MenuMode currentMode;
    extern Adafruit_ST7789 tft;

    // Switch device to web practice mode
    currentMode = MODE_WEB_PRACTICE;

    // Initialize the mode (this will draw the UI and set up decoder)
    startWebPracticeMode(tft);

    JsonDocument doc;
    doc["status"] = "active";
    doc["endpoint"] = "ws://" + mdnsHostname + ".local/ws/practice";

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Memory Chain mode API endpoint
  webServer.on("/api/memory-chain/start", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      deserializeJson(doc, data, len);

      extern MenuMode currentMode;
      extern Adafruit_ST7789 tft;

      int difficulty = doc["difficulty"].as<int>();
      int mode = doc["mode"].as<int>();
      int wpm = doc["wpm"].as<int>();
      bool sound = doc["sound"].as<bool>();
      bool hints = doc["hints"].as<bool>();

      currentMode = MODE_WEB_MEMORY_CHAIN;
      startWebMemoryChainMode(tft, difficulty, mode, wpm, sound, hints);

      JsonDocument response;
      response["status"] = "active";
      response["endpoint"] = "ws://" + mdnsHostname + ".local/ws/memory-chain";

      String output;
      serializeJson(response, output);
      request->send(200, "application/json", output);
    });

  // Setup modular API endpoints
  setupQSOAPI(webServer);
  setupWiFiAPI(webServer);
  setupSettingsAPI(webServer);
  setupMemoriesAPI(webServer);

  // Setup WebSocket for practice mode
  practiceWebSocket.onEvent(onPracticeWebSocketEvent);
  webServer.addHandler(&practiceWebSocket);

  // Setup WebSocket for memory chain mode
  memoryChainWebSocket.onEvent(onMemoryChainWebSocketEvent);
  webServer.addHandler(&memoryChainWebSocket);

  // Start server
  webServer.begin();
  webServerRunning = true;

  Serial.println("Web server started successfully");

  // Show appropriate access method based on mode
  if (isAPMode) {
    Serial.print("Access at: http://");
    Serial.print(WiFi.softAPIP());
    Serial.println("/");
    Serial.println("(mDNS not available in AP mode - use IP address only)");
  } else {
    Serial.print("Access at: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
    Serial.print("Or via mDNS: http://");
    Serial.print(mdnsHostname);
    Serial.println(".local/");
  }
}

/*
 * Stop the web server
 */
void stopWebServer() {
  if (!webServerRunning) {
    return;
  }

  webServer.end();
  MDNS.end();
  webServerRunning = false;
  Serial.println("Web server stopped");
}

#endif // WEB_SERVER_H
