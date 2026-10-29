@echo off
"%~dp0..\..\..\..\build\windows\audition-context\Release\audition-context.exe" --start
exit /b %errorlevel%
