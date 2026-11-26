@echo off
cd /d "%~dp0"
if not exist criminalmap.exe call compilar.bat semPausa
if not exist criminalmap.exe exit /b 1
start "" http://127.0.0.1:8080
criminalmap.exe
pause
