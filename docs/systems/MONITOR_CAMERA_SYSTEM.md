# 监控摄像机、历史与警报

2026-10-30：`BP_alarm` 两盏聚光灯的衰减半径从 1500 cm 调为 333.33 cm（原值除以 4.5），灯光角度、强度曲线和旋转逻辑不变。Showcase 现有实例与 PIE 读回相同半径；灯安装高度约 292 cm，缩小后地面红光会很有限。

2026-10-30：设备与告警标题新增文字按钮，通过现有 `Light_Begin/Light_Stop` 控制当前关卡警报实例：无开启灯时随机开启一盏，有开启灯时随机停止一盏；无可操作实例则禁用。它与 SOS 共享每盏灯的 `bAlarmActive`，不建立第二份警报状态；不构成远程求救或跌倒告警闭环。

2026-10-27：`L_ElderWellness_Main`与`L_TundraSanctuary`各复用四个BP_Camera实例，布置在室内四角并朝中心，初始隐藏`SM_Camera_WireFrame`；主UI第四项统一显隐。仍是原球形Overlap记录链，本次没有增加真实视锥遮挡检测、区域告警或跌倒红灯联动。

同日修复开场已在范围内的老人漏登记：BeginPlay先绑定重叠委托再设置半径，随后GetOverlappingActors并经原RegisterElder登记；MonitoredElders集合负责去重，离开时仍解绑。仅调整初始化，不增加Tick或第二套状态Owner。

现有 `BP_Camera → AMonitorCameraActor` 保留模型和 CameraScanVisualComponent；源码在 `MonitorCameraActor.cpp`、`CameraScanVisualComponent.cpp`。不创建第二套摄像机。

2026-10-27：主界面第四项在点击时切换各监控摄像机已有 `SM_Camera_WireFrame` 可见性，Showcase四台均经PIE显隐检查；当前资源为蓝色半透明实体效果。摄像机原 `CameraScanVisual.ScanMeshComponentName` 仍为旧名 `Cone_Brush_StaticMesh`，因此本按钮仅切换当前实际网格，不依赖该组件且不修改检测。

## CONFIRMED 当前检测链

MonitorRange 球体（原生默认半径1500cm，Pawn Overlap）→ Cast 老人 → RegisterElder → 集合去重、AddUniqueDynamic 绑定状态事件 → 读取接口名称/状态 → 更新摄像机显示和历史。EndOverlap → UnregisterElder/RemoveDynamic。无每帧状态查询。

**这不是摄像机真实 FOV/遮挡检测。** BP 图未补角度或 Trace 过滤；扫描线框/材质也只是显示。UMonitorableActorInterface 存在，但注册和成员集合仍具体依赖 AElderlyAICharacter，不应描述为完全接口解耦。

## 数据语义

- MonitoredElders 支持多实例；CurrentDetectedStateString 是最近一次更新老人，不是多行实时状态表。
- StateHistory 为每摄像机 String Array，StateHistoryText 每次全量拼接；使用系统时间，不是天空时钟；不持久保存。
- 注册时写“进入监控”初始记录；真正状态变化 Old!=New 才追加。离开后不再通过已解除的绑定记录，但当前显示字符串没有清空。
- 名字不是稳定 ID，重名老人仍可由集合区分，但文本历史不能可靠关联长期身份。
- 历史旧状态用 Enum DisplayName（当前英文），新状态中文；统一文本仍待办。

## 警报边界

`BP_alarm` 默认关闭；Showcase 已移除开场 Light_Begin 调用。每实例 bAlarmActive 是警报状态 Owner，UI 不保存第二份开关状态。

- `Light_Begin`：幂等启动，从首帧播放原亮度 Timeline，并保存旋转 Timer 句柄；已启动时不重启。
- `Light_Stop`：停止 Timeline、清除并失效旋转 Timer、两灯和灯体发光参数归零，状态 false。可重复调用。
- `Light_Toggle`：按本实例状态调用开始或停止。原 SOS 点击事件只在点击时查询 BP_alarm 并逐个 Toggle；场景为空时不执行。多个灯原先混合开关时各自反转，不是统一全关。
- 2026-09-04历史表现调整：Timer仍0.01秒，Yaw步长5→0.6，即约60°/秒、6秒一圈。两灯纯红、向下35°，内/外锥20°/35°、当时范围1500cm（现已按本页顶部记录调整为约333.33cm）、Candelas单位，沿用原0～10000亮度曲线。Cylinder复用网格，材质改M_AlarmBeacon，AlarmIntensity与原曲线同步，红色自发光倍率0.003；停止清零。Showcase原实例覆盖已同步，不改其位置/缩放。

此为本地场景警报，不是求救呼叫服务；未接 Falling/Fallen 或多摄像机告警闭环。运行证据见 `Saved/Tests/AlarmVisual_Verified.json`：默认关、SOS委托开关、关闭后延时静止/两灯0、外部开始/重复开始/停止、发光清零。最终版不是物理鼠标验收，未测试多实例混合态或打包。截图 `Saved/Screenshots/AlarmVisual_FinalOn.png` / `AlarmVisual_FinalOff.png`。

## 扩展方向（Planned，未执行）

本轮独立终审：PASS（SOS本地开关、减速红光、悬停反馈与上述证据边界）；最终Showcase、PIE停止、无脏包。此结论不扩大为完整监控告警系统验收。

在当前摄像机加候选/可见集合与视锥/LOS 验证，再按可见性绑定状态；处理启动已在范围、多个碰撞组件、目标销毁、摄像机 EndPlay、最后目标离场的显示语义。历史增加稳定 ID/结构化记录及容量策略时保留兼容展示。告警需先定义多摄像机去重、离场是否解除、人工确认规则，再接 BP_alarm。

验证至少包括前/后/边缘/墙后、入场/离场/重复入场、两老人同名、两个摄像机、同态不重复、跌倒两阶段、对象销毁和长时记录；当前测试通过情况见 [TESTING](../TESTING.md)，不能用老人自身 Sight 测试代替监控测试。
