# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains firmware and tooling for two devices in the Vail ecosystem:

### Vail Adapter (Master Branch)
Arduino-based firmware for Morse code key/paddle to USB conversion. It runs on SAMD21-based microcontrollers (Seeeduino XIAO SAMD21, Adafruit QT Py SAMD21, and Adafruit TRRS Trinkey SAMD21) and provides:
- USB HID keyboard output (Ctrl keys for dit/dah)
- USB MIDI control and output for DAW integration
- Multiple keyer modes (straight key, bug, iambic A/B, ultimatic, etc.)
- Sidetone generation via piezo buzzer
- Capacitive touch support
- Optional radio output for direct keying of amateur radios
- CW memory recording and playback (3 slots, 25 seconds each)

### Vail Summit (Vail-Summit Branch)
ESP32-S3 Feather based standalone morse code trainer with:
- Built-in display (ST7789 320x240)
- CardKB keyboard integration
- I2S audio output (MAX98357A amplifier)
- Multiple training modes (practice, hear-it-type-it, CW Academy)
- Morse Shooter game
- Vail repeater integration
- WiFi connectivity
- Battery monitoring

**Branch Structure:**
- `master` - Vail Adapter firmware + GitHub Pages updater site
- `vail-summit` - Vail Summit ESP32-S3 source code

## Project Structure (Vail Adapter - Master Branch)

The main firmware files are in the root directory:
- `vail-adapter.ino` - Main Arduino sketch (setup and loop coordination)
- `adapter.cpp/h` - VailAdapter class implementation
- `keyers.cpp/h` - Keyer mode implementations
- `memory.cpp/h` - CW memory recording and playback system
- `menu_handler.cpp/h` - Button menu state machine and operating modes
- `morse_audio.cpp/h` - Morse code playback and audio feedback
- `settings_eeprom.cpp/h` - EEPROM persistence for settings and memories
- `buttons.cpp/h` - Button handling with double-click detection
- `polybuzzer.cpp/h` - Audio output
- `bounce2.cpp/h` - Debouncing for physical inputs
- `touchbounce.cpp/h` - Debouncing for capacitive touch
- `config.h` - Hardware configuration

**Important:** Arduino compiles all `.ino` files in the same directory together. Keep only the main `vail-adapter.ino` file in the root directory to avoid conflicts with `setup()` and `loop()` functions.

## Building and Flashing

### Arduino CLI Commands

The project uses arduino-cli for building. Required libraries:
- MIDIUSB
- Adafruit FreeTouch Library
- FlashStorage_SAMD
- Keyboard

**Setup arduino-cli (first time only):**
```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
arduino-cli core update-index
arduino-cli core install Seeeduino:samd
arduino-cli core install adafruit:samd
arduino-cli lib install MIDIUSB "Adafruit FreeTouch Library" FlashStorage_SAMD Keyboard
```

**Build for XIAO SAMD21:**
```bash
arduino-cli compile --fqbn Seeeduino:samd:seeed_XIAO_m0 --output-dir build_output --export-binaries .
```

**Build for QT Py SAMD21:**
```bash
arduino-cli compile --fqbn adafruit:samd:adafruit_qtpy_m0 --output-dir build_output --export-binaries .
```

**Build for TRRS Trinkey SAMD21:**
```bash
arduino-cli compile --fqbn adafruit:samd:adafruit_TRRStrinkey_m0 --output-dir build_output --export-binaries .
```

**Convert .bin to .uf2 (for drag-and-drop flashing):**
```bash
python3 uf2conv.py -c -f 0x68ED2B88 -b 0x2000 build_output/*.bin -o firmware.uf2
```

### Hardware Configuration

Before building, you MUST edit `config.h` and uncomment exactly ONE hardware configuration:
- `V1_Basic_PCB` - Original PCB version
- `V2_Basic_PCB` - Revised PCB with updated pin mappings
- `Advanced_PCB` - Advanced PCB with radio output pins (A2/A3)
- `NO_PCB_GITHUB_SPECS` - Breadboard/hand-wired setup
- `TRRS_TRINKEY` - Adafruit TRRS Trinkey with built-in TRRS jack (see TRRS_TRINKEY_BUILD.md)

Each configuration sets different pin mappings for dit/dah/key inputs, piezo, and optional radio outputs.

**TRRS Trinkey Specific Notes:**
- Uses built-in TRRS jack for paddle/key input (TIP=dit on pin 0, RING1=dah on pin 2)
- Requires SLEEVE (pin 5) and RING2 (pin 4) to be driven LOW for proper TRRS jack operation
- Piezo buzzer connects to SDA (pin 7/PA08) via STEMMA QT connector
- No capacitive touch support (`NO_CAPACITIVE_TOUCH` defined)
- No LED control (`NO_LED` defined - NeoPixel requires special library)
- No button menu (no physical buttons on Trinkey)
- KEY_PIN not attached to prevent duplicate processing of pin 0 (dit timing fix)

For Advanced_PCB builds, also configure `RADIO_KEYING_ACTIVE_LOW` in config.h based on your radio's keying polarity.

## Architecture

### Core Components

**Main Loop (vail-adapter.ino)**
The main sketch has been refactored into a clean 270-line file that:
- Initializes all modules in `setup()` (morse audio, menu handler, EEPROM)
- Polls MIDI input and dispatches to VailAdapter
- Updates bounce debouncers for physical inputs (dit, dah, key)
- Updates capacitive touch inputs (qt_dit, qt_dah, qt_key)
- Delegates to `updateMenuHandler()` for all button logic
- Handles memory playback state machine
- Manages special LED states for radio mode and buzzer disable
- Manages TRS detection for straight key auto-configuration

**VailAdapter Class (adapter.cpp/h)**
The central state machine that:
- Manages keyboard vs. MIDI output modes
- Handles MIDI control messages (CC0=mode, CC1=speed, CC2=tone, PC=keyer type)
- Implements radio mode toggling (10 DAH presses within 500ms)
- Implements radio keyer mode toggling (5-second DAH hold in radio mode)
- Implements buzzer disable (5-second DIT hold)
- Routes paddle inputs to active keyer
- Controls radio output pins when HAS_RADIO_OUTPUT is defined
- Persists settings to EEPROM

**Keyer Mode Switching:**
When switching keyer types (via MIDI PC or button menu), `HandleMIDI()` calls `Reset()` on the old keyer before switching to the new one. This ensures any held key state (e.g., from iambic mode) is properly cleared and transmission is ended before the new keyer instance is created. Additionally, the `flushBounceState()` function is called after mode switches to clear any stale input states from the Bounce debouncers, preventing "stuck key" bugs when switching modes with cables plugged in.

**TRS Cable Detection (Straight Key Auto-Configuration):**
The firmware includes automatic detection of TRS (Tip-Ring-Sleeve) mono cables used for straight keys:
- **Boot-time detection** (vail-adapter.ino:99-107): During `setup()`, the DAH pin is checked 16 times. If continuously grounded, the `trs` flag is set.
- **Hot-plug detection** (vail-adapter.ino:143-195): **ONLY active when already in Straight Key mode (keyer type 1)**. Every 500ms, the firmware checks if the physical DAH pin has been continuously grounded for 1+ second. This mode-conditional approach ensures:
  - Detection only runs when user has intentionally switched to Straight Key mode
  - No interference with paddle operation (different keyer mode)
  - No conflict with radio mode features (DAH hold to toggle radio keyer mode)
  - Capacitive touch inputs are not monitored for TRS detection
  - If detected: Enables TRS mode and flushes bounce state
- **User workflow for hot-plugging**:
  1. Manually switch adapter to Straight Key mode (button menu: B3 long press, cycle to Straight Key, B2 long press to save)
  2. Plug in TRS/mono cable
  3. Wait 1 second → TRS mode auto-enables
  4. DIT pin now used for straight key input, DAH pin ignored
- **TRS mode behavior** (vail-adapter.ino:280-315): When `trs == true`:
  - DIT pin (tip) is used as the straight key input via `PADDLE_STRAIGHT`
  - DAH pin (ring, shorted to sleeve/ground) is ignored—`dah.update()` is called to maintain Bounce state but input is not processed
  - Prevents spurious "DAH pressed" events from grounded cable
- **Unplug detection**: If DAH pin stops being grounded while in TRS mode, the `trs` flag is cleared

This system prevents the "stuck key" bug that occurred when:
1. Switching from Iambic to Straight Key mode with a mono cable plugged in
2. Hot-plugging a straight key cable after boot

**Important**: TRS hot-plug detection will NEVER trigger during paddle operation (iambic/bug modes) because it only runs when the adapter is already in Straight Key mode. This eliminates all false positive scenarios.

**TRRS Trinkey Exception:**
On the TRRS Trinkey configuration, the KEY_PIN bouncer is not attached at all (vail-adapter.ino:61-64, 315-326). This is because KEY_PIN and DIT_PIN are both pin 0 on Trinkey, and processing the same pin twice caused a bug where:
- The keyer would correctly enforce minimum dit duration (e.g., 170ms)
- But the straight key handler would call `EndTx()` immediately when the paddle was released
- This stopped the buzzer prematurely, overriding the keyer's timing logic
- By not attaching KEY_PIN on Trinkey, only the paddle handler processes pin 0, allowing proper keyer timing

**Keyer System (keyers.cpp/h)**
Nine keyer algorithms are implemented as separate classes inheriting from base `Keyer`:
1. **Passthrough** (0) - Manual control, no automation
2. **StraightKeyer** (1) - For straight keys
3. **BugKeyer** (2) - Semi-automatic (auto-dit)
4. **ElBugKeyer** (3) - Electric bug variant
5. **SingleDotKeyer** (4) - Single dit mode
6. **UltimaticKeyer** (5) - Ultimatic priority logic
7. **PlainKeyer** (6) - Basic iambic without squeeze
8. **IambicAKeyer** (7) - Iambic mode A
9. **IambicBKeyer** (8) - Iambic mode B (most popular)
10. **KeyaheadKeyer** (9) - Key-ahead buffering

All keyers use the `Transmitter` interface to control output (BeginTx/EndTx), allowing the same keyer logic to work for keyboard, MIDI, and radio outputs.

**Input Debouncing**
- `Bounce` class (bounce2.cpp/h) - Software debouncing for physical switches (default 10ms interval)
- `TouchBounce` class (touchbounce.cpp/h) - Debouncing for Adafruit FreeTouch capacitive sensors
- **TRRS Trinkey:** Debounce interval increased to 25ms (vail-adapter.ino:69-78) due to different electrical characteristics of the switched TRRS jack

**Audio Output**
- `PolyBuzzer` class (polybuzzer.cpp/h) - Manages piezo buzzer tone generation using Arduino `tone()` function
- `equalTemperament.h` - Lookup table for MIDI note numbers to Hz frequencies
- `morse_audio.cpp/h` - Morse code playback and audio feedback module
  - Morse code functions: `playMorseDit()`, `playMorseDah()`, `playMorseChar()`, `playMorseWord()`
  - Audio feedback: `playAdjustmentBeep()`, `playErrorTone()`, `playDescendingTones()`, `playRecordingCountdown()`
  - Keyer announcements: `playKeyerTypeCode()` plays Morse identifiers for all 9 keyer types:
    - S (Straight), B (Bug), EB (ElBug), SD (SingleDot), U (Ultimatic), P (Plain), IA (Iambic A), IB (Iambic B), K (Keyahead)
  - Initialized via `initMorseAudio(&adapter, PIEZO_PIN)`

**CW Memory System (memory.cpp/h)**
The memory system provides recording and playback of CW sequences:
- **3 independent memory slots** - Each can store up to 25 seconds of CW
- **Run-length encoding** - Efficient storage with max 200 transitions per slot
- **Recording** - Captures actual key-down/key-up timing (bypasses keyer processing)
- **Playback modes**:
  - Memory management mode: Piezo-only preview for testing
  - Normal mode: Full output via keyboard/MIDI/radio for operation
- **Auto-trimming** - Removes trailing silence after last key release
- **EEPROM persistence** - All three slots saved across power cycles

Key data structures (memory.h):
- `CWMemory` - Stores transitions array and metadata for one slot
- `RecordingState` - Tracks active recording (slot, start time, transition count)
- `PlaybackState` - Manages playback timing and current position

State machine integration (menu_handler.cpp):
- `MODE_MEMORY_MANAGEMENT` - Entry via B1+B3 combo, exit via B1+B3 again
- `MODE_RECORDING_MEMORY_1/2/3` - Active recording for each slot
- `MODE_PLAYING_MEMORY` - Playback in progress

### Button Control & Menu System

**Button Debouncing (buttons.cpp/h)**
The button system uses a resistor ladder network with ButtonDebouncer class providing:
- **Debouncing** - 2 consistent readings required
- **Gesture detection**:
  - Quick press - Returns on button release
  - Long press - Fires after 2 seconds (once per press)
  - Combo press - Fires after 0.5 seconds for multi-button (once per press)
  - MIDI switch press - Fires after 3 seconds for B1+B2 combo (once per press)
  - Double-click - Detects rapid press-release-press within 400ms window
- **Max state tracking** - Records highest button combination during press

**Menu Handler Module (menu_handler.cpp/h)**
All button logic and menu state management has been extracted into a dedicated module:
- Operating modes: `MODE_NORMAL`, `MODE_SPEED_SETTING`, `MODE_TONE_SETTING`, `MODE_KEY_SETTING`, `MODE_MEMORY_MANAGEMENT`, `MODE_RECORDING_MEMORY_1/2/3`, `MODE_PLAYING_MEMORY`
- Helper functions: `ditDurationToWPM()`, `wpmToDitDuration()`, `applyTemporarySpeed()`, `applyTemporaryTone()`, `applyTemporaryKeyerType()`
- Main update function: `updateMenuHandler(currentTime, buttonDebouncer)` - called from main loop
- Initialized via `initMenuHandler(&adapter, memorySlots, &recordingState, &playbackState)`
- State access: `getMenuState()` returns `MenuHandlerState` struct with current mode and temp settings

**Operating Modes:**
- `MODE_NORMAL` - Default operation, quick press plays memories
- `MODE_SPEED_SETTING` - Adjust WPM (5-40 range), B1=faster, B3=slower, B2 long press to save
- `MODE_TONE_SETTING` - Adjust sidetone frequency (MIDI 43-85), B1=higher, B3=lower, B2 long press to save
- `MODE_KEY_SETTING` - Cycle through all 9 keyer types (1-9, excluding Passthrough), B1=next, B3=prev, B2 long press to save
  - Available types: Straight (1), Bug (2), ElBug (3), SingleDot (4), Ultimatic (5), Plain (6), Iambic A (7), Iambic B (8), Keyahead (9)
  - Each type announces itself via Morse code identifier when selected
- `MODE_MEMORY_MANAGEMENT` - Record/playback/clear memory slots

**State transition guards:**
- Long press only enters settings modes from MODE_NORMAL
- B1+B3 combo only toggles memory mode between NORMAL ↔ MEMORY_MANAGEMENT
- B1+B2 3-second hold toggles between keyboard and MIDI output modes (ONLY in MODE_NORMAL, plays "KM" or "MM" in Morse)
- Prevents cross-mode interference (e.g., can't enter speed mode from memory mode)
- Each mode has dedicated button handlers for isolated functionality
- 30-second timeout auto-saves and exits setting modes

### MIDI Integration

See `MIDI_INTEGRATION_SPEC.md` for full protocol details. Key points:
- **CC0** switches between keyboard and MIDI modes
- **CC1** sets dit duration (value × 2 milliseconds)
- **CC2** sets sidetone MIDI note number
- **PC** selects keyer mode (0-9)
- In MIDI mode with passthrough keyer (mode 0), outputs Note On/Off for C (straight), C# (dit), D (dah)

### Special Feature Activation Sequences

These are "Easter egg" features activated by specific input patterns:

1. **Keyboard/MIDI Mode Switch**: Hold B1+B2 for 3 seconds (in normal operation mode) → toggles between keyboard and MIDI output modes
   - Keyboard → MIDI: plays "MM" in Morse
   - MIDI → Keyboard: plays "KM" in Morse
   - Only active in MODE_NORMAL (won't trigger during settings changes, memory management, or recording)
   - Device still auto-switches from keyboard to MIDI mode when receiving MIDI control messages
   - Default boot mode: Keyboard
2. **Buzzer Disable**: Hold DIT for 5 seconds → toggles buzzer on/off (LED blinks slowly when disabled)
3. **Radio Mode**: Press DAH 10 times within 500ms → toggles radio mode (LED blinks rapidly when active)
4. **Radio Keyer Mode**: While in radio mode, hold DAH for 5 seconds → toggles radio keyer mode (direct radio keying vs. pass-through)

Radio mode and radio keyer mode are independent concepts:
- Radio mode enables radio output pins
- Radio keyer mode determines whether the keyer logic directly drives the radio pins (true) or just passes through paddle states (false)

### EEPROM Storage

**Settings and Memory Module (settings_eeprom.cpp/h)**
All EEPROM operations have been consolidated into a dedicated module:

**Adapter Settings Functions:**
- `loadSettingsFromEEPROM(adapter)` - Loads keyer type, dit duration, TX note
- `saveSettingsToEEPROM(keyerType, ditDuration, txNote)` - Persists settings
- `loadRadioKeyerModeFromEEPROM(adapter)` - Loads radio keyer mode
- `saveRadioKeyerModeToEEPROM(radioKeyerMode)` - Persists radio mode
- `loadToneFromEEPROM()` - Returns startup tone for VAIL announcement

**CW Memory Functions:**
- `loadMemoriesFromEEPROM(memorySlots)` - Loads all 3 memory slots
- `saveMemoryToEEPROM(slotNumber, memory)` - Saves one slot
- `clearMemoryInEEPROM(slotNumber)` - Clears one slot
- `getEEPROMAddressForSlot(slotNumber)` - Returns base address

**EEPROM Memory Map:**
- Keyer type (address 0)
- Dit duration (address 1-2, uint16_t)
- TX note (address 3)
- Valid flag (address 4, value 0x42)
- Radio keyer mode (address 5)
- CW Memory Slot 1 (EEPROM_MEMORY_1_ADDR): 400 bytes
- CW Memory Slot 2 (EEPROM_MEMORY_2_ADDR): 400 bytes
- CW Memory Slot 3 (EEPROM_MEMORY_3_ADDR): 400 bytes

Each memory slot stores:
- Transition count (2 bytes)
- Transition durations array (up to 200 × 2 bytes = 400 bytes max)

## Testing

No automated test suite exists. Manual testing procedures are documented in `docs/TESTING_GUIDE.md`.

## CI/CD

GitHub Actions workflow (`.github/workflows/build_uf2.yml`) automatically:
1. Builds firmware for all 9 hardware configurations (4 configs × 2 boards + Trinkey)
2. Converts .bin files to .uf2 format
3. Commits resulting firmware files to `docs/firmware_files/` on every push to master

Firmware naming convention:
- `xiao_basic_pcb_v1.uf2`, `xiao_basic_pcb_v2.uf2`, `xiao_advanced_pcb.uf2`, `xiao_non_pcb.uf2`
- `qtpy_basic_pcb_v1.uf2`, `qtpy_basic_pcb_v2.uf2`, `qtpy_advanced_pcb.uf2`, `qtpy_non_pcb.uf2`
- `trinkey_vail_adapter.uf2`

## GitHub Pages Maintenance

The project uses GitHub Pages to host a unified firmware update wizard at `https://vailadapter.com`. The site is located in the `docs/` directory and supports both devices:

### Vail Adapter Updates

**IMPORTANT:** When making significant commits to master (especially feature additions or bug fixes), update the "What's New in This Version" section in `docs/index.html`:

1. Update the date in the format: `<strong>Last Update:</strong> Month Day, Year`
2. Add or modify bullet points describing the changes
3. Keep the list focused on user-visible features and fixes
4. Ensure the manual link points to `https://vailadapter.com/manual`
5. Ensure setup instructions point to `https://vailmorse.com` (official Vail web repeater)

Example update locations in `docs/index.html`:
- Line ~24: Date stamp in "What's New" section
- Lines ~27-32: Feature bullet points

### Vail Summit Updates

The Summit updater uses a two-step ESP32 web flasher:
- **Device Icons**: 📡 for Adapter, ⛰️ for Summit
- **Firmware Location**: `docs/firmware_files/summit/` (bootloader.bin, partitions.bin, vail-summit.bin)
- **Flash Process**: See [SUMMIT_INTEGRATION.md](SUMMIT_INTEGRATION.md) for technical details

When updating Summit firmware:
1. Build firmware on `vail-summit` branch
2. Copy `.bin` files to `docs/firmware_files/summit/` on `master` branch
3. Update SUMMIT_INTEGRATION.md if needed
4. Consider adding firmware version display in future

## Common Development Patterns

### Adding a New Keyer Mode

1. Create new class inheriting from `Keyer` in `keyers.cpp`
2. Implement required methods: `Key()`, `Tick()`, `Reset()`, `SetDitDuration()`, `SetOutput()`, `Release()`
3. Add instance to `allKeyers[]` array at bottom of `keyers.cpp`
4. Update `GetKeyerByNumber()` function
5. Update MIDI_INTEGRATION_SPEC.md with new program change number

### Adding Hardware Configuration

1. Add new `#ifdef` block in `config.h` with pin definitions
2. Set `BOARD_NAME` string for serial output identification
3. If radio output is needed, define `RADIO_DIT_PIN`, `RADIO_DAH_PIN`, and `HAS_RADIO_OUTPUT`
4. Add new matrix entry to `.github/workflows/build_uf2.yml`

### Modifying MIDI Protocol

Changes to MIDI handling should:
1. Update `VailAdapter::HandleMIDI()` in adapter.cpp
2. Update MIDI_INTEGRATION_SPEC.md documentation
3. Consider EEPROM storage if the setting should persist

## Code Organization & Refactoring

The firmware has been refactored into a modular architecture for improved maintainability:

### Module Architecture

**Main Sketch (vail-adapter.ino - 270 lines)**
- Focused on setup and loop coordination
- Initializes all modules with proper dependencies
- Delegates to specialized modules rather than implementing logic directly

**Morse Audio Module (morse_audio.cpp/h - 263 lines)**
- All Morse code playback functions
- Audio feedback tones (beeps, errors, countdowns)
- Keyer type announcements in Morse
- Initialized via `initMorseAudio(&adapter, PIEZO_PIN)`

**Settings/EEPROM Module (settings_eeprom.cpp/h - 208 lines)**
- All EEPROM read/write operations
- Adapter settings persistence
- CW memory slot storage
- Centralized memory address management

**Menu Handler Module (menu_handler.cpp/h - 752 lines)**
- Complete button menu state machine
- All operating mode handlers
- Quick press, long press, combo, and double-click logic
- Timeout management for setting modes
- Initialized via `initMenuHandler(&adapter, memorySlots, &recordingState, &playbackState)`

**Double-Click Recording Priority:**
In `updateMenuHandler()`, double-click detection is checked BEFORE quick press handling to prevent conflicts. When recording to a memory slot, any ongoing playback is explicitly stopped before starting the recording. This prevents the bug where double-clicking to re-record a slot would trigger playback (from the first click) before recording (from the double-click detection).

### Benefits of This Structure

1. **Improved Maintainability** - Each module has a clear, focused responsibility
2. **Easier Testing** - Modules can be tested independently
3. **Better Readability** - 77% reduction in main sketch size (1,180 → 270 lines)
4. **Cleaner Dependencies** - Modules are initialized with only what they need
5. **Easier Debugging** - Logic is organized by function, not scattered through one large file

### When Adding New Features

- **Audio feedback?** → Add to `morse_audio.cpp`
- **New setting to persist?** → Add EEPROM functions to `settings_eeprom.cpp`
- **New menu mode or button behavior?** → Add handlers to `menu_handler.cpp`
- **Core keyer/adapter logic?** → Modify `adapter.cpp` or `keyers.cpp`
- **Main loop coordination?** → Modify `vail-adapter.ino` (but keep it minimal!)
