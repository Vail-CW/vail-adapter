# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VAIL SUMMIT is a portable morse code training device built on the ESP32-S3 Feather platform with an LCD display and modern UI. It's designed for ham radio operators to practice receiving and sending morse code. Input comes from a CardKB I2C keyboard, iambic paddle, and capacitive touch pads. The device includes training modes, settings management, WiFi connectivity to the Vail internet morse repeater, and extensive hardware integration (battery monitoring, I2S audio).

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
1. Adafruit GFX Library
2. Adafruit ST7735 and ST7789 Library
3. Adafruit MAX1704X (battery monitoring)
4. Adafruit LC709203F (backup battery monitor support)
5. WebSockets by Markus Sattler
6. ArduinoJson by Benoit Blanchon

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
- Games: `MODE_MORSE_SHOOTER`
- Radio: `MODE_RADIO_OUTPUT`, `MODE_CW_MEMORIES`
- Settings: `MODE_WIFI_SETTINGS`, `MODE_CW_SETTINGS`, `MODE_VOLUME_SETTINGS`
- Network: `MODE_VAIL_REPEATER`, QSO Logger via web interface

For detailed architecture information, see **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)**.

### Hardware Interfaces

**Display:** ST7789V 240×320 LCD (rotated to 320×240 landscape)
**Keyboard:** CardKB I2C (address 0x5F)
**Paddle Input:** GPIO 6 (DIT), GPIO 9 (DAH)
**Capacitive Touch:** GPIO 8 (DIT), GPIO 5 (DAH)
**Radio Output:** GPIO 18 (DIT), GPIO 17 (DAH)
**I2S Audio:** MAX98357A amplifier (BCK=14, LRC=15, DIN=16)
**Battery Monitor:** MAX17048 or LC709203F (I2C auto-detected)

For complete pin assignments and hardware details, see **[docs/HARDWARE.md](docs/HARDWARE.md)**.

### Configuration Management

All settings stored in ESP32 Preferences (non-volatile flash):

- **"wifi"** - Up to 3 WiFi network credentials
- **"cwsettings"** - WPM speed, tone frequency, key type
- **"audio"** - Volume percentage
- **"callsign"** - User callsign for Vail repeater
- **"cwa"** - CW Academy progress (track, session, practice/message types)
- **"radio"** - Radio mode (Summit Keyer vs Radio Keyer)
- **"qso_operator"** - Station info for QSO logging

Settings loaded on startup, saved immediately when changed.

### WiFi Configuration

**Station Mode:** Connects to existing WiFi networks (default)
**Access Point Mode:** Creates own WiFi network (`VAIL-SUMMIT-XXXXXX`, password: `vailsummit`)

To enter AP mode: Settings → WiFi Setup → Press 'A' key

**Web Server Access:**
- mDNS: `http://vail-summit.local/` (Station mode only)
- Direct IP: `http://192.168.4.1/` (AP mode) or device IP (Station mode)

For WiFi state machine details, see **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#wifi-configuration-and-ap-mode)**.

### Major Features

**CW Academy Training Mode** - 4-track, 16-session curriculum with progressive character introduction
**Morse Shooter Game** - Arcade-style game using iambic keyer to shoot falling letters
**Radio Output Mode** - Key external ham radios via 3.5mm jack (Summit Keyer or Radio Keyer modes)
**Morse Decoder** - Real-time adaptive decoding of paddle input with WPM tracking
**QSO Logger** - Device and web-based contact logging with ADIF/CSV export
**Web Interface** - Comprehensive browser-based control and QSO management
**Vail Repeater** - Internet morse repeater via WebSocket connection

For detailed feature documentation, see **[docs/FEATURES.md](docs/FEATURES.md)**.

### Web Interface

**Dashboard:** Battery status, WiFi info, QSO count, navigation cards
**QSO Logger:** Table view, map visualization, ADIF/CSV export
**Device Settings:** CW speed/tone, volume, callsign configuration
**System Info:** Firmware version, memory stats, storage usage
**Radio Control:** Remote morse code transmission via radio output

**Access:** `http://vail-summit.local/` or device IP address

For complete web interface documentation and API endpoints, see **[docs/WEB_INTERFACE.md](docs/WEB_INTERFACE.md)**.

## Common Development Patterns

### Adding a New Menu Mode

1. Add enum value to `MenuMode` in morse_trainer_menu.ino
2. Create header file (e.g., `training_newmode.h`) with state, UI, and input handler
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
- Cache text bounds with `getTextBounds()` for centering

### Morse Code Generation

```cpp
#include "morse_code.h"
const char* pattern = getMorseCode('A');  // Returns ".-"
MorseTiming timing(20);  // 20 WPM timing calculator
```

Use `playMorseString()` in morse_code.h for automatic playback with proper spacing.

For more development patterns, see **[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)**.

## Critical Constraints

1. **Never use `analogRead(A3)` or `analogRead(15)`** - breaks I2S audio completely
2. **Initialize I2S before display** - I2S needs higher DMA priority
3. **No display updates during audio playback** in practice/training modes - causes glitches
4. **Always use `beep()` or I2S functions for audio** - never manipulate GPIO 5 directly (repurposed pin)
5. **Load Preferences at startup, save immediately on change** - don't batch writes
6. **WebSocket handling must be non-blocking** - use state machine pattern in update loop
7. **Always initialize QSO structs** - `memset(&qso, 0, sizeof(QSO))` before populating to prevent garbage data

For troubleshooting common issues, see **[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md#troubleshooting)**.

## Firmware Version Management

**Version Information:** `config.h` (lines 9-14)

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

## Modular Header Files

Each major feature is isolated in its own header file:

**Core System:**
- `config.h` - Hardware configuration, pin assignments, timing constants, color scheme
- `morse_code.h` - Morse lookup table and timing calculations (PARIS method)

**Audio System:**
- `i2s_audio.h` - I2S driver for MAX98357A with software volume control
- `morse_wpm.h` - WPM timing utilities (PARIS/Farnsworth) - EUPL v1.2
- `morse_decoder.h` - Base decoder class (timings → text) - EUPL v1.2
- `morse_decoder_adaptive.h` - Adaptive speed tracking - EUPL v1.2

**Training Modes:**
- `training_hear_it_type_it.h` - Random callsign generator and receive training
- `training_practice.h` - Practice oscillator with iambic keyer and real-time decoding
- `training_cwa.h` - CW Academy curriculum (4 tracks, 16 sessions each)

**Games:**
- `game_morse_shooter.h` - Arcade game with falling letters and morse code shooting

**Radio Integration:**
- `radio_output.h` - Radio keying output (Summit Keyer and Radio Keyer modes)
- `radio_cw_memories.h` - CW message memories (placeholder for future)

**Settings:**
- `settings_wifi.h` - WiFi scanning, connection, credential storage
- `settings_cw.h` - CW speed, tone frequency, key type settings
- `settings_volume.h` - Volume adjustment and persistence
- `settings_general.h` - User callsign configuration

**Network:**
- `vail_repeater.h` - WebSocket client for vail.woozle.org morse repeater

**QSO Logging:**
- `qso_logger.h` - Data structures and field definitions
- `qso_logger_storage.h` - SPIFFS JSON storage for QSO logs
- `qso_logger_input.h` - Device-side input forms and field navigation
- `qso_logger_validation.h` - Validation functions for callsigns, frequencies, RST

**Web Interface:**
- `web_server.h` - AsyncWebServer for device control and QSO management
- `web_logger_enhanced.h` - HTML/CSS/JavaScript for enhanced QSO logger

## License Information

**Main Firmware:** Proprietary (VAIL SUMMIT Contributors)

**Morse Decoder Modules** (EUPL v1.2 - European Union Public Licence):
- `morse_wpm.h`
- `morse_decoder.h`
- `morse_decoder_adaptive.h`

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
