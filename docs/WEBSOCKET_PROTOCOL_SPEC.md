# Vail Repeater WebSocket Protocol Specification

**Version:** TBD
**Last Updated:** January 2026
**Target Audience:** Hardware developers, third-party integrators, and client implementers

---

## Table of Contents

1. [Overview](#overview)
2. [Connection Establishment](#connection-establishment)
3. [Message Format](#message-format)
4. [Morse Code Transmission](#morse-code-transmission)
5. [Clock Synchronization](#clock-synchronization)
6. [Keepalive and Heartbeat](#keepalive-and-heartbeat)
7. [Rooms and Channels](#rooms-and-channels)
8. [Chat Messages](#chat-messages)
9. [Best Practices and Rate Limiting](#best-practices-and-rate-limiting)
10. [Error Handling](#error-handling)
11. [Example Implementation](#example-implementation)

---

## Overview

The Vail Repeater is a real-time morse code relay service that allows multiple users to communicate via CW (continuous wave) over the internet. Think of it as a "party line" for morse code - when one person keys their transmitter, all connected clients hear the tones in near real-time.

### How It Works

1. Clients connect to a **room** (channel) via WebSocket
2. When a client keys their transmitter, timing data is sent to the server
3. The server broadcasts that timing data to **all clients** in the room, including the sender
4. Receiving clients filter out their own echoes and play tones from other users
5. Clock synchronization ensures tones play at the correct time across all clients

### Key Concepts

- **Duration Array:** Morse code is transmitted as alternating tone/silence durations in milliseconds
- **Timestamp:** All messages include a timestamp for synchronization
- **Clock Offset:** Clients calculate the difference between their clock and the server's
- **Rooms:** Isolated channels where users can communicate privately or publicly

---

## Connection Establishment

### WebSocket URL

```
wss://vailmorse.com/chat?repeater={room_name}
```

**Parameters:**
- `repeater` (required): The room/channel name to join

**Example:**
```
wss://vailmorse.com/chat?repeater=General
```

### Subprotocol Negotiation

The server supports two subprotocols. You **must** specify one during the WebSocket handshake or the connection will be rejected:

| Subprotocol | Description |
|-------------|-------------|
| `json.vailmorse.com` | JSON message format (recommended for most integrations) |
| `binary.vailmorse.com` | Binary format (lower overhead, limited features) |

**Example Connection (JavaScript):**
```javascript
const ws = new WebSocket(
    "wss://vailmorse.com/chat?repeater=General",
    ["json.vailmorse.com"]
);
```

**Example Connection (Python):**
```python
import websockets

async with websockets.connect(
    "wss://vailmorse.com/chat?repeater=General",
    subprotocols=["json.vailmorse.com"]
) as ws:
    # Connected
```

### First Message Requirement

**Important:** Immediately after connecting, you must send an initial message to register your client with the room. This message sets your callsign, TX tone, and room privacy settings:

```json
{
    "Timestamp": 1704067200000,
    "Duration": [],
    "Callsign": "K0TEST",
    "TxTone": 72,
    "Private": false,
    "Decoder": false
}
```

The server reads this first message to configure your session. If you don't send it promptly, you may miss messages or have incomplete registration.

---

## Message Format

All messages use JSON when connected with the `json.vailmorse.com` subprotocol.

### Message Structure

```json
{
    "Timestamp": 1704067200000,
    "Duration": [80, 80, 240],
    "Clients": 5,
    "Callsign": "K0TEST",
    "TxTone": 72,
    "Users": ["K0TEST", "W5XYZ"],
    "UsersInfo": [
        {"callsign": "K0TEST", "txTone": 72},
        {"callsign": "W5XYZ", "txTone": 69}
    ],
    "Rooms": [
        {"name": "General", "users": 5, "private": false}
    ],
    "Private": false,
    "Decoder": false,
    "Text": ""
}
```

### Field Reference

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `Timestamp` | int64 | **Yes** | Unix timestamp in **milliseconds** |
| `Duration` | uint16[] | **Yes** | Array of timing values (see [Morse Code Transmission](#morse-code-transmission)) |
| `Clients` | uint16 | No | Number of clients in the room (server-populated) |
| `Callsign` | string | No | Sender's callsign (for identification) |
| `TxTone` | uint8 | No | TX tone as MIDI note number (0-127, 0 = not set) |
| `Users` | string[] | No | List of connected callsigns (server-populated) |
| `UsersInfo` | object[] | No | Extended user info with callsigns and TX tones |
| `Rooms` | object[] | No | List of active public rooms (server-populated) |
| `Private` | bool | No | If true, room is hidden from public room lists |
| `Decoder` | bool | No | If true, morse decoder is enabled for this room |
| `Text` | string | No | Chat message text (see [Chat Messages](#chat-messages)) |

### Binary Protocol Format

When using the `binary.vailmorse.com` subprotocol, messages are encoded as binary with the following structure:

| Offset | Size | Type | Field |
|--------|------|------|-------|
| 0 | 8 bytes | int64 BE | Timestamp (milliseconds since epoch) |
| 8 | 2 bytes | uint16 BE | Clients count |
| 10 | N×2 bytes | uint16[] BE | Duration array |

**Notes:**
- All multi-byte values use **big-endian** byte order
- The binary format only supports `Timestamp`, `Clients`, and `Duration` fields
- Additional fields like `Callsign`, `TxTone`, `Users`, `Rooms`, etc. are **not available** in binary mode
- For full functionality (callsigns, TX tones, chat, room lists), use the JSON protocol

### MIDI Note to Frequency Reference

The `TxTone` field uses MIDI note numbers. Common values:

| MIDI Note | Frequency | Description |
|-----------|-----------|-------------|
| 60 | 262 Hz | Middle C |
| 69 | 440 Hz | A4 (standard tuning) |
| 72 | 523 Hz | C5 (default) |
| 76 | 659 Hz | E5 |

Formula: `frequency = 440 * 2^((midiNote - 69) / 12)`

---

## Morse Code Transmission

### Duration Array Format

The `Duration` field is the core of morse code transmission. It contains an array of durations in **milliseconds** that alternate between:

- **Even indices (0, 2, 4...):** Tone/key-down duration
- **Odd indices (1, 3, 5...):** Silence/key-up duration

### Example: Letter "A" (dit-dah)

At 20 WPM, a dit is approximately 60ms and a dah is 180ms (3x dit), with inter-element spacing of 60ms:

```json
{
    "Timestamp": 1704067200000,
    "Duration": [60, 60, 180]
}
```

Breakdown:
- `60` - dit (tone)
- `60` - inter-element gap (silence)
- `180` - dah (tone)

### Example: Letter "C" (dah-dit-dah-dit)

```json
{
    "Timestamp": 1704067200000,
    "Duration": [180, 60, 60, 60, 180, 60, 60]
}
```

### Sending Individual Key Events

For real-time keying (straight key or paddle), you can send each tone individually:

**Key Down:**
```json
{
    "Timestamp": 1704067200000,
    "Duration": [100]
}
```

**Important:** The timestamp should be when the key was pressed, adjusted for clock offset (see [Clock Synchronization](#clock-synchronization)).

### Message Echo Behavior

**Critical for integrators:** The server broadcasts every message to ALL clients in the room, **including the original sender**. This means:

1. When you send a message, you will receive it back from the server
2. You **must** implement echo detection to avoid playing your own tones
3. The echo can be used to measure round-trip latency
4. The echo confirms the server received and distributed your message

Echo detection compares incoming messages against recently sent messages using `Timestamp` and `Duration` array matching:

```javascript
function isOwnEcho(receivedMsg, sentMessages) {
    for (const sent of sentMessages) {
        if (sent.Timestamp === receivedMsg.Timestamp &&
            arraysEqual(sent.Duration, receivedMsg.Duration)) {
            return true;
        }
    }
    return false;
}
```

### Receiving and Playing Morse

When you receive a message with morse data:

1. Adjust the timestamp using your calculated clock offset
2. Iterate through the Duration array
3. For even indices: play tone for that duration
4. For odd indices: silence for that duration

```javascript
function playMorse(msg, clockOffset) {
    let playTime = msg.Timestamp + clockOffset;
    let isTone = true;

    for (let duration of msg.Duration) {
        if (isTone && duration > 0) {
            scheduleBeep(playTime, duration);
        }
        playTime += duration;
        isTone = !isTone;
    }
}
```

---

## Clock Synchronization

Accurate timing is critical for morse code. The protocol includes built-in clock synchronization.

### How It Works

1. The server broadcasts all messages (including your own) back to all clients in the room
2. When you receive any message, you can calculate your clock offset from the server's timestamp
3. Client calculates offset: `clockOffset = localTime - serverTimestamp`
4. All subsequent timestamps are adjusted by this offset

### Clock Sync from Any Message

Every message from the server contains a `Timestamp` field set by the server. Use messages with empty `Duration` arrays (keepalives, room updates) to update your clock offset without triggering audio playback:

```javascript
function handleMessage(msg) {
    const now = Date.now();

    // Update clock offset from any empty-duration message
    if (msg.Duration.length === 0) {
        this.clockOffset = now - msg.Timestamp;
    }

    // Process morse data if present...
}
```

### Initial Sync

After connecting and sending your first message, the server will broadcast a room update message. Use this to establish your initial clock offset before transmitting.

### Using the Clock Offset

**When sending:**
```javascript
const serverTimestamp = Date.now() - this.clockOffset;
```

**When receiving:**
```javascript
const localPlayTime = msg.Timestamp + this.clockOffset;
```

### Clock Tolerance

The server will disconnect clients whose clocks differ by more than **10 seconds**. Ensure your device has reasonably accurate time.

---

## Keepalive and Heartbeat

### Why Keepalives Are Necessary

- Prevents connection timeout (30-minute inactivity limit)
- Maintains clock synchronization
- Updates user presence in room lists

### Keepalive Interval

Send a keepalive message every **15 seconds** when idle.

### Keepalive Message Format

```json
{
    "Timestamp": 1704067200000,
    "Duration": [],
    "Callsign": "K0TEST",
    "TxTone": 72,
    "Private": false,
    "Decoder": false
}
```

**Critical:** The empty `Duration` array signals this is a keepalive, not morse data.

### Server Behavior

When you send a keepalive (empty Duration, no Text), the server:

1. Updates your activity timestamp (preventing inactivity disconnect)
2. Updates your callsign/TxTone if changed
3. Broadcasts a room update to all clients (including you) with current user list and room info

The broadcast message looks like:

```json
{
    "Timestamp": 1704067200500,
    "Duration": [],
    "Clients": 5,
    "Users": ["K0TEST", "W5XYZ"],
    "UsersInfo": [
        {"callsign": "K0TEST", "txTone": 72},
        {"callsign": "W5XYZ", "txTone": 69}
    ],
    "Rooms": [
        {"name": "General", "users": 5, "private": false}
    ],
    "Decoder": false
}
```

**Note:** The server does NOT echo your exact keepalive message back. Instead, it broadcasts a room status update to all clients. Use the `Timestamp` from any received empty-Duration message for clock synchronization.

---

## Rooms and Channels

### Room Behavior

- Rooms are created automatically when the first user joins
- Rooms are destroyed after being empty for 15+ minutes
- Room names are case-sensitive
- Users only receive messages from their own room

### Public vs Private Rooms

**Public Rooms (default):**
- Listed in the `Rooms` array sent to all clients
- User count is visible to everyone

**Private Rooms:**
- Set `"Private": true` in your keepalive messages
- Room will not appear in public room lists
- Still accessible if someone knows the exact room name

### Joining a Room

Simply connect to the WebSocket URL with your desired room name:

```
wss://vailmorse.com/chat?repeater=MyPrivateRoom
```

### Room Information

The server periodically broadcasts room information (every 2 seconds, rate-limited):

```json
{
    "Rooms": [
        {"name": "General", "users": 5, "private": false},
        {"name": "Advanced", "users": 2, "private": false}
    ]
}
```

---

## Chat Messages

The protocol supports text chat alongside morse code.

### Sending a Chat Message

```json
{
    "Timestamp": 1704067200000,
    "Duration": [],
    "Text": "Hello everyone!",
    "Callsign": "K0TEST"
}
```

### Receiving Chat Messages

Chat messages are broadcast to all room members and include the sender's callsign.

### Chat History

The server maintains the last 50 chat messages per room. New clients receive this history upon connection.

---

## Best Practices and Rate Limiting

### Preventing Server Overload

The Vail Repeater is a free community service. Please follow these guidelines to ensure it remains available:

#### 1. Keepalive Frequency
- Send keepalives **no more than once every 15 seconds**
- Do not send keepalives more frequently during active transmission

#### 2. Transmission Batching
- When possible, batch multiple elements into a single message
- Instead of sending each dit/dah separately, send complete characters or words

**Good (batched):**
```json
{"Duration": [60, 60, 180, 60, 60]}
```

**Avoid (individual elements):**
```json
{"Duration": [60]}
{"Duration": [180]}
{"Duration": [60]}
```

#### 3. Reconnection Backoff
- If disconnected, wait **at least 2 seconds** before reconnecting
- Implement exponential backoff for repeated failures
- **Never** implement aggressive reconnection loops

```javascript
let backoffMs = 2000;
const maxBackoff = 60000;

function reconnect() {
    setTimeout(() => {
        connect();
        backoffMs = Math.min(backoffMs * 2, maxBackoff);
    }, backoffMs);
}

function onConnected() {
    backoffMs = 2000; // Reset on successful connection
}
```

#### 4. Message Size Limits
- Keep `Duration` arrays reasonable (< 1000 elements)
- Avoid sending extremely long transmissions in a single message
- Break up long transmissions into multiple messages

### Preventing Runaway Events

#### Echo Detection

Your client will receive its own messages echoed back. **You must filter these out** to prevent feedback loops:

```javascript
const sentMessages = [];

function send(msg) {
    sentMessages.push(msg);
    ws.send(JSON.stringify(msg));

    // Clean up old sent messages after 5 seconds
    setTimeout(() => {
        const index = sentMessages.indexOf(msg);
        if (index > -1) sentMessages.splice(index, 1);
    }, 5000);
}

function receive(msg) {
    // Check if this is our own echo
    for (const sent of sentMessages) {
        if (messagesEqual(sent, msg)) {
            return; // Ignore our own echo
        }
    }

    // Process received message
    playMorse(msg);
}
```

#### Preventing Audio Feedback Loops

If your hardware has both a microphone and speaker:
- Mute the speaker while transmitting
- Add a squelch delay after transmission
- Use separate audio channels for TX and RX

#### Rate Limiting Outbound Messages

Implement client-side rate limiting:

```javascript
const MIN_MESSAGE_INTERVAL = 10; // milliseconds
let lastSendTime = 0;

function send(msg) {
    const now = Date.now();
    if (now - lastSendTime < MIN_MESSAGE_INTERVAL) {
        // Queue or drop message
        return;
    }
    lastSendTime = now;
    ws.send(JSON.stringify(msg));
}
```

### Connection Limits

- Do not open multiple connections to the same room
- Close connections cleanly when done
- Implement proper cleanup on application shutdown

---

## Error Handling

### Server Disconnect Reasons

| Reason | Description | Action |
|--------|-------------|--------|
| `inactivity` | No activity for 30 minutes | Reconnect on next user action |
| `clock skew` | Client clock differs by >10 seconds | Check system time, then reconnect |
| `invalid message` | Malformed JSON or protocol error | Check message format |

### Handling Disconnections

```javascript
ws.onclose = function(event) {
    if (event.reason.includes("inactivity")) {
        // User was idle - reconnect when they want to transmit
        awaitingReconnect = true;
    } else {
        // Unexpected disconnect - auto-reconnect with backoff
        scheduleReconnect();
    }
};
```

### Connection Health Monitoring

Track round-trip latency to detect connection issues:

```javascript
function measureLatency(sentMsg, receivedMsg) {
    const rtt = Date.now() - sentMsg.Timestamp - clockOffset;
    latencyHistory.push(rtt);

    if (rtt > 500) {
        console.warn("High latency detected:", rtt, "ms");
    }
}
```

---

## Example Implementation

### Minimal JavaScript Client

```javascript
class VailClient {
    constructor(room, callsign) {
        this.room = room;
        this.callsign = callsign;
        this.clockOffset = 0;
        this.sentMessages = [];
        this.connect();
    }

    connect() {
        const url = `wss://vailmorse.com/chat?repeater=${encodeURIComponent(this.room)}`;
        this.ws = new WebSocket(url, ["json.vailmorse.com"]);

        this.ws.onopen = () => {
            console.log("Connected to", this.room);
            this.sendKeepalive();
            this.keepaliveInterval = setInterval(() => this.sendKeepalive(), 15000);
        };

        this.ws.onmessage = (event) => {
            const msg = JSON.parse(event.data);
            this.handleMessage(msg);
        };

        this.ws.onclose = (event) => {
            clearInterval(this.keepaliveInterval);
            console.log("Disconnected:", event.reason);
            // Reconnect with backoff
            setTimeout(() => this.connect(), 2000);
        };
    }

    handleMessage(msg) {
        // Clock sync (empty Duration)
        if (msg.Duration.length === 0) {
            this.clockOffset = Date.now() - msg.Timestamp;
            return;
        }

        // Echo detection
        if (this.isOwnMessage(msg)) {
            return;
        }

        // Play received morse
        this.playMorse(msg);
    }

    isOwnMessage(msg) {
        for (let i = 0; i < this.sentMessages.length; i++) {
            const sent = this.sentMessages[i];
            if (sent.Timestamp === msg.Timestamp &&
                JSON.stringify(sent.Duration) === JSON.stringify(msg.Duration)) {
                this.sentMessages.splice(i, 1);
                return true;
            }
        }
        return false;
    }

    playMorse(msg) {
        let playTime = msg.Timestamp + this.clockOffset;
        let isTone = true;

        for (const duration of msg.Duration) {
            if (isTone && duration > 0) {
                this.scheduleBeep(playTime, duration);
            }
            playTime += duration;
            isTone = !isTone;
        }
    }

    scheduleBeep(startTime, duration) {
        // Implement audio playback based on your platform
        console.log("Beep at", startTime, "for", duration, "ms");
    }

    sendKeepalive() {
        this.ws.send(JSON.stringify({
            Timestamp: Date.now() - this.clockOffset,
            Duration: [],
            Callsign: this.callsign
        }));
    }

    transmit(durations) {
        const msg = {
            Timestamp: Date.now() - this.clockOffset,
            Duration: durations,
            Callsign: this.callsign
        };
        this.sentMessages.push(msg);
        this.ws.send(JSON.stringify(msg));

        // Clean up after 5 seconds
        setTimeout(() => {
            const idx = this.sentMessages.indexOf(msg);
            if (idx > -1) this.sentMessages.splice(idx, 1);
        }, 5000);
    }

    disconnect() {
        clearInterval(this.keepaliveInterval);
        this.ws.close();
    }
}

// Usage
const client = new VailClient("General", "K0TEST");

// Send letter "A" (dit-dah) at 20 WPM
client.transmit([60, 60, 180]);
```

### Hardware Integration Tips

For microcontrollers and embedded systems:

1. **JSON Parsing:** Use a lightweight JSON library or manual parsing
2. **WebSocket:** Many platforms have WebSocket libraries (ESP8266/ESP32, Arduino, etc.)
3. **Timing Precision:** Use hardware timers for accurate tone generation
4. **Network Latency:** Buffer incoming messages and schedule playback based on timestamps
5. **Memory:** The `Duration` array can be large; consider streaming or chunked processing

---

## Summary

| Parameter | Value |
|-----------|-------|
| WebSocket URL | `wss://vailmorse.com/chat?repeater={room}` |
| Subprotocol | `json.vailmorse.com` |
| Keepalive Interval | 15 seconds |
| Inactivity Timeout | 30 minutes |
| Clock Tolerance | +/- 10 seconds |
| Reconnect Backoff | Minimum 2 seconds |
| Empty Room Cleanup | 15 minutes |

---

## Contact and Support

For questions about integrating with the Vail Repeater:
- Visit [vailmorse.com](https://vailmorse.com)
- Open an issue on the project repository

Please be a good citizen of the Vail community - respect the shared resource and follow the rate limiting guidelines above.
