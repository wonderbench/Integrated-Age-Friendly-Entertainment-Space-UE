# 项目架构

审计基准：2026-09-03。以下为已检查的现状，非最初需求的完成声明。范围与证据见 [审计报告](docs/PROJECT_AUDIT_REPORT.md)。

## 架构选择

单个运行时模块 `Old_Man_Jiang` 提供老人、控制器、导航、动画支持、监控和 UI 基类；Blueprint 保留组件配置、资源、场景搭建和已有 UI/天气灯光图。没有理由为文档工作统一迁移到纯 C++ 或重新分模块。

```mermaid
flowchart TD
  GM["BGM_GameMode"] --> PC["BP_Controller / OrbitCameraController"]
  GM --> GS["BGS_State"]
  PC --> OBS["默认 BP_Player → BP_Character_Base"]
  PC --> UI["UBP_用户界面 + 原生老人调试控件"]
  UI -->|显式选择与接管请求| PC
  PC -->|选中 / 临时 Possess| EL["BP_AI / ElderlyAICharacter"]
  AI["ElderlyAIController + AI Perception Sight"] -->|通常 Possess / MoveTo| EL
  NAV["每老人 ElderNavigationComponent"] -->|请求与完成回调| AI
  PATH["BP_ElderSplinePath / 目标 Actor"] --> NAV
  EL -->|速度与转身数据| ABP["ABP_AI / ElderAnimInstance"]
  EL --> VIS["仅显示：SightCone / Wheelchair"]
  CAM["BP_Camera / MonitorCameraActor"] -->|球形 Overlap + 具体类型注册| EL
  EL -->|OnElderStateChanged| CAM
  CAM --> LOG["摄像机本地显示字符串 / 历史"]
  UI -->|天气 / 时间选择| GS
  GS --> SKY["Ultra Dynamic Sky / Weather"]
  GS -->|BP_Light 标签 + Light_Show 接口| LIGHT["BP_LightBase_Box + Timeline"]
  UI -->|SOS逐实例 Light_Toggle| ALARM["BP_alarm：Light_Begin / Light_Stop"]
```


## 状态与边界

- 老人实例拥有姓名/性别/状态；Controller 拥有选中与接管对象；路线组件拥有路线进度。
- AIController 拥有实际感知配置与寻路请求。角色上的 Sight 参数是初始化来源，不是已完成的运行时自动同步机制。
- `BGS_State` 拥有天气/时段选择，UDS 负责天空表现，单灯拥有目标开关及渐变进度。
- 监控本地历史是派生记录，不是老人状态权威；UI、AnimInstance 的展示数据也不是权威副本。
- 当前没有查到项目自有 GameInstance/SaveGame 体系；不能把 GameState、字符串历史当作持久存储。

## 已确认的包级环

```mermaid
flowchart LR
  EL["BP_AI"] -->|AnimClass| ABP["ABP_AI"]
  ABP -->|BP_AI 类型引用| EL
  MAIN["UBP_用户界面"] -->|包含子控件| TIME["UBP_时间框"]
  TIME -->|具体父控件引用| MAIN
```

包依赖环不等于执行递归或必然内存泄漏。本次只登记，后续有相关需求时再评估移除反向具体类型引用；不自动重设父类。

目标演进和确认门槛见 [ROADMAP](docs/ROADMAP.md)，状态所有权表见 [DATA_FLOW](docs/DATA_FLOW.md)。
