# Automated Release Testing Issue Creation

This document explains how to use the GitHub Actions workflow to automatically create all 9 testing issues (1 parent + 8 hardware-specific) with a single click.

## Quick Start

1. Go to your repository on GitHub
2. Click **Actions** tab
3. Select **"Create Release Testing Issues"** workflow (left sidebar)
4. Click **"Run workflow"** button (top right)
5. Fill in the form:
   - **Release version**: e.g., `v2.1.0`
   - **Firmware build date**: e.g., `2025-01-26`
   - **Target release date**: e.g., `2025-02-02`
   - **What's new**: Brief list of new features (optional)
6. Click **"Run workflow"** green button
7. Wait 10-30 seconds for issues to be created
8. Check the **Issues** tab - you'll see 9 new issues!

## What Gets Created

### 1. Parent Overview Issue
- Title: `Release Testing: v2.1.0 - Overview`
- Labels: `testing`, `release`, `tracking`
- Contains:
  - Release notes preview
  - Hardware configuration tracking table (with links to hardware issues)
  - Testing progress summary
  - Critical issues section
  - Pre-release checklist
  - Release approval section

### 2. Eight Hardware-Specific Issues
Each with:
- Title: `Release Testing v2.1.0 - [BOARD] [PCB_VERSION]`
- Labels: `testing`, `release`
- Pre-filled with:
  - Correct board and PCB version checkboxes marked
  - Link back to parent issue
  - Correct firmware filename
  - Complete test checklist (150+ items)
  - Tester sign-off section

**Hardware configs created:**
1. XIAO SAMD21 Non-PCB
2. XIAO SAMD21 Basic V1
3. XIAO SAMD21 Basic V2
4. XIAO SAMD21 Advanced
5. QT Py SAMD21 Non-PCB
6. QT Py SAMD21 Basic V1
7. QT Py SAMD21 Basic V2
8. QT Py SAMD21 Advanced

### 3. Automatic Linking
- Parent issue automatically updated with links to all 8 hardware issues
- Each hardware issue links back to parent
- Summary comment posted on parent issue with all issue numbers

## Form Fields Explained

### Release Version (Required)
**Format:** `vX.Y.Z` (e.g., `v2.1.0`, `v3.0.0`)

**Used for:**
- Issue titles
- Parent issue headings
- Hardware issue headers

**Example:** `v2.1.0`

### Firmware Build Date (Required)
**Format:** `YYYY-MM-DD` (e.g., `2025-01-26`)

**Used for:**
- Tracking when firmware was built
- Helps identify which CI run produced the firmware

**Example:** `2025-01-26`

### Target Release Date (Required)
**Format:** `YYYY-MM-DD` (e.g., `2025-02-02`)

**Used for:**
- Setting expectations for testers
- Planning timeline

**Example:** `2025-02-02` (one week after build date)

### What's New (Optional)
**Format:** Markdown list (one feature per line, starting with `-`)

**Used for:**
- Parent issue "What's New" section
- Can be edited later in the issue

**Example:**
```
- Added new Ultimatic keyer mode
- Fixed memory persistence bug
- Improved capacitive touch sensitivity
```

**Note:** If left blank, a placeholder will be inserted that you can edit later.

## After Issues Are Created

### Step 1: Review Parent Issue
1. Click on the parent issue link in the workflow summary
2. Edit the parent issue to add/update:
   - Bug fixes section
   - Known issues section
   - Any other release-specific details

### Step 2: Assign Testers
1. Look at the 8 hardware-specific issues
2. Assign each issue to someone who owns that hardware
3. Add their names to the parent issue's tracking table

### Step 3: Make Firmware Available
Ensure firmware files are available in one of these ways:
- Already in `docs/firmware_files/` (if CI built them)
- Attach UF2 files to parent issue
- Provide download links in parent issue

### Step 4: Notify Testers
- Comment on parent issue tagging all testers
- Or send direct messages with links to their assigned issues
- Include target completion date

### Step 5: Monitor Progress
- Check parent issue for overall status
- Review individual hardware issues for detailed results
- Respond to questions and bug reports

## Workflow Technical Details

### Permissions Required
The workflow needs `issues: write` permission, which is automatically granted by GitHub when using `GITHUB_TOKEN`.

### Workflow Trigger
- **Manual only** - Triggered via `workflow_dispatch`
- No automatic triggers (you control when to create issues)

### What the Workflow Does

1. **Creates Parent Issue**
   - Uses `actions/github-script@v7` to call GitHub Issues API
   - Populates template with your input values
   - Stores parent issue number for next steps

2. **Reads Hardware Template**
   - Checks out repository code
   - Reads `.github/ISSUE_TEMPLATE/release-testing-hardware.md`
   - Removes YAML frontmatter

3. **Creates 8 Hardware Issues**
   - Loops through all 8 hardware configurations
   - Customizes template for each (board, PCB version, firmware file)
   - Marks appropriate checkboxes
   - Links back to parent issue
   - Stores all issue numbers

4. **Updates Parent Issue**
   - Fetches parent issue body
   - Replaces `Issue #___` placeholders with actual issue numbers
   - Updates parent issue with new body

5. **Posts Summary**
   - Comments on parent issue with all links
   - Creates workflow run summary with clickable links

### Runtime
Typical execution time: **10-30 seconds** depending on GitHub API responsiveness.

### Error Handling
If the workflow fails:
- Check the Actions run logs for error messages
- Common issues:
  - Permissions problem (ensure `issues: write` is set)
  - Invalid date format
  - Template file not found (ensure templates exist in repo)
- You can re-run the workflow or create issues manually

## Customization

### Modifying Issue Templates
If you update the issue templates (`.github/ISSUE_TEMPLATE/*.md`), the workflow will automatically use the updated templates on the next run.

**Important:** Don't change the template variable placeholders:
- `[VERSION]` - Will be replaced with version
- `[BOARD]` - Will be replaced with board name
- `[PCB_VERSION]` - Will be replaced with PCB version
- `#___` - Will be replaced with parent issue number

### Adding More Hardware Configs
To add more hardware configurations (beyond the current 8):

1. Edit `.github/workflows/create-release-testing-issues.yml`
2. Find the `configs` array in the "Create Hardware-Specific Issues" step
3. Add new entries:
   ```javascript
   { board: 'New Board', pcb: 'New PCB', file: 'new_board.uf2' }
   ```
4. Update the parent template to include the new config in the tracking table

### Changing Labels
To use different labels, edit the workflow:
- Find `labels: ['testing', 'release', 'tracking']` (parent issue)
- Find `labels: ['testing', 'release']` (hardware issues)
- Change to your preferred labels

## Troubleshooting

### Q: Workflow doesn't appear in Actions tab
**A:** Ensure the workflow file is committed to the `master` branch. Workflow files must be on the default branch to appear.

### Q: "Run workflow" button is grayed out
**A:** You need write access to the repository. Check your permissions.

### Q: Issues created but not linked together
**A:** The "Update Parent Issue" step may have failed. Manually edit the parent issue to add the issue numbers.

### Q: Can I delete and recreate issues if I make a mistake?
**A:** Yes! Close/delete the issues and re-run the workflow. GitHub will create new issues with new numbers.

### Q: What if I want to test this without creating real issues?
**A:** Create a test repository, copy the templates and workflow, and run it there first.

### Q: Can I customize the issue content each time?
**A:** Currently, only version, dates, and "what's new" are customizable via the form. For more customization, edit the templates before running the workflow.

## Benefits Over Manual Creation

✅ **Speed** - 10 seconds vs. 15 minutes manual work
✅ **Consistency** - All issues use same format every time
✅ **Accuracy** - No typos or missed steps
✅ **Linking** - Issues automatically cross-linked
✅ **Convenience** - Single click instead of 9 issue creations
✅ **Less Error-Prone** - No forgetting to create a hardware config issue

## Related Documentation

- [RELEASE_TESTING_PROCESS.md](RELEASE_TESTING_PROCESS.md) - Complete testing workflow guide
- [.github/ISSUE_TEMPLATE/release-testing-overview.md](../.github/ISSUE_TEMPLATE/release-testing-overview.md) - Parent issue template
- [.github/ISSUE_TEMPLATE/release-testing-hardware.md](../.github/ISSUE_TEMPLATE/release-testing-hardware.md) - Hardware issue template
- [.github/workflows/create-release-testing-issues.yml](../.github/workflows/create-release-testing-issues.yml) - The workflow file itself

## Example Usage

### Scenario: Preparing for v2.1.0 Release

**Day 1 - Monday:**
1. Merge feature branches to master
2. CI builds all 8 firmware configs and commits to `docs/firmware_files/`
3. Run "Create Release Testing Issues" workflow:
   - Version: `v2.1.0`
   - Build date: `2025-01-27` (today)
   - Target date: `2025-02-03` (one week)
   - What's new:
     ```
     - Added keyahead mode
     - Fixed EEPROM persistence bug
     - Improved touch sensitivity
     ```
4. Workflow completes in 15 seconds
5. Review parent issue #50 and 8 hardware issues #51-#58
6. Edit parent issue to add known issues section
7. Assign testers:
   - Alice: XIAO Basic V2 (#53)
   - Bob: QT Py Advanced (#58)
   - Carol: XIAO Non-PCB (#51)
8. Comment on parent issue tagging @alice, @bob, @carol

**Days 2-6 - Testing:**
- Testers work through their assigned issues
- Check off tests as they complete them
- Report any bugs found

**Day 7 - Friday:**
- Review parent issue
- 3/8 configs tested and passing
- No critical bugs
- Mark parent issue "Ready for Release"
- Create GitHub release v2.1.0
- Close all 9 testing issues

## Video Tutorial (Future)

*TODO: Record a screen capture showing the workflow in action*

## Feedback

If you have suggestions for improving this workflow, please:
- Open a GitHub issue with the `enhancement` label
- Or submit a pull request with your improvements
