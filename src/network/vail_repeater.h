/*
 * Vail Repeater Module
 * WebSocket client for vailmorse.com morse code repeater
 *
 * REQUIRED LIBRARIES (install via Arduino Library Manager):
 * 1. WebSockets by Markus Sattler
 * 2. ArduinoJson by Benoit Blanchon
 */

#ifndef VAIL_REPEATER_H
#define VAIL_REPEATER_H

#include <WiFi.h>
#include <WiFiClientSecure.h>

// Set to 1 if you have the libraries installed, 0 if not
#define VAIL_ENABLED 1

#if VAIL_ENABLED
  #include <WebSocketsClient.h>
  #include <ArduinoJson.h>
  #include <deque>
#endif

#include "../core/config.h"
#include "../core/task_manager.h"  // For dual-core audio API
#include "../settings/settings_cw.h"
#include "internet_check.h"

// Default channel - always defined
String vailChannel = "General";

// Preferences for Vail settings persistence
Preferences vailPrefs;

// Load Vail settings from flash
void loadVailSettings() {
  vailPrefs.begin("vail", true);  // Read-only mode
  vailChannel = vailPrefs.getString("room", "General");
  vailPrefs.end();
  Serial.printf("[Vail] Loaded room: %s\n", vailChannel.c_str());
}

// Save Vail settings to flash
void saveVailSettings() {
  vailPrefs.begin("vail", false);  // Read-write mode
  vailPrefs.putString("room", vailChannel);
  vailPrefs.end();
  Serial.printf("[Vail] Saved room: %s\n", vailChannel.c_str());
}

// User identification
String vailCallsign = "GUEST";  // Default callsign (user can configure)
uint8_t vailTxTone = 72;        // MIDI note 72 = C5 (523 Hz) - default CW tone

// Audio settings
// When true, all received morse plays at user's local cwTone frequency for consistency
// When false, received morse plays at sender's TX tone frequency (original behavior)
#define VAIL_USE_LOCAL_TONE_FOR_RECEIVE true

#if VAIL_ENABLED

// SSL client for WebSocket
WiFiClientSecure wifiClient;

// Vail repeater state
enum VailState {
  VAIL_DISCONNECTED,
  VAIL_CONNECTING,
  VAIL_CONNECTED,
  VAIL_ERROR
};

// Vail globals
WebSocketsClient webSocket;
VailState vailState = VAIL_DISCONNECTED;
VailState lastVailState = VAIL_DISCONNECTED;
String vailServer = "vailmorse.com";
int vailPort = 443;  // WSS (secure WebSocket)
int connectedClients = 0;
int lastConnectedClients = 0;
String statusText = "";
// Note: needsUIRedraw removed - LVGL handles all UI updates via updateVailScreenLVGL()
unsigned long lastKeepaliveTime = 0;  // Track last keepalive message

// Transmit state
bool vailIsTransmitting = false;
unsigned long vailTxStartTime = 0;
bool vailTxToneOn = false;
unsigned long vailTxElementStart = 0;
std::vector<uint16_t> vailTxDurations;
// Echo filtering: Track recent transmission timestamps to filter echoes
std::deque<int64_t> recentTxTimestamps;
const size_t MAX_TX_TIMESTAMPS = 20;
int64_t vailToneStartTimestamp = 0;  // Timestamp when current tone started

// Keyer state for Vail (similar to practice mode)
bool vailDitPressed = false;
bool vailDahPressed = false;
bool vailKeyerActive = false;
bool vailSendingDit = false;
bool vailSendingDah = false;
bool vailInSpacing = false;
bool vailDitMemory = false;
bool vailDahMemory = false;
unsigned long vailElementStartTime = 0;
int vailDitDuration = 0;

// Receive state
struct VailMessage {
  int64_t timestamp;
  uint16_t clients;
  uint8_t txTone;  // Sender's TX tone (MIDI note number)
  std::vector<uint16_t> durations;
};

std::vector<VailMessage> rxQueue;
unsigned long playbackDelay = 500;  // 500ms delay for network jitter
int64_t clockSkew = 0;  // Offset to convert millis() to server time
int clockSkewSamples = 0;  // Number of clock skew samples received
const float CLOCK_SKEW_ALPHA = 0.3f;  // Exponential moving average weight

// Playback state machine variables
static bool isPlaying = false;
static size_t playbackIndex = 0;
static unsigned long playbackElementStart = 0;
static int playbackToneFrequency = 0;  // Current playback tone frequency

// Chat mode state
bool vailChatMode = false;  // false = vail info, true = chat view
bool hasUnreadMessages = false;  // Indicator for new messages
struct ChatMessage {
  String callsign;
  String message;
  unsigned long timestamp;
};
std::vector<ChatMessage> chatHistory;
String chatInput = "";
unsigned long chatLastBlink = 0;
bool chatCursorVisible = true;
const int MAX_CHAT_MESSAGES = 20;  // Keep last 20 messages
const int MAX_CHAT_INPUT = 40;     // Max 40 chars per message

// Room selection state
bool vailRoomSelectionMode = false;  // Room selection menu active
int roomMenuSelection = 0;  // Current menu selection
bool roomCustomInput = false;  // Custom room name input mode
String roomInput = "";
unsigned long roomLastBlink = 0;
bool roomCursorVisible = true;
struct RoomInfo {
  String name;
  int users;
  bool isPrivate;
};
std::vector<RoomInfo> activeRooms;
const int MAX_ROOM_NAME = 30;  // Max room name length

// User list state
bool vailUserListMode = false;  // User list view active
struct UserInfo {
  String callsign;
  uint8_t txTone;
};
std::vector<UserInfo> connectedUsers;

// Forward declarations
void startVailRepeater(LGFX &display);
void drawVailUI(LGFX &display);
void drawChatUI(LGFX &display);
void drawRoomSelectionUI(LGFX &display);
void drawRoomInputUI(LGFX &display);
void drawUserListUI(LGFX &display);
int handleVailInput(char key, LGFX &display);
int handleChatInput(char key, LGFX &display);
int handleRoomSelectionInput(char key, LGFX &display);
int handleRoomInputInput(char key, LGFX &display);
int handleUserListInput(char key, LGFX &display);
void updateVailRepeater();
void connectToVail(String channel);
void disconnectFromVail();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void sendVailMessage(std::vector<uint16_t> durations, int64_t timestamp = 0);
void sendChatMessage(String message);
void addChatMessage(String callsign, String message);
void sendInitialMessage();
void sendKeepalive();
void processReceivedMessage(String jsonPayload);
void playbackMessages();
int64_t getCurrentTimestamp();
void updateVailPaddles();

// Convert MIDI note number to frequency (Hz)
// Formula: frequency = 440 * 2^((note - 69) / 12)
float midiNoteToFrequency(uint8_t note) {
  if (note == 0) return 440.0;  // Default to A4 if not specified
  return 440.0 * pow(2.0, (note - 69) / 12.0);
}

// Get current timestamp in milliseconds (Unix epoch)
int64_t getCurrentTimestamp() {
  // Get time from NTP if available, otherwise use millis() with offset
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t timestamp = (int64_t)(tv.tv_sec) * 1000LL + (int64_t)(tv.tv_usec / 1000);

  // If timestamp is unreasonably small, we don't have NTP time yet
  // Use server clock skew to estimate
  if (timestamp < 1000000000000LL) {
    // No valid time, use millis() + clock skew
    timestamp = (int64_t)millis() + clockSkew;
  }

  return timestamp;
}

// Forward declaration of drawHeader (defined in main .ino file)
void drawHeader();

// Start Vail repeater mode
void startVailRepeater(LGFX &display) {
  vailState = VAIL_DISCONNECTED;
  statusText = "Enter channel name";
  vailIsTransmitting = false;
  rxQueue.clear();
  vailTxDurations.clear();

  // Initialize keyer state
  vailKeyerActive = false;
  vailInSpacing = false;
  vailDitMemory = false;
  vailDahMemory = false;
  vailDitDuration = DIT_DURATION(cwSpeed);

  // Initialize chat mode
  vailChatMode = false;
  hasUnreadMessages = false;
  chatInput = "";
  chatHistory.clear();

  // Initialize room selection
  vailRoomSelectionMode = false;
  roomCustomInput = false;
  roomMenuSelection = 0;
  roomInput = "";
  activeRooms.clear();

  // Initialize user list
  vailUserListMode = false;
  connectedUsers.clear();

  // UI is now handled by LVGL - see lv_mode_screens.h
}

// Connect to Vail repeater
void connectToVail(String channel) {
  // Check internet connectivity first
  InternetStatus inetStatus = getInternetStatus();
  if (inetStatus != INET_CONNECTED) {
    Serial.println("[Vail] Cannot connect - no internet connectivity");
    vailState = VAIL_ERROR;
    if (inetStatus == INET_WIFI_ONLY) {
      statusText = "WiFi connected but no internet";
    } else {
      statusText = "No WiFi connection";
    }
    return;
  }

  vailChannel = channel;
  saveVailSettings();  // Persist room selection for next boot
  vailState = VAIL_CONNECTING;
  statusText = "Connecting...";

  Serial.print("Connecting to Vail repeater: ");
  Serial.println(channel);

  // WebSocket connection with subprotocol
  String path = "/chat?repeater=" + channel;

  Serial.println("WebSocket connecting...");
  Serial.print("URL: wss://");
  Serial.print(vailServer);
  Serial.print(":");
  Serial.print(vailPort);
  Serial.println(path);

  // Set event handler first
  webSocket.onEvent(webSocketEvent);

  // Enable aggressive heartbeat to prevent disconnections
  // Parameters: ping interval (10s), pong timeout (5s), max retries (3)
  // Aggressive settings prevent Cloud Run and load balancer timeouts
  webSocket.enableHeartbeat(10000, 5000, 3);

  // Set subprotocol using extra headers (WebSocketsClient method)
  webSocket.setExtraHeaders("Sec-WebSocket-Protocol: json.vailmorse.com");

  // Simple beginSSL - library should handle SSL automatically
  webSocket.beginSSL(vailServer.c_str(), vailPort, path.c_str());

  // Set reconnect interval
  webSocket.setReconnectInterval(5000);

  Serial.println("WebSocket setup complete");
}

// Disconnect from Vail
void disconnectFromVail() {
  if (vailState == VAIL_DISCONNECTED) {
    return;  // Already disconnected
  }

  Serial.println("[Vail] Disconnecting...");

  // Stop any ongoing tone playback (use non-blocking request API)
  requestStopTone();

  // Send disconnect to server
  webSocket.disconnect();

  // Wait for disconnect to complete (with timeout)
  // This ensures the server receives the close frame
  unsigned long startTime = millis();
  const unsigned long timeout = 500;  // 500ms timeout

  while (vailState != VAIL_DISCONNECTED && (millis() - startTime) < timeout) {
    webSocket.loop();  // Process pending WebSocket events
    delay(10);
  }

  // Force state to disconnected if timeout occurred
  vailState = VAIL_DISCONNECTED;
  statusText = "Disconnected";

  // Stop any repeater playback
  isPlaying = false;
  playbackToneFrequency = 0;

  // Clear all queues and state to prevent stale data on reconnect
  rxQueue.clear();
  vailTxDurations.clear();
  recentTxTimestamps.clear();
  chatHistory.clear();
  connectedUsers.clear();
  activeRooms.clear();
  clockSkewSamples = 0;  // Reset clock sync on disconnect
  vailIsTransmitting = false;
  vailKeyerActive = false;
  vailInSpacing = false;
  vailDitMemory = false;
  vailDahMemory = false;

  Serial.println("[Vail] Disconnected and state cleared");
}

// WebSocket event handler
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected");
      vailState = VAIL_DISCONNECTED;
      statusText = "Disconnected";
      break;

    case WStype_CONNECTED:
      {
        Serial.println("[WS] Connected");
        vailState = VAIL_CONNECTED;
        statusText = "Connected";

        // Get the URL we connected to
        String url = String((char*)payload);
        Serial.print("[WS] Connected to: ");
        Serial.println(url);

        // Send initial connection message (required by API)
        sendInitialMessage();
        lastKeepaliveTime = millis();  // Reset keepalive timer
      }
      break;

    case WStype_TEXT:
      Serial.printf("[WS] Received: %s\n", payload);
      processReceivedMessage(String((char*)payload));
      break;

    case WStype_ERROR:
      Serial.println("[WS] Error");
      vailState = VAIL_ERROR;
      statusText = "Connection error";
      break;

    case WStype_PING:
      Serial.println("[WS] Ping");
      break;

    case WStype_PONG:
      Serial.println("[WS] Pong");
      break;

    default:
      break;
  }
}

// Process received JSON message
void processReceivedMessage(String jsonPayload) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, jsonPayload);

  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }

  VailMessage msg;
  msg.timestamp = doc["Timestamp"].as<int64_t>();
  msg.clients = doc["Clients"].as<uint16_t>();
  msg.txTone = doc["TxTone"] | 69;  // Default to MIDI note 69 (A4 = 440Hz) if not specified

  // Update client count (LVGL will update on next frame)
  if (connectedClients != msg.clients) {
    connectedClients = msg.clients;
  }

  // Parse Users array (optional - for future UI enhancements)
  if (doc.containsKey("Users")) {
    JsonArray users = doc["Users"];
    Serial.print("Connected users: ");
    for (JsonVariant user : users) {
      Serial.print(user.as<String>());
      Serial.print(" ");
    }
    Serial.println();
  }

  // Parse UsersInfo array (optional - detailed user info with TX tones)
  if (doc.containsKey("UsersInfo")) {
    connectedUsers.clear();
    JsonArray usersInfo = doc["UsersInfo"];
    Serial.println("User details:");
    for (JsonVariant userInfo : usersInfo) {
      String callsign = userInfo["callsign"] | "Unknown";
      uint8_t txTone = userInfo["txTone"] | 69;

      // Store user info
      UserInfo user;
      user.callsign = callsign;
      user.txTone = txTone;
      connectedUsers.push_back(user);

      Serial.print("  - ");
      Serial.print(callsign);
      Serial.print(" @ ");
      Serial.print(midiNoteToFrequency(txTone));
      Serial.println(" Hz");
    }
  }

  // Parse Rooms array (active public rooms)
  if (doc.containsKey("Rooms")) {
    activeRooms.clear();
    JsonArray rooms = doc["Rooms"];
    for (JsonVariant roomVar : rooms) {
      RoomInfo room;
      room.name = roomVar["name"] | "Unknown";
      room.users = roomVar["users"] | 0;
      room.isPrivate = roomVar["private"] | false;
      activeRooms.push_back(room);
    }
    Serial.print("Active rooms: ");
    Serial.println(activeRooms.size());
  }

  // Check for text chat message
  if (doc.containsKey("Text") && !doc["Text"].isNull()) {
    String text = doc["Text"].as<String>();
    String callsign = doc["Callsign"] | "Unknown";

    // Don't add our own messages again (we already added when sending)
    if (callsign != vailCallsign) {
      addChatMessage(callsign, text);
    }
  }

  JsonArray durations = doc["Duration"];
  if (durations.size() > 0) {
    // Check if this is our own message echoed back (within 2000ms tolerance for network round-trip)
    // Use queue of recent timestamps to handle rapid iambic keying
    bool isEcho = false;
    for (int64_t ts : recentTxTimestamps) {
      if (llabs(msg.timestamp - ts) < 2000) {
        isEcho = true;
        break;
      }
    }
    if (isEcho) {
      Serial.println("Ignoring echo of our own transmission");
      return;
    }

    for (uint16_t duration : durations) {
      msg.durations.push_back(duration);
    }

    // Add to receive queue with playback delay
    rxQueue.push_back(msg);

    Serial.print("Queued message: ");
    Serial.print(msg.durations.size());
    Serial.print(" elements at tone ");
    Serial.println(msg.txTone);
  } else {
    // Empty duration = clock sync message
    // Calculate offset from server time to our millis() with exponential moving average
    int64_t newSkew = msg.timestamp - (int64_t)millis();

    if (clockSkewSamples == 0) {
      // First sample - use directly
      clockSkew = newSkew;
    } else {
      // Apply exponential moving average for stability
      clockSkew = (int64_t)(CLOCK_SKEW_ALPHA * newSkew + (1.0f - CLOCK_SKEW_ALPHA) * clockSkew);
    }
    clockSkewSamples++;

    Serial.print("Clock sync: server=");
    Serial.print((long)msg.timestamp);
    Serial.print(" millis=");
    Serial.print((long)millis());
    Serial.print(" skew=");
    Serial.print((long)clockSkew);
    Serial.print(" (samples=");
    Serial.print(clockSkewSamples);
    Serial.println(")");
  }
}

// Send initial connection message (required by new API)
void sendInitialMessage() {
  StaticJsonDocument<256> doc;
  doc["Timestamp"] = getCurrentTimestamp();
  JsonArray duration = doc.createNestedArray("Duration");  // Empty array
  doc["Callsign"] = vailCallsign;
  doc["TxTone"] = vailTxTone;
  doc["Private"] = false;  // Public room
  doc["Decoder"] = false;  // Morse decoder disabled

  String output;
  serializeJson(doc, output);

  Serial.print("Sending initial message: ");
  Serial.println(output);

  webSocket.sendTXT(output);
}

// Send keepalive message (required every 30 seconds)
void sendKeepalive() {
  if (vailState != VAIL_CONNECTED) {
    return;
  }

  StaticJsonDocument<256> doc;
  doc["Timestamp"] = getCurrentTimestamp();
  JsonArray duration = doc.createNestedArray("Duration");  // Empty array
  doc["Callsign"] = vailCallsign;
  doc["TxTone"] = vailTxTone;
  doc["Private"] = false;  // Public room
  doc["Decoder"] = false;  // Morse decoder disabled

  String output;
  serializeJson(doc, output);

  Serial.print("Sending keepalive: ");
  Serial.println(output);

  webSocket.sendTXT(output);
}

// Send message to Vail repeater
void sendVailMessage(std::vector<uint16_t> durations, int64_t timestamp) {
  if (vailState != VAIL_CONNECTED) {
    Serial.println("Not connected to Vail");
    return;
  }

  StaticJsonDocument<512> doc;

  // Use provided timestamp (when tone started), or get current time if not provided
  if (timestamp == 0) {
    timestamp = getCurrentTimestamp();
  }
  doc["Timestamp"] = timestamp;
  // Note: Do NOT send Clients field - server populates this
  doc["Callsign"] = vailCallsign;  // Add callsign to all messages
  doc["TxTone"] = vailTxTone;      // Add TX tone to all messages

  JsonArray durArray = doc.createNestedArray("Duration");
  for (uint16_t dur : durations) {
    durArray.add(dur);
  }

  String output;
  serializeJson(doc, output);

  Serial.print("Sending (ts=");
  Serial.print((long)timestamp);
  Serial.print("): ");
  Serial.println(output);

  // Remember this timestamp to filter out the echo (use queue for rapid keying)
  recentTxTimestamps.push_back(timestamp);
  while (recentTxTimestamps.size() > MAX_TX_TIMESTAMPS) {
    recentTxTimestamps.pop_front();
  }

  webSocket.sendTXT(output);
}

// Update Vail repeater (call in main loop)
// Note: Legacy UI rendering removed - LVGL handles all UI updates via updateVailScreenLVGL()
void updateVailRepeater(LGFX &display) {
  webSocket.loop();

  // Send keepalive every 15 seconds (aggressive keepalive prevents Cloud Run and load balancer timeouts)
  // This matches the web repeater's 15s interval for optimal connection stability
  if (vailState == VAIL_CONNECTED && (millis() - lastKeepaliveTime > 15000)) {
    sendKeepalive();
    lastKeepaliveTime = millis();
  }

  // Update paddle transmission
  updateVailPaddles();

  // Playback received messages
  playbackMessages();

  // Note: UI updates are now handled by LVGL via updateVailScreenLVGL()
}

// Straight key handler for Vail
// Uses dual-core audio API: requests are non-blocking, audio task handles I2S on Core 0
void vailStraightKeyHandler() {
  bool ditPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);

  // Stop any playback before transmitting to prevent audio conflicts
  if (ditPressed && isPlaying) {
    requestStopTone();  // Non-blocking - audio task handles stop
    isPlaying = false;
    playbackToneFrequency = 0;
  }

  if (!vailIsTransmitting && ditPressed) {
    // Start transmission
    vailIsTransmitting = true;
    vailTxStartTime = millis();
    vailTxToneOn = true;
    vailTxElementStart = millis();
    vailTxDurations.clear();
    requestStartTone(cwTone);  // Non-blocking - audio task starts tone on Core 0
  }

  if (vailIsTransmitting) {
    // Audio task handles continuous buffer filling - no need to call continueTone()

    // State changed (tone -> silence or silence -> tone)
    if (ditPressed != vailTxToneOn) {
      unsigned long duration = millis() - vailTxElementStart;
      vailTxDurations.push_back((uint16_t)duration);
      vailTxElementStart = millis();
      vailTxToneOn = ditPressed;

      if (ditPressed) {
        requestStartTone(cwTone);  // Non-blocking tone start
      } else {
        requestStopTone();  // Non-blocking tone stop
      }
    }

    // End transmission after 3 dit units of silence (letter spacing)
    if (!ditPressed && (millis() - vailTxElementStart > (vailDitDuration * 3))) {
      unsigned long duration = millis() - vailTxElementStart;
      vailTxDurations.push_back((uint16_t)duration);
      sendVailMessage(vailTxDurations);
      vailIsTransmitting = false;
      vailTxDurations.clear();
      requestStopTone();  // Non-blocking tone stop
    }
  }
}

// Iambic keyer handler for Vail
// Uses dual-core audio API: requests are non-blocking, audio task handles I2S on Core 0
void vailIambicKeyerHandler() {
  unsigned long currentTime = millis();

  // Stop any playback before transmitting to prevent audio conflicts
  if ((vailDitPressed || vailDahPressed) && isPlaying) {
    requestStopTone();  // Non-blocking - audio task handles stop
    isPlaying = false;
    playbackToneFrequency = 0;
  }

  // If not actively sending or spacing, check for new input
  if (!vailKeyerActive && !vailInSpacing) {
    if (vailDitPressed || vailDitMemory) {
      // Start sending dit
      vailKeyerActive = true;
      vailSendingDit = true;
      vailSendingDah = false;
      vailInSpacing = false;
      vailElementStartTime = currentTime;
      vailToneStartTimestamp = getCurrentTimestamp();  // Capture when tone starts
      requestStartTone(cwTone);  // Non-blocking - audio task starts tone on Core 0

      // Start new transmission if needed
      if (!vailIsTransmitting) {
        vailIsTransmitting = true;
        vailTxStartTime = millis();
        vailTxDurations.clear();
      }

      vailDitMemory = false;
    }
    else if (vailDahPressed || vailDahMemory) {
      // Start sending dah
      vailKeyerActive = true;
      vailSendingDit = false;
      vailSendingDah = true;
      vailInSpacing = false;
      vailElementStartTime = currentTime;
      vailToneStartTimestamp = getCurrentTimestamp();  // Capture when tone starts
      requestStartTone(cwTone);  // Non-blocking - audio task starts tone on Core 0

      // Start new transmission if needed
      if (!vailIsTransmitting) {
        vailIsTransmitting = true;
        vailTxStartTime = millis();
        vailTxDurations.clear();
      }

      vailDahMemory = false;
    }
    // No activity - check if we should reset transmission state
    else if (vailIsTransmitting && (millis() - vailTxStartTime > 2000)) {
      // Reset transmission state after 2 seconds of inactivity
      vailIsTransmitting = false;
    }
  }
  // Currently sending an element
  else if (vailKeyerActive && !vailInSpacing) {
    unsigned long elementDuration = vailSendingDit ? vailDitDuration : (vailDitDuration * 3);

    // Audio task handles continuous buffer filling - no need to call continueTone()
    // This is the key change: removing the blocking continueTone() call that was causing distortion

    // Continuously check for paddle input during element send
    if (vailDitPressed && vailDahPressed) {
      if (vailSendingDit) {
        vailDahMemory = true;
      } else {
        vailDitMemory = true;
      }
    }
    else if (vailSendingDit && vailDahPressed) {
      vailDahMemory = true;
    }
    else if (vailSendingDah && vailDitPressed) {
      vailDitMemory = true;
    }

    // Check if element is complete
    if (currentTime - vailElementStartTime >= elementDuration) {
      // Send tone immediately using the timestamp from when it started
      sendVailMessage({(uint16_t)elementDuration}, vailToneStartTimestamp);

      // Element complete, turn off tone and start spacing
      requestStopTone();  // Non-blocking - audio task handles stop
      vailKeyerActive = false;
      vailSendingDit = false;
      vailSendingDah = false;
      vailInSpacing = true;
      vailElementStartTime = currentTime;
      vailTxStartTime = millis();  // Reset idle timer
    }
  }
  // In inter-element spacing
  else if (vailInSpacing) {
    // Continue checking paddles during spacing
    if (vailDitPressed && vailDahPressed) {
      vailDitMemory = true;
      vailDahMemory = true;
    }
    else if (vailDitPressed && !vailDitMemory) {
      vailDitMemory = true;
    }
    else if (vailDahPressed && !vailDahMemory) {
      vailDahMemory = true;
    }

    unsigned long spaceDuration = currentTime - vailElementStartTime;

    // Check if next element is starting (memory set)
    if ((vailDitMemory || vailDahMemory) && spaceDuration >= vailDitDuration) {
      // Don't send silences - just move to next element
      vailInSpacing = false;
      vailTxStartTime = millis();  // Reset idle timer
    }
    // No next element queued - check for longer pause (reset transmission state after 2 seconds)
    else if (!vailDitMemory && !vailDahMemory && spaceDuration >= 2000) {
      // End transmission (no need to send silence)
      vailInSpacing = false;
      vailIsTransmitting = false;
    }
  }
}

// Handle paddle input for transmission
void updateVailPaddles() {
  vailDitPressed = (digitalRead(DIT_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DIT_PIN) > TOUCH_THRESHOLD);
  vailDahPressed = (digitalRead(DAH_PIN) == PADDLE_ACTIVE) || (touchRead(TOUCH_DAH_PIN) > TOUCH_THRESHOLD);

  // Use keyer based on settings
  if (cwKeyType == KEY_STRAIGHT) {
    vailStraightKeyHandler();
  } else {
    vailIambicKeyerHandler();
  }
}

// Playback received messages (non-blocking)
// Uses dual-core audio API: all audio requests are non-blocking, handled by Core 0
void playbackMessages() {
  // Don't play if transmitting - transmission has audio priority
  if (vailIsTransmitting || vailKeyerActive) {
    if (isPlaying) {
      // Stop playback if we started transmitting
      requestStopTone();  // Non-blocking - audio task handles stop
      isPlaying = false;
      playbackToneFrequency = 0;
    }
    return;
  }

  if (rxQueue.empty() && !isPlaying) return;

  // Audio task handles continuous buffer filling automatically
  // No need for continueTone() calls from UI loop - this was causing the distortion!

  int64_t now = getCurrentTimestamp();

  // Start playing if not already playing
  if (!isPlaying && !rxQueue.empty()) {
    VailMessage &msg = rxQueue[0];
    int64_t playTime = msg.timestamp + playbackDelay;

    Serial.print("Checking playback: now=");
    Serial.print((long)now);
    Serial.print(" playTime=");
    Serial.print((long)playTime);
    Serial.print(" diff=");
    Serial.println((long)(playTime - now));

    if (now >= playTime) {
      Serial.print("Starting playback of ");
      Serial.print(msg.durations.size());
      Serial.print(" elements at ");
      Serial.print(midiNoteToFrequency(msg.txTone));
      Serial.println(" Hz");
      isPlaying = true;
      playbackIndex = 0;
      playbackElementStart = millis();

      // Start first element
      if (msg.durations.size() > 0) {
        Serial.print("First element duration: ");
        Serial.println(msg.durations[0]);
        // Play at local cwTone (consistent experience) or sender's TX tone
        #if VAIL_USE_LOCAL_TONE_FOR_RECEIVE
          playbackToneFrequency = cwTone;
        #else
          playbackToneFrequency = midiNoteToFrequency(msg.txTone);
        #endif
        requestStartTone(playbackToneFrequency);  // Non-blocking - audio task starts tone
      }
    }
  }

  // Continue playing current message
  if (isPlaying && !rxQueue.empty()) {
    VailMessage &msg = rxQueue[0];

    // Check if current element is done
    unsigned long elapsed = millis() - playbackElementStart;
    if (elapsed >= msg.durations[playbackIndex]) {
      // Move to next element
      playbackIndex++;

      if (playbackIndex >= msg.durations.size()) {
        // Message complete
        requestStopTone();  // Non-blocking - audio task handles stop
        isPlaying = false;
        playbackIndex = 0;
        playbackToneFrequency = 0;
        rxQueue.erase(rxQueue.begin());
        Serial.println("Playback complete");
      } else {
        // Start next element
        playbackElementStart = millis();

        Serial.print("Element ");
        Serial.print(playbackIndex);
        Serial.print(": ");
        Serial.print(msg.durations[playbackIndex]);
        Serial.print("ms ");

        if (playbackIndex % 2 == 0) {
          // Even index = tone
          Serial.println("TONE");
          #if VAIL_USE_LOCAL_TONE_FOR_RECEIVE
            playbackToneFrequency = cwTone;
          #else
            playbackToneFrequency = midiNoteToFrequency(msg.txTone);
          #endif
          requestStartTone(playbackToneFrequency);  // Non-blocking - audio task starts tone
        } else {
          // Odd index = silence
          Serial.println("SILENCE");
          playbackToneFrequency = 0;
          requestStopTone();  // Non-blocking - audio task handles stop
        }
      }
    }
  }
}

// Draw Vail UI
void drawVailUI(LGFX &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Clean main info card
  int cardX = 20;
  int cardY = 60;
  int cardW = SCREEN_WIDTH - 40;
  int cardH = 120;

  // Solid card background
  display.fillRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BG_LAYER2);

  // Subtle border
  display.drawRoundRect(cardX, cardY, cardW, cardH, 10, COLOR_BORDER_SUBTLE);

  // Channel
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT_SECONDARY);
  display.setCursor(cardX + 15, cardY + 15);
  display.print("Channel");

  display.setTextColor(COLOR_TEXT_PRIMARY);
  display.setTextSize(2);
  display.setCursor(cardX + 15, cardY + 30);
  display.print(vailChannel);

  // Status
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT_SECONDARY);
  display.setCursor(cardX + 15, cardY + 60);
  display.print("Status");

  display.setTextSize(1);
  display.setCursor(cardX + 15, cardY + 75);
  if (vailState == VAIL_CONNECTED) {
    display.setTextColor(COLOR_SUCCESS_PASTEL);
    display.print("Connected");
  } else if (vailState == VAIL_CONNECTING) {
    display.setTextColor(COLOR_WARNING_PASTEL);
    display.print("Connecting...");
  } else if (vailState == VAIL_ERROR) {
    display.setTextColor(COLOR_ERROR_PASTEL);
    display.print("Error");
  } else {
    display.setTextColor(COLOR_ERROR_PASTEL);
    display.print("Disconnected");
  }

  // Speed
  display.setTextSize(1);
  display.setTextColor(COLOR_TEXT_SECONDARY);
  display.setCursor(cardX + 15, cardY + 100);
  display.print("Speed");

  display.setTextColor(COLOR_ACCENT_CYAN);
  display.setTextSize(1);
  display.setCursor(cardX + 70, cardY + 100);
  display.print(cwSpeed);
  display.print(" WPM");

  // Operators (only when connected)
  if (vailState == VAIL_CONNECTED) {
    display.setTextColor(COLOR_TEXT_SECONDARY);
    display.setCursor(cardX + 170, cardY + 100);
    display.print("Ops");

    display.setTextColor(COLOR_SUCCESS_PASTEL);
    display.setCursor(cardX + 210, cardY + 100);
    display.print(connectedClients);
  }

  // TX indicator on card
  if (vailIsTransmitting) {
    display.fillCircle(cardX + cardW - 25, cardY + 25, 8, ST77XX_RED);
    display.setTextSize(1);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(cardX + cardW - 65, cardY + 22);
    display.print("TX");
  }

  // Instructions
  display.setTextSize(1);
  display.setTextColor(0x7BEF); // Light gray
  display.setCursor(30, 200);
  display.print("Use paddle to transmit");

  // Message notification indicator
  if (hasUnreadMessages) {
    // Draw pulsing "NEW MSG" indicator
    display.fillRoundRect(SCREEN_WIDTH - 80, 195, 70, 18, 4, ST77XX_RED);
    display.drawRoundRect(SCREEN_WIDTH - 80, 195, 70, 18, 4, ST77XX_WHITE);
    display.setTextSize(1);
    display.setTextColor(ST77XX_WHITE);
    display.setCursor(SCREEN_WIDTH - 72, 203);
    display.print("NEW MSG!");
  }

  // Footer with controls
  display.setTextColor(COLOR_WARNING);
  display.setTextSize(1);
  display.setCursor(5, SCREEN_HEIGHT - 12);
  if (hasUnreadMessages) {
    display.print("\x18Rooms \x19Chat(!) U Users \x1B\x1ASpd ESC Exit");
  } else {
    display.print("\x18Rooms \x19Chat U Users \x1B\x1ASpd ESC Exit");
  }
}

// Draw Chat UI
void drawChatUI(LGFX &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  display.setFont(nullptr);  // Use default font
  display.setTextColor(COLOR_TITLE);
  display.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  String title = "TEXT CHAT";
  getTextBounds_compat(display, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 70);
  display.print(title);
  display.setFont(nullptr); // Reset font

  // Channel indicator
  display.setTextSize(1);
  display.setTextColor(0x7BEF); // Light gray
  String channelText = "Channel: " + vailChannel;
  display.setCursor((SCREEN_WIDTH - channelText.length() * 6) / 2, 85);
  display.print(channelText);

  // Message history area (scrollable)
  int historyY = 95;
  int historyHeight = 90;
  int lineHeight = 15;

  display.setTextSize(1);
  int startIndex = max(0, (int)chatHistory.size() - 6);  // Show last 6 messages

  for (int i = startIndex; i < chatHistory.size(); i++) {
    int yPos = historyY + (i - startIndex) * lineHeight;

    // Draw callsign in color
    display.setTextColor(COLOR_WARNING);
    display.setCursor(5, yPos);
    display.print(chatHistory[i].callsign);
    display.print(":");

    // Draw message in white
    display.setTextColor(ST77XX_WHITE);
    int msgX = 5 + (chatHistory[i].callsign.length() + 1) * 6;
    display.setCursor(msgX, yPos);

    // Truncate message if too long
    String msg = chatHistory[i].message;
    int maxMsgLen = (SCREEN_WIDTH - msgX) / 6 - 1;
    if (msg.length() > maxMsgLen) {
      msg = msg.substring(0, maxMsgLen - 3) + "...";
    }
    display.print(msg);
  }

  // Input box
  int boxX = 5;
  int boxY = 190;
  int boxW = SCREEN_WIDTH - 10;
  int boxH = 30;

  display.fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x1082); // Dark blue fill
  display.drawRoundRect(boxX, boxY, boxW, boxH, 8, 0x34BF); // Light blue outline

  // Display chat input
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(1);
  display.setCursor(boxX + 8, boxY + 12);

  // Show last N characters if input is too long
  String displayInput = chatInput;
  int maxInputDisplay = (boxW - 20) / 6;
  if (displayInput.length() > maxInputDisplay) {
    displayInput = displayInput.substring(displayInput.length() - maxInputDisplay);
  }
  display.print(displayInput);

  // Blinking cursor
  if (chatCursorVisible) {
    int cursorX = boxX + 8 + (displayInput.length() * 6);
    if (cursorX < boxX + boxW - 10) {
      display.fillRect(cursorX, boxY + 10, 2, 10, COLOR_WARNING);
    }
  }

  // Footer with controls
  display.setTextColor(COLOR_WARNING);
  display.setTextSize(1);
  display.setCursor(10, SCREEN_HEIGHT - 12);
  display.print("Type msg  ENTER Send  \x18 Back  ESC Exit");
}

// Handle chat input
int handleChatInput(char key, LGFX &display) {
  // Update cursor blink
  if (millis() - chatLastBlink > 500) {
    chatCursorVisible = !chatCursorVisible;
    chatLastBlink = millis();
    drawChatUI(display);
  }

  if (key == KEY_BACKSPACE) {
    if (chatInput.length() > 0) {
      chatInput.remove(chatInput.length() - 1);
      chatCursorVisible = true;
      chatLastBlink = millis();
      drawChatUI(display);
    }
    return 0;
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Send message
    if (chatInput.length() > 0) {
      sendChatMessage(chatInput);
      addChatMessage(vailCallsign, chatInput);  // Add our own message to history
      chatInput = "";
      chatCursorVisible = true;
      chatLastBlink = millis();
      // No beep - would conflict with repeater audio playback
      drawChatUI(display);
    }
    return 0;
  }
  else if (key >= 32 && key <= 126 && chatInput.length() < MAX_CHAT_INPUT) {
    // Add printable character
    chatInput += key;
    chatCursorVisible = true;
    chatLastBlink = millis();
    // No beep - would conflict with repeater audio playback
    drawChatUI(display);
    return 0;
  }

  return 0;
}

// Handle Vail input
int handleVailInput(char key, LGFX &display) {
  if (key == KEY_ESC) {
    // If in user list, go back to vail info
    if (vailUserListMode) {
      vailUserListMode = false;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawVailUI(display);
      return 0;
    }
    // If in room custom input, go back to room selection
    if (roomCustomInput) {
      roomCustomInput = false;
      roomInput = "";
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawRoomSelectionUI(display);
      return 0;
    }
    // If in room selection, go back to vail info
    if (vailRoomSelectionMode) {
      vailRoomSelectionMode = false;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawVailUI(display);
      return 0;
    }
    // If in chat mode, go back to vail info
    if (vailChatMode) {
      vailChatMode = false;
      chatInput = "";
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawVailUI(display);
      return 0;
    }
    // Otherwise exit Vail mode
    disconnectFromVail();
    return -1;
  }

  // If in user list mode, handle input
  if (vailUserListMode) {
    return handleUserListInput(key, display);
  }

  // If in room custom input mode, handle input
  if (roomCustomInput) {
    return handleRoomInputInput(key, display);
  }

  // If in room selection mode, handle selection
  if (vailRoomSelectionMode) {
    return handleRoomSelectionInput(key, display);
  }

  // Arrow Up: Open room selection menu (if in vail info mode)
  if (key == KEY_UP) {
    if (vailChatMode) {
      // Go back to vail info from chat
      vailChatMode = false;
      chatInput = "";
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawVailUI(display);
      return 0;
    }
    // In vail info, open room selection
    vailRoomSelectionMode = true;
    roomMenuSelection = 0;
    beep(TONE_MENU_NAV, BEEP_SHORT);
    drawRoomSelectionUI(display);
    return 0;
  }

  // Arrow Down: Go to chat mode
  if (key == KEY_DOWN) {
    if (!vailChatMode) {
      // Enter chat mode
      vailChatMode = true;
      chatInput = "";
      chatCursorVisible = true;
      chatLastBlink = millis();
      hasUnreadMessages = false;  // Clear notification when entering chat
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawChatUI(display);
      return 0;
    }
    // Already in chat mode, ignore
    return 0;
  }

  // If in chat mode, handle chat input
  if (vailChatMode) {
    return handleChatInput(key, display);
  }

  // Arrow Left/Right: Adjust speed
  if (key == KEY_LEFT) {
    if (cwSpeed > 5) {
      cwSpeed--;
      vailDitDuration = DIT_DURATION(cwSpeed);
      saveCWSettings();
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
    return 0;
  }

  if (key == KEY_RIGHT) {
    if (cwSpeed < 40) {
      cwSpeed++;
      vailDitDuration = DIT_DURATION(cwSpeed);
      saveCWSettings();
      beep(TONE_MENU_NAV, BEEP_SHORT);
    }
    return 0;
  }

  // 'U' key: Open user list (only in vail info mode)
  if (key == 'u' || key == 'U') {
    if (!vailChatMode && !vailRoomSelectionMode && !roomCustomInput) {
      vailUserListMode = true;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawUserListUI(display);
      return 0;
    }
  }

  return 0;
}

// Add message to chat history
void addChatMessage(String callsign, String message) {
  ChatMessage msg;
  msg.callsign = callsign;
  msg.message = message;
  msg.timestamp = millis();

  chatHistory.push_back(msg);

  // Keep only last MAX_CHAT_MESSAGES
  while (chatHistory.size() > MAX_CHAT_MESSAGES) {
    chatHistory.erase(chatHistory.begin());
  }

  Serial.print("Chat: ");
  Serial.print(callsign);
  Serial.print(": ");
  Serial.println(message);

  // Set notification if not in chat mode (LVGL will update on next frame)
  if (!vailChatMode) {
    hasUnreadMessages = true;
  }
}

// Send text chat message over WebSocket
void sendChatMessage(String message) {
  if (vailState != VAIL_CONNECTED) {
    Serial.println("Not connected - cannot send chat message");
    return;
  }

  StaticJsonDocument<512> doc;
  doc["Timestamp"] = getCurrentTimestamp();
  JsonArray duration = doc.createNestedArray("Duration");  // Empty duration array
  doc["Callsign"] = vailCallsign;
  doc["TxTone"] = vailTxTone;
  doc["Text"] = message;  // Add text message field (per Vail API spec)

  String output;
  serializeJson(doc, output);

  Serial.print("Sending chat message: ");
  Serial.println(output);

  webSocket.sendTXT(output);
}

// Draw Room Selection UI
void drawRoomSelectionUI(LGFX &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  display.setFont(nullptr);  // Use default font
  display.setTextColor(COLOR_TITLE);
  display.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  String title = "SELECT ROOM";
  getTextBounds_compat(display, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 70);
  display.print(title);
  display.setFont(nullptr); // Reset font

  // Build menu items
  std::vector<String> menuItems;

  // Add active public rooms
  for (size_t i = 0; i < activeRooms.size(); i++) {
    String item = activeRooms[i].name + " (" + String(activeRooms[i].users) + ")";
    menuItems.push_back(item);
  }

  // Add "General" if not already in list
  bool hasGeneral = false;
  for (size_t i = 0; i < activeRooms.size(); i++) {
    if (activeRooms[i].name == "General") {
      hasGeneral = true;
      break;
    }
  }
  if (!hasGeneral) {
    menuItems.push_back("General");
  }

  // Add "Custom room..." option
  menuItems.push_back("Custom room...");

  // Draw menu items (show up to 6 items)
  int menuY = 90;
  int itemHeight = 20;
  int startIdx = max(0, roomMenuSelection - 5);
  int endIdx = min((int)menuItems.size(), startIdx + 6);

  for (int i = startIdx; i < endIdx; i++) {
    int yPos = menuY + (i - startIdx) * itemHeight;

    if (i == roomMenuSelection) {
      // Selected item - highlighted
      display.fillRect(10, yPos - 2, SCREEN_WIDTH - 20, itemHeight - 2, 0x249F);
      display.setTextColor(ST77XX_WHITE);
      display.setCursor(15, yPos + 6);
      display.print("> ");
      display.print(menuItems[i]);
    } else {
      // Unselected item
      display.setTextColor(0x7BEF);
      display.setCursor(20, yPos + 6);
      display.print(menuItems[i]);
    }
  }

  // Footer
  display.setTextColor(COLOR_WARNING);
  display.setTextSize(1);
  display.setCursor(10, SCREEN_HEIGHT - 12);
  display.print("\x18\x19 Navigate  ENTER Select  ESC Back");
}

// Handle Room Selection Input
int handleRoomSelectionInput(char key, LGFX &display) {
  // Calculate total menu items
  int totalItems = activeRooms.size() + 1;  // Active rooms + "Custom room..."
  bool hasGeneral = false;
  for (size_t i = 0; i < activeRooms.size(); i++) {
    if (activeRooms[i].name == "General") {
      hasGeneral = true;
      break;
    }
  }
  if (!hasGeneral) totalItems++;  // Add "General" if not in active rooms

  if (key == KEY_UP) {
    if (roomMenuSelection > 0) {
      roomMenuSelection--;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawRoomSelectionUI(display);
    }
    return 0;
  }
  else if (key == KEY_DOWN) {
    if (roomMenuSelection < totalItems - 1) {
      roomMenuSelection++;
      beep(TONE_MENU_NAV, BEEP_SHORT);
      drawRoomSelectionUI(display);
    }
    return 0;
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    beep(TONE_SELECT, BEEP_MEDIUM);

    // Determine which item was selected
    int customRoomIdx = totalItems - 1;  // Last item is always "Custom room..."

    if (roomMenuSelection == customRoomIdx) {
      // Custom room input
      roomCustomInput = true;
      roomInput = "";
      roomCursorVisible = true;
      roomLastBlink = millis();
      drawRoomInputUI(display);
    } else {
      // Selected an existing room
      String selectedRoom;
      if (roomMenuSelection < activeRooms.size()) {
        selectedRoom = activeRooms[roomMenuSelection].name;
      } else {
        selectedRoom = "General";
      }

      // Disconnect and reconnect to new room
      disconnectFromVail();
      delay(250);  // Allow time for clean disconnect before reconnecting
      connectToVail(selectedRoom);

      // Return to vail info
      vailRoomSelectionMode = false;
      drawVailUI(display);
    }
    return 0;
  }

  return 0;
}

// Draw Room Input UI
void drawRoomInputUI(LGFX &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  display.setFont(nullptr);  // Use default font
  display.setTextColor(COLOR_TITLE);
  display.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  String title = "CUSTOM ROOM";
  getTextBounds_compat(display, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 70);
  display.print(title);
  display.setFont(nullptr); // Reset font

  // Instructions
  display.setTextSize(1);
  display.setTextColor(0x7BEF);
  String prompt = "Enter room name:";
  display.setCursor((SCREEN_WIDTH - prompt.length() * 6) / 2, 90);
  display.print(prompt);

  // Input box
  int boxX = 20;
  int boxY = 110;
  int boxW = SCREEN_WIDTH - 40;
  int boxH = 40;

  display.fillRoundRect(boxX, boxY, boxW, boxH, 8, 0x1082);
  display.drawRoundRect(boxX, boxY, boxW, boxH, 8, 0x34BF);

  // Display room input
  display.setTextColor(ST77XX_WHITE);
  display.setTextSize(1);
  display.setCursor(boxX + 10, boxY + 18);

  // Show last N characters if input is too long
  String displayInput = roomInput;
  int maxInputDisplay = (boxW - 25) / 6;
  if (displayInput.length() > maxInputDisplay) {
    displayInput = displayInput.substring(displayInput.length() - maxInputDisplay);
  }
  display.print(displayInput);

  // Blinking cursor
  if (roomCursorVisible) {
    int cursorX = boxX + 10 + (displayInput.length() * 6);
    if (cursorX < boxX + boxW - 10) {
      display.fillRect(cursorX, boxY + 15, 2, 12, COLOR_WARNING);
    }
  }

  // Footer
  display.setTextColor(COLOR_WARNING);
  display.setTextSize(1);
  display.setCursor(10, SCREEN_HEIGHT - 12);
  display.print("Type name  ENTER Join  ESC Cancel");
}

// Handle Room Input Input
int handleRoomInputInput(char key, LGFX &display) {
  // Update cursor blink
  if (millis() - roomLastBlink > 500) {
    roomCursorVisible = !roomCursorVisible;
    roomLastBlink = millis();
    drawRoomInputUI(display);
  }

  if (key == KEY_BACKSPACE) {
    if (roomInput.length() > 0) {
      roomInput.remove(roomInput.length() - 1);
      roomCursorVisible = true;
      roomLastBlink = millis();
      drawRoomInputUI(display);
    }
    return 0;
  }
  else if (key == KEY_ENTER || key == KEY_ENTER_ALT) {
    // Join custom room
    if (roomInput.length() > 0) {
      // No beep - would conflict with repeater audio playback

      // Disconnect and reconnect to new room
      disconnectFromVail();
      delay(250);  // Allow time for clean disconnect before reconnecting
      connectToVail(roomInput);

      // Return to vail info
      roomCustomInput = false;
      vailRoomSelectionMode = false;
      roomInput = "";
      drawVailUI(display);
    }
    return 0;
  }
  else if (key >= 32 && key <= 126 && roomInput.length() < MAX_ROOM_NAME) {
    // Add printable character
    roomInput += key;
    roomCursorVisible = true;
    roomLastBlink = millis();
    // No beep - would conflict with repeater audio playback
    drawRoomInputUI(display);
    return 0;
  }

  return 0;
}

// Draw User List UI
void drawUserListUI(LGFX &display) {
  // Clear screen (preserve header)
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);

  // Title
  display.setFont(nullptr);  // Use default font
  display.setTextColor(COLOR_TITLE);
  display.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  String title = "USERS IN ROOM";
  getTextBounds_compat(display, title.c_str(), 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 70);
  display.print(title);
  display.setFont(nullptr); // Reset font

  // Room name subtitle
  display.setTextSize(1);
  display.setTextColor(0x7BEF);
  String roomText = "Room: " + vailChannel;
  display.setCursor((SCREEN_WIDTH - roomText.length() * 6) / 2, 85);
  display.print(roomText);

  // User count
  display.setTextColor(COLOR_WARNING);
  String countText = String(connectedUsers.size()) + " user(s) connected";
  display.setCursor((SCREEN_WIDTH - countText.length() * 6) / 2, 100);
  display.print(countText);

  // Draw user list
  int listY = 115;
  int itemHeight = 18;
  int maxVisible = 7;  // Show up to 7 users

  for (size_t i = 0; i < connectedUsers.size() && i < maxVisible; i++) {
    int yPos = listY + i * itemHeight;

    // Draw callsign
    display.setTextColor(ST77XX_WHITE);
    display.setTextSize(1);
    display.setCursor(15, yPos);
    display.print(connectedUsers[i].callsign);

    // Draw TX tone frequency
    display.setTextColor(0x7BEF);
    int freqHz = (int)midiNoteToFrequency(connectedUsers[i].txTone);
    String freqText = String(freqHz) + " Hz";
    int freqX = SCREEN_WIDTH - 15 - freqText.length() * 6;
    display.setCursor(freqX, yPos);
    display.print(freqText);

    // Draw separator line
    display.drawLine(10, yPos + 12, SCREEN_WIDTH - 10, yPos + 12, 0x2104);
  }

  // Show "..." if more users than can fit
  if (connectedUsers.size() > maxVisible) {
    display.setTextColor(0x7BEF);
    display.setCursor(SCREEN_WIDTH / 2 - 6, listY + maxVisible * itemHeight);
    display.print("...");
  }

  // Footer
  display.setTextColor(COLOR_WARNING);
  display.setTextSize(1);
  display.setCursor(10, SCREEN_HEIGHT - 12);
  display.print("ESC Back to Vail Info");
}

// Handle User List Input
int handleUserListInput(char key, LGFX &display) {
  // ESC handled in main handleVailInput
  // Just return 0 for any other keys
  return 0;
}

#else  // VAIL_ENABLED == 0

// Stub functions when libraries are not installed
void startVailRepeater(LGFX &display) {
  display.fillRect(0, 42, SCREEN_WIDTH, SCREEN_HEIGHT - 42, COLOR_BACKGROUND);
  display.setTextSize(1);
  display.setTextColor(ST77XX_RED);
  display.setCursor(20, 100);
  display.print("Vail repeater disabled");
  display.setCursor(20, 120);
  display.print("Install required libraries:");
  display.setCursor(20, 140);
  display.print("1. WebSockets");
  display.setCursor(20, 155);
  display.print("   by Markus Sattler");
  display.setCursor(20, 175);
  display.print("2. ArduinoJson");
  display.setCursor(20, 190);
  display.print("   by Benoit Blanchon");
}

void drawVailUI(LGFX &display) {
  startVailRepeater(display);
}

int handleVailInput(char key, LGFX &display) {
  if (key == KEY_ESC) return -1;
  return 0;
}

void updateVailRepeater(LGFX &display) {
  // Nothing to do
}

void connectToVail(String channel) {
  // Nothing to do
}

void disconnectFromVail() {
  // Nothing to do
}

#endif // VAIL_ENABLED

#endif // VAIL_REPEATER_H
