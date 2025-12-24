#!/usr/bin/env python3
"""
PNG to 1-bit Monochrome Bitmap Converter for ESP32
Converts a PNG image to a 1-bit packed bitmap array suitable for embedded displays.

Usage:
    python png_to_bitmap.py <input.png> <output.h> <var_name> <width> <height>

Example:
    python png_to_bitmap.py mountain-logo.png mountain_logo_bitmap.h MOUNTAIN_LOGO 220 160

Requirements:
    pip install Pillow
"""

from PIL import Image
import sys
import os


def png_to_1bit_array(png_path, output_path, var_name, width, height, invert=True):
    """
    Convert PNG to 1-bit monochrome bitmap array for ESP32

    Args:
        png_path: Path to input PNG file
        output_path: Path to output .h file
        var_name: Variable name prefix for the bitmap array
        width: Target width in pixels
        height: Target height in pixels
        invert: If True, invert colors (white becomes 1, black becomes 0)
    """

    print(f"Converting {png_path} to 1-bit bitmap...")
    print(f"Target size: {width}x{height} pixels")
    print(f"Invert colors: {invert}")

    # Load and convert image
    try:
        img = Image.open(png_path).convert('L')  # Convert to grayscale
    except FileNotFoundError:
        print(f"Error: File '{png_path}' not found")
        sys.exit(1)
    except Exception as e:
        print(f"Error loading image: {e}")
        sys.exit(1)

    # Resize image to target dimensions
    original_size = img.size
    print(f"Original size: {original_size[0]}x{original_size[1]}")
    img = img.resize((width, height), Image.LANCZOS)
    print(f"Resized to: {width}x{height}")

    # Get pixel data
    pixels = img.load()

    # Convert to 1-bit array (packed bytes, MSB first)
    bitmap_bytes = []
    bytes_per_row = (width + 7) // 8  # Round up to nearest byte

    for y in range(height):
        for x in range(0, width, 8):
            byte_val = 0
            for bit in range(8):
                if x + bit < width:
                    pixel = pixels[x + bit, y]

                    # Threshold at 128 (middle gray)
                    # If invert=True: white pixels (>128) = 1, black pixels = 0
                    # If invert=False: black pixels (<128) = 1, white pixels = 0
                    if invert:
                        is_set = pixel > 128
                    else:
                        is_set = pixel <= 128

                    if is_set:
                        byte_val |= (1 << (7 - bit))

            bitmap_bytes.append(byte_val)

    total_bytes = len(bitmap_bytes)
    print(f"Generated {total_bytes} bytes of bitmap data")

    # Generate C header file
    try:
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(f"/*\n")
            f.write(f" * Auto-generated bitmap from {os.path.basename(png_path)}\n")
            f.write(f" * Original size: {original_size[0]}x{original_size[1]} pixels\n")
            f.write(f" * Output size: {width}x{height} pixels\n")
            f.write(f" * Format: 1-bit monochrome (MSB first, packed bytes)\n")
            f.write(f" * Memory: {total_bytes} bytes in PROGMEM\n")
            f.write(f" * Inverted: {invert} (white={'1' if invert else '0'}, black={'0' if invert else '1'})\n")
            f.write(f" */\n\n")
            f.write(f"#ifndef {var_name}_BITMAP_H\n")
            f.write(f"#define {var_name}_BITMAP_H\n\n")
            f.write(f"#include <pgmspace.h>\n\n")
            f.write(f"#define {var_name}_WIDTH  {width}\n")
            f.write(f"#define {var_name}_HEIGHT {height}\n\n")
            f.write(f"// 1-bit monochrome bitmap data\n")
            f.write(f"// White pixels = logo visible, Black pixels = transparent background\n")
            f.write(f"const uint8_t {var_name.lower()}Bitmap[] PROGMEM = {{\n")

            # Write bytes in rows of 16 for readability
            for i in range(0, total_bytes, 16):
                row = bitmap_bytes[i:i+16]
                hex_row = ", ".join(f"0x{b:02X}" for b in row)

                # Add comma on all lines except the last
                if i + 16 < total_bytes:
                    f.write(f"  {hex_row},\n")
                else:
                    f.write(f"  {hex_row}\n")

            f.write(f"}};\n\n")
            f.write(f"#endif // {var_name}_BITMAP_H\n")

        print(f"\nSuccess! Generated {output_path}")
        print(f"Include in your code with: #include \"{os.path.basename(output_path)}\"")

    except Exception as e:
        print(f"Error writing output file: {e}")
        sys.exit(1)


def main():
    if len(sys.argv) < 6:
        print("Usage: python png_to_bitmap.py <input.png> <output.h> <var_name> <width> <height> [invert]")
        print("\nExample:")
        print("  python png_to_bitmap.py mountain-logo.png mountain_logo_bitmap.h MOUNTAIN_LOGO 220 160 true")
        print("\nArguments:")
        print("  input.png  - Input PNG file path")
        print("  output.h   - Output header file path")
        print("  var_name   - Variable name prefix (e.g., MOUNTAIN_LOGO)")
        print("  width      - Target width in pixels")
        print("  height     - Target height in pixels")
        print("  invert     - Optional: 'true' to invert colors (default: true)")
        sys.exit(1)

    png_path = sys.argv[1]
    output_path = sys.argv[2]
    var_name = sys.argv[3]
    width = int(sys.argv[4])
    height = int(sys.argv[5])
    invert = True  # Default to inverting

    if len(sys.argv) > 6:
        invert = sys.argv[6].lower() in ['true', '1', 'yes', 'y']

    png_to_1bit_array(png_path, output_path, var_name, width, height, invert)


if __name__ == "__main__":
    main()
