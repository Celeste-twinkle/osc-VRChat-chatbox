@echo off
setlocal
cd /d "%~dp0"
if not exist dist mkdir dist
rem 1. Build the embedded UI (compile Svelte, minify, merge -> ui\dist\)
node ui\build.js || exit /b 1
rem 2. Stop any running instance so FASM can overwrite the exe
taskkill /im vrc-chatbox-osc-asm.exe /f >nul 2>&1
timeout /t 1 /nobreak >nul
..\tools\fasm\FASM.EXE server.asm dist\vrc-chatbox-osc-asm.exe
