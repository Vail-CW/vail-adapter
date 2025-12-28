# Building and Development

This document covers build setup, compilation, upload, and firmware updates for the VAIL SUMMIT morse code trainer.

## Arduino IDE Setup

### Board Configuration

Select **Adafruit Feather ESP32-S3 2MB PSRAM** from the boards menu, then configure:

| Setting | Value | Notes |
|---------|-------|-------|
| **Board** | Adafruit Feather ESP32-S3 2MB PSRAM | Not "No PSRAM" variant |
| **USB CDC On Boot** | Enabled | Required for serial output |
| **USB Mode** | Hardware CDC and JTAG | **NOT TinyUSB** - TinyUSB causes boot failures |
| **Upload Mode** | UART0 / Hardware CDC | |
| **Flash Size** | 4MB (32Mb) | |
| **PSRAM** | QSPI PSRAM | **NOT OPI** - OPI causes PSRAM detection failures |
| **Partition Scheme** | Huge APP (3MB No OTA/1MB SPIFFS) | Provides 3MB app space + 1MB storage |
| **Upload Speed** | 921600 | |

**⚠️ Critical Settings:**
- **USB Mode must be "Hardware CDC and JTAG"** - Using "USB-OTG (TinyUSB)" will cause the device to crash before any serial output appears
- **PSRAM must be "QSPI PSRAM"** - The Adafruit Feather ESP32-S3 2MB PSRAM uses QSPI, not OPI. Using OPI will cause PSRAM detection to fail with error: `E (360) opi psram: PSRAM ID read error`

### ESP32 Core Version

**IMPORTANT:** This project requires **Arduino ESP32 core version 2.0.14** for display compatibility (especially for 4" ST7796S displays).

**To install/verify in Arduino IDE:**
1. Go to Tools → Board → Boards Manager
2. Search for "esp32"
3. Find "esp32 by Espressif Systems"
4. Select version **2.0.14** from dropdown
5. Click "Install" or verify current version

**Note:** Newer core versions (2.0.15+, 3.0.x) have compatibility issues with the ST7796S display driver that cause boot loops. Always use 2.0.14 for this project.

### Required Libraries

Install via Arduino Library Manager:

1. **LovyanGFX by lovyan03** - Advanced display graphics library for ST7796S 4.0" display
2. **lvgl v8.3.x** - UI framework (**must be v8.3.x, NOT v9.x** - API incompatible)
3. **Adafruit MAX1704X** - Battery monitoring for MAX17048
4. **Adafruit LC709203F** - Backup battery monitor support
5. **WebSockets by Markus Sattler** - WebSocket client for Vail repeater
6. **ArduinoJson by Benoit Blanchon** - JSON parsing/serialization
7. **NimBLE-Arduino** - Bluetooth Low Energy for BLE HID/MIDI

**Install from GitHub** (not in Library Manager):
- **ESPAsyncWebServer** - `https://github.com/me-no-dev/ESPAsyncWebServer`
- **AsyncTCP** - `https://github.com/me-no-dev/AsyncTCP` (dependency for ESPAsyncWebServer)

**⚠️ LVGL Version:** This project requires LVGL 8.3.x. LVGL 9.x has breaking API changes and will not compile. If using Arduino Library Manager, ensure you install version 8.3.11 or similar 8.3.x version.

**Note:** This project uses a 4.0" ST7796S display (480×320) with LovyanGFX library instead of the older Adafruit ST7789 library.

### Compilation and Upload

**Arduino IDE:**
```bash
# Open the main sketch in Arduino IDE
arduino morse_trainer/morse_trainer_menu.ino
```

**Arduino CLI:**
```bash
# Compile with required board options
arduino-cli compile \
  --fqbn "esp32:esp32:adafruit_feather_esp32s3:CDCOnBoot=cdc,PartitionScheme=huge_app,PSRAM=enabled,FlashSize=4M" \
  vail-summit/

# Upload (replace COM<X> with your port)
arduino-cli upload -p COM<X> \
  --fqbn "esp32:esp32:adafruit_feather_esp32s3:CDCOnBoot=cdc,PartitionScheme=huge_app,PSRAM=enabled,FlashSize=4M" \
  vail-summit/
```

**FQBN Options Explained:**
- `CDCOnBoot=cdc` - Enable USB CDC for serial output
- `PartitionScheme=huge_app` - 3MB app space, 1MB SPIFFS
- `PSRAM=enabled` - Enable PSRAM (auto-detects QSPI/OPI)
- `FlashSize=4M` - 4MB flash

### Serial Monitor

```bash
# Connect at 115200 baud for debug output
arduino-cli monitor -p COM<X> --config baudrate=115200
```

Debug output includes:
- I2C device detection (CardKB, battery monitor)
- WiFi connection status
- File system operations
- QSO logging activity
- Web server lifecycle events

## Firmware Version Management

### Version Information Location

`config.h` (lines 9-14):

```cpp
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_DATE "2025-01-30"  // Update this date each time you build new firmware
#define FIRMWARE_NAME "VAIL SUMMIT"
```

### When to Update

**FIRMWARE_VERSION:** Increment for major releases
- **Major version** (1.x.x → 2.x.x): Breaking changes or major new features
- **Minor version** (x.1.x → x.2.x): New features, backward compatible
- **Patch version** (x.x.1 → x.x.2): Bug fixes only

**FIRMWARE_DATE:** Update to current date (YYYY-MM-DD) **every time** you build firmware for distribution

**FIRMWARE_NAME:** Should remain "VAIL SUMMIT" unless device name changes

### Where Version Appears

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

**Deployment Workflow:**
1. Code is developed and tested on the `vail-summit` branch
2. Firmware is compiled using GitHub Actions workflow (recommended) or Arduino CLI manually
3. Binary files (`bootloader.bin`, `partitions.bin`, `vail-summit.bin`) are committed to `master` branch at `docs/firmware_files/summit/`
4. Users can flash firmware via web updater at `https://update.vailadapter.com`

### Building Firmware for Distribution

#### Option 1: GitHub Actions Workflow (Recommended)

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

#### Option 2: Manual Arduino CLI Build Process

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

The web updater at `https://update.vailadapter.com` uses **esptool-js** for browser-based flashing.

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

## Troubleshooting

### Device won't boot / No serial output

**Symptom:** Device flashes successfully but crashes immediately with no serial output. COM port disappears until using physical boot/reset buttons.

**Cause:** Wrong USB Mode setting.

**Fix:** In Arduino IDE, change Tools → USB Mode to **"Hardware CDC and JTAG"** (not "USB-OTG (TinyUSB)").

### PSRAM detection failure

**Symptom:** Serial output shows `E (360) opi psram: PSRAM ID read error: 0x00000000` and device fails to boot properly.

**Cause:** Wrong PSRAM type selected.

**Fix:** In Arduino IDE, change Tools → PSRAM to **"QSPI PSRAM"** (not "OPI PSRAM"). The Adafruit Feather ESP32-S3 2MB PSRAM board uses QSPI PSRAM.

### LVGL display buffer allocation failed

**Symptom:** Serial output shows `[LVGL] ERROR: Failed to allocate display buffers!`

**Cause:** PSRAM not properly initialized, usually due to wrong PSRAM setting in build options.

**Fix:** Ensure firmware is compiled with correct PSRAM setting. For GitHub Actions builds, the workflow uses `PSRAM=enabled`. For Arduino IDE, use "QSPI PSRAM".

### Build errors with LVGL 9.x

**Symptom:** Compilation fails with errors like `'getLVGLInputGroup' was not declared`, `lv_msgbox_create` wrong number of arguments, or `LV_INDEV_STATE_PRESSED` not found.

**Cause:** LVGL 9.x installed instead of 8.3.x.

**Fix:** Uninstall LVGL 9.x and install LVGL 8.3.11 (or any 8.3.x version). The APIs changed significantly between v8 and v9.

### Display shows wrong colors or garbled output

**Symptom:** Display works but colors are inverted or image is corrupted.

**Cause:** Wrong ESP32 core version or display configuration.

**Fix:** Ensure Arduino ESP32 core version 2.0.14 is installed. Newer versions (2.0.15+, 3.x) have compatibility issues with the ST7796S display.
