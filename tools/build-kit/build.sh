#!/usr/bin/env bash
#
# Vail Adapter - build every firmware variation from this repo (Linux / macOS)
#
#   tools/build-kit/build.sh
#
# Compiles the firmware source at the repo root for every hardware variation
# and writes the results to tools/build-kit/uf2_output/ :
#
#   xiao_basic_pcb_v1.uf2   qtpy_basic_pcb_v1.uf2   trinkey_vail_adapter.uf2
#   xiao_basic_pcb_v2.uf2   qtpy_basic_pcb_v2.uf2   arduino_micro.hex
#   xiao_advanced_pcb.uf2   qtpy_advanced_pcb.uf2
#   xiao_non_pcb.uf2        qtpy_non_pcb.uf2
#
# The repo's config.h is backed up and restored automatically - the script
# flips the hardware #define for each variation, then puts it back.
#
# First run downloads arduino-cli (into tools/build-kit/bin) plus the board
# cores and libraries - needs internet and a few hundred MB. Later runs are
# fast. Requires curl, and Python 3 (for the .bin -> .uf2 conversion).
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"     # repo root = the Arduino sketch folder
SRC="$ROOT"                           # sketch to compile (vail-adapter.ino lives here)
CONFIG="$ROOT/config.h"
OUTDIR="$HERE/uf2_output"
BUILD="$HERE/build_output"

# SAMD21 board-manager index URLs (comma-separated for --additional-urls)
URLS="https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json,https://adafruit.github.io/arduino-board-index/package_adafruit_index.json"
UF2_FAMILY="0x68ED2B88"   # SAMD21 UF2 family id
UF2_BASE="0x2000"         # bootloader occupies 0x0000-0x2000

# All hardware selection macros in config.h
DEFINES="V1_Basic_PCB V2_Basic_PCB Advanced_PCB NO_PCB_GITHUB_SPECS TRRS_TRINKEY ARDUINO_MICRO_BOARD"

# Build matrix:  "label|fqbn|config-define|output-name|ext"
VARIANTS=(
  "XIAO    V1 Basic|Seeeduino:samd:seeed_XIAO_m0|V1_Basic_PCB|xiao_basic_pcb_v1|uf2"
  "XIAO    V2 Basic|Seeeduino:samd:seeed_XIAO_m0|V2_Basic_PCB|xiao_basic_pcb_v2|uf2"
  "XIAO    Advanced|Seeeduino:samd:seeed_XIAO_m0|Advanced_PCB|xiao_advanced_pcb|uf2"
  "XIAO    No-PCB|Seeeduino:samd:seeed_XIAO_m0|NO_PCB_GITHUB_SPECS|xiao_non_pcb|uf2"
  "QT Py   V1 Basic|adafruit:samd:adafruit_qtpy_m0|V1_Basic_PCB|qtpy_basic_pcb_v1|uf2"
  "QT Py   V2 Basic|adafruit:samd:adafruit_qtpy_m0|V2_Basic_PCB|qtpy_basic_pcb_v2|uf2"
  "QT Py   Advanced|adafruit:samd:adafruit_qtpy_m0|Advanced_PCB|qtpy_advanced_pcb|uf2"
  "QT Py   No-PCB|adafruit:samd:adafruit_qtpy_m0|NO_PCB_GITHUB_SPECS|qtpy_non_pcb|uf2"
  "TRRS Trinkey|adafruit:samd:adafruit_TRRStrinkey_m0|TRRS_TRINKEY|trinkey_vail_adapter|uf2"
  "Arduino Micro (.hex)|arduino:avr:micro|ARDUINO_MICRO_BOARD|arduino_micro|hex"
)
CORES="Seeeduino:samd adafruit:samd arduino:avr"

# ---- locate or fetch arduino-cli ------------------------------------------
if command -v arduino-cli >/dev/null 2>&1; then
  ACLI="arduino-cli"
elif [ -x "$HERE/bin/arduino-cli" ]; then
  ACLI="$HERE/bin/arduino-cli"
else
  echo "==> arduino-cli not found - downloading a local copy into ./bin ..."
  mkdir -p "$HERE/bin"
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$HERE/bin" sh
  ACLI="$HERE/bin/arduino-cli"
fi
echo "==> $("$ACLI" version)"

# ---- python (for uf2conv) --------------------------------------------------
PY=""
for c in python3 python; do command -v "$c" >/dev/null 2>&1 && { PY="$c"; break; }; done
[ -z "$PY" ] && { echo "ERROR: Python 3 is required (for uf2conv.py)."; exit 1; }

# ---- cores + libraries -----------------------------------------------------
echo "==> Updating board index ..."
"$ACLI" core update-index --additional-urls "$URLS"
for c in $CORES; do
  echo "==> Installing core: $c ..."
  "$ACLI" core install "$c" --additional-urls "$URLS"
done
echo "==> Installing libraries ..."
"$ACLI" lib install MIDIUSB "Adafruit FreeTouch Library" FlashStorage_SAMD Keyboard >/dev/null

# ---- config.h backup/restore (the real repo config.h at the root) ---------
cp "$CONFIG" "$CONFIG.kitbak"
restore_config() { [ -f "$CONFIG.kitbak" ] && mv -f "$CONFIG.kitbak" "$CONFIG"; }
trap restore_config EXIT

sed_i() { local tmp; tmp="$(mktemp)"; sed -E "$1" "$2" > "$tmp" && mv "$tmp" "$2"; }
set_config() {   # $1 = hardware define to activate
  local active="$1" d
  for d in $DEFINES; do
    sed_i "s|^([[:space:]]*)#define[[:space:]]+$d([[:space:]]*)\$|\1//#define $d|" "$CONFIG"
  done
  sed_i "s|^([[:space:]]*)//[[:space:]]*#define[[:space:]]+$active([[:space:]]*)\$|\1#define $active|" "$CONFIG"
}

# ---- build every variation -------------------------------------------------
rm -rf "$OUTDIR"; mkdir -p "$OUTDIR"
ok=0; fail=0; FAILED=""
echo ""
for v in "${VARIANTS[@]}"; do
  IFS='|' read -r label fqbn def out ext <<< "$v"
  printf '==> %-22s %s\n' "$label" "($def)"
  set_config "$def"
  rm -rf "$BUILD"; mkdir -p "$BUILD"
  if ! "$ACLI" compile --fqbn "$fqbn" --output-dir "$BUILD" --export-binaries "$SRC" >"$BUILD/build.log" 2>&1; then
    echo "    x compile failed:"; tail -4 "$BUILD/build.log" | sed 's/^/      /'
    FAILED="$FAILED $out"; fail=$((fail+1)); continue
  fi
  if [ "$ext" = "uf2" ]; then
    bin="$(ls "$BUILD"/*.bin 2>/dev/null | head -n1)"
    [ -z "$bin" ] && { echo "    x no .bin produced"; FAILED="$FAILED $out"; fail=$((fail+1)); continue; }
    "$PY" "$HERE/uf2conv.py" -c -f "$UF2_FAMILY" -b "$UF2_BASE" "$bin" -o "$OUTDIR/$out.uf2" >/dev/null
  else
    hex="$(find "$BUILD" -name '*.hex' ! -name '*with_bootloader*' | head -n1)"
    [ -z "$hex" ] && { echo "    x no .hex produced"; FAILED="$FAILED $out"; fail=$((fail+1)); continue; }
    cp "$hex" "$OUTDIR/$out.hex"
  fi
  echo "    + uf2_output/$out.$ext"; ok=$((ok+1))
done

rm -rf "$BUILD"
echo ""
echo "================================================================"
echo "  Built $ok / $((ok+fail)) variations into:  $OUTDIR"
[ "$fail" -ne 0 ] && echo "  FAILED:$FAILED"
echo "================================================================"
[ "$fail" -eq 0 ]
