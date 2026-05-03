@echo off
REM Keep PATH after calling vcvars64; do not use setlocal.
cd /d "%~dp0"

set "WHERE=%SystemRoot%\System32\where.exe"
"%WHERE%" cl >nul 2>&1
if errorlevel 1 goto :bootstrap_msvc
goto :after_vc

:bootstrap_msvc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo [ERROR] vswhere.exe not found. Install Visual Studio 2022 or Build Tools.
  exit /b 1
)
echo Initializing MSVC environment via vswhere...
set "VCVARS="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Auxiliary\Build\vcvars64.bat`) do set "VCVARS=%%i"
if not defined VCVARS for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -find VC\Auxiliary\Build\vcvars64.bat`) do set "VCVARS=%%i"
if not defined VCVARS (
  echo [ERROR] vcvars64.bat not found. Install C++ build tools workload.
  exit /b 1
)
call "%VCVARS%"
"%WHERE%" cl >nul 2>&1
if errorlevel 1 (
  echo [ERROR] cl.exe still not found after vcvars64.bat.
  exit /b 1
)

:after_vc
call :append_windows_kit

:compile
call :compile_resources
if errorlevel 1 exit /b 1
cl /nologo /O2 /W3 /utf-8 /Fe:clipwheel.exe main.c history.c draw_utils.c cardlist.c wheel_preview.c clipwheel.res user32.lib gdi32.lib shell32.lib comctl32.lib dwmapi.lib advapi32.lib
if errorlevel 1 exit /b 1
echo.
echo Build OK: clipwheel.exe generated. Keep clipwheel.ini in the same directory.
exit /b 0

:compile_resources
set "RCEXE="
set "RCMODE=MSRC"
"%WHERE%" rc >nul 2>&1
if not errorlevel 1 set "RCEXE=rc"
if not defined RCEXE (
  "%WHERE%" llvm-rc >nul 2>&1
  if not errorlevel 1 (
    set "RCEXE=llvm-rc"
    set "RCMODE=LLVMRC"
  )
)
:rc_found
if not defined RCEXE (
  echo [ERROR] rc/llvm-rc not found. Install Windows SDK or LLVM resource compiler.
  exit /b 1
)
if /i "%RCMODE%"=="LLVMRC" (
  "%RCEXE%" /C 65001 /fo clipwheel.res clipwheel.rc
) else (
  "%RCEXE%" /nologo clipwheel.rc
)
exit /b %errorlevel%

:append_windows_kit
REM vcvars should include SDK paths; fallback to newest local Windows Kit.
set "KIT10=%ProgramFiles(x86)%\Windows Kits\10"
if not exist "%KIT10%\Include" goto :eof
set "KITVER="
for /f "delims=" %%V in ('dir /b /ad /o-n "%KIT10%\Include" 2^>nul') do (
  set "KITVER=%%V"
  goto :kit_done
)
:kit_done
if not defined KITVER goto :eof
set "INCLUDE=%INCLUDE%;%KIT10%\Include\%KITVER%\ucrt;%KIT10%\Include\%KITVER%\shared;%KIT10%\Include\%KITVER%\um"
set "LIB=%LIB%;%KIT10%\Lib\%KITVER%\ucrt\x64;%KIT10%\Lib\%KITVER%\um\x64"
goto :eof
