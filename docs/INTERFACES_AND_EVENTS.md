# 当前接口与事件


2026-10-29：C 键跌倒请求与调试跌倒按钮由老人 `bEnableFallTrigger` 控制，默认关闭；直接调用老人状态函数仍可使用，旧 C 键默认触发描述为历史行为。
2026-09-11更新：`AOrbitCameraController.RequestElderFall()` 为C对应的语义入口，优先当前接管对象，否则SelectedElder，Standing/Walking以外拒绝。Enhanced Input Started调用时额外检查游戏焦点/Alt UI。`TargetNav_2`现支持Fallen调用StartGettingUp；UI监听OnElderStateChanged投影ElderActionMode和可用性，Falling/GettingUp期间禁止重复点击，结束后恢复轮椅操作。红灯联动由用户暂缓。

## TargetNav_1 / 2 / 5（2026-09-10）

`UBP_用户界面.TargetNav_1.OnClicked` 进入 `SelectTargetNavigation(0)`：未接管时调用 Controller 的 `TakeControlOfSelectedElder`，已接管时调用 `ReleaseElderControl`，并按实际控制结果刷新高亮。`TargetNav_2` 进入 `SelectTargetNavigation(1)`：Standing/Walking调用`EnterWheelchair`，WheelchairIdle/Moving调用`ExitWheelchair`，Fallen调用`StartGettingUp`，过渡状态拒绝且不覆盖状态。`TargetNav_5` 进入 `SelectTargetNavigation(4)` 并调用既有 `ToggleSelectedElderSightCone()`；高亮按实际组件可见性投影。Controller/老人仍分别拥有选择、控制和状态；Widget 不创建权威状态副本。

原生声明位置：`Source/Old_Man_Jiang/Public`。下列是现存名称，不是要求新建的 Blueprint 列表。

| 拥有者 | 入口 / 事件 | 契约 |
| --- | --- | --- |
| AElderlyAICharacter | SetElderState(NewState) | 八状态唯一修改入口，同态直接返回 |
| 同上 | SetStanding、SetSitting、StartWalking、StartFalling、SetFallen、StartGettingUp、EnterWheelchair、ExitWheelchair、SetWheelchairMoving(bool) | 全部转调 SetElderState；StartGettingUp仅允许Fallen且必须有GetUpAnimation；StartWalking设状态，不等于自动选目标寻路 |
| 同上 | OnElderStateChanged(Actor, Name, PreviousState, NewState) | 原生动态多播；每实例数据，无时间参数 |
| 同上 | SetElderGender、SetSightConeVisible、ToggleSightCone | 外观/显示入口；不改变老人身份或控制关系 |
| 同上 | ApplyElderStateVisuals(NewState, OldState) | BlueprintImplementableEvent；当前 BP_AI 未实现，不能假设自动播放槽位动画 |
| 同上 | MoveElder、StopElderMovement、SetDesiredMoveSpeed | 控制移动；轻按松键与强制 Stop 的语义不同，复用已有 Controller 输入链 |
| UMonitorableActorInterface | GetMonitorName、GetMonitorState、GetMonitorStateString | 原生 BlueprintNativeEvent；老人实现；BP_Camera 的注册/订阅仍是具体老人类型 |
| AMonitorCameraActor | RegisterElder / UnregisterElder | 当前参数为 AElderlyAICharacter；AddUniqueDynamic 去重，不是通用 Actor 接口注册 |
| AOrbitCameraController | SetSelectedElder、OnSelectedElderChanged | 显式选中；界面绑定/解绑 |
| 同上 | TakeControlOfSelectedElder、TakeControlOfElder、ReleaseElderControl、IsControllingElder、GetAvailableElders | 本地控制切换；列表查询不允许改成每帧全场景扫描 |
| UProjectUserWidgetBase | GetProjectController、GetSelectedElder、ToggleSelectedElderSightCone、TakeControlOfSelectedElder、ReleaseElderControl | UI 统一转发入口；布局仍在原 Widget |
| 同上 | OnProjectWidgetReady、OnSelectedElderChanged | 蓝图表现钩子；父类 NativeConstruct/Destruct 管理选择事件绑定 |
| 同上 | SelectTargetLevel(Index)、SelectTargetNavigation(Index)、OnTargetLevelSelected(Index)、OnTargetNavigationSelected(Index) | 分别0–2/0–4，需对应bEnableTargetLevelSelection/bEnableTargetBottomNavigation启用。Level及尚未接入的Nav2/3仅管理展示和报告；Nav0切换当前选中老人接管，Nav1切换Standing/轮椅，Nav4切换当前选中老人SightCone；Widget仍不拥有这些业务状态 |
| UElderNavigationComponent | StartConfiguredNavigation、NavigateToActor、FollowSpline、PauseNavigation、ResumeNavigation、StopNavigation | 每实例路线配置和请求；不可达失败，不假装完成部分路径 |
| 同上 | OnNavigationFinished(bool bSuccess, FString Message) | 任务结束结果，不是每帧进度广播 |
| AElderlyAIController | RefreshSightConfig；感知更新蓝图钩子 | 真实感知来自 Perception，不从线框 Overlap 生成 |
| BGS_State | A_CallLightShow、B_Call_LightShow | 现有自定义事件，负责天空/标签接口分发 |
| BPI_Actor_Call | Light_Show(time, weather_temp, weather) | 仍在使用的蓝图接口；保留参数兼容，灯规则不再依赖 weather_temp |
| BP_LightBase_Box | Light_Show1 | 保留的手动灯光入口；清理前查调用者 |
| BP_alarm | Light_Begin / Light_Stop / Light_Toggle | 开始/停止幂等；Toggle反转本实例bAlarmActive。SOS逐实例Toggle；停止同时清Timer、Timeline、两灯及发光。不是多来源告警确认系统 |
| BPI_用户界面 | 点击缩放 | UBP_缩放 → 接收界面，驱动界面动画；不是老人接口 |
| WebUI HTML | ue.interface.ue2js(JSON) | 三个已接入图表文件接收数据；当前有固定 Delay 等待，不是加载就绪握手 |

修改签名必须同时检查 C++ 引用、蓝图调用、地图 LevelScript、WidgetTree 和资产依赖；只用 Asset Registry 缩窄后的“零调用”搜索不能证明无人使用。
