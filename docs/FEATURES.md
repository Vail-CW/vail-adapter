# Features

This document covers detailed documentation for major features including CW Academy, Morse Shooter, Radio Mode, and Morse Decoder.

## CW Academy Training Mode

### Overview

The CW Academy mode implements the official CW Academy curriculum across **four training tracks**. Each track is a comprehensive 16-session program with progressive difficulty.

### Training Tracks

1. **Beginner** - Learn CW from zero (4 → 44 characters over Sessions 1-10)
2. **Fundamental** - Build solid foundation (assumes basic CW knowledge)
3. **Intermediate** - Increase speed & skill (higher WPM targets)
4. **Advanced** - Master advanced CW techniques

### Module Architecture: `training_cwa.h`

**Track Structure:**
```cpp
enum CWATrack {
  TRACK_BEGINNER = 0,
  TRACK_FUNDAMENTAL = 1,
  TRACK_INTERMEDIATE = 2,
  TRACK_ADVANCED = 3
};
```

**Session Data Structure:**
```cpp
struct CWASession {
  int sessionNum;           // Session number (1-16)
  int charCount;            // Total characters learned by this session
  const char* newChars;     // New characters introduced
  const char* description;  // Session description
};
```

### Beginner Track Session Progression

**Sessions 1-10:** Progressive character introduction (4 chars → 44 chars)
- Session 1: A, E, N, T (4 chars) - Foundation
- Session 2: + S, I, O, 1, 4 (9 chars) - Numbers Begin
- Session 10: + X, Z, ., <BK>, <SK> (44 chars) - Complete!

**Sessions 11-13:** QSO (conversation) practice with all 44 characters

**Sessions 14-16:** On-air preparation and encouragement

### Practice Types

```cpp
enum CWAPracticeType {
  PRACTICE_COPY = 0,           // Listen and type what you hear
  PRACTICE_SENDING = 1,        // Send with physical key
  PRACTICE_DAILY_DRILL = 2     // Warm-up drills
};
```

**Practice Type Locking:**
- **Sessions 1-10:** Only Copy Practice available (advanced types locked)
- **Sessions 11+:** All practice types unlocked
- Visual lock indicators with "Unlocks at Session 11" hint
- Error beep feedback when attempting to access locked content

### Message Types

```cpp
enum CWAMessageType {
  MESSAGE_CHARACTERS = 0,      // Random character practice
  MESSAGE_WORDS = 1,           // Common words
  MESSAGE_ABBREVIATIONS = 2,   // CW abbreviations (73, QSL, etc.)
  MESSAGE_NUMBERS = 3,         // Number sequences
  MESSAGE_CALLSIGNS = 4,       // Random callsigns
  MESSAGE_PHRASES = 5          // Full sentences
};
```

### Navigation Flow

1. Training Menu → **CW Academy**
2. **Track Selection** (MODE_CW_ACADEMY_TRACK_SELECT)
3. **Session Selection** (MODE_CW_ACADEMY_SESSION_SELECT)
4. **Practice Type Selection** (MODE_CW_ACADEMY_PRACTICE_TYPE_SELECT)
5. **Message Type Selection** (MODE_CW_ACADEMY_MESSAGE_TYPE_SELECT)
6. **Copy Practice** (MODE_CW_ACADEMY_COPY_PRACTICE) - Fully implemented

### State Management

- Progress saved to ESP32 Preferences namespace "cwa"
- Current track, session, practice type, and message type persisted across reboots
- Functions: `loadCWAProgress()`, `saveCWAProgress()`
- Variables: `cwaSelectedTrack`, `cwaSelectedSession`, `cwaSelectedPracticeType`, `cwaSelectedMessageType`

### Implementation Status

**Chunk 1.1 - Track and Session Selection (Complete):**
- ✅ Menu integration: "CW Academy" added to Training menu
- ✅ Track selection screen with 4 tracks
- ✅ Session selection screen with 16 sessions (Beginner track defined)
- ✅ Two-level navigation with ESC back navigation
- ✅ Progress persistence

**Chunk 1.2 - Practice and Message Type Selection (Complete):**
- ✅ Practice type selection screen with 3 types
- ✅ Message type selection screen with 6 types
- ✅ Complete navigation flow
- ✅ Practice type locking for Sessions 1-10
- ✅ Visual lock indicators
- ✅ Error beep feedback

**Chunk 1.3 - Copy Practice Mode (Complete):**
- ✅ MODE_CW_ACADEMY_COPY_PRACTICE mode added
- ✅ Complete character sets for all 16 Beginner sessions
- ✅ 10-round copy practice sessions with score tracking
- ✅ Adjustable character count (1-10)
- ✅ Replay functionality (SPACE bar)
- ✅ Context-aware help text
- ✅ State machine (listening → input → feedback)

**Future Chunks:**
- ⏳ Session content for Fundamental/Intermediate/Advanced tracks
- ⏳ Proper word lists for word practice
- ⏳ Abbreviations content (73, QSL, QTH, etc.)
- ⏳ Number sequences
- ⏳ Realistic callsign generation
- ⏳ Common phrases and QSO exchanges
- ⏳ Sending practice mode
- ⏳ Daily Drill mode
- ⏳ ICR (Instant Character Recognition) mode
- ⏳ QSO practice mode

## Morse Shooter Game

### Overview

The Morse Shooter is an arcade-style game where falling letters descend from the top of the screen. The player uses morse code (straight key, iambic paddle, or capacitive touch) to shoot matching letters. The game uses the **adaptive morse decoder** from Practice Mode for accurate real-time decoding.

### Game Architecture: `game_morse_shooter.h`

**Game Constants:**
```cpp
#define MAX_FALLING_LETTERS 5           // Maximum simultaneous letters
#define LETTER_FALL_SPEED 1             // Pixels per update
#define LETTER_SPAWN_INTERVAL 3000      // ms between new letter spawns
#define GROUND_Y 225                    // Y position of ground
#define MAX_LIVES 5                     // Lives before game over
#define GAME_UPDATE_INTERVAL 1000       // ms between game physics updates
```

**Character Set:**
- 36 characters: E, T, I, A, N, M, S, U, R, W, D, K, G, O, H, V, F, L, P, J, B, X, C, Y, Z, Q, 0-9
- Ordered by common morse patterns (easier letters first)

### Adaptive Decoder Integration

The game uses `MorseDecoderAdaptive` (same as Practice Mode) for real-time morse code decoding:

**Decoder State:**
```cpp
MorseDecoderAdaptive shooterDecoder(20, 20, 30);  // Initial 20 WPM, buffer 30
String shooterDecodedText = "";                    // Captured decoded characters
unsigned long shooterLastStateChangeTime = 0;      // Timing state
bool shooterLastToneState = false;                 // Tone on/off tracking
unsigned long shooterLastElementTime = 0;          // Timeout tracking
```

**Key Features:**
- **Adaptive speed tracking** - Automatically adjusts to your sending speed
- **Straight key support** - Tracks actual key-down/key-up timings
- **Iambic keyer support** - Perfect dit/dah timing (Mode A/B)
- **Word gap timeout** - Uses 7-dit word gap (more forgiving than 3-dit character gap)
- **Real-time decoding** - Characters appear immediately when decoded

### Keyer Integration

**Straight Key Mode** (`KEY_STRAIGHT`):
- Uses DIT_PIN as straight key input
- Captures tone-on and silence durations
- Sends timing data to decoder via `addTiming()`
- More forgiving timeout (word gap vs character gap)

**Iambic Keyer Mode** (`KEY_IAMBIC_A` / `KEY_IAMBIC_B`):
- Full state machine: IDLE → SENDING → SPACING → IDLE
- Memory paddles for squeeze keying
- Precise WPM timing from device settings
- Decoder receives perfect dit/dah timing

**Screen Freeze During Keying:**
- Screen updates blocked while keying to prevent audio glitches
- Checks: `keyerActive`, `inSpacing`, `ditPressed`, `dahPressed`, `shooterDecodedText.length() > 0`

### Game Loop Architecture

**Dual Update System:**

1. **`updateMorseShooterInput(tft)`** - Called every main loop iteration
   - Straight key: Captures key-up/key-down timings
   - Iambic keyer: Runs state machine with memory paddles
   - Decoder: Feeds timing data, checks timeout (word gap)
   - On timeout: Flushes decoder and checks for shot

2. **`updateMorseShooterVisuals(tft)`** - Called every main loop iteration
   - Checks if keying is active
   - If keying: returns immediately (screen frozen)
   - If idle: updates game physics once per second
   - Spawns letters, updates positions, redraws HUD

**Timeout Logic (Word Gap = 7 dits):**
```cpp
float wordGapDuration = MorseWPM::wordGap(shooterDecoder.getWPM());
if (timeSinceLastElement > wordGapDuration) {
  shooterDecoder.flush();  // Decode buffered character
  checkMorseShoot(tft);    // Attempt to shoot
}
```

### Character Decoding and Shooting

**Decoding Flow:**
1. User keys morse code (straight key or iambic paddle)
2. Decoder captures timing data in real-time
3. After word gap timeout (7 dits), decoder flushes → character decoded
4. Decoder callback appends character to `shooterDecodedText`
5. `checkMorseShoot()` finds matching falling letter
6. If found: shoots letter, plays effects, clears decoded text
7. If no match: miss sound, clears decoded text

**Decoder Callback:**
```cpp
shooterDecoder.messageCallback = [](String morse, String text) {
  for (int i = 0; i < text.length(); i++) {
    shooterDecodedText += text[i];
  }
};
```

**Shooting Logic:**
- Get last decoded character from `shooterDecodedText`
- Convert to uppercase (if needed)
- Search active falling letters for match
- On match: laser/explosion animation, score +10, clear decoded text
- On miss: error beep (600 Hz), clear decoded text

**Collision Avoidance:**
- When spawning new letters, system checks existing letters
- Avoids placing new letters within 30px horizontally and 40px vertically
- Up to 20 placement attempts before giving up

### Visual Effects

**Ground Scenery (GROUND_Y = 225):**
- Houses with roofs (rectangles and triangles)
- Trees (triangles for foliage, rectangles for trunks)
- Turret at center bottom (tank-like shape with barrel)
- Retro arcade color palette

**Shooting Animations:**
1. Laser shot: Lines from turret to target (cyan/white)
2. Beep 1200 Hz for 50ms
3. Explosion: Concentric circles with radiating rays (yellow/red/white)
4. Beep 1000 Hz for 100ms
5. Clear play area and redraw all elements

**Cleanup Sequence (Critical):**
```cpp
fallingLetters[j].active = false;  // Mark inactive FIRST
drawLaserShot();                    // Visual effects
drawExplosion();
tft.fillRect(0, 42, SCREEN_WIDTH, GROUND_Y - 42, COLOR_BACKGROUND);
drawGroundScenery(tft);            // Redraw ground
drawFallingLetters(tft);           // Redraw active letters only
```

**Why order matters:** Letter must be marked inactive BEFORE redraw, or it will briefly reappear as a "ghost" after being shot.

### HUD Display

**Top Left Corner:**
- Score: Current points (10 points per letter)
- Lives: Remaining lives (red if ≤2, green otherwise)

**Bottom (Above Ground):**
- Current morse pattern being entered (cyan text, size 2)
- Cleared when pattern is empty

### Game Over and Restart

**Game Over Triggers:**
- Lives reach 0 (letters hit ground)

**Game Over Screen:**
- Large "GAME OVER" text (red)
- Final score
- High score (persisted across sessions)
- Instructions: ENTER to play again, ESC to exit

## Radio Mode

### Overview

The Radio Mode provides integration with external ham radios via 3.5mm jack outputs. It allows keying a connected radio using the Summit's paddle inputs with two distinct operating modes.

### Architecture: `radio_output.h`

**Radio Mode Types:**
```cpp
enum RadioMode {
  RADIO_MODE_SUMMIT_KEYER,   // Summit does keying logic, outputs straight key format
  RADIO_MODE_RADIO_KEYER     // Passthrough dit/dah contacts to radio's internal keyer
};
```

**Output Pins:**
- **DIT output:** GPIO 18 (A0) - `RADIO_KEY_DIT_PIN`
- **DAH output:** GPIO 17 (A1) - `RADIO_KEY_DAH_PIN`
- **Format:** 3.5mm TRS jack (Tip = Dit, Ring = Dah, Sleeve = GND)
- **Logic:** Active HIGH (pin goes HIGH when keying)

### Summit Keyer Mode

Device performs all keying logic internally and outputs a straight key format signal:

**Straight Key:**
- DIT pin outputs key-down/key-up timing
- DAH pin remains LOW
- Timing follows physical paddle presses directly

**Iambic (A or B):**
- Summit's iambic keyer generates dit/dah elements with proper timing
- DIT pin outputs composite keyed signal (straight key format)
- DAH pin remains LOW
- Timing based on configured WPM speed (`cwSpeed`)
- Full memory paddle support (squeeze keying)

**No Audio Output:**
- Radio output mode does not play sidetone through Summit's speaker
- External radio provides sidetone

### Radio Keyer Mode

Device passes paddle contacts directly to the radio:

**Straight Key:**
- DIT pin mirrors DIT paddle state (HIGH when pressed)
- DAH pin remains LOW

**Iambic:**
- DIT pin mirrors DIT paddle state
- DAH pin mirrors DAH paddle state
- Radio's internal keyer interprets squeeze keying and timing
- Radio's WPM setting controls speed (Summit's WPM ignored)

### Radio Output UI

The Radio Output screen displays three configurable settings:

1. **Speed (WPM):** 5-40 WPM
   - Only affects Summit Keyer mode
   - Ignored in Radio Keyer mode (radio controls speed)
   - Shares global `cwSpeed` setting with Practice mode

2. **Key Type:** Straight / Iambic A / Iambic B
   - Determines paddle input interpretation
   - Affects both modes
   - Shares global `cwKeyType` setting

3. **Radio Mode:** Summit Keyer / Radio Keyer
   - Toggles between internal and external keying logic
   - Persisted in Preferences namespace "radio"

**Navigation:**
- UP/DOWN: Select setting
- LEFT/RIGHT: Adjust value
- ESC: Exit to Radio menu

### Input Sources

Radio Output accepts input from multiple sources simultaneously (OR logic):

1. **Physical Paddle** (GPIO 6 for DIT, GPIO 9 for DAH)
2. **Capacitive Touch** (GPIO 8 for DIT, GPIO 5 for DAH)

Both checked every loop iteration for responsive keying.

### Use Cases

**Contest Operation:**
- Summit Keyer mode for consistent sending at configured speed
- Memory messages for exchanges (future feature)
- Physical paddle for flexibility

**Casual QSOs:**
- Radio Keyer mode to use radio's built-in keyer settings
- Capacitive touch pads for portable operation
- Radio provides sidetone and QSK

**Training Aid:**
- Practice sending at various speeds
- Compare Summit Keyer vs. Radio Keyer behavior
- Use with actual radio for on-air confidence

### CW Memories

The **CW Memories** feature allows storage and management of up to 10 reusable morse code message presets. Presets can be created, edited, deleted, and previewed on the device, or managed via the web interface. They integrate seamlessly with Radio Output mode for transmission.

#### Architecture: `radio_cw_memories.h`

**Preset Structure:**
```cpp
#define CW_MEMORY_MAX_SLOTS 10
#define CW_MEMORY_LABEL_MAX_LENGTH 15
#define CW_MEMORY_MESSAGE_MAX_LENGTH 100

struct CWMemoryPreset {
  char label[CW_MEMORY_LABEL_MAX_LENGTH + 1];     // Label (15 chars + null)
  char message[CW_MEMORY_MESSAGE_MAX_LENGTH + 1]; // Message (100 chars + null)
  bool isEmpty;
};
```

**Storage:**
- Persistent storage via ESP32 Preferences namespace `"cw_memories"`
- Keys: `"label1"` through `"label10"`, `"message1"` through `"message10"`
- Loaded at startup via `loadCWMemories()`
- Saved immediately on create/edit/delete

#### Device UI - CW Memories Mode

Accessible from: **Radio Menu → CW Memories**

**Main Screen:**
- Scrollable list of all 10 preset slots
- Shows `[Slot #] Label` or `[Slot #] (empty)` for unused slots
- Navigation: UP/DOWN to select, ENTER for context menu, ESC to exit

**Context Menu (Empty Slot):**
- Create Preset
- Cancel

**Context Menu (Occupied Slot):**
- Preview (plays on device speaker)
- Edit Preset
- Delete Preset
- Cancel

**Create/Edit Flow:**
1. **Label Entry:** Type label (max 15 chars), auto-uppercase
2. **Message Entry:** Type message (max 100 chars), validation for valid morse characters
3. **Save:** ENTER to save, ESC to cancel
4. Character validation: A-Z, 0-9, space, `.`,`,`,`?`,`/`,`-`, prosigns as `<AR>`, `<SK>`, etc.

**Delete Confirmation:**
- Yes/No dialog with LEFT/RIGHT to toggle, ENTER to confirm

**Preview:**
- Plays preset message on device speaker using current WPM and tone settings
- Does NOT transmit via radio output pins (GPIO 18/17 remain LOW)
- Press ESC to stop playback early

#### Radio Output Mode Integration

In Radio Output mode, press the **'M'** key to open the memory selector:

**Memory Selector Overlay:**
- Modal overlay showing all 10 preset slots
- Scrollable list (5 visible at a time)
- Shows slot number and label (or "(empty)")
- Navigation: UP/DOWN to select, ENTER to queue for transmission, ESC to cancel

**Transmission:**
- Selected preset message is queued via existing `queueRadioMessage()` function
- Uses current WPM speed and Radio Mode settings
- Transmitted via radio output pins (GPIO 18/17)
- Queue limit: 5 messages (error beep if full)

**UI Indicator:**
- Footer in Radio Output mode shows: `M Memories` as hint

#### Debugging and Troubleshooting

**Debug Logging:**
The CW Memories module includes comprehensive debug output via Serial (115200 baud) for troubleshooting:

- `loadCWMemories()` - Prints each slot's label and message when loading from Preferences
- `saveCWMemory()` - Prints label and message when saving to Preferences
- `previewCWMemory()` - Prints label, message, and length before playback
- `queueRadioMessage()` - Prints message and length when queuing for transmission
- `processRadioMessageQueue()` - Prints message and length when transmitting via radio
- `radioIambicKeyerHandler()` - Prints dit/dah duration values for timing verification

**Known Issues:**

1. **Summit Keyer Mode Timing:**
   - In Summit Keyer mode, if `radioDitDuration` becomes corrupted or zero, both manual keying and memory transmission produce "random dits" (rapid/instant timing)
   - Radio Keyer mode works correctly for manual keying (passthrough to radio's keyer)
   - Memories may still sound incorrect in Radio Keyer mode if message queue conflicts with keyer state machine

2. **State Machine Conflict:**
   - `playMorseCharViaRadio()` uses blocking delays while `updateRadioOutput()` keyer state machine runs simultaneously
   - Both functions control the same GPIO pins (18 and 17), causing interference
   - Current mitigation: `processRadioMessageQueue()` waits for keyer state machine to be idle in Summit Keyer mode (checks `radioKeyerActive` and `radioInSpacing` flags)

3. **Preview Crashes:**
   - Preview function (`previewCWMemory()`) may crash and reset device halfway through playback
   - Likely related to blocking audio playback conflicting with other system tasks
   - Workaround: Keep preview messages short or use radio transmission instead

**Troubleshooting Steps:**

1. Enable Serial Monitor at 115200 baud
2. Create a simple test memory (e.g., "TEST" → "SOS")
3. Preview the memory and check serial output for:
   - Correct label and message strings
   - Correct message length
   - Dit/dah duration values (should be non-zero and proportional to WPM)
4. Compare behavior between Summit Keyer and Radio Keyer modes
5. Check if manual keying works correctly in both modes (isolates keyer vs. message queue issues)

#### Supported Characters

**Valid Characters:**
- Letters: A-Z (auto-uppercased)
- Numbers: 0-9
- Punctuation: `.` `,` `?` `/` `-`
- Prosigns: Entered as text like `<AR>`, `<SK>`, `<BK>`, `<BT>`, `<CT>`, `<HH>`, `<SN>`, `<SOS>`
- Spaces: Word spacing in morse code

**Validation:**
- Label and message cannot be empty
- Message must contain only valid morse characters
- Invalid characters trigger error beep and validation message

#### Use Cases

**Contest Operation:**
- Store common exchanges: `"5NN CA"`, `"TU K6ABC"`, `"CQ CQ DE K6ABC K"`
- Quick send via 'M' key in Radio Output mode
- Consistent, error-free messaging

**Casual Operation:**
- Store callsign, standard sign-offs: `"73 ES CUL <SK>"`
- CQ calls with proper prosigns
- Frequently used phrases

**Training:**
- Pre-configured examples for learning proper formats
- Practice sending stored messages

#### State Variables

```cpp
CWMemoryPreset cwMemories[CW_MEMORY_MAX_SLOTS];  // Global array of presets
int cwMemorySelection;                            // Currently selected slot in UI
CWMemoryContextMenu contextMenuActive;            // Current context menu state
CWMemoryEditMode editMode;                        // Current edit mode state
bool memorySelectorActive;                        // True when selector active in Radio Output
int memorySelectorSelection;                      // Selected slot in Radio Output selector
```

## Morse Code Decoder (Adaptive)

### Overview

The morse decoder provides real-time decoding of paddle/key input with adaptive speed tracking. Based on the open-source [morse-pro](https://github.com/scp93ch/morse-pro) JavaScript library by Stephen C Phillips, ported to C++ for ESP32.

### Architecture: Three-Module Design

**Module Structure:**
- **`morse_wpm.h`** - WPM timing utilities (PARIS standard formulas)
- **`morse_decoder.h`** - Base decoder class (timings → morse patterns → text)
- **`morse_decoder_adaptive.h`** - Adaptive speed tracking with weighted averaging

### How It Works

**Input Format:**
- Decoder accepts timing values in milliseconds
- **Positive values** = tone ON (dit or dah)
- **Negative values** = silence (element gap, character gap, word gap)

**Adaptive Speed Algorithm:**

Every decoded element provides speed information:
- Dit → duration = 1 dit
- Dah → duration = 3 dits
- Character gap → duration = 3 fdits (Farnsworth)

The decoder maintains a circular buffer of the last 30 timing samples and uses **weighted averaging** (newer samples weighted more heavily: 1, 2, 3, ..., 30) to continuously refine its WPM estimate.

**Classification Thresholds:**
- Dit/Dah boundary: 2 × dit length
- Dah/Space boundary: 5 × Farnsworth dit length
- Noise threshold: 10ms (filters glitches)
- **Word gap stability:** fditLen locked to ditLen for consistent word detection (prevents threshold drift)

### Integration with Practice Mode

**Real-Time Decoding:**
- Enabled by default in practice mode
- Press 'D' key to toggle display on/off
- Modern card-style UI with three info panels:
  - **SET WPM** (cyan badge): Configured speed
  - **ACTUAL** (green badge): Detected WPM with color coding (green = matches, yellow = different)
  - **KEY TYPE** (yellow badge): Straight/Iambic A/Iambic B
- Decoder display shows 2 lines of decoded text (17 chars per line, size 3 font)
- Supports 9 prosigns: AR, AS, BK, BT, CT, HH, SK, SN, SOS (displayed as `<AR>`, etc.)
- Hovering colored badges for visual hierarchy
- Arrow keys adjust speed (up/down) and key type (left/right)

**Timing Capture:**
- **Straight Key:** Measures tone-on and silence durations directly
- **Iambic:** Uses element start/stop times from iambic state machine
- Feeds timings to decoder after each element completes
- **First-run initialization:** `lastStateChangeTime` set to 0 to prevent spurious decoding on first entry
- Only calculates durations after first valid key press

**UI Update Strategy:**
- Decoder callback sets `needsUIUpdate = true`
- Main loop checks flag after `updatePracticeOscillator()`
- **Only updates when tone is NOT playing** to avoid audio glitches
- Calls `drawDecodedTextOnly()` for partial screen update
- Full redraw avoided during practice for best audio performance

**Auto-Flush Logic:**
- Character boundary detection: 2.5 dits of silence triggers automatic flush (in `addTiming()`)
- Backup timeout: Word gap duration (7 dits) for mid-character abandonment (in `updatePracticeOscillator()`)
- Prevents premature character splitting due to timing jitter
- Ensures real-time character display without waiting for next element
- Character overflow protection: Auto-clears after 34 characters (17 chars × 2 lines)

### Performance

**Memory Footprint:**
- `MorseDecoderAdaptive` instance: ~1-2 KB
- Decoded text buffers (200 chars): ~200 bytes
- Timing buffers (30 samples × 2): ~240 bytes
- **Total: ~2.5 KB** (negligible on ESP32-S3)

**CPU Usage:**
- Decoder processing: <1ms per character
- No floating-point intensive operations
- Real-time suitable for ESP32 at 240 MHz

### Licensing

The morse decoder modules are licensed under **EUPL v1.2** (European Union Public Licence):

- Original code: Copyright (c) 2024 Stephen C Phillips
- ESP32 port: Copyright (c) 2025 VAIL SUMMIT Contributors
- **Weak copyleft** - can be used in proprietary firmware
- **Must keep decoder modules open source** if modified
- Compatible with GPL, LGPL, MPL

Main VAIL SUMMIT firmware can remain under any license. Only the three decoder modules (`morse_wpm.h`, `morse_decoder.h`, `morse_decoder_adaptive.h`) are EUPL-licensed.

### Future Applications

The decoder is designed as a reusable component for:

1. **CW Academy validation** - Auto-check student answers
2. **Vail repeater decoding** - Decode incoming morse from others
3. **Receive training** - Decode audio from I2S microphone (requires tone detection)
4. **Accuracy metrics** - Compare intended vs. decoded patterns
5. **Contest logging** - Real-time callsign/exchange capture
