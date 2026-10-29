@echo off
"%~dp0..\..\..\..\build\windows\audition-map\Release\audition-map.exe" --capture
exit /b %errorlevel%
