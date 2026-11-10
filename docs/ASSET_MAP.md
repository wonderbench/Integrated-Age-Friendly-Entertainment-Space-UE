# 资产与工程定位

2026-09-03 Asset Registry `/Game` 全量清单为 **1870 条注册对象记录**，不是 1870 个独立文件；包含重定向对象。同轮聚焦 32 个项目核心蓝图/接口。完整统计与核心依赖见 [快照](audit/core-assets-2026-09-03.json)。

| Content 目录 | 注册记录数 | 作用 |
| --- | ---: | --- |
| Project_File | 558 | 项目 BP、UI、设备资源、输入和测试地图 |
| MedaFurniturePack | 328 | 家具/场景、正式入口 Showcase |
| OldManAnimset | 65 | 老人移动转身动画、原型 Mannequin、示例地图 |
| UltraDynamicSky | 849 | 天空天气、预设、材质/Niagara、供应商工具 |
| Voyager | 62 | 资源包；内部用途未专项审计 |
| Scifi_HUD_Effects | 4 | HUD 视觉资源 |
| 字体 | 4 | 字体 |

类型包括 StaticMesh 370、SkeletalMesh 17、AnimSequence 88、BlendSpace 2、BlendSpace1D 1、Blueprint 102、AnimBlueprint 2、WidgetBlueprint 31、World 6、DataTable 5、UserDefinedEnum 45、UserDefinedStruct 17、Material 98、MaterialInstanceConstant 199、NiagaraSystem 23、ObjectRedirector 20。类别数量不代表功能使用数量。

## 重点资产

- 业务蓝图：`/Game/Project_File/BP`；设备在 `Actor`，UI 在 `UBP`，GameMode/GameState 在 `System`，样条在 `AI`。根目录旧路径可能是重定向器，不能当重复实现直接删。
- 原生类：`Source/Old_Man_Jiang/Public` 与 `Private`；自动测试：`Private/Tests`。
- 模型：`/Game/OldManAnimset/Mannequin/Character/Mesh/SK_Mannequin`，BP_AI 的 MaleMesh/FemaleMesh 均已保存为此路径。
- 动画：`/Game/Project_File/BP/ABP_AI`、`ABS_Move`；`/Game/OldManAnimset/Animations/TurnInPlace`、TIP_L90/L180/R90/R180_Alt、Idle_Stand、Walk 系列；独立 Idle_Stand_TurnRM 是转身中心样本。
- 显示视锥：SM_Camera_WireFrame 原摄像机资源与 SM_ElderSight_Forward 校正副本；在 Registry 按这两个精确名称定位，不改原模型轴来修老人。
- 轮椅/告警：`/Game/Project_File/BP/Actor/BP_WheelChair`、`BP_alarm`。
- 输入：`/Game/Project_File/Input/IA_Move`、`IA_ExitElderControl`、`IMC_ElderControl`。
- 天气/时间数据：`/Game/Project_File/BP/Data/DT_Weather`（晴/阴/雨），`DT_Time`（Morn=None/0、Moon=早晨/620、Noon=正午/1200、Night=晚上/2400）。保留现有行名与拼写 `Vlaue`；不要无引用迁移直接改名。
- HTML 非资产文件：`Content/Echarts/bar1.html`、`bing1.html`、`line-stack.html` 被三个 Widget 加载；bar2.html 也存在但本轮未确认调用方。含默认示例数据和外部 ECharts CDN；离线/打包另验。

## 六个地图

| 包路径 | 用途/验证边界 |
| --- | --- |
| /Game/Project_File/Maps/Showcase | 当前默认正式展示场景；2026-10-27从MedaFurniturePack/Levels迁入，原场景陈设保留 |
| /Game/MedaFurniturePack/Levels/Overview | 家具资源预览，本轮未加载深查 |
| /Game/OldManAnimset/DemoLevel/DemoMap | 动画资源演示，本轮未加载深查 |
| /Game/Project_File/Tests/L_ElderControlTest | 五组现有自动回归的入口地图 |
| /Game/Project_File/Tests/L_ElderNavigationDemo | 两老人、可延长样条/指定目标演示；有历史保存读回与运行证据 |
| /Game/UltraDynamicSky/Maps/DemoMap | 供应商天空演示，本轮未加载深查 |

没有查到项目自有 SaveGame/GameInstance/BT/Blackboard 资产；不排除未审计插件内部实现。材质/Niagara 本轮仅分类和核心依赖定位，没有逐资源验证渲染或性能。

## 2026-10-27 当前地图与吊牌入口

- 可开关微光蓝图目录：`/Game/Project_File/Props/Wayfinding/Blueprints`，六个子蓝图分别为 `BP_WF_Hanging_Rest`、`BP_WF_Hanging_Play`、`BP_WF_Hanging_WC`、`BP_WF_Hanging_Exit`、`BP_WF_Hanging_Fitness`、`BP_WF_Hanging_Read`，共用父类 `BP_WF_Hanging_Base`。默认关闭额外微光并使用原材质，使用方法见 [微光蓝图](WAYFINDING_GLOW_BLUEPRINTS_20261027.md)。

- 三个当前成品地图统一在 `/Game/Project_File/Maps`：`L_ElderWellness_Main`、`Showcase`、`L_LivingGrove_Social`。上方六图表是早期审计范围，不是当前全部地图数量。
- 吊挂导航牌：`/Game/Project_File/Props/Wayfinding/Meshes`；六种 `SM_WF_Hanging_Rest/Play/WC/Exit/Fitness/Read` 对应休息区、娱乐区、卫生间、出口、运动区、阅读区。用户自行摆放，三个场景没有新增吊牌实例。
- 材质与贴图在同目录 `Materials`、`Textures`；可编辑制作源在 `SourceArt/Wayfinding/Wayfinding_HangingSigns.blend`，使用说明见 [吊牌交付记录](WAYFINDING_SIGNS_20261027.md)。
