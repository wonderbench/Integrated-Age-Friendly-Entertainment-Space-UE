# 项目概况

项目是社区嵌入式适老娱乐空间的交互原型：观察空间、控制老人进行演示、查看摄像机/设备、切换天气时间与灯光。不是完整生产级老人监护、异常诊断或持久化平台。

## 运行组成（CONFIRMED）

- UE5.8；工程文件 `Old_Man_Jiang.uproject`；单运行时 C++ 模块 `Source/Old_Man_Jiang`。
- 启动地图：`/Game/Project_File/Maps/Showcase`，来自 `Config/DefaultEngine.ini`；2026-10-27从MedaFurniturePack/Levels迁入。
- Showcase 的 GameMode Override 为 `BGM_GameMode`；其已保存默认值指定 `BP_Controller`、`BP_Player`、`BGS_State`。
- `BP_Player` 继承玩家观察角色 `BP_Character_Base`，不是老人。
- 项目声明 WebUI、UE_MCP_Bridge、PythonScriptPlugin、ModelingToolsEditorMode；插件/引擎安装和打包能力不等于启用声明。本轮不修改第三方内容。
- 原生依赖包括 UMG、AI、GameplayTasks、NavigationSystem、EnhancedInput、Slate、AnimGraphRuntime；编辑器目标另依赖 UnrealEd。

## 场景与开发阶段

Showcase 的初次审计有47个灯、4个监控摄像机、1个警报灯和UDS天气/天空。2026-09-03后续已保留用户放置的BP_AI及4点样条，补NavMeshBounds_ElderRoute/Recast导航并保存；3次有渲染PIE验证当前路线自动完成，不代表其他楼层/全部空间已完成导航集成。

老人演示和回归有独立地图 `L_ElderNavigationDemo`、`L_ElderControlTest`。UI、状态与输入基础已有实现；真实摄像机视域、完整姿势动画、告警业务、统计数据来源和存档仍有缺口，详见 [当前状态](CURRENT_STATE.md)。
