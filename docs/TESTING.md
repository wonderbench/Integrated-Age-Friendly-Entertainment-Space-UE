# 验证协议

## 层级与前置

每次分别报告：源码/结构检查、完整原生构建、蓝图编译与保存读回、自动 PIE、有渲染姿态、人工输入、打包。不能用其中一种冒充全部验收。

开始前确认本项目、目标地图与所有未保存包；先处理用户修改，禁止盲目 Save All。用户已授权自行备份并安全关闭本项目编辑器；关闭请求超时不等于进程已退出，确认退出后才构建。独立测试与编辑器对同资产的保存必须串行。每次执行完成需独立审查智能体 PASS。

持久 C++ 行为修改安全关闭本项目编辑器/Live Coding 后，使用本机已有的 UE5.8 构建入口（执行前确认路径仍有效）：

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' Old_Man_JiangEditor Win64 Development '-Project=E:\Jiang_Work\Old_Man_Jiang\Old_Man_Jiang.uproject' -WaitMutex -NoHotReloadFromIDE
```

要求 Result: Succeeded，然后重启、编译修改及受影响的 Blueprint，检查警告/Invalid Reference并保存读回。纯文档不需要这套有状态操作。

## 现有自动测试

先打开 `/Game/Project_File/Tests/L_ElderControlTest`（不是Showcase或导航演示图），在未运行PIE的控制台按下列精确路径运行。轮椅持续转向改造使用WheelchairSteering、WheelchairSeating、WalkingSteering、ActorLocalWalking、CameraLook、TurnPlayback、InputMapping（均有OldMan.Elder.前缀）；普通WASD、AdjacentTurn可另跑。测试自行启动/结束PIE，异步等待；同项目其他任务须停止UE调用，避免错误日志/重启污染。不要把仍含旧契约的全前缀测试当当前验收。

- OldMan.Elder.Takeover：历史测试，固定世界XY/相机-135°和出生Standing断言尚待适配Actor Local及用户默认轮椅；本轮不运行、不宣称通过。
- OldMan.Elder.CameraLook
- OldMan.Elder.WalkingSteering：真实按键注入，普通Walking的W→WD/WA/SD连续速度、每帧转速上限、无根TIP/状态历史抖动、实际动画速度；TurnSpeed 2/3/5、松键停位继续尾转、尾转期间新轻点恢复根TIP；坐姿/轮椅/归还AI清理及AI中心到达。独立于轮椅持续转向，不代替正式场景脚底渲染与物理键盘验收。
- OldMan.Elder.WheelchairSteering：真实按键注入，D持5秒累计超过360°、A反向、W/S前后、WD/SD边走边转、WD→W/D及D→A交接、A+D相消、松键无尾转；逐帧转速/WD最低速度、鼠标控制旋转与实际Camera、45°/s参数、禁移/退出/归还AI清理。Alt只验证共享Release入口，非完整Alt物理按键测试。
- OldMan.Elder.ActorLocalWalking：普通步行连续A-W-D-W-A-WD再遍历八方向，鼠标独立，轻点/持键、局部侧移与AI归还；站立测试显式建条件。OldMan.Elder.WheelchairCamera保留旧固定目标/侧移契约，已被新WheelchairSteering替代，本次不运行或声称通过。
- OldMan.Elder.WheelchairSeating：L_ElderControlTest实跑Socket挂接、原偏移清除、轮底接地、超过4.3秒的循环坐姿、AI移动/到达、接管移动/归还、性别保持、退出高度恢复、TIP中途坐入无残留根转角及退出再转；直接调用既有移动入口，不冒充物理WASD验收。
- OldMan.Elder.WASD：PIE中真实按键注入的四对角线轻点、八方向持键/64cm/s、不同初始朝向的Actor局部坐标、双键两种顺序/释放一键、已开始转身后的新目标与松键、朝向相同轻点不走、0.2s门槛及禁移/归还AI清理；可在隐藏嵌入视口运行以隔离外部输入，不代替脚底渲染/物理键盘验收。
- OldMan.Elder.InputMapping：直接读取EnhancedPlayerInput，对照旧最高绝对值/新累加方式的WD结果，验证新方式W+S、A+D相消；测试临时旧值不保存，结束/异常退出恢复Cumulative。
- OldMan.Elder.AltUI：新PIE窗口内实际Slate键/鼠标路由，按住Alt后原调试按钮命中点击、UMG持焦松Alt、移动/视角抑制、双Alt、无焦点释放不抢占、回游戏恢复、返回观察和重复接管。测试仅临时展开已有调试面板；不保存或新建UI资产。需可显示的PIE窗口（嵌入视口被蓝图页隐藏时无法检验Slate命中），不是物理键盘/全部WebUI菜单验收。
- OldMan.Elder.AdjacentTurn
- OldMan.Elder.TurnPlayback
- OldMan.Elder.Navigation
- OldMan.Elder.SmoothNavigation：64/160cm/s直线逐帧速度、中间点数、监控事件数、实际动画节点倍率、终点停止；闭环跨圈连续速度。Navigation中长距离阶段时限按距离/速度计算，不放宽到达误差。

`OldMan.Showcase.SmoothNavigation` 单独在Showcase运行：验证已保存的真实路线和64cm/s、逐帧无停顿、状态历史、动画快照及脚部骨骼变化。该项不放入OldMan.Elder组，避免地图不同导致整组误失败。测试临时强制骨骼刷新只影响PIE实例，不保存地图；实际资产接线另读回验证。闭环连续性测试使用用户要求的64cm/s，原Navigation另保留300cm/s两圈压力测试；急弯的正常加速度减速不等同于中间点Stop/Standing跳变。

连续性口径：直线保留95%巡航速度下限；闭环排除首次起步后逐帧检查无停步/无Standing跳变，以摩擦、加速度和帧间隔构建测试场景的速度突降阈值（非UE严格物理上界），最大阈值必须小于巡航速度一半，防止长帧掩盖清速。弯道允许正常转向减速，不要求任意曲率全程严格恒速。动画检查实际时钟推进的样本，不能把零权重节点未更新缓存当当前倍率；投影允许NativeUpdate与postactor采样之间的一帧时序差。

## 按改动验收

`OldMan.Wheelchair.Rolling` 在PIE停止时从控制台运行，自动开始/结束PIE并生成两把临时轮椅。覆盖当前大小轮测量半径、距离/半径转角、倒退、停止、竖直位移、传送、原地差速、禁用/隐藏恢复、正等比缩放、多实例隔离、Body保持及父Actor带动。证据`Saved/Tests/WheelMotion_PIE.log`；实际Scene旋转和轮辐另用PIE组件读回及`WheelMotion_SideRest/SideRolled.png`检查。该项不表示老人坐姿/朝向对齐或小轮万向转向通过。

`OldMan.Showcase.CeilingVisibility` 在Showcase、PIE停止时经控制台运行，实际PIE检查115个已保存成员的初始隐藏、两次接管/返回、失败请求、选择变化、老人销毁、Controller结束、直接UnPossess及动态标记刷新；同时断言无标签模型、碰撞与灯强度不被接管操作修改。灯强度按每次操作所在帧比较，允许天气/警报在不同帧正常动画。测试删除只发生于PIE副本，不保存地图。

| 改动 | 必验 |
| --- | --- |
| 身份/状态/姿势 | 两老人独立名/性别/状态；Male→Female→Male不重置；全八状态、同态去重、禁移、Falling→Fallen、Fallen→GettingUp→Standing；真实跌倒/起身须验首次与重复播放、期间禁移和末帧恢复，座位姿势仍待资源 |
| 输入/根转身/视角 | 五组相关回归；轻按/持键/相邻/相反/双键交接、转身零平移、末尾不回位、连续片段、鼠标只看、右键松开、Tab和删除对象恢复；有渲染骨骼验收 |
| 导航 | 每老人不同目标、延长样条保存重载、循环、障碍/不可达/删除目标、外部MoveTo覆盖、暂停/接管/Controller重建；演示图不修改Showcase |
| 老人 Sight | Perception前后/边缘/遮挡、参数刷新、接管暂停恢复；显示开关不改变检测，长度/半角近似对应 |
| 摄像机/告警 | 真实视域实现后验前后/墙后；初始已重叠/多组件入离场/销毁/多摄像机/同名老人/重复绑定/历史不漏；警报开始解除另测，当前未实现闭环 |
| 天气灯光 | 9组合×两种选择顺序，所有灯而非一盏；启动同步、重复目标、渐亮渐灭中间值、过渡反向、灯标签/接口有效 |
| UI/图表 | 原MainUI不重复创建；无选中/多老人/删除/重建、焦点隔离、天气文字来源；HTML就绪、空数据、离线/CDN失败、打包文件可加载 |
| 未来存档 | 尚无现有测试；实现时覆盖版本迁移、稳定ID、多老人/多摄像机、跨关卡、损坏/缺失存档及错误恢复，不序列化瞬态Controller引用 |

## 历史证据，不是当前版本重跑

| 文件（Saved/Tests/） | 可支持的结论 |
| --- | --- |
| ElderTurnPlayback_Rendered.log | 同文件有失败和后续成功；03:39:39 UTC有渲染8例成功，不能抹去此前失败 |
| ElderCamera_Rendered_AfterRecovery.log | 04:00:29 UTC CameraLook成功，资产恢复后的相机链验证 |
| ElderNavigation_AllFive_PreOverride.log | 外部MoveTo优先修正前的五组运行，不是最终版全部重跑 |
| ElderNavigation_Final.log | 04:18:19 UTC Navigation最终版成功 |
| ElderNavigation_Takeover_Rendered.log | 04:24:35 UTC有渲染Takeover成功 |
| ElderNavigation_DemoVerification.md | 4点/1226cm保存重载，两老人各自完成目标，非人工Alt拖拽 |
| WeatherLights_Verified.json | 按钮委托触发18组合、47灯与渐变；不是物理点击，更不是当前未保存灯包通过 |
| ShowcaseNavigation_Verified.json | 补导航后的Showcase，3次有渲染PIE自动完成用户4点/1129.7cm样条；出生点及控制点导航投影通过，终点水平误差约1.2cm；非整栋楼/全部楼层验收 |

无渲染运行不能验脚部骨骼姿态；程序发输入或按钮委托不等于全部人工交互。已有 UI/渲染/退出导航警告按原日志保留，不能一概当失败，也不能全部忽略。

2026-09-09步行调校：WalkingPace_FinalBuild.log为Succeeded；WalkingPace_ShowcaseNavigation.log为Showcase.SmoothNavigation Success，1197帧、min49.955/max51.2、zero0、badAnim0、footRange18.001。WalkingPace_Runtime.json在旧测试地图临时匹配当前laor/ABP并仅测试期间刷新骨骼，6秒完整周期步行/椅/步行速率断言全部通过；不保存测试地图。首轮Showcase移动减速中止，空旷测试2秒短窗口足部幅度不足，均不算通过；最终按完整周期复测。导航测试旧速度、播放率断言及目标超时改读新有效速度/倍率，未重跑所有旧套件。未声称精确无滑步或全部人工视觉验收。

2026-09-08动画输出修复：`Saved/Tests/ElderAnimationRepair_Runtime.json` 为Showcase真实Tick下坐姿→站立→MoveElder位移及两轮跌倒/起身骨骼断言；骨骼高度是组件空间，起身采样为开始后约0.07/0.10秒，不是精确首帧0。`ElderAnimationRepair_AltUI.log` 中扩展AltUI实际Slate点击站立后W位移断言通过，检查在任何额外焦点恢复之前执行。完整构建、蓝图编译与重开引用分别见同前缀Build.log、Compile.json、Reload.json；Graph.json记录最终True/False连线。旧GettingUp图只记录connected，未能证明连线方向，已明确纠正。

2026-09-08 Sight 输入回归：`OldMan.Elder.AltUI` 增加真实 Slate 鼠标按下/松开与 W 键事件，覆盖 Alt 持续点击、鼠标松开前释放 Alt、无 Alt 的 GameAndUI 点击；断言切换、W 位移、GameOnly 捕获及视口键盘焦点。修复前无 Alt 场景只复现捕获断言失败（W 仍透传），未声称自动复现用户所有停走条件。修复后整项 Success，日志 `Saved/Tests/SightInput_Fixed.log`；完整闭编辑器构建 `SightInput_FixBuildFinal.log` 为 Succeeded。不是全部地图的人工交互回归。
