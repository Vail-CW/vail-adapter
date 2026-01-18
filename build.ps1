# VAIL SUMMIT Build Script for PowerShell
# Uses standalone Arduino CLI environment with pinned library versions

param(
    [Parameter(Position=0)]
    [ValidateSet("compile", "upload", "monitor", "list", "clean", "help")]
    [string]$Command = "compile",

    [Parameter(Position=1)]
    [string]$Port = ""
)

# Get project directory (where this script is located)
$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Change to arduino-cli directory for relative paths in config to work
$CliDir = Join-Path $ProjectDir "arduino-cli"
Set-Location $CliDir

# Arduino CLI configuration
$ArduinoCli = Join-Path $CliDir "arduino-cli.exe"
$ConfigFile = Join-Path $CliDir "arduino-cli.yaml"
$Fqbn = "esp32:esp32:adafruit_feather_esp32s3:CDCOnBoot=cdc,PartitionScheme=huge_app,PSRAM=enabled,FlashSize=4M,USBMode=hwcdc"
$BuildDir = Join-Path $ProjectDir "build"

function Write-Header {
    param([string]$Title)
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "  $Title" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
}

function Compile-Firmware {
    Write-Header "VAIL SUMMIT - Compiling Firmware"

    Write-Host "Board: " -NoNewline -ForegroundColor Gray
    Write-Host "Adafruit Feather ESP32-S3 2MB PSRAM"
    Write-Host "Core:  " -NoNewline -ForegroundColor Gray
    Write-Host "ESP32 2.0.14"
    Write-Host ""

    & $ArduinoCli compile `
        --config-file $ConfigFile `
        --fqbn $Fqbn `
        --output-dir $BuildDir `
        --export-binaries `
        $ProjectDir

    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "[ERROR] Compilation failed!" -ForegroundColor Red
        exit 1
    }

    Write-Host ""
    Write-Host "[SUCCESS] " -NoNewline -ForegroundColor Green
    Write-Host "Compilation complete!"
    Write-Host "Output files in: " -NoNewline -ForegroundColor Gray
    Write-Host $BuildDir
}

function Upload-Firmware {
    param([string]$ComPort)

    if ([string]::IsNullOrEmpty($ComPort)) {
        Write-Host "Usage: .\build.ps1 upload COM_PORT" -ForegroundColor Yellow
        Write-Host "Example: .\build.ps1 upload COM31"
        Write-Host ""
        Write-Host "Available ports:"
        & $ArduinoCli board list --config-file $ConfigFile
        exit 1
    }

    Write-Header "VAIL SUMMIT - Uploading Firmware"

    Write-Host "Port: " -NoNewline -ForegroundColor Gray
    Write-Host $ComPort
    Write-Host ""

    & $ArduinoCli upload `
        --config-file $ConfigFile `
        --fqbn $Fqbn `
        --port $ComPort `
        --input-dir $BuildDir `
        $ProjectDir

    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "[ERROR] Upload failed!" -ForegroundColor Red
        exit 1
    }

    Write-Host ""
    Write-Host "[SUCCESS] " -NoNewline -ForegroundColor Green
    Write-Host "Upload complete!"
}

function Start-Monitor {
    param([string]$ComPort)

    if ([string]::IsNullOrEmpty($ComPort)) {
        Write-Host "Usage: .\build.ps1 monitor COM_PORT" -ForegroundColor Yellow
        Write-Host "Example: .\build.ps1 monitor COM31"
        Write-Host ""
        Write-Host "Available ports:"
        & $ArduinoCli board list --config-file $ConfigFile
        exit 1
    }

    Write-Host ""
    Write-Host "Starting serial monitor on $ComPort at 115200 baud..."
    Write-Host "Press Ctrl+C to exit" -ForegroundColor Gray
    Write-Host ""

    & $ArduinoCli monitor `
        --config-file $ConfigFile `
        --port $ComPort `
        --config baudrate=115200
}

function List-Ports {
    Write-Header "Available Serial Ports"
    & $ArduinoCli board list --config-file $ConfigFile
}

function Clean-Build {
    Write-Host ""
    Write-Host "Cleaning build directory..."

    if (Test-Path $BuildDir) {
        Remove-Item -Path $BuildDir -Recurse -Force
        Write-Host "Build directory cleaned." -ForegroundColor Green
    } else {
        Write-Host "Build directory does not exist." -ForegroundColor Yellow
    }
}

function Show-Help {
    Write-Header "VAIL SUMMIT Build Script"

    Write-Host "Usage: .\build.ps1 [command] [options]"
    Write-Host ""
    Write-Host "Commands:" -ForegroundColor Yellow
    Write-Host "  compile        " -NoNewline -ForegroundColor Cyan
    Write-Host "Compile the firmware (default)"
    Write-Host "  upload PORT    " -NoNewline -ForegroundColor Cyan
    Write-Host "Upload firmware to device"
    Write-Host "  monitor PORT   " -NoNewline -ForegroundColor Cyan
    Write-Host "Open serial monitor"
    Write-Host "  list           " -NoNewline -ForegroundColor Cyan
    Write-Host "List available serial ports"
    Write-Host "  clean          " -NoNewline -ForegroundColor Cyan
    Write-Host "Remove build directory"
    Write-Host "  help           " -NoNewline -ForegroundColor Cyan
    Write-Host "Show this help message"
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1                  Compile firmware"
    Write-Host "  .\build.ps1 compile          Compile firmware"
    Write-Host "  .\build.ps1 upload COM31     Upload to COM31"
    Write-Host "  .\build.ps1 monitor COM31    Monitor COM31"
    Write-Host "  .\build.ps1 list             List serial ports"
    Write-Host "  .\build.ps1 clean            Clean build files"
    Write-Host ""
    Write-Host "Environment:" -ForegroundColor Yellow
    Write-Host "  Arduino CLI:   $ArduinoCli"
    Write-Host "  Config File:   $ConfigFile"
    Write-Host "  Board:         Adafruit Feather ESP32-S3 2MB PSRAM"
    Write-Host "  ESP32 Core:    2.0.14"
    Write-Host ""
}

# Execute command
switch ($Command) {
    "compile" { Compile-Firmware }
    "upload"  { Upload-Firmware -ComPort $Port }
    "monitor" { Start-Monitor -ComPort $Port }
    "list"    { List-Ports }
    "clean"   { Clean-Build }
    "help"    { Show-Help }
    default   { Show-Help }
}
