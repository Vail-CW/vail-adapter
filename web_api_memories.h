// web_api_memories.h
// CW Memories API Endpoints for VAIL SUMMIT
// Handles CRUD operations for CW memory presets

#ifndef WEB_API_MEMORIES_H
#define WEB_API_MEMORIES_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// External declarations
extern CWMemoryPreset cwMemories[CW_MEMORY_MAX_SLOTS];
extern void saveCWMemory(int slot);
extern void deleteCWMemory(int slot);
extern void previewCWMemory(int slot);
extern bool isValidMorseMessage(const char* message);
extern bool queueRadioMessage(const char* message);

// ============================================
// Setup Function - Register All CW Memories API Endpoints
// ============================================

void setupMemoriesAPI(AsyncWebServer &webServer) {

  // ============================================
  // GET /api/memories/list
  // Returns all 10 memory presets
  // ============================================
  webServer.on("/api/memories/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray presets = doc["presets"].to<JsonArray>();

    for (int i = 0; i < CW_MEMORY_MAX_SLOTS; i++) {
      JsonObject preset = presets.add<JsonObject>();
      preset["slot"] = i + 1;
      preset["label"] = cwMemories[i].isEmpty ? "" : cwMemories[i].label;
      preset["message"] = cwMemories[i].isEmpty ? "" : cwMemories[i].message;
      preset["isEmpty"] = cwMemories[i].isEmpty;
    }

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // ============================================
  // POST /api/memories/create
  // Creates or overwrites a memory preset
  // Body: { "slot": 1-10, "label": "...", "message": "..." }
  // ============================================
  webServer.on("/api/memories/create", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Validate slot
      int slot = doc["slot"] | 0;
      if (slot < 1 || slot > CW_MEMORY_MAX_SLOTS) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Slot out of range (1-10)\"}");
        return;
      }

      const char* label = doc["label"] | "";
      const char* message = doc["message"] | "";

      // Validate label
      if (strlen(label) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Label cannot be empty\"}");
        return;
      }
      if (strlen(label) > CW_MEMORY_LABEL_MAX_LENGTH) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Label too long (max 15 chars)\"}");
        return;
      }

      // Validate message
      if (strlen(message) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Message cannot be empty\"}");
        return;
      }
      if (strlen(message) > CW_MEMORY_MESSAGE_MAX_LENGTH) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Message too long (max 100 chars)\"}");
        return;
      }
      if (!isValidMorseMessage(message)) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Message contains invalid characters\"}");
        return;
      }

      // Create/update memory
      int slotIndex = slot - 1;
      strlcpy(cwMemories[slotIndex].label, label, CW_MEMORY_LABEL_MAX_LENGTH + 1);
      strlcpy(cwMemories[slotIndex].message, message, CW_MEMORY_MESSAGE_MAX_LENGTH + 1);
      cwMemories[slotIndex].isEmpty = false;

      saveCWMemory(slotIndex);

      Serial.print("Created/updated memory slot ");
      Serial.print(slot);
      Serial.print(": ");
      Serial.println(label);

      request->send(200, "application/json", "{\"success\":true}");
    });

  // ============================================
  // POST /api/memories/update
  // Updates an existing memory preset (same as create)
  // Body: { "slot": 1-10, "label": "...", "message": "..." }
  // ============================================
  webServer.on("/api/memories/update", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Validate slot
      int slot = doc["slot"] | 0;
      if (slot < 1 || slot > CW_MEMORY_MAX_SLOTS) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Slot out of range (1-10)\"}");
        return;
      }

      const char* label = doc["label"] | "";
      const char* message = doc["message"] | "";

      // Validate label
      if (strlen(label) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Label cannot be empty\"}");
        return;
      }
      if (strlen(label) > CW_MEMORY_LABEL_MAX_LENGTH) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Label too long (max 15 chars)\"}");
        return;
      }

      // Validate message
      if (strlen(message) == 0) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Message cannot be empty\"}");
        return;
      }
      if (strlen(message) > CW_MEMORY_MESSAGE_MAX_LENGTH) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Message too long (max 100 chars)\"}");
        return;
      }
      if (!isValidMorseMessage(message)) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Message contains invalid characters\"}");
        return;
      }

      // Update memory
      int slotIndex = slot - 1;
      strlcpy(cwMemories[slotIndex].label, label, CW_MEMORY_LABEL_MAX_LENGTH + 1);
      strlcpy(cwMemories[slotIndex].message, message, CW_MEMORY_MESSAGE_MAX_LENGTH + 1);
      cwMemories[slotIndex].isEmpty = false;

      saveCWMemory(slotIndex);

      Serial.print("Updated memory slot ");
      Serial.print(slot);
      Serial.print(": ");
      Serial.println(label);

      request->send(200, "application/json", "{\"success\":true}");
    });

  // ============================================
  // POST /api/memories/delete
  // Deletes a memory preset (clears the slot)
  // Body: { "slot": 1-10 }
  // ============================================
  webServer.on("/api/memories/delete", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Validate slot
      int slot = doc["slot"] | 0;
      if (slot < 1 || slot > CW_MEMORY_MAX_SLOTS) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Slot out of range (1-10)\"}");
        return;
      }

      int slotIndex = slot - 1;

      // Delete memory
      deleteCWMemory(slotIndex);

      Serial.print("Deleted memory slot ");
      Serial.println(slot);

      request->send(200, "application/json", "{\"success\":true}");
    });

  // ============================================
  // POST /api/memories/preview
  // Plays a memory preset on device speaker (not radio output)
  // Body: { "slot": 1-10 }
  // ============================================
  webServer.on("/api/memories/preview", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Validate slot
      int slot = doc["slot"] | 0;
      if (slot < 1 || slot > CW_MEMORY_MAX_SLOTS) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Slot out of range (1-10)\"}");
        return;
      }

      int slotIndex = slot - 1;

      // Check if slot is empty
      if (cwMemories[slotIndex].isEmpty) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Slot is empty\"}");
        return;
      }

      // Preview memory (plays on device speaker)
      Serial.print("Previewing memory slot ");
      Serial.print(slot);
      Serial.print(" from web interface: ");
      Serial.println(cwMemories[slotIndex].message);

      // NOTE: This preview function blocks during playback
      // For better UX, you might want to make this non-blocking in the future
      previewCWMemory(slotIndex);

      request->send(200, "application/json", "{\"success\":true}");
    });

  // ============================================
  // POST /api/memories/send
  // Queues a memory preset for transmission via radio output
  // Body: { "slot": 1-10 }
  // ============================================
  webServer.on("/api/memories/send", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);

      if (error) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
      }

      // Validate slot
      int slot = doc["slot"] | 0;
      if (slot < 1 || slot > CW_MEMORY_MAX_SLOTS) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Slot out of range (1-10)\"}");
        return;
      }

      int slotIndex = slot - 1;

      // Check if slot is empty
      if (cwMemories[slotIndex].isEmpty) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Slot is empty\"}");
        return;
      }

      // Queue message for transmission
      if (queueRadioMessage(cwMemories[slotIndex].message)) {
        Serial.print("Queued memory slot ");
        Serial.print(slot);
        Serial.print(" for transmission: ");
        Serial.println(cwMemories[slotIndex].message);
        request->send(200, "application/json", "{\"success\":true}");
      } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Message queue is full\"}");
      }
    });
}

#endif // WEB_API_MEMORIES_H
