# QSO Logger Mode - Product Specification

## Overview

The QSO Logger mode transforms VAIL SUMMIT into a portable ham radio contact logger with dual interfaces: on-device LCD UI for field logging and web-based interface for full-featured logging and management. The device can operate in WiFi Station mode (connecting to existing network) or WiFi AP mode (creating its own hotspot) to serve the web interface.

## Core Features

### 1. On-Device Logging Interface

**Quick Log Entry Screen:**
- Minimalist field-optimized interface for rapid contact logging
- Essential fields only (expandable via web interface)
- Optimized for CardKB input with shortcuts
- Real-time validation and visual feedback
- Contact counter: "Log #XX" display

**Essential Fields (On-Device):**
1. **Callsign** (required, uppercase, validated format)
2. **Frequency** (MHz, with band auto-detection)
3. **Mode** (CW/SSB/FM/AM/FT8/FT4/RTTY/PSK31)
4. **RST Sent** (default: 599 for CW, 59 for phone modes)
5. **RST Received** (default: 599 for CW, 59 for phone modes)
6. **Date/Time** (auto-filled from system clock, manual override)
7. **Notes** (optional, up to 60 characters)

**Input Shortcuts:**
- `TAB` or `DOWN`: Next field
- `SHIFT+TAB` or `UP`: Previous field
- `ENTER`: Save and clear for next contact
- `ESC`: Cancel and return to menu
- `F1`: Quick fill defaults (current time, default RST, last frequency)
- `F2`: Duplicate last contact (for pileups)
- `F3`: Toggle WiFi AP on/off
- `F4`: View recent logs (last 10)

**Visual Design:**
- Large text for field values (size 2)
- Color-coded field highlights (cyan for active, green for valid, red for invalid)
- Status bar: WiFi mode, client count, total logs, battery
- Minimal chrome, maximum data density

### 2. Log Management (On-Device)

**View Logs Screen:**
- Scrollable list of recent contacts (paginated, 5 per page)
- Each entry shows: Callsign, Frequency, Mode, Date/Time
- Select entry to view full details
- Options: Edit, Delete, Export trigger

**Log Statistics:**
- Total contacts logged
- Contacts by band (160m, 80m, 40m, 20m, 15m, 10m, 6m, 2m)
- Contacts by mode (CW, SSB, Digital)
- Unique callsigns worked (DX count)
- Session stats (contacts in last hour/day)

**Data Storage:**
- LittleFS filesystem for QSO records (JSON format)
- Preferences for settings (operator info, defaults)
- Automatic circular buffer: Keep last 500 contacts, auto-delete oldest
- File naming: `/logs/qso_YYYYMMDD_HHMMSS.json`

### 3. Web Interface Architecture

**Technology Stack:**
- ESP32 Async Web Server (ESPAsyncWebServer library)
- Single-page application (SPA) with vanilla JavaScript
- Responsive design (mobile-first, works on phones/tablets/laptops)
- RESTful API for all operations
- WebSocket for real-time updates (optional enhancement)

**Web Pages:**

#### 3.1 Dashboard (`/`)
- Welcome screen with device status
- Quick stats: Total logs, current session, band breakdown
- Links to Log Entry, View Logs, Settings, Export
- Real-time status indicators (battery, WiFi strength, storage used)

#### 3.2 Log Entry (`/log`)
- Full-featured contact entry form
- All essential fields plus extended fields:
  - **Name** (operator name)
  - **QTH** (location)
  - **Power** (watts, integer)
  - **Grid Square** (Maidenhead locator, validated)
  - **IOTA** (Islands On The Air reference)
  - **State/Province** (for US/Canada contacts)
  - **Country** (auto-populated via prefix lookup)
  - **Comment** (unlimited length)
  - **Contest** (contest name for contest logging)
  - **SRX** (serial received for contests)
  - **STX** (serial transmitted for contests)
- Band buttons for quick frequency entry (160m, 80m, 40m, etc.)
- Mode buttons for quick selection
- Auto-complete for callsigns (from previous logs)
- Prefix-based country/continent lookup
- Live validation with error messages
- "Save & New" vs "Save & View" buttons
- Keyboard shortcuts: Ctrl+S to save, Ctrl+N for new

#### 3.3 View Logs (`/logs`)
- Sortable, filterable table of all contacts
- Columns: Date, Time, Callsign, Frequency, Mode, RST S/R, Name, QTH
- Click row to expand full details
- Inline editing (click field to edit)
- Bulk selection with checkboxes
- Search bar: Filter by callsign, name, QTH, or notes
- Filters: Date range, band, mode, contest
- Pagination: 25/50/100 contacts per page
- Export selected contacts button

#### 3.4 Statistics (`/stats`)
- Charts and graphs (Chart.js library):
  - Contacts per day (bar chart, last 30 days)
  - Contacts by band (pie chart)
  - Contacts by mode (pie chart)
  - Contacts by hour of day (line chart)
- Top 10 lists:
  - Most worked stations
  - Most active bands
  - Most active modes
  - Countries worked
- DXCC progress tracker (countries worked vs total)
- Worked All States (WAS) progress (US states grid)
- Total distance worked (if grid squares available)

#### 3.5 Settings (`/settings`)
- **Operator Info:**
  - Callsign (saved to device)
  - Name
  - QTH
  - Grid Square
  - Station Info (rig, antenna, power)
- **Defaults:**
  - Default mode
  - Default frequency
  - Default RST values
  - Auto-increment frequency by band (e.g., +10 kHz for 20m CW)
- **WiFi Settings:**
  - Current mode (Station vs AP)
  - AP SSID and password configuration
  - Station connection status
  - IP address display
- **Data Management:**
  - Total logs count
  - Storage used / available
  - Clear all logs (with confirmation)
  - Factory reset

#### 3.6 Export (`/export`)
- Multiple export formats:
  - **ADIF** (.adi) - Standard amateur radio format
  - **Cabrillo** (.cbr) - Contest log format
  - **CSV** (.csv) - Spreadsheet format
  - **JSON** (.json) - Raw data format
- Export options:
  - All contacts
  - Date range selection
  - By band/mode filter
- Download directly to browser
- Copy to clipboard button (for mobile)

### 4. WiFi Modes

#### 4.1 Station Mode (Client Mode)
- Connect to existing WiFi network (home, hotel, field day tent)
- Use Settings → WiFi Setup to scan and connect
- Device gets IP from DHCP
- Web interface accessible at `http://vail-summit.local` (mDNS) or IP address
- Display IP on LCD status bar
- More power-efficient than AP mode

#### 4.2 Access Point Mode (Hotspot Mode)
- Device creates its own WiFi network
- Default SSID: `VAIL-SUMMIT-QSO` (configurable)
- Default password: `MORSE123` (configurable)
- Static IP: `192.168.4.1` (standard ESP32 AP address)
- Web interface at `http://192.168.4.1`
- Supports up to 4 simultaneous clients
- Display "AP MODE" badge on LCD
- Toggle via F3 shortcut or Settings menu

#### 4.3 Mode Switching
- Seamless switching between Station and AP modes
- No reboot required (just reconnect web clients)
- Settings persist across reboots
- Auto-start mode: Configurable (Station/AP/Off)

### 5. Data Model

**QSO Record Structure (JSON):**
```json
{
  "id": 12345678,          // Unix timestamp as unique ID
  "callsign": "W1AW",      // Required
  "frequency": 14.025,     // MHz, required
  "mode": "CW",            // Required
  "band": "20m",           // Auto-calculated from frequency
  "rst_sent": "599",       // Required
  "rst_rcvd": "599",       // Required
  "date": "20250428",      // YYYYMMDD
  "time_on": "1430",       // HHMM UTC
  "time_off": "1432",      // HHMM UTC (optional, defaults to time_on)
  "name": "Hiram",         // Optional
  "qth": "Newington, CT",  // Optional
  "power": 100,            // Watts, optional
  "gridsquare": "FN31pr",  // Optional
  "country": "USA",        // Auto-populated
  "state": "CT",           // Optional
  "iota": "",              // Optional
  "notes": "Nice fist",    // Optional
  "contest": "",           // Optional
  "srx": 0,                // Serial RX (contest)
  "stx": 0,                // Serial TX (contest)
  "operator": "K1ABC",     // Device callsign
  "station_callsign": "K1ABC"  // Same unless guest op
}
```

**Storage Layout:**
- `/logs/qso_YYYYMMDD.json` - Daily log files (array of QSO objects)
- `/logs/metadata.json` - Stats cache and indexes
- `/settings/operator.json` - Operator profile
- `/settings/defaults.json` - Default values

**Indexing Strategy:**
- In-memory hash map for callsign lookup (built on startup)
- Metadata file with counts per band/mode/day (updated on each log)
- Avoids scanning all files for stats

### 6. Export Formats

#### 6.1 ADIF (Amateur Data Interchange Format)
```
<ADIF_VER:5>3.1.4
<PROGRAMID:11>VAIL-SUMMIT
<PROGRAMVERSION:5>1.0.0
<EOH>

<QSO_DATE:8>20250428
<TIME_ON:4>1430
<CALL:4>W1AW
<FREQ:6>14.025
<BAND:3>20m
<MODE:2>CW
<RST_SENT:3>599
<RST_RCVD:3>599
<NAME:5>Hiram
<QTH:15>Newington, CT
<GRIDSQUARE:6>FN31pr
<TX_PWR:3>100
<COMMENT:9>Nice fist
<EOR>
```

#### 6.2 Cabrillo (Contest Format)
```
START-OF-LOG: 3.0
CALLSIGN: K1ABC
CONTEST: CQWW-CW
CATEGORY-OPERATOR: SINGLE-OP
CATEGORY-BAND: ALL
CATEGORY-POWER: LOW
CATEGORY-MODE: CW
CLAIMED-SCORE: 0
OPERATORS: K1ABC
NAME: John Doe
ADDRESS: 123 Main St
ADDRESS: Anytown, USA
EMAIL: k1abc@example.com
SOAPBOX: Made with VAIL SUMMIT
QSO: 14025 CW 2025-04-28 1430 K1ABC 599 001 W1AW 599 001
END-OF-LOG:
```

#### 6.3 CSV (Spreadsheet Format)
```
Date,Time,Callsign,Frequency,Mode,Band,RST Sent,RST Rcvd,Name,QTH,Power,Grid,Notes
2025-04-28,14:30,W1AW,14.025,CW,20m,599,599,Hiram,"Newington, CT",100,FN31pr,Nice fist
```

### 7. API Endpoints

**RESTful API Design:**

#### Logging Operations
- `GET /api/logs` - Get all logs (with query params: limit, offset, filter)
- `GET /api/logs/:id` - Get single log by ID
- `POST /api/logs` - Create new log
- `PUT /api/logs/:id` - Update existing log
- `DELETE /api/logs/:id` - Delete log
- `DELETE /api/logs` - Bulk delete (with body: array of IDs)

#### Statistics
- `GET /api/stats/summary` - Overall statistics
- `GET /api/stats/bands` - Contacts by band
- `GET /api/stats/modes` - Contacts by mode
- `GET /api/stats/countries` - Countries worked
- `GET /api/stats/daily/:days` - Daily contacts for last N days

#### Export
- `GET /api/export/adif` - Export as ADIF (query params: filter)
- `GET /api/export/cabrillo` - Export as Cabrillo
- `GET /api/export/csv` - Export as CSV
- `GET /api/export/json` - Export as JSON

#### Settings
- `GET /api/settings/operator` - Get operator info
- `PUT /api/settings/operator` - Update operator info
- `GET /api/settings/defaults` - Get default values
- `PUT /api/settings/defaults` - Update defaults
- `GET /api/settings/wifi` - Get WiFi status
- `POST /api/settings/wifi/mode` - Switch WiFi mode (station/ap)

#### System
- `GET /api/system/status` - Device status (battery, storage, WiFi, uptime)
- `GET /api/system/time` - Current device time
- `POST /api/system/time` - Set device time (for field use without NTP)
- `POST /api/system/reset` - Factory reset (with confirmation token)

**Response Format:**
```json
{
  "success": true,
  "data": { /* response payload */ },
  "message": "Operation successful",
  "timestamp": 1714308000
}
```

**Error Format:**
```json
{
  "success": false,
  "error": "Invalid callsign format",
  "code": 400,
  "timestamp": 1714308000
}
```

### 8. Validation Rules

**Callsign Validation:**
- Pattern: 1-3 letter prefix + 1 digit + 1-4 letter suffix
- Examples: W1AW, K6XYZ, G4ABC, JA1XYZ, VK2OM
- Special: /P, /M, /MM, /QRP suffixes allowed
- Convert to uppercase automatically

**Frequency Validation:**
- Range: 1.8 - 1300 MHz (covers 160m to 23cm)
- Band detection:
  - 1.8-2.0 → 160m
  - 3.5-4.0 → 80m
  - 7.0-7.3 → 40m
  - 14.0-14.35 → 20m
  - 21.0-21.45 → 15m
  - 28.0-29.7 → 10m
  - 50.0-54.0 → 6m
  - 144-148 → 2m
  - 420-450 → 70cm

**RST Validation:**
- CW: 1-5 digits (e.g., 599, 339, 579)
- Phone: 1-5 digits (e.g., 59, 33, 57)
- Valid readability: 1-5
- Valid strength: 1-9
- Valid tone: 1-9 (CW only)

**Grid Square Validation:**
- Pattern: AA##aa (e.g., FN31pr)
- AA: Field (A-R, longitude)
- ##: Square (0-9, latitude)
- aa: Subsquare (a-x, optional)
- Case insensitive, convert to standard case (AANNaa)

**Date/Time Validation:**
- Date: YYYYMMDD format
- Time: HHMM format (UTC preferred)
- Auto-fill from system clock
- Manual entry with validation

### 9. User Experience Enhancements

**Auto-Complete Features:**
- Callsign lookup from previous logs
- QTH suggestions based on callsign prefix
- Name recall for repeat contacts
- Frequency presets per band/mode

**Smart Defaults:**
- Last used frequency carried forward
- Mode-appropriate RST (599 for CW, 59 for phone)
- Current UTC date/time auto-filled
- Power from operator settings

**Duplicate Detection:**
- Warn if same callsign on same band/mode within 24 hours
- Highlight in yellow with "Possible Dupe" indicator
- Option to save anyway (for different bands/modes)

**Mobile Optimization:**
- Touch-friendly buttons (44px minimum)
- Swipe gestures for navigation
- Offline support (service worker caching)
- Portrait and landscape layouts
- Haptic feedback on button press (if supported)

**Accessibility:**
- Keyboard navigation (tab order)
- Screen reader labels (ARIA)
- High contrast mode toggle
- Adjustable font sizes

### 10. Integration with VAIL SUMMIT

**Menu Structure:**
```
Main Menu
└── Tools
    ├── QSO Logger
    │   ├── New Log Entry
    │   ├── View Logs
    │   ├── Statistics
    │   ├── Export Logs
    │   └── WiFi Setup
    └── ... (existing tools)
```

**Mode Integration:**
- New mode: `MODE_QSO_LOGGER_ENTRY` for on-device logging
- New mode: `MODE_QSO_LOGGER_VIEW` for viewing logs
- New mode: `MODE_QSO_LOGGER_STATS` for statistics
- Web server runs in background (update loop function)

**Shared Resources:**
- Use existing WiFi management from `settings_wifi.h`
- Reuse display utilities and color schemes from `config.h`
- Share CardKB input handling from main loop
- Use existing Preferences system for settings

**Header Files Structure:**
- `qso_logger.h` - Main logger logic and state management
- `qso_logger_ui.h` - On-device LCD UI rendering
- `qso_logger_storage.h` - LittleFS file operations
- `qso_logger_web.h` - Web server setup and API handlers
- `qso_logger_export.h` - Export format generators (ADIF, CSV, Cabrillo)
- `qso_logger_validation.h` - Input validation functions

### 11. Performance Considerations

**Memory Management:**
- JSON streaming for large log files (avoid loading all into RAM)
- Pagination for web interface (limit in-memory records)
- Circular buffer: Auto-delete logs beyond 500 contacts
- Static allocation for on-device UI (no dynamic memory in display loop)

**Storage Optimization:**
- Compressed JSON (no whitespace)
- Daily log files to avoid huge single files
- Metadata cache for fast stats queries
- Indexed callsign lookups

**Web Server Performance:**
- Async web server (non-blocking)
- GZIP compression for HTML/CSS/JS
- Minimal JavaScript libraries (prefer vanilla JS)
- Lazy loading for log table (load 25 rows at a time)
- Service worker for offline caching

**Battery Efficiency:**
- WiFi sleep mode when idle
- Auto-shutdown AP after 30 minutes of inactivity
- Display timeout during web-only use
- Efficient JSON parsing (no redundant copies)

### 12. Security Considerations

**WiFi AP Security:**
- WPA2 encryption mandatory
- Strong default password (10+ characters)
- Password change required on first use
- Hide SSID option

**Web Interface Security:**
- Basic authentication option (username/password)
- CORS headers (restrict to same origin)
- Input sanitization (prevent XSS)
- CSRF token for destructive operations
- Rate limiting on API endpoints

**Data Privacy:**
- No cloud sync (all data stays on device)
- No telemetry or analytics
- Export/delete operations require confirmation
- Factory reset option for resale

### 13. Testing Strategy

**Unit Tests:**
- Callsign validation
- Frequency to band conversion
- RST validation
- Grid square validation
- ADIF export format
- JSON serialization/deserialization

**Integration Tests:**
- WiFi mode switching
- Log entry → storage → retrieval
- Web API requests
- Export generation
- Duplicate detection

**Field Testing:**
- Battery life during 4-hour operation
- WiFi range (AP mode)
- Concurrent web clients (4 users)
- Storage capacity (500 logs)
- Display responsiveness during web activity

### 14. Future Enhancements

**Phase 2 Features:**
- ClubLog / QRZ.com integration (upload logs)
- LOTW (Logbook of the World) export
- Real-time contest scoring
- Band map display (visualize contacts)
- Propagation predictions (VOACAP integration)

**Phase 3 Features:**
- GPS integration (auto-populate grid square)
- FT8/FT4 decode integration (if using external radio)
- Awards tracking (DXCC, WAS, WAC, etc.)
- Logbook synchronization with cloud services
- Multi-operator support (guest logging)

### 15. Documentation Requirements

**User Documentation:**
- Quick start guide (printed card)
- Web interface tutorial (embedded help pages)
- Export format reference
- API documentation (for third-party apps)
- Troubleshooting guide

**Developer Documentation:**
- Architecture overview
- File format specifications
- API endpoint reference
- Build and deployment guide
- Contribution guidelines

## Success Criteria

**MVP Launch:**
- [ ] On-device log entry with 7 essential fields
- [ ] LittleFS storage with 500-log capacity
- [ ] WiFi AP mode with web interface
- [ ] Log viewing and editing via web
- [ ] ADIF export
- [ ] Basic statistics (counts by band/mode)
- [ ] Battery lasts 4+ hours with WiFi on

**Full Release:**
- [ ] WiFi Station mode support
- [ ] All export formats (ADIF, CSV, Cabrillo, JSON)
- [ ] Advanced statistics with charts
- [ ] Duplicate detection
- [ ] Auto-complete and smart defaults
- [ ] Mobile-responsive web UI
- [ ] Inline log editing
- [ ] Comprehensive documentation

## Technical Dependencies

**ESP32 Libraries:**
- ESPAsyncWebServer (web server)
- AsyncTCP (async networking)
- ArduinoJson (JSON parsing/generation)
- LittleFS (file system)
- DNSServer (captive portal for AP mode)
- ESPmDNS (hostname resolution)

**Web Libraries (CDN):**
- Chart.js (statistics graphs)
- Optional: Vanilla JS (no React/Vue to save space)

**Firmware Size Estimate:**
- Core logger code: ~50KB
- Web server framework: ~80KB
- Web interface HTML/CSS/JS: ~150KB (gzipped ~40KB)
- Total addition: ~180KB (well within ESP32-S3 4MB flash)

## Open Questions

1. Should the device support automatic QSO uploads to online logbooks (ClubLog, QRZ)?
2. Should contest mode have special features (serial numbers, exchange validation)?
3. Should there be a mobile app companion (native iOS/Android)?
4. Should GPS be added for automatic grid square calculation?
5. What's the priority for offline web interface (service worker caching)?
6. Should logs sync between multiple VAIL SUMMIT devices (via WiFi direct)?

## Conclusion

The QSO Logger mode transforms VAIL SUMMIT from a training device into a field-ready logging station. The dual interface (on-device + web) maximizes flexibility: quick field logging on the LCD, full-featured editing and management on a phone/laptop. The WiFi AP mode eliminates the need for internet access, making it ideal for portable operations, field days, and SOTA activations.

The modular architecture ensures easy integration with the existing VAIL SUMMIT codebase while maintaining the device's core training functionality. Storage optimization and battery efficiency keep it practical for all-day field use.