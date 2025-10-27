# Vail Summit Integration

This document explains how the Vail Summit device has been integrated into the existing vail-adapter GitHub Pages updater tool.

## Overview

The updater tool now supports two devices:
- **Vail Adapter** - SAMD21-based USB adapter (QT Py or XIAO) using UF2 firmware
- **Vail Summit** - ESP32-S3 standalone morse trainer using BIN firmware

## Architecture

### Repository Structure

```
vail-adapter/
├── master branch (main codebase)
│   ├── Vail Adapter source code
│   ├── docs/ (GitHub Pages site)
│   │   ├── index.html (unified updater)
│   │   ├── script.js (adapter flow logic)
│   │   ├── esp-flasher.js (ESP32 flasher)
│   │   ├── style.css (shared styles)
│   │   └── firmware_files/
│   │       ├── [adapter .uf2 files]
│   │       └── summit/
│   │           ├── bootloader.bin
│   │           ├── partitions.bin
│   │           └── vail-summit.bin
│   └── ...
│
└── vail-summit branch
    └── Vail Summit ESP32-S3 source code
```

### Workflow

1. **Development**: Work on Summit firmware in the `vail-summit` branch
2. **Build**: CI/CD builds the firmware and generates `.bin` files
3. **Deploy**: CI commits the binaries to `master` branch at `docs/firmware_files/summit/`
4. **Update**: GitHub Pages site on `master` can immediately reference the new binaries

## User Flow

### Vail Adapter Flow (UF2)
1. Select "Vail Adapter"
2. Choose model (Basic PCB / Advanced PCB / DIY)
3. Choose board (QT Py / XIAO)
4. Enter bootloader mode (WebSerial or manual reset)
5. Download UF2 file
6. Drag-and-drop to device

### Vail Summit Flow (ESP32 Web Flasher)
1. Select "Vail Summit"
2. Connect via WebSerial
3. Click "Flash Firmware"
4. Wait for automatic flashing process
5. Device reboots automatically

## Technical Implementation

### ESP32 Web Flasher

The Summit updater uses **esptool-js** for web-based ESP32 flashing:

- **Library**: `esptool-js@0.4.1` loaded via CDN
- **Transport**: Web Serial API (Chrome/Edge/Opera only)
- **Process**:
  1. Connect to serial port at 115200 baud
  2. Initialize ESPLoader
  3. Download firmware files from GitHub
  4. Flash bootloader (0x0), partitions (0x8000), and app (0x10000)
  5. Hard reset device

### Files

- **[docs/index.html](docs/index.html)** - Updated with new device selection step and Summit flash UI
- **[docs/script.js](docs/script.js)** - Updated wizard flow to handle device branching
- **[docs/esp-flasher.js](docs/esp-flasher.js)** - NEW: ESP32 flasher implementation
- **[docs/style.css](docs/style.css)** - Added `.info-box` styling for Summit instructions
- **[docs/firmware_files/summit/](docs/firmware_files/summit/)** - Firmware binary storage

## Setting Up Summit Firmware

### Option 1: Manual Upload
1. Build your Summit firmware on the `vail-summit` branch
2. Copy the generated files to `docs/firmware_files/summit/`:
   - `bootloader.bin`
   - `partitions.bin`
   - `vail-summit.bin`
3. Commit and push to `master`

### Option 2: CI/CD Automation (Recommended)

Create a GitHub Actions workflow on the `vail-summit` branch:

```yaml
name: Build and Deploy Summit Firmware

on:
  push:
    branches: [vail-summit]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Setup Arduino CLI / PlatformIO
        # ... setup build environment

      - name: Build firmware
        # ... build steps

      - name: Checkout master
        run: |
          git fetch origin master
          git checkout master

      - name: Copy binaries
        run: |
          cp build/bootloader.bin docs/firmware_files/summit/
          cp build/partitions.bin docs/firmware_files/summit/
          cp build/vail-summit.bin docs/firmware_files/summit/

      - name: Commit and push
        run: |
          git config user.name "GitHub Actions"
          git config user.email "actions@github.com"
          git add docs/firmware_files/summit/*.bin
          git commit -m "Update Summit firmware [skip ci]"
          git push origin master
```

## ESP32-S3 Build Configuration

When building for ESP32-S3 Feather, ensure your partition table and bootloader are configured correctly:

```ini
# platformio.ini example
[env:feather_esp32s3]
platform = espressif32
board = adafruit_feather_esp32s3
framework = arduino

board_build.partitions = default.csv
board_build.flash_mode = qio
board_upload.flash_size = 8MB
```

## Browser Compatibility

The ESP32 web flasher requires Web Serial API:
- ✅ Chrome
- ✅ Edge
- ✅ Opera
- ❌ Firefox (not supported)
- ❌ Safari (not supported)

## Future Enhancements

- [ ] Firmware version detection and display
- [ ] Over-the-air (OTA) updates for Summit
- [ ] WiFi configuration via web interface
- [ ] Automatic firmware file validation (checksums)
- [ ] Support for custom partition schemes

## Testing the Integration

1. Open the GitHub Pages site locally or at `https://vailadapter.com`
2. Select "Vail Summit"
3. The ESP32 flasher interface should appear
4. (With actual hardware) Connect device and test flashing

## Questions?

For issues or questions about the Summit integration, please open an issue on GitHub.
