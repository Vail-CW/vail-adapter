#!/bin/bash

# Build script for all Vail Adapter hardware configurations
# Generates 8 UF2 files (4 hardware configs × 2 boards)

set -e  # Exit on error

echo "=========================================="
echo "Vail Adapter Firmware Builder"
echo "=========================================="
echo ""

# Configuration arrays
BOARDS=("Seeeduino:samd:seeed_XIAO_m0" "adafruit:samd:adafruit_qtpy_m0")
BOARD_NAMES=("xiao" "qtpy")
HW_CONFIGS=("V1_Basic_PCB" "V2_Basic_PCB" "Advanced_PCB" "NO_PCB_GITHUB_SPECS")
HW_NAMES=("basic_pcb_v1" "basic_pcb_v2" "advanced_pcb" "non_pcb")

# UF2 conversion parameters
UF2_FAMILY_ID="0x68ED2B88"
UF2_BASE_ADDR="0x2000"

# Create output directory
mkdir -p firmware_uf2_files
rm -f firmware_uf2_files/*.uf2

# Function to set hardware define in config.h
set_hardware_define() {
    local define=$1
    echo "  Setting hardware define to: $define"

    # Comment out all defines first
    sed -i.bak -E 's/^(\s*#define\s+(V1_Basic_PCB|V2_Basic_PCB|Advanced_PCB|NO_PCB_GITHUB_SPECS))/\/\/ #define \2/' config.h

    # Uncomment the target define
    sed -i -E "s|^(\s*)//\s*#define\s+${define}|\1#define ${define}|" config.h
}

# Build counter
BUILD_COUNT=0
TOTAL_BUILDS=$((${#BOARDS[@]} * ${#HW_CONFIGS[@]}))

# Build all configurations
for board_idx in "${!BOARDS[@]}"; do
    FQBN="${BOARDS[$board_idx]}"
    BOARD_NAME="${BOARD_NAMES[$board_idx]}"

    for hw_idx in "${!HW_CONFIGS[@]}"; do
        HW_CONFIG="${HW_CONFIGS[$hw_idx]}"
        HW_NAME="${HW_NAMES[$hw_idx]}"
        UF2_NAME="${BOARD_NAME}_${HW_NAME}.uf2"

        BUILD_COUNT=$((BUILD_COUNT + 1))

        echo ""
        echo "=========================================="
        echo "Build $BUILD_COUNT of $TOTAL_BUILDS"
        echo "Board: $BOARD_NAME"
        echo "Hardware: $HW_CONFIG"
        echo "Output: $UF2_NAME"
        echo "=========================================="

        # Set hardware configuration
        set_hardware_define "$HW_CONFIG"

        # Clean previous build
        rm -rf build_output

        # Compile
        echo "  Compiling..."
        if arduino-cli compile --fqbn "$FQBN" --output-dir build_output --export-binaries . 2>&1 | tee compile.log | grep -E "(Sketch uses|Global variables use)"; then
            echo "  Compilation successful"
        else
            echo "  Checking compile.log for errors..."
            if grep -qi error compile.log; then
                echo "  ERROR: Compilation failed!"
                cat compile.log
                exit 1
            fi
        fi

        # Find the .bin file
        BIN_FILE=$(find build_output -name "*.bin" 2>/dev/null | head -n 1)
        if [ -z "$BIN_FILE" ]; then
            echo "  ERROR: No .bin file found!"
            ls -la build_output/ || echo "build_output directory not found"
            exit 1
        fi

        echo "  Found binary: $BIN_FILE"

        # Convert to UF2
        echo "  Converting to UF2..."
        if [ ! -f "uf2conv.py" ]; then
            echo "  ERROR: uf2conv.py not found!"
            exit 1
        fi
        /c/Python313/python uf2conv.py -c -f "$UF2_FAMILY_ID" -b "$UF2_BASE_ADDR" "$BIN_FILE" -o "firmware_uf2_files/$UF2_NAME"

        if [ -f "firmware_uf2_files/$UF2_NAME" ]; then
            SIZE=$(ls -lh "firmware_uf2_files/$UF2_NAME" | awk '{print $5}')
            echo "  ✓ Success! Size: $SIZE"
        else
            echo "  ERROR: UF2 conversion failed!"
            exit 1
        fi
    done
done

# Restore config.h backup
if [ -f config.h.bak ]; then
    mv config.h.bak config.h
    echo ""
    echo "Restored original config.h"
fi

# Clean up
rm -f compile.log

echo ""
echo "=========================================="
echo "Build Complete!"
echo "=========================================="
echo "Generated $BUILD_COUNT UF2 files in firmware_uf2_files/"
echo ""
ls -lh firmware_uf2_files/*.uf2
echo ""
echo "Done!"