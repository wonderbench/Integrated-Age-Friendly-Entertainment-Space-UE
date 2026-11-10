# Old_Man_Jiang：后续开发入口

这是 UE5.8 的社区适老空间原型：场景观察、独立老人 AI、玩家临时接管、监控、天气灯光和 UMG/WebUI。不是已完成的医疗监护产品。

## 开始任务

1. 先读 [Content/CODEX_PROJECT_GUIDE.md](Content/CODEX_PROJECT_GUIDE.md)，了解关键兼容要求和最近变更。
2. 读 [ARCHITECTURE.md](ARCHITECTURE.md) 和 [docs/CURRENT_STATE.md](docs/CURRENT_STATE.md)，再按任务读取一个相关系统文档。
3. 检查 Git 状态、编辑器当前项目/地图/PIE、未保存包、目标资产父类/组件/引用；文档不能代替当前代码和资产。
4. 先搜索现有实现，不能因为暂时没看到就新建第二套老人、摄像机、MainUI、轮椅或天气系统。
5. 明确本次范围、状态 Owner、调用入口、依赖和验证方式。分析/审计请求不授权功能变更。

## 按任务阅读

| 任务 | 入口 |
| --- | --- |
| 玩家、选人、接管、鼠标视角 | [PLAYER_CAMERA_SYSTEM](docs/systems/PLAYER_CAMERA_SYSTEM.md) |
| 老人状态、动画、轮椅、Sight、样条/目标导航 | [ELDER_AI_SYSTEM](docs/systems/ELDER_AI_SYSTEM.md) |
| 摄像机检测、记录、警报 | [MONITOR_CAMERA_SYSTEM](docs/systems/MONITOR_CAMERA_SYSTEM.md) |
| 天气、时间、灯光、Timeline | [WEATHER_LIGHTING_SYSTEM](docs/systems/WEATHER_LIGHTING_SYSTEM.md) |
| UMG、图表、UI 父类 | [UI_SYSTEM](docs/systems/UI_SYSTEM.md) |
| 公共接口、状态归属 | [DATA_FLOW](docs/DATA_FLOW.md)、[INTERFACES_AND_EVENTS](docs/INTERFACES_AND_EVENTS.md) |
| 未完成项、迁移顺序 | [TECH_DEBT](docs/TECH_DEBT.md)、[ROADMAP](docs/ROADMAP.md) |

完整目录在 [docs/README.md](docs/README.md)。资产索引与审计范围在 [ASSET_MAP](docs/ASSET_MAP.md) 和 [PROJECT_AUDIT_REPORT](docs/PROJECT_AUDIT_REPORT.md)。

## 不得破坏的职责

- `BP_Controller` / `AOrbitCameraController`：玩家输入、UI、选人和显式接管。
- `BP_Player → BP_Character_Base → AOrbitCameraCharacter`：默认观察 Pawn；不承载老人状态。
- `BP_AI → AElderlyAICharacter`：每实例身份与状态；默认独立 AIController，不默认由玩家 Possess。
- 所有老人状态经 `SetElderState`；身份、控制模式、路线进度不能用同一个 Enum 混在一起。
- `AElderlyAIController` 的 AI Perception 是老人真实视觉；视锥 Mesh 仅显示。监控摄像机当前球形 Overlap 不等于真实视锥/遮挡检测。
- `BP_Camera` 复用现有 `AMonitorCameraActor`；`BP_alarm` 是独立警报灯，尚未形成跌倒告警闭环。
- `BP_WheelChair` 复用；正式男女模型尚未接入，男女当前都用 `SK_Mannequin`。
- UBP 下 15 个 Widget 已继承 `UProjectUserWidgetBase`；保留原布局。UI 不拥有另一份老人状态权威。
- 天气/时间选择当前写入 `BGS_State`，经现有 `BPI_Actor_Call.Light_Show` 驱动灯。不能把仍使用的接口、标签分发或 Timeline 当废代码删除。

## 修改协议

- 先查全部调用点/引用，再改公共函数、Enum、Dispatcher、父类或资产路径；保留原有未提交/未保存工作。
- 二进制资产用编辑器、Asset Registry 或已有工具检查；只读文件名不能声称理解蓝图。先备份精确目标，再进行资产写入。
- 优先扩展既有 C++ 入口与 Blueprint 表现层；不要为“解耦”强制接口化所有内部组件关系。
- 状态只有一个权威写入者；允许明确标为展示、缓存、历史的只读投影，不反向覆盖 Owner。
- 可事件驱动的状态变化不用 Tick；禁每帧全场景寻人。输入/动画逐帧计算、有限转身 Tick 不属于状态轮询，不能盲目删除。
- 老人根转身由现有非循环 BlendSpacePlayer 提取根运动；不得恢复旧外部时钟 Evaluator、结束强设朝向或手动重复加根转角。
- 不编造资源、Socket、已完成系统或测试通过；缺失姿势动画不仅缺资源，还需完成表现接入。
- 不批量重命名/改父类/修复重定向器/删除历史节点；技术债先登记，小范围单独验收。
- 不提交密钥、设备标识或完整配置凭据到文档。

## 验证与交付

- 用户已授权：本项目需要备份、保存、关闭/重启编辑器时由智能体自行处理，不重复询问；先备份精确目标并保全未保存工作，不丢弃用户改动，不关闭其他项目。
- 每次执行完成都必须由独立审查智能体审查；不通过则继续修正、验证和复审。实际取得 PASS 后才称审查通过，审查与运行测试证据分别说明。
- 依 [TESTING](docs/TESTING.md) 执行与风险相称的检查。纯文档任务验证链接/事实/变更范围，不为此编译保存用户的脏资产。
- 持久 C++ 行为修改：安全保存目标并关闭本项目编辑器及 Live Coding，再完整构建 `Old_Man_JiangEditor Win64 Development -NoHotReloadFromIDE`，要求 `Result: Succeeded`；不要关闭别的项目。
- 独立测试进程与编辑器禁止同时对同资产进行加载测试和保存；此前发生过文件锁/保存失败。
- 修改过的 Blueprint 必须编译、检查错误/明显警告、保存读回；运行行为另做 PIE 验证。编译、资产验证、自动测试、人工交互、打包是不同证据。
- 修改职责、状态、接口、依赖或数据流时，同步对应 docs；每次实质任务仅在 Content 指南追加重要结果、证据边界和剩余问题。
- 文档写现状；目标方案明确标为 Planned。旧 PASS 不代表当前脏版本通过，不隐藏失败/复测或人工验收缺口。
- 不把命令流水、中间报错和重复资产清单塞进指南；保留影响后续判断的决定及未解决问题。
