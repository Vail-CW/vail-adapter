# Build kit

Compile the Vail Adapter firmware locally and get a `.uf2` (or `.hex` for the
Arduino Micro) for **every** hardware variation, in one command. This builds the
firmware source at the **repo root**, so what you flash always matches the code
in this checkout.

## Run it

From the repo root, or from this folder:

### Linux / macOS
```bash
chmod +x tools/build-kit/build.sh    # first time only
tools/build-kit/build.sh
```

### Windows
- Double-click `tools\build-kit\build.bat`, or
- From PowerShell:
  ```powershell
  powershell -NoProfile -ExecutionPolicy Bypass -File tools\build-kit\build.ps1
  ```

## What you get

Every variation is built into `tools/build-kit/uf2_output/`:

```
uf2_output/
├── xiao_basic_pcb_v1.uf2     qtpy_basic_pcb_v1.uf2
├── xiao_basic_pcb_v2.uf2     qtpy_basic_pcb_v2.uf2
├── xiao_advanced_pcb.uf2     qtpy_advanced_pcb.uf2
├── xiao_non_pcb.uf2          qtpy_non_pcb.uf2
├── trinkey_vail_adapter.uf2
└── arduino_micro.hex         (Arduino Micro is AVR -> .hex, not .uf2)
```

The script edits the repo's `config.h` to select each hardware target, then
**restores your original `config.h`** automatically when it finishes (even if a
build fails). You don't have to touch `config.h` yourself.

## Requirements

- **Internet** on the first run - it downloads arduino-cli (into
  `tools/build-kit/bin/`) plus the board cores (Seeed SAMD, Adafruit SAMD,
  Arduino AVR) and libraries, a few hundred MB. Later runs are fast and offline.
- **Python 3** - used only for the `.bin -> .uf2` step. Preinstalled on most
  Linux/macOS; on Windows install from python.org or the Microsoft Store.
- You do not need to install arduino-cli yourself; the script fetches a local
  copy if it isn't already on your PATH.

## Flash a build

1. Plug the board in with a data USB cable.
2. Double-tap the reset button - a removable drive appears (`XIAOBOOT`,
   `QTPYBOOT`, `ADAPTERBOOT`, ...).
3. Drag the matching `uf2_output/<board>.uf2` onto that drive.

The Arduino Micro doesn't use UF2 - flash `arduino_micro.hex` with
`arduino-cli upload` or a WebSerial/AVR109 flasher instead.

Firmware for releases is published as GitHub Release assets; this kit is for
building locally from source.
