---
name: Release Testing - Hardware Config
about: Testing checklist for a specific hardware configuration
title: 'Release Testing v[VERSION] - [BOARD] [PCB_VERSION]'
labels: ['testing', 'release']
assignees: ''
---

# Release Testing - [BOARD] [PCB_VERSION]

**Version:** v[VERSION]
**Board:** [ ] XIAO SAMD21 / [ ] QT Py SAMD21
**PCB Version:** [ ] Non-PCB / [ ] Basic V1 / [ ] Basic V2 / [ ] Advanced
**Firmware File:** `[board]_[pcb_version].uf2`

**Parent Issue:** #___ (link to overview issue)

---

## 👤 Tester Information

**Primary Tester:** ___________
**Testing Date:** YYYY-MM-DD
**Browser:** [e.g., Chrome v120, Firefox v115]
**Operating System:** [e.g., Windows 11, macOS 14, Ubuntu 22.04]
**USB Cable:** [e.g., USB-C data cable, USB-A to micro-USB]

**Additional Testers:**
- ___________ (Date: YYYY-MM-DD)
- ___________ (Date: YYYY-MM-DD)

---

## ✅ Core Functionality Tests

- [ ] Firmware update website loads correctly
- [ ] Correct UF2 file downloads for this hardware config
- [ ] Device enters bootloader mode successfully
- [ ] Bootloader drive appears (ADAPTERBOOT/QTPYBOOT/XIAOBOOT)
- [ ] Firmware flashes successfully (UF2 copied to bootloader drive)
- [ ] Device restarts automatically after flashing
- [ ] Device operates normally after update
- [ ] LED indicators work as expected
- [ ] Serial port enumeration works (device shows up in device manager/system)

---

## ✅ Keyer Mode Tests

- [ ] **Passthrough mode (0)** - Manual control, no automation
- [ ] **Straight key mode (1)** - Basic straight key operation
- [ ] **Bug mode (2)** - Semi-automatic, auto-dit works
- [ ] **El-Bug mode (3)** - Electric bug variant works
- [ ] **Single-dot mode (4)** - Single dit on paddle press
- [ ] **Ultimatic mode (5)** - Ultimatic priority logic works
- [ ] **Plain keyer mode (6)** - Basic iambic without squeeze
- [ ] **Iambic A mode (7)** - Iambic mode A works correctly
- [ ] **Iambic B mode (8)** - Iambic mode B works correctly (most popular)
- [ ] **Key-ahead mode (9)** - Key-ahead buffering works
- [ ] Keyer mode can be changed via button controls
- [ ] Keyer mode selection is intuitive
- [ ] Current keyer mode is indicated clearly (if applicable)

---

## ✅ Input Hardware Tests

**Physical Inputs:**
- [ ] Dit paddle input works reliably
- [ ] Dah paddle input works reliably
- [ ] Straight key input works (if using TRS jack)
- [ ] Inputs are debounced properly (no false triggers)
- [ ] Inputs respond quickly (no noticeable lag)

**Capacitive Touch (if using touch pads):**
- [ ] Capacitive touch dit input works
- [ ] Capacitive touch dah input works
- [ ] Capacitive touch key input works
- [ ] Touch sensitivity is appropriate (not too sensitive/insensitive)
- [ ] Touch inputs are reliable

**TRS Detection (if applicable):**
- [ ] TRS jack detection works for straight key auto-configuration
- [ ] Switching between paddle and straight key works reliably
- [ ] Correct keyer mode selected automatically

---

## ✅ USB Output Tests

**Keyboard Mode:**
- [ ] Keyboard mode active by default (or as expected)
- [ ] Dit outputs Left Ctrl key correctly
- [ ] Dah outputs Right Ctrl key correctly
- [ ] Straight key outputs correctly (if applicable)
- [ ] Key presses detected in text editor or Vail website
- [ ] No stuck keys or missed releases

**MIDI Mode:**
- [ ] Can switch to MIDI mode via MIDI CC0 message
- [ ] MIDI note outputs correctly for dit (C# / note 61)
- [ ] MIDI note outputs correctly for dah (D / note 62)
- [ ] MIDI note outputs correctly for straight key (C / note 60)
- [ ] MIDI Note On/Off timing is accurate
- [ ] Can switch back to Keyboard mode

**MIDI Control:**
- [ ] MIDI CC1 (speed control) works
- [ ] MIDI CC2 (tone control) works
- [ ] MIDI Program Change (keyer type selection) works
- [ ] MIDI control changes are responsive

---

## ✅ Audio/Sidetone Tests

- [ ] Sidetone buzzer sounds on dit input
- [ ] Sidetone buzzer sounds on dah input
- [ ] Sidetone buzzer sounds on straight key input
- [ ] Tone frequency is audible and clear
- [ ] Tone pitch is appropriate for Morse code
- [ ] Tone volume is adequate (not too quiet)
- [ ] Tone frequency adjustment works (via button long-press)
- [ ] Tone changes are noticeable across adjustment range
- [ ] Tone frequency persists after power cycle
- [ ] **Buzzer disable feature** works (5-second DIT hold, LED blinks slowly)
- [ ] Buzzer remains disabled after power cycle (if saved)
- [ ] Can re-enable buzzer (another 5-second DIT hold)

---

## ✅ Speed Control Tests

- [ ] Speed setting mode accessible via button long-press
- [ ] Can increase WPM with button presses
- [ ] Can decrease WPM with button presses
- [ ] WPM range works correctly (5-40 WPM)
- [ ] Speed changes are noticeable
- [ ] Dit/dah timing is accurate at 10 WPM
- [ ] Dit/dah timing is accurate at 20 WPM
- [ ] Dit/dah timing is accurate at 30 WPM
- [ ] Speed setting persists after power cycle
- [ ] MIDI CC1 speed control works (if MIDI mode tested)
- [ ] Speed indication is clear (if applicable)

---

## ✅ Memory System Tests

**Memory Management Mode:**
- [ ] Memory management mode accessible (B1+B3 combo press)
- [ ] Can exit memory management mode (B1+B3 again)
- [ ] Mode transitions are clear and predictable

**Recording:**
- [ ] Can start recording to memory slot 1
- [ ] Piezo provides feedback during recording (if buzzer enabled)
- [ ] Can record actual paddle/key inputs (bypass keyer)
- [ ] Can record up to 25 seconds
- [ ] Recording stops automatically at 25 seconds
- [ ] Can stop recording early
- [ ] Can record to memory slot 2
- [ ] Can record to memory slot 3
- [ ] Recording captures timing accurately
- [ ] Recording auto-trims trailing silence

**Playback:**
- [ ] Playback from slot 1 works (piezo-only in memory mode)
- [ ] Playback from slot 2 works
- [ ] Playback from slot 3 works
- [ ] Playback timing matches recorded input
- [ ] Can play memories in normal mode (full output via button quick-press)
- [ ] Playback via keyboard output works in normal mode
- [ ] Playback via MIDI output works in normal mode (if tested)
- [ ] Can stop playback mid-stream

**Memory Management:**
- [ ] Memory clear function works (long press while playing)
- [ ] Can clear individual slots
- [ ] Cleared slots remain empty
- [ ] Memories persist after power cycle
- [ ] All 3 slots can be filled and persist
- [ ] Overwriting existing memory works

---

## ✅ Button/Menu System Tests

**Button Detection:**
- [ ] Button 1 (B1) detected reliably
- [ ] Button 2 (B2) detected reliably (if applicable)
- [ ] Button 3 (B3) detected reliably
- [ ] Button presses are debounced (no double-triggers)

**Button Gestures:**
- [ ] **Quick press** triggers appropriate action (< 0.5s)
- [ ] **Long press** enters settings modes (2 seconds)
- [ ] **Combo press** (B1+B3) toggles memory mode (0.5s hold)
- [ ] **Double-click** detection works (within 400ms) (if applicable)
- [ ] Gestures are consistent and predictable

**Mode Transitions:**
- [ ] Can enter Speed setting mode from Normal mode
- [ ] Can enter Tone setting mode from Normal mode
- [ ] Can enter Keyer type setting mode from Normal mode
- [ ] Can toggle Memory management mode from Normal mode
- [ ] Mode transitions work correctly (NORMAL ↔ SPEED ↔ TONE ↔ KEY)
- [ ] Cannot accidentally enter wrong mode
- [ ] Can exit settings modes back to Normal mode
- [ ] Menu system is intuitive and predictable

---

## ✅ Special Features Tests

### Radio Mode Features (Advanced PCB Only)

**Radio Mode Toggle:**
- [ ] Radio mode toggle works (10 DAH presses within 500ms)
- [ ] LED blinks rapidly when radio mode active
- [ ] Radio mode state persists after power cycle
- [ ] Can toggle radio mode off

**Radio Keyer Mode:**
- [ ] Radio keyer mode toggle works (5-second DAH hold in radio mode)
- [ ] Radio keyer mode affects output behavior as expected
- [ ] Radio keyer mode state persists after power cycle

**Radio Output Pins (Advanced PCB Only):**
- [ ] Radio dit output pin (A2) functions correctly
- [ ] Radio dah output pin (A3) functions correctly
- [ ] Radio output timing matches paddle inputs
- [ ] Radio output works with keyer processing (if radio keyer mode enabled)
- [ ] Radio output respects RADIO_KEYING_ACTIVE_LOW setting

**N/A for this hardware config:**
- [ ] This hardware does not have radio output (not Advanced PCB)

---

## ✅ Settings Persistence Tests

- [ ] Keyer type persists after power cycle
- [ ] Dit duration (speed) persists after power cycle
- [ ] TX note (sidetone tone) persists after power cycle
- [ ] Radio mode persists after power cycle (if applicable)
- [ ] Radio keyer mode persists after power cycle (if applicable)
- [ ] Buzzer disable state persists after power cycle
- [ ] Memory slot 1 persists after power cycle
- [ ] Memory slot 2 persists after power cycle
- [ ] Memory slot 3 persists after power cycle
- [ ] EEPROM valid flag (0x42) prevents corruption on first boot
- [ ] Settings survive multiple power cycles (test 3+ times)

---

## ✅ Stress & Edge Case Tests

- [ ] Rapid dit/dah alternation works smoothly
- [ ] Very fast input (squeeze) handled correctly
- [ ] Very slow input handled correctly
- [ ] Holding dit for extended period (10+ seconds) works
- [ ] Holding dah for extended period (10+ seconds) works
- [ ] Button mashing doesn't crash or lock up device
- [ ] Switching modes rapidly works reliably
- [ ] Device doesn't overheat during extended use (30+ minutes)
- [ ] USB reconnection after cable unplug/replug works
- [ ] Device recovery after sleep/hibernate works

---

## 🐛 Issues Found

**Instructions:** Check the appropriate box and describe any issues in detail below.

- [ ] ✅ No issues found - all tests passing
- [ ] ⚠️ Minor issues found - doesn't block release
- [ ] ❌ Critical issues found - blocks release

### Issue Reports

**Format for each issue:**
```
### Issue #1: [Brief description]
**Severity:** [Critical / Major / Minor]
**Test Section:** [e.g., "Keyer Mode Tests"]
**Steps to reproduce:**
1. ...
2. ...
3. ...
**Expected behavior:** ...
**Actual behavior:** ...
**Workaround (if any):** ...
**Related GitHub Issue:** #___ (create separate bug issue if critical)
```

---

## 📊 Test Results Summary

**Total Tests Run:** ___ / ___
**Tests Passed:** ___
**Tests Failed:** ___
**Tests Skipped/N/A:** ___

**Overall Result:**
- [ ] ✅ **PASS** - All critical tests passing, ready for release
- [ ] ⚠️ **PASS WITH ISSUES** - Minor issues found, but acceptable for release
- [ ] ❌ **FAIL** - Critical issues found, must be fixed before release

**Tester Sign-Off:**
- [ ] I have thoroughly tested this hardware configuration
- [ ] I have documented all issues found
- [ ] I recommend this configuration for release (or document why not)

**Signed:** ___________ **Date:** YYYY-MM-DD

---

## 💬 Additional Notes & Observations

[Add any additional comments, observations, or suggestions here]

---

**Remember to update the parent issue (#___) with the results from this testing!**
