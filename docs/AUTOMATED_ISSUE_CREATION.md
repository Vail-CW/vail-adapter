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
   - **Build and attach firmware**: `true` (recommended) or `false`
6. Click **"Run workflow"** green button
7. Wait for completion:
   - **With firmware build**: 5-10 minutes (builds all 8 configs)
   - **Without firmware build**: 10-30 seconds (issues only)
8. Check the **Issues** tab - you'll see 9 new issues!
9. If firmware was built, each hardware issue will have a comment with download link

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

### 4. Optional Firmware Builds (Recommended!)
If "Build and attach firmware" is enabled:
- All 8 firmware configurations built automatically
- UF2 files stored as workflow artifacts (30-day retention)
- Comment posted on each hardware issue with:
  - Firmware filename
  - Build date and version
  - Commit hash
  - File size
  - Link to download from workflow artifacts
- **Important:** Test firmware is NOT committed to `docs/firmware_files/` (keeps it private)
- Firmware only becomes public after testing approval and merge to master

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

### Build and Attach Firmware (Required)
**Format:** Boolean checkbox - `true` (checked) or `false` (unchecked)

**Used for:**
- Building test firmware for all 8 hardware configurations
- Providing firmware directly to testers via issue comments
- Keeping test firmware private (not on GitHub Pages)

**Recommended:** ✅ **true** (checked)

**When to use `true`:**
- Testing a new feature or bug fix from a branch
- Want to ensure testers have exact firmware build
- Want to keep test firmware private until approved

**When to use `false`:**
- Firmware already exists in `docs/firmware_files/`
- Testing production firmware from master branch
- Want faster issue creation (10 seconds vs 5-10 minutes)
- Will manually attach firmware files

**Example:** Leave checked (default: `true`)

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

### Step 3: Verify Firmware Availability

**If you built firmware (recommended):**
- Check each hardware issue for the firmware comment
- Verify the workflow artifacts link works
- Testers can download directly from their issue

**If you didn't build firmware:**
- Ensure firmware files are in `docs/firmware_files/`
- Or manually attach UF2 files to each hardware issue
- Or provide download links in parent issue

### Step 4: Notify Testers
- Comment on parent issue tagging all testers
- Or send direct messages with links to their assigned issues
- Include target completion date

### Step 5: Monitor Progress
- Check parent issue for overall status
- Review individual hardware issues for detailed results
- Respond to questions and bug reports

## Workflow Technical Details

### Architecture

The workflow has two main jobs that run sequentially:

**Job 1: build-test-firmware (Optional, 5-10 minutes)**
- Runs only if "Build and attach firmware" is enabled
- Uses matrix strategy to build all 8 configs in parallel
- Same build process as production `build_uf2.yml` workflow
- Uploads UF2 files as workflow artifacts (30-day retention)
- Does NOT commit files to repository (keeps test firmware private)

**Job 2: create-issues (Always runs, 10-30 seconds)**
- Waits for firmware build to complete (if enabled)
- Downloads firmware artifacts (if built)
- Creates parent overview issue
- Creates 8 hardware-specific issues
- Updates parent issue with hardware issue links
- Posts firmware info comments on hardware issues (if built)
- Posts summary comment on parent issue

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

✅ **Speed** - 5-10 minutes (with builds) vs. hours of manual work
✅ **Consistency** - All issues use same format every time
✅ **Accuracy** - No typos or missed steps
✅ **Linking** - Issues automatically cross-linked
✅ **Convenience** - Single click instead of 9 issue creations + 8 builds
✅ **Less Error-Prone** - No forgetting to create a hardware config issue
✅ **Private Test Firmware** - Builds stay as artifacts, not on GitHub Pages
✅ **Exact Builds** - Testers get exact firmware from specific commit/branch
✅ **Audit Trail** - Firmware comment shows commit hash, date, size

## Related Documentation

- [RELEASE_TESTING_PROCESS.md](RELEASE_TESTING_PROCESS.md) - Complete testing workflow guide
- [.github/ISSUE_TEMPLATE/release-testing-overview.md](../.github/ISSUE_TEMPLATE/release-testing-overview.md) - Parent issue template
- [.github/ISSUE_TEMPLATE/release-testing-hardware.md](../.github/ISSUE_TEMPLATE/release-testing-hardware.md) - Hardware issue template
- [.github/workflows/create-release-testing-issues.yml](../.github/workflows/create-release-testing-issues.yml) - The workflow file itself

## Firmware Build Workflow

### Why Build Firmware in the Workflow?

**Problem:** If you commit test firmware to `docs/firmware_files/`, it becomes publicly available on your GitHub Pages site before testing is complete.

**Solution:** Build firmware as workflow artifacts that are:
- Only accessible to people with repo access
- Attached to issues via comments with download links
- Automatically deleted after 30 days
- Never committed to the repository

### How It Works:

```
Test Branch (e.g., feature/new-keyer-mode)
    ↓
Run "Create Release Testing Issues" workflow
    ↓
[Build Job] Compile all 8 configs → Store as artifacts (private)
    ↓
[Issues Job] Create 9 issues → Post firmware links in comments
    ↓
Testers download firmware from workflow artifacts link
    ↓
Testing happens over 3-7 days
    ↓
All tests pass! ✅
    ↓
Merge test branch → master
    ↓
[build_uf2.yml workflow runs automatically]
    ↓
Commits production firmware to docs/firmware_files/ (public)
    ↓
GitHub Pages serves tested, approved firmware to users
```

### Downloading Firmware from Workflow Artifacts:

**For Testers:**
1. Go to your assigned hardware issue
2. Find the "Test Firmware Attached" comment
3. Click the "workflow artifacts" link
4. Scroll down to "Artifacts" section
5. Download your hardware config's UF2 file (e.g., `xiao_basic_pcb_v2.uf2`)

**Note:** You need read access to the repository to download artifacts.

## Example Usage

### Scenario: Preparing for v2.1.0 Release

**Day 1 - Monday (Test Branch Approach):**
1. Create test branch `release/v2.1.0` from master with new features
2. Push test branch to GitHub
3. Go to Actions → "Create Release Testing Issues"
4. Select branch: `release/v2.1.0`
5. Fill in workflow form:
   - Version: `v2.1.0`
   - Build date: `2025-01-27` (today)
   - Target date: `2025-02-03` (one week)
   - Build firmware: ✅ **checked** (true)
   - What's new:
     ```
     - Added keyahead mode
     - Fixed EEPROM persistence bug
     - Improved touch sensitivity
     ```
6. Click "Run workflow"
7. Wait 5-10 minutes for builds to complete
8. Workflow creates parent issue #50 and hardware issues #51-#58
9. Each hardware issue has firmware comment with download link
10. Edit parent issue to add known issues section
11. Assign testers:
    - Alice: XIAO Basic V2 (#53)
    - Bob: QT Py Advanced (#58)
    - Carol: XIAO Non-PCB (#51)
12. Comment on parent issue tagging @alice, @bob, @carol

**Days 2-6 - Testing:**
- Alice downloads firmware from issue #53's comment (clicks artifact link)
- Bob downloads firmware from issue #58's comment
- Carol downloads firmware from issue #51's comment
- Testers flash firmware to their hardware
- Check off tests as they complete them
- Bob reports minor buzzer issue in issue #58

**Day 4 - Wednesday (Bug Fix):**
- Bug #59 created for Bob's buzzer issue
- Fix pushed to `release/v2.1.0` branch
- Re-run "Create Release Testing Issues" workflow? No, just rebuild:
  - Push new commit to test branch
  - Manually attach new UF2 to issue #58
  - Or: Bob can build locally from branch
- Bob retests with fixed firmware - passes ✅

**Day 7 - Friday (Release):**
- Review parent issue #50
- 3/8 configs tested and passing
- Bug fix merged into `release/v2.1.0` branch
- No critical bugs remaining
- Mark parent issue "Ready for Release"
- Merge `release/v2.1.0` → `master`
- CI automatically builds and commits to `docs/firmware_files/`
- Create GitHub release v2.1.0 with files from `docs/firmware_files/`
- Close all 9 testing issues
- Tested firmware now public on GitHub Pages!

## Video Tutorial (Future)

*TODO: Record a screen capture showing the workflow in action*

## Feedback

If you have suggestions for improving this workflow, please:
- Open a GitHub issue with the `enhancement` label
- Or submit a pull request with your improvements
