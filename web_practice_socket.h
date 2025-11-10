/*
 * Web Practice WebSocket Handler
 * Handles WebSocket connections for web-based practice mode
 * Receives timing data from browser and sends decoded results
 */

#ifndef WEB_PRACTICE_SOCKET_H
#define WEB_PRACTICE_SOCKET_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Forward declaration
extern bool webPracticeModeActive;
extern AsyncWebSocket practiceWebSocket;

/*
 * WebSocket Event Handler for Practice Mode
 */
void onPracticeWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  extern MorseDecoderAdaptive webPracticeDecoder;  // Decoder instance for web practice mode

  switch(type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      webPracticeModeActive = true;
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      webPracticeModeActive = false;

      // Exit web practice mode if active
      extern MenuMode currentMode;
      if (currentMode == MODE_WEB_PRACTICE) {
        currentMode = MODE_MAIN_MENU;
      }
      break;

    case WS_EVT_DATA: {
      AwsFrameInfo *info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;  // Null terminate
        String message = (char*)data;

        // Parse JSON message
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);

        if (!error) {
          String type = doc["type"].as<String>();

          if (type == "timing") {
            // Timing data from browser
            float duration = doc["duration"].as<float>();
            bool positive = doc["positive"].as<bool>();
            String key = doc["key"].as<String>();

            Serial.printf("Received timing: %s, duration: %.1f ms, positive: %d\n",
                         key.c_str(), duration, positive);

            // Feed timing to decoder
            if (positive) {
              webPracticeDecoder.addTiming(duration);
            } else {
              webPracticeDecoder.addTiming(-duration);  // Negative for silence
            }
          }
          else if (type == "start") {
            Serial.println("Practice mode start requested via WebSocket");
            webPracticeModeActive = true;
          }
        }
      }
      break;
    }

    case WS_EVT_ERROR:
      Serial.printf("WebSocket error: %u\n", *((uint16_t*)arg));
      break;

    case WS_EVT_PONG:
      // Ignore pong responses
      break;
  }
}

/*
 * Send decoded morse character to browser
 */
void sendPracticeDecoded(String morse, String text) {
  if (webPracticeModeActive && practiceWebSocket.count() > 0) {
    JsonDocument doc;
    doc["type"] = "decoded";
    doc["morse"] = morse;
    doc["text"] = text;

    String output;
    serializeJson(doc, output);
    practiceWebSocket.textAll(output);
  }
}

/*
 * Send WPM update to browser
 */
void sendPracticeWPM(float wpm) {
  if (webPracticeModeActive && practiceWebSocket.count() > 0) {
    JsonDocument doc;
    doc["type"] = "wpm";
    doc["value"] = wpm;

    String output;
    serializeJson(doc, output);
    practiceWebSocket.textAll(output);
  }
}

#endif // WEB_PRACTICE_SOCKET_H
