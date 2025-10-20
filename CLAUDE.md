# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Vail Adapter is an Arduino-based firmware for Morse code key/paddle to USB conversion. It runs on SAMD21-based microcontrollers (Seeeduino XIAO SAMD21 and Adafruit QT Py SAMD21) and provides:
- USB HID keyboard output (Ctrl keys for dit/dah)
- USB MIDI control and output for DAW integration
- Multiple keyer modes (straight key, bug, iambic A/B, ultimatic, etc.)
- Sidetone generation via piezo buzzer
- Capacitive touch support
- Optional radio output for direct keying of amateur radios

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

**Convert .bin to .uf2 (for drag-and-drop flashing):**
```bash
python3 uf2conv.py -c -f 0x68ED2B88 -b 0x2000 build_output/*.bin -o firmware.uf2
```

### Hardware Configuration

Before building, you MUST edit `config.h` and uncomment exactly ONE hardware configuration:
- `V1_PCB` - Original PCB version
- `V1_2_PCB` - Revised PCB with updated pin mappings
- `V2_ADVANCED_PCB` - Advanced PCB with radio output pins (A2/A3)
- `NO_PCB_GITHUB_SPECS` - Breadboard/hand-wired setup

Each configuration sets different pin mappings for dit/dah/key inputs, piezo, and optional radio outputs.

For V2_ADVANCED_PCB builds, also configure `RADIO_KEYING_ACTIVE_LOW` in config.h based on your radio's keying polarity.

## Architecture

### Core Components

**Main Loop (vail-adapter.ino:199-245)**
- Polls MIDI input and dispatches to VailAdapter
- Updates bounce debouncers for physical inputs (dit, dah, key)
- Updates capacitive touch inputs (qt_dit, qt_dah, qt_key)
- Handles special LED states for radio mode and buzzer disable
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
- `Bounce` class (bounce2.cpp/h) - Software debouncing for physical switches
- `TouchBounce` class (touchbounce.cpp/h) - Debouncing for Adafruit FreeTouch capacitive sensors

**Audio Output**
- `PolyBuzzer` class (polybuzzer.cpp/h) - Manages piezo buzzer tone generation using Arduino `tone()` function
- `equalTemperament.h` - Lookup table for MIDI note numbers to Hz frequencies

### MIDI Integration

See `MIDI_INTEGRATION_SPEC.md` for full protocol details. Key points:
- **CC0** switches between keyboard and MIDI modes
- **CC1** sets dit duration (value × 2 milliseconds)
- **CC2** sets sidetone MIDI note number
- **PC** selects keyer mode (0-9)
- In MIDI mode with passthrough keyer (mode 0), outputs Note On/Off for C (straight), C# (dit), D (dah)

### Special Feature Activation Sequences

These are "Easter egg" features activated by specific input patterns:

1. **Buzzer Disable**: Hold DIT for 5 seconds → toggles buzzer on/off (LED blinks slowly when disabled)
2. **Radio Mode**: Press DAH 10 times within 500ms → toggles radio mode (LED blinks rapidly when active)
3. **Radio Keyer Mode**: While in radio mode, hold DAH for 5 seconds → toggles radio keyer mode (direct radio keying vs. pass-through)

Radio mode and radio keyer mode are independent concepts:
- Radio mode enables radio output pins
- Radio keyer mode determines whether the keyer logic directly drives the radio pins (true) or just passes through paddle states (false)

### EEPROM Storage

Settings persisted across power cycles (vail-adapter.ino:64-131):
- Keyer type (address 0)
- Dit duration (address 1-2, uint16_t)
- TX note (address 3)
- Radio keyer mode (address 5)
- Valid flag (address 4, value 0x42)

## Testing

No automated test suite exists. Manual testing procedures are documented in `docs/TESTING_GUIDE.md`.

## CI/CD

GitHub Actions workflow (`.github/workflows/build_uf2.yml`) automatically:
1. Builds firmware for all 8 hardware configurations (4 configs × 2 boards)
2. Converts .bin files to .uf2 format
3. Commits resulting firmware files to `docs/firmware_files/` on every push to master

Firmware naming convention:
- `xiao_basic_pcb_v1.uf2`, `xiao_basic_pcb_v2.uf2`, `xiao_advanced_pcb.uf2`, `xiao_non_pcb.uf2`
- `qtpy_basic_pcb_v1.uf2`, `qtpy_basic_pcb_v2.uf2`, `qtpy_advanced_pcb.uf2`, `qtpy_non_pcb.uf2`

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
