# Architecture

This document covers the system architecture, state machine, main loop responsibilities, and critical subsystems.

## Mode-Based State Machine

The system operates as a state machine with different modes (`MenuMode` enum in morse_trainer_menu.ino:32-44). Each mode has its own input handler and UI renderer. The main loop delegates to the appropriate mode handler based on `currentMode`.

### Key Modes

**Menu Navigation:**
- `MODE_MAIN_MENU` - Top-level menu (Training, Games, Settings, Radio, Logs, Sleep)
- `MODE_TRAINING_MENU` - Training submenu
- `MODE_SETTINGS_MENU` - Settings submenu
- `MODE_GAMES_MENU` - Games submenu
- `MODE_RADIO_MENU` - Radio submenu

**Training Modes:**
- `MODE_HEAR_IT_TYPE_IT` - Receive training (type what you hear)
- `MODE_PRACTICE` - Practice oscillator with paddle keying
- `MODE_CW_ACADEMY_TRACK_SELECT` - CW Academy track selection
- `MODE_CW_ACADEMY_SESSION_SELECT` - Session selection
- `MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT` - Practice type selection
- `MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT` - Message type selection
- `MODE_CW_ACADEMY_COPY_PRACTICE` - CW Academy copy practice (listen and type)

**Games:**
- `MODE_MORSE_SHOOTER` - Arcade-style game (shoot falling letters with morse code)

**Radio Integration:**
- `MODE_RADIO_OUTPUT` - Key external ham radios via 3.5mm jack outputs
- `MODE_CW_MEMORIES` - Store and playback CW message memories (placeholder)

**Network:**
- `MODE_VAIL_REPEATER` - Internet morse repeater via WebSocket

**Settings:**
- `MODE_WIFI_SETTINGS` - WiFi scanning, connection, credential management
- `MODE_CW_SETTINGS` - CW speed, tone frequency, key type
- `MODE_VOLUME_SETTINGS` - Volume adjustment
- `MODE_CALLSIGN_SETTINGS` - User callsign configuration

**Logging:**
- QSO Logger accessed via web interface (no dedicated device mode for input)

## Main Loop Responsibilities

The `loop()` function (morse_trainer_menu.ino:210-258) performs:

1. **Status Icon Updates** - Every 5 seconds (except during practice/training to avoid audio interference)
2. **Mode-Specific Updates** - Calls update functions (`updatePracticeOscillator()`, `updateVailRepeater()`, `updateMorseShooterInput()`, `updateMorseShooterVisuals()`, `updateRadioOutput()`)
3. **Keyboard Polling** - CardKB via I2C at 10ms intervals (50ms during practice/game modes for audio priority)
4. **Triple-ESC Sleep Timeout** - Tracks ESC presses for deep sleep entry

### Loop Timing

**Normal operation:** 10ms delay between iterations
**Audio-critical modes (Practice, Morse Shooter, Radio Output):** 50ms keyboard polling, 1ms main loop delay

## Audio System Architecture

The audio system uses **I2S DMA** for high-quality, glitch-free output through the MAX98357A amplifier. This replaces the legacy PWM buzzer.

### Critical Timing Requirements

1. **I2S DMA has highest interrupt priority** (`ESP_INTR_FLAG_LEVEL3`) to beat SPI display DMA
2. **I2S must be initialized BEFORE the display** to establish DMA priority
3. **Audio buffers are filled in interrupt context** - tone generation must be fast
4. **During practice mode, display updates are COMPLETELY DISABLED** to avoid audio glitches
5. **`continueTone()` function maintains phase continuity** for smooth audio transitions

### Volume Control

- **Software attenuation** (0-100%) applied during sample generation in `i2s_audio.h`
- Settings persisted in Preferences namespace "audio"
- Hardware gain on MAX98357A is fixed (GAIN pin configuration)

### Audio Functions

```cpp
beep(frequency_hz, duration_ms);  // Short beeps/tones (blocking)
startTone(frequency_hz);          // Start continuous tone
continueTone();                   // Maintain tone with phase continuity
stopTone();                       // Stop continuous tone
```

**Important:** Always use these functions instead of direct I2S manipulation. They handle volume scaling and phase continuity.

## Morse Code Timing

All timing uses the **PARIS standard** (50 dit units per word):

- **Dit duration:** `1200 / WPM` milliseconds
- **Dah duration:** `3 × dit`
- **Inter-element gap:** `1 × dit`
- **Letter gap:** `3 × dit`
- **Word gap:** `7 × dit`

**WPM Range:** 5-40 WPM (configurable per mode, stored in Preferences)

### MorseTiming Class

```cpp
#include "morse_code.h"
MorseTiming timing(20);  // 20 WPM timing calculator

// Available properties:
// timing.ditDuration
// timing.dahDuration
// timing.elementGap
// timing.letterGap
// timing.wordGap
```

## Input Handling Pattern

Each mode implements three key functions:

### Required Functions

1. **`start<Mode>(tft)`** - Initialize mode state and draw initial UI
2. **`handle<Mode>Input(key, tft)`** - Process keyboard input, return -1 to exit mode
3. **`update<Mode>()`** (optional) - Called every loop iteration for real-time updates

### Return Codes

Input handlers return:
- **`-1`** - Exit mode (return to parent menu)
- **`0` or `1`** - Normal input processed
- **`2`** - Full UI redraw requested
- **`3`** - Partial UI update (e.g., input box only)

### Example Implementation

```cpp
void startMyMode(Adafruit_ST7789& tft) {
  // Initialize state variables
  myModeState = 0;

  // Draw initial UI
  tft.fillScreen(COLOR_BACKGROUND);
  // ... draw UI elements
}

int handleMyModeInput(char key, Adafruit_ST7789& tft) {
  if (key == 0x1B) {  // ESC key
    return -1;  // Exit mode
  }

  // Process other keys
  // ...

  return 0;  // Input processed
}

void updateMyMode() {
  // Called every loop iteration
  // Perform real-time updates (non-blocking)
}
```

## Configuration Management

All user settings are stored in ESP32 Preferences (non-volatile flash storage).

### Preferences Namespaces

- **"wifi"** - Up to 3 WiFi network credentials (ssid1-3, pass1-3)
  - Auto-connect tries all saved networks in order at startup
  - Most recently connected network stored in slot 1
  - Saved networks marked with star (*) in WiFi settings UI
- **"cwsettings"** - WPM speed, tone frequency (Hz), key type
- **"audio"** - Volume percentage
- **"callsign"** - User callsign for Vail repeater
- **"cwa"** - CW Academy progress (track, session, practice type, message type)
- **"radio"** - Radio mode (Summit Keyer vs Radio Keyer)
- **"qso_operator"** - Station info (callsign, grid square, POTA reference)

### Best Practices

- Load settings on startup
- Save immediately when changed (don't batch writes)
- Use consistent key names across modules

## WiFi Configuration and AP Mode

### WiFi Connectivity Modes

The device supports two WiFi modes for flexible connectivity:

**Station (STA) Mode:**
- Default mode - connects to existing WiFi networks
- Auto-connects to saved networks on startup (tries all 3 slots in order)
- Web server accessible via mDNS at `http://vail-summit.local` or device IP
- Supports up to 3 saved network credentials

**Access Point (AP) Mode:**
- Creates its own WiFi network for direct connection
- Useful when no WiFi available or for initial setup
- SSID format: `VAIL-SUMMIT-XXXXXX` (XXXXXX = chip ID in hex)
- Default password: `vailsummit`
- IP address: `192.168.4.1` (standard ESP32 AP address)
- mDNS not available in AP mode (use IP address only)

### Entering AP Mode

**From Device:**
1. Navigate to Settings → WiFi Setup
2. Press 'A' key to enable AP mode
3. Device creates WiFi network and starts web server
4. Screen shows network name, password, and IP address

**Connecting to AP:**
1. On phone/laptop, connect to `VAIL-SUMMIT-XXXXXX` WiFi network
2. Enter password: `vailsummit`
3. Open browser to `http://192.168.4.1/`
4. Access web interface for device configuration

### Automatic AP Mode Exit

When connecting to a WiFi network from AP mode (via web or device):

1. **Device automatically:**
   - Stops AP mode and web server
   - Connects to the selected WiFi network
   - Saves credentials to preferences
   - Shows "Connected!" message for 2 seconds
   - Returns to main menu
   - Web server restarts in Station mode (via WiFi event handler)

2. **User experience:**
   - Web interface shows success modal: "Check Your Device"
   - Phone loses connection to AP (expected behavior)
   - Device is now accessible on the new WiFi network at its assigned IP

3. **Error handling:**
   - If connection fails, AP mode automatically restarts
   - User can try again without manual intervention

### Web Server Behavior

**WiFi Event Handler** (vail-summit.ino:182-190):
- Automatically starts web server when WiFi connects (Station mode)
- Automatically stops web server when WiFi disconnects
- `setupWebServer()` detects AP vs Station mode and configures accordingly

**AP Mode Web Server:**
- Starts automatically when `startAPMode()` is called
- Skips mDNS setup (not supported in AP mode)
- Accessible only via IP address: `http://192.168.4.1/`
- Stops automatically when AP mode exits

**Station Mode Web Server:**
- Starts via WiFi event handler on connection
- Includes mDNS responder: `http://vail-summit.local/`
- Accessible via mDNS or IP address
- Stops via WiFi event handler on disconnection

### WiFi Settings Module (settings_wifi.h)

**State Machine:**
```cpp
enum WiFiSettingsState {
  WIFI_STATE_SCANNING,        // Scanning for networks
  WIFI_STATE_NETWORK_LIST,    // Showing available networks
  WIFI_STATE_PASSWORD_INPUT,  // Entering password
  WIFI_STATE_CONNECTING,      // Attempting connection
  WIFI_STATE_CONNECTED,       // Successfully connected
  WIFI_STATE_ERROR,          // Connection failed
  WIFI_STATE_RESET_CONFIRM,  // Confirming credential reset
  WIFI_STATE_AP_MODE         // Access Point mode active
};
```

**Key Functions:**
- `startAPMode()` - Creates AP, starts web server
- `stopAPMode()` - Stops AP, returns to Station mode
- `connectToWiFi(ssid, password)` - Handles connection with automatic AP exit
- `scanNetworks()` - Scans for available WiFi networks
- `saveWiFiCredentials()` - Stores up to 3 network credentials
- `autoConnectWiFi()` - Tries saved networks on startup

**Tracking Variables:**
- `isAPMode` - Global flag for AP mode state
- `connectedFromAPMode` - Tracks if connection made from AP mode
- `connectionSuccessTime` - Timestamp for auto-exit timer

## Vail Repeater Protocol

The Vail repeater (vail_repeater.h) uses a WebSocket connection with JSON messages:

### Transmission Format

```json
{"Timestamp":1759710473428,"Clients":0,"Duration":[198]}
```

- Each tone sent immediately as separate message
- **Timestamp:** Unix epoch milliseconds (when tone started)
- **Duration array:** Contains single tone duration in milliseconds
- Silences are implicit (gaps between tones)

### Reception Format

```json
{"Timestamp":1759710473428,"Clients":2,"Duration":[100,50,100,150]}
```

- **Even indices** (0, 2, 4...): tone durations
- **Odd indices** (1, 3, 5...): silence durations
- Clock skew calculated from initial handshake for synchronization
- 500ms playback delay buffer for network jitter
- Echo filtering: messages with our own timestamp are ignored

## Battery Monitoring

Two battery monitor chips are supported (I2C auto-detection on startup):

- **MAX17048** at address `0x36` (primary, used on Adafruit ESP32-S3 Feather V2)
- **LC709203F** at address `0x0B` (backup/alternative)

The fuel gauge chips provide voltage and state-of-charge percentage. Calibration improves over several charge/discharge cycles.

**USB charging detection is disabled** because GPIO 15 (A3) is used by I2S for the LRC clock signal. Using `analogRead(A3)` reconfigures the pin and completely breaks I2S audio output.
