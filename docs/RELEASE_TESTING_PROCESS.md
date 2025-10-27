# Release Testing Process

This document describes how to use GitHub Issues for collaborative release testing of Vail Adapter firmware.

## Overview

Before releasing new firmware to users, we use a **two-tier GitHub Issue system** to coordinate testing across all 8 hardware configurations:

1. **Parent "Overview" Issue** - Tracks overall release progress, links to all hardware-specific issues
2. **Hardware-Specific Issues** - One issue per hardware config (XIAO/QT Py × 4 PCB versions) with detailed test checklists

This ensures quality testing across all hardware variants and catches configuration-specific issues before they reach end users.

## For Release Managers

### Creating Release Testing Issues

**Step 1: Create the Parent Overview Issue**

1. Go to your GitHub repository's Issues tab
2. Click "New Issue"
3. Select the **"Release Testing Overview (Parent Issue)"** template
4. Fill in the version number (e.g., "Release Testing: v2.1.0 - Overview")
5. Update the firmware build date and target release date
6. Fill in the "What's New in This Version" section
7. Create the issue - note the issue number (e.g., #42)

**Step 2: Create Hardware-Specific Testing Issues**

For each of the 8 hardware configurations, create a separate issue:

1. Click "New Issue" again
2. Select the **"Release Testing - Hardware Config"** template
3. Update the title with board and PCB version:
   - "Release Testing v2.1.0 - XIAO Non-PCB"
   - "Release Testing v2.1.0 - XIAO Basic V1"
   - "Release Testing v2.1.0 - XIAO Basic V2"
   - "Release Testing v2.1.0 - XIAO Advanced"
   - "Release Testing v2.1.0 - QT Py Non-PCB"
   - "Release Testing v2.1.0 - QT Py Basic V1"
   - "Release Testing v2.1.0 - QT Py Basic V2"
   - "Release Testing v2.1.0 - QT Py Advanced"
4. Fill in the board type and PCB version checkboxes
5. Link back to the parent issue (replace #___ with parent issue number)
6. Create the issue

**Step 3: Link Hardware Issues to Parent**

1. Go back to the parent overview issue
2. Edit the issue description
3. Add the issue numbers for each hardware config in the tracking table
4. Assign testers to specific hardware configurations (if known)

### Managing the Testing Process

1. **Assign testers** - Assign each hardware-specific issue to someone with that hardware
2. **Monitor progress** - Check the parent issue to see overall status at a glance
3. **Review hardware issues** - Click into individual hardware issues to see detailed test results
4. **Track coverage** - Update the parent issue's hardware tracking table as configs complete
5. **Address issues** - If critical bugs are found, link them in the parent issue's "Critical Issues" section
6. **Make go/no-go decision** - Once minimum requirements are met (2+ configs tested), decide whether to release

### Minimum Release Requirements

Before releasing, ensure:
- ✅ At least **2 different hardware configurations** fully tested and passing
- ✅ At least **2 different people** have signed off
- ✅ No **critical failures** reported in any hardware-specific issue
- ✅ CI/CD build completed successfully for all 8 configs
- ✅ `docs/index.html` updated with release notes

**Ideal Release Requirements:**
- 🎯 At least **4 different hardware configurations** tested (ideally mix of XIAO/QT Py and PCB versions)
- 🎯 Both XIAO and QT Py tested (at least one config each)
- 🎯 At least one Advanced PCB tested (for radio output features)

### After Successful Testing

1. Check off the "Pre-Release Checklist" items in the parent overview issue
2. Update the "Testing Progress Summary" section
3. Mark the parent issue status as "Ready for Release"
4. Create the GitHub release with all 8 firmware files
5. Close all hardware-specific testing issues
6. Close the parent overview issue
7. Announce the release

## For Testers

### How to Test a Release

1. **Find the parent issue** - Look for "Release Testing: v[VERSION] - Overview"
2. **Choose your hardware** - Find the hardware-specific issue that matches your configuration
3. **Claim the issue** - Comment that you're testing this config and add your name to the "Primary Tester" field
4. **Download firmware** - Get the appropriate UF2 file from parent issue or `docs/firmware_files/`
5. **Run through tests** - Check off items in the hardware-specific issue as you test them
6. **Report issues** - Add detailed bug reports in the "Issues Found" section
7. **Sign off** - Check the final "Tester Sign-Off" boxes when complete
8. **Update parent issue** - Comment on the parent issue that your hardware config is complete

### Working with Hardware-Specific Issues

Each hardware config has its own dedicated issue with a comprehensive test checklist organized by category:

- **Core Functionality** - Basic firmware update and operation
- **Keyer Modes** - All 10 keyer modes (passthrough, straight, bug, iambic, etc.)
- **Input Hardware** - Physical and capacitive touch inputs
- **USB Output** - Keyboard and MIDI modes
- **Audio/Sidetone** - Buzzer functionality and control
- **Speed Control** - WPM adjustment and timing accuracy
- **Memory System** - Recording and playback features
- **Button/Menu** - UI navigation and gestures
- **Special Features** - Radio mode, touch inputs, TRS detection
- **Settings Persistence** - EEPROM storage across power cycles
- **Stress Tests** - Edge cases and extended use

### Testing Tips

**You don't need to test everything:**
- Skip tests that don't apply to your hardware (e.g., radio features on non-Advanced PCB)
- Focus on critical functionality first (keyer modes, USB output, basic operations)
- Mark N/A items clearly in comments

**Be thorough where it matters:**
- Test the keyer modes you actually use
- Verify settings persist across power cycles (critical!)
- Test memory system if you use it
- Check stress cases (rapid input, long holds, mode switching)

### Reporting Issues

**In the hardware-specific issue:**
1. Check the appropriate box: "No issues" / "Minor issues" / "Critical issues"
2. Use the provided issue report format in the "Issues Found" section
3. Include: Severity, steps to reproduce, expected vs actual behavior
4. For critical bugs, create a separate GitHub issue and link it

**In the parent issue:**
If you found a **critical** bug that blocks release:
1. Comment on the parent overview issue
2. Describe the critical issue briefly
3. Link to the hardware-specific issue with details
4. Tag the maintainer

**Severity Levels:**
- **Critical** - Firmware doesn't work, data loss, device bricking, major feature broken
- **Major** - Important feature not working, but workarounds exist
- **Minor** - Cosmetic issues, edge cases, nice-to-have features

## Example Workflow

### Complete Testing Cycle Example

**Day 1: Development & CI**
- Maintainer merges feature branches to master
- CI automatically builds all 8 firmware variants
- UF2 files committed to `docs/firmware_files/`

**Day 1: Testing Issues Created**
- Maintainer creates parent overview issue #42 for v2.1.0
- Maintainer creates 8 hardware-specific issues (#43-#50)
- Hardware issues linked to parent issue
- Testers assigned based on available hardware

**Days 2-7: Testing Phase**
- Tester Alice claims issue #44 (XIAO Basic V2), checks off tests as she goes
- Tester Bob claims issue #50 (QT Py Advanced), finds minor buzzer issue ⚠️
- Tester Carol claims issue #43 (XIAO Non-PCB), reports all passing ✅
- Bug issue #51 created for Bob's buzzer issue, fix merged, new firmware built
- Bob retests with new firmware - now passing ✅
- Alice and Carol mark their issues complete with sign-off

**Day 8: Release Decision**
- Maintainer reviews parent issue #42
- Minimum requirements met (3 configs, 3 testers, no critical bugs)
- Updates parent issue: "3/8 configs tested, all passing"
- Maintainer marks parent issue "Ready for Release"
- GitHub release v2.1.0 created with all 8 firmware files
- All testing issues (#42-#50) closed with reference to release

**Day 8: Post-Release**
- Release announced to users
- Testing issues remain as historical documentation
- Users can reference to see which configs were tested

## Benefits of This System

✅ **Hardware-specific testing** - Each config gets thorough, dedicated testing
✅ **Clear ownership** - One tester per hardware config, no confusion
✅ **Collaborative** - Multiple people can test different configs simultaneously
✅ **Scalable** - Easy to see status of all 8 configs at a glance in parent issue
✅ **Comprehensive** - Detailed checklists ensure nothing is missed
✅ **Transparent** - All results visible and linkable
✅ **Documented** - Historical record of exactly what was tested on which hardware
✅ **Flexible** - Can release with 2 configs tested, or wait for more coverage
✅ **Low friction** - No external tools, just GitHub issues
✅ **Trackable** - GitHub shows progress bars based on checked boxes

## Tips & Best Practices

### For Release Managers
- **Create all issues at once** - Parent + 8 hardware issues, link them all together
- **Assign wisely** - Match testers to hardware they actually own
- **Set realistic timeline** - Give testers at least 3-5 days
- **Prioritize diversity** - Better to have 4 different configs than 4 tests of same config
- **Be responsive** - Answer tester questions quickly to unblock them
- **Don't rush** - If critical issues found, fix and retest before release
- **Update parent issue** - Keep the progress summary current so people can see status

### For Testers
- **Claim early** - Add your name as soon as you commit to testing
- **Test systematically** - Work through sections in order, check boxes as you go
- **Focus on critical paths** - Keyer modes, USB output, persistence are most important
- **Power cycle frequently** - Many bugs only show up after restart
- **Document thoroughly** - Good bug reports help maintainers fix issues fast
- **Communicate delays** - If you can't finish, let maintainer know so they can reassign
- **Ask questions** - Better to clarify than test wrong thing

### General
- **Keep templates updated** - Add new test items as firmware features evolve
- **Learn from history** - Review old testing issues to see what bugs were found
- **Use labels effectively** - `testing`, `release`, `critical`, `hardware-specific`
- **Link everything** - Cross-reference related issues, bugs, and releases
- **Celebrate** - Recognize testers' contributions! 🎉

## Troubleshooting

**Q: Do we really need 8 separate testing issues?**
A: Yes! It keeps each hardware config's testing isolated and clear. The parent issue ties them together for easy tracking.

**Q: Can multiple people test the same hardware config?**
A: Yes! The "Additional Testers" section allows multiple people to sign off on the same config. This is great for critical releases.

**Q: What if we can't test all 8 configurations?**
A: That's expected! Minimum is 2 configs. Prioritize:
- Mix of XIAO and QT Py (at least one each)
- Mix of PCB versions (Non-PCB, Basic, Advanced)
- Most common configs your users have

**Q: What if someone finds a critical bug?**
A:
1. Mark it in the hardware-specific issue
2. Create a separate GitHub issue for the bug
3. Add to parent issue's "Critical Issues" section
4. Fix the bug, rebuild all 8 configs
5. Ask testers to retest with new firmware

**Q: How do we handle urgent hotfix releases?**
A: For critical bugs (bricking, security):
- Expedite with just 1-2 configs tested
- Focus on the specific bug area
- Document limited testing in release notes
- Plan full testing for next release

**Q: What if a tester disappears mid-testing?**
A:
- Wait 2-3 days
- Reassign to someone else
- They can pick up where the previous tester left off (checklist shows progress)

**Q: GitHub only allows one person to edit at a time. Is that a problem?**
A: Not really! Most edits are quick (checking a box). If someone's editing, wait 30 seconds and try again.

## Related Documents

- [TESTING_GUIDE.md](TESTING_GUIDE.md) - Detailed manual testing procedures for the firmware update website
- [.github/ISSUE_TEMPLATE/release-testing-overview.md](../.github/ISSUE_TEMPLATE/release-testing-overview.md) - Parent issue template
- [.github/ISSUE_TEMPLATE/release-testing-hardware.md](../.github/ISSUE_TEMPLATE/release-testing-hardware.md) - Hardware-specific issue template
- [CLAUDE.md](../CLAUDE.md) - Project architecture and development guide

## Quick Reference

### Issue Creation Checklist

- [ ] Create parent overview issue with version number
- [ ] Create 8 hardware-specific issues (XIAO × 4 + QT Py × 4)
- [ ] Link hardware issues to parent (add issue numbers)
- [ ] Assign testers to hardware configs they own
- [ ] Attach or link to firmware files
- [ ] Fill in "What's New" section
- [ ] Set target release date

### Release Checklist

- [ ] Minimum 2 hardware configs tested
- [ ] Minimum 2 different testers signed off
- [ ] No critical bugs blocking release
- [ ] CI builds successful for all 8 configs
- [ ] `docs/index.html` updated
- [ ] Update parent issue progress summary
- [ ] Mark parent issue "Ready for Release"
- [ ] Create GitHub release
- [ ] Close all testing issues
- [ ] Announce release