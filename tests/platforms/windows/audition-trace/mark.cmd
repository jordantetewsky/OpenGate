@echo off
"%~dp0..\..\..\..\build\windows\audition-trace\Release\audition-trace.exe" --mark
exit /b %errorlevel%
