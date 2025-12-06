# Storage Module

SD card storage management for VAIL SUMMIT.

## Overview

The storage module provides SD card file management capabilities through a web interface. The SD card shares the SPI bus with the display and uses lazy initialization to avoid boot-time conflicts.

## Files

- **`sd_card.h`** - Core SD card functionality (initialization, file operations, storage stats)

## Hardware Configuration

**GPIO Pin:** 38 (SD_CS - Chip Select)
**Shared SPI Bus:** MOSI=35, SCK=36, MISO=37 (same as display)
**Display CS:** GPIO 10 (separate chip select allows bus sharing)

## SD Card Requirements

- **Format:** FAT32 (required - exFAT not supported)
- **Recommended Size:** 4-32 GB SDHC
- **Speed Class:** Class 10 or higher
- **Important:** Cards >32 GB must be reformatted to FAT32

## Initialization Strategy

The SD card uses **lazy initialization** (on-demand):

1. **Boot Time:** SD card is NOT initialized (prevents SPI conflicts with display)
2. **First Access:** When user accesses `/storage` page, SD card initializes automatically
3. **SPI Sharing:** LovyanGFX display is configured with `bus_shared=true` to allow coexistence

### Why Lazy Init?

Initializing the SD card during boot can interfere with the display's SPI configuration, causing a blank screen. By deferring initialization until first use, we ensure:
- Display always boots correctly
- SPI bus is properly shared between devices
- No user impact (SD init happens transparently on first page access)

## API Functions

### `bool initSDCard()`
Initialize SD card and detect card type. Returns `true` if successful, `false` if no card or init failed.

### `void updateSDCardStats()`
Update global variables for card size and usage statistics.

### `String listSDFiles(const char* dirname, bool recursive = false, int depth = 0)`
List files in directory, returns JSON array of file objects.

### `bool deleteSDFile(const char* path)`
Delete file at specified path.

### `bool fileExists(const char* path)`
Check if file exists.

### `size_t getFileSize(const char* path)`
Get file size in bytes.

### `String readSDFile(const char* path)`
Read entire file contents (for small files).

### `bool writeSDFile(const char* path, const char* data)`
Write data to file (overwrites existing).

### `bool appendSDFile(const char* path, const char* data)`
Append data to existing file.

## Web Interface Integration

The storage module integrates with the web server via:

- **`web_api_storage.h`** - REST API endpoints for file operations
- **`web_pages_storage.h`** - HTML/CSS/JS storage management page

### REST API Endpoints

- `GET /api/storage/status` - Get SD card statistics
- `GET /api/storage/files?path=/` - List files in directory
- `GET /api/storage/download?file=/filename` - Download file
- `DELETE /api/storage/delete?file=/filename` - Delete file
- `POST /api/storage/upload` - Upload file (multipart/form-data)

### Web Page

Access at: `http://vail-summit.local/storage`

Features:
- Storage usage graph
- File browser with icons and sizes
- Upload files via file picker
- Download and delete buttons for each file

## Usage Examples

### Initialize SD Card
```cpp
if (initSDCard()) {
  Serial.println("SD card ready!");
  Serial.printf("Size: %llu MB\n", sdCardSize);
}
```

### Write File
```cpp
writeSDFile("/data.txt", "Hello from VAIL SUMMIT!");
```

### Read File
```cpp
String contents = readSDFile("/data.txt");
Serial.println(contents);
```

### List Files
```cpp
String fileList = listSDFiles("/");
Serial.println(fileList);  // Prints JSON array
```

## Future Use Cases

This SD card infrastructure enables:

1. **Data Logging** - Log training sessions, practice statistics, WPM progress
2. **QSO Backups** - Export QSO logs to SD card for offline backup
3. **Configuration Backups** - Save/restore device settings to SD card
4. **Firmware Updates** - Store firmware binaries for OTA updates
5. **Custom Content** - Load callsign lists, practice text, custom character sets

## Troubleshooting

**Display is blank after boot:**
- Ensure SD card init is NOT called during boot sequence
- SD card should only initialize on first web access

**"SD card not available" error:**
- Check card is inserted
- Verify card is formatted as FAT32 (not exFAT)
- Check wiring: CS=38, MOSI=35, SCK=36, MISO=37

**Cannot upload large files:**
- FAT32 has 4 GB file size limit
- Check available free space on card

**Files not showing in web interface:**
- Ensure card is FAT32 formatted
- Check files are in root directory (/)
- Refresh the page to trigger re-initialization

## Technical Notes

- SD library uses 4 MHz SPI clock (safe speed for compatibility)
- Mount point: `/sd`
- Max open files: 5 (default SD library limit)
- File paths must start with `/` (e.g., `/data.txt`)
