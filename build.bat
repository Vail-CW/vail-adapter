@echo off
REM VAIL SUMMIT Build Script for Windows
REM Uses standalone Arduino CLI environment with pinned library versions

setlocal EnableDelayedExpansion

REM Get the directory where this script is located
set "PROJECT_DIR=%~dp0"
REM Remove trailing backslash if present
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

REM Change to arduino-cli directory for relative paths in config to work
set "CLI_DIR=%PROJECT_DIR%\arduino-cli"
cd /d "%CLI_DIR%"

REM Arduino CLI configuration
set "ARDUINO_CLI=%CLI_DIR%\arduino-cli.exe"
set "CONFIG_FILE=%CLI_DIR%\arduino-cli.yaml"
set "FQBN=esp32:esp32:adafruit_feather_esp32s3:CDCOnBoot=cdc,PartitionScheme=huge_app,PSRAM=enabled,FlashSize=4M,USBMode=hwcdc"
set "SKETCH=%PROJECT_DIR%"
set "BUILD_DIR=%PROJECT_DIR%\build"

REM Check for command line arguments
if "%1"=="" goto compile
if "%1"=="compile" goto compile
if "%1"=="upload" goto upload
if "%1"=="monitor" goto monitor
if "%1"=="list" goto list
if "%1"=="clean" goto clean
if "%1"=="help" goto help
goto help

:compile
echo.
echo ========================================
echo  VAIL SUMMIT - Compiling Firmware
echo ========================================
echo.
echo Board: Adafruit Feather ESP32-S3 2MB PSRAM
echo Core: ESP32 2.0.14
echo.

"%ARDUINO_CLI%" compile ^
    --config-file "%CONFIG_FILE%" ^
    --fqbn "%FQBN%" ^
    --output-dir "%BUILD_DIR%" ^
    --export-binaries ^
    "%SKETCH%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilation failed!
    exit /b 1
)

echo.
echo [SUCCESS] Compilation complete!
echo Output files in: %BUILD_DIR%
goto end

:upload
REM Check if port is specified
if "%2"=="" (
    echo Usage: build.bat upload COM_PORT
    echo Example: build.bat upload COM31
    echo.
    echo Available ports:
    "%ARDUINO_CLI%" board list --config-file "%CONFIG_FILE%"
    exit /b 1
)

set "PORT=%2"
echo.
echo ========================================
echo  VAIL SUMMIT - Uploading Firmware
echo ========================================
echo.
echo Port: %PORT%
echo.

"%ARDUINO_CLI%" upload ^
    --config-file "%CONFIG_FILE%" ^
    --fqbn "%FQBN%" ^
    --port "%PORT%" ^
    --input-dir "%BUILD_DIR%" ^
    "%SKETCH%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Upload failed!
    exit /b 1
)

echo.
echo [SUCCESS] Upload complete!
goto end

:monitor
REM Check if port is specified
if "%2"=="" (
    echo Usage: build.bat monitor COM_PORT
    echo Example: build.bat monitor COM31
    echo.
    echo Available ports:
    "%ARDUINO_CLI%" board list --config-file "%CONFIG_FILE%"
    exit /b 1
)

set "PORT=%2"
echo.
echo Starting serial monitor on %PORT% at 115200 baud...
echo Press Ctrl+C to exit
echo.

"%ARDUINO_CLI%" monitor ^
    --config-file "%CONFIG_FILE%" ^
    --port "%PORT%" ^
    --config baudrate=115200

goto end

:list
echo.
echo ========================================
echo  Available Serial Ports
echo ========================================
echo.
"%ARDUINO_CLI%" board list --config-file "%CONFIG_FILE%"
goto end

:clean
echo.
echo Cleaning build directory...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    echo Build directory cleaned.
) else (
    echo Build directory does not exist.
)
goto end

:help
echo.
echo ========================================
echo  VAIL SUMMIT Build Script
echo ========================================
echo.
echo Usage: build.bat [command] [options]
echo.
echo Commands:
echo   compile        Compile the firmware (default)
echo   upload PORT    Upload firmware to device
echo   monitor PORT   Open serial monitor
echo   list           List available serial ports
echo   clean          Remove build directory
echo   help           Show this help message
echo.
echo Examples:
echo   build.bat                  Compile firmware
echo   build.bat compile          Compile firmware
echo   build.bat upload COM31     Upload to COM31
echo   build.bat monitor COM31    Monitor COM31
echo   build.bat list             List serial ports
echo   build.bat clean            Clean build files
echo.
echo Environment:
echo   Arduino CLI:   %ARDUINO_CLI%
echo   Config File:   %CONFIG_FILE%
echo   Board:         Adafruit Feather ESP32-S3 2MB PSRAM
echo   ESP32 Core:    2.0.14
echo.

:end
endlocal
