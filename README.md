# ClipWheel

ClipWheel 是一个面向 Windows 桌面的轮盘式剪贴板效率工具，支持热键呼出、历史管理、固定常用内容、自动粘贴与托盘常驻。

## 完整介绍
ClipWheel 的设计目标是把“复制粘贴”从线性、多步骤操作，优化成接近肌肉记忆的瞬时动作。传统剪贴板流程通常是：先复制内容，再切换窗口，回忆目标文本，反复尝试粘贴和撤销；在高频输入场景下，这种流程会不断打断思路。ClipWheel 通过一个可视化的径向轮盘把最近历史和固定文本聚合在同一个入口：用户只需按住热键呼出轮盘，鼠标滑向目标分区，松开即可完成剪贴板回填并可选自动粘贴。应用在后台监听剪贴板变化并维护本地历史数据，控制中心用于管理固定项、删除历史、修改热键和切换自动粘贴模式；托盘交互支持最小化常驻、快捷菜单和通知提示。最终体验不是“多一个工具窗口”，而是将碎片化文本操作压缩为一套连续、低认知成本的桌面交互。

## 功能亮点
- 热键呼出径向轮盘，松开完成选择
- 固定项优先展示，历史项自动更新
- 可选自动粘贴：选中后自动发送 `Ctrl+V`
- 控制中心支持内容管理与热键设置
- 托盘常驻：支持右键菜单、双击恢复、最小化到托盘
- 多分辨率图标与跨平台图标资产（Windows/macOS/Linux）

## 项目结构
```text
clipwheel/
  assets/icons/              # 图标资产（windows/linux/macos）
  packaging/windows/         # Windows 安装脚本模板
  scripts/                   # 资源生成、冒烟测试脚本
  website/                   # 项目介绍网站（可用于 GitHub Pages）
  main.c history.c history.h # 核心源码
  clipwheel.rc resource.h    # Windows 资源定义
  build.bat                  # 本地构建入口
```

## 环境要求
- Windows 10/11 x64
- Visual Studio 2022 Build Tools（C++ toolchain + Windows SDK）

## 构建与运行
1. 在项目根目录执行：
   ```bat
   build.bat
   ```
2. 保证 `clipwheel.exe` 与 `clipwheel.ini` 位于同一目录。
3. 运行 `clipwheel.exe`，应用会打开控制中心并加入系统托盘。

## 配置文件
`clipwheel.ini`

```ini
[Hotkey]
VK=192
Mod=0

[Behavior]
AutoPaste=1
```

- `VK`：十进制虚拟键码
- `Mod`：修饰键位掩码（1=Alt，2=Ctrl，4=Shift，可叠加）
- `AutoPaste`：`1` 自动粘贴，`0` 仅更新剪贴板

## 图标与品牌资源
生成完整图标资产：

```bat
python scripts\generate-brand-assets.py
```

输出包含：
- Windows：`assets/icons/windows/clipwheel.ico`（多尺寸）
- macOS：`assets/icons/clipwheel.icns` + `assets/icons/macos.iconset/`
- Linux：`assets/icons/linux/*.png` + `clipwheel.svg` + `clipwheel.desktop`

## 项目网站
网站入口：
- [website/index.html](C:/Users/Administrator/Projects/clipwheel/website/index.html)

本地预览（任选其一）：
1. 直接双击打开 `website/index.html`
2. 用静态服务器启动目录 `website/`（推荐）

ClipWheel 是一个聚焦中文办公与开发场景的轮盘式剪贴板效率工具。它把“复制-切换窗口-粘贴”的碎片动作整合为一次热键呼出与鼠标方向选择：按住热键调出轮盘，滑向目标内容，松开即可完成回填，并可按配置自动执行粘贴。应用在后台实时记录剪贴板历史，支持固定常用文本、清理历史、快速复制、托盘常驻与热键自定义，让高频输入时的上下文切换更少、误操作更少、节奏更连贯。相较传统列表式剪贴板管理器，ClipWheel 更强调低认知负担和“肌肉记忆化”交互，适合文案编辑、客服话术、开发调试、运营回复等需要反复粘贴与快速切换文本的用户。

## 许可证
见 [LICENSE](C:/Users/Administrator/Projects/clipwheel/LICENSE)
