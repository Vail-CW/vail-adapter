# POTA & Location Enhancement Proposal

## Overview

Add comprehensive location tracking and POTA (Parks on the Air) integration to the QSO logger.

## 1. Operator Location Settings

### New Fields in Operator Settings

```cpp
// Add to qso_logger.h - Operator Settings
char operatorGridsquare[9];    // Maidenhead grid (e.g., "EN52wa")
char operatorQTH[41];           // Location description (e.g., "Chicago, IL")
char operatorPOTARef[11];       // Current POTA park activating (e.g., "K-0817")
char operatorPOTAName[61];      // Park name (auto-filled from API)
```

### Settings UI Screen

New settings screen: **Settings → Location**

Fields to configure:
1. **Grid Square** - Manual entry (validated format: 2 letters + 2 digits + optional 2 letters)
2. **QTH** - Free text location description
3. **POTA Park** - Optional, lookup from API or manual entry

### How It Works

- Settings saved in Preferences namespace `"qso_operator"`
- Values automatically populate into every new QSO log entry
- Can be changed before each logging session (e.g., when traveling)
- POTA park cleared after each session (not persistent by default)

## 2. POTA Integration

### POTA API Details

**Base URL:** `https://api.pota.app/`

**Key Endpoints:**

1. **Get Park Details**
   ```
   GET /park/{reference}
   Example: https://api.pota.app/park/K-0817

   Response:
   {
     "reference": "K-0817",
     "name": "Indiana Dunes National Park",
     "locationDesc": "IN, US",
     "latitude": 41.6533,
     "longitude": -87.0524,
     "grid4": "EN61",
     "grid6": "EN61am",
     "parktypeId": 1,
     "parktypeDesc": "US National Park",
     "active": 1,
     "entityId": 291
   }
   ```

2. **Find Parks Near Location**
   ```
   GET /location/parks/{grid}
   Example: https://api.pota.app/location/parks/EN52

   Response: Array of parks near grid square
   ```

3. **Live Activator Spots** (future use)
   ```
   GET /spot/activator
   Returns: Recent POTA activations
   ```

### QSO Struct Changes

**Add to QSO struct (qso_logger.h):**

```cpp
struct QSO {
  // ... existing fields ...

  // Operator location (from settings)
  char operator_grid[9];        // Operator's Maidenhead grid
  char operator_qth[41];         // Operator's location
  char operator_pota_ref[11];    // Park operator is activating
  char operator_pota_name[61];   // Park name (display/export)

  // Contact's POTA info (optional)
  char their_pota_ref[11];       // Park they're activating (if any)
  char their_pota_name[61];      // Their park name
};
```

### Log Entry Form Enhancement

**New optional fields in log entry:**

1. **Their POTA Ref** - Enter park reference they're activating
   - Auto-lookup name from API when reference entered
   - Shows "(P2P)" indicator in UI if both stations are activating

2. **My POTA Park** - Quick override/set for current activation
   - Defaults to operator settings value
   - Can change per-QSO if needed

### POTA API Module

**New file:** `pota_api.h`

```cpp
// POTA API integration
struct POTAPark {
  char reference[11];
  char name[61];
  char locationDesc[41];
  char grid4[5];
  char grid6[7];
  float latitude;
  float longitude;
  bool valid;
};

// Functions:
bool lookupPOTAPark(const char* reference, POTAPark& park);
bool findNearbyParks(const char* grid, POTAPark parks[], int& count);
bool validatePOTAReference(const char* reference);
```

**Implementation:**
- Use WiFi + HTTPClient library
- 5 second timeout for API calls
- Fallback: Allow manual entry if API unavailable
- Cache last lookup to avoid repeated API calls

## 3. UI/UX Flow

### Scenario 1: Setting Your Location

1. Go to **Settings → Location**
2. Enter grid square: `EN52wa`
3. Enter QTH: `Chicago, IL`
4. Optionally set POTA park: `K-0817`
   - Device calls API: `GET /park/K-0817`
   - Shows: "Indiana Dunes NP" (confirmation)
   - Saves reference + name
5. Settings saved, applied to all future QSOs

### Scenario 2: Logging a Contact (POTA Activation)

1. **New Log Entry** screen
2. Your info auto-filled from settings:
   - Operator: `KE9BOS @ K-0817 (Indiana Dunes NP)`
3. Enter their callsign: `W1AW`
4. Optional: Enter their POTA ref: `K-1234`
   - Device calls API: `GET /park/K-1234`
   - Shows: "Acadia NP" (confirmation)
   - Displays "(P2P)" badge (Park-to-Park contact!)
5. Save QSO with both locations recorded

### Scenario 3: Logging Without WiFi

- POTA lookup unavailable (no WiFi)
- Can still manually enter POTA reference
- Device stores reference without name
- Name can be filled later via web interface or ADIF import

## 4. ADIF Export Enhancement

ADIF fields to add:

**Operator Location:**
- `MY_GRIDSQUARE` - Your grid square
- `MY_CITY` - Your QTH/city
- `MY_SIG` - Special Interest Group = "POTA"
- `MY_SIG_INFO` - Your POTA park reference

**Contact's Location:**
- `GRIDSQUARE` - Their grid (existing field)
- `SIG` - "POTA" if they're activating
- `SIG_INFO` - Their POTA park reference

**Example ADIF:**
```
<CALL:5>W1AW <QSO_DATE:8>20251028 <TIME_ON:4>1234
<BAND:3>20m <MODE:2>CW <RST_SENT:3>599 <RST_RCVD:3>599
<MY_GRIDSQUARE:6>EN52wa <MY_CITY:11>Chicago, IL
<MY_SIG:4>POTA <MY_SIG_INFO:6>K-0817
<SIG:4>POTA <SIG_INFO:6>K-1234
<EOR>
```

## 5. Implementation Phases

### Phase A: Basic Location (No POTA)
**Time: 1-2 hours**

1. Add operator location fields to settings
2. Create Settings → Location screen
3. Add grid/QTH fields to QSO struct
4. Auto-populate operator location in log entry
5. Include in ADIF export

**No API calls, just manual entry**

### Phase B: POTA Integration
**Time: 3-4 hours**

1. Create `pota_api.h` module
2. Add POTA fields to QSO struct
3. Implement API lookup functions
4. Add POTA reference input to log entry form
5. Auto-lookup park names when WiFi available
6. Update ADIF export with POTA fields
7. Display P2P indicator when both activating

### Phase C: Advanced Features (Future)
**Time: 2-3 hours**

1. GPS integration (if GPS module added)
2. Auto-calculate grid square from GPS
3. Nearby park lookup based on grid
4. Live POTA spot integration
5. Statistics: Show POTA activations count

## 6. Data Structure Summary

### Operator Settings (Preferences)
```cpp
// Namespace: "qso_operator"
char operatorCallsign[11];      // Existing
char operatorGridsquare[9];     // NEW
char operatorQTH[41];           // NEW
char operatorPOTARef[11];       // NEW
char operatorPOTAName[61];      // NEW
```

### QSO Struct (per contact)
```cpp
struct QSO {
  // Existing fields: callsign, freq, mode, band, rst, date, time, notes...

  // NEW - Operator location
  char operator_grid[9];
  char operator_qth[41];
  char operator_pota_ref[11];
  char operator_pota_name[61];

  // NEW - Contact's location
  char their_pota_ref[11];
  char their_pota_name[61];

  // Existing: gridsquare, qth, country, state (for contacted station)
};
```

## 7. Validation Rules

**Grid Square Format:**
- 2 letters (field) + 2 digits (square) + optional 2 letters (subsquare)
- Examples: `EN52`, `EN52wa`, `FN31pr`
- Case insensitive, stored uppercase

**POTA Reference Format:**
- Country prefix + dash + number
- Examples: `K-0817`, `VE-0123`, `G-0456`
- Validated against API if WiFi available

## 8. Benefits

1. **Complete Log Records** - Every QSO includes your location
2. **POTA Credit** - Proper logging for POTA activations
3. **P2P Detection** - Automatically recognize Park-to-Park contacts
4. **ADIF Compliance** - Exports work with QRZ, LoTW, ClubLog
5. **Portable** - Works offline (manual entry) or online (API lookup)

## Questions to Decide

1. **Should operator POTA park persist across reboots?**
   - Option A: Clear it each reboot (assume single activation session)
   - Option B: Keep it until manually changed
   - **Recommendation:** Clear it (most activations are single-session)

2. **Should we add a "POTA Mode" toggle?**
   - Enables POTA-specific UI enhancements
   - Shows P2P counter, activation stats
   - **Recommendation:** Yes, but Phase C (future)

3. **GPS module support?**
   - Auto-calculate grid square from GPS coordinates
   - Requires hardware addition (GPS module via UART or I2C)
   - **Recommendation:** Phase C if user wants to add GPS

4. **Should we cache park lookups?**
   - Store recent park lookups in SPIFFS
   - Avoid repeated API calls for same parks
   - **Recommendation:** Yes, simple LRU cache of 20 parks

## Recommended Approach

**Start with Phase A (Basic Location):**
- Quick to implement (1-2 hours)
- No WiFi dependency
- Immediately useful
- Foundation for POTA features

**Then add Phase B (POTA Integration):**
- Requires WiFi but has fallback
- Significant value for POTA operators
- Professional-grade logging

**This gives you:**
- Complete location tracking
- POTA activation logging
- Proper ADIF exports
- All within ~5 hours of work
