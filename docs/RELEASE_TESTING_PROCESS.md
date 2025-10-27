# Release Testing Process

This document describes how to use GitHub Issues for collaborative release testing of Vail Adapter firmware.

## Overview

Before releasing new firmware to users, we use a GitHub Issue with task lists to coordinate testing across multiple hardware configurations and testers. This ensures quality and catches issues before they reach end users.

## For Release Managers

### Creating a Release Testing Issue

1. Go to your GitHub repository's Issues tab
2. Click "New Issue"
3. Select the "Release Testing Checklist" template
4. Fill in the version number in the title (e.g., "Release Testing: v2.1.0")
5. Update the firmware build date and target release date
6. Add the release notes preview section with what's new
7. Create the issue

### Managing the Testing Process

1. **Assign testers** - Comment on the issue and tag people who can test specific hardware configs
2. **Monitor progress** - Check the issue regularly for tester comments and sign-offs
3. **Track coverage** - Update the "Hardware Configuration Coverage" section as tests are completed
4. **Address issues** - If testers report bugs, create separate GitHub issues and link them
5. **Make go/no-go decision** - Once minimum requirements are met, decide whether to release

### Minimum Release Requirements

Before releasing, ensure:
- ✅ At least **2 different hardware configurations** tested
- ✅ At least **2 different people** have signed off
- ✅ No **critical failures** reported
- ✅ CI/CD build completed successfully for all 8 configs
- ✅ `docs/index.html` updated with release notes

### After Successful Testing

1. Check off the "Pre-Release Checklist" items in the issue
2. Mark the issue status as "Ready for Release"
3. Create the GitHub release with firmware files
4. Close the testing issue
5. Announce the release

## For Testers

### How to Test a Release

1. **Find the testing issue** - Look for an open issue titled "Release Testing: v[VERSION]"
2. **Choose your hardware** - Identify which configuration(s) you can test
3. **Download firmware** - Get the appropriate UF2 file from the issue or `docs/firmware_files/`
4. **Run through tests** - Use the sign-off template in the issue
5. **Report results** - Post a comment with your filled-out testing template

### Using the Testing Template

Copy the sign-off template from the issue into a new comment:

```markdown
### Testing Results - [Your Name]

**Date:** 2025-01-26
**Hardware:** [X] XIAO SAMD21 / [ ] QT Py SAMD21
**PCB Version:** [ ] Non-PCB / [X] Basic V2 / [ ] Advanced
...
```

### What to Test

The template includes sections for:
- **Core Functionality** - Basic firmware update and operation
- **Keyer Modes** - Various CW keyer types
- **USB Output** - Keyboard and MIDI modes
- **Audio/Sidetone** - Buzzer functionality
- **Speed Control** - WPM adjustment
- **Memory System** - Recording and playback
- **Button/Menu** - UI navigation
- **Special Features** - Advanced features (radio mode, touch, etc.)
- **Settings Persistence** - EEPROM storage across power cycles

You don't need to test every single item if it doesn't apply to your hardware (e.g., radio output on non-Advanced PCB), but try to cover as much as possible.

### Reporting Issues

If you find a bug:
1. Check the "Issues found" box in your template
2. Describe the issue in detail under "Issue Details"
3. Include steps to reproduce
4. Note whether it's critical (blocks release) or minor
5. Mark your overall result as ❌ FAIL or ⚠️ PASS WITH ISSUES

### Tips for Effective Testing

- Test systematically - go through each section in order
- Power cycle between major test sections to verify persistence
- Try edge cases (e.g., very fast/slow speeds, full memory slots)
- Note your specific hardware configuration clearly
- Include any relevant context (USB cable type, computer OS, etc.)

## Example Workflow

### Complete Testing Cycle Example

**Week 1: Development & CI**
- Maintainer merges feature branches
- CI builds all 8 firmware variants
- UF2 files committed to repository

**Week 1: Testing Issue Created**
- Maintainer creates release testing issue for v2.1.0
- Issue tagged with 'testing' and 'release' labels
- Testers assigned or notified

**Week 1-2: Testing Phase**
- Tester Alice tests XIAO Basic V2 - reports all pass ✅
- Tester Bob tests QT Py Advanced - finds minor buzzer issue ⚠️
- Tester Carol tests XIAO Non-PCB - reports all pass ✅
- Bug issue created for buzzer issue, fix merged
- Bob retests - now passes ✅

**Week 2: Release Decision**
- Minimum requirements met (3 configs, 3 testers)
- No critical issues
- Maintainer marks "Ready for Release"
- GitHub release created
- Testing issue closed

**Week 2: Post-Release**
- Release announced
- Issue remains as documentation of testing
- Users can reference for known issues

## Benefits of This System

✅ **Collaborative** - Multiple people can test simultaneously
✅ **Transparent** - All testing results visible in one place
✅ **Documented** - Historical record of what was tested
✅ **Flexible** - Template can be adapted for different test needs
✅ **Low friction** - No external tools, just GitHub issues
✅ **Trackable** - Easy to see what's tested and what's not

## Tips & Best Practices

### For Release Managers
- Create testing issue as soon as firmware builds are ready
- Give testers at least 3-5 days for testing
- Prioritize getting diverse hardware coverage over exhaustive testing
- Be responsive to tester questions and issues
- Don't rush the release if critical issues are found

### For Testers
- Test as soon as you can to avoid blocking release
- Be thorough but pragmatic - focus on critical functionality
- Report issues clearly with reproducible steps
- If you can't complete testing, comment so others know
- Update your hardware configuration in the coverage section

### General
- Keep the template up to date as firmware features evolve
- Archive old testing issues for historical reference
- Use labels effectively (testing, release, critical, etc.)
- Link related issues (bugs found during testing)
- Celebrate successful releases! 🎉

## Troubleshooting

**Q: Can multiple people edit the main checklist?**
A: GitHub issues only allow one person to edit at a time, but multiple people can comment with their individual test results. This is actually better for tracking who tested what.

**Q: What if we can't test all 8 configurations?**
A: Prioritize the most common configurations. Minimum requirement is 2 configs, but more is better. Consider which hardware your users actually have.

**Q: What if someone finds a critical bug?**
A: Create a separate GitHub issue for the bug, fix it, rebuild firmware, and ask testers to retest. Update the testing issue with the new firmware version.

**Q: How do we handle urgent hotfix releases?**
A: For critical security or bricking bugs, you can expedite testing with just 1 tester on 1 config, but document the limited testing scope in the release notes.

## Related Documents

- [TESTING_GUIDE.md](TESTING_GUIDE.md) - Detailed manual testing procedures for the firmware update website
- [.github/ISSUE_TEMPLATE/release-testing.md](../.github/ISSUE_TEMPLATE/release-testing.md) - The actual issue template
- [CLAUDE.md](../CLAUDE.md) - Project architecture and development guide