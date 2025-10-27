---
name: Release Testing Checklist
about: Use this template for testing new firmware releases before publishing
title: 'Release Testing: v[VERSION]'
labels: ['testing', 'release']
assignees: ''
---

# Release Testing Checklist - v[VERSION]

**Firmware Build Date:** YYYY-MM-DD
**Target Release Date:** YYYY-MM-DD
**Firmware Files Location:** attached below

---

## 📋 Testing Instructions

**How to test:**
1. Check off the hardware configuration(s) you're testing in the "Hardware Configuration Coverage" section below
2. Test your available hardware and check off items as you complete them
3. Add comments to this issue with any failures, notes, or observations
4. Update the "Tester Notes" section with your findings

---

## 🎯 Hardware Configuration Coverage

**Instructions:** Check the box for the config you're testing and add your name. Test results are tracked in the sections below.

### XIAO SAMD21
- [ ] **Non-PCB/Breadboard** - Tester: ___________
- [ ] **Basic PCB V1** - Tester: ___________
- [ ] **Basic PCB V2** - Tester: ___________
- [ ] **Advanced PCB** - Tester: ___________

### QT Py SAMD21
- [ ] **Non-PCB/Breadboard** - Tester: ___________
- [ ] **Basic PCB V1** - Tester: ___________
- [ ] **Basic PCB V2** - Tester: ___________
- [ ] **Advanced PCB** - Tester: ___________

---

## ✅ Core Functionality Tests

- [ ] Firmware update website loads correctly
- [ ] UF2 file downloads for selected hardware config
- [ ] Device enters bootloader mode successfully
- [ ] Firmware flashes successfully (UF2 copied to bootloader drive)
- [ ] Device restarts and operates normally after update
- [ ] LED indicators work as expected

---

## ✅ Keyer Mode Tests

- [ ] Straight key mode (mode 1) works
- [ ] Bug mode (mode 2) - auto-dit works
- [ ] El-Bug mode (mode 3) works
- [ ] Single-dot mode (mode 4) works
- [ ] Ultimatic mode (mode 5) works
- [ ] Plain keyer mode (mode 6) works
- [ ] Iambic A mode (mode 7) works
- [ ] Iambic B mode (mode 8) works - most popular mode
- [ ] Key-ahead mode (mode 9) works
- [ ] Keyer mode can be changed via buttons

---

## ✅ USB Output Tests

- [ ] Keyboard mode outputs Ctrl keys correctly (dit=Left Ctrl, dah=Right Ctrl)
- [ ] Can switch to MIDI mode via MIDI CC0
- [ ] MIDI mode outputs notes correctly (C, C#, D for straight/dit/dah)
- [ ] MIDI control messages work (CC1=speed, CC2=tone, PC=keyer type)

---

## ✅ Audio/Sidetone Tests

- [ ] Sidetone buzzer sounds on dit/dah inputs
- [ ] Tone frequency is audible and clear
- [ ] Tone frequency adjustment works (via button long-press)
- [ ] Tone frequency persists after power cycle
- [ ] Buzzer disable feature works (5-second DIT hold, LED blinks slowly)
- [ ] Buzzer state persists after power cycle

---

## ✅ Speed Control Tests

- [ ] Speed setting mode accessible via button long-press
- [ ] WPM increases with button presses
- [ ] WPM decreases with button presses
- [ ] WPM range works correctly (5-40 WPM)
- [ ] Speed persists after power cycle
- [ ] MIDI CC1 speed control works (if MIDI tested)
- [ ] Dit timing is accurate at various speeds

---

## ✅ Memory System Tests

- [ ] Memory management mode accessible (B1+B3 combo press)
- [ ] Can record to memory slot 1 (piezo feedback during recording)
- [ ] Can record to memory slot 2
- [ ] Can record to memory slot 3
- [ ] Playback from slot 1 works (piezo-only in memory mode)
- [ ] Playback from slot 2 works
- [ ] Playback from slot 3 works
- [ ] Can play memories in normal mode (full output via button quick-press)
- [ ] Memory clear function works (long press while playing)
- [ ] Memories persist after power cycle
- [ ] Recording captures timing accurately (up to 25 seconds)
- [ ] Recording auto-trims trailing silence

---

## ✅ Button/Menu Tests

- [ ] Button presses detected reliably
- [ ] Quick press triggers appropriate action
- [ ] Long press (2 seconds) enters settings modes
- [ ] Combo press (B1+B3) toggles memory management mode
- [ ] Double-click detection works (within 400ms)
- [ ] Mode transitions work correctly (NORMAL ↔ SPEED ↔ TONE ↔ KEY)
- [ ] Can't accidentally enter wrong mode from current mode
- [ ] Menu system is intuitive and predictable

---

## ✅ Special Features Tests

**Radio Mode Features (if HAS_RADIO_OUTPUT defined):**
- [ ] Radio mode toggle works (10 DAH presses within 500ms, LED blinks rapidly)
- [ ] Radio keyer mode toggle works (5-second DAH hold in radio mode)
- [ ] Radio output pins A2/A3 function correctly (Advanced PCB only)
- [ ] Radio mode state persists after power cycle
- [ ] Radio keyer mode state persists after power cycle

**Capacitive Touch (if using touch pads):**
- [ ] Capacitive touch dit input works
- [ ] Capacitive touch dah input works
- [ ] Capacitive touch key input works
- [ ] Touch sensitivity is appropriate

**TRS Detection:**
- [ ] TRS jack detection works for straight key auto-configuration
- [ ] Switching between paddle and straight key works reliably

---

## ✅ Settings Persistence Tests

- [ ] Keyer type persists after power cycle
- [ ] Dit duration (speed) persists after power cycle
- [ ] TX note (tone) persists after power cycle
- [ ] Radio keyer mode persists after power cycle (if applicable)
- [ ] All 3 memory slots persist after power cycle
- [ ] EEPROM valid flag (0x42) prevents corruption on first boot

---

## 🐛 Issues & Tester Notes

**Instructions:** Comment on this issue with any bugs, observations, or notes. Reference specific test items above.

**Format for reporting issues:**
```
### Issue: [Brief description]
**Tester:** [Your name]
**Hardware:** [e.g., XIAO Basic V2]
**Severity:** [Critical / Major / Minor]
**Steps to reproduce:**
1. ...
2. ...
**Expected:** ...
**Actual:** ...
```

---

## 📝 Release Notes Preview

**What's New in This Version:**
- [Add bullet points of new features/fixes for this release]
-
-

**Known Issues:**
- [List any known issues that won't block release]
-

---

## ✅ Pre-Release Checklist (Maintainer)

- [ ] Merge test branch to master
- [ ] All 8 hardware configs built successfully in CI (automatic after merge)
- [ ] UF2 files committed to `docs/firmware_files/`
- [ ] At least 2 hardware configurations tested by different people
- [ ] All critical tests passing (no critical bugs blocking release)
- [ ] `docs/index.html` "What's New" section updated with release date and features
- [ ] GitHub release created with firmware files attached
- [ ] Release announcement prepared

---

## 🚀 Release Approval

**Minimum Requirements:**
- At least **2 different hardware configurations** tested and passing
- At least **2 different testers** sign off
- **No critical failures** reported
- Maintainer review completed

**Final Approval:** Brett Hollifield
**Status:** [ ] Ready for Release / [ ] Blocked / [ ] Needs More Testing