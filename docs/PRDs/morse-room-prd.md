# Morse Room - Multi-Device Communication PRD

## Overview
Morse Room is a real-time multi-device communication feature that allows 3-5 VAIL SUMMIT devices to communicate together via broadcast morse code transmission. Using ESP-NOW protocol, devices can send and receive morse code with ultra-low latency (<10ms), creating a local "morse chat room" for practice sessions, group training, or social CW exchanges.

## Product Vision
Enable morse code operators to practice and communicate together in a shared wireless environment, similar to being in the same room with traditional oscillators and keys, but with modern wireless convenience and visual feedback.

## Core User Experience

### User Journey
1. **Enter Morse Room**: Navigate to Training → Morse Room from main menu
2. **Auto-Discovery**: Device automatically discovers nearby VAIL SUMMIT devices
3. **Send Morse**: Key morse code using paddle/touch input
4. **Receive Morse**: Hear other operators' morse through speaker
5. **Visual Feedback**: See who's currently keying and their callsigns
6. **Exit**: Return to training menu to restore WiFi connectivity

### Key User Benefits
- **Real-time practice** with multiple operators
- **Ultra-low latency** - feels like direct connection
- **No network setup** required - automatic peer discovery
- **Visual indicators** showing who's transmitting
- **WPM tracking** for each operator

## Technical Architecture

### ESP-NOW Protocol

**Why ESP-NOW?**
- **Ultra-low latency**: <10ms end-to-end (vs 20-60ms for WiFi WebSocket)
- **Connectionless**: No pairing or AP setup required
- **Broadcast support**: True multicast to all nearby devices
- **Low power**: More efficient than WiFi TCP/IP stack
- **Simple implementation**: ~500 lines of code

**Technical Specifications:**
- **Protocol**: Espressif ESP-NOW (2.4GHz WiFi-based)
- **Max payload**: 250 bytes per packet
- **Peer limit**: 20 unencrypted peers (more than sufficient)
- **Range**: 10-30m indoors, 100m+ outdoors
- **Data rate**: 1 Mbps
- **Packet loss**: <0.1% in clean environment

### Trade-off: WiFi Coexistence

**Critical Limitation:**
ESP-NOW experiences >80% packet loss when device is connected to WiFi Station mode (connected to AP).

**Solution: Modal Operation**
- Morse Room mode **disables WiFi Station** when active
- Web server, Vail repeater unavailable during Morse Room session
- WiFi automatically reconnects when exiting Morse Room
- Similar to existing modal architecture (Practice mode, Radio Output, etc.)

**Rationale:**
- Morse Room is a focused training mode (like Practice)
- Users won't need web interface during live morse session
- Ultra-low latency is more valuable than WiFi connectivity for this use case
- Seamless WiFi reconnection on exit maintains convenience

### Data Structures

#### Morse Event Packet
```cpp
struct MorseEvent {
  char deviceName[16];    // "VAIL-K7ABC"
  char callsign[12];      // "K7ABC"
  uint8_t elementType;    // 0=DIT, 1=DAH, 2=SPACE, 3=WORD_SPACE
  uint16_t duration;      // Duration in milliseconds
  uint32_t timestamp;     // millis() when sent
  uint8_t wpm;            // Sender's current WPM setting
} __attribute__((packed));  // Total: 35 bytes
```

**Bandwidth Analysis:**
- Average morse: ~5-10 events/second per operator
- 5 operators sending simultaneously: ~50 events/second
- Data rate: 50 events × 35 bytes = 1750 bytes/second (~14 Kbps)
- Well within ESP-NOW 1 Mbps capacity

#### Device Tracking
```cpp
struct ConnectedDevice {
  uint8_t macAddress[6];      // Hardware MAC address
  char deviceName[16];        // User-friendly name
  char callsign[12];          // Ham radio callsign
  int rssi;                   // Signal strength (dBm)
  unsigned long lastSeen;     // Timestamp of last packet
  bool isActive;              // Currently keying morse
  uint8_t lastWPM;            // Most recent WPM
};
```

**Device List Management:**
- Track up to 20 devices (ESP-NOW peer limit)
- Remove devices after 30 seconds of inactivity
- RSSI updates on every received packet
- Active indicator set on morse events, cleared after 500ms

### Broadcast Architecture

**Pattern: True Broadcast**
- All devices send to broadcast MAC: `FF:FF:FF:FF:FF:FF`
- No hub/spoke or mesh topology needed
- Every device receives all packets simultaneously
- Connectionless - no connection management overhead

**Advantages over WiFi WebSocket:**
| Metric | ESP-NOW | WebSocket |
|--------|---------|-----------|
| Latency | <10ms | 20-60ms |
| Setup | Automatic | Requires AP host |
| Connections | Connectionless | Client/server model |
| Bandwidth | 1 Mbps | Depends on WiFi |
| Simultaneous TX | True broadcast | Server relay |

## Display & UI Design

### Morse Room Screen Layout

```
┌──────────────────────────────────┐
│ MORSE ROOM      [5 devices] 📶   │ ← Header
├──────────────────────────────────┤
│ Connected Devices:               │
│ ● K7ABC (VAIL-K7ABC)  -65dBm    │ ← Active (keying)
│ ○ W1AW  (VAIL-W1AW)   -70dBm    │ ← Idle
│ ○ N3FI  (VAIL-N3FI)   -68dBm    │
│ ○ KE7Z  (VAIL-KE7Z)   -72dBm    │
│ ○ AI6YR (VAIL-AI6YR)  -71dBm    │
├──────────────────────────────────┤
│ Active: K7ABC (18 WPM)          │ ← Current sender
│ THE QUICK BROWN FOX              │ ← Decoded text
├──────────────────────────────────┤
│ D=Decoder ON  ESC=Exit          │ ← Footer
└──────────────────────────────────┘
```

### UI Elements

**Device List:**
- Scrollable list of connected devices
- Color-coded status: Green (active), White (idle), Gray (weak signal)
- RSSI indicator: Color bars or dBm value
- Callsign and device name
- Bullet indicator: Filled (●) when keying, empty (○) when idle

**Active Sender Panel:**
- Large callsign display of current sender
- WPM indicator (updated in real-time)
- Decoded morse text (if decoder enabled)
- Visual "keying" animation (pulse effect)

**Footer Controls:**
- **D key**: Toggle morse decoder on/off
- **ESC key**: Exit Morse Room (prompt if decoder had content)
- **Status indicators**: Device count, own signal strength

### Visual Feedback

**When Sending:**
- Own callsign highlights in yellow
- Audio sidetone plays through I2S speaker
- Brief flash on screen border (pulse effect)

**When Receiving:**
- Sender's callsign highlights in cyan
- Audio plays through I2S speaker
- Received morse appears as dots/dashes (if decoder off) or text (if decoder on)

**Signal Strength Indicators:**
- Strong (>-60 dBm): Green bars (full)
- Medium (-60 to -75 dBm): Yellow bars (2/3)
- Weak (<-75 dBm): Red bars (1/3)
- Lost (>30s no packets): Grayed out, removed after timeout

## Feature Specifications

### Phase 1: Core Functionality (MVP)

**1. ESP-NOW Initialization**
- Initialize ESP-NOW on mode entry
- Register broadcast peer (`FF:FF:FF:FF:FF:FF`)
- Set up send/receive callbacks
- Configure WiFi in STA mode (disconnected) for ESP-NOW operation

**2. Morse Transmission**
- Capture paddle/touch input (dit/dah)
- Generate `MorseEvent` packets with timing
- Broadcast to all nearby devices
- Play local sidetone through I2S

**3. Morse Reception**
- Receive `MorseEvent` packets from other devices
- Extract sender info (callsign, device name, WPM)
- Play morse tone through I2S audio
- Update device list and UI

**4. Device Discovery**
- Automatically detect nearby VAIL SUMMIT devices
- Track MAC address, callsign, device name
- Update RSSI values from packet metadata
- Remove stale devices (30s timeout)

**5. Basic UI**
- Display connected devices list
- Show current sender (callsign + WPM)
- Visual keying indicators
- Exit to training menu

**6. Settings Persistence**
- Save device name to Preferences
- Save morse room preferences (decoder state, last WPM)
- Load callsign from existing General settings

### Phase 2: Enhanced Features

**7. Real-time Morse Decoder**
- Integrate adaptive morse decoder (from Practice mode)
- Decode received morse to text
- Display decoded characters on screen
- Toggle decoder on/off with 'D' key
- Per-sender decoder state (track sequence per device)

**8. WPM Tracking**
- Display each sender's WPM in real-time
- Color-code WPM display (green if matches local WPM, yellow if different)
- Track WPM history per device
- Show average WPM for session

**9. RSSI Signal Strength**
- Display signal strength per device (dBm)
- Visual bar indicators (3-bar system)
- Color coding (green/yellow/red)
- Sort devices by signal strength option

**10. Device Naming**
- Custom device name beyond callsign
- Edit via settings screen
- Broadcast in morse event packets
- Display in device list

**11. Selective Muting**
- Long-press on device to mute/unmute
- Muted devices don't play audio
- Visual indicator for muted state
- Persist mute list in session

**12. Statistics & Logging**
- Session duration tracking
- Total characters sent/received per device
- Connection history (who joined when)
- Export session log to SPIFFS

### Phase 3: Advanced Features (Future)

**13. Multi-channel Support**
- Create named "rooms" (channels)
- ESP-NOW channel field to separate groups
- QR code for room sharing
- Password-protected rooms (encrypted ESP-NOW)

**14. Frequency Assignment**
- Each device plays different tone frequency
- Makes it easier to distinguish senders aurally
- Configurable frequency offset
- Visual frequency indicator

**15. Message History**
- Scrollable history of decoded text
- Timestamp per message
- Save history to SPIFFS
- Export as text file

**16. QSO Mode**
- Structured QSO exchange template
- Track sent/received signal reports
- Auto-log to QSO Logger
- Contest exchange validation

## Input Handling

### Keyboard Controls

**In Device List:**
- **Up/Down arrows**: Scroll device list
- **D key**: Toggle morse decoder
- **M key**: Mute/unmute selected device (Phase 2)
- **ESC key**: Exit Morse Room mode

**Paddle/Touch Input:**
- **DIT paddle/touch**: Send dit (broadcast + local sidetone)
- **DAH paddle/touch**: Send dah (broadcast + local sidetone)
- **Iambic mode**: Full iambic keyer support (from Practice mode)

### Input Processing Flow

```
Paddle Press (DIT/DAH)
    ↓
Generate MorseEvent packet
    ↓
Broadcast via ESP-NOW (to all devices)
    ↓
Play local sidetone (I2S audio)
    ↓
Update UI (own callsign highlight)
```

```
ESP-NOW Receive Callback
    ↓
Parse MorseEvent packet
    ↓
Update device list (callsign, RSSI, lastSeen)
    ↓
Queue audio playback (I2S buffer)
    ↓
Update UI (sender highlight + decoded text)
```

## Audio System Integration

### I2S Audio Playback

**Local Transmission:**
- Play sidetone immediately on paddle press
- Use existing `beep()` function from `i2s_audio.h`
- Duration matches actual paddle press time
- Frequency: user's configured CW tone (default 700 Hz)

**Remote Reception:**
- Parse `duration` field from `MorseEvent`
- Call `beep(frequency, duration)` for received morse
- Non-blocking playback (queued in I2S buffer)
- Mix multiple simultaneous senders (if needed)

**Audio Priority:**
- Morse Room audio has highest priority
- No display updates during active audio (existing pattern)
- Keyboard polling reduced to 50ms (like Practice mode)

### Volume Control

- Use existing volume settings from Preferences
- Apply volume scaling to both local and remote morse
- Mute option disables audio but keeps visual feedback

## Data Persistence

### Preferences Storage

**Namespace: "morse_room"**
```cpp
preferences.begin("morse_room", false);

// Device identification
String deviceName = preferences.getString("device_name", "VAIL-" + lastFourMAC);

// Session settings
bool decoderEnabled = preferences.getBool("decoder_on", true);
uint8_t lastWPM = preferences.getUInt8("last_wpm", 20);
uint16_t toneFreq = preferences.getUInt16("tone_freq", 700);

// Statistics
uint32_t totalSessions = preferences.getUInt32("total_sessions", 0);
uint32_t totalCharsSent = preferences.getUInt32("chars_sent", 0);
uint32_t totalCharsRecv = preferences.getUInt32("chars_recv", 0);

preferences.end();
```

### Session Data

**SPIFFS Storage (Optional Phase 3):**
- Session logs: `/logs/morse_room_YYYYMMDD_HHMMSS.json`
- Contains: Participant list, timestamps, decoded text, statistics
- Exportable via web interface

## Implementation Details

### File Structure

**New Files:**
```
morse_room_espnow.h          (~500 lines)
├── ESP-NOW initialization
├── Send/receive callbacks
├── Device list management
├── Morse event handlers
├── UI rendering functions
└── Preferences management
```

**Modified Files:**
```
menu_ui.h                     (+3 lines)
├── Add MODE_MORSE_ROOM enum
├── Add "Morse Room" to training menu

menu_navigation.h             (+15 lines)
├── Add selectMenuItem() case
├── Add handleKeyPress() routing

vail-summit.ino              (+8 lines)
├── Include morse_room_espnow.h
├── Add updateMorseRoom() call in loop()
├── Exclude from status updates
```

### ESP-NOW Setup Code

```cpp
// Initialize ESP-NOW (morse_room_espnow.h)
void initMorseRoomESPNOW() {
  // Set WiFi mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Don't connect to AP

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    statusMessage = "ESP-NOW Error";
    return;
  }

  Serial.println("ESP-NOW initialized successfully");

  // Register broadcast peer
  esp_now_peer_info_t peerInfo = {};
  uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memcpy(peerInfo.peer_addr, broadcastAddr, 6);
  peerInfo.channel = 0;  // Use current channel
  peerInfo.encrypt = false;  // No encryption for broadcast

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add broadcast peer");
    return;
  }

  // Register callbacks
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("ESP-NOW ready for morse room");
}
```

### Send Morse Event

```cpp
void sendMorseEvent(uint8_t elementType, uint16_t duration) {
  MorseEvent event;

  // Populate event data
  strncpy(event.deviceName, myDeviceName, 16);
  strncpy(event.callsign, myCallsign, 12);
  event.elementType = elementType;
  event.duration = duration;
  event.timestamp = millis();
  event.wpm = currentWPM;

  // Broadcast to all devices
  uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_err_t result = esp_now_send(broadcastAddr, (uint8_t*)&event, sizeof(event));

  if (result == ESP_OK) {
    Serial.println("Morse event sent successfully");
    totalCharsSent++;
  } else {
    Serial.printf("Send error: %d\n", result);
  }
}
```

### Receive Callback

```cpp
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(MorseEvent)) {
    Serial.printf("Invalid packet size: %d\n", len);
    return;
  }

  MorseEvent* event = (MorseEvent*)data;

  // Update device list
  updateDeviceList(mac, event->deviceName, event->callsign, event->wpm);

  // Get RSSI from WiFi
  wifi_pkt_rx_ctrl_t* rxCtrl = (wifi_pkt_rx_ctrl_t*)data;
  int rssi = rxCtrl->rssi;
  updateDeviceRSSI(mac, rssi);

  // Play morse audio
  if (!isDeviceMuted(mac)) {
    if (event->elementType == 0) { // DIT
      beep(currentToneFreq, event->duration);
    } else if (event->elementType == 1) { // DAH
      beep(currentToneFreq, event->duration);
    }
  }

  // Update UI (flag for main loop)
  morseRoomNeedsUpdate = true;
  lastActiveSender = event->callsign;
  lastSenderWPM = event->wpm;

  totalCharsRecv++;
}
```

### Mode Entry/Exit

```cpp
void startMorseRoom(Adafruit_ST7789& tft) {
  Serial.println("Starting Morse Room mode");

  // Stop web server if running
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Disconnecting WiFi for ESP-NOW");
    stopWebServer();
    WiFi.disconnect();
  }

  // Load settings
  loadMorseRoomSettings();

  // Initialize ESP-NOW
  initMorseRoomESPNOW();

  // Initialize UI state
  deviceListScrollPos = 0;
  lastActiveSender = "";
  decodedText = "";

  // Draw UI
  drawMorseRoomUI(tft);

  Serial.println("Morse Room ready");
}

void exitMorseRoom() {
  Serial.println("Exiting Morse Room mode");

  // Save statistics
  saveMorseRoomStats();

  // Deinitialize ESP-NOW
  esp_now_unregister_send_cb();
  esp_now_unregister_recv_cb();
  esp_now_deinit();

  // Reconnect WiFi
  Serial.println("Reconnecting WiFi...");
  autoConnectWiFi();
}
```

## Testing Plan

### Development Testing

**Phase 1: Single Device**
1. Initialize ESP-NOW successfully
2. Send morse events (verify callback fires)
3. Receive own broadcasts (loopback test)
4. UI updates correctly
5. Exit mode cleanly

**Phase 2: Two Devices**
1. Device A discovers Device B (and vice versa)
2. Device A sends morse → Device B receives and plays audio
3. Device B sends morse → Device A receives and plays audio
4. Bidirectional communication works simultaneously
5. RSSI values update correctly
6. Device list shows both devices

**Phase 3: Three Devices**
1. All three devices see each other
2. Each device receives broadcasts from other two
3. Simultaneous sending doesn't cause packet loss
4. UI updates for all active senders

**Phase 4: Five Devices (Stress Test)**
1. All five devices maintain connection
2. Device list scrolls correctly (if more than 5 visible)
3. Simultaneous morse from multiple senders plays correctly
4. No audio glitches or I2S buffer issues
5. RSSI tracking works for all devices

### Performance Testing

**Latency Measurement:**
- Use oscilloscope or audio analyzer
- Measure time from paddle press to audio output on remote device
- Target: <10ms end-to-end latency
- Test at various distances (1m, 5m, 10m, 20m)

**Packet Loss:**
- Monitor ESP-NOW delivery success rate
- Target: <1% packet loss in clean environment
- Test with WiFi interference (nearby APs, microwaves)
- Verify graceful degradation with poor signal

**Range Testing:**
- Indoor: Measure max range (walls, obstacles)
- Outdoor: Measure line-of-sight range
- Document RSSI thresholds for reliable operation
- Test in various environments (home, ham club, outdoor field)

**Simultaneous Transmission:**
- All 5 devices send morse at same time
- Verify ESP-NOW collision handling
- Check for audio mixing/clipping issues
- Monitor I2S buffer underruns

### User Acceptance Testing

**Scenario 1: Morse Practice Session**
- 3 operators join Morse Room
- Each operator sends practice callsigns
- Operators can hear each other clearly
- No confusion about who's sending
- Easy to exit and return to normal modes

**Scenario 2: Group Training**
- Instructor (1 device) sends morse
- Students (4 devices) copy morse
- All students hear instructor clearly
- Instructor can monitor student list
- WPM tracking shows student speeds

**Scenario 3: Social CW Ragchew**
- 5 operators chat via morse
- Decoder shows text for readability
- Signal strength indicators help troubleshoot
- Session feels natural and responsive
- No frustrating delays or dropouts

## User Documentation

### Quick Start Guide

```
MORSE ROOM - Quick Start

1. Navigate: Main Menu → Training → Morse Room
2. Wait for device discovery (3-5 seconds)
3. Start keying morse with your paddle
4. Hear other operators' morse through speaker
5. Press 'D' to toggle text decoder
6. Press ESC to exit

NOTES:
- WiFi web server unavailable during Morse Room
- Automatically reconnects WiFi on exit
- Works best within 10-30 meters
- Requires other VAIL SUMMIT devices nearby
```

### Troubleshooting

**No devices found:**
- Verify other devices are in Morse Room mode
- Check distance (move closer)
- Restart Morse Room mode
- Check for WiFi interference

**Choppy audio:**
- Reduce distance between devices
- Check signal strength (RSSI)
- Move away from WiFi APs
- Reduce simultaneous senders

**Can't hear specific device:**
- Check if device is muted
- Verify device is sending (watch for highlight)
- Check RSSI (signal may be too weak)

**WiFi won't reconnect:**
- Exit and re-enter Morse Room
- Manually reconnect via WiFi settings
- Restart device if persistent

## Success Metrics

### Technical Metrics
- Latency: <10ms end-to-end (paddle to audio)
- Packet loss: <1% in normal conditions
- Range: 10m+ indoors reliable
- Capacity: 5+ simultaneous devices
- Stability: No crashes during 30+ minute sessions

### User Experience Metrics
- Time to first morse exchange: <30 seconds
- User comprehension: Understand who's sending
- Audio quality: Clear, no glitches
- Ease of exit: Returns to menu reliably
- WiFi restoration: Reconnects automatically

## Future Enhancements

### Hardware Integration
- **LED indicators**: External LEDs show TX/RX activity
- **External speaker**: Optional speaker for group listening
- **Battery optimization**: Low-power ESP-NOW modes

### Software Features
- **Recording**: Save sessions as audio files
- **Replay**: Play back recorded sessions
- **Transcription**: Export decoded text to SPIFFS
- **Web interface**: Monitor morse room from browser (when WiFi enabled)

### Network Extensions
- **ESP-NOW + WiFi**: Investigate hybrid mode (ESP-NOW with AP mode, not STA)
- **Internet bridge**: One device bridges morse room to Vail repeater
- **Mesh routing**: Extended range via multi-hop relay

## Competitive Analysis

### Existing Solutions

**Hardware CW Oscillators:**
- Pros: Simple, reliable, well-understood
- Cons: Wired connections, limited features, no visual feedback

**Vail Internet Repeater (existing VAIL SUMMIT feature):**
- Pros: Global reach, many participants, web-based
- Cons: Requires WiFi/internet, higher latency (~100-300ms), no local operation

**Morse Room (this feature):**
- Pros: Ultra-low latency, local operation, automatic discovery, visual feedback
- Cons: Limited range, requires multiple VAIL SUMMIT devices, no internet connectivity

### Unique Value Proposition
Morse Room fills the gap between wired local oscillators and internet-based systems, providing wireless local communication with modern UX and minimal latency.

## Risks & Mitigations

### Risk 1: WiFi Coexistence
**Risk**: Users expect WiFi and Morse Room to work simultaneously
**Impact**: High - users may be confused by WiFi disconnection
**Mitigation**:
- Clear UI messaging when entering/exiting mode
- Automatic WiFi reconnection on exit
- User documentation explains trade-off
- Future: Investigate ESP-NOW + AP mode compatibility

### Risk 2: ESP-NOW Range Limitations
**Risk**: Users may expect longer range than 10-30m indoors
**Impact**: Medium - limits use cases
**Mitigation**:
- Document expected range in user guide
- RSSI indicators help users optimize placement
- Future: BLE fallback or WiFi UDP option

### Risk 3: Device Compatibility
**Risk**: Only works with VAIL SUMMIT devices (not universal)
**Impact**: Low - feature is VAIL SUMMIT-specific by design
**Mitigation**:
- Document compatibility clearly
- Future: Open protocol specification for third-party devices

### Risk 4: Packet Loss in Crowded RF
**Risk**: Performance degrades in high WiFi interference environments
**Impact**: Medium - affects reliability
**Mitigation**:
- ESP-NOW is more robust than WiFi TCP
- Visual RSSI indicators help diagnose issues
- Graceful degradation (missed packets don't crash)

## Launch Plan

### Development Phases

**Phase 1: Core Implementation (Week 1-2)**
- ESP-NOW initialization and broadcast
- Basic morse transmission/reception
- Device discovery and list management
- Simple UI (device list + morse display)
- Testing with 2-3 devices

**Phase 2: Enhanced Features (Week 3-4)**
- Morse decoder integration
- WPM tracking and display
- RSSI signal strength indicators
- UI polish and animations
- Testing with 5+ devices

**Phase 3: Beta Testing (Week 5-6)**
- Deploy to beta testers with multiple devices
- Gather feedback on latency, range, UX
- Identify and fix edge cases
- Performance optimization

**Phase 4: Release (Week 7)**
- Merge to main branch
- Update documentation (FEATURES.md, CLAUDE.md)
- Release firmware via update.vailadapter.com
- Announce on forums/social media

### Success Criteria for Launch
- ✅ Core functionality works reliably with 5 devices
- ✅ Latency consistently <15ms
- ✅ Packet loss <2% in typical environments
- ✅ No crashes or memory leaks in 1-hour sessions
- ✅ User documentation complete
- ✅ Beta testers report positive experience

## Conclusion

Morse Room represents a unique convergence of modern wireless technology and traditional morse code practice, enabling operators to experience the camaraderie of shared practice sessions without the constraints of wired connections. By leveraging ESP-NOW's ultra-low latency and broadcast capabilities, VAIL SUMMIT can deliver a real-time collaborative experience that feels immediate and natural, filling a valuable niche between local wired setups and internet-based systems.

The modal operation approach (WiFi disabled during Morse Room) is a pragmatic trade-off that prioritizes the core user experience—responsive, low-latency morse communication—while maintaining the device's versatility through automatic reconnection. This PRD provides a comprehensive roadmap for implementation, from core functionality through advanced features, with clear technical specifications and testing criteria to ensure a robust and delightful user experience.
