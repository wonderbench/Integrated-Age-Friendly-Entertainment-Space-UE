# 技术债与功能缺口

2026-11-10 只读审计。优先级按影响排序，状态按证据划分。源码缺口、运行复现、历史验证和设计风险各自记录。

| 优先级 / 编号 | 状态 | 证据和后续验收 |
| --- | --- | --- |
| P0 / R01 版本验收 | **尚未验证** | 发布前将完整构建、蓝图编译、自动 PIE、Cook/Package、打包物实测绑定到同一源码与资产快照。禁止并行测试加载及保存目标资产。约束见 AGENTS.md 和 [测试说明](TESTING.md)。 |
| P1 / M01 监控几何 | **仍未完成** | MonitorCameraActor.cpp:7–27 用球形 Pawn 重叠产生候选，无相机 FOV/LOS。CameraScanVisualComponent.cpp:12–65 独立管理显示。分离候选和可见集合，在同一可配置角度下测试正前方、背后、遮挡、边界与跨帧抖动。 |
| P1 / M02 跌倒告警 | **仍未完成** | MonitorCameraActor.cpp:39–49 只记状态；ProjectUserWidgetBase.cpp:310–329 实现手动逐实例开关。定义按事件、人员与分区去重、解除、确认和过期规则，核对文字/SOS两入口及多摄像机重叠。 |
| P1 / E01 姿态表现 | **部分完成** | 原“跌倒只给计时”的债已被运行姿态分支部分消化。ElderAnimInstance.cpp:30–49 已提供跌倒末帧、起身显式时钟和轮椅投影；普通 Sitting、起身源动作末段异常、接触 IK、正式外观仍待完成。先复核骨骼重定向、地面接触与退出姿态。见 [老人系统](systems/ELDER_AI_SYSTEM.md)。 |
| P1 / D01 双向包依赖 | **仍未完成** | 本轮确认 BP_AI ↔ ABP_AI_Laor、主 UI ↔ 时间框均为双向硬包引用。原 ABP_AI → BP_AI 有硬引用，反向硬引用现指向 Laor 动画图。双向包引用增加迁移和加载耦合；执行时序需单独验证。见 [引用明细](audit/exports/references.csv)。 |
| P1 / U04 资产包交付 | **尚未验证** | 三图通过 C++ 字符串调用 OpenLevel（ProjectUserWidgetBase.cpp:22–23、232–240）；需验证三图及 WebUI 文件进入 Cook/Stage、插件对 UE5.8 可用、干净机器可启动。完整审计资料仍需补运行依赖：三图纯硬引用闭包中 Registry 未列为缺失，但 ZIP 分别省略 Showcase 916、ElderWellness 829、LivingGrove 857 个 /Game 硬依赖目标。见 [各地图包内依赖缺口](audit/exports/map_bundle_dependency_gap.json)。 |
| P2 / M03 监控对象扩展 | **部分完成** | MonitorCameraActor.cpp:23–41 的集合及回调仍绑定 AElderlyAICharacter；接口只查询名称与状态。只有扩展监控类型时再抽象订阅契约。 |
| P2 / M04 重叠生命周期 | **已修复（初始登记）**，**部分完成（离场）** | MonitorCameraActor.cpp:15–24 已扫描起始重叠；:26–38 仍按 Actor 整体移除且未更新 CurrentDetectedStateString。复测初始重叠、多人并发、多个组件与销毁，并定义无对象及剩余对象的显示语义。 |
| P2 / M05 / M06 监控历史 | **仍未完成** | MonitorCameraActor.cpp:43–49 使用无界字符串数组并每次 Join 整段；未见稳定 ID、分批输出、归档策略。初始进入以专用文本记录，前一状态用枚举 DisplayName，后一状态用接口文本。先定结构、文本映射、保留上限与归档。 |
| P2 / E02 Sight 参数同步 | **部分完成** | ElderlyAIController.cpp:23–43 在 Possess 同步感知参数，有独立 RefreshSightConfig；角色端显示另由 ElderlyAICharacter.cpp:223–273 实现。验收显示与真实感知参数职责，明确运行时更新路径。 |
| P2 / E03 轮椅及导航成本 | **尚未验证** | 本轮 BP_WheelChair 导出 225 条组件记录，历史记录曾为 219 条；组件计数不代表运行实例成本。见 [蓝图索引](audit/exports/blueprint_index.json)。用当前依赖/蓝图记录和多老人压力测试量化组件、碰撞与移动负担，验证通道净宽和急弯。见 [蓝图职责](BLUEPRINT_RESPONSIBILITIES.md)。 |
| P2 / U01 / U03 图表及旧引用 | **部分完成** | Content/Echarts/line-stack.html 已本地 Canvas，其他图表仍有 CDN；历史报告有旧 UI 包路径。先用本轮依赖类型和重定向核对，再做离线/打包与就绪握手测试。避免把包缺口和软引用候选直接归为运行错误。 |
| P2 / U02 / W02 时钟与状态 | **尚未验证** | 旧记录的系统时钟、固定天气字样、模拟时间和 UDS 时刻分属不同概念。确定显示来源、跨图初始化和保存策略。见 [天气灯光系统](systems/WEATHER_LIGHTING_SYSTEM.md)、[UI 系统](systems/UI_SYSTEM.md)。 |
| P2 / W01 天气写入契约 | **部分完成** | 历史 UI 直接写 BGS 字段，灯具保留 Light_Show 和 Timeline。当前 BGS_State 图确认时段 Timeline → UDS Set Time Of Day，天气切换分支 → B Call Light Show；Event Tick 的执行输出未连接。先验三个地图组合，再在现有 Owner 统一校验入口。见 [BGS_State 图](audit/exports/blueprints/Game/Project_File/BP/System/BGS_State.json)。见 [天气灯光系统](systems/WEATHER_LIGHTING_SYSTEM.md)。 |
| P2 / P01 阴影性能 | **部分完成** | DefaultEngine.ini:13–14 为全局 64/8；历史局部短样本约 11.73→17.43 ms，仍有细闪。后续固定相机、天气、画质、分辨率和硬件做三图 GPU 分项与画质矩阵。见 [阴影稳定性](GROVE_SHADOW_STABILITY_20261102.md)、[采样矩阵](GROVE_SAMPLING_MATRIX_20261102.md)。 |
| P2 / P02 周期刷新 | **尚未验证影响** | ElderlyAICharacter.cpp:232、253–267 在显示时约每 33ms 做一条起点裁切和最多 33 条显示射线；ProjectUserWidgetBase.cpp:268–275、342–356 初次遍历后只处理警报集合，每 0.25s 刷新。先做多老人、多摄像机的 CPU Trace，再决定局部刷新或事件化。 |
| P3 / C01 历史遗留 | **尚未验证** | 时间框旧节点、旧控件名和拼写、重定向、警报旋转 Timer 等仍是候选。以当前图、调用点、重定向解析及行为验收决定清理。见 [蓝图职责](BLUEPRINT_RESPONSIBILITIES.md)、[路线图](ROADMAP.md)。 |

全量 Registry 导出含 24,033 条硬、2,868 条软包引用，201 条未解析候选中有 26 条源自 /Game。候选按磁盘、重定向、挂载和实际加载逐项确认。三图纯硬闭包和全 Registry 候选的统计范围不同。见 [注册表汇总](audit/exports/registry_summary.json)、[未解析候选](audit/exports/unresolved.csv)。

源文件名对应 Source/Old_Man_Jiang/Private 或 Public 下同名文件。详细状态见 [当前实现状态](CURRENT_STATE.md)。

