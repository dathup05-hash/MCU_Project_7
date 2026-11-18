@echo off
REM Chay toan bo unit test bang Ceedling (gcc + ruby cua MSYS2 UCRT64)
set PATH=C:\msys64\ucrt64\bin;%PATH%
cd /d "%~dp0"
ceedling test:all
pause
