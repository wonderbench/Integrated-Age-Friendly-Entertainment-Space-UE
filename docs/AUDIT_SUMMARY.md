# UE5.8 项目只读审计摘要

2026-11-10。项目已具备三场景原型的主要交互路径，包括观察与老人接管、样条移动、天气/时段、警报灯和 UI。后续的发布验证应集中在三图跨关卡、打包依赖和关键交互。监控候选采集、跌倒状态、人工警报分别存在，自动告警仍缺判定、联动和解除规则。每项状态及证据限制见 [当前实现状态](CURRENT_STATE.md)。

| 业务闭环 | 可追溯的实现 | 待补证据或能力 |
| --- | --- | --- |
| 观察 → 选人 → 接管 → 归还 AI | Controller 拥有选中与控制，老人保留同一实例和状态，AI 暂停/恢复。见 OrbitCameraController_ElderControl.cpp:52–161、ElderlyAIController.cpp:85–116。 | 当前版本实测鼠标、键盘、接管显隐、失效对象和跨关卡。 |
| UI 路线 → 行走/轮椅 → 暂停 → 恢复 → 终点 | Widget 检查路线所有者和代次，导航组件维护路线进度。见 ProjectUserWidgetBase.cpp:358–448、ElderNavigationComponent.cpp:104–166。 | 三图通行净宽、姿态组合、恢复位置及打包导航网格。 |
| 跌倒 → 状态变化 → 本地监控历史 → 警报 | 状态变更和球形重叠监控记录有实现；手动文字警报有独立入口。见 ElderlyAICharacter.cpp:180–217、MonitorCameraActor.cpp:15–49、ProjectUserWidgetBase.cpp:310–329。 | 几何可见性、按事件去重、告警联动、确认解除与持久历史。起身衔接另验动作质量。 |
| 天气/时段 → 天空灯光与场景表现 | 当前 BGS_State 引脚证实天气分支、时段 Timeline 及标签广播 Light_Show。磁盘配置保留平行光阴影采样 64/8。见 [天气灯光系统](systems/WEATHER_LIGHTING_SYSTEM.md)、DefaultEngine.ini:13–14。 | 对照最新蓝图和三图关卡实例，并在当前版本验证组合和打包。 |
| 关卡切换 → 当地 UI/设备 → 数据呈现 | OpenLevel 的三条项目路径与三份磁盘地图对应；仪表盘设备/人数为演示值。见 ProjectUserWidgetBase.cpp:20–29、232–267。 | Cook、WebUI 离线、真实数据契约、跨关卡和档案保留。 |

架构上需维持当前状态分工：老人持有业务状态，AIController 持有感知，导航组件持有路线，玩家 Controller 持有选择与接管，Widget 读取/发请求。当前双向硬包依赖是 BP_AI ↔ ABP_AI_Laor 和主 UI ↔ 时间框。原生 UI 字符串控件名、警报反射契约及远程图表脚本也增加变更联动范围。监控使用具体老人类并无限积累文本，扩展至多人长时间运行时需先测量日志增长和委托清理。证据见 [架构](../ARCHITECTURE.md)、ProjectUserWidgetBase.cpp:168–193、30–35、MonitorCameraActor.cpp:23–49、Content/Echarts。

三图静态加载均读取到 BGM_GameMode、四台监控、一名老人和一条样条。Showcase、ElderWellness、LivingGrove 分别有 1/4/0 个 BP_alarm 和 115/40/8 个顶棚标签 Actor。当前主 Widget 导出未找到 SOS 图标到 Light_Toggle 的直接路径，图标绑定及点击尚待确认。文字警报入口由原生基类单独创建。证据见 [场景读回](audit/exports/scenes)、[蓝图索引](audit/exports/blueprint_index.json)。

Registry 有 11,587 条资产记录、11,412 个包及 26,901 条硬软包引用。201 条未解析边是排查候选，其中 26 条来自 /Game。三图纯硬闭包未发现 Registry 缺失候选；资料包分别省略 916/829/857 个 /Game 硬依赖，完整加载及 Cook 仍依赖原工程。证据见 [注册表汇总](audit/exports/registry_summary.json)、[各地图包内依赖缺口](audit/exports/map_bundle_dependency_gap.json)。
性能的优先测量点是 LivingGrove 的动态植被阴影。2026-11-02 固定机位历史短样本约从 11.73 ms 升至 17.43 ms，阴影颗粒减轻，仍有细闪。历史 Nanite 候选降低开销但阴影偏浅，已撤回。多老人 Sight 显示射线、轮椅组件数和历史字符串增长是源码风险，尚缺当前压力采样。证据见 [阴影稳定性](GROVE_SHADOW_STABILITY_20261102.md)、[采样矩阵](GROVE_SAMPLING_MATRIX_20261102.md)、ElderlyAICharacter.cpp:223–273、MonitorCameraActor.cpp:43–49。

1. **先建立发布验证基线。** 固定源码、资产、配置和插件版本，修复会阻断 Cook 的真实缺失硬依赖，分别记录构建、蓝图编译、自动 PIE、打包和独立程序实测。
2. **补监控到警报的链路。** 定义真正可见的老人、告警事件所有者、多源去重及解除行为；先覆盖一个摄像机和一个老人，再验证重叠、多关及历史留存。
3. **完成高频交互与姿态。** 在三个地图上验证接管/归还和路线回归，修正起身素材或重定向，补普通坐姿并量化轮椅通行。
4. **做三图目标设备性能对照。** 统一画质、相机和天气，分别量化阴影、植被、Sight 与历史记录的成本，再选择优化方案。
5. **接通统计与保存。** 先定义设备、活动及健康数据的来源、隐私边界和跨图保留规则，再替换演示计数、接离线图表和可追踪历史。

源文件名对应 Source/Old_Man_Jiang/Private 或 Public 下同名文件。历史结果以各记录的快照和范围为限；本次审计的实时读取、未执行项目与缺失证据见同包导出及验证清单。详细缺口见 [技术债](TECH_DEBT.md)。

