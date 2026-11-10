# 构建、蓝图编译、自动 PIE 与打包证据

审查日期 2026-11-10（UTC+8）。对象为 `E:/Jiang_Work/Old_Man_Jiang/Old_Man_Jiang.uproject`。本报告只读取日志、记录、文件元数据及测试声明。当前工作区包含未提交及未跟踪文件，历史记录没有覆盖本次磁盘快照及编辑器内存态的完整版本指纹。本报告引用项目内相对路径和日志行号，原始日志、构建产物、崩溃文件及配置凭据不随资料包分发。

| 检查 | 本轮状态 | 最新可归属记录与适用范围 |
| --- | --- | --- |
| `Old_Man_JiangEditor Win64 Development -NoHotReloadFromIDE` | 未执行 | **历史通过（项目记录）**。2026-11-05 项目指南第 3–7 行报告关闭编辑器后完整构建成功，且本机目标 DLL、构建收据的磁盘时间均为 16:22:57。`ElderAnimInstance.cpp` 与 `FallTransition_20261105/After` 副本哈希一致。未找到当日该目标的原始 UBT 终态日志。最近找到带明确目标、参数和 `Result: Succeeded` 的原始归档是 2026-10-30 `Saved/CodexBackups/MonitorToggle_20261030/build_final.log` 第 2、30 行。 |
| 六个优先 Blueprint 编译 | 未执行 | **全组当前状态未验证**。2026-11-05 启动日志记录 `UBP_时间框` 的编译警告，并在开始 Showcase PIE 时写明无需重编蓝图。它既未提供六个目标的编译统计，也不能证明当前无警告。2026-09-02 `Saved/Logs/CompileElderAssetsFinal2.log` 第 1632、1641、1657、1666 行记录 `BP_AI`、`ABP_AI`、`BP_Controller`、`BP_Camera` 完成旧编译脚本的记录，无法覆盖后续变更。 |
| 命名的自动 PIE 测试 | 未执行 | **历史通过与失败均有**。最近找到的原始 `LogAutomationController` 终态为 2026-09-07 `OldMan.Elder.TurnPlayback`、`OldMan.Elder.WheelchairSteering` 成功。2026-09-09 的 `OldMan.Showcase.SmoothNavigation` 成功由 `docs/TESTING.md` 第 71 行记录，当前项目内没有该段指向的 `Saved/Tests` 原始附件。 |
| Showcase 运行时脚本/委托检查 | 未执行 | **历史局部运行**。2026-11-05 主日志记录 Showcase PIE 创建和结束，指南记录调用跌倒/起身状态函数并读回变量。已找到的原始编辑器日志可证实该场景进入 PIE，无法独立证明所有变量断言，也未见命名 Automation 成功标记。视觉衔接、脚和手穿地以及末段姿势仍需验收。 |
| Windows 构建、Cook、Stage、打包及独立包启动 | 未执行 | **本项目未找到可归属的结果**。检查了项目 `Saved/Logs`、`Saved/StagedBuilds` 与本机 UE 5.8 AutomationTool 日志。全局最新的 2026-11-10 `ExitCode=0` 属另一个 `.uproject`，排除。本项目不能记为成功或失败，既往有无实际运行过仍未确认。 |

## 证据索引与边界

- **E1（11 月 5 日构建与源码）** `Content/CODEX_PROJECT_GUIDE.md:3–7`，`docs/systems/ELDER_AI_SYSTEM.md` 首段，`Saved/CodexBackups/FallTransition_20261105/After/ElderAnimInstance.cpp`。`Binaries/Win64/UnrealEditor-Old_Man_Jiang.dll`、`Binaries/Win64/Old_Man_JiangEditor.target` 与当日 `ElderAnimInstance.cpp.obj` 为编译产物佐证。时间戳仅佐证生成时间，不能代替完整 `Result` 或工作区版本指纹。
- **E2（10 月 30 日构建失败及复测）** `Saved/CodexBackups/AlarmButton_20261030/build.log:24,30,39` 记录 `UButton::SetIsFocusable` 不存在及局部 `Slot` 遮蔽成员，最终失败。`build_final.log:30` 与 `build_reviewfix.log:30` 记录后续成功。另有 `Saved/CodexBackups/MonitorToggle_20261030/build_final.log:2,30` 成功。此失败应保留在历史中，不能视为当前版本仍有相同编译错误。
- **E3（蓝图编译与警告）** `Saved/Logs/Old_Man_Jiang.log:2157,2438` 显示时间框节点 `Reset Create Main UI` 有非纯函数无执行引脚的编译警告，以及 PIE 启动时无需重编。`Saved/Logs/CompileElderAssetsFinal2.log:1632,1641,1643,1657,1666,1672` 记录同一旧警告与历史编译标记。`Saved/CodexBackups/AlarmRangeAlign_20261030/compile.json:8–10` 记录旧 `BP_alarm` 编译成功且错误、警告均为 0，作用域为该蓝图。
- **E4（原始自动化日志）** `Saved/Crashes` 内 2026-09-07 `Old_Man_Jiang.log:2699,2800` 的 `LogAutomationController` 结果为 `TurnPlayback` 和 `WheelchairSteering` 成功。`Saved/Logs/UIPanelFill_GPUCrash_20260904.log:2659,3760,3883` 显示 `CeilingVisibility`、`CameraLook`、`Takeover` 成功，日志所在会话名称和背景不能用于推断全会话正常结束。`Saved/Crashes` 内 2026-09-03 两份独立日志中，UTC 08:11:07 和 08:22:02 的 `OldMan.Elder.SmoothNavigation` 失败，08:24:59 后续运行成功。重复崩溃副本按时间与测试名去重。工程目前在 `Source/Old_Man_Jiang/Private/Tests/` 中声明 18 个命名测试，静态声明不构成执行证明，也不保证测试地图和断言仍适用于现在的配置。
- **E5（11 月 5 日 PIE）** `Saved/Logs/Old_Man_Jiang.log:2441–2446,2485,2500,2563,2570` 记录 Showcase PIE 实例创建、启动和结束。`Content/CODEX_PROJECT_GUIDE.md:3–7` 记录起身调用、变量读回和剩余视觉问题。`Saved/CodexBackups/MonitorToggle_20261030/final_runtime.txt` 则是较旧的监控网格显隐和按钮委托证据。脚本调用不覆盖所有物理交互或全地图回归。
- **E6（打包归属）** `%APPDATA%/Unreal Engine/AutomationTool/Logs/E+UE_5.8/Log.txt` 第 3 行的项目名与第 1800 行的退出码对应另一项目。项目日志中未找到本项目 `BuildCookRun` 终态，`Saved/StagedBuilds` 未见产物。打包与独立包启动分别保留为待验证。

后续验收需要用同一份已保存版本分别记录目标构建、逐蓝图错误和警告统计、按地图划分的命名 PIE 测试结果，以及可归属本项目的打包和独立包启动记录。建议将版本指纹与每个结果一同归档。