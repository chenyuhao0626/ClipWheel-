@echo off
REM 一次性安装：Visual Studio 2022 Build Tools +「C++ 生成工具」工作负载。
REM 请以管理员身份运行本脚本（右键 → 以管理员身份运行），下载约数 GB，需等待完成。
cd /d "%~dp0"

winget install -e --id Microsoft.VisualStudio.2022.BuildTools ^
  --accept-package-agreements --accept-source-agreements ^
  --override "--passive --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"

if errorlevel 1 (
  echo.
  echo [提示] 若 winget 失败，请从以下地址手动下载 Build Tools：
  echo https://visualstudio.microsoft.com/zh-hans/downloads/
  echo 安装时勾选「使用 C++ 的桌面开发」或「C++ 生成工具」，并包含 Windows SDK。
  pause
  exit /b 1
)

echo.
echo 安装结束后请运行 build.bat 编译 ClipWheel。若仍报找不到 windows.h，请确认已安装 Windows 10/11 SDK。
pause
