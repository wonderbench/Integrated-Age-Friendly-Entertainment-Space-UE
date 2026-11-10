# 状态所有权与数据流

## 当前 State Ownership（CONFIRMED）

| 状态 | 权威 Owner | 更新入口 / 通知 | 消费者与限制 |
| --- | --- | --- | --- |
| ElderName、Gender | 每个 AElderlyAICharacter | 生成/实例配置；切性别用 SetElderGender | 监控/UI；直接写 Gender 不会执行换 Mesh，改名暂无专门事件 |
| CurrentState、PreviousState | 每个 AElderlyAICharacter | SetElderState；OnElderStateChanged | 动画、导航、摄像机；同态不广播 |
| SelectedElder | AOrbitCameraController | SetSelectedElder；OnSelectedElderChanged | UI；不等于当前 Possess 对象 |
| 接管对象/观察 Pawn | AOrbitCameraController 与实际 Possession | TakeControlOfElder / ReleaseElderControl | 输入路由与 UI 提示；不复制老人数据 |
| 路线配置与进度 | 每老人 ElderNavigationComponent | NavigateToActor / FollowSpline / Pause / Resume / Stop；OnNavigationFinished | AIController 执行请求；运行中改配置字段不自动重发路径 |
| 真实 Sight 配置 | AElderlyAIController.SightConfig | OnPossess 从老人读初值；RefreshSightConfig 生效 | AI Perception；角色 Sight 字段与显示不是自动同步的第二权威 |
| 视锥显示/大小 | 老人视觉组件 | ToggleSightCone / SetSightConeVisible / RefreshSightCone | 仅渲染投影，不能作为看见目标的依据 |
| 监控成员/历史 | 每个 AMonitorCameraActor | Overlap 注册解绑；老人事件追加 | CurrentDetectedStateString 是最近一次更新，不是全体老人状态表 |
| 天气/时段选择 | BGS_State.ST_Weather / ST_Time | 现有天气/时间 UI 写值后 A_CallLightShow | UDS、灯光；ST_Weather_Temp 为遗留兼容数据，不作为灯规则权威 |
| 天空过渡时间 | BGS_State 的目标值 + UDS 当前表现 | 天空 Timeline | 不等于 UI 日历时钟，也不应反向覆盖已选择的时段 |
| 单灯目标/当前亮度 | BP_LightBase_Box | Light_Show 计算目标；Timeline 驱动 SpotLight | IsOpenLighting? 是目标布尔；亮度是过渡中间值，二者不能硬比较相等 |
| 警报启停 | 每实例 BP_alarm.bAlarmActive | Light_Begin / Light_Stop / Light_Toggle | SOS只转发Toggle；两灯和灯体材质是显示投影，默认关闭 |
| UI 展开/区域高亮 | 对应 Widget | 本地事件/动画 | 合法 UI 状态，不得混为业务 Authority |
| 图表数据 | 当前 HTML 默认数组/调用方 JSON | ue2js | 真实统计权威来源 UNKNOWN，不从控件反推业务状态 |

## 更新链

老人命令 → `SetElderState` 去重 → Previous/Current → 移动限制/轮椅/表现钩子 → Dispatcher → 已注册摄像机追加历史。Falling 按 FallAnimation 时长设 Timer 转为 Fallen；GettingUp 按 GetUpAnimation 时长设 Timer 转为 Standing；两者都不是实际播放完成回调。

UI 明确选人 → Controller.SelectedElder → 请求接管同一老人 → 暂停 AI/路线 → 玩家输入与既有动画 → 退出恢复观察 Pawn/AI。切换下拉选择不能偷换已经接管的对象。

天气或时段按钮 → 更新 GameState 的相应字段 → `A_CallLightShow` 立即调用 `B_Call_LightShow` → 查 `BP_Light` 标签 → `BPI_Actor_Call.Light_Show` 传完整当前组合 → 单灯目标去重 → Timeline Play/Reverse。天空自己的渐变不阻塞灯规则计算。

“唯一权威”不禁止缓存/投影/历史；必须定义来源、更新事件、生命周期和过期显示。当前监控离场后字符串不清空属于待处理展示语义，不应让摄像机写回老人状态来修复。
