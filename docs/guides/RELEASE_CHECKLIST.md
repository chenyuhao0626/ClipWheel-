# GitHub 发布清单（v2.1.0）

## 发布前（代码与内容）
- [ ] 确认网站入口为 `website/index.html`（用户最终版本）
- [ ] 确认 `README.md`、`CHANGELOG.md`、`LICENSE`、`GITHUB_READY.md` 已更新
- [ ] 确认未提交构建产物（`*.exe/*.obj/*.res/*.pdb`）
- [ ] 本地执行 `build.bat`
- [ ] 本地执行 `scripts\\smoke-test.bat`（如当前仓库提供）

## 发布中（GitHub 操作）
- [ ] 推送代码到 GitHub 仓库（建议：`clipwheel`）
- [ ] 设置 About 与 Topics：`clipboard` `productivity` `windows` `win32` `hotkey` `radial-menu`
- [ ] 启用 GitHub Pages（`Settings -> Pages -> main /website`）
- [ ] 创建 Release：`v2.1.0`
- [ ] 发布说明粘贴 `RELEASE_NOTES_v2.1.0.md`
- [ ] 上传附件：`clipwheel.exe` `clipwheel.ini`（可选再加 zip）

## 发布后（校验）
- [ ] 校验 Release 附件可下载、可运行
- [ ] 校验 GitHub Pages 页面可访问、样式正常
- [ ] 校验 README 链接与文档路径有效
- [ ] 补充 1~2 张界面截图或 GIF 提升展示质量
