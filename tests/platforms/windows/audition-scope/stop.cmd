@echo off
"%~dp0..\..\..\..\build\windows\audition-scope\Release\audition-scope.exe" --stop
exit /b %errorlevel%
