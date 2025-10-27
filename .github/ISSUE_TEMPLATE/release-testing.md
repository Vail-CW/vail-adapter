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

## 📋 Testing Sign-Off

**Instructions for Testers:**
1. Comment on this issue with your testing results using the template below
2. Test on your available hardware configuration(s)
3. Check off completed tests in your comment
4. Report any failures with details

**Sign-off Template (copy and paste into a comment):**

```markdown
### Testing Results - [Your Name]

**Date:** YYYY-MM-DD
**Hardware:** [ ] XIAO SAMD21 / [ ] QT Py SAMD21
**PCB Version:** [ ] Non-PCB / [ ] Basic V1 / [ ] Basic V2 / [ ] Advanced
**Browser Tested:** [e.g., Chrome v120, Firefox v115]
**Operating System:** [e.g., Windows 11, macOS 14, Ubuntu 22.04]

#### ✅ Core Functionality Tests
- [ ] Device enters bootloader mode successfully
- [ ] Firmware flashes successfully
- [ ] Device restarts and operates normally after update

#### ✅ Keyer Mode Tests
- [ ] Straight key mode works
- [ ] Bug mode works (auto-dit)
- [ ] Iambic A mode works
- [ ] Iambic B mode works
- [ ] Other keyer modes tested: _____________

#### ✅ USB Output Tests
- [ ] Keyboard mode outputs Ctrl keys correctly
- [ ] MIDI mode outputs notes correctly
- [ ] Mode switching via MIDI CC0 works

#### ✅ Audio/Sidetone Tests
- [ ] Sidetone buzzer sounds on dit/dah
- [ ] Tone frequency adjustment works (button controls)
- [ ] Buzzer disable feature works (5-second DIT hold)

#### ✅ Speed Control Tests
- [ ] Speed setting mode accessible via button
- [ ] WPM adjusts correctly (5-40 WPM range)
- [ ] Speed persists after power cycle
- [ ] MIDI CC1 speed control works (if tested)

#### ✅ Memory System Tests
- [ ] Memory management mode accessible (B1+B3 combo)
- [ ] Recording to slot 1 works
- [ ] Recording to slot 2 works
- [ ] Recording to slot 3 works
- [ ] Playback from each slot works
- [ ] Memory clear function works
- [ ] Memories persist after power cycle

#### ✅ Button/Menu Tests
- [ ] Quick press plays memories in normal mode
- [ ] Long press enters settings modes
- [ ] Button combos work (B1+B3 for memory mode)
- [ ] Double-click detection works (if applicable)
- [ ] Mode transitions work correctly

#### ✅ Special Features Tests (if applicable)
- [ ] Radio mode toggle works (10 DAH presses in 500ms)
- [ ] Radio keyer mode toggle works (5-second DAH hold in radio mode)
- [ ] Radio output pins function correctly (Advanced PCB only)
- [ ] Capacitive touch inputs work (if using touch pads)
- [ ] TRS detection for straight key works

#### ✅ Settings Persistence Tests
- [ ] Keyer type persists after power cycle
- [ ] Speed setting persists after power cycle
- [ ] Tone setting persists after power cycle
- [ ] Radio keyer mode persists after power cycle

#### 🐛 Issues Found
- [ ] No issues found
- [ ] Issues found (describe below)

**Issue Details:**
[If you found any issues, describe them here with steps to reproduce]

**Additional Notes:**
[Any other observations, suggestions, or comments]

**Overall Result:** [ ] ✅ PASS / [ ] ❌ FAIL / [ ] ⚠️ PASS WITH ISSUES
```

---

## 🎯 Hardware Configuration Coverage

Track which configurations have been tested by checking off below:

### XIAO SAMD21
- [ ] Non-PCB/Breadboard - Tester: ___________
- [ ] Basic PCB V1 - Tester: ___________
- [ ] Basic PCB V2 - Tester: ___________
- [ ] Advanced PCB - Tester: ___________

### QT Py SAMD21
- [ ] Non-PCB/Breadboard - Tester: ___________
- [ ] Basic PCB V1 - Tester: ___________
- [ ] Basic PCB V2 - Tester: ___________
- [ ] Advanced PCB - Tester: ___________

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

- [ ] Merge test branh to master
- [ ] All 8 hardware configs built successfully and automatic after merge
- [ ] At least 2 hardware configurations tested by different people
- [ ] All critical tests passing
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