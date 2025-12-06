# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## IMPORTANT: DO NOT COMPILE

**NEVER** run compilation commands (arduino-cli compile, etc.) for this project. The user will always handle compilation themselves. Do not check for compilation or build the project under any circumstances.

## Project Overview

VAIL SUMMIT is a portable morse code training device built on the ESP32-S3 Feather platform with an LCD display and modern UI. It's designed for ham radio operators to practice receiving and sending morse code. Input comes from a CardKB I2C keyboard, iambic paddle, and capacitive touch pads. The device includes training modes, settings management, WiFi connectivity to the Vail internet morse repeater, and extensive hardware integration.

## Documentation Structure

This project uses modular documentation. For detailed information on specific topics, see:

- **[docs/BUILDING.md](docs/BUILDING.md)** - Build setup, compilation, upload, and firmware updates
- **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** - System architecture, state machine, main loop, and audio system
- **[docs/HARDWARE.md](docs/HARDWARE.md)** - Hardware interfaces, pin assignments, I2C devices
- **[docs/FEATURES.md](docs/FEATURES.md)** - Feature-specific documentation (CW Academy, Morse Shooter, Radio Mode, Decoder)
- **[docs/WEB_INTERFACE.md](docs/WEB_INTERFACE.md)** - Web server, QSO logger, REST API, and web pages
- **[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)** - Development patterns, critical constraints, troubleshooting

## Quick Reference

### Building and Development

**Arduino IDE Setup:**
- Board: **ESP32S3 Dev Module** or **Adafruit Feather ESP32-S3**
- USB CDC On Boot: **Enabled**
- Flash Size: **4MB**
- PSRAM: **OPI PSRAM**
- Upload Speed: **921600**

**Required Libraries** (Install via Arduino Library Manager):
1. LovyanGFX by lovyan03 (display graphics for ST7796S)
2. Adafruit MAX1704X (battery monitoring)
3. Adafruit LC709203F (backup battery monitor support)
4. WebSockets by Markus Sattler
5. ArduinoJson by Benoit Blanchon
6. ESPAsyncWebServer (install from GitHub)

**Arduino ESP32 Core:** Version 2.0.14 (required for ST7796S display compatibility)

**Compilation:**
```bash
# Arduino IDE
arduino morse_trainer/morse_trainer_menu.ino

# Arduino CLI
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32s3 morse_trainer/
arduino-cli upload -p COM<X> --fqbn esp32:esp32:adafruit_feather_esp32s3 morse_trainer/
```

For detailed build instructions, firmware updates, and GitHub Actions workflow, see **[docs/BUILDING.md](docs/BUILDING.md)**.

### Architecture Overview

The system operates as a **mode-based state machine**:
- Each mode has its own input handler and UI renderer
- Main loop delegates to the appropriate mode handler based on `currentMode`
- Modular header files isolate each feature (training, games, radio, settings, web)

**Key modes:**
- Training: `MODE_HEAR_IT_TYPE_IT`, `MODE_PRACTICE`, `MODE_CW_ACADEMY_*`
- Games: `MODE_MORSE_SHOOTER`, `MODE_MORSE_MEMORY`
- Radio: `MODE_RADIO_OUTPUT`, `MODE_CW_MEMORIES`
- Settings: `MODE_WIFI_SETTINGS`, `MODE_CW_SETTINGS`, `MODE_VOLUME_SETTINGS`
- Network: `MODE_VAIL_REPEATER`, `MODE_WEB_PRACTICE`, `MODE_WEB_MEMORY_CHAIN`, `MODE_WEB_HEAR_IT`, QSO Logger via web interface

For detailed architecture information, see **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)**.

### Hardware Interfaces

**Display:** ST7796S 4.0" 480×320 LCD (landscape orientation) - uses LovyanGFX library
**SD Card:** Integrated on display board (CS=38, shares SPI with display, FAT32 format required)
**Keyboard:** CardKB I2C (address 0x5F)
**Paddle Input:** GPIO 6 (DIT), GPIO 9 (DAH)
**Capacitive Touch:** GPIO 8 (DIT), GPIO 5 (DAH)
**Radio Output:** GPIO 18 (DIT), GPIO 17 (DAH)
**I2S Audio:** MAX98357A amplifier (BCK=14, LRC=15, DIN=16)
**Battery Monitor:** MAX17048 or LC709203F (I2C auto-detected)
**Display SPI:** MOSI=35, SCK=36, MISO=37, CS=10, RST=11, DC=12
**SD Card SPI:** MOSI=35, SCK=36, MISO=37, CS=38 (shares bus with display)

For complete pin assignments and hardware details, see **[docs/HARDWARE.md](docs/HARDWARE.md)**.

### Menu Navigation

The device uses a card-based menu UI with keyboard navigation:

**Navigation Keys:**
- **Up/Down arrows** - Scroll through menu cards
- **Right arrow** - Select highlighted menu item (enter submenu/mode)
- **Enter** - Select highlighted menu item (same as Right arrow)
- **ESC** - Go back to parent menu / Exit current mode
- **ESC (triple press)** - Enter deep sleep mode from main menu

Each menu card displays an icon, title, and right arrow (→) visual indicator. The right arrow key input matches the visual arrow shown on cards.

For detailed menu architecture, see **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#menu-navigation)**.

### Configuration Management

All settings stored in ESP32 Preferences (non-volatile flash):

- **"wifi"** - Up to 3 WiFi network credentials
- **"cwsettings"** - WPM speed, tone frequency, key type
- **"audio"** - Volume percentage
- **"callsign"** - User callsign for Vail repeater
- **"webpw"** - Web interface password (optional, HTTP Basic Auth)
- **"cwa"** - CW Academy progress (track, session, practice/message types)
- **"radio"** - Radio mode (Summit Keyer vs Radio Keyer)
- **"cw_memories"** - CW message presets (10 slots, label + message)
- **"qso_operator"** - Station info for QSO logging
- **"memory"** - Memory Chain game settings (difficulty, mode, speed, sound, hints, high score)
- **"hear_it"** - Hear It Type It training settings (mode, group length, custom characters)

Settings loaded on startup, saved immediately when changed.

### WiFi Configuration

**Station Mode:** Connects to existing WiFi networks (default)
**Access Point Mode:** Creates own WiFi network (`VAIL-SUMMIT-XXXXXX`, password: `vailsummit`)

To enter AP mode: Settings → WiFi Setup → Press 'A' key

**Web Server Access:**
- mDNS: `http://vail-summit.local/` (Station mode only)
- Direct IP: `http://192.168.4.1/` (AP mode) or device IP (Station mode)

**Web Server Security:**
- Optional password protection via HTTP Basic Auth
- Configure via: Settings → Web Password
- Password stored securely in flash (8-16 characters)
- Device-only configuration (no web-based password changes)
- Disable option available if password forgotten

For WiFi state machine details, see **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#wifi-configuration-and-ap-mode)**.

### SD Card Storage

**Format:** FAT32 (required - exFAT not supported)
**Recommended Size:** 4-32 GB SDHC (Class 10 or higher)
**Initialization:** On-demand (lazy init when storage page accessed to avoid SPI conflicts with display)
**Access:** Web interface at `http://vail-summit.local/storage`

**Supported Operations:**
- File browser with size and type display
- Upload files from computer to SD card
- Download files from SD card to computer
- Delete files from SD card
- View storage statistics (total/used/free space)

**Use Cases:**
- Data logging (training sessions, practice stats)
- QSO log backups (export logs to SD card)
- Configuration backups (save/restore device settings)
- Firmware updates (store firmware files for future updates)
- Custom content (callsign lists, practice text files)

**Important Notes:**
- Cards larger than 32 GB must be reformatted from exFAT to FAT32
- SD card shares SPI bus with display (uses separate chip select GPIO 38)
- No boot-time initialization to prevent display conflicts
- First access to storage page triggers SD card initialization

### Major Features

**CW Academy Training Mode** - 4-track, 16-session curriculum with progressive character introduction
**Morse Shooter Game** - Arcade-style game with adaptive decoder, supports straight key and iambic keyer
**Memory Chain Game** - Progressive memory training game with increasing sequence length and multiple difficulty modes
**Radio Output Mode** - Key external ham radios via 3.5mm jack (Summit Keyer or Radio Keyer modes)
**CW Memories** - Store and manage up to 10 reusable morse code message presets
**Morse Decoder** - Real-time adaptive decoding of paddle input with WPM tracking
**QSO Logger** - Device and web-based contact logging with ADIF/CSV export
**Storage Management** - Web-based SD card file management with upload/download capabilities
**Web Interface** - Comprehensive browser-based control and QSO management
**Vail Repeater** - Internet morse repeater via WebSocket connection

For detailed feature documentation, see **[docs/FEATURES.md](docs/FEATURES.md)**.

### Web Interface

**Dashboard:** Battery status, WiFi info, QSO count, navigation cards
**QSO Logger:** Table view, map visualization, ADIF/CSV export
**Device Settings:** CW speed/tone, volume, callsign configuration
**System Info:** Firmware version, memory stats, storage usage
**Radio Control:** Remote morse code transmission via radio output
**Storage Management:** SD card file browser, upload/download files, view storage stats

**Access:** `http://vail-summit.local/` or device IP address

For complete web interface documentation and API endpoints, see **[docs/WEB_INTERFACE.md](docs/WEB_INTERFACE.md)**.

## Common Development Patterns

### Adding a New Menu Mode

1. Add enum value to `MenuMode` in vail-summit.ino
2. Create header file in appropriate `src/` folder (e.g., `src/training/training_newmode.h`) with state, UI, and input handler
3. Add mode to menu arrays (options and icons)
4. Update `selectMenuItem()` to handle selection
5. Update `handleKeyPress()` to route input to your handler
6. Update `drawMenu()` to call your UI renderer
7. Add Preferences namespaces for persistent settings

### Creating Audio Feedback

```cpp
beep(frequency_hz, duration_ms);  // For short beeps/tones
startTone(frequency_hz);          // For continuous tone (manual stop)
stopTone();                       // Stop continuous tone
```

Always use these functions instead of direct I2S manipulation. They handle volume scaling and phase continuity.

### Display Optimization

- Only redraw what changes (use return codes from input handlers)
- Disable all display updates during audio-critical operations
- Use `fillRect()` to clear regions before redrawing text
- Use `getTextBounds_compat()` template function for text measurement (LovyanGFX compatibility wrapper)

### Morse Code Generation

```cpp
#include "src/core/morse_code.h"
const char* pattern = getMorseCode('A');  // Returns ".-"
MorseTiming timing(20);  // 20 WPM timing calculator
```

Use `playMorseString()` in `src/core/morse_code.h` for automatic playback with proper spacing.

For more development patterns, see **[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)**.

## Critical Constraints

1. **Never use `analogRead(A3)` or `analogRead(15)`** - breaks I2S audio completely
2. **Initialize I2S before display** - I2S needs higher DMA priority
3. **No display updates during audio playback** in practice/training modes - causes glitches
4. **Always use `beep()` or I2S functions for audio** - never manipulate GPIO 5 directly (repurposed pin)
5. **Load Preferences at startup, save immediately on change** - don't batch writes
6. **WebSocket handling must be non-blocking** - use state machine pattern in update loop
7. **Always initialize QSO structs** - `memset(&qso, 0, sizeof(QSO))` before populating to prevent garbage data
8. **LovyanGFX API compatibility** - Use `getTextBounds_compat()` instead of `getTextBounds()`, use `nullptr` for default fonts
9. **Display color order** - ST7796S requires BGR mode (`cfg.rgb_order = false`) for correct colors
10. **SD card initialization** - Must be done AFTER display init (or on-demand) to avoid SPI bus conflicts
11. **SD card format** - Must use FAT32 (exFAT not supported by Arduino SD library)

For troubleshooting common issues, see **[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md#troubleshooting)**.

## Firmware Version Management

**Version Information:** `src/core/config.h` (lines 9-14)

```cpp
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_DATE "2025-01-30"  // Update each build
#define FIRMWARE_NAME "VAIL SUMMIT"
```

**Update Guidelines:**
- **VERSION**: Increment for releases (major.minor.patch)
- **DATE**: Update to current date (YYYY-MM-DD) every build
- **NAME**: Should remain "VAIL SUMMIT"

Version appears in: Web dashboard, System Info page, Serial output, ADIF exports

For firmware build and deployment details, see **[docs/BUILDING.md](docs/BUILDING.md#firmware-updates)**.

## Repository Structure

**Branch Structure:**
- **`vail-summit` branch** (this branch): Summit ESP32-S3 firmware source code
- **`master` branch**: Web updater tool + compiled firmware binaries

**Deployment:**
1. Develop/test on `vail-summit` branch
2. Compile firmware (GitHub Actions or Arduino CLI)
3. Binaries committed to `master` at `docs/firmware_files/summit/`
4. Users flash via `https://update.vailadapter.com`

## Morse Code Timing (PARIS Standard)

All timing uses the **PARIS standard** (50 dit units per word):

- Dit duration: `1200 / WPM` milliseconds
- Dah duration: `3 × dit`
- Inter-element gap: `1 × dit`
- Letter gap: `3 × dit`
- Word gap: `7 × dit`

**WPM Range:** 5-40 WPM (configurable per mode, stored in Preferences)

## Input Handling Pattern

Each mode implements three key functions:

1. **`start<Mode>(tft)`** - Initialize mode state and draw initial UI
2. **`handle<Mode>Input(key, tft)`** - Process keyboard input, return -1 to exit
3. **`update<Mode>()`** (optional) - Called every loop iteration for real-time updates

**Return codes:**
- `-1` - Exit mode (return to parent menu)
- `0` or `1` - Normal input processed
- `2` - Full UI redraw requested
- `3` - Partial UI update (e.g., input box only)

## Code Organization

The firmware uses a modular architecture with all header files organized into thematic folders under `src/`:

```
vail-summit/
├── src/
│   ├── core/           # Core system files
│   ├── audio/          # Audio system and morse decoder
│   ├── ui/             # UI components and menu system
│   ├── training/       # Training modes
│   ├── games/          # Games
│   ├── radio/          # Radio integration
│   ├── settings/       # Settings management
│   ├── qso/            # QSO logging
│   ├── web/            # Web interface
│   │   ├── server/     # Web server core
│   │   ├── api/        # REST API endpoints
│   │   ├── pages/      # HTML/CSS/JS pages
│   │   └── modes/      # WebSocket handlers
│   └── network/        # Network services
├── vail-summit.ino     # Main sketch file
└── docs/               # Documentation
```

### Core System (`src/core/`)

- `config.h` - Hardware configuration, pin assignments, timing constants, color scheme
- `morse_code.h` - Morse lookup table and timing calculations (PARIS method)
- `hardware_init.h` - Hardware initialization routines

### Audio System (`src/audio/`)

- `i2s_audio.h` - I2S driver for MAX98357A with software volume control
- `morse_wpm.h` - WPM timing utilities (PARIS/Farnsworth) - EUPL v1.2
- `morse_decoder.h` - Base decoder class (timings → text) - EUPL v1.2
- `morse_decoder_adaptive.h` - Adaptive speed tracking - EUPL v1.2

### UI Components (`src/ui/`)

- `menu_navigation.h` - Menu navigation and input routing
- `menu_ui.h` - Menu rendering and card-based UI
- `status_bar.h` - Status bar icons (WiFi, battery, audio)

### Training Modes (`src/training/`)

- `training_hear_it_type_it.h` - Configurable receive training (callsigns, letters, numbers, custom) with 5 modes
- `training_practice.h` - Practice oscillator with iambic keyer and real-time decoding
- `training_cwa.h` - CW Academy main menu and session selection
- `training_cwa_core.h` - CW Academy core functions and state management
- `training_cwa_data.h` - CW Academy curriculum data (4 tracks, 16 sessions)
- `training_cwa_copy_practice.h` - CW Academy copy practice mode (listen and type)
- `training_cwa_send_practice.h` - CW Academy send practice mode (key what you see)
- `training_cwa_qso_practice.h` - CW Academy QSO practice mode (simulated contacts)
- `training_koch_method.h` - Koch Method main menu and mode selection
- `training_koch_core.h` - Koch Method core logic and progression
- `training_koch_ui.h` - Koch Method UI rendering

### Games (`src/games/`)

- `game_morse_shooter.h` - Arcade-style game with adaptive decoder, straight key and iambic keyer support, in-game settings
- `game_morse_memory.h` - Memory Chain progressive sequence game with difficulty levels and game modes

### Radio Integration (`src/radio/`)

- `radio_output.h` - Radio keying output (Summit Keyer and Radio Keyer modes)
- `radio_cw_memories.h` - CW message memories (10 programmable slots)

### Settings Management (`src/settings/`)

- `settings_wifi.h` - WiFi scanning, connection, credential storage, AP mode
- `settings_cw.h` - CW speed, tone frequency, key type settings
- `settings_volume.h` - Volume adjustment and persistence
- `settings_callsign.h` - User callsign configuration
- `settings_general.h` - General device settings
- `settings_web_password.h` - Web interface password protection

### QSO Logging (`src/qso/`)

- `qso_logger.h` - Data structures and field definitions
- `qso_logger_storage.h` - SPIFFS JSON storage for QSO logs
- `qso_logger_input.h` - Device-side input forms and field navigation
- `qso_logger_validation.h` - Validation functions for callsigns, frequencies, RST
- `qso_logger_view.h` - QSO log viewing and browsing
- `qso_logger_ui.h` - QSO logger UI rendering
- `qso_logger_settings.h` - QSO logger settings (operator info, POTA)
- `qso_logger_statistics.h` - QSO statistics and metrics

### Web Interface (`src/web/`)

**Server Core (`src/web/server/`):**
- `web_server.h` - AsyncWebServer setup and routing (main coordinator)
- `web_server_api.h` - Core API functions (device status, QSO logs, ADIF/CSV export)

**REST API (`src/web/api/`):**
- `web_api_wifi.h` - WiFi API endpoints (scan, connect, credentials)
- `web_api_qso.h` - QSO logger API endpoints (create, read, update, delete)
- `web_api_settings.h` - Settings API endpoints (CW, volume, callsign)
- `web_api_memories.h` - CW memories API endpoints (CRUD operations)

**HTML Pages (`src/web/pages/`):**
- `web_pages_dashboard.h` - Main dashboard HTML/CSS/JS
- `web_pages_wifi.h` - WiFi setup page HTML/CSS/JS
- `web_pages_practice.h` - Practice mode page HTML/CSS/JS
- `web_pages_memory_chain.h` - Memory Chain game page HTML/CSS/JS
- `web_pages_hear_it_type_it.h` - Hear It Type It training page HTML/CSS/JS
- `web_pages_radio.h` - Radio control page HTML/CSS/JS
- `web_pages_settings.h` - Device settings page HTML/CSS/JS
- `web_pages_system.h` - System diagnostics page HTML/CSS/JS
- `web_logger_enhanced.h` - Enhanced QSO logger HTML/CSS/JS

**WebSocket Handlers (`src/web/modes/`):**
- `web_practice_socket.h` - WebSocket handler for practice mode
- `web_practice_mode.h` - Device-side web practice mode handler
- `web_memory_chain_socket.h` - WebSocket handler for memory chain game
- `web_memory_chain_mode.h` - Device-side web memory chain mode handler
- `web_hear_it_socket.h` - WebSocket handler for hear it type it mode
- `web_hear_it_mode.h` - Device-side web hear it mode handler

### Network Services (`src/network/`)

- `vail_repeater.h` - WebSocket client for vailmorse.com morse repeater
- `ntp_time.h` - NTP time synchronization
- `pota_api.h` - Parks On The Air API integration

### Storage (`src/storage/`)

- `sd_card.h` - SD card initialization and file management functions

### Include Path Conventions

**From main .ino file:**
```cpp
#include "src/core/config.h"
#include "src/audio/i2s_audio.h"
#include "src/training/training_practice.h"
```

**Between header files (relative paths):**
```cpp
// From src/training/training_practice.h
#include "../core/config.h"
#include "../audio/i2s_audio.h"
#include "../core/morse_code.h"
```

## License Information

**Main Firmware:** Proprietary (VAIL SUMMIT Contributors)

**Morse Decoder Modules** (EUPL v1.2 - European Union Public Licence):
- `src/audio/morse_wpm.h`
- `src/audio/morse_decoder.h`
- `src/audio/morse_decoder_adaptive.h`

Original decoder code: Copyright (c) 2024 Stephen C Phillips
ESP32 port: Copyright (c) 2025 VAIL SUMMIT Contributors

**EUPL v1.2 Summary:**
- Weak copyleft - can be used in proprietary firmware
- Must keep decoder modules open source if modified
- Compatible with GPL, LGPL, MPL

Only the three decoder modules are EUPL-licensed. Main firmware can remain under any license.

## Support and Documentation

For help or to report issues:
- `/help` - Get help with using Claude Code
- Report issues: https://github.com/anthropics/claude-code/issues

For specific topic documentation:
- Building: [docs/BUILDING.md](docs/BUILDING.md)
- Architecture: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- Hardware: [docs/HARDWARE.md](docs/HARDWARE.md)
- Features: [docs/FEATURES.md](docs/FEATURES.md)
- Web Interface: [docs/WEB_INTERFACE.md](docs/WEB_INTERFACE.md)
- Development: [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)
