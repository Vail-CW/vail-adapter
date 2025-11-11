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

// Include password protection module (must be before other includes)
#include "settings_web_password.h"

// Include modular web components
#include "web_pages_dashboard.h"
#include "web_pages_wifi.h"
#include "web_pages_practice.h"
#include "web_pages_memory_chain.h"
#include "web_pages_hear_it_type_it.h"
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
#include "web_hear_it_socket.h"

// Global web server instance
AsyncWebServer webServer(80);

// WebSocket for practice mode
AsyncWebSocket practiceWebSocket("/ws/practice");

// WebSocket for hear it type it mode
AsyncWebSocket hearItWebSocket("/ws/hear-it");

// mDNS hostname
String mdnsHostname = "vail-summit";

// Server state
bool webServerRunning = false;
bool webPracticeModeActive = false;

// Forward declarations
void setupWebServer();
void stopWebServer();
bool checkWebAuth(AsyncWebServerRequest *request);
void onPracticeWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void sendPracticeDecoded(String morse, String text);
void sendPracticeWPM(float wpm);
void startWebPracticeMode(Adafruit_ST7789& tft);
void startWebMemoryChainMode(Adafruit_ST7789& tft, int difficulty, int mode, int wpm, bool sound, bool hints);
void startWebHearItMode(Adafruit_ST7789& tft);

/*
 * Check web authentication
 * Returns true if request is authenticated or auth is disabled
 */
bool checkWebAuth(AsyncWebServerRequest *request) {
  // If auth is disabled, allow all requests
  if (!webAuthEnabled || webPassword.length() == 0) {
    return true;
  }

  // Check for HTTP Basic Auth header
  if (!request->authenticate("admin", webPassword.c_str())) {
    request->requestAuthentication();
    return false;
  }

  return true;
}

// Serve functions for modular pages
void serveRadioPage(AsyncWebServerRequest *request) {
  extern const char RADIO_HTML[] PROGMEM;
  request->send_P(200, "text/html", RADIO_HTML);
}

void serveSettingsPage(AsyncWebServerRequest *request) {
  extern const char SETTINGS_HTML[] PROGMEM;
  request->send_P(200, "text/html", SETTINGS_HTML);
}

void serveSystemPage(AsyncWebServerRequest *request) {
  extern const char SYSTEM_HTML[] PROGMEM;
  request->send_P(200, "text/html", SYSTEM_HTML);
}

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
    if (!checkWebAuth(request)) return;
    serveDashboard(request, FIRMWARE_VERSION, FIRMWARE_DATE, mdnsHostname);
  });

  // ============================================
  // QSO Logger Page (Enhanced)
  // ============================================
  webServer.on("/logger", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    request->send_P(200, "text/html", LOGGER_HTML);
  });

  // ============================================
  // WiFi Setup Page
  // ============================================
  webServer.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    serveWiFiSetup(request);
  });

  // ============================================
  // Radio Control Page
  // ============================================
  webServer.on("/radio", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    serveRadioPage(request);
  });

  // ============================================
  // Device Settings Page
  // ============================================
  webServer.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    serveSettingsPage(request);
  });

  // ============================================
  // System Info Page
  // ============================================
  webServer.on("/system", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    serveSystemPage(request);
  });

  // ============================================
  // Practice Mode Page
  // ============================================
  webServer.on("/practice", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    request->send_P(200, "text/html", PRACTICE_PAGE_HTML);
  });

  // ============================================
  // Memory Chain Game Page
  // ============================================
  webServer.on("/memory-chain", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    request->send_P(200, "text/html", MEMORY_CHAIN_HTML);
  });

  // ============================================
  // Hear It Type It Training Page
  // ============================================
  webServer.on("/hear-it", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    request->send_P(200, "text/html", HEAR_IT_TYPE_IT_HTML);
  });

  // ============================================
  // API Endpoints
  // ============================================

  // Device status endpoint
  webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    request->send(200, "application/json", getDeviceStatusJSON());
  });

  // QSO logs list endpoint
  webServer.on("/api/qsos", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    request->send(200, "application/json", getQSOLogsJSON());
  });

  // ADIF export endpoint
  webServer.on("/api/export/adif", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    String adif = generateADIF();
    AsyncWebServerResponse *response = request->beginResponse(200, "application/x-adif", adif);
    response->addHeader("Content-Disposition", "attachment; filename=vail-summit-logs.adi");
    request->send(response);
  });

  // CSV export endpoint
  webServer.on("/api/export/csv", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    String csv = generateCSV();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csv);
    response->addHeader("Content-Disposition", "attachment; filename=vail-summit-logs.csv");
    request->send(response);
  });

  // Practice mode API endpoint
  webServer.on("/api/practice/start", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
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
    [](AsyncWebServerRequest *request) {
      if (!checkWebAuth(request)) {
        request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!checkWebAuth(request)) return;

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

  // Hear It Type It mode API endpoint
  webServer.on("/api/hear-it/start", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (!checkWebAuth(request)) {
        request->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      }
    },
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!checkWebAuth(request)) return;

      JsonDocument doc;
      deserializeJson(doc, data, len);

      extern MenuMode currentMode;
      extern Adafruit_ST7789 tft;

      // Get settings from request
      int mode = doc["mode"].as<int>();
      int length = doc["length"].as<int>();
      String customChars = doc["customChars"].as<String>();

      // Update device settings
      hearItSettings.mode = (HearItMode)mode;
      hearItSettings.groupLength = length;
      hearItSettings.customChars = customChars;

      Serial.printf("Web Hear It settings: mode=%d, length=%d, custom=%s\n",
                    mode, length, customChars.c_str());

      // Switch device to web hear it mode
      currentMode = MODE_WEB_HEAR_IT;

      // Initialize the mode (this will draw the UI and start playing character groups)
      startWebHearItMode(tft);

      JsonDocument response;
      response["status"] = "active";
      response["endpoint"] = "ws://" + mdnsHostname + ".local/ws/hear-it";

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

  // Setup WebSocket for hear it type it mode
  hearItWebSocket.onEvent(onHearItWebSocketEvent);
  webServer.addHandler(&hearItWebSocket);

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
