# 项目知识库

入口：[AGENTS](../AGENTS.md) → [架构](../ARCHITECTURE.md) → [当前状态](CURRENT_STATE.md) → 对应系统。维护日期：2026-09-03。

## 文档分工

| 文档 | 唯一重点 |
| --- | --- |
| [PROJECT_OVERVIEW](PROJECT_OVERVIEW.md) | 用途、运行入口、工程组成 |
| [CURRENT_STATE](CURRENT_STATE.md) | 完成程度与缺口 |
| [SYSTEM_MAP](SYSTEM_MAP.md) | 系统 Owner、输入输出、扩展位置 |
| [BLUEPRINT_RESPONSIBILITIES](BLUEPRINT_RESPONSIBILITIES.md) | 32 个核心蓝图/接口的职责 |
| [DATA_FLOW](DATA_FLOW.md) | 状态权威与更新链 |
| [INTERFACES_AND_EVENTS](INTERFACES_AND_EVENTS.md) | 当前可用公共入口 |
| [ASSET_MAP](ASSET_MAP.md) | 资产、地图、数据与依赖定位 |
| [NAMING_CONVENTIONS](NAMING_CONVENTIONS.md) | 保留现有命名，新增规则 |
| [CODING_RULES](CODING_RULES.md) | 实施与维护约束 |
| [TESTING](TESTING.md) | 验证矩阵与历史证据 |
| [TECH_DEBT](TECH_DEBT.md) | 分级风险和未完成项 |
| [ROADMAP](ROADMAP.md) | 已完成与待批准迁移 |
| [PROJECT_AUDIT_REPORT](PROJECT_AUDIT_REPORT.md) | 本轮结论、覆盖范围、限制 |

分系统：[玩家相机](systems/PLAYER_CAMERA_SYSTEM.md)、[老人 AI](systems/ELDER_AI_SYSTEM.md)、[监控](systems/MONITOR_CAMERA_SYSTEM.md)、[天气灯光](systems/WEATHER_LIGHTING_SYSTEM.md)、[UI](systems/UI_SYSTEM.md)。

决策：[混合架构](decisions/ADR-001-hybrid-ownership.md)、[状态与视觉分离](decisions/ADR-002-state-and-sight.md)、[保留灯光接口](decisions/ADR-003-weather-light-interface.md)、[根转身播放契约](decisions/ADR-004-root-turn-playback.md)。

## 证据标识

- **CONFIRMED**：当前源码、资产元数据、已读取的图/连线或配置确有此内容；不自动代表运行验收。
- **INFERRED**：根据结构推断的风险或建议，不作为已复现故障。
- **UNKNOWN / NEEDS MANUAL CHECK**：未核实或需专项/人工运行验证。
- **历史测试**：有保存日志的旧运行结果，仅对应当时版本与测试范围。

当前结构文档与重要变更日志分开维护。日志保留在 [Content 指南](../Content/CODEX_PROJECT_GUIDE.md)；机器可读核心快照在 [audit/core-assets-2026-09-03.json](audit/core-assets-2026-09-03.json)。不要把所有资产节点和测试日志复制进 Markdown。
