State          | Min Value | Max Value
---------------|-----------|----------
None           | 0         | 300
Button 1       | 400       | 560
Button 2       | 580       | 650
Button 3       | 665       | 700
Buttons 1+2    | 750       | 790
Buttons 1+3    | 805       | 825
Buttons 2+3    | 826       | 850
```

### Button Detection Logic
- Take 10 analog samples and average
- Require 2 consistent readings for debouncing
- Track maximum button state during press sequence
- Evaluate gesture only when ALL buttons return to released state (within NONE range)
- If reading falls outside all ranges, use last stable state
- Provide immediate audio feedback when long press threshold (>2s) is reached
- Provide immediate audio feedback when combo threshold (>0.5s) is reached

---

## 4. Interaction Design Principles

### Timing Definitions
- **Quick Press:** <1 second duration
- **Long Press:** >2 seconds duration
- **Combination Press:** >0.5 seconds held together

### Gesture Recognition Flow
1. Detect button press (analog value enters valid range)
2. Track maximum button combination seen during press
3. Measure duration of press
4. **Provide immediate audio feedback at threshold:**
   - At 2 seconds: Play mode entry announcement (e.g., "SPEED", "TONE", "KEY")
   - At 0.5 seconds: Play combo mode announcement (e.g., "MEM")
5. When all buttons released:
   - System is ready for next input in the entered mode
   - For quick presses (<1s), execute action immediately upon release

### Universal Rule
After any gesture triggers and audio feedback plays, wait for ALL buttons to return to released state before processing the next gesture. This prevents accidental cascading actions.

---

## 5. Audio Feedback System

All user feedback is delivered via piezo buzzer using Morse code and tones.

### Morse Announcements
- **Speed at user's current WPM setting**
- **Characters/Words:**
  - "SPEED" - entering speed setting mode
  - "TONE" - entering tone setting mode
  - "KEY" - entering key type setting mode
  - "MEM" - entering memory management mode
  - "RR" - save and exit confirmation
  - "CLR" - memory cleared
  - Individual keyer mode characters (see section 8.3)
  - Numbers 1, 2, 3 for memory slot identification

### Special Tones
- **Error Tone:** Low frequency buzz (when hitting setting limits)
- **Descending Tone Pattern:** Series of descending frequency tones (exit without save)
- **Recording Start:** "Doot, doot, dah" countdown pattern (inspired by Mario Kart race start)
- **Setting Adjustment Beeps:**
  - Speed: Single beep at current frequency
  - Tone: Beep at the new frequency being set

---

## 6. Operating Modes

### 6.1 Normal Operation Mode (Default)

This is the default state when the adapter powers on or after exiting any other mode.

**Quick Press Actions:**
- **Button 1:** Play memory slot 1
- **Button 2:** Play memory slot 2
- **Button 3:** Play memory slot 3
  - If memory slot is empty: do nothing (silent)
  - If memory has content: play back via current output mode (MIDI/Keyboard/Radio)
  - Playback cannot be interrupted
  - User's CW key is disabled during playback

**Long Press Actions:**
- **Button 1 (>2s):** Enter speed setting mode
  - At 2 second mark: Play "SPEED" in Morse
  - User can release button after hearing announcement
  - Mode is now active
- **Button 2 (>2s):** Enter tone setting mode
  - At 2 second mark: Play "TONE" in Morse
  - User can release button after hearing announcement
  - Mode is now active
- **Button 3 (>2s):** Enter key type setting mode
  - At 2 second mark: Play "KEY" in Morse
  - User can release button after hearing announcement
  - Mode is now active

**Combination Press Actions:**
- **Buttons 1+3 (>0.5s):** Enter memory management mode
  - At 0.5 second mark: Play "MEM" in Morse
  - User can release buttons after hearing announcement
  - Mode is now active

---

### 6.2 Speed Setting Mode

Allows user to adjust WPM (words per minute) speed setting.

**Entry:**
- Long press Button 1 from normal mode for >2 seconds
- At 2 second mark: Play "SPEED" announcement

**Controls:**
- **Button 1 (quick press):** Increase speed by 1 WPM
  - Play single beep
  - User can test with connected CW key
- **Button 3 (quick press):** Decrease speed by 1 WPM
  - Play single beep
  - User can test with connected CW key
- **Button 2 (long press >2s):** Save and exit
  - At 2 second mark: Play "RR" in Morse
  - User can release button after hearing "RR"
  - Return to normal operation mode

**Range & Limits:**
- Minimum: 5 WPM
- Maximum: 40 WPM
- When at limit and attempting to go beyond: stay at limit, play error tone (low buzz)

**Inactivity Timeout:**
- 30 seconds of no button press or key activity
- Play descending tone pattern
- Auto-exit to normal mode (saves current value)
- Timer resets on any button press or CW key activity

**Testing During Adjustment:**
- User's connected CW paddle/key remains functional
- Sidetone plays at current speed setting being adjusted

---

### 6.3 Tone Setting Mode

Allows user to adjust sidetone frequency.

**Entry:**
- Long press Button 2 from normal mode for >2 seconds
- At 2 second mark: Play "TONE" announcement

**Controls:**
- **Button 1 (quick press):** Increase frequency by one index
  - Play beep at NEW frequency
  - User can test with connected CW key
- **Button 3 (quick press):** Decrease frequency by one index
  - Play beep at NEW frequency
  - User can test with connected CW key
- **Button 2 (long press >2s):** Save and exit
  - At 2 second mark: Play "RR" in Morse
  - User can release button after hearing "RR"
  - Return to normal operation mode

**Range & Limits:**
- Uses equal temperament note array (provided in notes.h)
- Minimum: Index 39 (82 Hz)
- Maximum: Index 96 (2094 Hz)
- Total available tones: 58
- When at limit and attempting to go beyond: stay at limit, play error tone (low buzz)

**Inactivity Timeout:**
- 30 seconds of no button press or key activity
- Play descending tone pattern
- Auto-exit to normal mode (saves current value)
- Timer resets on any button press or CW key activity

**Testing During Adjustment:**
- User's connected CW paddle/key remains functional
- Sidetone plays at frequency being adjusted

---

### 6.4 Key Type Setting Mode

Allows user to cycle through the 9 available keyer modes.

**Entry:**
- Long press Button 3 from normal mode for >2 seconds
- At 2 second mark: Play "KEY" announcement

**Controls:**
- **Button 1 (quick press):** Next keyer mode (forward through list)
  - Play mode identifier in Morse (see section 8.3)
- **Button 3 (quick press):** Previous keyer mode (backward through list)
  - Play mode identifier in Morse (see section 8.3)
- **Button 2 (long press >2s):** Save and exit
  - At 2 second mark: Play "RR" in Morse
  - User can release button after hearing "RR"
  - Return to normal operation mode

**Keyer Mode Cycle Order (by popularity):**
1. Straight (S)
2. Iambic B (IB)
3. Iambic A (IA)
4. Iambic (I)
5. Bug (B)
6. ElBug (EB)
7. Ultimatic (U)
8. SingleDot (SD)
9. Keyahead (K)

Pressing Button 1 at Keyahead wraps to Straight.  
Pressing Button 3 at Straight wraps to Keyahead.

**Inactivity Timeout:**
- 30 seconds of no button press or key activity
- Play descending tone pattern
- Auto-exit to normal mode (saves current value)
- Timer resets on any button press or CW key activity

**No Testing During Adjustment:**
- Unlike speed/tone modes, user cannot test CW key in this mode
- Mode change takes effect only after save

---

### 6.5 Memory Management Mode

Allows user to record, play, clear, and manage three CW memory slots.

**Entry:**
- Buttons 1+3 combo held for >0.5 seconds from normal mode
- At 0.5 second mark: Play "MEM" in Morse
- User can release buttons after hearing "MEM"

**Exit:**
- Buttons 1+3 combo held for >0.5 seconds again
- At 0.5 second mark: Play descending tone pattern
- User can release buttons after hearing descending tones
- Return to normal operation mode

**Memory Slot Controls (each button manages its own memory):**

**Button 1 manages Memory Slot 1:**
- **Quick press (<1s):** Play memory 1
  - If empty: do nothing (silent)
  - If has content: play via piezo only (no MIDI/Keyboard/Radio output)
  - Playback cannot be interrupted
  - User's CW key is disabled during playback
- **Long press (>2s):** Clear memory 1
  - At 2 second mark: Play "1 CLR" in Morse (number one, then CLR)
  - User can release button after hearing "1 CLR"
  - Memory slot 1 is now empty
- **Double click:** Start recording memory 1
  - Play "doot, doot, dah" countdown
  - Begin recording immediately after countdown
  - Record user's CW key input with actual timing (not keyer-processed)
  - Maximum recording duration: 25 seconds
  - User presses Button 1 again (single click) to stop and save
  - If 25 seconds reached: auto-save and play "RR"
- **Single click while recording:** Stop recording and save
  - Play "RR" in Morse
  - Memory saved, ready for playback

**Button 2 manages Memory Slot 2:**
- Same controls as Button 1, but for memory slot 2
- Clear announcement: "2 CLR"

**Button 3 manages Memory Slot 3:**
- Same controls as Button 1, but for memory slot 3
- Clear announcement: "3 CLR"

**Recording Specifications:**
- Maximum duration per memory: 25 seconds
- Captures actual key timing (not keyer-processed)
- Records key-down and key-up durations
- Auto-save at 25 seconds plays "RR" confirmation
- **Automatic trimming:** Recording is trimmed to end at the last key release event
  - Eliminates dead air between last key release and save button press
  - Ensures clean playback without trailing silence
  - Trimming occurs during save process (both manual and auto-save)

**Playback Specifications (in Memory Management Mode):**
- Output: Piezo speaker only
- No MIDI/Keyboard/Radio output
- Cannot be interrupted
- CW key disabled during playback

**No Inactivity Timeout:**
- Memory management mode does NOT auto-exit
- User must explicitly press B1+B3 to exit

---

## 7. Memory Storage Architecture

### 7.1 Storage Medium
- **Location:** EEPROM
- **Slots:** 3 independent memory slots
- **Duration:** Up to 25 seconds per slot

### 7.2 Data Format
Use run-length encoding to efficiently store timing:
- Store alternating key-down and key-up durations
- Use millisecond precision
- Format: sequence of duration values representing alternating key states
- **Trimming:** Store only up to the last key-release event, discarding any trailing silence

### 7.3 Storage Requirements
- Typical CW at 20 WPM: approximately 6 timing transitions per second
- 25 seconds = approximately 150 transitions
- Three memory slots plus metadata should fit within available EEPROM space
- Trimming reduces storage requirements by eliminating dead air

### 7.4 Data Integrity
- Include length field for each memory
- Empty memory indicated by length = 0
- Validate data integrity on read/write operations

### 7.5 Recording Trimming Logic
**When to trim:**
- On manual save (user clicks button to stop recording)
- On auto-save (25 second limit reached)

**How to trim:**
- Identify the timestamp of the last key-release event
- Discard all timing data after the last key-release
- Store only the trimmed sequence
- This eliminates the "dead air" period between last key release and save button press

**Example:**
```
User keys: DIT-space-DAH-space-DIT [releases key] [3 seconds of silence] [presses save button]

Stored: DIT-space-DAH-space-DIT [END]
NOT stored: [3 seconds of silence]
```

---

## 8. Technical Implementation Details

### 8.1 Button State Machine

**States:**
- NORMAL_MODE
- SPEED_SETTING_MODE
- TONE_SETTING_MODE
- KEY_SETTING_MODE
- MEMORY_MANAGEMENT_MODE
- RECORDING_MEMORY_1
- RECORDING_MEMORY_2
- RECORDING_MEMORY_3
- PLAYING_MEMORY (blocking)

### 8.2 Button Detection Requirements

**Detection Logic:**
- Sample analog pin with averaging (10 samples)
- Debounce with consistent reading requirement (2 consecutive matches)
- Track maximum button state during press
- Detect press duration continuously
- Provide immediate feedback at thresholds:
  - Long press: feedback at 2 seconds while button still held
  - Combo press: feedback at 0.5 seconds while buttons still held
- Evaluate final action on full release (all buttons return to NONE state)

**Threshold Feedback:**
- When button held crosses 2 second threshold → play mode entry announcement
- When combo held crosses 0.5 second threshold → play combo announcement
- User receives immediate confirmation without needing to release first
- After announcement plays, user can release and mode is active

### 8.3 Keyer Mode Identifiers

Mapping from keyer array index to Morse announcement:

| Index | Keyer Name  | Code Identifier | Morse Pattern |
|-------|-------------|-----------------|---------------|
| 1     | Straight    | S               | ...           |
| 8     | Iambic B    | IB              | .. -...       |
| 7     | Iambic A    | IA              | .. .-         |
| 6     | Iambic      | I               | ..            |
| 2     | Bug         | B               | -...          |
| 3     | ElBug       | EB              | . -...        |
| 5     | Ultimatic   | U               | ..-           |
| 4     | SingleDot   | SD              | ... -..       |
| 9     | Keyahead    | K               | -.-           |

### 8.4 Tone Generation Reference

Use existing `equalTemperamentNote[]` array from notes.h:
- Available range: indices 39-96 (82 Hz to 2094 Hz)
- Increment/decrement by 1 index per button press

### 8.5 Inactivity Timer Requirements

**Timeout Duration:** 30 seconds

**Applies to:**
- Speed setting mode
- Tone setting mode
- Key type setting mode

**Does NOT apply to:**
- Normal operation mode
- Memory management mode

**Reset Triggers:**
- Any button press
- Any CW key activity (dit, dah, or straight key)

**Timeout Action:**
- Play descending tone pattern
- Save current settings to EEPROM
- Return to normal operation mode

### 8.6 Memory Recording Requirements

**Recording Trigger:** Double-click detection on any button in memory management mode

**Recording Process:**
1. Play "doot, doot, dah" countdown
2. Begin capturing key-down and key-up timing
3. Track timestamp of each key event
4. Continue until user clicks button again OR 25 seconds elapsed
5. **Trim recording to last key-release event**
6. Play "RR" confirmation
7. Save trimmed recording to EEPROM with run-length encoding

**Timing Capture:**
- Record actual user timing, not keyer-processed
- Capture both key-down and key-up durations
- Maintain millisecond precision
- Track timestamp of last key-release for trimming

**Trimming Algorithm:**
- During recording, continuously track the timestamp of the most recent key-release event
- When save is triggered (manual or auto):
  - Identify the last key-release timestamp
  - Discard all timing data after that timestamp
  - Store only the trimmed sequence
- This eliminates dead air between last keying and save button press

**Storage:**
- Use efficient run-length encoding
- Store trimmed data in EEPROM immediately after recording stops
- Validate write success

### 8.7 Memory Playback Requirements

**In Normal Mode:**
- Quick press plays memory via current output mode (MIDI/Keyboard/Radio)
- Respects adapter's current operating mode
- If slot empty: do nothing (silent)

**In Memory Management Mode:**
- Quick press plays memory via piezo only
- No output to MIDI/Keyboard/Radio
- If slot empty: do nothing (silent)

**Playback Behavior:**
- Cannot be interrupted once started
- CW key input disabled during playback
- Playback timing matches recording exactly
- No trailing silence (due to trimming during save)

---

## 9. User Experience Flows

### 9.1 Adjusting Speed
1. User long-presses Button 1
2. At 2 seconds, adapter plays "SPEED" in Morse
3. User releases button (mode is now active)
4. User presses Button 1 to increase or Button 3 to decrease
5. Each press: single beep plays
6. User tests with paddle to hear new speed
7. User long-presses Button 2
8. At 2 seconds, adapter plays "RR"
9. User releases button
10. Returns to normal mode

### 9.2 Recording a Memory
1. From normal mode, user holds B1+B3
2. At 0.5 seconds, adapter plays "MEM"
3. User releases buttons (now in memory management mode)
4. User double-clicks Button 2 (for memory slot 2)
5. Adapter plays "doot, doot, dah" countdown
6. User keys their message with paddle: "CQ CQ CQ DE K0BOS"
7. User releases paddle (last key-up event recorded)
8. User takes time to find and press Button 2 (e.g., 2-3 seconds delay)
9. User single-clicks Button 2 to stop
10. Adapter trims recording to end at last key-release (eliminating the 2-3 second delay)
11. Adapter plays "RR"
12. User holds B1+B3 again
13. At 0.5 seconds, adapter plays descending tones
14. User releases buttons
15. Returns to normal mode
16. Later playback contains "CQ CQ CQ DE K0BOS" with no trailing silence

### 9.3 Playing a Memory in Normal Mode
1. User quick-presses Button 3
2. If memory 3 is stored, plays via current output mode (MIDI/Keyboard/Radio)
3. If memory 3 is empty, nothing happens (silent)
4. User cannot interrupt playback
5. Playback contains no trailing silence due to trimming

### 9.4 Clearing a Memory
1. From normal mode, user holds B1+B3
2. At 0.5 seconds, adapter plays "MEM"
3. User releases buttons
4. User long-presses Button 1
5. At 2 seconds, adapter plays "1 CLR"
6. User releases button (memory 1 is now empty)
7. User holds B1+B3 to exit
8. At 0.5 seconds, adapter plays descending tones
9. User releases buttons

---

## 10. Edge Cases & Error Handling

### 10.1 Analog Reading Outside Thresholds
- Use last stable reading
- Continue sampling until valid range detected
- Debouncing will filter transient noise

### 10.2 Button Held Across Mode Transitions
- Wait for full release before processing next gesture
- Prevents accidental double-actions

### 10.3 Memory Full During Recording
- Recording automatically stops at 25 seconds
- Trim to last key-release event
- Play "RR" to indicate save
- Memory is usable immediately

### 10.4 EEPROM Write Failures
- Validate write with readback
- If validation fails, implement retry logic
- If ultimately fails, play error tone and discard recording

### 10.5 Rapid Button Presses
- Debouncing prevents false triggers
- Each gesture must fully complete (release) before next

### 10.6 Setting Adjustments at Limits
- Stay at limit value
- Play error tone (low buzz)
- Do not wrap around

### 10.7 Power Loss During Recording
- Recording is not saved until user stops or 25s auto-save
- Previous memory content remains intact
- No corruption to EEPROM

### 10.8 Playback Interrupted by Power Loss
- Playback simply stops
- No state corruption
- User can replay memory after power restoration

### 10.9 Multiple Buttons Transitioning
- R2R circuit naturally prioritizes highest combination
- maxButtonState tracking ensures correct combo detected
- Release detection waits for full NONE state

### 10.10 User Holds Button Past Threshold
- Audio feedback plays at threshold (2s or 0.5s)
- User knows action has triggered
- User can release at any point after hearing feedback
- No need to hold indefinitely

### 10.11 Recording Contains Only Key-Down (No Release)
- If recording ends while key is still held down
- Save includes the key-down event
- Trimming logic handles this gracefully
- Playback will reflect the held state

### 10.12 No Keying During Recording
- If user saves without any key events
- Memory is saved as empty (length = 0)
- Subsequent playback: do nothing (silent)

### 10.13 Long Pause Between Key Events
- All pauses between key events are preserved
- Only trailing silence after LAST key-release is trimmed
- Internal pauses are intentional and part of the message

---

## 11. Settings Persistence

### 11.1 Settings Storage
**Stored in EEPROM:**
- Speed (WPM value)
- Tone (index into equal temperament array)
- Keyer mode (index into keyer array)
- Memory slot 1 content and metadata
- Memory slot 2 content and metadata
- Memory slot 3 content and metadata

### 11.2 Default Values (Factory Reset)
- Speed: 20 WPM
- Tone: Index 69 (440 Hz - musical A)
- Keyer mode: 1 (Straight)
- All memory slots: empty

### 11.3 Settings Load on Boot
- Read EEPROM on startup
- Validate data integrity
- If invalid, write defaults
- Apply settings to adapter immediately

### 11.4 Settings Save Triggers
- Speed mode: Save on "RR" exit or timeout
- Tone mode: Save on "RR" exit or timeout
- Key mode: Save on "RR" exit or timeout
- Memory: Save immediately after recording stops (with trimming applied)

---

## 12. Audio Feedback Specifications

### 12.1 Morse Code Playback
- Speed: User's current WPM setting
- Frequency: User's current tone setting
- Character spacing: Standard Morse timing
- Word spacing: Standard Morse timing

### 12.2 Error Tone
- Frequency: 200 Hz (low)
- Duration: 200ms
- Pattern: Continuous buzz

### 12.3 Descending Tone Pattern
- Start frequency: 1000 Hz
- End frequency: 400 Hz
- Number of steps: 5
- Duration per step: 100ms
- Total duration: 500ms

### 12.4 Recording Countdown ("doot, doot, dah")
- First doot: 800 Hz, 200ms
- Pause: 200ms
- Second doot: 800 Hz, 200ms
- Pause: 200ms
- Dah: 600 Hz, 600ms
- Pause: 200ms, then begin recording

### 12.5 Setting Adjustment Beeps
- Duration: 50ms
- Frequency: Current tone setting (for tone mode) or standard tone (for speed mode)

---

## 13. Testing Requirements

### 13.1 Unit Tests
- Button detection accuracy across all thresholds
- Debouncing effectiveness with rapid presses
- Timing thresholds (quick/long/combo) accuracy with immediate feedback at thresholds
- EEPROM read/write reliability
- Memory encoding/decoding correctness
- Recording trimming algorithm accuracy

### 13.2 Integration Tests
- Mode transitions work correctly
- Immediate audio feedback at 2s and 0.5s thresholds
- Inactivity timeout triggers properly
- Memory playback matches recording
- Settings persist across power cycles
- Audio feedback plays at correct times
- Trimming removes trailing silence but preserves intentional pauses

### 13.3 User Acceptance Tests
- Users can adjust speed from 5-40 WPM
- Users can adjust tone across full range
- Users can cycle through all 9 keyer modes
- Users can record 25-second memories
- Users can play, clear, and re-record memories
- No accidental button triggers during normal CW operation
- All Morse announcements are clear and understandable
- Users receive immediate feedback when holding buttons (don't have to guess)
- Recorded memories play back without trailing silence
- Intentional pauses within messages are preserved

### 13.4 Edge Case Tests
- Behavior at setting limits
- Rapid button presses don't cause crashes
- Power loss during recording doesn't corrupt EEPROM
- Multiple simultaneous button presses detected correctly
- Timeout triggers exactly at 30 seconds
- Recording stops exactly at 25 seconds
- Feedback plays at exactly 2s for long press, 0.5s for combo
- Trimming works correctly with various timing scenarios:
  - Quick save after last key release (minimal trimming)
  - Long delay before save (significant trimming)
  - Save while key is still held down
  - Recording with no key events
  - Recording with long internal pauses (preserved)

---

## 14. Development Phases

### Phase 1: Button Detection Infrastructure
**Deliverables:**
- R2R analog reading with averaging
- Debouncing logic
- Button state detection (single & combo)
- Gesture timing (quick/long/combo) with continuous duration tracking
- Immediate threshold feedback (audio at 2s and 0.5s while still held)
- Release detection

**Acceptance Criteria:**
- All 7 button states detected reliably
- Debouncing prevents false triggers
- Timing thresholds accurate within 50ms
- Audio feedback plays immediately at thresholds without requiring release

### Phase 2: Settings Modes (Speed, Tone, Key)
**Deliverables:**
- Mode state machine
- Speed adjustment (5-40 WPM)
- Tone adjustment (index 39-96)
- Key type cycling (9 modes)
- Inactivity timeout
- EEPROM persistence
- Morse announcements for all modes
- "RR" save confirmation
- Error tone for limits

**Acceptance Criteria:**
- All three settings adjustable via buttons
- Settings persist across power cycles
- Timeout works correctly
- Audio feedback is clear
- CW key testing works during speed/tone adjustment

### Phase 3: Memory Management
**Deliverables:**
- Memory management mode
- EEPROM storage structure
- Recording with run-length encoding
- Recording trimming logic (remove trailing silence)
- 25-second duration limit
- Memory playback (piezo only in management mode)
- Clear memory function
- "doot, doot, dah" countdown
- "RR" save confirmation
- "[N] CLR" announcements

**Acceptance Criteria:**
- Can record up to 25 seconds
- Timing accuracy within 10ms
- Playback matches recording (without trailing silence)
- Trimming correctly removes dead air after last key release
- Trimming preserves intentional pauses within message
- Clear function works
- EEPROM doesn't corrupt

### Phase 4: Normal Mode Memory Playback
**Deliverables:**
- Quick press plays memory in normal mode
- Output routing (MIDI/Keyboard/Radio based on adapter mode)
- Empty slot handling (silent)
- Playback blocking with input disabled

**Acceptance Criteria:**
- Memory plays correctly via configured output
- Empty slots do nothing
- Cannot interrupt playback
- CW key disabled during playback
- No trailing silence in playback

### Phase 5: Polish & Testing
**Deliverables:**
- All audio feedback tuned and tested
- Edge cases handled
- User documentation
- Performance optimization

**Acceptance Criteria:**
- All user acceptance tests pass
- No crashes or hangs
- Responsive feel (<100ms latency)
- Clean, maintainable code

---

## 15. Dependencies & Constraints

### 15.1 Hardware Dependencies
- R2R hat PCB must be manufactured and tested
- Analog values must match documented thresholds
- PIN 4 must be available on SAMD21

### 15.2 Software Dependencies
- Existing Vail Adapter firmware (keyers.cpp, adapter.h)
- Existing EEPROM library
- Existing tone generation code
- Existing MIDI/Keyboard output infrastructure

### 15.3 Constraints
- EEPROM size: Limited available space for memories + settings
- Memory duration: 25 seconds maximum per slot
- Tone range: Limited to indices 39-96 from equal temperament array
- No visual feedback (audio only)

### 15.4 Compatibility
- Must not interfere with existing Vail Adapter functionality
- Must work in MIDI mode, Keyboard mode, and Radio mode
- Must preserve existing startup behavior
- Must maintain compatibility with existing EEPROM layout for other settings

---

## 16. Success Criteria

### Launch Criteria
- [ ] All 7 button states detected reliably
- [ ] Immediate audio feedback at thresholds (2s and 0.5s)
- [ ] All 3 settings modes functional
- [ ] Memory recording/playback working
- [ ] Recording trimming removes trailing silence
- [ ] EEPROM persistence confirmed
- [ ] Audio feedback clear and understandable
- [ ] No crashes or data corruption
- [ ] User documentation complete

### Post-Launch Metrics
- User adoption rate of button controls vs. computer settings
- Average time to adjust settings via buttons
- Memory feature usage frequency
- User satisfaction with trimmed playback (no dead air)
- Support tickets related to button controls
- User satisfaction surveys

---

## 17. Documentation Requirements

### User Documentation
- Button layout diagram with USB port reference
- Quick reference card for all button combinations
- Step-by-step guide for each mode
- Troubleshooting common issues
- Audio feedback reference (what each sound means)
- Explanation of automatic trimming feature

### Developer Documentation
- Code architecture overview
- State machine diagram
- EEPROM memory map
- Button detection algorithm explanation
- Memory encoding format specification
- Trimming algorithm explanation

---

## Appendix A: Morse Code Reference

### Letters Used in Audio Feedback
```
Letter | Morse
-------|-------
A      | .-
B      | -...
C      | -.-.
D      | -..
E      | .
I      | ..
K      | -.-
L      | .-..
M      | --
N      | -.
O      | ---
P      | .--.
R      | .-.
S      | ...
T      | -
U      | ..-
Y      | -.--
```

### Numbers
```
1      | .----
2      | ..---
3      | ...--
```

### Full Words
```
SPEED  | ... .--. . . -..
TONE   | - --- -. .
KEY    | -.- . -.--
MEM    | -- . --
CLR    | -.-. .-.. .-.
RR     | .-. .-.
```

---

## Appendix B: State Diagram
```
[Normal Mode] ─┬─ B1 long (2s) ──> Play "SPEED" ──> [Speed Setting Mode]
               │                          │
               ├─ B2 long (2s) ──> Play "TONE" ───> [Tone Setting Mode]
               │                          │
               ├─ B3 long (2s) ──> Play "KEY" ────> [Key Setting Mode]
               │                          │
               ├─ B1+B3 (0.5s) ──> Play "MEM" ────> [Memory Management Mode]
               │                          │
               └─ B1/B2/B3 quick ─> Play Memory (if exists)

All Settings Modes:
  - B1 quick: Increase
  - B3 quick: Decrease
  - B2 long (2s): Play "RR" ──> [Normal Mode]
  - 30s timeout: Play descending tones ──> [Normal Mode]

Memory Management Mode:
  - B1/B2/B3 quick: Play memory (piezo only)
  - B1/B2/B3 long (2s): Play "[N] CLR" ──> Clear memory
  - B1/B2/B3 double: Play countdown ──> [Recording State]
  - B1+B3 (0.5s): Play descending tones ──> [Normal Mode]

Recording State:
  - Track last key-release timestamp continuously
  - B1/B2/B3 single: Trim to last key-release ──> Play "RR" ──> [Memory Management Mode]
  - 25s elapsed: Trim to last key-release ──> Play "RR" ──> [Memory Management Mode]
```

---

## Appendix C: Recording Trimming Examples

### Example 1: Quick Save
```
User actions:
- Keys: DIT DAH DIT [release]
- Immediately presses save (0.2s delay)

Stored timing: DIT DAH DIT [END]
Trimmed: 0.2 seconds
```

### Example 2: Slow Save
```
User actions:
- Keys: CQ CQ CQ [release]
- Searches for button (3.5s delay)
- Presses save

Stored timing: CQ CQ CQ [END]
Trimmed: 3.5 seconds
```

### Example 3: Message with Internal Pauses
```
User actions:
- Keys: CQ [release] [1.5s pause] DE [release] [1.5s pause] K0BOS [release]
- Presses save (2s delay)

Stored timing: CQ [1.5s] DE [1.5s] K0BOS [END]
Trimmed: 2 seconds (only trailing silence)
Preserved: 1.5s + 1.5s internal pauses (intentional)
```

### Example 4: Auto-Save at 25 Seconds
```
User actions:
- Keys continuously for 23 seconds
- Last key release at 23.2s
- Continues holding button or searching
- Auto-save triggers at 25s

Stored timing: [All keying up to 23.2s] [END]
Trimmed: 1.8 seconds (25s - 23.2s)