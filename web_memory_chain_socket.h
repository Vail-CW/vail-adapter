/*
 * Web Memory Chain - WebSocket Handler
 * Handles real-time communication between browser and device during Memory Chain gameplay
 */

#ifndef WEB_MEMORY_CHAIN_SOCKET_H
#define WEB_MEMORY_CHAIN_SOCKET_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "morse_decoder_adaptive.h"

// WebSocket instance for Memory Chain
AsyncWebSocket memoryChainWebSocket("/ws/memory-chain");

// WebSocket state
bool webMemoryChainModeActive = false;
MorseDecoderAdaptive webMemoryChainDecoder(15.0f);  // Will be updated with user settings

/*
 * WebSocket Event Handler for Memory Chain Mode
 */
void onMemoryChainWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch(type) {
    case WS_EVT_CONNECT:
      Serial.printf("Memory Chain WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      webMemoryChainModeActive = true;

      // Note: Initial state and game start will be triggered from the main loop
      // We can't call functions from web_memory_chain_mode.h here due to include order
      // Instead, we set a flag that the main update loop will check
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("Memory Chain WebSocket client #%u disconnected\n", client->id());
      webMemoryChainModeActive = false;

      // Exit web memory chain mode if active
      extern MenuMode currentMode;
      if (currentMode == MODE_WEB_MEMORY_CHAIN) {
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
            // Timing data from browser (morse input)
            float duration = doc["duration"].as<float>();
            bool positive = doc["positive"].as<bool>();

            Serial.printf("Memory Chain timing: duration: %.1f ms, positive: %d\n",
                         duration, positive);

            // Feed timing to decoder
            if (positive) {
              webMemoryChainDecoder.addTiming(duration);
            } else {
              webMemoryChainDecoder.addTiming(-duration);  // Negative for silence
            }
          }
        }
      }
      break;
    }

    case WS_EVT_ERROR:
      Serial.printf("Memory Chain WebSocket error: %u\n", *((uint16_t*)arg));
      break;

    case WS_EVT_PONG:
      // Ignore pong responses
      break;
  }
}

/*
 * Send game state update to browser
 * States: ready, playing, listening, feedback, game_over
 */
void sendMemoryChainState(String state, String description) {
  if (webMemoryChainModeActive && memoryChainWebSocket.count() > 0) {
    JsonDocument doc;
    doc["type"] = "state";
    doc["state"] = state;
    doc["description"] = description;

    String output;
    serializeJson(doc, output);
    memoryChainWebSocket.textAll(output);

    Serial.printf("Memory Chain state: %s - %s\n", state.c_str(), description.c_str());
  }
}

/*
 * Send sequence to browser
 * characters: The sequence (e.g., "ETIA")
 * show: Whether to display characters or hide them
 */
void sendMemoryChainSequence(String characters, bool show) {
  if (webMemoryChainModeActive && memoryChainWebSocket.count() > 0) {
    JsonDocument doc;
    doc["type"] = "sequence";
    doc["characters"] = characters;
    doc["show"] = show;

    String output;
    serializeJson(doc, output);
    memoryChainWebSocket.textAll(output);

    Serial.printf("Memory Chain sequence: %s (show: %d)\n", characters.c_str(), show);
  }
}

/*
 * Send score update to browser
 */
void sendMemoryChainScore(int current, int high) {
  if (webMemoryChainModeActive && memoryChainWebSocket.count() > 0) {
    JsonDocument doc;
    doc["type"] = "score";
    doc["current"] = current;
    doc["high"] = high;

    String output;
    serializeJson(doc, output);
    memoryChainWebSocket.textAll(output);
  }
}

/*
 * Send feedback (correct/wrong) to browser
 */
void sendMemoryChainFeedback(bool correct) {
  if (webMemoryChainModeActive && memoryChainWebSocket.count() > 0) {
    JsonDocument doc;
    doc["type"] = "feedback";
    doc["correct"] = correct;

    String output;
    serializeJson(doc, output);
    memoryChainWebSocket.textAll(output);

    Serial.printf("Memory Chain feedback: %s\n", correct ? "correct" : "wrong");
  }
}

/*
 * Send game over message to browser
 */
void sendMemoryChainGameOver(int finalScore, String reason) {
  if (webMemoryChainModeActive && memoryChainWebSocket.count() > 0) {
    JsonDocument doc;
    doc["type"] = "game_over";
    doc["finalScore"] = finalScore;
    doc["reason"] = reason;

    String output;
    serializeJson(doc, output);
    memoryChainWebSocket.textAll(output);

    Serial.printf("Memory Chain game over: score %d, reason: %s\n", finalScore, reason.c_str());
  }
}

#endif // WEB_MEMORY_CHAIN_SOCKET_H
