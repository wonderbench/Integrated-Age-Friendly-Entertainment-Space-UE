# Integrated-Age-Friendly-Entertainment-Space-UE
嵌入式适老娱乐空间

UE5.8 社区适老空间交互仿真原型，包含三张演示地图、独立老人 AI、观察与接管、样条导航、天气灯光和界面。仓库整理了关键源码、资产及技术文档，完整运行还需要原工程的其余资源和插件。

| 目录 | 内容 |
| --- | --- |
| [Source](Source/) | C++ 运行时模块、自动化测试、构建目标 |
| [Config](Config/) | 项目默认配置，公开副本中的 SecurityToken 已清空 |
| [Content](Content/) | UE 核心蓝图、UI、三张地图和图表网页，保留原资源路径 |
| [docs](docs/) | 架构、状态、技术债和系统说明 |
| [docs/audit](docs/audit/) | 蓝图图表、三图读回、注册表、硬软引用、插件和历史验证证据 |

- [架构说明](ARCHITECTURE.md)
- [当前状态](docs/CURRENT_STATE.md)
- [技术债](docs/TECH_DEBT.md)
- [功能、风险与开发建议](docs/AUDIT_SUMMARY.md)
- [只读审计与文件清单](docs/audit/README.md)

蓝图及地图二进制文件使用 Git LFS。审计导出来自 UE5.8 读取的磁盘资产，构建、蓝图编译、PIE 和打包结果以独立验证证据表为准。
