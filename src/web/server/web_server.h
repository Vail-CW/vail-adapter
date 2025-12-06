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
#include "../../settings/settings_web_password.h"

// Include modular web components
#include "../pages/web_pages_setup.h"      // Setup splash page (always in flash)
#include "../pages/web_pages_dashboard.h"  // Fallback dashboard (in flash for now)
#include "../pages/web_pages_wifi.h"
#include "../pages/web_pages_practice.h"
#include "../pages/web_pages_memory_chain.h"
#include "../pages/web_pages_hear_it_type_it.h"
#include "../pages/web_pages_radio.h"
#include "../pages/web_pages_settings.h"
#include "../pages/web_pages_system.h"
#include "../pages/web_pages_storage.h"
#include "../api/web_api_wifi.h"
#include "../api/web_api_qso.h"
#include "../api/web_api_settings.h"
#include "../api/web_api_memories.h"
#include "../api/web_api_storage.h"
#include "web_server_api.h"  // Same folder
#include "web_file_downloader.h"  // GitHub download functions
#include "../pages/web_logger_enhanced.h"
#include "../modes/web_practice_socket.h"
#include "../modes/web_memory_chain_socket.h"
#include "../modes/web_hear_it_socket.h"
#include "../../storage/sd_card.h"

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
bool webFilesOnSD = false;  // True if web files exist on SD card
bool webServerRestartPending = false;  // Flag to restart server (checked in main loop)

// Forward declarations
void setupWebServer();
void stopWebServer();
void restartWebServer();
void requestWebServerRestart();
bool checkWebAuth(AsyncWebServerRequest *request);
void onPracticeWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void sendPracticeDecoded(String morse, String text);
void sendPracticeWPM(float wpm);
void startWebPracticeMode(LGFX& tft);
void startWebMemoryChainMode(LGFX& tft, int difficulty, int mode, int wpm, bool sound, bool hints);
void startWebHearItMode(LGFX& tft);

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
  // Check if web files exist on SD card
  // ============================================
  webFilesOnSD = webFilesExist();
  Serial.printf("Web files on SD card: %s\n", webFilesOnSD ? "YES" : "NO");

  // ============================================
  // Web Files Download/Upload API (always available)
  // ============================================
  webServer.on("/api/webfiles/download", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    // Start download in background (non-blocking would require task, for now synchronous)
    // Note: This blocks the request but provides progress via polling
    bool success = downloadWebFilesFromGitHub();

    JsonDocument doc;
    doc["success"] = success;
    if (!success) {
      doc["error"] = webDownloadProgress.errorMessage;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);

    // If successful, update flag (will take effect on next request)
    if (success) {
      webFilesOnSD = true;
    }
  });

  webServer.on("/api/webfiles/progress", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", getWebDownloadProgressJson());
  });

  webServer.on("/api/webfiles/cancel", HTTP_POST, [](AsyncWebServerRequest *request) {
    cancelWebFileDownload();
    request->send(200, "application/json", "{\"cancelled\":true}");
  });

  webServer.on("/api/webfiles/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["installed"] = webFilesOnSD;
    doc["version"] = getWebFilesVersion();

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // Web file upload endpoint (for manual installation)
  webServer.on("/api/webfiles/upload", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      request->send(200, "application/json", "{\"status\":\"complete\"}");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!checkWebAuth(request)) return;

      // Get target path from form data or use filename
      String path = "/www/" + filename;
      if (request->hasParam("path", true)) {
        path = request->getParam("path", true)->value();
      }

      // Create /www directory if needed
      if (index == 0) {
        if (!sdCardAvailable) {
          initSDCard();
        }
        if (!SD.exists("/www")) {
          SD.mkdir("/www");
        }
        createDirectoriesForPath(path.c_str());
        Serial.printf("Uploading web file: %s\n", path.c_str());
      }

      // Open file in write mode at start, append mode for subsequent chunks
      File file;
      if (index == 0) {
        file = SD.open(path.c_str(), FILE_WRITE);
      } else {
        file = SD.open(path.c_str(), FILE_APPEND);
      }

      if (file) {
        file.write(data, len);
        file.close();
      }

      if (final) {
        Serial.printf("Upload complete: %s (%u bytes)\n", path.c_str(), index + len);
        webFilesOnSD = webFilesExist();  // Re-check
      }
    }
  );

  // Web server restart endpoint (called after file upload to switch from setup to normal mode)
  webServer.on("/api/webfiles/restart", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;

    // Check if web files now exist
    webFilesOnSD = webFilesExist();

    if (webFilesOnSD) {
      request->send(200, "application/json", "{\"success\":true,\"message\":\"Server will restart\"}");
      // Request restart - will be handled in main loop
      requestWebServerRestart();
    } else {
      request->send(200, "application/json", "{\"success\":false,\"message\":\"No web files found\"}");
    }
  });

  // ============================================
  // Setup Mode vs Normal Mode Routing
  // ============================================
  if (!webFilesOnSD) {
    // SETUP MODE: Serve setup splash page for all HTML routes
    Serial.println("Web server running in SETUP MODE (no web files on SD)");

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (!checkWebAuth(request)) return;
      request->send_P(200, "text/html", SETUP_HTML);
    });

    // All other page routes redirect to setup
    webServer.on("/logger", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");
    });
    webServer.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");
    });
    webServer.on("/radio", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");
    });
    webServer.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");
    });
    webServer.on("/system", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");
    });
    webServer.on("/storage", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");
    });
    webServer.on("/practice", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");
    });
    webServer.on("/memory-chain", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");
    });
    webServer.on("/hear-it", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");
    });

  } else {
    // NORMAL MODE: Serve pages from SD card
    Serial.println("Web server running in NORMAL MODE (serving from SD card)");

    // Serve static files from SD card /www/ directory
    webServer.serveStatic("/", SD, "/www/").setDefaultFile("index.html");
  }

  // ============================================
  // API Endpoints (always available regardless of mode)
  // ============================================

  // Main Dashboard Page (fallback if SD serving fails)
  // Note: In normal mode, serveStatic handles "/" but we keep this as fallback
  if (webFilesOnSD) {
    // Dashboard is served from SD, but keep API working
  }

  // Legacy dashboard endpoint for backward compatibility
  webServer.on("/dashboard-legacy", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!checkWebAuth(request)) return;
    serveDashboard(request, FIRMWARE_VERSION, FIRMWARE_DATE, mdnsHostname);
  });

  // ============================================
  // API Endpoints (always available)
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
    extern LGFX tft;

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
      extern LGFX tft;

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
      extern LGFX tft;

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
  registerStorageAPI(&webServer);

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

/*
 * Restart the web server (to pick up new routes after file upload)
 * This should be called from the main loop when webServerRestartPending is true
 */
void restartWebServer() {
  Serial.println("Restarting web server...");
  stopWebServer();
  delay(100);  // Brief delay to ensure clean shutdown
  setupWebServer();
  webServerRestartPending = false;
}

/*
 * Check if web server restart is pending (call from main loop)
 */
bool isWebServerRestartPending() {
  return webServerRestartPending;
}

/*
 * Request web server restart (safe to call from request handlers)
 */
void requestWebServerRestart() {
  webServerRestartPending = true;
  Serial.println("Web server restart requested");
}

#endif // WEB_SERVER_H
