---
name: Release Testing Overview (Parent Issue)
about: Parent tracking issue for coordinating testing across all hardware configurations
title: 'Release Testing: v[VERSION] - Overview'
labels: ['testing', 'release', 'tracking']
assignees: ''
---

# Release Testing Overview - v[VERSION]

**Firmware Build Date:** YYYY-MM-DD
**Target Release Date:** YYYY-MM-DD
**Firmware Files Location:** `docs/firmware_files/` or attach below

---

## 📋 What's New in This Version

**New Features:**
- [Add bullet points of new features]
-
-

**Bug Fixes:**
- [Add bullet points of bug fixes]
-
-

**Known Issues:**
- [List any known issues that won't block release]
-

---

## 🎯 Hardware Configuration Testing Status

Create individual testing issues for each hardware configuration below. Check off when testing is complete and passing.

### XIAO SAMD21 Configurations

- [ ] **[XIAO Non-PCB]** - Issue #___ - Tester: ___________
- [ ] **[XIAO Basic PCB V1]** - Issue #___ - Tester: ___________
- [ ] **[XIAO Basic PCB V2]** - Issue #___ - Tester: ___________
- [ ] **[XIAO Advanced PCB]** - Issue #___ - Tester: ___________

### QT Py SAMD21 Configurations

- [ ] **[QT Py Non-PCB]** - Issue #___ - Tester: ___________
- [ ] **[QT Py Basic PCB V1]** - Issue #___ - Tester: ___________
- [ ] **[QT Py Basic PCB V2]** - Issue #___ - Tester: ___________
- [ ] **[QT Py Advanced PCB]** - Issue #___ - Tester: ___________

**Instructions:**
1. Create a new issue for each hardware config using the "Release Testing - Hardware Config" template
2. Link each issue above by replacing #___ with the issue number
3. Assign testers who have access to each hardware configuration
4. Check the box when that config's testing is complete and passing

---

## 📊 Testing Progress Summary

**Configs Tested:** 0 / 8
**Configs Passing:** 0 / 8
**Configs Failing:** 0 / 8
**Critical Issues Found:** 0

**Update this section as testing progresses**

---

## 🐛 Critical Issues Blocking Release

List any critical bugs found during testing that must be fixed before release:

- [ ] No critical issues found

**Critical Issue Format:**
```
- [ ] [Brief description] - Found in: [Hardware Config] - Reported in: Issue #___
```

---

## ✅ Pre-Release Checklist (Maintainer)

### Build & CI
- [ ] Merge test branch to master
- [ ] All 8 hardware configs built successfully in CI
- [ ] UF2 files committed to `docs/firmware_files/`
- [ ] All 8 UF2 files verified (correct size, not corrupted)

### Testing Requirements
- [ ] **Minimum:** At least 2 different hardware configurations tested and passing
- [ ] **Ideal:** At least 4 different hardware configurations tested and passing
- [ ] At least 2 different testers have signed off
- [ ] No critical bugs blocking release

### Documentation Updates
- [ ] `docs/index.html` "What's New" section updated with release date and features
- [ ] Release notes prepared with changelog
- [ ] Known issues documented (if any)

### Release Process
- [ ] GitHub release created with all 8 firmware files attached
- [ ] Release tagged with version number (e.g., v2.1.0)
- [ ] Release announcement prepared for users
- [ ] Close all related testing issues

---

## 🚀 Release Approval

**Minimum Requirements Met:**
- [ ] At least **2 hardware configurations** tested and passing
- [ ] At least **2 different testers** signed off
- [ ] **No critical failures** reported
- [ ] **All CI builds** successful

**Final Approval By:** [Maintainer Name]
**Approval Date:** YYYY-MM-DD

**Status:**
- [ ] ✅ Ready for Release
- [ ] ⚠️ Needs More Testing
- [ ] ❌ Blocked (see critical issues above)

---

## 📝 Notes & Discussion

Use comments below for:
- General discussion about the release
- Cross-hardware issues
- Testing coordination
- Release timeline updates
