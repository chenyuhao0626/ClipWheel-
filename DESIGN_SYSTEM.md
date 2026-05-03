# ClipWheel 现代化设计系统

## 设计理念
"简约高级灵动" - 融合 Apple 设计语言的精致感、Google Material Design 3 的动态感，以及 AI 时代的前卫美学。

## 核心设计原则
1. **玻璃态优先**: 使用半透明、模糊和光影创造层次感
2. **渐变柔和**: 避免生硬的纯色，使用微妙的渐变过渡
3. **呼吸感设计**: 适当的留白和圆角创造舒适感
4. **动态反馈**: 每个交互都有细腻的视觉反馈
5. **色彩克制**: 精简的色板，每个颜色都有明确目的

## 色彩系统

### 主色调 - AI 渐变系列
```cpp
// 品牌主色 - 现代蓝紫渐变
#define COL_BRAND_START    RGB(99, 102, 241)   // Indigo-500
#define COL_BRAND_END      RGB(168, 85, 247)   // Purple-500
#define COL_BRAND_LIGHT    RGB(199, 210, 254)  // Indigo-200
#define COL_BRAND_DARK     RGB(79, 70, 229)    // Indigo-600

// 功能色
#define COL_SUCCESS        RGB(34, 197, 94)    // Green-500
#define COL_WARNING        RGB(251, 191, 36)   // Amber-400
#define COL_ERROR          RGB(239, 68, 68)    // Red-500
```

### 深色模式背景系统
```cpp
// 多层次深色背景
#define COL_BG_DEEP        RGB(9, 9, 11)       // 深邃黑底
#define COL_BG_SURFACE     RGB(24, 24, 27)     // 表面层
#define COL_BG_CARD        RGB(39, 39, 42)     // 卡片层
#define COL_BG_HOVER       RGB(63, 63, 70)     // 悬停层

// 玻璃态背景
#define COL_GLASS_DARK     RGBA(39, 39, 42, 0.8)    // 暗色玻璃
#define COL_GLASS_LIGHT    RGBA(255, 255, 255, 0.1) // 亮色玻璃
```

### 文本颜色系统
```cpp
// 文本层级
#define COL_TEXT_PRIMARY   RGB(250, 250, 250)  // 主要文本
#define COL_TEXT_SECONDARY RGB(163, 163, 174)  // 次要文本
#define COL_TEXT_TERTIARY  RGB(113, 113, 122)  // 辅助文本
#define COL_TEXT_DISABLED  RGB(82, 82, 91)     // 禁用文本
```

### 边框和分割线
```cpp
#define COL_BORDER_SUBTLE  RGB(63, 63, 70)     // 微妙边框
#define COL_BORDER_FOCUS   RGB(129, 140, 248)  // 聚焦边框
#define COL_DIVIDER_SOFT   RGB(39, 39, 42)     // 柔和分割线
```

## 组件设计规范

### 轮盘界面重构
1. **背景**: 深邃渐变 + 噪点纹理
2. **轮盘主体**: 玻璃态 + 内发光 + 渐变边框
3. **扇区**: 渐变填充 + 悬停光效
4. **中心卡片**: 毛玻璃效果 + 动态阴影
5. **文字**: 清晰层级 + 悬停高亮

### 控制中心重构
1. **整体布局**: 流式卡片布局 + 适度留白
2. **导航栏**: 渐变背景 + 玻璃态效果
3. **卡片**: 圆角16px + 微妙阴影 + 悬停提升
4. **按钮**: 渐变背景 + 悬停光效 + 点击反馈
5. **列表**: 现代化卡片项 + 选中态渐变边框

## 动画和过渡
1. **轮盘显示**: 缩放 + 淡入 (0.3s ease-out)
2. **扇区悬停**: 渐变色流动 + 轻微放大
3. **按钮交互**: 背景渐变 + 阴影变化
4. **页面切换**: 滑动过渡 + 淡入淡出

## 字体系统
- **标题**: "Microsoft YaHei UI", Semibold, 24px
- **正文**: "Microsoft YaHei UI", Regular, 16px  
- **辅助**: "Microsoft YaHei UI", Regular, 14px
- **代码**: "Consolas", Regular, 14px

## 间距系统
- **xs**: 4px, **sm**: 8px, **md**: 16px, **lg**: 24px, **xl**: 32px
- **卡片间距**: 16px, **组件间距**: 12px, **文字间距**: 4px

## 圆角系统
- **小圆角**: 8px (按钮、标签)
- **中圆角**: 12px (输入框、小卡片)
- **大圆角**: 16px (主要卡片)
- **特大圆角**: 24px (轮盘、模态框)

## 阴影系统
```cpp
// 多层阴影创造深度感
#define SHADOW_SM   "0 1px 2px rgba(0,0,0,0.3)"
#define SHADOW_MD   "0 4px 6px rgba(0,0,0,0.4), 0 2px 4px rgba(0,0,0,0.3)"
#define SHADOW_LG   "0 10px 15px rgba(0,0,0,0.5), 0 4px 6px rgba(0,0,0,0.4)"
#define SHADOW_XL   "0 20px 25px rgba(0,0,0,0.6), 0 10px 10px rgba(0,0,0,0.5)"
#define GLOW_BRAND  "0 0 20px rgba(99, 102, 241, 0.5)"
```

## 光效系统
1. **内发光**: 轮盘边缘的柔和光晕
2. **外发光**: 选中项的环境光
3. **扫光效果**: 轮盘显示时的光扫过动画
4. **粒子效果**: 背景中微妙的浮动粒子

这套设计系统将让 ClipWheel 具备顶级工具的视觉质感，同时保持原有的所有功能特性。