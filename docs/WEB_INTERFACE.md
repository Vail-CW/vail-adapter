# Web Interface

This document covers the web server, QSO logger, REST API endpoints, and all web pages.

## Web Server Architecture

### Technology Stack

- **ESPAsyncWebServer** - Non-blocking HTTP server
- **mDNS** - Accessible at `http://vail-summit.local`
- **WiFi Event Handlers** - Auto-start/stop on connect/disconnect
- **PROGMEM HTML** - Stores web pages in flash to save RAM
- **RESTful API** - JSON endpoints for CRUD operations

### Server Lifecycle

```cpp
WiFi connects → WiFi event fires → setupWebServer() → mDNS starts → Server begins
WiFi disconnects → stopWebServer() → mDNS ends → Server stops
```

**WiFi Event Handler** (vail-summit.ino:182-190):
- Automatically starts web server when WiFi connects (Station mode)
- Automatically stops web server when WiFi disconnects
- `setupWebServer()` detects AP vs Station mode and configures accordingly

### Access Methods

**mDNS (Recommended):**
```
http://vail-summit.local/
```

**Direct IP:**
```
http://192.168.1.xxx/  (Station mode)
http://192.168.4.1/    (AP mode)
```

**Browser Compatibility:**
- Desktop: Chrome, Firefox, Safari, Edge
- Mobile: iOS Safari, Chrome Android
- Requires JavaScript enabled

## Dashboard Page (/)

### Status Cards

- **Battery:** Voltage and percentage (from MAX17048/LC709203F)
- **WiFi:** Connection status and signal strength
- **QSO Count:** Total number of logged contacts
- **Real-time updates:** Every 10 seconds via `/api/status`

### Navigation Cards

- **QSO Logger** - View, manage, export logs
- **Device Settings** - CW speed, tone, volume, callsign
- **WiFi Setup** - Network configuration
- **System Info** - Firmware, memory, storage stats
- **Radio Control** - Remote morse code transmission

## QSO Logger Web Interface

**File:** `web_logger_enhanced.h` - Complete HTML/CSS/JavaScript in PROGMEM

### Features

**1. Station Settings (Header Badges)**
- Displays callsign and grid square
- Click to open modal for editing
- Saved to Preferences namespace "qso_operator"
- Auto-populated in new QSOs (my_gridsquare, my_pota_ref)

**2. QSO Table View**
- Sortable columns: Date/Time, Callsign, Freq/Band, Mode, RST, Grids, POTA
- Real-time search/filter across all fields
- Edit and Delete buttons for each QSO
- Responsive design (horizontal scroll on mobile)

**3. Statistics Cards**
- Total QSOs (all time)
- Today's QSO count
- Unique callsigns worked

**4. New QSO Modal**
- Full input form with validation
- Required fields: Callsign*, Frequency*, Mode*
- Optional: RST sent/rcvd, their grid, their POTA, notes
- Auto-uppercases callsigns and grid squares
- Auto-trims whitespace

**5. Edit QSO Modal**
- Pre-filled with existing QSO data
- Same validation as new QSO
- Updates in place

**6. Map Visualization**
- Leaflet.js integration (CDN-loaded)
- Shows today's QSOs with grid squares
- Markers at grid square center coordinates
- Click marker for callsign/grid popup
- Grid-to-lat/lon conversion using Maidenhead algorithm

**7. Export Functions**
- ADIF download (`.adi` file)
- CSV download (`.csv` file)

### Form Validation (Client-Side)

**HTML5 Attributes:**
```html
<input type="text" id="qsoCallsign" required
       minlength="3" maxlength="10"
       pattern="[A-Za-z0-9]+"
       placeholder="W1ABC"
       title="3-10 alphanumeric characters with at least one digit">

<input type="number" id="qsoFrequency" required
       min="1.8" max="1300" step="0.001"
       placeholder="14.025"
       title="Frequency between 1.8 and 1300 MHz">
```

**JavaScript Validation:**
```javascript
function validateCallsign(callsign) {
  if (callsign.length < 3 || callsign.length > 10) return 'Length error';
  if (!/^[A-Za-z0-9]+$/.test(callsign)) return 'Alphanumeric only';
  if (!/\d/.test(callsign)) return 'Must contain at least one digit';
  return null;
}
```

**Data Normalization:**
- Callsigns auto-uppercased
- Grid squares auto-uppercased
- Whitespace trimmed from all fields
- Default RST values (599) if empty

### Map Functionality

**Grid Square to Lat/Lon Conversion:**
```javascript
function gridToLatLon(grid) {
  // Example: "FN31pr" → [41.5, -73.0]
  const lon = (grid.charCodeAt(0) - 65) * 20 - 180
            + (grid.charCodeAt(2) - 48) * 2 + 1;
  const lat = (grid.charCodeAt(1) - 65) * 10 - 90
            + (grid.charCodeAt(3) - 48) + 0.5;
  return [lat, lon];
}
```

**Today's QSO Filter:**
```javascript
const today = new Date().toISOString().split('T')[0].replace(/-/g, '');
const todayQSOs = allQSOs.filter(q => q.date === today && q.gridsquare);
```

**Marker Management:**
- Clears existing markers before redrawing
- Only shows QSOs with valid grid squares (4+ characters)
- Click marker to see callsign and grid
- Map auto-centers on USA (lat: 39.8283, lon: -98.5795, zoom: 4)

## QSO Storage Architecture

### File Organization

```
/logs/
  qso_20251028.json    # Today's logs
  qso_20251027.json    # Yesterday's logs
  metadata.json        # Statistics cache
```

### Log File Format

```json
{
  "date": "20251028",
  "count": 3,
  "logs": [
    {
      "id": 1730154000,
      "callsign": "W1ABC",
      "frequency": 14.025,
      "mode": "CW",
      "band": "20m",
      "rst_sent": "599",
      "rst_rcvd": "599",
      "date": "20251028",
      "time_on": "1430",
      "gridsquare": "FN31pr",
      "my_gridsquare": "EN82xx",
      "my_pota_ref": "US-2256",
      "their_pota_ref": "",
      "notes": "Nice QSO"
    }
  ]
}
```

**Key Design Decisions:**
- **One file per date** - Simplifies daily log management
- **JSON format** - Human-readable, easy to parse
- **Unique IDs** - Unix timestamp (milliseconds) prevents duplicates
- **ADIF-compatible fields** - Direct mapping to ADIF export

### QSO Data Structure

The `QSO` struct (`qso_logger.h`) contains all ADIF-compatible fields:

**Required fields:**
- `callsign` (char[11]) - 3-10 alphanumeric with at least one digit
- `frequency` (float) - 1.8-1300 MHz
- `mode` (char[10]) - CW, SSB, FM, AM, FT8, FT4, RTTY, PSK31
- `band` (char[6]) - Auto-calculated from frequency
- `date` (char[9]) - YYYYMMDD format
- `time_on` (char[7]) - HHMM format

**Optional fields:**
- RST sent/received, name, QTH, power, grid squares (my/their)
- POTA references (my/their), IOTA, country, state
- Contest info, operator/station callsigns, notes

## Web Server API Endpoints

### Status & Data

**`GET /api/status`** - Device status
```json
{
  "battery": {"voltage": 3.85, "percent": 75},
  "wifi": {"connected": true, "ssid": "MyNetwork", "rssi": -45},
  "qsoCount": 142
}
```

**`GET /api/qsos`** - All QSO logs as JSON array
```json
[
  {"id": 1730154000, "callsign": "W1ABC", "frequency": 14.025, ...},
  {"id": 1730154120, "callsign": "K6XYZ", "frequency": 7.025, ...}
]
```

**`GET /api/export/adif`** - ADIF 3.1.4 formatted file
```
ADIF Export from VAIL SUMMIT
<PROGRAMID:11>VAIL SUMMIT
<PROGRAMVERSION:5>1.0.0
<ADIF_VER:5>3.1.4
<EOH>
<CALL:5>W1ABC <FREQ:6>14.025 <MODE:2>CW ...
```

**`GET /api/export/csv`** - CSV formatted file
```
Callsign,Frequency,Band,Mode,Date,Time,RST Sent,RST Rcvd,...
W1ABC,14.025,20m,CW,20251028,1430,599,599,...
```

### Station Settings

**`GET /api/settings/station`** - Load station settings
```json
{
  "callsign": "W1ABC",
  "gridsquare": "FN31pr",
  "pota": "US-2256"
}
```

**`POST /api/settings/station`** - Save station settings
- Body: `{"callsign": "W1ABC", "gridsquare": "FN31", "pota": "US-2256"}`
- Saves to Preferences namespace "qso_operator"

### QSO CRUD Operations

**`POST /api/qsos/create`** - Create new QSO
- Body: JSON with callsign, frequency, mode (required) + optional fields
- Auto-calculates band from frequency via `frequencyToBand()`
- **Critical:** Must initialize QSO struct with `memset(&qso, 0, sizeof(QSO))` to prevent JSON parsing errors
- Auto-fills date/time if not provided

**`POST /api/qsos/update`** - Update existing QSO
- Body: JSON with date, id (required) + updated fields
- Loads day's log file, finds QSO by ID, updates fields, saves back
- Recalculates band if frequency changes

**`DELETE /api/qsos/delete?date=YYYYMMDD&id=1234567890`** - Delete QSO
- Finds and removes QSO from log file
- Updates count in file
- Deletes file if no QSOs remain

## Device Settings Page (/settings)

### CW Settings Card

- Speed slider (5-40 WPM) with live display
- Tone frequency slider (400-1200 Hz, 50 Hz steps) with live display
- Key type dropdown (Straight, Iambic A, Iambic B)
- Save button with validation and feedback

**API Endpoints:**
- `GET /api/settings/cw` - Returns `{wpm, tone, keyType}`
- `POST /api/settings/cw` - Updates CW settings with validation

### Audio Settings Card

- Volume slider (0-100%) with live display
- Save button

**API Endpoints:**
- `GET /api/settings/volume` - Returns `{volume}`
- `POST /api/settings/volume` - Updates volume (0-100 validation)

### Station Settings Card

- Callsign text input (max 10 characters)
- Auto-uppercases input
- Save button

**API Endpoints:**
- `GET /api/settings/callsign` - Returns `{callsign}`
- `POST /api/settings/callsign` - Updates callsign

**Implementation:**
- Settings load automatically on page load
- Real-time slider updates show current values
- Validation on save (range checking)
- Success/error messages with 5-second auto-dismiss
- Saves to ESP32 Preferences immediately
- Updates global variables (`cwSpeed`, `cwTone`, `cwKeyType`, `vailCallsign`)

## System Info Page (/system)

### Information Cards

**1. Firmware Card**
- Version (e.g., "1.0.0")
- Build Date (e.g., "2025-01-30")

**2. System Card**
- Uptime (formatted as days/hours/minutes/seconds)
- CPU Speed (MHz)
- Flash Size (MB)

**3. Memory Card**
- Free RAM (KB/MB)
- Min Free RAM (lowest point since boot)
- Free PSRAM (KB/MB, or "N/A")
- Min Free PSRAM

**4. Storage Card**
- SPIFFS Used (KB/MB)
- SPIFFS Total (KB/MB)
- QSO Logs (count)

**5. WiFi Card**
- Status (Connected/Disconnected)
- SSID (network name)
- IP Address
- Signal Strength (RSSI in dBm, color-coded)

**6. Battery Card**
- Voltage (V, 2 decimal places)
- Charge (percentage)
- Monitor (MAX17048, LC709203F, or None)

**API Endpoint:**
- `GET /api/system/info` - Comprehensive JSON with all diagnostic data

**Features:**
- Auto-refresh every 10 seconds
- Smart formatting (bytes → KB/MB)
- WiFi signal color coding (green >-60dBm, yellow -60 to -70dBm, red <-70dBm)
- Last update timestamp
- Graceful fallbacks for missing data

## Radio Control Page (/radio)

### Radio Mode Status Card

- Shows whether Radio Output mode is active/inactive
- Polls every 5 seconds
- "Enter Radio Mode" button to switch device to MODE_RADIO_OUTPUT
- Status badge updates in real-time

### Transmission Settings Card

- WPM speed slider (5-40 WPM) with live display
- Adjusts device's global `cwSpeed` setting
- Saved to Preferences immediately on change

### Send Morse Code Message Card

- Text area for message input (max 200 characters)
- Character counter
- Send button queues message for transmission
- Info box explains radio output behavior

**API Endpoints:**
- `GET /api/radio/status` - Returns `{active, mode}`
- `POST /api/radio/enter` - Switches device to Radio Output mode
- `POST /api/radio/send` - Queues message via `queueRadioMessage()`
- `GET /api/radio/wpm` - Returns current WPM speed
- `POST /api/radio/wpm` - Updates WPM speed (5-40 validation)

### Message Queue System

**Implementation** (`radio_output.h`):
- Circular buffer holds up to 5 messages (200 characters each)
- `processRadioMessageQueue()` runs in main loop via `updateRadioOutput()`
- Waits if user is manually keying (doesn't interrupt)
- Sends messages character-by-character with proper WPM timing
- Keys radio via GPIO 18 (DIT) and GPIO 17 (DAH) pins
- Uses blocking morse character playback with accurate spacing
- Debug logging shows character timing and queue status

**User Workflow:**
1. User opens `/radio` in browser
2. Clicks "Enter Radio Mode" to switch device
3. Types message in text area
4. Clicks "Send Message" - message is queued on device
5. Device automatically transmits as morse code via 3.5mm radio output
6. Can queue up to 5 messages
7. Messages transmit sequentially without interrupting manual keying

### CW Memory Presets Card (Collapsible)

**File:** `web_server.h` - RADIO_HTML section with integrated JavaScript

**Features:**
- Collapsible card with ▶/▼ toggle indicator
- Dropdown selector showing all populated memory presets
- Format: `"Slot #: Label"` (e.g., `"1: CQ CALL"`)
- "Send Memory" button to queue preset for radio transmission
- Quick-send functionality for contest operation

**UI Elements:**
```html
<select id="memorySelect">
  <option value="">-- Select a memory --</option>
  <option value="1">1: CQ CALL</option>
  <option value="2">2: 5NN CA</option>
  ...
</select>
<button onclick="sendMemory()">Send Memory</button>
```

**Behavior:**
- Dropdown auto-populates from `/api/memories/list` on page load
- Only shows non-empty slots
- Send button calls `/api/memories/send` with selected slot
- Success message confirms queuing for transmission
- Auto-refreshes preset list every 15 seconds

### Manage CW Memories Card (Collapsible)

**Features:**
- Collapsible card with full CRUD table
- "Create New Preset" button opens modal dialog
- Table shows all 10 slots (empty and occupied)
- Actions per row: Preview, Edit, Delete (or Create for empty slots)

**Table Structure:**
```
Slot | Label      | Message                  | Actions
-----|------------|--------------------------|-------------------------
1    | CQ CALL    | CQ CQ DE K6ABC K         | [Preview] [Edit] [Delete]
2    | (empty)    |                          | [Create]
...
```

**Create/Edit Modal:**
- **Slot Selector:** Dropdown 1-10 (disabled when editing)
- **Label Input:** Max 15 characters with live counter
- **Message Input:** Textarea, max 100 characters with live counter
- **Auto-uppercase:** Both fields automatically uppercased
- **Validation:** Empty fields, character limits, morse-valid characters
- **Save/Cancel Buttons**

**Actions:**
1. **Preview** - Calls `/api/memories/preview` to play on device speaker
2. **Edit** - Opens modal pre-filled with existing data
3. **Delete** - Confirms via `confirm()` dialog, then calls `/api/memories/delete`
4. **Create** - Opens modal for new preset at selected slot

**JavaScript Functions:**
```javascript
loadMemories()              // Load all presets from API
updateMemoriesDropdown()    // Refresh quick-send dropdown
updateMemoriesTable()       // Refresh management table
sendMemory()                // Send selected preset via radio
previewMemory(slot)         // Preview on device speaker
showCreateModal(slot)       // Open create dialog
showEditModal(slot)         // Open edit dialog
saveMemory()                // Create or update preset
deleteMemory(slot)          // Delete preset with confirmation
```

**API Endpoints Used:**

**CW Memories API** (`web_api_memories.h`):

1. **`GET /api/memories/list`**
   - Returns all 10 memory presets
   - Response: `{presets: [{slot, label, message, isEmpty}, ...]}`

2. **`POST /api/memories/create`**
   - Creates new preset
   - Body: `{slot: 1-10, label: "...", message: "..."}`
   - Validation: slot range, label length (15), message length (100), morse characters
   - Response: `{success: true}` or `{success: false, error: "..."}`

3. **`POST /api/memories/update`**
   - Updates existing preset (same as create)
   - Body: `{slot: 1-10, label: "...", message: "..."}`
   - Same validation as create
   - Response: `{success: true}` or `{success: false, error: "..."}`

4. **`POST /api/memories/delete`**
   - Deletes preset (clears slot)
   - Body: `{slot: 1-10}`
   - Response: `{success: true}` or `{success: false, error: "..."}`

5. **`POST /api/memories/preview`**
   - Plays preset on device speaker (not radio output)
   - Body: `{slot: 1-10}`
   - Blocking operation - plays entire message before returning
   - Response: `{success: true}` or `{success: false, error: "..."}`

6. **`POST /api/memories/send`**
   - Queues preset for radio transmission
   - Body: `{slot: 1-10}`
   - Uses existing message queue (max 5 messages)
   - Response: `{success: true}` or `{success: false, error: "queue full"}`

**Data Flow:**
1. Page loads → `loadMemories()` fetches `/api/memories/list`
2. Dropdown and table populated from response
3. User clicks "Send Memory" → `sendMemory()` → POST `/api/memories/send`
4. User clicks "Preview" → `previewMemory(slot)` → POST `/api/memories/preview`
5. User creates/edits → Modal opens → `saveMemory()` → POST `/api/memories/create` or `/update`
6. User deletes → Confirm → `deleteMemory(slot)` → POST `/api/memories/delete`
7. After any mutation → `loadMemories()` refreshes UI

**Validation:**
- **Label:** 1-15 characters, auto-uppercased, cannot be empty
- **Message:** 1-100 characters, auto-uppercased, cannot be empty, morse-valid only
- **Morse-valid characters:** A-Z, 0-9, space, `.`,`,`,`?`,`/`,`-`, `<`, `>` (for prosigns)
- **Slot:** 1-10 integer
- Server-side validation returns descriptive error messages

**Mobile Responsive:**
- Table scrolls horizontally on small screens
- Modal scales to viewport
- Buttons stack vertically on mobile
- Touch-friendly button sizing

**Debug and Troubleshooting:**

The CW Memories API includes comprehensive debug logging for troubleshooting transmission and playback issues:

- All API endpoints log operations to Serial (115200 baud)
- Preview endpoint logs blocking behavior (message plays completely before HTTP response)
- Send endpoint logs queue status (success or queue full error)
- Server-side validation provides descriptive error messages in JSON response

Common issues:
- **Preview crashes:** Long messages may cause device reset during blocking playback
- **Transmission gibberish:** Check Serial output for dit/dah timing values and keyer state
- **Queue full errors:** Maximum 5 messages can be queued; wait for transmission to complete

For detailed debugging steps, see [docs/FEATURES.md - CW Memories Debugging](FEATURES.md#debugging-and-troubleshooting)

## Practice Mode Page (/practice)

**File:** `web_pages_practice.h` - Complete HTML/CSS/JavaScript for web-based practice mode

### Overview

Web Practice Mode allows users to practice morse code via their browser using a USB Vail adapter connected to their computer. The browser captures keyboard/MIDI events from the Vail adapter, sends timing data to the Summit device via WebSocket, and displays decoded morse code in real-time.

### Features

**1. Real-time Decoder**
- Adaptive morse decoder runs on Summit device
- Decoder reuses existing `morse_decoder_adaptive.h` implementation
- WPM tracking with color-coded indicator (green = matching configured speed)
- Decoded text appears in browser window as you key

**2. Status Bar**
- Connection indicator (green pulsing dot when connected)
- Real-time WPM display updates as you send
- Connection status text (Connected/Disconnected)

**3. Decoded Output Area**
- Dark terminal-style text display
- Shows morse patterns and decoded characters
- Scrollable output with word spacing detection
- Auto-scrolls to bottom as new text arrives

**4. Keyboard Input Support**
- **Left Control** = Dit paddle
- **Right Control** = Dah paddle
- Compatible with Vail USB adapter in keyboard mode
- JavaScript captures `keydown`/`keyup` events with millisecond precision

**5. Future MIDI Support** (documented for implementation)
- Web MIDI API integration (Chrome/Edge/Opera/Brave only)
- Direct MIDI Note On/Off events from Vail adapter
- Sub-millisecond timing precision
- See `docs/WEB_MIDI_PROTOCOL.md` for implementation details

### Architecture

**Hybrid Client-Server Design:**

```
Browser (Computer + Vail Adapter)
├── JavaScript Event Capture (Left/Right Ctrl keydown/up)
├── Display UI (Decoded text, WPM, status)
└── WebSocket Client (Send timing, receive decoded text)
        ↕ WebSocket
VAIL SUMMIT Device
├── WebSocket Server (/ws/practice)
├── morse_decoder_adaptive.h (Reused decoder)
└── Device Display (Static "Web Practice Mode Active")
```

### WebSocket Protocol

**Endpoint:** `ws://vail-summit.local/ws/practice`

**Client → Device Messages:**

```json
{
  "type": "timing",
  "duration": 123.4,     // milliseconds
  "positive": true,      // true = tone, false = silence
  "key": "dit"           // "dit" or "dah" (informational)
}
```

**Device → Client Messages:**

```json
// Decoded character
{
  "type": "decoded",
  "morse": ".-",         // Morse pattern
  "text": "A"            // Decoded character
}

// WPM update
{
  "type": "wpm",
  "value": 22.5          // Detected speed in WPM
}
```

### API Endpoints

**`POST /api/practice/start`**
- Switches Summit device to `MODE_WEB_PRACTICE`
- Browser calls this before connecting WebSocket
- Returns: `{"status": "active", "endpoint": "ws://vail-summit.local/ws/practice"}`

**GET `/practice`**
- Serves practice mode HTML page
- Returns: `PRACTICE_PAGE_HTML` from PROGMEM

### User Workflow

1. User opens `http://vail-summit.local/practice` in browser
2. Connects USB Vail adapter to computer
3. Clicks "Start Practice" button
   - Browser sends `POST /api/practice/start`
   - Device switches to `MODE_WEB_PRACTICE`
   - Device display shows "Web Practice Mode Active"
4. Browser opens WebSocket connection to `/ws/practice`
5. User keys morse code with paddle
   - Vail adapter generates sidetone (instant audio feedback)
   - Browser captures Left/Right Ctrl key events
   - JavaScript calculates duration between keydown/keyup
   - Sends timing data to device via WebSocket
6. Device decoder processes timing
   - Adaptive algorithm detects speed and decodes characters
   - Callbacks send decoded text and WPM to browser
7. Browser displays decoded text in scrolling output area
8. To exit:
   - Press ESC on Summit device keypad, OR
   - Close browser tab (WebSocket disconnect triggers exit)

### Implementation Files

**Web Components:**
- `web_pages_practice.h` - HTML/CSS/JavaScript page (~120 lines)
- `web_server.h` - WebSocket handlers and API endpoint (~90 lines)

**Device Components:**
- `web_practice_mode.h` - Mode handler and decoder integration
- `menu_ui.h` - `MODE_WEB_PRACTICE` enum definition
- `menu_navigation.h` - Input routing for ESC key
- `vail-summit.ino` - Main loop updates for WebSocket cleanup

**Storage Impact:** ~30KB total (< 1% of 4MB flash)

### Advantages Over Device Practice Mode

**Benefits:**
- ✅ Practice remotely (device can be across room for viewing)
- ✅ Larger display (computer monitor vs 240px LCD)
- ✅ Easier to read decoded text
- ✅ Session logging possible (browser localStorage)
- ✅ Mobile device support (touch keyer for phones/tablets)

**Trade-offs:**
- ⚠️ Requires WiFi connection (battery drain)
- ⚠️ 10-50ms network latency for display (audio is instant via Vail adapter)
- ⚠️ Keyboard input requires browser focus
- ⚠️ Not a replacement for hardware paddle practice (different feel)

### Future Enhancements

See `docs/WEB_MIDI_PROTOCOL.md` for:
- Web MIDI API integration for sub-millisecond timing
- Direct Vail adapter settings control (key type, speed, tone)
- MIDI passthrough mode configuration
- Browser compatibility fallbacks

## ADIF Export Format

### Header

```
ADIF Export from VAIL SUMMIT
<PROGRAMID:11>VAIL SUMMIT
<PROGRAMVERSION:5>1.0.0
<ADIF_VER:5>3.1.4
<EOH>
```

### QSO Records

```
<CALL:5>W1ABC <FREQ:6>14.025 <MODE:2>CW <QSO_DATE:8>20251028
<TIME_ON:6>143000 <RST_SENT:3>599 <RST_RCVD:3>599
<GRIDSQUARE:6>FN31pr <MY_GRIDSQUARE:6>EN82xx
<MY_SIG:4>POTA <MY_SIG_INFO:7>US-2256 <EOR>
```

### POTA Support

Uses ADIF 3.1.4 special event tags:
- `<MY_SIG:4>POTA <MY_SIG_INFO:7>US-2256` - Operator's POTA activation
- `<SIG:4>POTA <SIG_INFO:7>US-2254` - Contact's POTA activation
- Compatible with QRZ, LoTW, POTA upload

## Common Development Patterns

### Adding a New API Endpoint

**GET Endpoint:**
```cpp
webServer.on("/api/myendpoint", HTTP_GET, [](AsyncWebServerRequest *request) {
  JsonDocument doc;
  doc["data"] = "value";
  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
});
```

**POST Endpoint with JSON Body:**
```cpp
webServer.on("/api/myendpoint", HTTP_POST,
  [](AsyncWebServerRequest *request) {},
  NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    JsonDocument doc;
    deserializeJson(doc, data, len);
    // Process doc...
    request->send(200, "application/json", "{\"success\":true}");
  });
```

## Critical Implementation Notes

1. **Always initialize QSO structs:** `memset(&qso, 0, sizeof(QSO))` before populating fields to prevent garbage data causing JSON parsing errors

2. **Band auto-calculation:** Web interface doesn't send band field; server calculates it via `frequencyToBand(frequency)`

3. **Map marker updates:** Call `updateMapMarkers()` after loading QSOs to refresh map pins

4. **File operations:** All file writes are atomic (read→modify→write) to prevent corruption

5. **Error handling:** Web API returns JSON error objects: `{"success":false,"error":"message"}`

## Troubleshooting

**"Failed to load QSOs: SyntaxError: Bad control character"**
- Cause: QSO struct not initialized before populating
- Fix: Add `memset(&newQSO, 0, sizeof(QSO))` before field assignments
- Location: `web_server.h` create/update endpoints

**Map shows no pins:**
- Check browser console for "QSOs with grids today" count
- Verify QSOs have valid grid squares (4+ characters)
- Ensure date format matches (YYYYMMDD)
- Call `updateMapMarkers()` after QSO changes

**Station settings not saving:**
- Check Preferences namespace: "qso_operator"
- Verify WiFi connected (required for web access)
- Check serial monitor for save confirmation

**Cannot access web server:**
- Verify WiFi connected (check device WiFi status icon)
- Try direct IP if mDNS fails: `http://192.168.1.xxx/`
- Check serial monitor for "Web server started" message
- Ensure port 80 not blocked by firewall
