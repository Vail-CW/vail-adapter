/*
 * Web Server API Functions
 * Extracted from web_server.h for better maintainability
 * Handles JSON API endpoints for device status, QSO logs, and exports
 */

#ifndef WEB_SERVER_API_H
#define WEB_SERVER_API_H

#include <ArduinoJson.h>
#include <FS.h>
#include "../../core/config.h"

/*
 * Get device status as JSON
 * Returns battery, WiFi, QSO count, firmware version, and active mode
 */
String getDeviceStatusJSON() {
  extern bool isAPMode;  // From settings_wifi.h

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

  // WiFi status with mode information
  if (isAPMode) {
    doc["wifi"] = "AP Mode";
    doc["ip"] = WiFi.softAPIP().toString();
    doc["rssi"] = 0;  // Not applicable in AP mode
    doc["wifiMode"] = "AP";
  } else {
    doc["wifi"] = WiFi.isConnected() ? "Connected" : "Disconnected";
    doc["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "N/A";
    doc["rssi"] = WiFi.isConnected() ? WiFi.RSSI() : 0;
    doc["wifiMode"] = "STA";
  }

  // QSO count
  extern StorageStats storageStats;
  doc["qsoCount"] = storageStats.totalLogs;

  // Firmware info
  doc["firmware"] = FIRMWARE_VERSION;

  // Current active mode
  extern MenuMode currentMode;
  const char* modeStr = "Unknown";
  switch (currentMode) {
    case MODE_MAIN_MENU: modeStr = "Main Menu"; break;
    case MODE_TRAINING_MENU: modeStr = "Training Menu"; break;
    case MODE_WEB_PRACTICE: modeStr = "Web Practice"; break;
    case MODE_WEB_MEMORY_CHAIN: modeStr = "Web Memory Chain"; break;
    case MODE_WEB_HEAR_IT: modeStr = "Web Hear It Type It"; break;
    case MODE_PRACTICE: modeStr = "Practice"; break;
    case MODE_HEAR_IT_TYPE_IT: modeStr = "Hear It Type It"; break;
    case MODE_CW_ACADEMY_TRACK_SELECT: modeStr = "CW Academy"; break;
    case MODE_CW_ACADEMY_SESSION_SELECT: modeStr = "CW Academy"; break;
    case MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT: modeStr = "CW Academy"; break;
    case MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT: modeStr = "CW Academy"; break;
    case MODE_CW_ACADEMY_COPY_PRACTICE: modeStr = "CW Academy"; break;
    case MODE_CW_ACADEMY_SENDING_PRACTICE: modeStr = "CW Academy"; break;
    case MODE_CW_ACADEMY_QSO_PRACTICE: modeStr = "CW Academy"; break;
    case MODE_MORSE_SHOOTER: modeStr = "Morse Shooter"; break;
    case MODE_MORSE_MEMORY: modeStr = "Memory Chain"; break;
    case MODE_GAMES_MENU: modeStr = "Games Menu"; break;
    case MODE_RADIO_MENU: modeStr = "Radio Menu"; break;
    case MODE_RADIO_OUTPUT: modeStr = "Radio Output"; break;
    case MODE_CW_MEMORIES: modeStr = "CW Memories"; break;
    case MODE_SETTINGS_MENU: modeStr = "Settings Menu"; break;
    case MODE_WIFI_SETTINGS: modeStr = "WiFi Settings"; break;
    case MODE_CW_SETTINGS: modeStr = "CW Settings"; break;
    case MODE_VOLUME_SETTINGS: modeStr = "Volume Settings"; break;
    case MODE_CALLSIGN_SETTINGS: modeStr = "Callsign Settings"; break;
    case MODE_VAIL_REPEATER: modeStr = "Vail Repeater"; break;
    case MODE_BLUETOOTH: modeStr = "Bluetooth"; break;
    case MODE_TOOLS_MENU: modeStr = "Tools Menu"; break;
    case MODE_QSO_LOGGER_MENU: modeStr = "QSO Logger Menu"; break;
    case MODE_QSO_LOG_ENTRY: modeStr = "QSO Logger"; break;
    case MODE_QSO_VIEW_LOGS: modeStr = "View Logs"; break;
    case MODE_QSO_STATISTICS: modeStr = "QSO Statistics"; break;
    case MODE_QSO_LOGGER_SETTINGS: modeStr = "QSO Settings"; break;
    default: modeStr = "Device Mode"; break;
  }
  doc["activeMode"] = modeStr;

  String output;
  serializeJson(doc, output);
  return output;
}

/*
 * Get all QSO logs as JSON
 */
String getQSOLogsJSON() {
  extern fs::SPIFFSFS FileSystem;

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
              JsonArray logs = logDoc["logs"];
              for (JsonVariant v : logs) {
                logsArray.add(v);
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

  doc["count"] = totalCount;

  String output;
  serializeJson(doc, output);
  return output;
}

/*
 * Generate ADIF export
 */
String generateADIF() {
  extern fs::SPIFFSFS FileSystem;

  String adif = "ADIF Export from VAIL SUMMIT\n";
  adif += "<PROGRAMID:11>VAIL SUMMIT\n";
  adif += "<PROGRAMVERSION:" + String(strlen(FIRMWARE_VERSION)) + ">" + String(FIRMWARE_VERSION) + "\n";
  adif += "<ADIF_VER:5>3.1.4\n";
  adif += "<EOH>\n\n";

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
              JsonArray logs = logDoc["logs"];
              for (JsonVariant v : logs) {
                JsonObject qso = v.as<JsonObject>();

                // Required fields
                if (qso.containsKey("callsign") && qso["callsign"].as<String>().length() > 0) {
                  String call = qso["callsign"].as<String>();
                  adif += "<CALL:" + String(call.length()) + ">" + call + " ";
                }

                if (qso.containsKey("frequency")) {
                  String freq = String(qso["frequency"].as<float>(), 3);
                  adif += "<FREQ:" + String(freq.length()) + ">" + freq + " ";
                }

                if (qso.containsKey("mode") && qso["mode"].as<String>().length() > 0) {
                  String mode = qso["mode"].as<String>();
                  adif += "<MODE:" + String(mode.length()) + ">" + mode + " ";
                }

                if (qso.containsKey("date") && qso["date"].as<String>().length() > 0) {
                  String date = qso["date"].as<String>();
                  adif += "<QSO_DATE:" + String(date.length()) + ">" + date + " ";
                }

                if (qso.containsKey("time_on") && qso["time_on"].as<String>().length() > 0) {
                  String time = qso["time_on"].as<String>();
                  // Convert HHMM to HHMMSS
                  if (time.length() == 4) {
                    time += "00";
                  }
                  adif += "<TIME_ON:" + String(time.length()) + ">" + time + " ";
                }

                // Optional fields
                if (qso.containsKey("rst_sent") && qso["rst_sent"].as<String>().length() > 0) {
                  String rst = qso["rst_sent"].as<String>();
                  adif += "<RST_SENT:" + String(rst.length()) + ">" + rst + " ";
                }

                if (qso.containsKey("rst_rcvd") && qso["rst_rcvd"].as<String>().length() > 0) {
                  String rst = qso["rst_rcvd"].as<String>();
                  adif += "<RST_RCVD:" + String(rst.length()) + ">" + rst + " ";
                }

                if (qso.containsKey("gridsquare") && qso["gridsquare"].as<String>().length() > 0) {
                  String grid = qso["gridsquare"].as<String>();
                  adif += "<GRIDSQUARE:" + String(grid.length()) + ">" + grid + " ";
                }

                if (qso.containsKey("my_gridsquare") && qso["my_gridsquare"].as<String>().length() > 0) {
                  String grid = qso["my_gridsquare"].as<String>();
                  adif += "<MY_GRIDSQUARE:" + String(grid.length()) + ">" + grid + " ";
                }

                // POTA support
                if (qso.containsKey("my_pota_ref") && qso["my_pota_ref"].as<String>().length() > 0) {
                  String pota = qso["my_pota_ref"].as<String>();
                  adif += "<MY_SIG:4>POTA ";
                  adif += "<MY_SIG_INFO:" + String(pota.length()) + ">" + pota + " ";
                }

                if (qso.containsKey("their_pota_ref") && qso["their_pota_ref"].as<String>().length() > 0) {
                  String pota = qso["their_pota_ref"].as<String>();
                  adif += "<SIG:4>POTA ";
                  adif += "<SIG_INFO:" + String(pota.length()) + ">" + pota + " ";
                }

                adif += "<EOR>\n";
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
 * Generate CSV export
 */
String generateCSV() {
  extern fs::SPIFFSFS FileSystem;

  String csv = "Callsign,Frequency,Mode,Band,Date,Time,RST Sent,RST Rcvd,Grid,My Grid,My POTA,Their POTA,Notes\n";

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
              JsonArray logs = logDoc["logs"];
              for (JsonVariant v : logs) {
                JsonObject qso = v.as<JsonObject>();

                csv += qso["callsign"].as<String>() + ",";
                csv += String(qso["frequency"].as<float>(), 3) + ",";
                csv += qso["mode"].as<String>() + ",";
                csv += qso["band"].as<String>() + ",";
                csv += qso["date"].as<String>() + ",";
                csv += qso["time_on"].as<String>() + ",";
                csv += qso["rst_sent"].as<String>() + ",";
                csv += qso["rst_rcvd"].as<String>() + ",";
                csv += qso["gridsquare"].as<String>() + ",";
                csv += qso["my_gridsquare"].as<String>() + ",";
                csv += qso["my_pota_ref"].as<String>() + ",";
                csv += qso["their_pota_ref"].as<String>() + ",";

                // Escape quotes in notes
                String notes = qso["notes"].as<String>();
                notes.replace("\"", "\"\"");
                csv += "\"" + notes + "\"";

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

#endif // WEB_SERVER_API_H
