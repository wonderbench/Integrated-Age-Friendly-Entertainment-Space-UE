# 核心蓝图职责

2026-09-03：核对 `/Game/Project_File` 下 32 个 Blueprint/AnimBlueprint/WidgetBlueprint（包含两个接口）；父类、组件、变量、自有函数和 EventGraph 均读取，部分调用链进一步查连线。不是对所有第三方 Blueprint 的逐节点审计。

下表路径相对 `/Game/Project_File/BP/`；原生类名省略 UE 的 A/U 前缀。详细机器快照见 [core-assets](audit/core-assets-2026-09-03.json)。

## 角色、设备与装配

| 资产 | 当前父类 | 职责 / 主要依赖 | 不应承担 |
| --- | --- | --- | --- |
| BP_Controller | OrbitCameraController | 创建原 MainUI；观察输入/选人/接管；依赖玩家 Pawn、UI、老人入口 | 老人内部状态、灯亮度、存档全集 |
| BP_Character_Base | OrbitCameraCharacter | 观察角色及相机组件 | 老人 AI 状态机 |
| BP_Player | BP_Character_Base | GameMode 默认 Pawn 配置 | 默认替换成 BP_AI |
| BP_AI | ElderlyAICharacter | 老人原生能力的资产/组件装配；ABP_AI、轮椅、视锥 | MainUI 或全场景管理器 |
| ABP_AI | ElderAnimInstance | 移动/转身动画图；读取 BP_AI 与父类动画参数 | 修改老人权威状态、AI 检测 |
| AI/BP_ElderSplinePath | ElderSplinePath | 可编辑 RouteSpline 场景路线 | 作为老人子对象随身体移动 |
| System/BGM_GameMode | GameModeBase | 配置 BP_Controller、BP_Player、BGS_State | 每老人数据 |
| System/BGS_State | GameStateBase | 天气时段选择、天空时间渐变、灯接口广播 | 当前未实现的持久存档职责 |
| BPI_Actor_Call | Interface | Light_Show 灯光消息契约 | 保存天气权威状态 |
| Actor/BP_ActorBase | Actor | 设备基础场景根 | 所有设备业务逻辑全集 |
| Actor/BP_Camera | MonitorCameraActor | 原扫描视觉组件 + 原生监控范围/历史 | 直接改老人状态；把扫描 Mesh 当检测体 |
| Actor/BP_LightBase | BP_ActorBase | 灯基础继承层，含历史开关字段 | 第二份独立天气状态 |
| Actor/BP_LightBase_Box | BP_LightBase | SpotLight、Light_Show、手动 Light_Show1、Timeline | 删除仍在使用的灯接口；推断 UI 按钮来源 |
| Actor/BP_alarm | BP_ActorBase | 警报亮度/旋转表现；Light_Begin | 冒充已接入跌倒识别服务 |
| Actor/BP_WheelChair | BP_ActorBase | 轮椅组合模型；工具返回 219 个组件记录 | 老人身份/寻路权威 |
| Actor/BP_Toilet | Actor | 卫生间组合模型，13 个组件记录 | 未发现的交互/报警业务 |

## UI：15 个 Widget 均继承 ProjectUserWidgetBase

每个资产前缀为 `UBP/`。除继承的老人访问入口外，原布局、动画、事件保留。共同约束：不能复制老人/天气业务权威，不靠“全场景找第一个老人”发送命令。

| Widget | 当前职责 / 依赖 |
| --- | --- |
| UBP_用户界面 | 主界面组合；向时间框注入父控件引用、向缩放注入接口接收者、向天气时间控件注入 GameState |
| UBP_左侧栏 | 组合小菜单、当前区域等左侧展示 |
| UBP_右侧栏 | 组合天气、时间、能耗、设备展示；部分子控件实例仍保留旧名称 |
| UBP_小菜单 | 菜单按钮/悬停与激活状态；不据标签文字推断跨关卡加载已实现 |
| UBP_当前区域 | SetIsActive / 区域选择外观与引用；不是已验证的房间业务注册表 |
| UBP_缩放 | BPI_用户界面.点击缩放 请求，驱动主 UI 展开/收起 |
| UBP_时间框 | 每秒 Now 格式化日历时钟，文本尾部“阴”写死；有未连接的旧返回/重建 UI 链 |
| UBP_天气选择 | 晴/阴/雨按钮读取 DT_Weather，写 BGS_State 后通知 |
| UBP_时间选择 | 早晨/正午/晚上读取 DT_Time，写 BGS_State 后通知 |
| UBP_人次统计 | WebUI 加载 Echarts/bar1.html，ue2js 更新入口 |
| UBP_安装指标 | WebUI 加载 Echarts/bing1.html，ue2js 更新入口 |
| UBP_能耗曲线 | WebUI 加载 Echarts/line-stack.html，ue2js 更新入口 |
| UBP_地点 | 位置展示布局；未发现完整地点业务逻辑 |
| UBP_活动内容计划 | 活动展示布局；未发现活动调度后端 |
| UBP_设备统计 | 设备展示布局；未确认数字与真实设备事件绑定 |

另有 `UBP/Support/BPI/BPI_用户界面`（Interface，点击缩放），不是 Widget，不能改成 ProjectUserWidgetBase。`E_区域类型` 是 Enum，同样不是控件。

已确认依赖环：BP_AI ↔ ABP_AI；UBP_用户界面 ↔ UBP_时间框。当前存在具体类型引用不等于应一律删除；按相关任务检查加载依赖和实际调用再小范围处理。
