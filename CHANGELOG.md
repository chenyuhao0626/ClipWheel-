# Changelog

## 2.2.0 - 2026-05-03
- 新增键盘方向键轮盘导航（↑↓←→ / PgUp/PgDn/Home/End），无需鼠标即可选择扇区
- 新增右键上下文菜单（复制/固定/编辑/取消固定/删除）
- 新增撤销栈（Undo）支持，删除或取消固定后可一键撤销
- 新增开机自启动选项（注册表 Run 键）
- 新增 Ctrl+F 搜索栏基础框架
- 修复热键吞键问题：Mod=0 时不再拦截修饰键组合（如 Shift+波浪号）
- 修复浮动模式 overlay 超出屏幕导致的扇区黑色空洞
- 修复浮动模式轮盘位置偏移：统一 ClientToScreen 坐标参照系
- 修复控制中心缩放残影：WM_SETREDRAW + RDW_UPDATENOW 原子重绘
- 修复轮盘顶部/底部截断：扩大 overlay 边界并修正边界 clamp
- 将 main.c (2849行) 拆分为 5 个模块：app.h / draw_utils.c / cardlist.c / wheel_preview.c / main.c
- 新增 GitHub Actions CI 工作流（构建 + 自测 + 冒烟测试）
- 新增 AGENTS.md 项目指引文件
- 整理项目目录结构，文档归档至 docs/，截图归档至 assets/screenshots/

## 2.0.0 - 2026-04-18
- 重构主界面与轮盘交互流程
- 新增控制中心与托盘菜单增强
- 新增图标资产生成脚本（Windows/macOS/Linux）
- 补充安装脚本模板与网站介绍页
- 完善仓库文档与发布准备
