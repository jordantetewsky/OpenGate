@echo off
"%~dp0..\..\..\..\build\windows\audition-trace\Release\audition-trace.exe" --start
exit /b %errorlevel%
