@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-unbound-release.ps1" %*
exit /b %errorlevel%
