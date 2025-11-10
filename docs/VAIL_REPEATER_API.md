# Vail Repeater API Documentation for Arduino/WiFi Devices

## Overview

The Vail Repeater uses WebSocket connections to enable real-time Morse code communication between multiple users. This document describes the protocol for connecting Arduino or other WiFi-enabled devices to the Vail Repeater.

## Connection Details

### WebSocket Endpoint

```
ws://[server-address]:8080/chat?repeater=[room-name]
```

**Parameters:**
- `server-address`: Your Vail server hostname/IP
- `room-name`: The room to join (e.g., "General", "Channel 1", or custom room name)

**Example:**
```
ws://vailrepeater.local:8080/chat?repeater=General
```

### Subprotocols

The server supports two subprotocol formats. You must specify one when connecting:

1. **JSON Protocol** (Recommended for Arduino): `json.vail.woozle.org`
2. **Binary Protocol**: `binary.vail.woozle.org`

**Arduino ESP8266/ESP32 Example:**
```cpp
webSocket.beginSSL("your-server.com", 443, "/chat?repeater=General");
webSocket.setSubprotocol("json.vail.woozle.org");
```

## Message Format (JSON)

All messages are JSON objects with the following structure:

### Outgoing Message Structure (Client → Server)

```json
{
  "Timestamp": 1697500000000,
  "Duration": [100, 300, 100],
  "Callsign": "KE9BOS",
  "TxTone": 72,
  "Private": false,
  "Text": ""
}
```

**Fields:**
- `Timestamp` (int64): Unix timestamp in milliseconds (`Date.now()` in JS, `millis()` in Arduino)
- `Duration` (array of uint16): Array of tone durations in milliseconds (empty for non-morse messages)
- `Callsign` (string): Your cok it wallsign or identifier (optional, but recommended)
- `TxTone` (uint8): MIDI note number (0-127) for your transmit tone (0 = not specified, 72 = C5/523Hz)
- `Private` (bool): Whether the room should be private (only used on first connection)
- `Text` (string): Text chat message (optional, empty for morse transmissions)

### Incoming Message Structure (Server → Client)

```json
{
  "Timestamp": 1697500000123,
  "Duration": [100, 300, 100],
  "Clients": 5,
  "Users": ["KE9BOS", "KC3WOA", "TestUser"],
  "UsersInfo": [
    {"callsign": "KE9BOS", "txTone": 72},
    {"callsign": "KC3WOA", "txTone": 65}
  ],
  "Rooms": [
    {"name": "General", "users": 5, "private": false}
  ],
  "Text": "",
  "TxTone": 72
}
```

**Fields:**
- `Timestamp` (int64): Server timestamp in milliseconds
- `Duration` (array): Tone durations to play (empty for status updates)
- `Clients` (uint16): Number of connected clients
- `Users` (array of strings): List of connected user callsigns
- `UsersInfo` (array of objects): Detailed user info including TX tones
- `Rooms` (array of objects): List of active public rooms
- `Text` (string): Chat message text (if present)
- `TxTone` (uint8): The TX tone of the sender (for received morse code)

## Connection Flow

### 1. Initial Connection

When you first connect, send an initial message with your callsign and TX tone:

```json
{
  "Timestamp": 1697500000000,
  "Duration": [],
  "Callsign": "YourCallsign",
  "TxTone": 72,
  "Private": false
}
```

**Important:** This message must be sent immediately after connecting to register your presence.

### 2. Keepalive

Send a message every 30 seconds to prevent connection timeout:

```json
{
  "Timestamp": 1697500030000,
  "Duration": [],
  "Callsign": "YourCallsign",
  "TxTone": 72
}
```

### 3. Sending Morse Code

To transmit morse code, send the tone durations:

```json
{
  "Timestamp": 1697500001000,
  "Duration": [100, 50, 300, 50, 100],
  "TxTone": 72
}
```

**Duration Array Format:**
- Each number represents the duration of a tone ON or OFF period
- Alternates: ON, OFF, ON, OFF, ON...
- First element is always ON (dit/dah)
- All values in milliseconds

**Example - Letter "A" (· —):**
```json
{
  "Duration": [100, 100, 300]
}
```
- 100ms ON (dit)
- 100ms OFF (space between dit and dah)
- 300ms ON (dah)

### 4. Updating Callsign or TX Tone

Send an empty Duration message with updated fields:

```json
{
  "Timestamp": 1697500005000,
  "Duration": [],
  "Callsign": "NewCallsign",
  "TxTone": 80
}
```

## TX Tone (MIDI Notes)

The `TxTone` field uses MIDI note numbers (0-127):

| MIDI Note | Note Name | Frequency (Hz) |
|-----------|-----------|----------------|
| 60        | C4        | 261.6          |
| 64        | E4        | 329.6          |
| 65        | F4        | 349.2          |
| 69        | A4        | 440.0          |
| 72        | C5        | 523.3          |
| 76        | E5        | 659.3          |
| 81        | A5        | 880.0          |

**Formula:** `frequency = 440 * 2^((note - 69) / 12)`

**Recommended range:** 60-84 (C4 to C6) for comfortable listening

## Text Chat

To send a text chat message:

```json
{
  "Timestamp": 1697500010000,
  "Duration": [],
  "Text": "Hello from Arduino!",
  "Callsign": "YourCallsign"
}
```

## Timing Requirements

### Clock Synchronization

The server validates that message timestamps are within **10 seconds** of server time. If your Arduino's clock drifts too far, messages will be rejected with:

```
"Your clock is off by too much"
```

**Solution:** Use NTP to keep your Arduino's clock synchronized.

### Message Timing

- Messages with future timestamps are queued and played at the specified time
- Messages older than current time by more than allowed delay will be rejected
- Typical RX delay: 2-4 seconds (configurable by client)

## Error Handling

### Connection Errors

If connection fails, check:
1. Correct WebSocket URL and port
2. Subprotocol is specified
3. Server is running and accessible

### Message Rejection

Messages may be rejected if:
- Clock offset is > 10 seconds
- Invalid JSON format
- Missing required fields
- Timestamp is too old/too far in future

### Write Timeout

The server has a 5-second write timeout. If your device doesn't receive messages within 5 seconds, the connection may be closed.

## Arduino Example Code Skeleton

```cpp
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>

WebSocketsClient webSocket;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char* VAIL_SERVER = "your-server.com";
const int VAIL_PORT = 8080;
const char* REPEATER_ROOM = "General";
const char* CALLSIGN = "YourCallsign";
const uint8_t TX_TONE = 72;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin("your-ssid", "your-password");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Start NTP
  timeClient.begin();
  timeClient.update();

  // Connect to WebSocket
  String path = "/chat?repeater=" + String(REPEATER_ROOM);
  webSocket.begin(VAIL_SERVER, VAIL_PORT, path);
  webSocket.setSubprotocol("json.vail.woozle.org");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();
  timeClient.update();

  // Send keepalive every 30 seconds
  static unsigned long lastKeepalive = 0;
  if (millis() - lastKeepalive > 30000) {
    sendKeepalive();
    lastKeepalive = millis();
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.println("Connected to Vail Repeater");
      sendInitialMessage();
      break;

    case WStype_TEXT:
      handleIncomingMessage(payload, length);
      break;

    case WStype_DISCONNECTED:
      Serial.println("Disconnected");
      break;
  }
}

void sendInitialMessage() {
  StaticJsonDocument<256> doc;
  doc["Timestamp"] = getCurrentTimestamp();
  JsonArray duration = doc.createNestedArray("Duration");
  doc["Callsign"] = CALLSIGN;
  doc["TxTone"] = TX_TONE;
  doc["Private"] = false;

  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(output);
}

void sendKeepalive() {
  StaticJsonDocument<256> doc;
  doc["Timestamp"] = getCurrentTimestamp();
  JsonArray duration = doc.createNestedArray("Duration");
  doc["Callsign"] = CALLSIGN;
  doc["TxTone"] = TX_TONE;

  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(output);
}

void sendMorseCode(uint16_t* durations, size_t count) {
  StaticJsonDocument<512> doc;
  doc["Timestamp"] = getCurrentTimestamp();
  JsonArray durationArray = doc.createNestedArray("Duration");
  for (size_t i = 0; i < count; i++) {
    durationArray.add(durations[i]);
  }
  doc["TxTone"] = TX_TONE;

  String output;
  serializeJson(doc, output);
  webSocket.sendTXT(output);
}

void handleIncomingMessage(uint8_t* payload, size_t length) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.println("JSON parse error");
    return;
  }

  // Check if this is a morse transmission
  if (doc.containsKey("Duration") && doc["Duration"].size() > 0) {
    uint8_t senderTxTone = doc["TxTone"] | 69; // Default to A4
    JsonArray durations = doc["Duration"];

    // Play the morse code at the sender's TX tone
    playMorseCode(durations, senderTxTone);
  }

  // Check for text message
  if (doc.containsKey("Text") && !doc["Text"].isNull()) {
    String text = doc["Text"];
    String sender = doc["Callsign"] | "Unknown";
    Serial.println("Chat from " + sender + ": " + text);
  }

  // Update connected users count
  if (doc.containsKey("Clients")) {
    int clients = doc["Clients"];
    Serial.println("Connected clients: " + String(clients));
  }
}

void playMorseCode(JsonArray durations, uint8_t txTone) {
  // Calculate frequency from MIDI note
  float frequency = 440.0 * pow(2.0, (txTone - 69) / 12.0);

  // Play each duration
  bool isOn = true;
  for (JsonVariant v : durations) {
    uint16_t duration = v.as<uint16_t>();
    if (isOn) {
      // Turn on tone at calculated frequency
      tone(BUZZER_PIN, frequency, duration);
    }
    delay(duration);
    isOn = !isOn;
  }
}

unsigned long getCurrentTimestamp() {
  // Get current Unix timestamp in milliseconds
  return timeClient.getEpochTime() * 1000UL + (millis() % 1000);
}
```

## Testing Your Device

1. **Connect and verify:**
   - Watch server logs for your connection message
   - Check that `TxTone` is set correctly (not 0)

2. **Send a test transmission:**
   - Send a simple pattern: `[100, 100, 300]` (dit-dah)
   - Verify other clients hear it at your TX tone

3. **Receive a transmission:**
   - Have another client send morse
   - Parse the `Duration` array and `TxTone` field
   - Play the tone at the sender's frequency

## Common Issues

### TX Tone shows as 0
- Ensure you're sending `TxTone` in the initial connection message
- Update to latest server version that accepts TX tone on connect

### Audio timing issues
- Synchronize clock with NTP
- Account for network latency in your timing
- Consider RX delay (clients typically use 2-4 seconds)

### Connection drops
- Send keepalive messages every 30 seconds
- Handle reconnection gracefully
- Check WiFi signal strength

## Additional Notes

- **Private Rooms:** Set `"Private": true` in initial message to create a private room
- **Multiple Rooms:** Connect to different endpoints with different `?repeater=` parameters
- **SSL/TLS:** Production servers may use `wss://` instead of `ws://`
- **Port:** Default is 8080, but production may use 443 (HTTPS/WSS)

## Support

For issues or questions:
- Email: ke9bos@pigletradio.org
- Discord: https://discord.gg/GBzj8cBat7

---

**Last Updated:** October 2025
**Protocol Version:** 1.0
**Compatible Server Version:** October 2025+
