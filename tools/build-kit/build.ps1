# Vail Adapter - build every firmware variation from this repo (Windows PowerShell)
#
#   tools\build-kit\build.ps1
#
# Compiles the firmware source at the repo root for every hardware variation
# and writes the results to tools\build-kit\uf2_output\ :
#
#   xiao_basic_pcb_v1.uf2   qtpy_basic_pcb_v1.uf2   trinkey_vail_adapter.uf2
#   xiao_basic_pcb_v2.uf2   qtpy_basic_pcb_v2.uf2   arduino_micro.hex
#   xiao_advanced_pcb.uf2   qtpy_advanced_pcb.uf2
#   xiao_non_pcb.uf2        qtpy_non_pcb.uf2
#
# The repo's config.h is backed up and restored automatically - the script
# flips the hardware #define for each variation, then puts it back.
#
# First run downloads arduino-cli (into tools\build-kit\bin) plus the board
# cores and libraries. Requires Python 3 (for the .bin -> .uf2 conversion).
#
# If Windows blocks the script, run it like this:
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
$ErrorActionPreference = 'Stop'

$Here   = Split-Path -Parent $MyInvocation.MyCommand.Path
$Root   = (Resolve-Path (Join-Path $Here '..\..')).Path   # repo root = the sketch folder
$Src    = $Root
$Config = Join-Path $Root 'config.h'
$OutDir = Join-Path $Here 'uf2_output'
$Build  = Join-Path $Here 'build_output'

$Urls = 'https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json,https://adafruit.github.io/arduino-board-index/package_adafruit_index.json'
$Uf2Family = '0x68ED2B88'   # SAMD21 UF2 family id
$Uf2Base   = '0x2000'       # bootloader occupies 0x0000-0x2000

$Defines = @('V1_Basic_PCB','V2_Basic_PCB','Advanced_PCB','NO_PCB_GITHUB_SPECS','TRRS_TRINKEY','ARDUINO_MICRO_BOARD')

# Build matrix: label, fqbn, config-define, output-name, ext
$Variants = @(
  @{ label='XIAO    V1 Basic'; fqbn='Seeeduino:samd:seeed_XIAO_m0';          def='V1_Basic_PCB';        out='xiao_basic_pcb_v1';  ext='uf2' }
  @{ label='XIAO    V2 Basic'; fqbn='Seeeduino:samd:seeed_XIAO_m0';          def='V2_Basic_PCB';        out='xiao_basic_pcb_v2';  ext='uf2' }
  @{ label='XIAO    Advanced'; fqbn='Seeeduino:samd:seeed_XIAO_m0';          def='Advanced_PCB';        out='xiao_advanced_pcb';  ext='uf2' }
  @{ label='XIAO    No-PCB';   fqbn='Seeeduino:samd:seeed_XIAO_m0';          def='NO_PCB_GITHUB_SPECS'; out='xiao_non_pcb';       ext='uf2' }
  @{ label='QT Py   V1 Basic'; fqbn='adafruit:samd:adafruit_qtpy_m0';        def='V1_Basic_PCB';        out='qtpy_basic_pcb_v1';  ext='uf2' }
  @{ label='QT Py   V2 Basic'; fqbn='adafruit:samd:adafruit_qtpy_m0';        def='V2_Basic_PCB';        out='qtpy_basic_pcb_v2';  ext='uf2' }
  @{ label='QT Py   Advanced'; fqbn='adafruit:samd:adafruit_qtpy_m0';        def='Advanced_PCB';        out='qtpy_advanced_pcb';  ext='uf2' }
  @{ label='QT Py   No-PCB';   fqbn='adafruit:samd:adafruit_qtpy_m0';        def='NO_PCB_GITHUB_SPECS'; out='qtpy_non_pcb';       ext='uf2' }
  @{ label='TRRS Trinkey';     fqbn='adafruit:samd:adafruit_TRRStrinkey_m0'; def='TRRS_TRINKEY';        out='trinkey_vail_adapter'; ext='uf2' }
  @{ label='Arduino Micro';    fqbn='arduino:avr:micro';                     def='ARDUINO_MICRO_BOARD'; out='arduino_micro';      ext='hex' }
)
$Cores = @('Seeeduino:samd','adafruit:samd','arduino:avr')

# ---- locate or fetch arduino-cli ------------------------------------------
$LocalCli = Join-Path $Here 'bin\arduino-cli.exe'
if (Get-Command arduino-cli -ErrorAction SilentlyContinue) {
  $Acli = 'arduino-cli'
} elseif (Test-Path $LocalCli) {
  $Acli = $LocalCli
} else {
  Write-Host "==> arduino-cli not found - downloading a local copy into .\bin ..."
  New-Item -ItemType Directory -Force -Path (Join-Path $Here 'bin') | Out-Null
  $Zip = Join-Path $env:TEMP 'arduino-cli.zip'
  Invoke-WebRequest -Uri 'https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Windows_64bit.zip' -OutFile $Zip
  Expand-Archive -Path $Zip -DestinationPath (Join-Path $Here 'bin') -Force
  Remove-Item $Zip -Force
  $Acli = $LocalCli
}
& $Acli version

# ---- python (for uf2conv) --------------------------------------------------
$Py = $null
foreach ($c in @('python','python3','py')) {
  if (Get-Command $c -ErrorAction SilentlyContinue) { $Py = $c; break }
}
if (-not $Py) { Write-Error "Python 3 is required (for uf2conv.py). Install it from python.org or the Microsoft Store."; exit 1 }

# ---- cores + libraries -----------------------------------------------------
Write-Host "==> Updating board index ..."
& $Acli core update-index --additional-urls $Urls
foreach ($c in $Cores) {
  Write-Host "==> Installing core: $c ..."
  & $Acli core install $c --additional-urls $Urls
}
Write-Host "==> Installing libraries ..."
& $Acli lib install MIDIUSB 'Adafruit FreeTouch Library' FlashStorage_SAMD Keyboard | Out-Null

# ---- config.h helpers (backup / activate-one / restore) -------------------
$Backup = "$Config.kitbak"
Copy-Item $Config $Backup -Force
function Set-Config([string]$Active) {
  $text = Get-Content $Config -Raw
  foreach ($d in $Defines) {
    $text = [regex]::Replace($text, "(?m)^(\s*)#define\s+$d\s*$", "`${1}//#define $d")
  }
  $text = [regex]::Replace($text, "(?m)^(\s*)//\s*#define\s+$Active\s*$", "`${1}#define $Active")
  Set-Content -Path $Config -Value $text -NoNewline -Encoding ascii
}

# ---- build every variation -------------------------------------------------
if (Test-Path $OutDir) { Remove-Item -Recurse -Force $OutDir }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$ok = 0; $failed = @()
Write-Host ""
try {
  foreach ($v in $Variants) {
    Write-Host ("==> {0,-22} ({1})" -f $v.label, $v.def)
    Set-Config $v.def
    if (Test-Path $Build) { Remove-Item -Recurse -Force $Build }
    New-Item -ItemType Directory -Force -Path $Build | Out-Null
    & $Acli compile --fqbn $v.fqbn --output-dir $Build --export-binaries $Src *> (Join-Path $Build 'build.log')
    if ($LASTEXITCODE -ne 0) {
      Write-Host "    x compile failed:"; Get-Content (Join-Path $Build 'build.log') -Tail 4 | ForEach-Object { "      $_" }
      $failed += $v.out; continue
    }
    if ($v.ext -eq 'uf2') {
      $bin = Get-ChildItem -Path $Build -Filter *.bin | Select-Object -First 1
      if (-not $bin) { Write-Host "    x no .bin produced"; $failed += $v.out; continue }
      & $Py (Join-Path $Here 'uf2conv.py') -c -f $Uf2Family -b $Uf2Base $bin.FullName -o (Join-Path $OutDir "$($v.out).uf2") | Out-Null
    } else {
      $hex = Get-ChildItem -Path $Build -Filter *.hex | Where-Object { $_.Name -notlike '*with_bootloader*' } | Select-Object -First 1
      if (-not $hex) { Write-Host "    x no .hex produced"; $failed += $v.out; continue }
      Copy-Item $hex.FullName (Join-Path $OutDir "$($v.out).hex") -Force
    }
    Write-Host "    + uf2_output\$($v.out).$($v.ext)"; $ok++
  }
}
finally {
  Move-Item -Path $Backup -Destination $Config -Force   # always restore config.h
  if (Test-Path $Build) { Remove-Item -Recurse -Force $Build }
}

$total = $Variants.Count
Write-Host ""
Write-Host "================================================================"
Write-Host "  Built $ok / $total variations into:  $OutDir"
if ($failed.Count -gt 0) { Write-Host ("  FAILED: " + ($failed -join ' ')) }
Write-Host "================================================================"
