@echo off
"%~dp0..\..\..\..\build\windows\audition-trace\Release\audition-trace.exe" --stop
exit /b %errorlevel%
