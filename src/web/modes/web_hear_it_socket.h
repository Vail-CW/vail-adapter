/*
 * Web Hear It Type It WebSocket Handler
 * Handles WebSocket connections for web-based "Hear It Type It" training mode
 * Sends callsign audio playback triggers and receives user answers
 */

#ifndef WEB_HEAR_IT_SOCKET_H
#define WEB_HEAR_IT_SOCKET_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Forward declarations
extern bool webHearItModeActive;
extern AsyncWebSocket hearItWebSocket;

// State tracking for web mode
extern String webCurrentCallsign;
extern int webCurrentWPM;
extern int webAttempts;
extern bool webWaitingForInput;

/*
 * WebSocket Event Handler for Hear It Type It Mode
 */
void onHearItWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch(type) {
    case WS_EVT_CONNECT:
      Serial.printf("Hear It WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      webHearItModeActive = true;
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("Hear It WebSocket client #%u disconnected\n", client->id());
      webHearItModeActive = false;

      // Exit web hear it mode if active
      extern MenuMode currentMode;
      if (currentMode == MODE_WEB_HEAR_IT) {
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

          if (type == "submit") {
            // User submitted an answer
            String answer = doc["answer"].as<String>();
            answer.toUpperCase();

            Serial.printf("User submitted answer: %s (correct: %s)\n",
                         answer.c_str(), webCurrentCallsign.c_str());

            bool correct = answer.equals(webCurrentCallsign);

            // Send result back to client
            JsonDocument response;
            response["type"] = "result";
            response["correct"] = correct;
            response["answer"] = webCurrentCallsign;

            String output;
            serializeJson(response, output);
            server->textAll(output);

            if (correct) {
              // Play success beep on device
              beep(TONE_SELECT, BEEP_LONG);

              // Trigger new callsign after a short delay
              extern bool webSkipRequested;
              webSkipRequested = true;
            } else {
              // Play error beep on device
              beep(TONE_ERROR, BEEP_LONG);
              webAttempts++;

              // Trigger replay after incorrect answer
              extern bool webReplayRequested;
              webReplayRequested = true;
            }
          }
          else if (type == "replay") {
            // User wants to replay the callsign
            Serial.println("Replay requested");
            beep(TONE_MENU_NAV, BEEP_SHORT);

            // Trigger replay - will be handled by web mode update loop
            extern bool webReplayRequested;
            webReplayRequested = true;
          }
          else if (type == "skip") {
            // User wants to skip to next callsign
            Serial.println("Skip requested");
            beep(TONE_MENU_NAV, BEEP_SHORT);

            // Trigger skip - will be handled by web mode update loop
            extern bool webSkipRequested;
            webSkipRequested = true;
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
 * Send new callsign info to browser
 * Includes morse code pattern for browser audio playback
 */
void sendHearItNewCallsign(String callsign, int wpm) {
  if (webHearItModeActive && hearItWebSocket.count() > 0) {
    // Convert callsign to morse code pattern
    String morsePattern = "";
    for (int i = 0; i < callsign.length(); i++) {
      const char* pattern = getMorseCode(callsign[i]);
      if (pattern != nullptr) {
        if (i > 0) morsePattern += " ";  // Space between letters
        morsePattern += pattern;
      }
    }

    JsonDocument doc;
    doc["type"] = "new_callsign";
    doc["callsign"] = callsign;
    doc["wpm"] = wpm;
    doc["morse"] = morsePattern;  // Send morse pattern for browser playback

    String output;
    serializeJson(doc, output);
    hearItWebSocket.textAll(output);

    Serial.printf("Sent new callsign to web: %s @ %d WPM (morse: %s)\n",
                  callsign.c_str(), wpm, morsePattern.c_str());
  }
}

/*
 * Notify browser that audio is playing
 */
void sendHearItPlaying() {
  if (webHearItModeActive && hearItWebSocket.count() > 0) {
    JsonDocument doc;
    doc["type"] = "playing";

    String output;
    serializeJson(doc, output);
    hearItWebSocket.textAll(output);

    Serial.println("Notified web: playing callsign");
  }
}

/*
 * Notify browser that it's ready for input
 */
void sendHearItReadyForInput() {
  if (webHearItModeActive && hearItWebSocket.count() > 0) {
    JsonDocument doc;
    doc["type"] = "ready_for_input";

    String output;
    serializeJson(doc, output);
    hearItWebSocket.textAll(output);

    Serial.println("Notified web: ready for input");
  }
}

#endif // WEB_HEAR_IT_SOCKET_H
