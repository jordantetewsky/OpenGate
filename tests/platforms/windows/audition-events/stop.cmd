@echo off
"%~dp0..\..\..\..\build\windows\audition-events\Release\audition-events.exe" --stop
exit /b %errorlevel%
