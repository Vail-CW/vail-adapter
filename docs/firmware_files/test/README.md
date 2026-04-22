# Test firmware channel

Files in this directory are **pre-release / beta builds** served to users who have opted into the test channel on [vailadapter.com](https://vailadapter.com).

## How files get here

Not by hand. Trigger the GitHub Actions workflow:

1. GitHub → **Actions** → **Build and Deploy Arduino UF2 Firmware**
2. Click **Run workflow** (top-right)
3. Pick the branch you want to build (e.g. a PR branch)
4. Set **Where to deploy the built firmware** = `test`
5. Run

The workflow builds all 10 targets (9 SAMD21 UF2s + Arduino Micro HEX) from that branch, checks out `master`, and commits the binaries to this directory with a message like:

```
chore: update TEST firmware from <branch> @ <sha>
```

A `BUILD_INFO.txt` is written alongside the firmware so testers can confirm what they're flashing.

## How testers reach them

Users activate the test channel on vailadapter.com by either:

- Clicking the **🧪 Test channel** link in the site navigation
- Visiting the site with `?test=1` in the URL

Once active, a persistent warning banner appears, and the wizard downloads from `firmware_files/test/…` instead of `firmware_files/…`.

## Promoting a test build to stable

When a test build is validated:

1. Merge the source branch to `master` (this triggers a normal push build that deploys to `firmware_files/` via the CI)
2. Optionally clean up `firmware_files/test/` — stale test builds are not auto-purged
