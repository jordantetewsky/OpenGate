@echo off
"%~dp0..\..\..\..\build\windows\audition-context\Release\audition-context.exe" --stop
exit /b %errorlevel%
