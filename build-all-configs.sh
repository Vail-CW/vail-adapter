#!/bin/bash

# Build all hardware configurations for vail-adapter using arduino-cli
# This script generates UF2 files for all hardware permutations

set -e  # Exit on any error

# Hardware configurations to build
CONFIGS=("V1_PCB" "V1_2_PCB" "V2_ADVANCED_PCB" "NO_PCB_GITHUB_SPECS")

# Board FQBNs (Fully Qualified Board Names)
declare -A BOARDS
BOARDS[qtpy]="adafruit:samd:adafruit_qtpy_m0"
BOARDS[xiao]="Seeeduino:samd:seeed_XIAO_m0"

# Create build directory if it doesn't exist
mkdir -p build

# Function to modify config.h for a specific hardware configuration
modify_config() {
    local config=$1
    echo "Configuring for $config..."

    # Create a backup of the original config.h
    cp config.h config.h.bak

    # Comment out all configurations first (handle both uncommented and commented lines)
    sed -i 's/^#define V1_PCB/\/\/ #define V1_PCB/' config.h
    sed -i 's/^#define V1_2_PCB/\/\/ #define V1_2_PCB/' config.h
    sed -i 's/^#define V2_ADVANCED_PCB/\/\/ #define V2_ADVANCED_PCB/' config.h
    sed -i 's/^#define NO_PCB_GITHUB_SPECS/\/\/ #define NO_PCB_GITHUB_SPECS/' config.h

    # Uncomment the specific configuration
    sed -i "s/^\/\/ #define ${config}/#define ${config}/" config.h

    echo "Configuration set to: $config"
    grep "#define.*PCB" config.h | head -4
}

# Function to restore original config.h
restore_config() {
    if [ -f config.h.bak ]; then
        mv config.h.bak config.h
        echo "Restored original config.h"
    fi
}

# Function to convert bin to uf2
bin_to_uf2() {
    local bin_file=$1
    local uf2_file=$2

    # Download uf2conv.py if it doesn't exist
    if [ ! -f build/uf2conv.py ]; then
        echo "Downloading uf2conv.py..."
        mkdir -p build
        curl -L https://raw.githubusercontent.com/microsoft/uf2/master/utils/uf2conv.py > build/uf2conv.py
        curl -L https://raw.githubusercontent.com/microsoft/uf2/master/utils/uf2families.json > build/uf2families.json
        chmod +x build/uf2conv.py
    fi

    # Convert bin to uf2 (offset 0x2000 for SAMD21)
    /c/Python313/python build/uf2conv.py -b 0x2000 -c -o "$uf2_file" "$bin_file"
}

# Trap to ensure config is restored on script exit
trap restore_config EXIT

echo "Building UF2 files for all hardware configurations..."
echo "Configurations: ${CONFIGS[*]}"
echo "Board types: ${!BOARDS[*]}"
echo ""

# Build for each configuration
for config in "${CONFIGS[@]}"; do
    echo "========================================="
    echo "Building for configuration: $config"
    echo "========================================="

    # Modify config.h for this hardware configuration
    modify_config "$config"

    # Build for each board type
    for board_name in "${!BOARDS[@]}"; do
        fqbn="${BOARDS[$board_name]}"
        echo "Building $config for $board_name board ($fqbn)..."

        # Clean previous build files
        rm -f build/vail-adapter.ino.*

        # Compile the sketch directly to build directory
        arduino-cli compile --fqbn "$fqbn" --output-dir "build" vail-adapter.ino

        # Check if the binary file was created
        bin_file="build/vail-adapter.ino.bin"
        if [ -f "$bin_file" ] && [ -s "$bin_file" ]; then
            # Generate UF2 file with descriptive name (underscores, period only before extension)
            # Use more intuitive naming for PCB configs
            config_name="$config"
            if [ "$config" = "V1_PCB" ]; then
                config_name="BASIC_PCB_V1"
            elif [ "$config" = "V1_2_PCB" ]; then
                config_name="BASIC_PCB_V2"
            elif [ "$config" = "V2_ADVANCED_PCB" ]; then
                config_name="ADVANCED_PCB"
            fi

            uf2_file="build/vail-adapter_${config_name}_${board_name}.uf2"
            echo "Converting $bin_file to $uf2_file"
            bin_to_uf2 "$bin_file" "$uf2_file"
            echo "✓ Generated: $uf2_file"
        else
            echo "✗ Error: Binary file $bin_file not found or empty"
        fi
    done

    echo ""
done

echo "========================================="
echo "Build Summary"
echo "========================================="
echo "Generated UF2 files:"
ls -la build/*.uf2 2>/dev/null || echo "No UF2 files found"

echo ""
echo "All builds completed!"
echo ""
echo "Usage instructions:"
echo "- For QT Py boards: Use files ending with _qtpy.uf2"
echo "- For Xiao boards: Use files ending with _xiao.uf2"
echo "- File naming: vail-adapter_[CONFIG]_[BOARD].uf2"
echo "- Hardware configurations:"
echo "  * V1_PCB: Original V1 PCB design"
echo "  * V1_2_PCB: Basic PCB V2 (recommended for new builds)"
echo "  * V2_ADVANCED_PCB: Advanced PCB with radio output"
echo "  * NO_PCB_GITHUB_SPECS: Breadboard/no PCB configuration"