# TundraGarden 场景合入主工程（2026-10-09）

来源为 `E:/Jiang_Work/Old_Man_Jiang_TundraGarden`，目标为本工程。2026-10-27当前成品入口已迁至 `/Game/Project_File/Maps/L_LivingGrove_Social`；原合入路径为 `/Game/TundraGarden/LivingGrove/Maps/L_LivingGrove_Social`，另有四个历史/实验地图仍属 `Content/TundraGarden`。这次按新增命名空间拷入全部 157 个 UE 关卡和资源文件，以及 `SourceArt/TundraGarden` 的 142 个制作源文件；复制的文件逐一通过 SHA-256 对照。五份原场景说明也已放到 `docs/`，其中旧文档提到的“独立目录”和 `Saved` 证据路径是制作时的历史记录，合入后的项目入口以本文为准。

主工程原有 `Content/TundraGarden` 与 `SourceArt/TundraGarden` 均不存在，因此未覆盖同路径资产。本轮复制命令只写入上述两个新目录及五份新增文档，另在备份后向原本未跟踪的项目指南追加本轮记录；没有对 `Old_Man_Jiang.uproject`、`Config`、`Plugins`、`Source`、原有 `ElderWellness` 资产或默认启动地图执行覆盖与保存。合入前主工程已有大量未提交工作，本轮没有对其全部文件建立逐项哈希基线，因此不把全工程逐文件不变当作已独立验证。此地图直接或间接复用现有 `ElderWellness` 和 `MedaFurniturePack` 资源，不另建同名副本；没有新业务状态 Owner、调用入口或老人/UI/天气逻辑。

主工程路径的 Asset Registry 扫描识别 157 个导入资产及 575 条资源引用，指向 `/Game` 的依赖均有对应文件，缺失数为 0；其中 41 个引用包位于 `TundraGarden` 之外。随后以主工程 `.uproject` 独立启动 `L_LivingGrove_Social` 游戏进程，读回 1706 个 Actor，确认接边顶棚、环线、喷泉绕行步道、12 个指示牌立柱与 6 把支撑椅均加载，运行校验为 PASS，并保存到达视角截图。证据在 `Saved/TundraMerge20261009/audit.json`、`game_validation.json`、`game.log`、`arrival.png`。主项目指南的合入前精确副本在 `Saved/CodexBackups/TundraMerge_20261009/`。

这些结果证明资产可识别、引用文件齐全，以及这张地图能在主工程中加载和出图。没有做主工程编辑器手工漫游、轮椅实际通行、老人/UMG/天气跨关卡回归、完整构建或打包验证。旧场景文档的制作期 PASS 不自动扩大为这些范围的验收。
