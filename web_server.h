/*
 * Web Server Module
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

// Note: qso_logger_storage.h is already included by the main .ino file before this header
#include "web_logger_enhanced.h"

// Global web server instance
AsyncWebServer webServer(80);

// mDNS hostname
String mdnsHostname = "vail-summit";

// Server state
bool webServerRunning = false;

// Forward declarations
void setupWebServer();
void stopWebServer();
String getDeviceStatusJSON();
String getQSOLogsJSON();
String generateADIF();
String generateCSV();

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

  // Set up mDNS responder
  if (MDNS.begin(mdnsHostname.c_str())) {
    Serial.print("mDNS responder started: http://");
    Serial.print(mdnsHostname);
    Serial.println(".local");
    MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("Error setting up mDNS responder!");
  }

  // ============================================
  // Main Dashboard Page
  // ============================================
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>VAIL SUMMIT - Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            padding: 20px;
            min-height: 100vh;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        header {
            text-align: center;
            margin-bottom: 40px;
            padding: 30px 20px;
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
        h1 { font-size: 2.5rem; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }
        .subtitle { font-size: 1.1rem; opacity: 0.9; }
        .status-bar {
            display: flex;
            gap: 20px;
            justify-content: center;
            margin-top: 20px;
            flex-wrap: wrap;
        }
        .status-item {
            background: rgba(255,255,255,0.15);
            padding: 10px 20px;
            border-radius: 8px;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .status-icon { font-size: 1.5rem; }
        .dashboard-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 40px;
        }
        .card {
            background: rgba(255,255,255,0.1);
            border-radius: 15px;
            padding: 25px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255,255,255,0.2);
            transition: transform 0.2s, box-shadow 0.2s;
        }
        .card:hover {
            transform: translateY(-5px);
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        }
        .card h2 {
            font-size: 1.5rem;
            margin-bottom: 15px;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .card-icon { font-size: 2rem; }
        .card p { opacity: 0.9; margin-bottom: 15px; line-height: 1.6; }
        .btn {
            display: inline-block;
            padding: 12px 24px;
            background: rgba(255,255,255,0.2);
            color: #fff;
            text-decoration: none;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.3);
            transition: all 0.2s;
            cursor: pointer;
            font-size: 1rem;
        }
        .btn:hover {
            background: rgba(255,255,255,0.3);
            border-color: rgba(255,255,255,0.5);
        }
        .btn-primary {
            background: #00d4ff;
            color: #1e3c72;
            border: none;
            font-weight: 600;
        }
        .btn-primary:hover { background: #00b8e6; }
        footer {
            text-align: center;
            padding: 20px;
            opacity: 0.7;
            font-size: 0.9rem;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>📡 VAIL SUMMIT</h1>
            <p class="subtitle">Portable Morse Code Training Device</p>
            <div class="status-bar" id="statusBar">
                <div class="status-item">
                    <span class="status-icon">🔋</span>
                    <span id="battery">Loading...</span>
                </div>
                <div class="status-item">
                    <span class="status-icon">📶</span>
                    <span id="wifi">Connected</span>
                </div>
                <div class="status-item">
                    <span class="status-icon">📊</span>
                    <span id="qsoCount">0 QSOs</span>
                </div>
            </div>
        </header>

        <div class="dashboard-grid">
            <div class="card">
                <h2><span class="card-icon">📝</span> QSO Logger</h2>
                <p>View, manage, and export your contact logs. Download ADIF files for upload to QRZ, LoTW, and other services.</p>
                <a href="/logger" class="btn btn-primary">Open Logger</a>
            </div>

            <div class="card">
                <h2><span class="card-icon">⚙️</span> Device Settings</h2>
                <p>Configure CW speed, tone frequency, volume, key type, and other device preferences.</p>
                <a href="/settings" class="btn btn-primary">Manage Settings</a>
            </div>

            <div class="card">
                <h2><span class="card-icon">📡</span> WiFi Setup</h2>
                <p>Scan for networks and configure WiFi credentials for internet connectivity.</p>
                <a href="/wifi" class="btn btn-primary">WiFi Config</a>
            </div>

            <div class="card">
                <h2><span class="card-icon">ℹ️</span> System Info</h2>
                <p>View firmware version, memory usage, storage stats, and device diagnostics.</p>
                <a href="/system" class="btn btn-primary">View Info</a>
            </div>
        </div>

        <footer>
            <p>VAIL SUMMIT Web Interface | Firmware v1.0</p>
            <p>Access this page at: <strong>http://)rawliteral" + mdnsHostname + R"rawliteral(.local</strong></p>
        </footer>
    </div>

    <script>
        // Load device status
        async function loadStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();

                document.getElementById('battery').textContent = data.battery;
                document.getElementById('wifi').textContent = data.wifi;
                document.getElementById('qsoCount').textContent = data.qsoCount + ' QSOs';
            } catch (error) {
                console.error('Failed to load status:', error);
            }
        }

        // Load status on page load and refresh every 10 seconds
        loadStatus();
        setInterval(loadStatus, 10000);
    </script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
  });

  // ============================================
  // QSO Logger Page (Enhanced)
  // ============================================
  webServer.on("/logger", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", LOGGER_HTML);
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

  // ============================================
  // Station Settings Endpoints
  // ============================================

  // Get station settings
  webServer.on("/api/settings/station", HTTP_GET, [](AsyncWebServerRequest *request) {
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
  webServer.on("/api/settings/station", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
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
  webServer.on("/api/qsos/create", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
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

      // Save QSO using existing function
      if (saveQSO(newQSO)) {
        Serial.println("QSO created via web interface");
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save QSO\"}");
      }
    });

  // Update existing QSO
  webServer.on("/api/qsos/update", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
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

      // Load the day's log file
      String filename = "/logs/qso_" + String(date) + ".json";
      File file = FileSystem.open(filename, "r");
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

      // Save updated log file
      file = FileSystem.open(filename, "w");
      if (!file) {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to open file for writing\"}");
        return;
      }

      serializeJson(logDoc, file);
      file.close();

      Serial.println("QSO updated via web interface");
      request->send(200, "application/json", "{\"success\":true}");
    });

  // Delete QSO
  webServer.on("/api/qsos/delete", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("date") || !request->hasParam("id")) {
      request->send(400, "application/json", "{\"success\":false,\"error\":\"Missing date or id\"}");
      return;
    }

    String date = request->getParam("date")->value();
    unsigned long id = request->getParam("id")->value().toInt();

    // Load the day's log file
    String filename = "/logs/qso_" + date + ".json";
    File file = FileSystem.open(filename, "r");
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

    // If no QSOs left, delete the file
    if (newCount == 0) {
      FileSystem.remove(filename);
      Serial.println("Log file deleted (no QSOs remaining)");
    } else {
      // Save updated log file
      file = FileSystem.open(filename, "w");
      if (!file) {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to open file for writing\"}");
        return;
      }

      serializeJson(logDoc, file);
      file.close();
    }

    // Note: Metadata will be refreshed when QSO logger loads on device
    // or when the page reloads (it counts logs dynamically in getQSOLogsJSON)

    Serial.println("QSO deleted via web interface");
    request->send(200, "application/json", "{\"success\":true}");
  });

  // Start server
  webServer.begin();
  webServerRunning = true;

  Serial.println("Web server started successfully");
  Serial.print("Access at: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  Serial.print("Or via mDNS: http://");
  Serial.print(mdnsHostname);
  Serial.println(".local/");
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
 * Get device status as JSON
 */
String getDeviceStatusJSON() {
  JsonDocument doc;

  // Battery status (using external variables)
  extern bool hasLC709203;
  extern bool hasMAX17048;
  extern Adafruit_LC709203F lc;
  extern Adafruit_MAX17048 maxlipo;

  float batteryVoltage = 0;
  float batteryPercent = 0;

  if (hasMAX17048) {
    batteryVoltage = maxlipo.cellVoltage();
    batteryPercent = maxlipo.cellPercent();
  } else if (hasLC709203) {
    batteryVoltage = lc.cellVoltage();
    batteryPercent = lc.cellPercent();
  }

  char batteryStr[32];
  snprintf(batteryStr, sizeof(batteryStr), "%.2fV (%.0f%%)", batteryVoltage, batteryPercent);
  doc["battery"] = batteryStr;

  // WiFi status
  doc["wifi"] = WiFi.isConnected() ? "Connected" : "Disconnected";
  doc["ip"] = WiFi.localIP().toString();
  doc["rssi"] = WiFi.RSSI();

  // QSO count
  doc["qsoCount"] = storageStats.totalLogs;

  // Firmware info
  doc["firmware"] = "1.0.0";

  String output;
  serializeJson(doc, output);
  return output;
}

/*
 * Get all QSO logs as JSON
 */
String getQSOLogsJSON() {
  JsonDocument doc;
  JsonArray logsArray = doc["logs"].to<JsonArray>();

  int totalCount = 0;

  // Iterate through all log files
  File root = FileSystem.open("/logs");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());

        // Extract basename
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
          filename = filename.substring(lastSlash + 1);
        }

        // Process QSO log files
        if (filename.startsWith("qso_") && filename.endsWith(".json")) {
          File logFile = FileSystem.open(file.path(), "r");
          if (logFile) {
            String content = logFile.readString();
            logFile.close();

            JsonDocument logDoc;
            DeserializationError error = deserializeJson(logDoc, content);

            if (!error && logDoc.containsKey("logs")) {
              JsonArray qsos = logDoc["logs"].as<JsonArray>();
              for (JsonObject qso : qsos) {
                logsArray.add(qso);
                totalCount++;
              }
            }
          }
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  doc["total"] = totalCount;

  String output;
  serializeJson(doc, output);
  return output;
}

/*
 * Generate ADIF export file
 */
String generateADIF() {
  String adif = "";

  // ADIF header
  adif += "ADIF Export from VAIL SUMMIT\n";
  adif += "<PROGRAMID:11>VAIL SUMMIT\n";
  adif += "<PROGRAMVERSION:5>1.0.0\n";
  adif += "<ADIF_VER:5>3.1.4\n";
  adif += "<EOH>\n\n";

  // Iterate through all log files and generate ADIF records
  File root = FileSystem.open("/logs");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
          filename = filename.substring(lastSlash + 1);
        }

        if (filename.startsWith("qso_") && filename.endsWith(".json")) {
          File logFile = FileSystem.open(file.path(), "r");
          if (logFile) {
            String content = logFile.readString();
            logFile.close();

            JsonDocument logDoc;
            DeserializationError error = deserializeJson(logDoc, content);

            if (!error && logDoc.containsKey("logs")) {
              JsonArray qsos = logDoc["logs"].as<JsonArray>();
              for (JsonObject qso : qsos) {
                // Format each QSO as ADIF record
                const char* call = qso["callsign"] | "";
                if (strlen(call) > 0) {
                  adif += "<CALL:" + String(strlen(call)) + ">" + call + " ";
                }

                float freq = qso["frequency"] | 0.0f;
                if (freq > 0) {
                  String freqStr = String(freq, 6);
                  adif += "<FREQ:" + String(freqStr.length()) + ">" + freqStr + " ";
                }

                const char* mode = qso["mode"] | "";
                if (strlen(mode) > 0) {
                  adif += "<MODE:" + String(strlen(mode)) + ">" + mode + " ";
                }

                const char* date = qso["date"] | "";
                if (strlen(date) == 8) {
                  adif += "<QSO_DATE:8>" + String(date) + " ";
                }

                const char* time = qso["time_on"] | "";
                if (strlen(time) >= 4) {
                  String timeStr = String(time) + "00"; // Add seconds
                  adif += "<TIME_ON:6>" + timeStr + " ";
                }

                const char* rstS = qso["rst_sent"] | "";
                if (strlen(rstS) > 0) {
                  adif += "<RST_SENT:" + String(strlen(rstS)) + ">" + rstS + " ";
                }

                const char* rstR = qso["rst_rcvd"] | "";
                if (strlen(rstR) > 0) {
                  adif += "<RST_RCVD:" + String(strlen(rstR)) + ">" + rstR + " ";
                }

                const char* myGrid = qso["my_gridsquare"] | "";
                if (strlen(myGrid) > 0) {
                  adif += "<MY_GRIDSQUARE:" + String(strlen(myGrid)) + ">" + myGrid + " ";
                }

                const char* grid = qso["gridsquare"] | "";
                if (strlen(grid) > 0) {
                  adif += "<GRIDSQUARE:" + String(strlen(grid)) + ">" + grid + " ";
                }

                const char* myPota = qso["my_pota_ref"] | "";
                if (strlen(myPota) > 0) {
                  adif += "<MY_SIG:4>POTA <MY_SIG_INFO:" + String(strlen(myPota)) + ">" + myPota + " ";
                }

                const char* theirPota = qso["their_pota_ref"] | "";
                if (strlen(theirPota) > 0) {
                  adif += "<SIG:4>POTA <SIG_INFO:" + String(strlen(theirPota)) + ">" + theirPota + " ";
                }

                const char* notes = qso["notes"] | "";
                if (strlen(notes) > 0) {
                  adif += "<NOTES:" + String(strlen(notes)) + ">" + notes + " ";
                }

                adif += "<EOR>\n\n";
              }
            }
          }
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  return adif;
}

/*
 * Generate CSV export file
 */
String generateCSV() {
  String csv = "";

  // CSV header
  csv += "Date,Time,Callsign,Frequency,Band,Mode,RST Sent,RST Rcvd,My Grid,My POTA,Their Grid,Their POTA,Notes\n";

  // Iterate through all log files
  File root = FileSystem.open("/logs");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String filename = String(file.name());
        int lastSlash = filename.lastIndexOf('/');
        if (lastSlash >= 0) {
          filename = filename.substring(lastSlash + 1);
        }

        if (filename.startsWith("qso_") && filename.endsWith(".json")) {
          File logFile = FileSystem.open(file.path(), "r");
          if (logFile) {
            String content = logFile.readString();
            logFile.close();

            JsonDocument logDoc;
            DeserializationError error = deserializeJson(logDoc, content);

            if (!error && logDoc.containsKey("logs")) {
              JsonArray qsos = logDoc["logs"].as<JsonArray>();
              for (JsonObject qso : qsos) {
                // Format date YYYYMMDD to YYYY-MM-DD
                String date = qso["date"] | "";
                if (date.length() == 8) {
                  date = date.substring(0,4) + "-" + date.substring(4,6) + "-" + date.substring(6,8);
                }

                // Format time HHMM to HH:MM
                String time = qso["time_on"] | "";
                if (time.length() >= 4) {
                  time = time.substring(0,2) + ":" + time.substring(2,4);
                }

                csv += date + ",";
                csv += time + ",";
                csv += String(qso["callsign"] | "") + ",";
                csv += String(qso["frequency"] | 0.0f, 3) + ",";
                csv += String(qso["band"] | "") + ",";
                csv += String(qso["mode"] | "") + ",";
                csv += String(qso["rst_sent"] | "") + ",";
                csv += String(qso["rst_rcvd"] | "") + ",";
                csv += String(qso["my_gridsquare"] | "") + ",";
                csv += String(qso["my_pota_ref"] | "") + ",";
                csv += String(qso["gridsquare"] | "") + ",";
                csv += String(qso["their_pota_ref"] | "") + ",";

                // Escape notes field (may contain commas)
                String notes = qso["notes"] | "";
                if (notes.indexOf(',') >= 0 || notes.indexOf('"') >= 0) {
                  notes.replace("\"", "\"\"");
                  csv += "\"" + notes + "\"";
                } else {
                  csv += notes;
                }
                csv += "\n";
              }
            }
          }
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }

  return csv;
}

#endif // WEB_SERVER_H
