@echo off
REM Double-click wrapper for build.ps1 (Windows). Builds every firmware
REM variation from the repo's firmware source into uf2_output\.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
echo.
pause
