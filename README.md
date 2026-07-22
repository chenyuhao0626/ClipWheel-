# ClipWheel

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![CI](https://github.com/FlashNovaYu/ClipWheel-/actions/workflows/ci.yml/badge.svg)](https://github.com/FlashNovaYu/ClipWheel-/actions)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%2F11-blue)]()
[![Version](https://img.shields.io/badge/Version-v2.3.0-green)]()

> 轮盘式剪贴板效率工具 — 热键呼出、径向选择、自动粘贴、托盘常驻

ClipWheel 是一个面向 Windows 桌面的轮盘式剪贴板效率工具。它把「复制 → 切换窗口 → 粘贴」的碎片动作整合为一次热键呼出与鼠标方向选择：按住热键调出轮盘，滑向目标内容，松开即可完成回填，并可按配置自动执行粘贴。

## ✨ 功能亮点

- **热键呼出径向轮盘** — 按住热键，松开完成选择
- **鼠标自动居中** — 呼出时鼠标自动移至屏幕中央，选择后恢复原位
- **固定项 + 历史管理** — 最多 7 个固定项 + 32 条历史记录
- **自动粘贴** — 选中后自动发送 `Ctrl+V`
- **深色/浅色主题** — 完整设计令牌系统，50+ 色值常量
- **控制中心** — 内容管理、热键设置、自动粘贴开关
- **托盘常驻** — 右键菜单、双击恢复、最小化到托盘
- **开机自启动** — 注册表 Run 键
- **撤销操作** — 删除或取消固定后可一键撤销
- **键盘导航** — 方向键 / PgUp / PgDn / Home / End 选择扇区
- **右键菜单** — 复制 / 固定 / 编辑 / 取消固定 / 删除

## 🎯 设计理念

传统剪贴板流程：复制内容 → 切换窗口 → 回忆目标文本 → 反复粘贴撤销。在高频输入场景下，这种流程不断打断思路。

ClipWheel 通过可视化径向轮盘把最近历史和固定文本聚合在同一个入口，将碎片化文本操作压缩为一套连续、低认知成本的桌面交互——不是「多一个工具窗口」，而是接近肌肉记忆的瞬时动作。

适合文案编辑、客服话术、开发调试、运营回复等需要反复粘贴与快速切换文本的场景。

## 🖼️ 截图

| 控制中心 | 轮盘交互 |
|---------|---------|
| ![控制中心](assets/screenshots/screenshot.png) | ![轮盘](assets/screenshots/screenshot_zoom.png) |

## 📦 下载

从 [Releases](https://github.com/FlashNovaYu/ClipWheel-/releases) 下载最新版本，解压后将 `clipwheel.exe` 与 `clipwheel.ini` 放在同一目录即可运行。

## 🛠️ 构建

### 环境要求

- Windows 10/11 x64
- Visual Studio 2022 Build Tools（C++ toolchain + Windows SDK）

### 快速构建

```bat
build.bat
```

`build.bat` 通过 `vswhere` 自动定位 MSVC，执行 `cl /O2 /W3 /utf-8` 编译。资源编译器优先使用 `rc`，回退到 `llvm-rc`。

### 一键安装构建工具

```bat
install-msvc-build-tools.bat
```

### 自测

```bat
clipwheel.exe --selftest
```

无界面模式：`clipwheel.exe --selftest-noui`

冒烟测试：`scripts\smoke-test.bat`

## ⚙️ 配置

`clipwheel.ini` 示例：

```ini
[Hotkey]
VK=192        ; 十进制虚拟键码（192 = 波浪号 `）
Mod=0         ; 修饰键位掩码（1=Alt, 2=Ctrl, 4=Shift，可叠加）

[Behavior]
AutoPaste=1   ; 1=自动粘贴, 0=仅更新剪贴板
```

## 🏗️ 架构

```
ClipWheelOverlay  ← 隐藏顶层弹窗，渲染径向轮盘（分层窗口 alpha ~95%）
ClipWheelMain     ← 控制中心主窗口（~1180×760）
  ├── ClipWheelCardList (pin)      ← 固定项卡片列表
  ├── ClipWheelCardList (history)  ← 历史记录卡片列表
  └── ClipWheelPreview             ← 轮盘预览控件（拖放交互）

WH_KEYBOARD_LL    ← 全局键盘钩子（热键检测 + 松开隐藏）
```

### 数据层

| 常量 | 值 | 含义 |
|------|-----|------|
| `CW_MAX_SLOT` | 7 | 轮盘显示槽位 |
| `CW_MAX_PIN` | 7 | 最大固定项 |
| `CW_MAX_HIST` | 32 | 最大历史条目 |
| `CW_MAX_CHARS` | 4096 | 每条最大文本长度 |

数据持久化至 `%APPDATA%\ClipWheel\clips.bin`（二进制格式，magic `0x43505748`，version 3）。日志输出至 `%APPDATA%\ClipWheel\clipwheel.log`。

## 📁 项目结构

```
clipwheel/
├── main.c              # 主入口：轮盘叠加窗、控制中心、键盘钩子（109KB）
├── app.h               # 共享头文件：设计令牌、常量、类型、函数签名
├── history.c / .h      # 剪贴板历史持久化 + 固定项管理（含自测）
├── draw_utils.c / .h   # GDI 绘图工具集：渐变/圆角/缓动/按钮渲染
├── cardlist.c / .h     # 卡片列表控件：固定项/历史列表/右键菜单/拖拽
├── wheel_preview.c / .h # 轮盘预览控件：控制中心实时轮盘交互
├── undo.c              # 撤销栈
├── build.bat           # 本地构建脚本
├── clipwheel.ini       # 用户配置文件
├── clipwheel.exe       # 编译产物（已提交）
├── .github/workflows/  # CI：构建 + 自测 + 冒烟测试
├── assets/             # 品牌资源与截图
│   ├── icons/          #   多平台图标（Windows/macOS/Linux）
│   └── screenshots/    #   功能截图
├── docs/               # 设计文档与介绍网站
├── packaging/          # 安装包脚本（Inno Setup）
├── scripts/            # 辅助脚本（图标生成、冒烟测试、打包）
└── dist/               # 分发包输出
```

## 🎨 设计令牌

完整设计系统定义在 `app.h` 中：

- **表面层级**：`COL_BG_DEEP` → `COL_BG_SURFACE` → `COL_BG_CARD` → `COL_BG_ELEVATED` → `COL_BG_OVERLAY`
- **强调色**：Indigo 系（`#6366F1` → `#818CF8` → `#C7D2FE`）
- **文本**：四级（Primary / Secondary / Tertiary / Disabled）
- **状态**：Success（绿）、Danger（红）、Border（边框）
- **轮盘**：专用色值（背景、扇区、边框、光晕、取消区）

## 📜 版本历史

详见 [CHANGELOG.md](CHANGELOG.md)

## 📄 许可证

[MIT License](LICENSE)
