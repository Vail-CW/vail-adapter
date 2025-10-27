# Vail Summit Firmware Files

This directory contains the firmware binaries for the Vail Summit device (ESP32-S3 based).

## Files

- `vail-summit.bin` - Main firmware binary
- `bootloader.bin` - ESP32-S3 bootloader
- `partitions.bin` - Partition table

## Flashing

These files are automatically flashed via the web updater at https://update.vailadapter.com using esptool-js.

## Build Process

The firmware is built from the `vail-summit` branch and the binaries are committed to master for easy access by the GitHub Pages updater tool.
