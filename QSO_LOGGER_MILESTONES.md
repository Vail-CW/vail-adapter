# QSO Logger - Implementation Milestones

## Architecture Overview

Following VAIL SUMMIT patterns:
- **Modular header files** for each feature area
- **Menu enum** in `menu_ui.h` for new modes
- **Card-based UI** matching existing carousel design
- **Input handlers** return -1 to exit, 0 for normal input, 2 for redraw
- **Preferences** for persistent settings
- **LittleFS** for log storage

## Milestone 1: Menu Integration & Basic Structure
**Goal:** Create menu entry point and basic UI framework
**Status:** Ready to implement
**Test:** Navigate to QSO Logger from Tools menu

### Tasks:
1. Add QSO Logger modes to `MenuMode` enum in `menu_ui.h`
   - `MODE_TOOLS_MENU` - New submenu for tools
   - `MODE_QSO_LOGGER_MENU` - QSO Logger submenu
   - `MODE_QSO_LOG_ENTRY` - Log entry screen
   - `MODE_QSO_VIEW_LOGS` - View logs screen

2. Update main menu to include "Tools" option
   - Add "Tools" to `mainMenuOptions[]` and `mainMenuIcons[]`
   - Icon: "L" for Logger/Tools

3. Create `qso_logger.h` with basic structure
   - State variables
   - Forward declarations
   - Include guards

4. Create `qso_logger_ui.h` with placeholder UI functions
   - `drawToolsMenu()` - Tools submenu
   - `drawQSOLoggerMenu()` - QSO Logger submenu
   - `drawQSOLogEntryUI()` - Log entry screen (placeholder)
   - `drawQSOViewLogsUI()` - View logs screen (placeholder)

5. Update `menu_ui.h` to dispatch to new UI functions
   - Add cases in `drawMenu()` for new modes
   - Add tools submenu options array

6. Update `menu_navigation.h` to handle new menu selections
   - Add handlers in `selectMenuItem()` for tools menu
   - Create placeholder input handlers

7. Update `vail-summit.ino` to include new headers

### Testing:
- Compile successfully
- Navigate: Main Menu → Tools → QSO Logger
- See placeholder screens
- ESC back to previous menu works

### Files Created:
- `qso_logger.h` (new)
- `qso_logger_ui.h` (new)

### Files Modified:
- `menu_ui.h`
- `menu_navigation.h`
- `vail-summit.ino`

---

## Milestone 2: Data Model & Storage
**Goal:** Implement LittleFS storage and QSO data structures
**Dependencies:** Milestone 1
**Test:** Create and retrieve a test QSO log entry

### Tasks:
1. Create `qso_logger_storage.h`
   - QSO struct definition (all fields)
   - LittleFS initialization function
   - Functions: saveQSO(), loadQSOs(), deleteQSO()
   - Circular buffer logic (500 log limit)

2. Initialize LittleFS in setup()
   - Add LittleFS.begin() to hardware initialization
   - Error handling if mount fails

3. Implement JSON serialization
   - QSO to JSON (ArduinoJson library)
   - JSON to QSO parsing
   - Daily log file format: `/logs/qso_YYYYMMDD.json`

4. Implement metadata caching
   - File: `/logs/metadata.json`
   - Counts by band, mode, date
   - Update on each log save

5. Create test function to save/load a dummy QSO
   - Debug output to Serial
   - Verify file creation in LittleFS

### Testing:
- Upload firmware with test QSO save
- Verify Serial output shows successful save
- Check LittleFS file system via Serial commands
- Test circular buffer (save 501 entries, verify oldest deleted)

### Files Created:
- `qso_logger_storage.h` (new)

### Files Modified:
- `vail-summit.ino` (add LittleFS init)

---

## Milestone 3: Log Entry Form (On-Device)
**Goal:** Implement functional log entry form with 7 essential fields
**Dependencies:** Milestone 2
**Test:** Enter a complete QSO log via CardKB and save to storage

### Tasks:
1. Design log entry form UI (matching existing style)
   - Card-based layout for each field
   - Current field highlighted in cyan
   - Field labels and input values
   - Status footer with instructions

2. Implement field navigation
   - TAB/DOWN: Next field
   - SHIFT+TAB/UP: Previous field
   - Current field tracking (0-6)

3. Implement input handling per field type
   - Callsign: Uppercase letters/numbers, validate format
   - Frequency: Decimal number, auto-detect band
   - Mode: Cycle through CW/SSB/FM/AM/FT8/FT4/RTTY/PSK31
   - RST Sent/Rcvd: Default 599 (CW) or 59 (phone), editable
   - Date/Time: Auto-filled from millis(), manual override
   - Notes: Free text, up to 60 characters

4. Implement validation per field
   - Callsign regex: `^[A-Z0-9]{3,10}$` (basic)
   - Frequency: 1.8 - 1300 MHz
   - RST: 1-5 digits
   - Required field indicators (red if empty)

5. Implement save action (ENTER on last field or F1)
   - Validate all required fields
   - Create QSO struct
   - Call saveQSO()
   - Show success message
   - Clear form for next entry

6. Create `qso_logger_validation.h` for validation functions
   - `validateCallsign(String)`
   - `validateFrequency(float)`
   - `validateRST(String)`
   - `frequencyToBand(float)` - Returns "20m", "40m", etc.

### Testing:
- Enter complete QSO log
- Verify all fields accept input
- Test field navigation (TAB, arrows)
- Verify validation (try invalid callsign, frequency)
- Check saved log appears in LittleFS
- Test "save and new" workflow (enter 3 logs in a row)

### Files Created:
- `qso_logger_validation.h` (new)

### Files Modified:
- `qso_logger.h` (state variables for form)
- `qso_logger_ui.h` (complete log entry UI)
- `menu_navigation.h` (input handler)

---

## Milestone 4: View Logs (On-Device)
**Goal:** Display saved logs in scrollable list
**Dependencies:** Milestone 3
**Test:** View previously entered logs, scroll through list

### Tasks:
1. Implement log list UI
   - Paginated view (5 logs per page)
   - Each entry: Callsign, Frequency, Mode, Date/Time
   - Scroll arrows (up/down)
   - Card-based design matching menu style

2. Implement log loading from storage
   - Load all logs from daily files
   - Sort by timestamp (newest first)
   - Cache in memory (limit to last 50 for performance)

3. Implement pagination logic
   - UP/DOWN to scroll
   - Page indicator: "Log X of Y"
   - Wrap around at ends

4. Implement log detail view
   - ENTER on log entry to see full details
   - Show all fields (including notes)
   - Options: Back, Delete

5. Implement delete confirmation
   - Show warning dialog
   - ENTER to confirm, ESC to cancel
   - Update storage and metadata
   - Refresh list

### Testing:
- View logs after entering several entries
- Scroll through list with UP/DOWN
- Enter log detail view
- Delete a log and verify removed from list
- Check metadata updated after delete

### Files Modified:
- `qso_logger_ui.h` (view logs UI)
- `qso_logger_storage.h` (add loadAllQSOs function)
- `menu_navigation.h` (view logs input handler)

---

## Milestone 5: Statistics Display (On-Device)
**Goal:** Show summary statistics from logged QSOs
**Dependencies:** Milestone 4
**Test:** View accurate statistics matching logged data

### Tasks:
1. Implement statistics UI
   - Total contacts count
   - Contacts by band (bar display)
   - Contacts by mode (bar display)
   - Unique callsigns (DX count)
   - Session stats (last hour, today)

2. Implement statistics calculation
   - Read from metadata.json (fast)
   - Fallback: Calculate from log files if metadata missing
   - Cache results in memory

3. Create visual bar displays
   - Horizontal bars for band breakdown
   - Color-coded by band (20m = green, 40m = blue, etc.)
   - Percentages next to bars

### Testing:
- Enter logs on multiple bands and modes
- View statistics screen
- Verify counts are accurate
- Test with empty log (should show "No logs yet")

### Files Modified:
- `qso_logger_ui.h` (statistics UI)
- `qso_logger_storage.h` (statistics functions)
- `menu_navigation.h` (statistics input handler)

---

## Milestone 6: Export ADIF (On-Device Trigger)
**Goal:** Export logs to ADIF format for download
**Dependencies:** Milestone 4
**Test:** Generate ADIF file viewable via Serial or save to LittleFS

### Tasks:
1. Create `qso_logger_export.h`
   - `generateADIF(QSO* logs, int count)` → String
   - ADIF header with program ID and version
   - Convert each QSO to ADIF record format
   - Handle optional fields (name, QTH, grid, etc.)

2. Implement export trigger from menu
   - "Export Logs" option in QSO Logger menu
   - Select format: ADIF (others later)
   - Save to `/exports/qso_export_YYYYMMDD_HHMMSS.adi`
   - Show success message with filename

3. Add Serial dump option (for testing)
   - Print ADIF to Serial monitor
   - Useful before web interface is ready

### Testing:
- Export logs to ADIF
- Copy Serial output to text file
- Validate ADIF format with online validator
- Import into amateur radio logging software (WSJT-X, Log4OM, etc.)

### Files Created:
- `qso_logger_export.h` (new)

### Files Modified:
- `menu_navigation.h` (export menu handler)
- `qso_logger_ui.h` (export UI)

---

## Milestone 7: WiFi AP Mode & Web Server Setup
**Goal:** Start WiFi AP and serve basic web interface
**Dependencies:** Milestone 6
**Test:** Connect phone to VAIL-SUMMIT-QSO network, access web page

### Tasks:
1. Create `qso_logger_web.h`
   - Include ESPAsyncWebServer library
   - WiFi AP initialization function
   - Web server initialization
   - Root route handler: `/` → HTML dashboard

2. Implement WiFi AP mode toggle
   - F3 shortcut to toggle AP on/off
   - Settings menu option
   - Store AP SSID and password in Preferences
   - Default: SSID="VAIL-SUMMIT-QSO", Password="MORSE123"

3. Create minimal HTML dashboard
   - Embedded in firmware as string literal
   - Responsive design (mobile-friendly)
   - Links to future pages (placeholder)
   - Shows device status (battery, logs count)

4. Implement mDNS for easy access
   - Hostname: `vail-summit.local`
   - Works in Station mode (when connected to WiFi)

5. Add web server to main loop
   - Non-blocking server.handleClient() equivalent
   - Update loop function in vail-summit.ino

6. Display AP status on LCD
   - Show "AP MODE" badge in status bar
   - Show IP address (192.168.4.1)
   - Client count indicator

### Testing:
- Toggle WiFi AP on via F3
- Connect phone to VAIL-SUMMIT-QSO network
- Open browser to http://192.168.4.1
- See dashboard page load
- Verify non-blocking (LCD still responsive)

### Files Created:
- `qso_logger_web.h` (new)

### Files Modified:
- `vail-summit.ino` (web server loop)
- `status_bar.h` (AP mode indicator)
- `menu_navigation.h` (F3 AP toggle)

---

## Milestone 8: Web API - Log CRUD Operations
**Goal:** Implement REST API for creating, reading, updating, deleting logs
**Dependencies:** Milestone 7
**Test:** Use Postman or browser console to create/view/edit/delete logs

### Tasks:
1. Implement API endpoints in `qso_logger_web.h`
   - `GET /api/logs` - Get all logs (with pagination)
   - `GET /api/logs/:id` - Get single log
   - `POST /api/logs` - Create new log
   - `PUT /api/logs/:id` - Update log
   - `DELETE /api/logs/:id` - Delete log

2. Implement JSON request/response handling
   - Parse incoming JSON (ArduinoJson)
   - Validate required fields
   - Return standardized response format
   - Error handling with proper HTTP codes

3. Implement pagination for GET /api/logs
   - Query params: `?limit=25&offset=0`
   - Return total count in response
   - Default limit: 25

4. Add CORS headers
   - Allow cross-origin requests (for development)
   - Restrict to same origin in production

### Testing:
- Use curl or Postman to test API endpoints
- Create log via POST
- Retrieve logs via GET
- Update log via PUT
- Delete log via DELETE
- Verify data persists in LittleFS
- Test pagination with 30+ logs

### Files Modified:
- `qso_logger_web.h` (API endpoints)

---

## Milestone 9: Web Interface - Log Entry Form
**Goal:** Build full-featured log entry form in web interface
**Dependencies:** Milestone 8
**Test:** Enter complete QSO with all fields via web browser

### Tasks:
1. Create HTML log entry page (embedded in firmware)
   - Form with all fields (essential + extended)
   - Responsive design (works on phone)
   - Modern CSS styling (dark theme)
   - JavaScript for validation and submission

2. Implement extended fields UI
   - Name, QTH, Grid Square, Power
   - IOTA, State/Province, Country
   - Comment (text area)
   - Contest name, SRX, STX

3. Implement band buttons
   - Quick frequency entry (160m, 80m, 40m, 20m, 15m, 10m, 6m, 2m)
   - Click button to set frequency to band center

4. Implement mode buttons
   - Quick mode selection (CW, SSB, FM, AM, FT8, FT4, RTTY, PSK31)

5. Implement client-side validation
   - Callsign format check
   - Frequency range validation
   - Grid square format validation
   - Show error messages inline

6. Implement form submission
   - POST to /api/logs
   - Show success/error message
   - Options: "Save & New" vs "Save & View"
   - Clear form after successful save

7. Implement auto-complete for callsigns
   - Fetch previous contacts for callsign
   - Pre-fill name, QTH from history
   - Dropdown suggestions

### Testing:
- Enter QSO with all fields via web
- Test band buttons (verify frequency set correctly)
- Test mode buttons
- Test validation (try invalid callsign, grid square)
- Test auto-complete (enter partial callsign)
- Verify log saves to storage
- Test on mobile phone (responsive design)

### Files Modified:
- `qso_logger_web.h` (HTML/CSS/JS embedded)

---

## Milestone 10: Web Interface - View/Edit Logs
**Goal:** Display logs in sortable/filterable table with inline editing
**Dependencies:** Milestone 9
**Test:** View logs, sort by date, filter by band, edit entry

### Tasks:
1. Create log table HTML
   - Responsive table design
   - Columns: Date, Time, Callsign, Freq, Mode, RST S/R, Name, QTH
   - Expandable rows for full details

2. Implement sorting
   - Click column header to sort
   - Toggle ascending/descending
   - Visual indicator (arrow icon)

3. Implement filtering
   - Search bar (filter by callsign, name, QTH)
   - Band filter dropdown
   - Mode filter dropdown
   - Date range picker

4. Implement pagination
   - Show 25/50/100 logs per page
   - Page navigation controls
   - Total logs count

5. Implement inline editing
   - Click field to edit
   - Show save/cancel buttons
   - PUT request to update
   - Visual feedback on save

6. Implement bulk selection
   - Checkboxes for each row
   - "Select All" checkbox
   - Bulk delete button (with confirmation)

### Testing:
- View logs table in browser
- Sort by different columns
- Filter by callsign (search bar)
- Filter by band and mode (dropdowns)
- Edit log inline (change frequency)
- Delete log (single and bulk)
- Test pagination (scroll through pages)

### Files Modified:
- `qso_logger_web.h` (HTML/CSS/JS for log viewer)

---

## Milestone 11: Web Interface - Statistics & Charts
**Goal:** Display statistics with visual charts
**Dependencies:** Milestone 10
**Test:** View charts showing contacts by band, mode, and day

### Tasks:
1. Implement statistics API endpoint
   - `GET /api/stats/summary` - Overall stats
   - `GET /api/stats/bands` - Contacts by band
   - `GET /api/stats/modes` - Contacts by mode
   - `GET /api/stats/daily/:days` - Daily contacts

2. Create statistics HTML page
   - Include Chart.js library (CDN)
   - Responsive grid layout
   - Multiple chart types

3. Implement charts
   - Contacts by band (pie chart)
   - Contacts by mode (pie chart)
   - Contacts per day (bar chart, last 30 days)
   - Contacts by hour (line chart)

4. Implement Top 10 lists
   - Most worked stations
   - Most active bands
   - Most active modes

5. Implement progress trackers
   - DXCC progress (countries worked)
   - WAS progress (US states grid)

### Testing:
- View statistics page in browser
- Verify charts render correctly
- Check data accuracy (matches log entries)
- Test responsive design (mobile)

### Files Modified:
- `qso_logger_web.h` (stats API and HTML)

---

## Milestone 12: Export Formats (Web Interface)
**Goal:** Export logs in multiple formats via web interface
**Dependencies:** Milestone 11
**Test:** Export logs as ADIF, CSV, Cabrillo, JSON

### Tasks:
1. Implement export API endpoints
   - `GET /api/export/adif` - Export as ADIF
   - `GET /api/export/csv` - Export as CSV
   - `GET /api/export/cabrillo` - Export as Cabrillo
   - `GET /api/export/json` - Export as JSON

2. Implement CSV export in `qso_logger_export.h`
   - Header row with column names
   - One row per QSO
   - Comma-separated values
   - Quote strings containing commas

3. Implement Cabrillo export in `qso_logger_export.h`
   - Contest log format
   - Header section with operator info
   - QSO lines
   - END-OF-LOG

4. Create export HTML page
   - Format selection (ADIF/CSV/Cabrillo/JSON)
   - Filter options (date range, band, mode)
   - Download button
   - Copy to clipboard button (for mobile)

5. Implement query params for filtering
   - `?start_date=YYYYMMDD&end_date=YYYYMMDD`
   - `?band=20m`
   - `?mode=CW`

### Testing:
- Export logs in all formats
- Verify file downloads correctly
- Import ADIF into Log4OM or WSJT-X
- Open CSV in Excel/Google Sheets
- Validate Cabrillo format with contest software
- Test filtered exports (date range, band, mode)

### Files Modified:
- `qso_logger_export.h` (CSV, Cabrillo generators)
- `qso_logger_web.h` (export API and HTML)

---

## Milestone 13: Settings & Operator Profile
**Goal:** Manage device settings and operator info via web interface
**Dependencies:** Milestone 12
**Test:** Update operator callsign, view settings in web interface

### Tasks:
1. Implement settings API endpoints
   - `GET /api/settings/operator` - Get operator info
   - `PUT /api/settings/operator` - Update operator info
   - `GET /api/settings/defaults` - Get default values
   - `PUT /api/settings/defaults` - Update defaults

2. Load/save operator settings from Preferences
   - Namespace: "qso_operator"
   - Fields: callsign, name, QTH, grid, station_info

3. Load/save default values from Preferences
   - Namespace: "qso_defaults"
   - Fields: mode, frequency, rst, power

4. Create settings HTML page
   - Form for operator info
   - Form for defaults
   - Save buttons
   - Success/error feedback

5. Implement WiFi mode toggle via web
   - Show current mode (Station/AP)
   - Button to switch modes
   - Show IP address and connection status

### Testing:
- Update operator callsign via web
- Verify saved to Preferences
- Update default values
- Create new log and verify defaults applied
- Toggle WiFi mode (AP → Station)
- Verify settings persist after reboot

### Files Modified:
- `qso_logger_web.h` (settings API and HTML)
- `qso_logger.h` (load/save settings functions)

---

## Milestone 14: Station Mode & mDNS
**Goal:** Connect to existing WiFi network and use mDNS hostname
**Dependencies:** Milestone 13
**Test:** Connect to home WiFi, access logger at http://vail-summit.local

### Tasks:
1. Implement Station mode connection
   - Use existing WiFi credentials from settings_wifi.h
   - Connect on startup if credentials saved
   - Display "STA MODE" badge on LCD
   - Show IP address in status bar

2. Implement mDNS hostname
   - Register hostname: "vail-summit"
   - Access via http://vail-summit.local
   - Works in Station mode only

3. Implement mode switching
   - Settings page: toggle between Station and AP
   - Store preferred mode in Preferences
   - Auto-start preferred mode on boot

4. Implement fallback logic
   - If Station mode fails to connect, fall back to AP mode
   - Show error message on LCD
   - Retry connection every 30 seconds

### Testing:
- Configure WiFi credentials via Settings → WiFi Setup
- Reboot device
- Verify connects to WiFi in Station mode
- Access web interface at http://vail-summit.local
- Disconnect WiFi, verify falls back to AP mode
- Manually switch between modes via web settings

### Files Modified:
- `qso_logger_web.h` (mDNS setup, mode switching)
- `vail-summit.ino` (WiFi init on startup)

---

## Milestone 15: Polish & Optimization
**Goal:** Performance tuning, UI polish, battery optimization
**Dependencies:** Milestone 14
**Test:** 4+ hour battery life with WiFi on, smooth UI, no memory leaks

### Tasks:
1. Performance optimization
   - Profile memory usage (heap, stack)
   - Optimize JSON parsing (streaming instead of loading all)
   - Implement lazy loading for log table (virtual scrolling)
   - Cache frequently accessed data

2. UI polish
   - Loading spinners for API requests
   - Better error messages (user-friendly)
   - Confirmation dialogs for destructive actions
   - Toast notifications for success/error
   - Keyboard shortcuts (documented in help)

3. Battery optimization
   - WiFi sleep mode when idle (no clients connected)
   - Auto-shutdown AP after 30 minutes of inactivity
   - Display timeout during web-only use
   - Configurable auto-sleep timer

4. Error handling
   - Handle full storage gracefully
   - Handle malformed log data
   - Handle network errors (AP connection lost)
   - Show helpful error messages

5. Documentation
   - Inline comments for complex logic
   - README section for QSO Logger
   - User guide (embedded in web interface)
   - API documentation page

### Testing:
- Battery life test (4 hours continuous use)
- Stress test (enter 100 logs via web)
- Memory leak test (monitor heap over time)
- Error condition tests (full storage, bad data)
- Usability test with real user (field day scenario)

### Files Modified:
- All QSO logger files (polish)
- `qso-logger-mode.md` (update with final details)

---

## Summary of Milestones

| Milestone | Goal | Estimated Time | Dependencies |
|-----------|------|----------------|--------------|
| 1 | Menu Integration | 1-2 hours | None |
| 2 | Data Model & Storage | 2-3 hours | M1 |
| 3 | Log Entry Form (On-Device) | 3-4 hours | M2 |
| 4 | View Logs (On-Device) | 2-3 hours | M3 |
| 5 | Statistics Display | 2-3 hours | M4 |
| 6 | Export ADIF | 1-2 hours | M4 |
| 7 | WiFi AP & Web Server | 2-3 hours | M6 |
| 8 | Web API | 3-4 hours | M7 |
| 9 | Web Log Entry Form | 3-4 hours | M8 |
| 10 | Web View/Edit Logs | 4-5 hours | M9 |
| 11 | Statistics & Charts | 3-4 hours | M10 |
| 12 | Export Formats | 2-3 hours | M11 |
| 13 | Settings & Profile | 2-3 hours | M12 |
| 14 | Station Mode & mDNS | 2-3 hours | M13 |
| 15 | Polish & Optimization | 4-6 hours | M14 |

**Total Estimated Time:** 38-51 hours

---

## Development Strategy

### Phase 1: On-Device Foundation (M1-M6)
Build complete on-device logging system first. This provides immediate value and allows testing without web interface complexity.

**Deliverable:** Fully functional on-device QSO logger with storage and export.

### Phase 2: Web Interface Core (M7-M10)
Add WiFi AP mode and web interface for enhanced functionality. Focus on log entry and viewing.

**Deliverable:** Web-based logging accessible via phone/laptop.

### Phase 3: Advanced Features (M11-M15)
Statistics, multiple export formats, settings management, and optimization.

**Deliverable:** Production-ready QSO logger system.

---

## Next Steps

Ready to begin **Milestone 1: Menu Integration & Basic Structure**. This will:
- Add "Tools" menu to main menu
- Create QSO Logger submenu structure
- Set up placeholder screens
- Verify navigation works correctly

Let me know when you're ready to proceed!
