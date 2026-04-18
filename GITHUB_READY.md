# ClipWheel GitHub 上架准备

本文档用于快速完成 ClipWheel 的 GitHub 发布与展示准备。

## 1. 仓库命名建议
- `clipwheel`

## 2. 仓库 About（简短）
- 轮盘式剪贴板效率工具：热键呼出、历史管理、固定文本、自动粘贴与托盘常驻。

## 3. 仓库详细介绍（长文案）
ClipWheel 是一个聚焦中文办公与开发场景的轮盘式剪贴板效率工具。它把“复制-切换窗口-粘贴”的碎片动作整合为一次热键呼出与鼠标方向选择：按住热键调出轮盘，滑向目标内容，松开即可完成回填，并可按配置自动执行粘贴。应用在后台实时记录剪贴板历史，支持固定常用文本、清理历史、快速复制、托盘常驻与热键自定义，让高频输入时的上下文切换更少、误操作更少、节奏更连贯。相较传统列表式剪贴板管理器，ClipWheel 更强调低认知负担和“肌肉记忆化”交互，适合文案编辑、客服话术、开发调试、运营回复等需要反复粘贴与快速切换文本的用户。

## 4. 建议标签（Topics）
- `clipboard`
- `windows`
- `win32`
- `productivity`
- `hotkey`
- `radial-menu`
- `tool`

## 5. Releases 建议内容（v2.0）
- 全新控制中心与中文界面优化
- 轮盘交互与松开触发逻辑修复
- 图标系统升级（多分辨率 ICO + 跨平台资源）
- 托盘菜单与最小化托盘体验增强
- 项目官网更新（`website/index.html` 单文件站点）

## 6. GitHub Pages
1. 进入仓库 `Settings -> Pages`
2. `Build and deployment` 选择 `Deploy from a branch`
3. Branch 选择 `main`，目录选择 `/website`
4. 保存后等待生成，访问站点 URL 验证页面

## 7. 推荐上传资产
- `clipwheel.exe`
- `clipwheel.ini`
- `README.md`
- 截图/GIF（控制中心 + 轮盘呼出效果）
