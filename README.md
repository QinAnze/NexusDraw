<p align="center">
  <img src="resources/logo.png" width="128" alt="NexusDraw">
</p>

<h1 align="center">NexusDraw</h1>

<p align="center">AI 驱动的数据科学可视化平台</p>

<p align="center">
  <img src="https://img.shields.io/badge/platform-Windows%20x64-lightgrey">
  <img src="https://img.shields.io/badge/Qt-6.11-green">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue">
  <img src="https://img.shields.io/badge/Python-3.13-yellow">
  <img src="https://img.shields.io/badge/R-4.6.1-blueviolet">
  <img src="https://img.shields.io/badge/license-MIT-brightgreen">
</p>

---

## 简介

NexusDraw 是一个基于 Qt6/C++ 的桌面数据可视化工具。连接任意 OpenAI 兼容的 LLM API，用自然语言描述需求，AI 自动生成 Python 或 R 代码并执行，图表即时呈现。

**开箱即用** — 内置完整的 Python 3.13 和 R 4.6.1 便携环境，无需手动安装任何依赖。（仅Releases内打包的内容）

## 工作流

```
上传 CSV → 选择 Python/R → 选择配色方案 → 描述需求 → AI 生成代码 → 自动执行 → 图表呈现 → 导出
```

## 功能

- 支持任意 OpenAI 兼容 API
- 三种模式：科学绘图（Python/R）| 流程图（SVG）| 对话
- 21 种科学配色方案，色号标准化，跨工具通用
- 图表交互：滚轮缩放、拖拽平移、双击新窗口
- Markdown 文件上传和渲染
- 代码语法高亮（Python/R/SVG）
- 代码报错 AI 自动修复（最多 3 次）
- 收藏夹：批量导出，支持 PNG/JPEG/BMP/SVG
- 内置终端，系统命令
- 导出：PNG / JPEG / BMP / SVG（矢量）

## 预装库

| Python | 版本 | R | 版本 |
|---|---|---|---|
| numpy | 2.4 | ggplot2 | 4.0 |
| pandas | 3.0 | dplyr | 1.2 |
| matplotlib | 3.11 | tidyr | 1.3 |
| seaborn | 0.13 | ggpubr | 1.0 |
| scipy | 1.18 | patchwork | 1.3 |
| scikit-learn | 1.9 | ggrepel | 0.9 |
| statsmodels | 0.14 | ComplexHeatmap | 2.28 |
| scanpy | 1.12 | Seurat | 5.5 |
| plotly | 6.9 | | |
| networkx | 3.6 | | |

## 快速开始

1. 下载 `Setup.zip`
2. 解压后双击 `Setup/NexusDraw-Setup.bat` 启动安装向导
3. 选择安装位置，确认创建快捷方式
4. 运行 NexusDraw，`设置 → AI 配置` 填入 Base URL / API Key / Model
5. 上传数据，选择语言和配色，开始绘图

卸载：运行安装目录下的 `NexusDraw-Uninstall.bat` 或开始菜单中的卸载快捷方式

## 项目结构

```
├── src/core/          # AI通信、代码执行、配置管理
├── src/ui/            # 主窗口、面板、对话框
├── resources/         # 样式、图标、Skills、配色方案
├── portable_python/   # 便携 Python 3.13
├── portable_r/        # 便携 R 4.6.1
└── build/             # 编译输出
```

## 构建

注意：Python与R的library未上传，需手动安装配置。

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<Qt6路径> -G "MinGW Makefiles"
cmake --build build --config Release
```

## License

MIT
