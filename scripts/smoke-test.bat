@echo off
REM Smoke test: build, start app, verify process exists, then stop.
setlocal
cd /d "%~dp0.."
call build.bat
if errorlevel 1 exit /b 1
C:\Windows\System32\taskkill.exe /IM clipwheel.exe /F >nul 2>&1
start /B "" clipwheel.exe
C:\Windows\System32\ping.exe -n 4 127.0.0.1 >nul
C:\Windows\System32\tasklist.exe /FI "IMAGENAME eq clipwheel.exe" | C:\Windows\System32\findstr.exe clipwheel.exe
if errorlevel 1 (
  echo [FAIL] clipwheel.exe process not detected.
  exit /b 1
)
C:\Windows\System32\taskkill.exe /IM clipwheel.exe /F >nul
echo [PASS] Process starts successfully. Smoke test done.
