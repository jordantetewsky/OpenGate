@echo off
"%~dp0..\..\..\..\build\windows\audition-scope\Release\audition-scope.exe" --start
exit /b %errorlevel%
