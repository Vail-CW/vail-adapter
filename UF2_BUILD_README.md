# UF2 Build Instructions for Vail Adapter

This document explains how to build UF2 firmware files for all hardware configurations of the Vail Adapter project.

## Overview

The Vail Adapter supports 4 different hardware configurations defined in `config.h`:
- `BASIC_PCB_V1` - Original V1 PCB design
- `BASIC_PCB_V2` - Basic PCB V2 (recommended for new builds)
- `ADVANCED_PCB` - Advanced PCB with radio output capabilities
- `NO_PCB_GITHUB_SPECS` - Breadboard/no PCB configuration

Each configuration can be built for 2 different microcontroller boards:
- **QT Py** (Adafruit QT Py M0) - FQBN: `adafruit:samd:adafruit_qtpy_m0`
- **Xiao** (Seeeduino Xiao) - FQBN: `Seeeduino:samd:seeed_XIAO_m0`

## Prerequisites

1. **Arduino CLI** - Must be installed and available in PATH
2. **Required Arduino Cores**:
   - `adafruit:samd` (for QT Py boards)
   - `Seeeduino:samd` (for Xiao boards)
3. **Python 3** - Required for UF2 conversion (uf2conv.py)
4. **Internet connection** - For downloading uf2conv.py on first run

## Quick Build - All Configurations

To build UF2 files for all hardware configurations and both board types:

```bash
./build-all-configs.sh
```

This script will:
1. Download uf2conv.py utility if needed
2. For each hardware configuration:
   - Modify `config.h` to enable the specific configuration
   - Compile for both QT Py and Xiao boards
   - Convert .bin files to .uf2 format
   - Create descriptively named output files
3. Restore original `config.h`
4. Display summary of generated files

## Output Files

Files are generated in the `build/` directory with this naming convention:
```
vail-adapter_[CONFIG]_[BOARD].uf2
```

For example:
- `vail-adapter_BASIC_PCB_V1_qtpy.uf2`
- `vail-adapter_BASIC_PCB_V2_qtpy.uf2`
- `vail-adapter_ADVANCED_PCB_xiao.uf2`

## Manual Build Process

If you need to build a specific configuration manually:

### 1. Modify config.h
Edit `config.h` to uncomment only the desired configuration:
```c
#define BASIC_PCB_V1          // Enable this configuration
// #define BASIC_PCB_V2       // Basic PCB V2 (recommended for new builds)
// #define ADVANCED_PCB       // Advanced PCB with radio output
// #define NO_PCB_GITHUB_SPECS
```

### 2. Compile with Arduino CLI
```bash
# For QT Py
arduino-cli compile --fqbn adafruit:samd:adafruit_qtpy_m0 --output-dir build vail-adapter.ino

# For Xiao
arduino-cli compile --fqbn Seeeduino:samd:seeed_XIAO_m0 --output-dir build vail-adapter.ino
```

### 3. Convert to UF2
```bash
# Download uf2conv.py if needed
curl -L https://raw.githubusercontent.com/microsoft/uf2/master/utils/uf2conv.py > build/uf2conv.py
curl -L https://raw.githubusercontent.com/microsoft/uf2/master/utils/uf2families.json > build/uf2families.json

# Convert (adjust Python path as needed)
python build/uf2conv.py -b 0x2000 -c -o output.uf2 build/vail-adapter.ino.bin
```

## Installing Arduino Cores

If cores are not installed:

```bash
# Add board package URLs
arduino-cli core update-index --additional-urls https://adafruit.github.io/arduino-board-index/package_adafruit_index.json,https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json

# Install cores
arduino-cli core install adafruit:samd
arduino-cli core install Seeeduino:samd
```

## Troubleshooting

### Common Issues

1. **"arduino-cli: command not found"**
   - Install Arduino CLI from https://arduino.github.io/arduino-cli/

2. **"Error resolving FQBN"**
   - Install required cores (see above)
   - Update board index: `arduino-cli core update-index`

3. **"Python not found"**
   - Install Python 3 or adjust Python path in build script

4. **Empty .bin files**
   - Don't use `--build-path` with arduino-cli
   - Use `--output-dir` only

5. **Permission errors**
   - Make build script executable: `chmod +x build-all-configs.sh`

### File Locations

- **Build script**: `build-all-configs.sh`
- **Configuration**: `config.h`
- **Output directory**: `build/`
- **Main sketch**: `vail-adapter.ino`

## Notes

- The build script automatically backs up and restores `config.h`
- UF2 files use 0x2000 offset for SAMD21 microcontrollers
- V2_ADVANCED_PCB builds are slightly larger due to radio output code
- Compilation warnings about USB endpoints are normal and can be ignored

## File Naming Convention

Output files follow this pattern:
```
vail-adapter_[HARDWARE_CONFIG]_[BOARD_TYPE].uf2
```

Where:
- `HARDWARE_CONFIG`: BASIC_PCB_V1, BASIC_PCB_V2, ADVANCED_PCB, NO_PCB_GITHUB_SPECS
- `BOARD_TYPE`: qtpy, xiao
- All components separated by underscores
- Only one period before the file extension

Note: The config define names now match the output file names directly for consistency.