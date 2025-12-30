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

**Arduino IDE Setup (Critical Settings):**

| Setting | Value | ⚠️ Warning |
|---------|-------|------------|
| Board | Adafruit Feather ESP32-S3 2MB PSRAM | |
| USB CDC On Boot | Enabled | |
| **USB Mode** | **Hardware CDC and JTAG** | NOT TinyUSB (causes boot crash) |
| Upload Mode | UART0 / Hardware CDC | |
| Flash Size | 4MB (32Mb) | |
| **PSRAM** | **QSPI PSRAM** | NOT OPI (causes PSRAM detection failure) |
| Partition Scheme | Huge APP (3MB No OTA/1MB SPIFFS) | |
| Upload Speed | 921600 | |

**Required Libraries** (Install via Arduino Library Manager):
1. LovyanGFX by lovyan03 (display driver for ST7796S)
2. **lvgl v8.3.x** (UI framework - **NOT v9.x**, API incompatible)
3. Adafruit MAX1704X (battery monitoring)
4. Adafruit LC709203F (backup battery monitor support)
5. WebSockets by Markus Sattler
6. ArduinoJson by Benoit Blanchon
7. NimBLE-Arduino (Bluetooth BLE HID/MIDI)
8. ESPAsyncWebServer (install from GitHub)
9. AsyncTCP (install from GitHub, dependency for ESPAsyncWebServer)

**Arduino ESP32 Core:** Version 2.0.14 (required for ST7796S display compatibility)

**Arduino CLI Compilation:**
```bash
arduino-cli compile \
  --fqbn "esp32:esp32:adafruit_feather_esp32s3:CDCOnBoot=cdc,PartitionScheme=huge_app,PSRAM=enabled,FlashSize=4M" \
  vail-summit/
```

For detailed build instructions, firmware updates, and GitHub Actions workflow, see **[docs/BUILDING.md](docs/BUILDING.md)**.

### Architecture Overview

The system operates as a **mode-based state machine with LVGL UI**:
- **LVGL** handles all UI rendering, input processing, and screen transitions
- Each mode has its own LVGL screen created by `src/lvgl/` modules
- Main loop calls `lv_timer_handler()` to process LVGL tasks
- Mode changes trigger screen transitions with animations
- CardKB input is processed through LVGL's input driver system

**Key modes:**
- Training: `MODE_HEAR_IT_TYPE_IT`, `MODE_PRACTICE`, `MODE_CW_ACADEMY_*`
- Games: `MODE_MORSE_SHOOTER`, `MODE_MORSE_MEMORY`
- Radio: `MODE_RADIO_OUTPUT`, `MODE_CW_MEMORIES`
- Settings: `MODE_WIFI_SETTINGS`, `MODE_CW_SETTINGS`, `MODE_VOLUME_SETTINGS`
- Network: `MODE_VAIL_REPEATER`, `MODE_WEB_PRACTICE`, `MODE_WEB_MEMORY_CHAIN`, `MODE_WEB_HEAR_IT`, QSO Logger via web interface

For detailed architecture information, see **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)**.

### Hardware Interfaces

**Display:** ST7796S 4.0" 480×320 LCD (landscape orientation) - LovyanGFX driver with LVGL rendering
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

The device uses LVGL-based card menus with keyboard navigation:

**Navigation Keys:**
- **Up/Down arrows** - Navigate between menu items (mapped to `LV_KEY_PREV`/`LV_KEY_NEXT`)
- **Left/Right arrows** - Adjust values in sliders/settings (mapped to `LV_KEY_LEFT`/`LV_KEY_RIGHT`)
- **Enter** - Select/activate focused item (mapped to `LV_KEY_ENTER`)
- **ESC** - Go back to parent menu / Exit current mode (mapped to `LV_KEY_ESC`)
- **ESC (triple press)** - Enter deep sleep mode from main menu

Menu screens use LVGL's input group system for keyboard focus management. Each navigable widget is added to the group via `addNavigableWidget()`.

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

## LVGL UI System

The UI is built entirely with LVGL (Light and Versatile Graphics Library) v8.3.x. LovyanGFX serves as the display driver, while LVGL handles all rendering, input, and animations.

### LVGL Configuration

**lv_conf.h Location:** `Arduino/libraries/lv_conf.h` (next to lvgl folder, NOT in project)

**Key Settings:**
```cpp
#define LV_COLOR_DEPTH 16           // RGB565
#define LV_COLOR_16_SWAP 0          // Byte swap handled in flush callback
#define LV_MEM_SIZE (48U * 1024U)   // 48KB internal heap
#define LV_TICK_CUSTOM 1            // Uses millis() for timing
#define LV_DISP_DEF_REFR_PERIOD 33  // ~30 FPS
#define LV_INDEV_DEF_READ_PERIOD 30 // Input polling rate
```

**Enabled Fonts:** Montserrat 12, 14, 16, 18, 20, 22, 24, 28

**Enabled Widgets:** btn, label, slider, dropdown, list, textarea, msgbox, bar, checkbox, switch, roller, table, menu, spinner, led

### LVGL File Structure

```
src/lvgl/
├── lv_init.h              # Display/input driver initialization
├── lv_screen_manager.h    # Screen transitions, navigation groups
├── lv_theme_summit.h      # Custom dark theme and styles
├── lv_widgets_summit.h    # Reusable widget factories
├── lv_splash_screen.h     # Boot splash with progress bar
├── lv_menu_screens.h      # Menu screen creators
├── lv_settings_screens.h  # Settings screen creators
├── lv_training_screens.h  # Training mode screens
├── lv_game_screens.h      # Game mode screens
├── lv_mode_screens.h      # Network/radio mode screens
└── lv_mode_integration.h  # Mode state machine integration
```

### Display Driver (lv_init.h)

```cpp
// Initialize LVGL with LovyanGFX display
bool initLVGL(LGFX& tft);

// Get input group for keyboard navigation
lv_group_t* getLVGLInputGroup();

// Key mapping: CardKB → LVGL
// KEY_UP (0xB5)    → LV_KEY_PREV
// KEY_DOWN (0xB6)  → LV_KEY_NEXT
// KEY_LEFT (0xB4)  → LV_KEY_LEFT
// KEY_RIGHT (0xB7) → LV_KEY_RIGHT
// KEY_ENTER (0x0D) → LV_KEY_ENTER
// KEY_ESC (0x1B)   → LV_KEY_ESC
```

**Display Flush:** Uses `pushPixels(..., true)` with swap565 enabled for correct colors on SPI display.

### Screen Manager (lv_screen_manager.h)

```cpp
// Create a new screen with default styling
lv_obj_t* createScreen();

// Load screen with animation (clears navigation group handled externally)
void loadScreen(lv_obj_t* screen, ScreenAnimType anim);

// Add widget to navigation group with ESC handler
void addNavigableWidget(lv_obj_t* widget);

// Clear all widgets from navigation group
void clearNavigationGroup();

// Set global back navigation callback
void setBackCallback(BackActionCallback callback);
```

**Screen Transitions:**
- `SCREEN_ANIM_NONE` - Instant switch
- `SCREEN_ANIM_FADE` - Fade in/out (default)
- `SCREEN_ANIM_SLIDE_LEFT` - Slide left (entering submenu)
- `SCREEN_ANIM_SLIDE_RIGHT` - Slide right (going back)

### Theme System (lv_theme_summit.h)

Custom dark theme with cyan/teal accent colors:
```cpp
// Initialize theme (call after LVGL init)
void initSummitTheme();

// Color palette
LV_COLOR_BG_DEEP      // 0x0841 - Deep dark background
LV_COLOR_BG_LAYER2    // 0x1082 - Slightly lighter
LV_COLOR_ACCENT       // 0x07FF - Cyan accent
LV_COLOR_TEXT_PRIMARY // 0xFFFF - White text
LV_COLOR_TEXT_SECONDARY // 0x8410 - Gray text
LV_COLOR_SUCCESS      // 0x07E0 - Green
LV_COLOR_WARNING      // 0xFD20 - Orange
LV_COLOR_ERROR        // 0xF800 - Red

// Style getters
lv_style_t* getStyleMenuCard();        // Menu button style
lv_style_t* getStyleMenuCardFocused(); // Focused state with glow
lv_style_t* getStyleLabelTitle();      // Large title text
lv_style_t* getStyleLabelBody();       // Normal body text
```

### Creating LVGL Screens

**Pattern for new screens:**
```cpp
lv_obj_t* createMyScreen() {
    // 1. Clear old navigation widgets
    clearNavigationGroup();

    // 2. Create screen with theme background
    lv_obj_t* screen = createScreen();
    applyScreenStyle(screen);

    // 3. Add UI elements
    lv_obj_t* btn = lv_btn_create(screen);
    // ... configure button ...

    // 4. Add navigable widgets to input group
    addNavigableWidget(btn);

    return screen;
}
```

**Loading screens:**
```cpp
lv_obj_t* screen = createMyScreen();
loadScreen(screen, SCREEN_ANIM_SLIDE_LEFT);
```

### Mode Integration (lv_mode_integration.h)

Bridges LVGL screens with the mode state machine:

```cpp
// Initialize (call after LVGL and theme init)
void initLVGLModeIntegration();

// Show initial screen after boot
void showInitialLVGLScreen();

// Menu selection callback (triggers mode change)
void onLVGLMenuSelect(int target_mode);

// Back navigation callback (ESC key)
void onLVGLBackNavigation();

// Create screen for any mode
lv_obj_t* createScreenForModeInt(int mode);

// Get parent mode for back navigation
int getParentModeInt(int mode);
```

### Main Loop Integration

```cpp
void loop() {
    // Process LVGL (handles UI, input, animations)
    lv_timer_handler();

    // Other non-UI tasks...
}
```

### LVGL Critical Constraints

1. **Navigation group management** - Always call `clearNavigationGroup()` BEFORE creating a new screen's widgets, then add widgets with `addNavigableWidget()`
2. **Screen deletion** - Use `loadScreen()` with delete flag; don't manually delete screens
3. **Input state constants** - Use `LV_INDEV_STATE_PR` and `LV_INDEV_STATE_REL` (not `_PRESSED`/`_RELEASED`)
4. **Color byte swap** - Handle in flush callback with `pushPixels(..., true)`, keep `LV_COLOR_16_SWAP = 0`
5. **lv_conf.h location** - Must be in `Arduino/libraries/` folder next to lvgl, NOT in project folder
6. **Font availability** - Only use fonts enabled in lv_conf.h (Montserrat 12-28)
7. **Static variables in headers** - LVGL screen files use static variables; be aware of include order

## Common Development Patterns

### Adding a New Menu Mode (LVGL)

1. Add enum value to `MenuMode` in vail-summit.ino
2. Add mode constant to `lv_mode_integration.h` (e.g., `#define LVGL_MODE_NEW_FEATURE 80`)
3. Create screen function in appropriate `src/lvgl/lv_*_screens.h`:
   ```cpp
   lv_obj_t* createNewFeatureScreen() {
       lv_obj_t* screen = createScreen();
       applyScreenStyle(screen);
       // Add widgets, call addNavigableWidget() for each
       return screen;
   }
   ```
4. Add to `createScreenForModeInt()` in `lv_mode_integration.h`
5. Add to menu items array in `lv_menu_screens.h`
6. Add parent mapping in `getParentModeInt()` for back navigation
7. Add Preferences namespaces for persistent settings

### Adding a New Menu Mode (Legacy Pattern - deprecated)

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

**LVGL handles most optimization automatically:**
- LVGL only redraws dirty regions (partial updates)
- Use `lv_obj_invalidate()` to force redraw of specific objects
- Screen transitions use `lv_scr_load_anim()` with configurable duration

**For direct LovyanGFX access (rare):**
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

### Audio System
1. **Never use `analogRead(A3)` or `analogRead(15)`** - breaks I2S audio completely
2. **Initialize I2S before display** - I2S needs higher DMA priority
3. **No display updates during audio playback** in practice/training modes - causes glitches
4. **Always use `beep()` or I2S functions for audio** - never manipulate GPIO 5 directly (repurposed pin)

### LVGL UI
5. **lv_conf.h location** - Must be in `Arduino/libraries/` folder next to lvgl library, NOT in project
6. **Color byte swap** - Use `pushPixels(..., true)` in flush callback, keep `LV_COLOR_16_SWAP = 0`
7. **Navigation group order** - Call `clearNavigationGroup()` BEFORE creating screen widgets
8. **Input state constants** - Use `LV_INDEV_STATE_PR`/`LV_INDEV_STATE_REL` (v8.3 API)
9. **Screen deletion** - Let `loadScreen()` handle deletion via `lv_scr_load_anim(..., true)`

### Display Hardware
10. **Display color order** - ST7796S requires BGR mode (`cfg.rgb_order = false`)
11. **LovyanGFX API compatibility** - Use `getTextBounds_compat()` instead of `getTextBounds()`, use `nullptr` for default fonts

### Storage & Data
12. **Load Preferences at startup, save immediately on change** - don't batch writes
13. **WebSocket handling must be non-blocking** - use state machine pattern in update loop
14. **Always initialize QSO structs** - `memset(&qso, 0, sizeof(QSO))` before populating to prevent garbage data
15. **SD card initialization** - Must be done AFTER display init (or on-demand) to avoid SPI bus conflicts
16. **SD card format** - Must use FAT32 (exFAT not supported by Arduino SD library)

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

**IMPORTANT: Version Update on Git Push**

Before pushing changes to GitHub, you MUST:
1. Check the current version in `src/core/config.h`
2. Ask the user what the new version should be (e.g., "Current version is 0.2-4inch. What should the new version be?")
3. Update `FIRMWARE_VERSION` to the user-specified version
4. Update `FIRMWARE_DATE` to today's date
5. Include the version update in the commit

Version appears in: Boot splash screen, System Info settings, Web dashboard, Serial output, ADIF exports

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
│   ├── lvgl/           # LVGL UI screens and integration (NEW)
│   ├── ui/             # Legacy UI components (being migrated)
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
│   ├── network/        # Network services
│   └── storage/        # SD card management
├── vail-summit.ino     # Main sketch file
└── docs/               # Documentation
```

### Core System (`src/core/`)

- `config.h` - Hardware configuration, pin assignments, timing constants, color scheme
- `morse_code.h` - Morse lookup table and timing calculations (PARIS method)
- `hardware_init.h` - Hardware initialization routines

### LVGL UI System (`src/lvgl/`)

- `lv_init.h` - LVGL initialization, display driver (LovyanGFX flush), CardKB input driver
- `lv_screen_manager.h` - Screen stack, transitions, navigation group management
- `lv_theme_summit.h` - Custom dark theme, color palette, reusable styles
- `lv_widgets_summit.h` - Widget factory functions (cards, sliders, inputs)
- `lv_splash_screen.h` - Boot splash screen with mountain logo and progress bar
- `lv_menu_screens.h` - Menu screen creators (main, CW, training, settings, etc.)
- `lv_settings_screens.h` - Settings screens (volume, brightness, CW, callsign, WiFi)
- `lv_training_screens.h` - Training mode screen creators
- `lv_game_screens.h` - Game mode screen creators
- `lv_mode_screens.h` - Network/radio mode screen creators
- `lv_mode_integration.h` - Bridges LVGL with mode state machine, handles navigation

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
