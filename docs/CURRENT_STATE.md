# 当前实现状态

审计日期：2026-11-10。本文为独立资料包中的审计修订版。**已修复**表示本轮静态证据确认旧缺口的对应实现已补齐，**部分完成**表示已有功能链但仍有覆盖或交付缺口，**仍未完成**表示所列业务能力在核查范围内没有完整实现，**尚未验证**表示缺少匹配当前版本的实测证据。源码、资产结构、历史运行和当前运行分别记录。

| 功能 | 状态与事实 | 来源与验证边界 |
| --- | --- | --- |
| 三图入口及切换 | **已修复**。Widget 原生基类已包含三图按钮绑定、当前关高亮与按包路径调用 OpenLevel；默认地图为 Showcase。三图为 Showcase、L_ElderWellness_Main、L_LivingGrove_Social。 | ProjectUserWidgetBase.cpp:20–29、172–194、232–251；DefaultEngine.ini:66–67；Content/Project_File/Maps 三份磁盘关卡。2026-10-28 有三图截图和 PIE 记录（Saved/CodexBackups/LevelDashboard_20261028），打包后的地图纳入和切换尚未验证。 |
| 老人身份及状态 | **部分完成**。每实例老人是状态所有者，C++ 以 SetElderState 发事件并维护八个状态；玩家选中、接管和路线进度各自独立。 | ElderTypes.h、ElderlyAICharacter.cpp:172–222、OrbitCameraController_ElderControl.cpp:52–161。普通坐姿、独立男女外观还待接入；当前 BP_AI 父类为 ElderlyAICharacter，关卡各一实例；当前硬引用指向 ABP_AI_Laor，原 ABP_AI 仍存在。 |
| 跌倒、倒地与起身 | **部分完成**。Fall/GetUp 以动画长度计时收尾。起身保留跌倒末帧为下层姿态，起身淡出保留时钟且下一次激活归零；用户触发开关默认关闭。 | ElderlyAICharacter.cpp:180–217、ElderlyAICharacter.h:120、ElderAnimInstance.cpp:30–49、OrbitCameraController_ElderControl.cpp:198–205。2026-11-05 构建与 Showcase PIE 变量读回为历史，骨骼采样有穿地/末段姿势疑点，视觉衔接仍未验收。见 [老人系统](systems/ELDER_AI_SYSTEM.md)。 |
| 步行、根转身、轮椅 | **部分完成**。原生转身跟踪现有 BlendSpacePlayer；轮椅静止/移动有动画投影及车轮运动组件。普通坐姿不是轮椅坐姿，人体接触、手脚 IK、轮椅宽度与家具避碰未闭环。 | ElderAnimInstance.cpp:22–30、50–72 及 ReportTurnAfterAnimationUpdate；ElderlyAICharacter.cpp、WheelchairWheelMotionComponent.cpp；动画资源绑定与局部历史 PIE 见 [老人系统](systems/ELDER_AI_SYSTEM.md)。 |
| 玩家观察、接管/归还 | **部分完成**。玩家控制器接管同一老人；AI 暂停感知和移动，归还时恢复原控制器并通知路线组件。观察相机边界、顶棚显隐有历史三图证据。 | OrbitCameraController_ElderControl.cpp:52–161、ElderlyAIController.cpp:85–116；Saved/CodexBackups/MultiMapControl_20261028。真实鼠标键盘、全部遮挡和打包场景行为待当前版本复测。 |
| 路线按钮、样条导航 | **已修复（入口）**。导航组件维护代次、暂停、恢复及后续采样；原生界面重复点高亮路线或切其他底部按钮会暂停由该界面启动且代次仍匹配的路线，重选可恢复。**部分完成（通行）**。 | ElderNavigationComponent.cpp:30–46、104–166；ProjectUserWidgetBase.cpp:358–448；2026-10-28 三关及 10-30 切换记录见 Saved/CodexBackups/MultiMapControl_20261028 和 RouteToggle_20261030。全场景、不同姿态、家具、角落与装包 NavMesh 仍待验证。 |
| 老人 Sight | **部分完成**。实际感知由 AIController 的 AI Perception Sight 维护；显示由角色从头部起点进行遮挡裁切，显示距离默认 600cm，受真实半径上限约束。 | ElderlyAIController.cpp:10–43；ElderlyAICharacter.h:110–118、ElderlyAICharacter.cpp:223–273；运行中修改角色参数不会自动同步至感知，感知通知有入口，复杂行为仍需设计。 |
| 摄像机登记与历史 | **已修复（初始重叠扫描）**。BeginPlay 绑定球形重叠后主动登记现有老人；订阅状态变更并追加本地历史。**部分完成（监控）**，检测仍无摄像机视角、朝向、遮挡判定。 | MonitorCameraActor.cpp:7–49；CameraScanVisualComponent.cpp:12–65。扫描 Mesh 用于显示并关闭碰撞；已重叠场景、离开/销毁、多组件及多老人时序仍需当前 PIE 验证。 |
| 监控范围显示 | **已修复（显示入口）**。UI 第四项统一显隐摄像机已有的 SM_Camera_WireFrame，并按操作后的显示结果刷新高亮。 | ProjectUserWidgetBase.cpp:452–475；Saved/CodexBackups/MonitorToggle_20261030/final_runtime.txt 记录旧 Showcase 四组件显隐。三图实际组件匹配和当前点击待验证。 |
| 手动警报与跌倒闭环 | **部分完成**。主界面文字按钮收集满足反射契约的警报实例，无活动灯时随机开启一个，有活动灯时随机停止一个；监控未订阅到自动警报策略。 | ProjectUserWidgetBase.cpp:30–35、268–275、278–356；MonitorCameraActor.cpp:39–49。历史证据见 Saved/CodexBackups/AlarmButton_20261030。当前 UBP_用户界面 的 ChangeColor 和 EventGraph 未发现 SOS 或 Light_Toggle 的直接调用链，可能在子控件；本轮未确认图标绑定及运行。静态关卡警报数为 1/4/0，零实例场景需核对禁用显示。 |
| 天气、时段及灯光 | **部分完成**。架构保持 BGS_State → 天空和 Light_Show 接口 → 关卡灯 Timeline。 | [天气灯光系统](systems/WEATHER_LIGHTING_SYSTEM.md)、[蓝图职责](BLUEPRINT_RESPONSIBILITIES.md)。历史 18 组合、47 灯只适用于当时范围；本轮 BGS_State 图上可沿引脚验证天气切换、时段 Timeline 及 B_Call_LightShow → 标签取 Actor → 循环 → Light Show；当前多图和打包回归待执行。 |
| UI 图表、设备统计与持久化 | **仍未完成**。仪表盘人数和设备数量是演示常量；有监控本地历史和图表入口，当前自有源码未发现 SaveGame 或真实业务数据汇总的实现。 | ProjectUserWidgetBase.cpp:242–267、MonitorCameraActor.cpp:43–49、Content/Echarts；旧图表有 CDN，能耗 line-stack 使用本地 Canvas 和 ue2js。蓝图/插件的其他数据源未凭原生源码排除。 |
| LivingGrove 阴影 | **部分完成**。磁盘配置保留 VSM SMRT 平行光 64/8。历史对照说明降噪伴随明显 GPU 成本，局部仍有细闪。 | DefaultEngine.ini:13–14；[阴影稳定性](GROVE_SHADOW_STABILITY_20261102.md)、[采样矩阵](GROVE_SAMPLING_MATRIX_20261102.md)。没有当前多图、目标设备或打包性能验收。 |
| 导视资源与历史场景 | **部分完成**。历史交付记录有六种吊挂导视模型及对应微光蓝图；另一个 TundraSanctuary 不属于上述三图按钮映射。 | [导视模型](WAYFINDING_SIGNS_20261027.md)、[微光蓝图](WAYFINDING_GLOW_BLUEPRINTS_20261027.md)、ProjectUserWidgetBase.cpp:22–23。关卡是否实际放置及光照驱动接线以当前资产导出为准。 |

## 三张关卡的当前磁盘读回

| 地图 | 加载 Actor | BP_AI | BP_ElderSplinePath | BP_Camera | BP_alarm | ElderControlCeiling 标签 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Showcase | 451 | 1 | 1 | 4 | 1 | 115 |
| L_ElderWellness_Main | 1,110 | 1 | 1 | 4 | 4 | 40 |
| L_LivingGrove_Social | 1,135 | 1 | 1 | 4 | 0 | 8 |

三图 DefaultGameMode 均指向 BGM_GameMode。Actor 数来自逐关静态加载，范围是当次可见于编辑器读取的 Actor。分类数依据具体生成类，不含运行时生成和其他可能的实现；顶棚数依据标签，尚未验证运行中显隐。参见 [Showcase](audit/exports/scenes/Showcase.json)、[ElderWellness](audit/exports/scenes/L_ElderWellness_Main.json)、[LivingGrove](audit/exports/scenes/L_LivingGrove_Social.json)。

当前结构证据含六个优先 Blueprint、ABP_AI_Laor 及相关设备和系统。父类与图、节点和引脚数量见 [蓝图索引](audit/exports/blueprint_index.json)。引脚连线记录用于核对静态路径，运行验证另计。
源文件名对应 Source/Old_Man_Jiang/Private 或 Public 下同名文件。历史日志目录用于定位原证据，日志、截图及备份不随关键资料包分发。当前蓝图/场景读回、编译、自动 PIE 与打包的覆盖范围另见审计证据；历史通过不升级为本次通过。

项目仍采用同一批角色和设备资产及其原生类。保留 [架构](../ARCHITECTURE.md)、[技术债](TECH_DEBT.md)、[路线图](ROADMAP.md) 和 [测试说明](TESTING.md) 作为后续入口。

