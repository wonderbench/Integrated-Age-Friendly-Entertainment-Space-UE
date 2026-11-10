# 系统地图

以下为现有系统。公共入口详见 [接口表](INTERFACES_AND_EVENTS.md)，状态/事件详见 [所有权表](DATA_FLOW.md)；问题统一在 [TECH_DEBT](TECH_DEBT.md) 维护。

| 系统 / 用途 | Owner | 输入 → 输出 / 依赖 | 扩展位置 |
| --- | --- | --- | --- |
| 启动与装配 | BGM_GameMode、Showcase | 地图配置 → Controller/Pawn/GameState；关卡启动警报 | 地图配置，不往 GameMode 堆老人状态 |
| 玩家观察/交互 | OrbitCameraController、OrbitCameraCharacter | 玩家输入/UI → 观察视角、SelectedElder、接管 | 既有 Controller 接口；没有通用全场景交互框架的证据 |
| 老人实体/身份/状态 | ElderlyAICharacter / BP_AI | 测试/输入/AI命令 → 状态事件、移动限制、表现钩子 | 角色统一入口；独立能力再考虑组件 |
| AI 感知与寻路 | ElderlyAIController | Sight 刺激/MoveTo → 感知回调、寻路完成 | 当前 AIController；无项目 BT/Blackboard 资产 |
| 路线任务 | ElderNavigationComponent + ElderSplinePath | 每实例目标/曲线 → MoveTo、路线进度、完成事件 | 老人组件；场景路线独立可编辑 |
| 动画与视锥 | ElderAnimInstance、ABP_AI、角色组件 | 速度/根转身/Sight参数 → 骨骼与显示 | 复用原 ABP；待补姿势分支，不让显示承担检测 |
| 监控记录 | MonitorCameraActor / BP_Camera | Overlap/状态事件 → 本摄像机字符串历史 | 既有摄像机加入真实可见性与生命周期检查 |
| 摄像机扫描效果 | CameraScanVisualComponent | 材质参数 → 扫描 Mesh 表现 | 仅表现，不生成识别结果 |
| 警报展示 | BP_alarm | SOS → Light_Toggle；Light_Begin/Stop控制亮度、旋转及发光 | 多源告警/解除规则仍需定义，未接监控事件 |
| 天气时间/灯光 | BGS_State + UDS + BP_LightBase_Box | 两类选择按钮 → 天空过渡、标签接口分发、渐亮/渐灭 | 保持完整时间天气组合规则，复用接口 |
| UI/图表 | UProjectUserWidgetBase + 原 UBP + WebUI | Controller/GameState/JSON → 控件、HTML图表 | UI 只展示业务投影；真实统计来源另设计 |
| 场景/供应商资源 | MedaFurniturePack、UDS、OldManAnimset等 | 模型/材质/天空资源 → 场景表现 | 优先配置，不顺手改第三方内部逻辑 |

“健康状态、房间级告警聚合、存档、真实业务数据源”是后续需求，不在此表冒充已实现系统。
