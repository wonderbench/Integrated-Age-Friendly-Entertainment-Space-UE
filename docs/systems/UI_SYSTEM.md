# UI 与 WebUI 图表

2026-10-30：监控摄像按钮点击后，高亮按该次操作得到的监控范围显示结果更新；再次点击隐藏范围并取消高亮。切换其他导航后返回监控时，也按本次显隐结果决定高亮；无匹配网格时不高亮。其他导航的单选规则保留。

2026-10-30：“警报”文字按钮在左侧栏设计坐标内下移 15 px，使其更接近设备标题中英文之间的水平中心线；按钮业务不变。

2026-10-30 警报入口：左侧设备与告警标题右侧由 `UProjectUserWidgetBase` 运行时创建“警报”文字按钮。当前关卡没有可操作的 `BP_alarm` 时禁用；全部关闭时随机开启一盏，已有开启灯时随机解除其中一盏。高亮、安全状态及告警数量读取实际灯状态，数量为开启实例数，不是历史告警记录；如外部入口同时开启多盏，需逐次解除。复用现有 0.25 秒展示刷新，未新增警报权威状态。


2026-10-30：再次点击高亮的路线按钮会暂停界面所拥有且代次匹配的路线并清除高亮，保留进度；下一次点击走既有暂停恢复逻辑。覆盖旧记录中重复点击从起点重新发起的说明。

2026-10-28 关卡信息：`UProjectUserWidgetBase` 复用原关卡按钮切换 Showcase、L_ElderWellness_Main、L_LivingGrove_Social，并按当前地图显示选中态。概览与设备数字为约定的固定演示值：摄像头各 4 台，环境传感器 3/4/7，健康设备 3/5/5，在线合计 10/13/16；老人 1 人、活动及参与人次为 0。安全状态读取场景 `BP_alarm` 的 `bAlarmActive`，按实例计数，0.25 秒刷新；不代表远程设备在线或完整医疗告警系统。地图字符串路径的打包包含尚未验证。此前仅展示选中的历史说明不代表当前行为。
2026-10-27按钮修复：底部老人相关项在无选人且场景恰有一位有效老人时选中该老人，多人继续显式选择；视角切换后UI保持可点击，再次点击交还AI并恢复观察。路线按钮先将轮椅切到站立，再通过既有路线组件从起点重新请求步行；跌倒/起身等不可移动状态不强制切换。限定实际输入与自动PIE证据见`Saved/CodexBackups/ElderNavButtons_20261027`。

2026-10-27跨图复用：`L_ElderWellness_Main`、`L_TundraSanctuary` 使用既有 `BGM_GameMode → BP_Controller/BP_Player/BGS_State`，由原Controller创建一次主UI；没有复制第二套Widget或状态Owner。两图Foundation标记`Camera_1`供原Controller按Bounds初始化观察中心。底部五项继续走既有接管、姿态、路线、监控范围、老人视线链；左侧关卡按钮仍仅切换选中显示，尚无地图跳转。

`Content/Project_File/BP/UBP` 下15个 Widget 当前统一继承 `UProjectUserWidgetBase`。主界面仍是 `UBP_用户界面`，布局/动画/子控件在 Blueprint；原生 ElderDebugControlWidget 是既有选人调试面板。

2026-09-09目标布局已替换此前暖金布局：继续1920×1080原主界面，左概览/关卡/设备、右空间/合并天气时段/能耗、底部5个无字无图标按钮。原父类增加默认关闭、按Widget启用的关卡/底部展示选中状态；不新增地图跳转或统计Owner。原天气时间按钮与GameState链保留，未接入统计用`--`。旧地区/缩放入口在目标画面隐藏，小菜单原4个按钮作为设备图标热区保留原业务。独立限定范围PASS、0编译错误/1旧警告，非100%像素复刻；详见[布局与证据](../UI_TARGET_LAYOUT_20260909.md)。以下旧布局坐标为历史记录。

2026-09-07暖金换肤已取得1920×1080限定范围PASS：直接替换原MainUI及子Widget视觉，28张独立PNG位于`/Game/UI/NewGoldStyle`，现有层级、动态文字/图表和业务Owner保留，老人原生面板同步换肤。主标题仅X从53调至118以避开Header房屋，其余Slot和模块布局不变；详见[换肤记录与素材映射](../UI_GOLD_STYLE_REPLACEMENT.md)，下述蓝色复原内容为历史基线。

## 交互边界

### 2026-10-27：底部文字与第三、第四项

五项按钮增加居中的 `TargetNavLabel_1..5`，选中文字随 `SelectedNavigationIndex` 变亮，TextBlock不拦截鼠标；按钮依旧单选高亮。第三项仅对已选老人已有Spline配置发起路线，接管时先交回AI，重复点击重新从起点发起；无有效路线不构造目标。第四项点击时统一切换监控摄像机已有 `SM_Camera_WireFrame` 的可见性，未修改检测。当前展示为蓝色半透明实体范围；下方第三、第四项未接入的旧说明由本段覆盖。

### 2026-10-08：底部五项单选显示

`TargetNav_1..5` 点击后以 `SelectedNavigationIndex` 保持唯一高亮，切换选项恢复前一项普通贴图；同项重复点击仍高亮。该属性只属于 UI 显示，不再由接管、轮椅、起立或视锥的开关结果清空。第二项在动作不可执行时仍可用于导航选择，业务动作仍按老人状态检查。下方2026-09-10关于“关闭功能即取消高亮”的旧描述已由本条覆盖。

### 2026-09-11：C键跌倒与第二项起立（覆盖下方旧规则）

Controller以运行时 Enhanced Input `IA_ElderFall / IMC_ElderFall` 将C映射到Started；只在游戏焦点且未处于Alt UI操作时执行。接管时作用于被接管老人，否则作用于显式SelectedElder；仅Standing/Walking可跌倒，重复、轮椅与空选择拒绝。状态仍经原StartFalling/SetElderState。

TargetNav_2随选中老人状态事件更新：Standing/Walking=Wheelchair，轮椅=Stand，Fallen=GetUp，Falling/GettingUp=Busy且禁用；无选择=Unavailable。Fallen点击复用StartGettingUp，资源缺失禁用，动画结束恢复Wheelchair。换人/销毁/Widget重建负责解绑；只读ElderActionMode与Tooltip说明语义，按钮图标继续留空。用户暂缓红灯联动，等待分区配置。

### 底部目标导航（2026-09-10）

五个空白按钮继续复用 `TargetNav_1..5`，图标尚未导入。第一项调用既有 Controller 接管链：观察模式点击后接管当前选中老人，再点归还观察 Pawn；接管被拒绝时不误亮。第二项只在 `Standing` 与 `WheelchairIdle/Moving` 间调用既有 `EnterWheelchair/ExitWheelchair`，不会覆盖跌倒、起身或其他过渡状态。第五项经 `UProjectUserWidgetBase → AOrbitCameraController::ToggleSelectedElderSightCone` 切换当前选中老人的 `SightConeMesh`；显示时第五项高亮，再点隐藏并取消高亮。没有选中老人时不改变按钮状态。没有新增第二套老人、轮椅、视锥或状态 Owner。

第三项预设样条方案、第四项监控摄像头，以及第五项视野内导航标识3D注释仍是 Planned。本段旧第二项规则已由上方2026-09-11状态机规则覆盖；区域红灯仍等待用户之后提供分区。

2026-09-08高清贴图：Desktop/2的20张匹配素材替换原NewGoldStyle纹理，保留资产路径与全部UI蓝图；原图备份及两张跳过原因见[高清替换记录](../UI_HD_REPLACEMENT.md)。运行1920画面已检查，未扩展为全部物理点击/打包验收。

2026-09-08老人状态按钮：复用原生`ElderDebugControlWidget`，通过Controller.SelectedElder选人；Standing显示轮椅/跌倒，轮椅状态显示可用站立按钮；Falling显示禁用站立按钮，Fallen在配置GetUpAnimation时启用站立按钮并调用StartGettingUp，GettingUp期间按钮保持可见禁用。其余状态/空选择全部Collapsed。按钮调用现有EnterWheelchair/StartFalling/ExitWheelchair/StartGettingUp；HandleStand按当前状态选择退出轮椅或倒地起身，不建立第二份老人状态Owner。绑定选择、老人状态、销毁事件，换人/NativeDestruct解绑，无Tick轮询。

三张用户PNG原样导入`/Game/UI/NewGoldStyle/T_UI_ElderWheelchair`、`T_UI_ElderFall`、`T_UI_ElderStand`。前两张不带Alpha，用同目录`M_UI_ElderWheelchair/Fall`径向Opacity遮去黑色外背景；原PNG不修改。按钮统一66×66（初稿110的0.6倍，布局及命中框一起缩小），未用RenderTransform。1920布局中轮椅/站立槽位(144,350)，跌倒(280,350)；原选人组右移避开导航栏，状态说明位于(144,468)。沿用原定位按钮展开/收起面板，未改变默认收起规则。已有AlwaysCook目录覆盖新资产，但本轮未打包。

初版完整构建Succeeded；Showcase PIE委托点击、七状态显隐、空选择、解绑、面板重建和销毁历史通过，1920实际站立/轮椅截图已检查。后续FallingDown与GettingUp均已接动画：Falling按钮可见禁用，Fallen在有起身动画时启用，GettingUp期间再次禁用；两轮起身、重复从0秒播放和轮椅退出语义见`Saved/Tests/GettingUp_ExplicitClock_*`。不是物理鼠标或所有DPI验收。初版证据`Saved/Tests/ElderActions_PIE.json`、`ElderActions_FinalValidation.json`及`FallHands_PIE.json`仅为旧规则基线。

- SOS：原小菜单按钮点击逐实例调用BP_alarm.Light_Toggle，默认关、再次点击关；后续事件可直接调用Light_Begin/Light_Stop。2026-09-04补SOS悬停1.01倍、移出1倍，幅度与上方一致但为原生即时缩放（不是0.1秒动画）；90px命中框不变，无裁切。运行读回90→90.9→90px。
- 老人按钮：父类 GetProjectController/GetSelectedElder → Controller 的选择、接管或视锥 API；不直接取第一个老人，不将老人作为默认玩家 Pawn。
- 父类 NativeConstruct 绑定选择事件、NativeDestruct 解绑；业务状态来自 Owner，Widget 只保留展示/高亮/展开状态。
- 天气/时间按钮目前直接引用 BGS_State 写选择并通知。审计不把这条已存在链伪装成全事件总线，也不为解耦立即重写。
- `BPI_用户界面.点击缩放` 控制主 UI 动画；主界面给 UBP_缩放 注入接收者。不要误认为是老人相机变焦入口。
- 主UI包含时间框，时间框又持具体父UI引用，形成包级环。部分子Widget实例仍保留旧“空间说明/娱乐内容使用统计”名称；这些不等于另有一套天气/时间系统。

## 图表与时间

人次统计→bar1.html；安装指标→bing1.html；能耗曲线→line-stack.html，文件均在 Content/Echarts，使用 WebUI 加载并调用 ue2js。能耗页已改为本地动态Canvas、严格校验与空态，不依赖CDN；其他页仍有ECharts CDN。Widget仍使用原固定Delay等待。默认数组是示例，真实业务数据/设备统计绑定未验证。

时间框每秒读Now，日期无千位分隔、时分秒补零，星期来自系统日期。原硬编码“阴”已移除；顶部天气图标仍是原静态图标，未同步所选天气。退出按钮接QuitGame。旧重建UI/返回链Bad cast节点未接执行线，保留原警告，不当作当前必定执行崩溃。

## 1920×1080参考图复原

2026-09-04用户要求紧凑布局：工具条X1270；右侧四模块屏幕Y127/359/497/647、宽552及各模块高度不变，布局间距4px，组中心575.5不变。素材、业务和收展动画不改；详见复原记录最新节。

重构原MainUI与10个子Widget，没有新建重复Widget Blueprint或C++。设计根为Canvas→ScaleBox→1920×1080 SizeBox→原Canvas；右侧四框统一X=1349/W=552。独立PNG、UMG文字和真实Button组合；天气/时段选中效果读取原BGS_State，业务链不变。原UI收展按钮保留；新增+/-调用原Controller缩放；定位按钮开关单一原生选人调试面板。源素材映射、真实功能/仅视觉边界、验收证据与剩余差异见[复原记录](../UI_REFERENCE_RECONSTRUCTION.md)。

## 后续扩展

2026-09-08：显示/隐藏老人视线按钮仍走既有 Controller 入口；非 Alt 接管点击后自动归还游戏焦点与捕获。Alt 按住时继续保留 UI 操作，观察模式不强制捕获；未修改按钮布局或禁止按钮键盘聚焦。

2026-09-04续修：空间说明也已恢复15_53_29原素材内部；UBP_右侧栏的PanelFill/TitleRule/TitleAccent停用，正文和标题独立，保持552×228外框。下段“空间说明仍使用材质”为此前状态，已被本项取代；能耗按用户要求保持原状。

空间说明/能耗PanelFill仍使用共享UI材质M_UI_PanelRecess。2026-09-04天气/时段Prepared恢复原PNG渐变、内框和装饰，仅局部移除烘焙标题；两个Widget停用PanelFill/重复分隔线/文字装饰，原TextBlock和业务链保留。停止PIE后两Texture重导保存成功、导入哈希与Prepared一致；两Widget编译0错误0警告。高分辨率离屏图不等于完整场景窗口验收，详见复原记录。

保留现有UI，新增业务数据投影明确来源；图表以页面就绪回调取代固定延时需单独实施。测试选人为空/两人切换/对象销毁、接管/退出、视锥反复切换、Widget重建解绑、天气时段组合、图表离线与打包，以及游戏视口/编辑器焦点。
