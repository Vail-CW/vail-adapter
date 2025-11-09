# Development Guide

This document covers development patterns, critical constraints, and troubleshooting for VAIL SUMMIT development.

## Development Patterns

### Adding a New Menu Mode

1. **Add enum value to `MenuMode`** in morse_trainer_menu.ino (around line 32-44)
2. **Create header file** (e.g., `training_newmode.h`) with:
   - State variables
   - UI drawing functions
   - Input handler function
3. **Add mode to menu arrays** (options and icons)
4. **Update `selectMenuItem()`** to handle selection
5. **Update `handleKeyPress()`** to route input to your handler
6. **Update `drawMenu()`** to call your UI renderer
7. **Add Preferences namespaces** for persistent settings (if needed)

### Creating Audio Feedback

```cpp
beep(frequency_hz, duration_ms);  // For short beeps/tones (blocking)
startTone(frequency_hz);          // For continuous tone (manual stop)
continueTone();                   // Maintain tone with phase continuity
stopTone();                       // Stop continuous tone
```

**Important:** Always use these functions instead of direct I2S manipulation. They handle volume scaling and phase continuity.

### Display Optimization

**Best Practices:**
- Only redraw what changes (use return codes from input handlers)
- Disable all display updates during audio-critical operations (practice mode, morse playback)
- Use `fillRect()` to clear regions before redrawing text
- Cache text bounds with `getTextBounds()` for centering

**Return Codes from Input Handlers:**
- `-1` - Exit mode (return to parent menu)
- `0` or `1` - Normal input processed
- `2` - Full UI redraw requested
- `3` - Partial UI update (e.g., input box only)

### Morse Code Generation

```cpp
#include "morse_code.h"

// Get morse pattern for a character
const char* pattern = getMorseCode('A');  // Returns ".-"

// Create timing calculator for specific WPM
MorseTiming timing(20);  // 20 WPM timing calculator
// Available properties:
// - timing.ditDuration
// - timing.dahDuration
// - timing.elementGap
// - timing.letterGap
// - timing.wordGap

// Play string as morse code
playMorseString("HELLO WORLD");  // Automatic playback with proper spacing
```

### Working with Preferences

```cpp
#include <Preferences.h>

Preferences prefs;

// Save settings
void saveMySettings() {
  prefs.begin("my_namespace", false);  // false = read/write mode
  prefs.putInt("setting1", value1);
  prefs.putString("setting2", value2);
  prefs.end();
}

// Load settings
void loadMySettings() {
  prefs.begin("my_namespace", true);  // true = read-only mode
  int value1 = prefs.getInt("setting1", default_value);
  String value2 = prefs.getString("setting2", "default");
  prefs.end();
}
```

**Best Practices:**
- Load settings on startup
- Save immediately when changed (don't batch writes)
- Use consistent namespace and key names
- Provide sensible defaults for missing values

### Mode Implementation Template

```cpp
// In your_mode.h

// State variables
static int myModeState = 0;
static String myModeInput = "";

// Start function - called when entering mode
void startMyMode(Adafruit_ST7789& tft) {
  // Initialize state
  myModeState = 0;
  myModeInput = "";

  // Draw initial UI
  tft.fillScreen(COLOR_BACKGROUND);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("My Mode");

  // ... additional UI setup
}

// Input handler - called when key pressed
int handleMyModeInput(char key, Adafruit_ST7789& tft) {
  // ESC exits mode
  if (key == 0x1B) {
    return -1;  // Return to parent menu
  }

  // Process other keys
  if (key == 0x0D) {  // ENTER
    // Do something
    return 2;  // Request full redraw
  }

  // Normal input processing
  return 0;  // Input processed
}

// Update function - called every loop iteration (optional)
void updateMyMode() {
  // Perform real-time updates
  // Must be non-blocking!

  // Example: check state and update as needed
  if (myModeState == 1) {
    // Do something
  }
}
```

### Non-Blocking Code Patterns

**Good (Non-Blocking):**
```cpp
static unsigned long lastUpdate = 0;

void updateMyMode() {
  unsigned long now = millis();

  if (now - lastUpdate >= 1000) {  // Update every second
    lastUpdate = now;
    // Do update
  }
}
```

**Bad (Blocking):**
```cpp
void updateMyMode() {
  delay(1000);  // DON'T DO THIS - blocks entire system
  // Do update
}
```

## Critical Constraints

### 1. Never Use analogRead(A3) or analogRead(15)

**Why:** GPIO 15 (A3) is used by I2S for the LRC clock signal. Any `analogRead(A3)` call reconfigures the pin and **completely breaks I2S audio output**.

**Impact:** No audio, system may crash
**Workaround:** None - USB charging detection disabled for this reason

### 2. Initialize I2S Before Display

**Why:** I2S DMA needs higher interrupt priority than SPI display DMA

**Correct Order:**
```cpp
void setup() {
  setupI2SAudio();  // FIRST
  setupDisplay();   // SECOND
}
```

**Impact if wrong:** Audio glitches, crackling, or complete audio failure

### 3. No Display Updates During Audio Playback

**Why:** Display SPI DMA interferes with I2S audio DMA, causing glitches

**In Practice/Training Modes:**
- Display updates completely disabled during keying
- UI updates only when tone is NOT playing
- Partial screen updates preferred (e.g., `drawDecodedTextOnly()`)

**Example:**
```cpp
void updatePracticeOscillator() {
  // Check keyer state, update audio

  // Only update display when NOT playing audio
  if (!toneIsPlaying() && needsUIUpdate) {
    drawDecodedTextOnly();  // Partial update
    needsUIUpdate = false;
  }
}
```

### 4. Always Use beep() or I2S Functions for Audio

**Why:** Direct GPIO manipulation of GPIO 5 (repurposed from buzzer pin) won't produce audio and may interfere with capacitive touch

**Correct:**
```cpp
beep(600, 100);        // 600 Hz beep for 100ms
startTone(700);        // Start 700 Hz continuous tone
continueTone();        // Maintain tone (phase continuity)
stopTone();           // Stop tone
```

**Incorrect:**
```cpp
digitalWrite(5, HIGH);  // DON'T - GPIO 5 is now touch pad, not buzzer
```

### 5. Load Preferences at Startup, Save Immediately on Change

**Why:** Prevent data loss, ensure consistency across modes

**Good:**
```cpp
void setup() {
  loadCWSettings();    // Load on startup
  loadWiFiSettings();
}

void onSettingChanged() {
  cwSpeed = newSpeed;
  saveCWSettings();    // Save immediately
}
```

**Bad:**
```cpp
void onSettingChanged() {
  cwSpeed = newSpeed;
  // Don't save until later - WRONG
}
```

### 6. WebSocket Handling Must Be Non-Blocking

**Why:** Blocking WebSocket operations freeze entire system

**Use State Machine Pattern:**
```cpp
enum VailState {
  VAIL_DISCONNECTED,
  VAIL_CONNECTING,
  VAIL_CONNECTED,
  VAIL_ERROR
};

static VailState vailState = VAIL_DISCONNECTED;

void updateVailRepeater() {
  switch (vailState) {
    case VAIL_DISCONNECTED:
      // Start connection (non-blocking)
      break;
    case VAIL_CONNECTING:
      // Check connection status
      break;
    case VAIL_CONNECTED:
      // Process messages
      break;
  }
}
```

### 7. Always Initialize QSO Structs

**Why:** Uninitialized memory contains garbage data, causing JSON parsing errors

**Correct:**
```cpp
QSO newQSO;
memset(&newQSO, 0, sizeof(QSO));  // CRITICAL - zero out struct first

// Now populate fields
strcpy(newQSO.callsign, "W1ABC");
newQSO.frequency = 14.025;
```

**Incorrect:**
```cpp
QSO newQSO;  // Contains garbage data
strcpy(newQSO.callsign, "W1ABC");  // Other fields have random values
```

## Audio System Details

### Phase Continuity

**Why `continueTone()` exists:**
When switching between dit and dah elements in iambic keying, stopping and restarting the tone causes audio clicks. `continueTone()` maintains phase continuity for smooth audio.

**Usage:**
```cpp
startTone(cwTone);        // Start initial tone
// ... element playing
continueTone();           // Transition to next element (no click)
// ... element playing
stopTone();              // Final stop
```

### Volume Scaling

Volume is applied during sample generation in `i2s_audio.h`:

```cpp
// Volume range: 0-100
// Sample scaling: sample = sample * (volume / 100.0)
```

**Settings:**
- Saved in Preferences namespace "audio"
- Global variable: `audioVolume`
- Function: `setVolume(percent)`

### Audio Priority

During audio-critical modes (Practice, Morse Shooter, Radio Output):
- Keyboard polling slowed to 50ms (from 10ms)
- Display updates disabled during keying
- Main loop delay reduced to 1ms (from 10ms in radio mode)

## Display System Details

### Screen Rotation

Display is rotated to landscape orientation:

```cpp
#define SCREEN_ROTATION 1  // 0=portrait, 1=landscape, 2=portrait flip, 3=landscape flip
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
```

### Color Scheme

Colors defined in `config.h`:

```cpp
#define COLOR_BACKGROUND 0x0000    // Black
#define COLOR_TEXT 0xFFFF          // White
#define COLOR_HEADER 0x1082        // Dark blue
#define COLOR_ACCENT 0x07FF        // Cyan
#define COLOR_SUCCESS 0x07E0       // Green
#define COLOR_ERROR 0xF800         // Red
```

### Text Drawing Optimization

**Center text horizontally:**
```cpp
int16_t x1, y1;
uint16_t w, h;
const char* text = "Hello";
tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
int centerX = (SCREEN_WIDTH - w) / 2;
tft.setCursor(centerX, y);
tft.print(text);
```

**Clear specific region:**
```cpp
// Clear before redrawing to avoid ghosting
tft.fillRect(x, y, width, height, COLOR_BACKGROUND);
```

## Troubleshooting

### Audio distortion/clicking

**Symptoms:** Crackling, popping, or distorted audio

**Possible Causes:**
1. I2S initialized after display - Fix: Initialize I2S first in `setup()`
2. Display updates during audio playback - Fix: Disable display updates during keying
3. Volume clipping - Fix: Reduce volume to ≤90%

**Debug:**
```cpp
Serial.println("I2S initialized");
Serial.println("Display initialized");
// Verify order
```

### WiFi connection fails

**Symptoms:** Cannot connect to WiFi network

**Possible Causes:**
1. Wrong credentials
2. Network out of range
3. Saved credentials corrupted

**Debug:**
```cpp
// Check serial monitor for:
// - SSID/password echo
// - Connection status
// - Error messages
```

**Fix:**
1. Use Settings → WiFi Setup to scan and save credentials
2. Check serial monitor for connection status
3. Reset saved credentials if corrupted (Settings → WiFi Setup → Long press R)

### Battery percentage inaccurate

**Symptoms:** Battery shows wrong percentage

**Possible Causes:**
1. Fuel gauge not calibrated
2. Wrong battery monitor detected

**Fix:**
- MAX17048/LC709203F requires calibration over several charge/discharge cycles
- Initial readings may be lower than actual charge
- Check serial monitor for battery monitor detection (`0x36` = MAX17048, `0x0B` = LC709203F)

### Keys not responding

**Symptoms:** CardKB or paddle input ignored

**Possible Causes:**
1. I2C device not detected
2. Wrong I2C address
3. Wiring issue

**Debug:**
```cpp
// Check serial monitor for I2C scan results:
// - 0x5F = CardKB keyboard
// - 0x36 = MAX17048 battery monitor
// - 0x0B = LC709203F battery monitor
```

**Fix:**
1. Verify I2C wiring (SDA=GPIO3, SCL=GPIO4)
2. Check CardKB address (should be 0x5F)
3. Test with I2C scanner sketch

### Display freezes

**Symptoms:** Screen stops updating, device unresponsive

**Possible Causes:**
1. Blocking code in main loop
2. Infinite loop in mode handler
3. Long-running operation

**Fix:**
1. Move long operations to separate update functions
2. Use state machines for multi-step operations
3. Add watchdog timer if needed
4. Check serial monitor for last executed code

### Web server inaccessible

**Symptoms:** Cannot access web interface

**Possible Causes:**
1. WiFi not connected
2. mDNS not working
3. Port 80 blocked

**Debug:**
```cpp
// Check serial monitor for:
// - "Web server started" message
// - WiFi connection status
// - Device IP address
```

**Fix:**
1. Verify WiFi connected (check device WiFi status icon)
2. Try direct IP if mDNS fails: `http://192.168.1.xxx/`
3. Check serial monitor for "Web server started" message
4. Ensure port 80 not blocked by firewall

### QSO logging errors

**Symptoms:** QSO save fails, JSON parsing errors

**Possible Causes:**
1. QSO struct not initialized - **MOST COMMON**
2. Invalid field values
3. SPIFFS full or corrupted

**Fix:**
```cpp
// Always initialize QSO struct before populating
QSO newQSO;
memset(&newQSO, 0, sizeof(QSO));  // CRITICAL

// Now safe to populate fields
strcpy(newQSO.callsign, "W1ABC");
```

**Debug:**
- Check serial monitor for JSON parsing errors
- Verify SPIFFS has free space (System Info page)
- Check field validation (callsign length, frequency range, etc.)

### Capacitive touch not working

**Symptoms:** Touch pads don't register touches

**Possible Causes:**
1. Using T-constants instead of GPIO numbers (ESP32-S3 bug)
2. Wrong threshold value
3. Checking wrong direction (< instead of >)

**Fix:**
```cpp
// CORRECT - Use GPIO numbers directly
int ditValue = touchRead(8);   // GPIO 8, not T8
int dahValue = touchRead(5);   // GPIO 5, not T5

// CORRECT - Touch values RISE when touched on ESP32-S3
if (ditValue > TOUCH_THRESHOLD) {  // NOT < threshold
  // Touch detected
}
```

**Debug:**
```cpp
// Print raw touch values to serial monitor
Serial.print("DIT: ");
Serial.print(touchRead(8));
Serial.print(" DAH: ");
Serial.println(touchRead(5));
// Adjust TOUCH_THRESHOLD based on values seen
```

### CW Memories transmission issues

**Symptoms:** Memories play as "random dits" or gibberish on radio output

**Possible Causes:**
1. `radioDitDuration` corrupted or zero in Summit Keyer mode
2. Conflict between `playMorseCharViaRadio()` blocking delays and `radioIambicKeyerHandler()` state machine
3. Both functions controlling same GPIO pins simultaneously

**Current Mitigation:**
```cpp
// In processRadioMessageQueue() - wait for keyer to be idle
if (radioMode == RADIO_MODE_SUMMIT_KEYER) {
  if (ditPressed || dahPressed || radioKeyerActive || radioInSpacing) {
    return; // Wait for keyer to be completely idle
  }
}
```

**Debug Steps:**
1. Enable Serial Monitor at 115200 baud
2. Create simple test memory (e.g., "TEST" → "SOS")
3. Check serial output for:
   ```
   Loading CW memory 1: TEST → SOS
   Previewing memory 1: SOS (length: 3)
   Queueing message for radio: SOS (length: 3)
   Transmitting from queue: SOS (length: 3)
   Dit duration: 60ms (should be non-zero)
   ```
4. Compare behavior:
   - **Summit Keyer mode:** Check if manual keying also produces gibberish
   - **Radio Keyer mode:** Check if manual keying works (should passthrough correctly)
5. If `radioDitDuration` is 0 or corrupt, timing will be instant/rapid

**Known Issues:**
- Summit Keyer mode may produce incorrect timing if `radioDitDuration` not calculated correctly
- Preview function may crash device halfway through long messages (blocking audio playback)
- Memory transmission may sound different than preview if keyer state machine interferes

**Workarounds:**
1. Use Radio Keyer mode for memory transmission (relies on radio's keyer, not device)
2. Keep preview messages short to avoid crashes
3. Monitor Serial output to verify message content and timing values

## Performance Optimization

### Memory Management

**Heap Fragmentation:**
- Avoid frequent malloc/free
- Use static buffers where possible
- Prefer stack allocation for small objects

**PSRAM Usage:**
- ESP32-S3 has 2MB PSRAM
- Use for large buffers (audio, display, QSO storage)

**Monitor Memory:**
```cpp
Serial.print("Free heap: ");
Serial.println(ESP.getFreeHeap());
Serial.print("Free PSRAM: ");
Serial.println(ESP.getFreePsram());
```

### CPU Usage

**Avoid Blocking:**
- Use millis() for timing instead of delay()
- State machines for multi-step operations
- Non-blocking I/O (AsyncWebServer, non-blocking WiFi)

**Optimize Hot Paths:**
- Audio generation (interrupt context)
- Iambic keyer state machine (called every loop)
- Display updates (use partial updates)

### Storage Optimization

**SPIFFS Best Practices:**
- One QSO log file per date (not one per QSO)
- JSON format (human-readable, parseable)
- Atomic file writes (read→modify→write)

**Monitor Storage:**
```cpp
Serial.print("SPIFFS used: ");
Serial.print(SPIFFS.usedBytes());
Serial.print(" / ");
Serial.println(SPIFFS.totalBytes());
```

## Testing Checklist

Before deploying firmware:

**Audio System:**
- [ ] Beep sounds at various frequencies (400-1200 Hz)
- [ ] Volume control works (0-100%)
- [ ] No clicks/pops during iambic keying
- [ ] Decoder shows correct WPM

**Input Devices:**
- [ ] CardKB keys respond
- [ ] Physical paddle works (DIT and DAH)
- [ ] Capacitive touch works (DIT and DAH)
- [ ] Arrow keys navigate menus

**WiFi:**
- [ ] Auto-connect to saved network
- [ ] Manual connect works
- [ ] AP mode creates network
- [ ] Web server accessible

**Web Interface:**
- [ ] Dashboard loads
- [ ] QSO logger works (create/edit/delete)
- [ ] Settings save correctly
- [ ] ADIF/CSV export valid

**Training Modes:**
- [ ] Practice mode works
- [ ] CW Academy navigation works
- [ ] Copy practice scoring works
- [ ] Decoder displays text

**Games:**
- [ ] Morse Shooter loads
- [ ] Letters fall and spawn
- [ ] Shooting works
- [ ] Score persists

**Radio Output:**
- [ ] Radio mode switches
- [ ] Keying outputs to GPIO pins
- [ ] Message queue works
- [ ] Web remote control works

## Common Gotchas

1. **GPIO 15 (A3) cannot be used for analog input** - breaks I2S audio
2. **Touch values RISE on ESP32-S3** - opposite of ESP32 classic
3. **Must use GPIO numbers in touchRead()** - T-constants don't work
4. **QSO struct must be zeroed** - prevents JSON parsing errors
5. **I2S before display initialization** - DMA priority order
6. **No display updates during audio** - causes glitches
7. **WebSocket must be non-blocking** - use state machine
8. **Preferences save immediately** - don't batch writes
