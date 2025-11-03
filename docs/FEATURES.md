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

The Morse Shooter is an arcade-style game where falling letters descend from the top of the screen. The player uses an iambic keyer (paddle or touch pads) to send morse code patterns that shoot matching letters.

### Game Architecture: `game_morse_shooter.h`

**Game Constants:**
```cpp
#define MAX_FALLING_LETTERS 5           // Maximum simultaneous letters
#define LETTER_FALL_SPEED 1             // Pixels per update
#define LETTER_SPAWN_INTERVAL 3000      // ms between new letter spawns
#define GROUND_Y 225                    // Y position of ground
#define MAX_LIVES 5                     // Lives before game over
#define GAME_UPDATE_INTERVAL 1000       // ms between game physics updates
#define GAME_LETTER_TIMEOUT 1200        // ms before pattern is submitted
```

**Character Set:**
- 36 characters: E, T, I, A, N, M, S, U, R, W, D, K, G, O, H, V, F, L, P, J, B, X, C, Y, Z, Q, 0-9
- Ordered by common morse patterns (easier letters first)

### Iambic Keyer Integration

The game uses the **exact same iambic keyer logic** as practice mode for consistent feel:

**State Machine:** IDLE → SENDING → SPACING → IDLE
- `keyerActive` - Currently sending an element (dit or dah)
- `inSpacing` - In the inter-element gap
- `sendingDit` / `sendingDah` - Which element is being sent
- `ditMemory` / `dahMemory` - Memory paddles for squeeze keying

**Key Features:**
- Non-blocking state machine (checked every loop iteration)
- Proper iambic A/B behavior with memory paddles
- Accurate WPM timing from device settings (`cwSpeed`)
- Uses `startTone()` / `continueTone()` / `stopTone()` for glitch-free audio
- **Screen completely freezes during keying** to prevent audio interference

### Game Loop Architecture

**Dual Update System:**

1. **`updateMorseShooterInput(tft)`** - Called every main loop iteration
   - Runs iambic keyer state machine
   - Handles pattern timeout detection
   - Screen freezes if any keying activity detected

2. **`updateMorseShooterVisuals(tft)`** - Called every main loop iteration
   - Checks if keying is active (paddles, tone, gap, or pattern exists)
   - If keying: returns immediately (screen frozen)
   - If idle: updates game physics once per second
   - Updates falling letters, spawns new letters, redraws HUD

**Critical Design Decision:** Screen updates are **completely blocked** during any keying activity to ensure smooth, glitch-free audio at the configured WPM speed.

### Pattern Matching and Shooting

**Pattern Completion:**
1. Pattern builds as user keys morse code (e.g., ".-" for A)
2. After last element, user releases paddles
3. System waits GAME_LETTER_TIMEOUT (1200ms) for inactivity
4. Pattern is matched against morse code table
5. If match found, searches for falling letter with that character
6. If found: shoots letter, plays laser/explosion, updates score
7. If no match or wrong code: error beep, pattern cleared

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

### CW Memories (Placeholder)

The **CW Memories** menu option is currently a placeholder for future implementation:

**Planned Features:**
- Store up to 8-10 CW message memories
- Playback stored messages via radio output
- Common contest exchanges, CQ calls, 73, etc.
- Integration with both keyer modes

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
