# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VAIL SUMMIT is a portable morse code training device built on the ESP32-S3 Feather platform with an LCD display and modern UI. It's designed for ham radio operators to practice receiving and sending morse code. Input comes from a CardKB I2C keyboard, iambic paddle, and capacitive touch pads. The device includes training modes, settings management, WiFi connectivity to the Vail internet morse repeater, and extensive hardware integration (battery monitoring, I2S audio).

## Building and Development

### Arduino IDE Setup
- Board: **ESP32S3 Dev Module** or **Adafruit Feather ESP32-S3**
- USB CDC On Boot: **Enabled**
- Flash Size: **4MB**
- PSRAM: **OPI PSRAM**
- Upload Speed: **921600**

### Required Libraries
Install via Arduino Library Manager:
1. Adafruit GFX Library
2. Adafruit ST7735 and ST7789 Library
3. Adafruit MAX1704X (battery monitoring)
4. Adafruit LC709203F (backup battery monitor support)
5. WebSockets by Markus Sattler
6. ArduinoJson by Benoit Blanchon

### Compilation and Upload
```bash
# Open the main sketch in Arduino IDE
arduino morse_trainer/morse_trainer_menu.ino

# Or use arduino-cli
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3 morse_trainer/
arduino-cli upload -p COM<X> --fqbn esp32:esp32:adafruit_feather_esp32s3 morse_trainer/
```

### Serial Monitor
```bash
# Connect at 115200 baud for debug output
arduino-cli monitor -p COM<X> --config baudrate=115200
```

## Architecture

### Mode-Based State Machine
The system operates as a state machine with different modes (`MenuMode` enum in morse_trainer_menu.ino:32-44). Each mode has its own input handler and UI renderer. The main loop delegates to the appropriate mode handler based on `currentMode`.

**Key modes:**
- `MODE_MAIN_MENU` / `MODE_TRAINING_MENU` / `MODE_SETTINGS_MENU` / `MODE_GAMES_MENU` / `MODE_RADIO_MENU`: Menu navigation
- `MODE_HEAR_IT_TYPE_IT`: Receive training (type what you hear)
- `MODE_PRACTICE`: Practice oscillator with paddle keying
- `MODE_CW_ACADEMY_TRACK_SELECT` / `MODE_CW_ACADEMY_SESSION_SELECT` / `MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT` / `MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT`: CW Academy curriculum navigation
- `MODE_CW_ACADEMY_COPY_PRACTICE`: CW Academy copy practice (listen and type)
- `MODE_MORSE_SHOOTER`: Arcade-style game where you shoot falling letters by sending their morse code
- `MODE_RADIO_OUTPUT`: Key external ham radios via 3.5mm jack outputs
- `MODE_CW_MEMORIES`: Store and playback CW message memories (placeholder)
- `MODE_VAIL_REPEATER`: Internet morse repeater via WebSocket
- `MODE_WIFI_SETTINGS` / `MODE_CW_SETTINGS` / `MODE_VOLUME_SETTINGS` / `MODE_CALLSIGN_SETTINGS`: Configuration screens

### Modular Header Files
Each major feature is isolated in its own header file with state, UI drawing, input handling, and update logic:

- **`config.h`**: Central hardware configuration, pin assignments, timing constants, color scheme
- **`morse_code.h`**: Morse lookup table and timing calculations (PARIS method)
- **`morse_wpm.h`**: WPM timing utilities (PARIS standard, Farnsworth) - EUPL v1.2 licensed
- **`morse_decoder.h`**: Base morse decoder class (timings → text) - EUPL v1.2 licensed
- **`morse_decoder_adaptive.h`**: Adaptive speed tracking decoder - EUPL v1.2 licensed
- **`i2s_audio.h`**: I2S audio driver for MAX98357A amplifier with software volume control
- **`training_hear_it_type_it.h`**: Random callsign generator and receive training
- **`training_practice.h`**: Practice oscillator with iambic keyer logic and real-time morse decoding
- **`training_cwa.h`**: CW Academy curriculum with 16-session progression (Session 1-10: character introduction, Session 11-13: QSO practice, Session 14-16: on-air prep)
- **`game_morse_shooter.h`**: Arcade game with falling letters, iambic keyer input, laser shooting, and score tracking
- **`radio_output.h`**: Radio keying output for external ham radios (Summit Keyer and Radio Keyer modes)
- **`radio_cw_memories.h`**: CW message memories for playback (placeholder for future implementation)
- **`settings_wifi.h`**: WiFi scanning, connection, credential storage
- **`settings_cw.h`**: CW speed, tone frequency, key type settings
- **`settings_volume.h`**: Volume adjustment and persistence
- **`settings_general.h`**: User callsign configuration
- **`vail_repeater.h`**: WebSocket client for vail.woozle.org morse repeater
- **`qso_logger.h`**: QSO logging data structures and field definitions
- **`qso_logger_storage.h`**: SPIFFS-based JSON storage for QSO logs
- **`qso_logger_input.h`**: Device-side QSO input forms and field navigation
- **`qso_logger_validation.h`**: Validation functions for callsigns, frequencies, RST reports
- **`web_server.h`**: AsyncWebServer for web-based QSO management and device control
- **`web_logger_enhanced.h`**: HTML/CSS/JavaScript for enhanced web QSO logger interface

### Main Loop Responsibilities
The `loop()` function (morse_trainer_menu.ino:210-258):
1. Updates status icons every 5 seconds (except during practice/training to avoid audio interference)
2. Calls mode-specific update functions (`updatePracticeOscillator()`, `updateVailRepeater()`)
3. Polls CardKB keyboard via I2C at 10ms intervals (50ms during practice for audio priority)
4. Handles triple-ESC sleep timeout tracking

### Audio System Architecture
The audio system uses I2S DMA for high-quality, glitch-free output through the MAX98357A amplifier. This replaces the legacy PWM buzzer.

**Critical timing requirements:**
- I2S DMA has highest interrupt priority (`ESP_INTR_FLAG_LEVEL3`) to beat SPI display DMA
- I2S must be initialized **before** the display to establish DMA priority
- Audio buffers are filled in interrupt context, so tone generation must be fast
- During practice mode, display updates are **completely disabled** to avoid audio glitches
- The `continueTone()` function maintains phase continuity for smooth audio transitions

**Volume control:**
- Software attenuation (0-100%) applied during sample generation in `i2s_audio.h`
- Settings persisted in Preferences namespace "audio"
- Hardware gain on MAX98357A is fixed (GAIN pin configuration)

### Morse Code Timing
All timing uses the **PARIS standard** (50 dit units per word):
- Dit duration: `1200 / WPM` milliseconds
- Dah duration: `3 × dit`
- Inter-element gap: `1 × dit`
- Letter gap: `3 × dit`
- Word gap: `7 × dit`

WPM range: 5-40 WPM (configurable per mode, stored in Preferences)

### Input Handling Pattern
Each mode implements three key functions:
1. **`start<Mode>(tft)`**: Initialize mode state and draw initial UI
2. **`handle<Mode>Input(key, tft)`**: Process keyboard input, return -1 to exit mode
3. **`update<Mode>()`** (optional): Called every loop iteration for real-time updates

Input handlers return:
- `-1`: Exit mode (return to parent menu)
- `0` or `1`: Normal input processed
- `2`: Full UI redraw requested
- `3`: Partial UI update (e.g., input box only)

### Vail Repeater Protocol
The Vail repeater (vail_repeater.h) uses a WebSocket connection with JSON messages:

**Transmission format:**
```json
{"Timestamp":1759710473428,"Clients":0,"Duration":[198]}
```
- Each tone sent immediately as separate message
- Timestamp: Unix epoch milliseconds (when tone started)
- Duration array contains single tone duration in milliseconds
- Silences are implicit (gaps between tones)

**Reception format:**
```json
{"Timestamp":1759710473428,"Clients":2,"Duration":[100,50,100,150]}
```
- Even indices (0,2,4...): tone durations
- Odd indices (1,3,5...): silence durations
- Clock skew calculated from initial handshake for synchronization
- 500ms playback delay buffer for network jitter
- Echo filtering: messages with our own timestamp are ignored

### Battery Monitoring
Two battery monitor chips are supported (I2C auto-detection on startup):
- **MAX17048** at address `0x36` (primary, used on Adafruit ESP32-S3 Feather V2)
- **LC709203F** at address `0x0B` (backup/alternative)

The fuel gauge chips provide voltage and state-of-charge percentage. Calibration improves over several charge/discharge cycles.

**USB charging detection is disabled** because GPIO 15 (A3) is used by I2S for the LRC clock signal. Using `analogRead(A3)` reconfigures the pin and completely breaks I2S audio output.

### Pin Repurposing Notes
**GPIO 5** (originally `BUZZER_PIN`): Now used for capacitive touch dit pad (`TOUCH_DIT_PIN`). PWM buzzer replaced by I2S audio system.

**GPIO 13** (originally `TFT_BL` backlight PWM): Now used for capacitive touch dah pad (`TOUCH_DAH_PIN`). Display backlight is hardwired to 3.3V for always-on operation.

**GPIO 15** (A3, `USB_DETECT_PIN`): Cannot be used for analog input because it's the I2S LRC clock signal. Any `analogRead(A3)` call will break audio.

## Configuration Management

All user settings are stored in ESP32 Preferences (non-volatile flash storage):

- **"wifi"** namespace: Up to 3 WiFi network credentials (ssid1-3, pass1-3)
  - Auto-connect tries all saved networks in order at startup
  - Most recently connected network stored in slot 1
  - Saved networks marked with star (*) in WiFi settings UI
- **"cwsettings"** namespace: WPM speed, tone frequency (Hz), key type
- **"audio"** namespace: volume percentage
- **"callsign"** namespace: user callsign for Vail repeater
- **"cwa"** namespace: CW Academy progress (track, session, practice type, message type)

Settings are loaded on startup and saved immediately when changed.

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

**WiFi Event Handler (vail-summit.ino:182-190):**
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

## Hardware Interfaces

### Display (ST7789V via SPI)
- 240×320 pixels rotated to 320×240 landscape (`SCREEN_ROTATION = 1`)
- Chip select, data/command, and reset pins on GPIOs 10-12
- Hardware SPI on GPIOs 35 (MOSI) and 36 (SCK)
- Backlight hardwired to 3.3V (always on)

### Keyboard (CardKB via I2C)
- I2C address `0x5F` on GPIOs 3 (SDA) and 4 (SCL)
- Special key codes: `0xB5`=UP, `0xB6`=DOWN, `0xB4`=LEFT, `0xB7`=RIGHT, `0x0D`=ENTER, `0x1B`=ESC
- Polled at 10ms intervals (50ms during practice mode)

### Paddle Input
- DIT on GPIO 6, DAH on GPIO 9 (active LOW with internal pullups)
- Supports straight key, Iambic A, and Iambic B modes
- Iambic logic in `training_practice.h` implements memory modes and squeeze keying

### Capacitive Touch
- DIT on GPIO 8 (T8), DAH on GPIO 5 (T5)
- Threshold configured in `config.h` (`TOUCH_THRESHOLD = 40000`)
- Uses ESP32-S3 internal capacitive sensing (no external components required)
- **CRITICAL**: Must use GPIO numbers directly in `touchRead()`, not T-constants (ESP32-S3 bug)
- **CRITICAL**: Touch values RISE when touched on ESP32-S3 (check `> threshold`, not `< threshold`)
- GPIO 13 conflicts with GPIO 14 (I2S/touch shield channel) and causes peripheral freeze
- GPIO 8 and GPIO 5 work reliably together without conflicts

### Radio Keying Output
- DIT output on GPIO 18 (A0), DAH output on GPIO 17 (A1)
- Digital outputs for keying external ham radios via 3.5mm TRS jack
- Requires external transistor/relay driver circuit for radio compatibility

### I2S Audio (MAX98357A)
- BCK on GPIO 14, LRC on GPIO 15, DIN on GPIO 16
- 44.1kHz sample rate, 16-bit stereo
- Software volume control (0-100%)
- Hardware gain set by GAIN pin (float=9dB, GND=12dB, VIN=6dB)

## Firmware Version Management

**Version Information Location:** `config.h` (lines 9-14)

```cpp
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_DATE "2025-01-30"  // Update this date each time you build new firmware
#define FIRMWARE_NAME "VAIL SUMMIT"
```

**When to Update:**
- **FIRMWARE_VERSION**: Increment for major releases (1.0.0 → 1.1.0 → 2.0.0)
  - Major version: Breaking changes or major new features
  - Minor version: New features, backward compatible
  - Patch version: Bug fixes only
- **FIRMWARE_DATE**: Update to current date (YYYY-MM-DD) every time you build firmware for distribution
- **FIRMWARE_NAME**: Should remain "VAIL SUMMIT" unless device name changes

**Where Version Appears:**
- Web dashboard footer
- System Info page (version + build date)
- Serial output on startup
- ADIF export headers

**Important:** Always update FIRMWARE_DATE before building firmware, even if FIRMWARE_VERSION stays the same. This helps track when a specific build was created.

## Firmware Updates

VAIL SUMMIT firmware can be updated via:
1. **Web-based flasher** at `https://update.vailadapter.com` (recommended for users)
2. **Arduino IDE** (for developers)

### Repository Integration

**Important:** This is the Vail Summit source code on the `vail-summit` branch of the vail-adapter repository.

**Branch Structure:**
- **`vail-summit` branch** (this branch): Summit ESP32-S3 firmware source code
- **`master` branch**: Web updater tool + compiled firmware binaries

When firmware changes are ready for distribution:
1. Code is developed and tested on this `vail-summit` branch
2. Firmware is compiled using GitHub Actions workflow (recommended) or Arduino CLI manually
3. Binary files (`bootloader.bin`, `partitions.bin`, `vail-summit.bin`) are committed to `master` branch at `docs/firmware_files/summit/`
4. Users can flash firmware via web updater at `https://update.vailadapter.com`

### Building Firmware for Distribution

**Option 1: GitHub Actions Workflow (Recommended)**

The repository includes a GitHub Actions workflow that automates the build and deployment process:

1. Go to the **Actions** tab on GitHub
2. Select **"Build and Deploy Summit Firmware"**
3. Click **"Run workflow"**
4. Choose the source branch (default: `vail-summit`)
5. Click **"Run workflow"** button

The workflow will:
- Build firmware using Arduino CLI for ESP32-S3 Feather
- Generate `bootloader.bin`, `partitions.bin`, and `vail-summit.bin`
- Automatically switch to `master` branch
- Copy binaries to `docs/firmware_files/summit/`
- Commit and push to `master` (with `[skip ci]` to avoid loops)
- Upload build artifacts for 30-day retention

**Option 2: Manual Arduino CLI Build Process:**
```bash
# The .ino file and .h files are in the root directory
# Create a folder matching the sketch name
mkdir -p vail-summit
cp *.h vail-summit/
cp vail-summit.ino vail-summit/

# Compile for ESP32-S3 Feather
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3 --output-dir build --export-binaries vail-summit/vail-summit.ino

# Generated files in build/:
# - vail-summit.ino.bootloader.bin (bootloader at 0x0)
# - vail-summit.ino.partitions.bin (partition table at 0x8000)
# - vail-summit.ino.bin (application at 0x10000)

# Manually copy to master branch
git fetch origin master:master
git checkout master
mkdir -p docs/firmware_files/summit
cp build/vail-summit.ino.bootloader.bin docs/firmware_files/summit/bootloader.bin
cp build/vail-summit.ino.partitions.bin docs/firmware_files/summit/partitions.bin
cp build/vail-summit.ino.bin docs/firmware_files/summit/vail-summit.bin
git add docs/firmware_files/summit/*.bin
git commit -m "Update Summit firmware [skip ci]"
git push origin master
```

**Firmware Stats:**
- Bootloader: ~23KB
- Partitions: ~3KB
- Application: ~1.3MB
- Total flash time: ~30 seconds

### Web-Based Flasher Details

The web updater at `https://update.vailadapter.com` uses **esptool-js** for browser-based flashing:

**Two-Step Process:**
1. **Step 1: Enter Bootloader Mode**
   - User selects device in normal mode (e.g., COM31)
   - Tool triggers 1200 baud reset to enter bootloader
   - Device reconnects with new COM port (e.g., COM32)

2. **Step 2: Connect and Flash**
   - User selects device in bootloader mode
   - Tool flashes all three binary files with progress indicators
   - User unplugs/replugs device after flashing completes

**Technical Implementation:**
- Uses Web Serial API (Chrome/Edge/Opera only)
- Converts firmware to binary strings for esptool-js compatibility
- MD5 verification disabled to avoid format issues
- Real-time progress indicators for each file
- Dark mode UI matching site theme

See `SUMMIT_INTEGRATION.md` in vail-adapter repo for complete technical details.

## Deep Sleep Power Management

**Entering sleep:** Triple-tap ESC in main menu within 2 seconds

**Wake source:** DIT paddle press (GPIO 6)

**Power consumption:**
- Active (WiFi on): ~200mA
- Active (WiFi off): ~100mA
- Deep sleep: ~20µA

Device performs full restart from `setup()` after wake.

## CW Academy Training Mode

### Overview
The CW Academy mode implements the official CW Academy curriculum across **four training tracks** (documented in `cw-academy-training-mode.md`). Each track is a comprehensive 16-session program with progressive difficulty.

### Training Tracks
1. **Beginner**: Learn CW from zero (4 → 44 characters over Sessions 1-10)
2. **Fundamental**: Build solid foundation (assumes basic CW knowledge)
3. **Intermediate**: Increase speed & skill (higher WPM targets)
4. **Advanced**: Master advanced CW techniques

### Module Architecture: `training_cwa.h`

**Track Structure:**
```cpp
enum CWATrack {
  TRACK_BEGINNER = 0,
  TRACK_FUNDAMENTAL = 1,
  TRACK_INTERMEDIATE = 2,
  TRACK_ADVANCED = 3
};
```

**Session Data Structure:**
```cpp
struct CWASession {
  int sessionNum;           // Session number (1-16)
  int charCount;            // Total characters learned by this session
  const char* newChars;     // New characters introduced
  const char* description;  // Session description
};
```

**Beginner Track Session Progression:**
- **Sessions 1-10**: Progressive character introduction (4 chars → 44 chars)
  - Session 1: A, E, N, T (4 chars) - Foundation
  - Session 2: + S, I, O, 1, 4 (9 chars) - Numbers Begin
  - Session 10: + X, Z, ., <BK>, <SK> (44 chars) - Complete!
- **Sessions 11-13**: QSO (conversation) practice with all 44 characters
- **Sessions 14-16**: On-air preparation and encouragement

**Practice Types:**
```cpp
enum CWAPracticeType {
  PRACTICE_COPY = 0,           // Listen and type what you hear
  PRACTICE_SENDING = 1,        // Send with physical key
  PRACTICE_DAILY_DRILL = 2     // Warm-up drills
};
```
- **Sessions 1-10**: Only Copy Practice available (advanced types locked)
- **Sessions 11+**: All practice types unlocked

**Message Types:**
```cpp
enum CWAMessageType {
  MESSAGE_CHARACTERS = 0,      // Random character practice
  MESSAGE_WORDS = 1,           // Common words
  MESSAGE_ABBREVIATIONS = 2,   // CW abbreviations (73, QSL, etc.)
  MESSAGE_NUMBERS = 3,         // Number sequences
  MESSAGE_CALLSIGNS = 4,       // Random callsigns
  MESSAGE_PHRASES = 5          // Full sentences
};
```

**State Management:**
- Progress saved to ESP32 Preferences namespace "cwa"
- Current track, session, practice type, and message type persisted across reboots
- Functions: `loadCWAProgress()`, `saveCWAProgress()`
- Variables: `cwaSelectedTrack`, `cwaSelectedSession`, `cwaSelectedPracticeType`, `cwaSelectedMessageType`

**Navigation Flow:**
1. Training Menu → CW Academy
2. **Track Selection** (MODE_CW_ACADEMY_TRACK_SELECT)
3. **Session Selection** (MODE_CW_ACADEMY_SESSION_SELECT)
4. **Practice Type Selection** (MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT)
5. **Message Type Selection** (MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT)
6. **Copy Practice** (MODE_CW_ACADEMY_COPY_PRACTICE) - Fully implemented

**UI Screens:**
- `drawCWATrackSelectUI()`: Renders track selection with 4 tracks
  - Shows track name, description, and position (e.g., "2 of 4")
  - Up/down navigation with visual cyan arrows
  - Modern dark blue card design (0x1082 fill, 0x34BF outline)
- `drawCWASessionSelectUI()`: Renders session selection with 16 sessions
  - Shows session number, character count, description, and new characters
  - Up/down navigation with visual cyan arrows
  - Displays context: track name and session position
- `drawCWAPracticeTypeSelectUI()`: Renders practice type selection with 3 types
  - Shows practice type name, description, and position
  - **Locking feature**: Sessions 1-10 show "LOCKED" for Sending/Daily Drill
  - Locked types display "Unlocks at Session 11" hint
  - Displays session context at top
  - Arrows disabled when types are locked
- `drawCWAMessageTypeSelectUI()`: Renders message type selection with 6 types
  - Shows message type name and position
  - Displays practice type context at top
  - Up/down navigation with visual arrows
- `drawCWACopyPracticeUI()`: Renders copy practice mode interface
  - Shows round number (X/10) and current score
  - Displays character count setting ("Chars: X")
  - Three display states:
    - **Listening state**: Shows "Listening..." before morse plays
    - **Input state**: Shows input box with typed characters, "Type what you heard:" prompt
    - **Feedback state**: Shows correct/incorrect with visual color coding (green/red)
  - Context-aware footer with appropriate controls for each state
  - Final score screen after 10 rounds with percentage

**Input Handlers:**
- `handleCWATrackSelectInput()`: Processes track selection input
  - Returns: -1 to exit to training menu, 0 for normal input, 1 to navigate to session selection, 2 for redraw
  - UP/DOWN: Navigate tracks, ENTER: Continue to session selection, ESC: Back to training menu
- `handleCWASessionSelectInput()`: Processes session selection input
  - Returns: -1 to exit to track selection, 0 for normal input, 1 to navigate to practice type selection, 2 for redraw
  - UP/DOWN: Navigate sessions, ENTER: Continue to practice type selection, ESC: Back to track selection
- `handleCWAPracticeTypeSelectInput()`: Processes practice type selection input
  - Returns: -1 to exit to session selection, 0 for normal input, 1 to navigate to message type selection, 2 for redraw
  - UP/DOWN: Navigate practice types (disabled if session <= 10), ENTER: Continue to message type, ESC: Back to session selection
  - Plays error beep (600 Hz, 100ms) when attempting to navigate to locked types
  - Automatically forces selection to PRACTICE_COPY if session <= 10
- `handleCWAMessageTypeSelectInput()`: Processes message type selection input
  - Returns: -1 to exit to practice type selection, 0 for normal input, 1 to start copy practice, 2 for redraw
  - UP/DOWN: Navigate message types, ENTER: Start copy practice mode, ESC: Back to practice type selection
- `handleCWACopyPracticeInput()`: Processes copy practice input
  - Returns: -1 to exit to message type selection, 0 for normal input, 2 for full UI redraw
  - **Character count adjustment** (works in all states):
    - UP arrow: Increase character count (max 10)
    - DOWN arrow: Decrease character count (min 1)
  - **During input state**:
    - SPACE: Replay current morse code
    - ENTER: Submit answer and show feedback
    - Printable characters: Add to input (uppercase, max 20 chars)
  - **During feedback state**:
    - Any key: Continue to next round (or exit if round 10)
  - ESC: Exit to message type selection at any time

**Entry Points:**
- `startCWAcademy()`: Called when user selects "CW Academy" from Training menu
  - Loads saved progress and displays track selection screen
- `startCWACopyPractice()`: Called when user selects message type and presses ENTER
  - Initializes copy practice session (resets round counter and score)
  - Draws initial UI
- `startCWACopyRound()`: Called to begin each of the 10 practice rounds
  - Generates random content using `generateCWAContent()`
  - Displays UI in listening state
  - Plays morse code using `playMorseString()`
  - Transitions to input state

### Implementation Status (Chunks 1.1 + 1.2 + 1.3 Complete)
**Chunk 1.1 - Track and Session Selection:**
- ✅ Menu integration: "CW Academy" added to Training menu
- ✅ Track selection screen with 4 tracks (Beginner/Fundamental/Intermediate/Advanced)
- ✅ Session selection screen with 16 sessions (Beginner track defined)
- ✅ Session data structures with character counts
- ✅ Two-level navigation: Track → Session (ESC goes back)
- ✅ Progress persistence (saved track and session selection)

**Chunk 1.2 - Practice and Message Type Selection:**
- ✅ Practice type selection screen with 3 types (Copy/Sending/Daily Drill)
- ✅ Message type selection screen with 6 types (Characters/Words/Abbreviations/Numbers/Callsigns/Phrases)
- ✅ Complete navigation flow: Track → Session → Practice Type → Message Type
- ✅ Practice type locking: Sessions 1-10 only allow Copy Practice
- ✅ Visual lock indicators with "Unlocks at Session 11" hint
- ✅ Error beep feedback when attempting to access locked content
- ✅ Progress persistence expanded (practice type and message type saved)

**Chunk 1.3 - Copy Practice Mode:**
- ✅ MODE_CW_ACADEMY_COPY_PRACTICE mode added to menu system
- ✅ Complete character sets for all 16 Beginner sessions
  - Session 1: "AENT" (4 characters)
  - Session 2: "AENT SI O14" (9 characters)
  - Session 3-10: Progressive addition up to 44 characters
  - Sessions 11-16: Full character set including prosigns
- ✅ Copy practice implementation with 10-round sessions
  - Clear UI flow: Display input box → Play morse code → Accept user input → Show feedback → Next round
  - Score tracking with correct/total count and percentage
  - Visual feedback (green for correct, red for incorrect)
  - Final score display after 10 rounds
- ✅ Content generation respects session character sets
  - `generateCWAContent()`: Generates random content using only characters available in current session
  - Character mode: Random individual characters
  - Word mode: Simple 2-5 letter word sequences from session characters
  - Other types: Placeholder using session characters
- ✅ Adjustable character count (1-10)
  - UP/DOWN arrow keys to adjust
  - Real-time display of current setting ("Chars: X")
  - Setting applies to all message types
- ✅ Replay functionality
  - SPACE bar replays current round's morse code
  - No conflict with letter input (space excluded from typed characters)
- ✅ Context-aware help text in footer
  - Different instructions for each state (listening, typing, feedback)
  - Shows arrow symbols for character count adjustment
- ✅ State machine with three states
  - Listening: Shows "Listening..." before morse plays
  - Waiting for input: Shows input box and typed characters
  - Showing feedback: Displays correct/incorrect with answer comparison

**Future Chunks:**
- ⏳ Session content for Fundamental/Intermediate/Advanced tracks
- ⏳ Proper word lists for word practice (currently uses random letter sequences)
- ⏳ Abbreviations content (73, QSL, QTH, etc.)
- ⏳ Number sequences and formatted numbers
- ⏳ Realistic callsign generation (W1ABC, K6XYZ patterns)
- ⏳ Common phrases and QSO exchanges
- ⏳ Sending practice mode implementation (send with key)
- ⏳ Daily Drill mode implementation
- ⏳ ICR (Instant Character Recognition) mode for Session 11+
- ⏳ QSO practice mode for Sessions 11-13

### Adding New Practice Content
When adding content for each session (future chunks):
1. Define content arrays in `training_cwa.h` for each message type
2. Organize by session (1-16) and type (characters, words, abbreviations, numbers, callsigns, phrases)
3. Use progressive difficulty within each session
4. Reference `cw-academy-training-mode.md` for official content

## Morse Code Decoder (Adaptive)

### Overview
The morse decoder provides real-time decoding of paddle/key input with adaptive speed tracking. Based on the open-source [morse-pro](https://github.com/scp93ch/morse-pro) JavaScript library by Stephen C Phillips, ported to C++ for ESP32.

### Architecture: Three-Module Design

**Module Structure:**
- **`morse_wpm.h`** - WPM timing utilities (PARIS standard formulas)
- **`morse_decoder.h`** - Base decoder class (timings → morse patterns → text)
- **`morse_decoder_adaptive.h`** - Adaptive speed tracking with weighted averaging

### How It Works

**Input Format:**
- Decoder accepts timing values in milliseconds
- **Positive values** = tone ON (dit or dah)
- **Negative values** = silence (element gap, character gap, word gap)

**Adaptive Speed Algorithm:**
Every decoded element provides speed information:
- Dit → duration = 1 dit
- Dah → duration = 3 dits
- Character gap → duration = 3 fdits (Farnsworth)

The decoder maintains a circular buffer of the last 30 timing samples and uses **weighted averaging** (newer samples weighted more heavily: 1, 2, 3, ..., 30) to continuously refine its WPM estimate.

**Classification Thresholds:**
- Dit/Dah boundary: 2 × dit length
- Dah/Space boundary: 5 × Farnsworth dit length
- Noise threshold: 10ms (filters glitches)

### Integration with Practice Mode

**Real-Time Decoding:**
- Enabled by default in practice mode
- Press 'D' key to toggle display on/off
- Shows decoded text (4-5 lines, word-wrapped, scrolling)
- Displays detected WPM with color coding (green = matches configured, yellow = different)
- Supports 9 prosigns: AR, AS, BK, BT, CT, HH, SK, SN, SOS (displayed as `<AR>`, etc.)

**Timing Capture:**
- **Straight Key**: Measures tone-on and silence durations directly
- **Iambic**: Uses element start/stop times from iambic state machine
- Feeds timings to decoder after each element completes
- **First-run initialization**: `lastStateChangeTime` set to 0 to prevent spurious decoding on first entry to practice mode
- Only calculates silence/tone durations after first valid key press

**UI Update Strategy:**
- Decoder callback sets `needsUIUpdate = true`
- Main loop checks flag after `updatePracticeOscillator()`
- **Only updates when tone is NOT playing** to avoid audio glitches
- Calls `drawDecodedTextOnly()` for partial screen update (decoded text area only)
- Full redraw avoided during practice for best audio performance

**Auto-Flush Logic:**
- Character boundary detection: 2.5 dits of silence triggers automatic flush
- Manual timeout backup: 7 dits (word gap) if auto-flush missed
- Prevents premature character splitting due to timing jitter
- Ensures real-time character display without waiting for next element

### Performance

**Memory Footprint:**
- `MorseDecoderAdaptive` instance: ~1-2 KB
- Decoded text buffers (200 chars): ~200 bytes
- Timing buffers (30 samples × 2): ~240 bytes
- **Total: ~2.5 KB** (negligible on ESP32-S3)

**CPU Usage:**
- Decoder processing: <1ms per character
- No floating-point intensive operations
- Real-time suitable for ESP32 at 240 MHz

### Licensing

The morse decoder modules are licensed under **EUPL v1.2** (European Union Public Licence):
- Original code: Copyright (c) 2024 Stephen C Phillips
- ESP32 port: Copyright (c) 2025 VAIL SUMMIT Contributors
- **Weak copyleft** - can be used in proprietary firmware
- **Must keep decoder modules open source** if modified
- Compatible with GPL, LGPL, MPL

Main VAIL SUMMIT firmware can remain under any license. Only the three decoder modules (`morse_wpm.h`, `morse_decoder.h`, `morse_decoder_adaptive.h`) are EUPL-licensed.

### Future Applications

The decoder is designed as a reusable component for:
1. **CW Academy validation** - Auto-check student answers
2. **Vail repeater decoding** - Decode incoming morse from others
3. **Receive training** - Decode audio from I2S microphone (requires tone detection)
4. **Accuracy metrics** - Compare intended vs. decoded patterns
5. **Contest logging** - Real-time callsign/exchange capture

## Morse Shooter Game

### Overview
The Morse Shooter is an arcade-style game where falling letters descend from the top of the screen. The player uses an iambic keyer (paddle or touch pads) to send morse code patterns that shoot matching letters. It combines entertainment with morse code practice.

### Game Architecture: `game_morse_shooter.h`

**Game Constants:**
```cpp
#define MAX_FALLING_LETTERS 5           // Maximum simultaneous letters
#define LETTER_FALL_SPEED 1             // Pixels per update (slow and steady)
#define LETTER_SPAWN_INTERVAL 3000      // ms between new letter spawns
#define GROUND_Y 225                    // Y position of ground (near bottom)
#define MAX_LIVES 5                     // Lives before game over
#define GAME_UPDATE_INTERVAL 1000       // ms between game physics updates
#define GAME_LETTER_TIMEOUT 1200        // ms before pattern is submitted
```

**Character Set:**
- 36 characters: E, T, I, A, N, M, S, U, R, W, D, K, G, O, H, V, F, L, P, J, B, X, C, Y, Z, Q, 0-9
- Ordered by common morse patterns (easier letters first)

### Iambic Keyer Integration

The game uses the **exact same iambic keyer logic** as practice mode for consistent feel:

**State Machine:** IDLE → SENDING → SPACING → IDLE
- `keyerActive`: Currently sending an element (dit or dah)
- `inSpacing`: In the inter-element gap
- `sendingDit` / `sendingDah`: Which element is being sent
- `ditMemory` / `dahMemory`: Memory paddles for squeeze keying

**Key Features:**
- Non-blocking state machine (checked every loop iteration)
- Proper iambic A/B behavior with memory paddles
- Accurate WPM timing from device settings (`cwSpeed`)
- Uses `startTone()` / `continueTone()` / `stopTone()` for glitch-free audio
- Screen completely freezes during keying to prevent audio interference

### Game Loop Architecture

**Dual Update System:**
1. **`updateMorseShooterInput(tft)`** - Called every main loop iteration
   - Runs iambic keyer state machine
   - Handles pattern timeout detection
   - Screen freezes if any keying activity detected

2. **`updateMorseShooterVisuals(tft)`** - Called every main loop iteration
   - Checks if keying is active (paddles, tone, gap, or pattern exists)
   - If keying: returns immediately (screen frozen)
   - If idle: updates game physics once per second (GAME_UPDATE_INTERVAL)
   - Updates falling letters, spawns new letters, redraws HUD

**Critical Design Decision:** Screen updates are **completely blocked** during any keying activity to ensure smooth, glitch-free audio at the configured WPM speed.

### Pattern Matching and Shooting

**Pattern Completion:**
- Pattern builds as user keys morse code (e.g., ".-" for A)
- After last element, user releases paddles
- System waits GAME_LETTER_TIMEOUT (1200ms) for inactivity
- Pattern is matched against morse code table
- If match found, searches for falling letter with that character
- If found: shoots letter, plays laser/explosion, updates score
- If no match or wrong code: error beep, pattern cleared

**Collision Avoidance:**
When spawning new letters, the system checks existing letters and avoids placing new letters within 30 pixels horizontally and 40 pixels vertically of existing letters (up to 20 attempts).

### Visual Effects

**Ground Scenery (GROUND_Y = 225):**
- Houses with roofs (simple rectangles and triangles)
- Trees (triangles for foliage, rectangles for trunks)
- Turret at center bottom (tank-like shape with barrel)
- All drawn with retro arcade color palette

**Shooting Animations:**
1. Laser shot: Lines from turret to target (cyan/white)
2. Beep 1200 Hz for 50ms
3. Explosion: Concentric circles with radiating rays (yellow/red/white)
4. Beep 1000 Hz for 100ms
5. Clear play area and redraw all elements

**Cleanup Sequence (Critical for No Ghosting):**
```cpp
fallingLetters[j].active = false;  // Mark inactive FIRST
drawLaserShot();                    // Visual effects
drawExplosion();
tft.fillRect(0, 42, SCREEN_WIDTH, GROUND_Y - 42, COLOR_BACKGROUND);  // Clear
drawGroundScenery(tft);            // Redraw ground
drawFallingLetters(tft);           // Redraw active letters only
```

**Why order matters:** Letter must be marked inactive BEFORE redraw, or it will briefly reappear as a "ghost" after being shot.

### HUD Display

**Top Left Corner:**
- Score: Current points (10 points per letter)
- Lives: Remaining lives (red if ≤2, green otherwise)

**Bottom (Above Ground):**
- Current morse pattern being entered (cyan text, size 2)
- Cleared when pattern is empty

### Game Over and Restart

**Game Over Triggers:**
- Lives reach 0 (letters hit ground)

**Game Over Screen:**
- Large "GAME OVER" text (red)
- Final score
- High score (persisted across sessions)
- Instructions: ENTER to play again, ESC to exit

### Main Loop Integration

The game mode is integrated into the main loop with special handling:

```cpp
if (currentMode == MODE_MORSE_SHOOTER) {
  updateMorseShooterInput(tft);   // Every loop - keyer logic
  updateMorseShooterVisuals(tft); // Every loop - but internally rate-limited
}
```

Keyboard polling is slowed to 50ms during game mode (same as practice) to prioritize audio quality over keyboard responsiveness.

### Important Implementation Notes

1. **Audio Priority:** Screen updates are completely disabled during keying. Game visuals pause while user sends morse code, then resume smoothly.

2. **Timing Accuracy:** Uses `MorseTiming` class with device's `cwSpeed` setting for precise dit/dah/gap durations matching practice mode.

3. **Phase Continuity:** Uses `continueTone()` during element sending to maintain audio phase and prevent clicks.

4. **State Tracking:** Uses static variable `wasKeyingLastTime` to detect transition from keying to idle, ensuring pattern timeout is measured from when paddles are released (not constantly reset).

5. **Memory Paddles:** Full iambic keyer support - squeeze both paddles for alternating dits/dahs, or press opposite paddle during element for queued sending.

### Future Enhancements

Potential improvements documented in `MORSE_SHOOTER_README.md`:
- Difficulty levels (faster falling, more letters)
- Power-ups and bonuses
- Multiple letter types (different colors/values)
- Boss battles (send longer phrases)
- Leaderboard persistence

## Radio Mode

### Overview
The Radio Mode provides integration with external ham radios via 3.5mm jack outputs. It allows keying a connected radio using the Summit's paddle inputs (physical or capacitive touch) with two distinct operating modes.

### Architecture: `radio_output.h`

**Radio Mode Types:**
```cpp
enum RadioMode {
  RADIO_MODE_SUMMIT_KEYER,   // Summit does the keying logic, outputs straight key format
  RADIO_MODE_RADIO_KEYER     // Passthrough dit/dah contacts to radio's internal keyer
};
```

**Output Pins:**
- **DIT output**: GPIO 18 (A0) - `RADIO_KEY_DIT_PIN`
- **DAH output**: GPIO 17 (A1) - `RADIO_KEY_DAH_PIN`
- **Format**: 3.5mm TRS jack (Tip = Dit, Ring = Dah, Sleeve = GND)
- **Logic**: Active HIGH (pin goes HIGH when keying)

**Hardware Interface:**
- Resistor + transistor driver circuit on both output lines (same design as standard Vail adapters)
- Transistors pull radio keying inputs to ground when activated
- Tested and working with external radios
- **CRITICAL**: Direct GPIO-to-radio connection may damage equipment - always use driver circuit

### Summit Keyer Mode

In Summit Keyer mode, the device performs all keying logic internally and outputs a straight key format signal:

**Straight Key:**
- DIT pin outputs key-down/key-up timing
- DAH pin remains LOW
- Timing follows physical paddle presses directly

**Iambic (A or B):**
- Summit's iambic keyer generates dit/dah elements with proper timing
- DIT pin outputs composite keyed signal (straight key format)
- DAH pin remains LOW
- Timing based on configured WPM speed (`cwSpeed`)
- Full memory paddle support (squeeze keying)

**No Audio Output:**
- Radio output mode does not play sidetone through Summit's speaker
- External radio provides sidetone

### Radio Keyer Mode

In Radio Keyer mode, the device passes paddle contacts directly to the radio, letting the radio's internal keyer handle timing:

**Straight Key:**
- DIT pin mirrors DIT paddle state (HIGH when pressed)
- DAH pin remains LOW

**Iambic:**
- DIT pin mirrors DIT paddle state
- DAH pin mirrors DAH paddle state
- Radio's internal keyer interprets squeeze keying and timing
- Radio's WPM setting controls speed (Summit's WPM setting ignored)

### Radio Output UI

The Radio Output screen displays three configurable settings:

1. **Speed (WPM)**: 5-40 WPM
   - Only affects Summit Keyer mode
   - Ignored in Radio Keyer mode (radio controls speed)
   - Shares global `cwSpeed` setting with Practice mode

2. **Key Type**: Straight / Iambic A / Iambic B
   - Determines paddle input interpretation
   - Affects both Summit Keyer and Radio Keyer modes
   - Shares global `cwKeyType` setting

3. **Radio Mode**: Summit Keyer / Radio Keyer
   - Toggles between internal and external keying logic
   - Persisted in Preferences namespace "radio"

**Navigation:**
- UP/DOWN: Select setting
- LEFT/RIGHT: Adjust value
- ESC: Exit to Radio menu

### Input Sources

Radio Output accepts input from three sources simultaneously (OR logic):

1. **Physical Paddle** (GPIO 6 for DIT, GPIO 9 for DAH)
2. **Capacitive Touch** (GPIO 8 for DIT, GPIO 5 for DAH)
3. Both checked every loop iteration for responsive keying

### Main Loop Integration

**Update Function:**
```cpp
if (currentMode == MODE_RADIO_OUTPUT) {
  updateRadioOutput();  // Called every loop iteration (1ms delay)
}
```

**Performance Considerations:**
- I2C keyboard polling slowed to 50ms (same as Practice mode)
- Status icon updates disabled during Radio Output mode
- No display updates during keying to maximize timing accuracy

### State Management

**Preferences:**
- Namespace: "radio"
- Stored value: `radioMode` (int)
- Loaded on startup via `loadRadioSettings()`
- Saved immediately when changed

**Keyer State (Summit Keyer mode only):**
- `radioKeyerActive`: Currently sending element
- `radioInSpacing`: In inter-element gap
- `radioDitMemory` / `radioDahMemory`: Memory paddles for squeeze keying
- `radioDitDuration`: Calculated from `cwSpeed` using PARIS method

### CW Memories (Placeholder)

The **CW Memories** menu option is currently a placeholder for future implementation:

**Planned Features:**
- Store up to 8-10 CW message memories
- Playback stored messages via radio output
- Common contest exchanges, CQ calls, 73, etc.
- Integration with both Summit Keyer and Radio Keyer modes

**Current Implementation:**
- Displays "Coming Soon..." placeholder screen
- ESC returns to Radio menu
- No storage or playback functionality yet

### Use Cases

**Contest Operation:**
- Summit Keyer mode for consistent sending at configured speed
- Memory messages for exchanges (future)
- Physical paddle for flexibility

**Casual QSOs:**
- Radio Keyer mode to use radio's built-in keyer settings
- Capacitive touch pads for portable operation
- Radio provides sidetone and QSK

**Training Aid:**
- Practice sending at various speeds
- Compare Summit Keyer vs. Radio Keyer behavior
- Use with actual radio for on-air confidence

### Implementation Notes

1. **No Tone Output:** The `updateRadioOutput()` function does not call `startTone()` or `beep()` - the external radio provides audio sidetone.

2. **Timing Priority:** Main loop delay reduced to 1ms during Radio Output mode for precise timing.

3. **Output Safety:** GPIO pins configured as OUTPUT with initial LOW state. Always use external driver circuitry.

4. **Settings Sharing:** Speed and Key Type are shared with Practice mode via global `cwSpeed` and `cwKeyType` variables. Changes in Radio Output affect Practice mode and vice versa.

5. **State Cleanup:** When exiting Radio Output mode, both output pins are set LOW to ensure radio is not left keyed.

## Common Development Patterns

### Adding a New Menu Mode
1. Add enum value to `MenuMode` in morse_trainer_menu.ino
2. Create header file (e.g., `training_newmode.h`) with state variables, UI functions, and input handler
3. Add mode to menu arrays (options and icons)
4. Update `selectMenuItem()` to handle selection
5. Update `handleKeyPress()` to route input to your handler
6. Update `drawMenu()` to call your UI renderer
7. Add any Preferences namespaces for persistent settings

### Creating Audio Feedback
```cpp
beep(frequency_hz, duration_ms);  // For short beeps/tones
startTone(frequency_hz);          // For continuous tone (manual stop)
stopTone();                       // Stop continuous tone
```

Always use these functions instead of direct I2S manipulation. They handle volume scaling and phase continuity.

### Display Optimization
- Only redraw what changes (use return codes from input handlers)
- Disable all display updates during audio-critical operations (practice mode, morse playback)
- Use `fillRect()` to clear regions before redrawing text
- Cache text bounds with `getTextBounds()` for centering

### Morse Code Generation
```cpp
#include "morse_code.h"
const char* pattern = getMorseCode('A');  // Returns ".-"
MorseTiming timing(20);  // 20 WPM timing calculator
// timing.ditDuration, timing.dahDuration, timing.elementGap, etc.
```

Use `playMorseString()` in morse_code.h for automatic playback of strings with proper spacing.

## Critical Constraints

1. **Never use `analogRead(A3)` or `analogRead(15)`** - it breaks I2S audio completely
2. **Initialize I2S before display** - I2S needs higher DMA priority
3. **No display updates during audio playback** in practice/training modes - causes glitches
4. **Always use `beep()` or I2S functions for audio** - never manipulate GPIO 5 directly (repurposed pin)
5. **Load Preferences at startup, save immediately on change** - don't batch writes
6. **WebSocket handling must be non-blocking** - use state machine pattern in update loop

## Troubleshooting Common Issues

**Audio distortion/clicking:** Check that I2S is initialized before display. Verify no display updates during audio playback. Ensure volume isn't clipping (keep ≤90%).

**WiFi connection fails:** Use Settings → WiFi Setup to scan and save credentials. Check serial monitor for SSID/password echo and connection status.

**Battery percentage inaccurate:** MAX17048 requires calibration over several charge cycles. Initial readings may be lower than actual charge.

**Keys not responding:** Check serial monitor for I2C scan results. Verify CardKB at 0x5F, battery monitor at 0x36.

**Display freezes:** Usually caused by blocking code in main loop. Move long operations to separate update functions or use state machines.

## QSO Logger

### Overview
The QSO Logger provides comprehensive contact logging functionality both on-device and via web interface. It stores logs in SPIFFS as JSON files organized by date, with support for ADIF and CSV export.

### Architecture: Multi-Module Design

**Module Structure:**
- **`qso_logger.h`** - Core data structures (QSO struct with 30+ fields)
- **`qso_logger_storage.h`** - SPIFFS file operations, JSON serialization, metadata management
- **`qso_logger_input.h`** - Device-side input forms with field navigation
- **`qso_logger_validation.h`** - Validation functions for callsigns, frequencies, RST, grid squares

### QSO Data Structure

The `QSO` struct contains all ADIF-compatible fields:

**Required fields:**
- `callsign` (char[11]) - Must be 3-10 alphanumeric characters with at least one digit
- `frequency` (float) - Must be 1.8-1300 MHz
- `mode` (char[10]) - CW, SSB, FM, AM, FT8, FT4, RTTY, PSK31
- `band` (char[6]) - Auto-calculated from frequency
- `date` (char[9]) - YYYYMMDD format
- `time_on` (char[7]) - HHMM format

**Optional fields:**
- RST sent/received, name, QTH, power, grid squares (my/their)
- POTA references (my/their), IOTA, country, state
- Contest info (name, serial sent/received)
- Operator/station callsigns
- Notes (char[101])

### Storage Architecture

**File Organization:**
```
/logs/
  qso_20251028.json    # Today's logs
  qso_20251027.json    # Yesterday's logs
  metadata.json        # Statistics cache
```

**Log File Format:**
```json
{
  "date": "20251028",
  "count": 3,
  "logs": [
    {
      "id": 1730154000,
      "callsign": "W1ABC",
      "frequency": 14.025,
      "mode": "CW",
      "band": "20m",
      "rst_sent": "599",
      "rst_rcvd": "599",
      "date": "20251028",
      "time_on": "1430",
      "gridsquare": "FN31pr",
      "my_gridsquare": "EN82xx",
      "my_pota_ref": "US-2256",
      "their_pota_ref": "",
      "notes": "Nice QSO"
    }
  ]
}
```

**Key Design Decisions:**
- **One file per date** - Simplifies daily log management
- **JSON format** - Human-readable, easy to parse
- **Unique IDs** - Unix timestamp (milliseconds) prevents duplicates
- **ADIF-compatible fields** - Direct mapping to ADIF export

### Device-Side Logging (qso_logger_input.h)

**Field Navigation:**
- 10 input fields arranged in logical order
- UP/DOWN arrows navigate between fields
- LEFT/RIGHT arrows or alphanumeric input for text fields
- ENTER saves QSO after validation
- ESC cancels and returns to menu

**Field Validation:**
- **Callsign**: 3-10 chars, alphanumeric, must contain digit
- **Frequency**: 1.8-1300 MHz, auto-calculates band
- **Mode**: Select from predefined list
- **RST**: Optional, defaults to 599/59
- **Grid squares**: 4-8 character Maidenhead locator
- **POTA**: Format US-NNNN

**Auto-Completion:**
- Date/time auto-filled from RTC
- Band auto-calculated from frequency
- RST defaults to 599 for CW, 59 for phone

### Validation Functions (qso_logger_validation.h)

**`validateCallsign(callsign)`:**
- Length: 3-10 characters
- Pattern: Alphanumeric only
- Requirement: At least one digit
- Examples: W1ABC ✓, ABC ✗ (no digit), W ✗ (too short)

**`validateFrequency(freq)`:**
- Range: 1.8 - 1300.0 MHz
- Covers: 160m through 23cm bands

**`validateRST(rst)`:**
- CW format: 3 digits (e.g., "599")
- Phone format: 2 digits (e.g., "59")
- Digits in valid range (R: 1-5, S: 1-9, T: 1-9)

**`validateGridSquare(grid)`:**
- Length: 4, 6, or 8 characters
- Format: AA##aa## (e.g., "FN31pr")
- Field/square/subsquare precision

**`frequencyToBand(freq)`:**
- Maps frequency to band string
- Examples: 14.025 → "20m", 7.125 → "40m"
- Supports HF, VHF, UHF, microwave bands

### Storage Operations (qso_logger_storage.h)

**`saveQSO(qso)`:**
1. Determine filename from date: `/logs/qso_YYYYMMDD.json`
2. Load existing logs for that date (if any)
3. Append new QSO to logs array
4. Serialize back to JSON with `qsoToJson()`
5. Write atomically to file
6. **Critical**: Always initialize QSO struct with `memset(&qso, 0, sizeof(QSO))` before populating to avoid garbage data

**`qsoToJson(qso, jsonObject)`:**
- Converts QSO struct to JSON object
- Only includes non-empty fields (space-efficient)
- ArduinoJson handles string escaping automatically
- Serial debug logging for POTA/location fields

**`loadQSOsForDate(date)`:**
- Reads specific date's log file
- Returns array of QSO objects
- Used by device log viewer

**Metadata Management:**
- `loadMetadata()` - Load stats on startup
- `saveMetadata()` - Persist stats to file
- Tracks: total logs, logs by band/mode, oldest/newest IDs

### Device Menu Integration

**QSO Logger Menu (MODE_QSO_LOGGER):**
1. **Log QSO** - Opens input form
2. **View Logs** - Browse by date
3. **Delete Logs** - Remove specific QSO or entire date
4. **Export** - Removed; use web interface

**Log Viewer Features:**
- Navigate through QSOs with UP/DOWN
- Shows: callsign, freq/band, mode, RST, time
- Scrollable details view
- Delete individual QSOs

## Web Server

### Overview
The web server provides a comprehensive browser-based interface for QSO management, device settings, and system monitoring. It auto-starts when WiFi connects and is accessible via mDNS or IP address.

### Architecture: AsyncWebServer

**Technology Stack:**
- **ESPAsyncWebServer** - Non-blocking HTTP server
- **mDNS** - Accessible at `http://vail-summit.local`
- **WiFi Event Handlers** - Auto-start/stop on connect/disconnect
- **PROGMEM HTML** - Stores web pages in flash to save RAM
- **RESTful API** - JSON endpoints for CRUD operations

**Server Lifecycle:**
```cpp
WiFi connects → WiFi event fires → setupWebServer() → mDNS starts → Server begins
WiFi disconnects → stopWebServer() → mDNS ends → Server stops
```

### Dashboard Page (/)

**Status Cards:**
- Battery voltage and percentage (from MAX17048/LC709203F)
- WiFi connection status and signal strength
- Total QSO count
- Real-time updates every 10 seconds via `/api/status`

**Navigation Cards:**
- **QSO Logger** - View, manage, export logs
- **Device Settings** - CW speed, tone, volume, callsign
- **WiFi Setup** - Network configuration
- **System Info** - Firmware, memory, storage stats
- **Radio Control** - Remote morse code transmission via radio output

### Enhanced QSO Logger Web Interface

**File: `web_logger_enhanced.h`** - Complete HTML/CSS/JavaScript in PROGMEM

**Features:**

1. **Station Settings** (Header Badges)
   - Displays callsign and grid square
   - Click to open modal for editing
   - Saved to Preferences namespace "qso_operator"
   - Auto-populated in new QSOs (my_gridsquare, my_pota_ref)

2. **QSO Table View**
   - Sortable columns: Date/Time, Callsign, Freq/Band, Mode, RST, Grids, POTA
   - Real-time search/filter across all fields
   - Edit and Delete buttons for each QSO
   - Responsive design (horizontal scroll on mobile)

3. **Statistics Cards**
   - Total QSOs (all time)
   - Today's QSO count
   - Unique callsigns worked

4. **New QSO Modal**
   - Full input form with validation
   - Required fields: Callsign*, Frequency*, Mode*
   - Optional: RST sent/rcvd, their grid, their POTA, notes
   - Auto-uppercases callsigns and grid squares
   - Auto-trims whitespace

5. **Edit QSO Modal**
   - Pre-filled with existing QSO data
   - Same validation as new QSO
   - Updates in place

6. **Map Visualization**
   - Leaflet.js integration (CDN-loaded)
   - Shows today's QSOs with grid squares
   - Markers at grid square center coordinates
   - Click marker for callsign/grid popup
   - Grid-to-lat/lon conversion using Maidenhead algorithm

7. **Export Functions**
   - ADIF download (`.adi` file)
   - CSV download (`.csv` file)

### Web Server API Endpoints (web_server.h)

**Status & Data:**
- `GET /api/status` - Device status (battery, WiFi, QSO count)
- `GET /api/qsos` - All QSO logs as JSON array
- `GET /api/export/adif` - ADIF 3.1.4 formatted file
- `GET /api/export/csv` - CSV formatted file

**Station Settings:**
- `GET /api/settings/station` - Load callsign, grid, POTA from Preferences
- `POST /api/settings/station` - Save station settings
  - Body: `{"callsign": "W1ABC", "gridsquare": "FN31", "pota": "US-2256"}`

**QSO CRUD Operations:**
- `POST /api/qsos/create` - Create new QSO
  - Body: JSON with callsign, frequency, mode (required) + optional fields
  - Auto-calculates band from frequency via `frequencyToBand()`
  - **Critical**: Must initialize QSO struct with `memset(&qso, 0, sizeof(QSO))` to prevent JSON parsing errors from garbage data
  - Auto-fills date/time if not provided

- `POST /api/qsos/update` - Update existing QSO
  - Body: JSON with date, id (required) + updated fields
  - Loads day's log file, finds QSO by ID, updates fields, saves back
  - Recalculates band if frequency changes

- `DELETE /api/qsos/delete?date=YYYYMMDD&id=1234567890` - Delete QSO
  - Finds and removes QSO from log file
  - Updates count in file
  - Deletes file if no QSOs remain

### Form Validation (Client-Side)

**HTML5 Attributes:**
```html
<input type="text" id="qsoCallsign" required
       minlength="3" maxlength="10"
       pattern="[A-Za-z0-9]+"
       placeholder="W1ABC"
       title="3-10 alphanumeric characters with at least one digit">

<input type="number" id="qsoFrequency" required
       min="1.8" max="1300" step="0.001"
       placeholder="14.025"
       title="Frequency between 1.8 and 1300 MHz">
```

**JavaScript Validation:**
```javascript
function validateCallsign(callsign) {
  if (callsign.length < 3 || callsign.length > 10) return 'Length error';
  if (!/^[A-Za-z0-9]+$/.test(callsign)) return 'Alphanumeric only';
  if (!/\d/.test(callsign)) return 'Must contain at least one digit';
  return null;
}

function validateFrequency(freq) {
  if (freq < 1.8 || freq > 1300) return 'Range error';
  return null;
}
```

**Data Normalization:**
- Callsigns auto-uppercased
- Grid squares auto-uppercased
- Whitespace trimmed from all fields
- Default RST values (599) if empty

### ADIF Export Format

**Header:**
```
ADIF Export from VAIL SUMMIT
<PROGRAMID:11>VAIL SUMMIT
<PROGRAMVERSION:5>1.0.0
<ADIF_VER:5>3.1.4
<EOH>
```

**QSO Records:**
```
<CALL:5>W1ABC <FREQ:6>14.025 <MODE:2>CW <QSO_DATE:8>20251028
<TIME_ON:6>143000 <RST_SENT:3>599 <RST_RCVD:3>599
<GRIDSQUARE:6>FN31pr <MY_GRIDSQUARE:6>EN82xx
<MY_SIG:4>POTA <MY_SIG_INFO:7>US-2256 <EOR>
```

**POTA Support:**
- Uses ADIF 3.1.4 special event tags
- `<MY_SIG:4>POTA <MY_SIG_INFO:7>US-2256` - Operator's POTA activation
- `<SIG:4>POTA <SIG_INFO:7>US-2254` - Contact's POTA activation
- Compatible with QRZ, LoTW, POTA upload

### Map Functionality

**Grid Square to Lat/Lon Conversion:**
```javascript
function gridToLatLon(grid) {
  // Example: "FN31pr" → [41.5, -73.0]
  const lon = (grid.charCodeAt(0) - 65) * 20 - 180
            + (grid.charCodeAt(2) - 48) * 2 + 1;
  const lat = (grid.charCodeAt(1) - 65) * 10 - 90
            + (grid.charCodeAt(3) - 48) + 0.5;
  return [lat, lon];
}
```

**Today's QSO Filter:**
```javascript
const today = new Date().toISOString().split('T')[0].replace(/-/g, '');
const todayQSOs = allQSOs.filter(q => q.date === today && q.gridsquare);
```

**Marker Management:**
- Clears existing markers before redrawing
- Only shows QSOs with valid grid squares (4+ characters)
- Click marker to see callsign and grid
- Map auto-centers on USA (lat: 39.8283, lon: -98.5795, zoom: 4)

### Device Settings Page (/settings)

**Purpose:** Configure device parameters via web interface

**Features:**

1. **CW Settings Card**
   - Speed slider (5-40 WPM) with live display
   - Tone frequency slider (400-1200 Hz, 50 Hz steps) with live display
   - Key type dropdown (Straight, Iambic A, Iambic B)
   - Save button with validation and feedback

2. **Audio Settings Card**
   - Volume slider (0-100%) with live display
   - Save button

3. **Station Settings Card**
   - Callsign text input (max 10 characters)
   - Auto-uppercases input
   - Save button

**API Endpoints:**
- `GET /api/settings/cw` - Returns `{wpm, tone, keyType}`
- `POST /api/settings/cw` - Updates CW settings with validation (5-40 WPM, 400-1200 Hz, keyType 0-2)
- `GET /api/settings/volume` - Returns `{volume}`
- `POST /api/settings/volume` - Updates volume (0-100 validation)
- `GET /api/settings/callsign` - Returns `{callsign}`
- `POST /api/settings/callsign` - Updates callsign (length validation, auto-uppercase)

**Implementation:**
- Settings load automatically on page load
- Real-time slider updates show current values
- Validation on save (range checking for all numeric inputs)
- Success/error messages with 5-second auto-dismiss
- Saves to ESP32 Preferences immediately (namespaces: "cw", "audio", "callsign")
- Updates global variables (`cwSpeed`, `cwTone`, `cwKeyType`, `vailCallsign`)
- Calls existing save functions (`saveCWSettings()`, `setVolume()`, `saveCallsign()`)

### System Info Page (/system)

**Purpose:** Display comprehensive device diagnostics and status

**Information Cards:**

1. **Firmware Card**
   - Version (e.g., "0.1")
   - Build Date (e.g., "2025-10-30")

2. **System Card**
   - Uptime (formatted as days/hours/minutes/seconds)
   - CPU Speed (MHz)
   - Flash Size (formatted as MB)

3. **Memory Card**
   - Free RAM (formatted as KB/MB)
   - Min Free RAM (lowest point since boot)
   - Free PSRAM (formatted as KB/MB, or "N/A" if no PSRAM)
   - Min Free PSRAM (lowest point since boot)

4. **Storage Card**
   - SPIFFS Used (formatted as KB/MB)
   - SPIFFS Total (formatted as KB/MB)
   - QSO Logs (count from `storageStats.totalLogs`)

5. **WiFi Card**
   - Status (Connected/Disconnected)
   - SSID (network name)
   - IP Address
   - Signal Strength (RSSI in dBm, color-coded: green >-60, yellow -60 to -70, red <-70)

6. **Battery Card**
   - Voltage (V, 2 decimal places)
   - Charge (percentage)
   - Monitor (MAX17048, LC709203F, or None)

**API Endpoint:**
- `GET /api/system/info` - Returns comprehensive JSON with all diagnostic data

**Features:**
- Auto-refresh every 10 seconds
- Smart formatting (bytes → KB/MB, uptime → human-readable)
- WiFi signal color coding for quick status assessment
- Last update timestamp displayed at bottom
- Graceful fallbacks for missing data (N/A, 0, Unknown)

### Radio Control Page (/radio)

**Purpose:** Remote morse code transmission via radio output jack

**Features:**

1. **Radio Mode Status Card**
   - Shows whether Radio Output mode is active/inactive (polls every 5 seconds)
   - "Enter Radio Mode" button to switch device to MODE_RADIO_OUTPUT
   - Status badge updates in real-time

2. **Transmission Settings Card**
   - WPM speed slider (5-40 WPM) with live display
   - Adjusts device's global `cwSpeed` setting
   - Saved to Preferences immediately on change

3. **Send Morse Code Message Card**
   - Text area for message input (max 200 characters)
   - Character counter
   - Send button queues message for transmission
   - Info box explains radio output behavior (no sidetone, uses device settings)

**API Endpoints:**
- `GET /api/radio/status` - Returns `{active, mode}` (checks if `currentMode == MODE_RADIO_OUTPUT`)
- `POST /api/radio/enter` - Switches device to Radio Output mode, calls `startRadioOutput(tft)`
- `POST /api/radio/send` - Queues message via `queueRadioMessage()`, returns success or "queue full" error
- `GET /api/radio/wpm` - Returns current WPM speed
- `POST /api/radio/wpm` - Updates WPM speed (5-40 validation), calls `saveCWSettings()`

**Message Queue System (radio_output.h):**
- Circular buffer holds up to 5 messages (200 characters each)
- `processRadioMessageQueue()` runs in main loop via `updateRadioOutput()`
- Waits if user is manually keying (doesn't interrupt)
- Sends messages character-by-character with proper WPM timing
- Keys radio via GPIO 18 (DIT) and GPIO 17 (DAH) pins
- Uses blocking morse character playback with accurate letter/word spacing
- Debug logging shows character timing and queue status

**User Workflow:**
1. User opens `/radio` in browser
2. Clicks "Enter Radio Mode" to switch device
3. Types message in text area
4. Clicks "Send Message" - message is queued on device
5. Device automatically transmits as morse code via 3.5mm radio output
6. Can queue up to 5 messages
7. Messages transmit sequentially without interrupting manual keying

### Access Methods

**mDNS (Recommended):**
```
http://vail-summit.local/
```

**Direct IP:**
```
http://192.168.1.xxx/
```
(Check serial monitor for IP address on WiFi connect)

**Browser Compatibility:**
- Desktop: Chrome, Firefox, Safari, Edge
- Mobile: iOS Safari, Chrome Android
- Requires JavaScript enabled

### Common Development Patterns

**Adding a New API Endpoint:**
```cpp
webServer.on("/api/myendpoint", HTTP_GET, [](AsyncWebServerRequest *request) {
  JsonDocument doc;
  doc["data"] = "value";
  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
});
```

**Adding a POST Endpoint with JSON Body:**
```cpp
webServer.on("/api/myendpoint", HTTP_POST,
  [](AsyncWebServerRequest *request) {},
  NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    JsonDocument doc;
    deserializeJson(doc, data, len);
    // Process doc...
    request->send(200, "application/json", "{\"success\":true}");
  });
```

### Critical Implementation Notes

1. **Always initialize QSO structs:** `memset(&qso, 0, sizeof(QSO))` before populating fields to prevent garbage data causing JSON parsing errors

2. **Band auto-calculation:** Web interface doesn't send band field; server calculates it via `frequencyToBand(frequency)`

3. **Map marker updates:** Call `updateMapMarkers()` after loading QSOs to refresh map pins

4. **File operations:** All file writes are atomic (read→modify→write) to prevent corruption

5. **Error handling:** Web API returns JSON error objects: `{"success":false,"error":"message"}`

### Troubleshooting Web Interface

**"Failed to load QSOs: SyntaxError: Bad control character":**
- Cause: QSO struct not initialized before populating
- Fix: Add `memset(&newQSO, 0, sizeof(QSO))` before field assignments
- Location: `web_server.h` create/update endpoints

**Map shows no pins:**
- Check browser console for "QSOs with grids today" count
- Verify QSOs have valid grid squares (4+ characters)
- Ensure date format matches (YYYYMMDD)
- Call `updateMapMarkers()` after QSO changes

**Station settings not saving:**
- Check Preferences namespace: "qso_operator"
- Verify WiFi connected (required for web access)
- Check serial monitor for save confirmation

**Cannot access web server:**
- Verify WiFi connected (check device WiFi status icon)
- Try direct IP if mDNS fails: `http://192.168.1.xxx/`
- Check serial monitor for "Web server started" message
- Ensure port 80 not blocked by firewall
