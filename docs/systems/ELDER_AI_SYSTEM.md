# 老人 AI、动画与导航

2026-11-05：起身时保持跌倒末帧作底层姿态，退出时保留起身播放时间以供淡出，重新激活时归零。闭编辑器构建成功、Showcase PIE 变量读回和限定源码复审通过；现有起身素材与跌倒末帧姿态差异大，运行骨骼采样提示可能穿地、末段姿势异常，视觉衔接尚未验收，需检查源动画和重定向。跌倒触发入口保持默认停用。


2026-11-03：老人显示距离改为独立 `SightDisplayDistance`，默认600cm，限制在真实 SightRadius 内；真实AI感知半径未改。旧说明中显示长度沿 SightRadius 已被此条替代。

2026-10-30：再次点击高亮的路线按钮会暂停界面所拥有且代次匹配的路线并清除高亮，保留进度；下一次点击走既有暂停恢复逻辑。覆盖旧记录中重复点击从起点重新发起的说明。

2026-10-29 视线起点：`SightDisplayOriginOffset` 在 BP_AI 的 Elder|Sight|Visualization 中按厘米设置，X前/Y右/Z上，默认(15,0,0)，以角色水平朝向计算并随头部位置；头部到偏移点的 Visibility 路径被挡时清除线条。该参数只影响显示，不改 AI Sight。旧组件在构造时会被重置且旧网格不再绘制，不能用于调整运行线条位置。

2026-10-29：视线线条默认浅青蓝，`SightDisplayColor` 可调；`bEnableFallTrigger` 默认 false，禁用 C 键请求并隐藏调试跌倒按钮，底层状态函数及动画保留。运行时直接改开关后调试面板要等待状态或选择刷新。

2026-10-29：老人视线显示改为头部位置朝角色方向发出的32条外缘及1条中心线，按 Visibility 简单碰撞逐线裁切，显示期间33ms刷新；忽略老人及附属轮椅。BP_AI 的 Elder|Sight|Visualization 中水平/垂直显示半角默认35/22度，可独立调节，长度沿用 SightRadius；此显示角度与真实 AI Sight 55度参数独立。旧 SightConeMesh 仍提供显隐状态但运行中不绘制，UI继续调用原入口。Showcase PIE验证按钮显示/隐藏；遮挡细节、运动、其余两关及打包尚待验证，不代表真实视觉检测。
2026-10-28：主 UI 路线按钮维持当前可移动姿态（轮椅移动或步行）；切换其他底部按钮时暂停本界面发起且代次仍匹配的路线，重选时恢复同一已暂停样条进度。三关 PIE 状态、暂停两秒位置和恢复状态记录见 `Saved/CodexBackups/MultiMapControl_20261028/RouteMatrix.json`；这不是全部路线可达性或打包验收。

2026-10-27场景接入：`L_ElderWellness_Main`、`L_TundraSanctuary` 各放置一个复用BP_AI的`Runtime_Elder`及本图`Runtime_ElderRoute`，初始轮椅状态，保留当前模型/动画配置。每图使用动态Recast和独立NavMeshBoundsVolume；路线引用不指向Showcase。适老空间起点(0,-950,88.5)，苔原起点(500,-1250,88.5)，屋顶补`ElderControlCeiling`标签。路线实际移动测试不等于全场景可达性、严格贴线或所有家具避碰验收。

主体：`BP_AI / AElderlyAICharacter`；独立原生 `AElderlyAIController`；`ABP_AI / UElderAnimInstance`；每实例 `UElderNavigationComponent`。不需要新建重复老人或重复枚举资产。

## 身份、状态、轮椅

ElderName 每实例独立，空名按性别候选生成。BP_AI及Showcase男女目前都试用laor；SetElderGender 切 Mesh，不清名字/状态。八状态由 EElderState 与 SetElderState 维护，辅助测试函数均转调统一入口。

Falling 的 Timer 按 FallAnimation 长度自动转 Fallen。2026-09-08 BP_AI已设置FallingDown_Laor，ABP_AI_Laor通过原生只读FallPose投影播放非循环动画并保持末帧；直接进入Fallen从末帧初始化。Fallen可经StartGettingUp进入GettingUp，按GetUpAnimation长度回到Standing；显式动画时间保证重复起身从0秒开始。普通Sitting仍未接表现。轮椅分支保留，见下方。

资源槽位：MaleMesh、FemaleMesh、StandAnimation、SitAnimation、WalkAnimation、FallAnimation、FallenAnimation、GetUpAnimation、WheelchairIdleAnimation、WheelchairMoveAnimation。站立/走路目前来自原ABP/混合空间；跌倒、起身和轮椅两动画槽位已接运行表现。轮椅复用ChildActor `BP_WheelChair`；进入/退出切换状态、坐姿与显示，附属碰撞禁用。车架静态网格统一位于`Body` SceneComponent下；左右大轮/小轮保留独立SceneComponent作为转动分组。

## 轮椅车轮滚动

### 手杖附件（2026-09-08）

原老人无运行时手杖组件；新增原生CrutchMesh，复用`Model/Crutch/Xingdongfuzhu04_01`，附着Mesh的现有Crutch插槽，组件相对变换Identity，不改用户插槽。无碰撞/导航影响。状态Owner仍为Character：Falling、Fallen、WheelchairIdle、WheelchairMoving隐藏，其余三态显示；无插槽时隐藏以避免浮在原点。既有状态/初始化/换性别刷新链更新，无Tick轮询。

完整构建Succeeded、BP_AI编译0错误0警告；当时七状态及UI委托轮椅/跌倒入口PIE验证通过，站立握持截图见Saved/Tests/CrutchState_Standing.png，数据CrutchState_PIE.json。GettingUp后续沿用同一显隐链并在起身期间隐藏。不是全部动画手指接触IK验收。备份CrutchState_20260908，用户原脏骨架/地图保存前后均保全。

编辑用零件副本（2026-09-04）：Showcase的`Wheelchair_Extracted`文件夹包含219个独立StaticMeshActor，统一挂在`WC_Parts_Root`（世界位置500,500,0）下的Body/四个轮轴节点；六个层级节点用TargetPoint承载。选根可整体平移，选`WC_Parts_L_BigWheel`等轮轴可绕原轴转动。沿用原Mesh资产，包含用户修改后的网格轴心；不合并/重新居中。此副本没有WheelMotion自动滚动逻辑，既有BP_WheelChair和老人子实例不变。225节点变换/父子关系及四轮45°对照证据：`Saved/Tests/WheelchairExtract_Verified.json`；整体平移后再次验证见`Saved/Tests/WheelchairExtract_Placement.json`。不是新的游戏轮椅系统。

`BP_WheelChair.WheelMotion` 是 `UWheelchairWheelMotionComponent`，保持 `BP_ActorBase` 父类及现有 Body/四个轮组结构。BeginPlay 仅查找本 Actor 的 `L_BigWheel`、`R_BigWheel`、`L_LittleWheel`、`R_LittleWheel`，从轮组子网格的局部包围盒测量半径：当前大轮23.1672cm、小轮11.2002cm，轮轴为局部X，车头为局部+Y。每帧PostPhysics按各轮心世界位移在滚动方向上的投影计算 `转角=距离/半径`，故倒退反转、小轮更快、转弯内外轮差速；从初始相对姿态叠加旋转，不移动车架或改变老人状态。

运行时移动整个轮椅Actor（或它的父对象）即可自动滚动；单独修改Body不会带动轮轴。该组件只做表现，不提供轮椅导航或物理驱动。支持当前正等比缩放；侧移/垂直移动不制造前后滚动，小轮万向转向、非等比/负缩放的精确无滑滚动未实现。

## 坐入轮椅与移动（2026-09-07）

继续使用同一个BP_AI，无需轮椅AI或新的Pawn。调用老人`EnterWheelchair()`进入WheelchairIdle；随后原`ElderNavigation.NavigateToActor`/`FollowSpline`或AI MoveTo驱动同一Character，开始/停止自动切WheelchairMoving/Idle。玩家仍通过原选人、接管及WASD入口驾驶该老人，Tab交还AI；`ExitWheelchair()`恢复Standing。`SetWheelchairMoving`只改状态，不会凭空产生移动目标。

Wheelchair子Actor挂到Mesh的`Socket_Wheel_Chair`（当前为root骨骼Socket），保留用户Socket变换，移除原胶囊下的-90°重复偏移。`WheelchairSocketOffset`默认Identity，`RefreshWheelchairAttachment`在构造、换性别和状态变化时重挂；换模型找不到Socket时隐藏附属轮椅，需配置有效插槽后刷新。轮椅实例无碰撞/导航阻挡，实际碰撞继续使用老人胶囊；不等于精确的宽轮椅碰撞导航。

BP_AI的WheelchairIdleAnimation/WheelchairMoveAnimation均指定已有`Sitting_Idle`。UElderAnimInstance仅提供只读姿势投影，ABP_AI输出前以bool混合原状态机与循环SequencePlayer，0.2秒过渡；不替换AnimInstance，不修改原走路/根转身子图。Idle/Moving切换继续同一坐姿循环；退出保留旧Sequence供淡出，避免参考姿势闪烁。轮椅态屏蔽旧站立TIP淡出的根运动，轮椅转向继续使用CharacterMovement/原程序转向。

按当前模型和Socket测得轮底约低于地面18.14cm，`WheelchairMeshHeightOffset=18.15cm`在游戏世界轮椅态同时抬高人物Mesh和附属轮椅，退出差量恢复，不累计；编辑器站姿预览不应用此运行时高度。换正式模型/Socket/轮椅时须重新标定该值，不能宣称任意车型自动贴地。Sitting_Idle为通用坐姿，尚无推轮手臂动作或手脚IK。

组件的 `Wheels` 保存四个轮组名称和 `RadiusOverrideCm`（0自动测量，正数为未缩放半径）；配置在BeginPlay读取。`SetWheelMotionEnabled` 开关滚动，`ResetWheelMotion(false)` 丢弃旧位移采样，`ResetWheelMotion(true)` 还恢复初始轮角；两个Getter读取测得半径/当前累计轮角。隐藏/禁用后重新显示先重建采样，默认单帧位移超过200cm或转角超过120°视为重新定位；较小的传送应显式Reset。轮组名称或资源缺失会记录一次警告并跳过该轮。

## 移动与根转身

2026-09-09步行调校：DesiredMoveSpeed保留基础64；普通有效速度=基础×WalkingSpeedScale（默认0.8，即51.2），轮椅仍等于基础64。GetEffectiveMoveSpeed供读取，状态刷新与SetDesiredMoveSpeed统一应用。WalkingAnimationRateScale默认1.5：普通步行样本率=基础/ReferenceSpeed×倍率，当前0.6；轮椅底层locomotion投影保留0.4，坐姿独立播放器不改。局部实际速度除以播放率得到混合坐标，近似减小步幅；不是IK/精确防滑。下文64/0.4段落是此前基线，现值以本段为准。

接管移动复用CharacterMovement、ABS_Move和单一`MoveElder`入口。普通老人静止时保留先根转身后走；已经Walking且持续有输入时改变方向则边走边转，不清速度/不进入完整TIP。普通步行新按键/组合按角色当时`GetActorForwardVector×Y + GetActorRightVector×X`锁定目标，移动沿实时Forward。ViewYaw/弃用WheelchairControlYaw只留兼容，不参与计算；鼠标只看。

轮椅手动驾驶（2026-09-07）：同一MoveElder按轮椅状态分支，W/S为实时Actor Forward的正/负推进，A/D为持续Yaw速率；按多久转多久，可超过360°，松键立即停转，不锁90°/180°目标、不留尾转。W+D等组合同时推进/转向，各轴独立限制[-1,1]，数字双键不降速；倒退时A仍左转/D仍右转。`WheelchairTurnRate=90°/s`可调，实际旋转乘DeltaSeconds。W/S从零启动仍保留0.2s持键门槛，A/D立即响应；只转不平移保持WheelchairIdle，不刷状态日志。轮椅不使用普通`bTurnBeforeMove`开关；归还AI恢复既有导航朝向，不改AI MoveTo。

手动旋转权威：静止普通TIP由UE根运动驱动；普通Walking由`UpdateMovingTurn`独占Yaw；轮椅由同一角色Tick的持键分支独占Yaw，两者不同时执行，Movement自动朝向关闭。普通`TurnSpeed=3`是插值响应参数（UI建议2–5），`MaxMovingTurnRate=90°/s`限制大角度转速，两者不改静止根动画播放率。普通转向单帧时间最多50ms以防长帧大跳，低于20fps时会更慢；结束残差<0.05°关闭临时Tick，不补瞬转。轮椅按真实帧时间积分，不用普通50ms限幅。ControllerYaw/DesiredRotation保持关闭；归还AI恢复原导航旋转。关闭`bTurnBeforeMove`只影响普通步行的可选局部平移。

`MoveHoldThreshold=0.2s`保留起步门槛，静止新输入仍经0.12s合并、完整根片段和图表淡出复位。Walking转弯不再等待角度误差达标，可保留角差并持续移动；保持Walking而不反复站立，原ABP继续由实际Velocity驱动。松键立即停平移，但完成最后平滑朝向；尾转中重新按键视为新的静止手势，回到TIP规则。Sitting/Falling/Fallen、Stop、归还AI和轮椅边界切换清转向意图。`bMovingTurn`仅为内部临时旋转阶段，不是另一份老人状态。

2026-09-04输入对照：原`TakeHighestAbsoluteValue`下WD实际同样为(1,1)，不能把该枚举误说成丢轴根因。旧问题在转身提交后丢弃后续目标、以及斜向缓冲中松键清请求；本轮修复意图保存与轻点/持键门槛。IA_Move改Cumulative用于明确求和和W+S/A+D相消，不是重做输入蓝图。

默认 DesiredMoveSpeed 为64cm/s（原160的1/2.5），BP_AI与Showcase实例一致；BeginPlay及接管切换同步CharacterMovement.MaxWalkSpeed。手工覆盖速度仍受支持。ABP_AI保留原ABS_Move和原始LocalVelocity变量；UElderAnimInstance输出 LocomotionPlayRate=DesiredMoveSpeed/LocomotionReferenceSpeed（默认160），局部速度除以该倍率作为混合坐标，8个Walk/Strafe样本接该播放率。64时为0.4，保留原160时的步幅混合；Idle、TIP独立，不整体慢放Mesh或更改全局时间。正式替换走路资源后需重新校准ReferenceSpeed。

ABP 用非循环 BlendSpacePlayer 播 TurnInPlace，分支进入重置；Proxy.PostUpdate 读取实际进度，等待旧片段淡出归零才开始下一段。转角来自 UE 提取的根运动，不能在结束强设朝向，也不能同时手动累加根角。TIP样本角度校准和转速锁定保留；`TurnInputGracePeriod`默认120ms用于普通静止待转目标，合并短暂双键交接，松键可立即提交；普通Walking与轮椅持续转向不等待此缓冲。根动画速度用`TurnAnimationPlayRate=1`，独立于行走`TurnSpeed`和轮椅`WheelchairTurnRate`；普通无动画静止转身继续用`TurnRate=110°/秒`。转身时OrientRotationToMovement关闭，完成恢复；UseControllerDesiredRotation和ControllerYaw继续关闭，AI归还沿用原寻路旋转。细节/历史失败见 [Content 指南](../../Content/CODEX_PROJECT_GUIDE.md) 与 [ADR-004](../decisions/ADR-004-root-turn-playback.md)。

## 样条和目标（已实现）

1. 场景放 `BP_ElderSplinePath`，编辑 `RouteSpline` 点，支持添加/延长与 Closed Loop；Construction 不覆盖手工点。
2. BP_AI 的 ElderNavigation：NavigationMode=Spline，选择该 SplinePath；或 TargetActor 模式用吸管选自己的目标 Actor。
3. 默认 None、bAutoStart=false；勾选自动启动或调用 StartConfiguredNavigation。运行时用 FollowSpline/NavigateToActor 切换，不仅写字段。
4. NavMesh MoveTo 按默认150cm间距采样，默认20cm到达半径；导航可绕障，不是沿曲线强设位置。必须有可达 NavMesh，不接受部分路径假到达。
5. Pause/Resume/Stop 与完成事件属于每实例组件；临时接管自动暂停/恢复，手动暂停不自动恢复。外部 MoveTo 覆盖托管路线，旧排队请求不能抢回。

中间样条段成功时不清速度或切Standing，同一完成回调续接下一段；只对这些段关闭PathFollowing终点停止，解绑/暂停/失败/最终完成恢复正常停止策略。AlreadyAtGoal保留延期避免递归，已到达的中间点不进入引擎清速分支；闭环圈界跳过与Length重复的零点。真正结束先停步再广播完成。仍为NavMesh采样导航，不强行贴曲线或瞬移。

演示 `L_ElderNavigationDemo`，回归 `L_ElderControlTest`。2026-09-03已为Showcase内用户放置的BP_AI和4点样条补 `NavMeshBounds_ElderRoute`，3次有渲染PIE自动完成约11.3m路线并停止，终点水平误差约1.2cm。范围中心(-640,530,100)、半尺寸(1100,1100,250)cm；以后延长到范围外须扩大导航覆盖并重新构建，本次不代表整栋楼或其他楼层均可导航。

## Sight 与显示

真实检测由 AIController.AIPerception + SightConfig；OnPossess 从角色读参数，RefreshSightConfig 应用；提供 OnElderTargetPerceptionUpdated 钩子，不等于已实现复杂避障/社交决策。

视觉用 SM_ElderSight_Forward（由原摄像机线框校正），默认隐藏/无碰撞，从头部位置朝角色水平前方显示；显示半角限制1–85°。显示不产生检测。运行中改变 Sight 配置需同步有效感知和显示；当前无自动双向同步入口，见 E02。

## 已导入的坐姿待机资源（2026-09-04）

`/Game/OldManAnimset/Animations/Sitting_Idle`、`Sitting_Idle_1`、`Sitting_Idle_2` 分别对应下载的三个同名Sitting Idle FBX（无编号、(1)、(2)）。均为30fps的非叠加AnimSequence，使用现有UE4_Mannequin_Skeleton，时长4.3/2.9667/6.4秒，无根位移；用于现有SK_Mannequin。

`/Game/OldManAnimset/Retarget/SittingIdle` 保存精确源参考骨架、三份ExactSource动画、两个IK Rig和RTG_Mixamo_to_OldMan_Sitting。源参考Mesh只有骨架绑定标记，用于重定向，不能用作老人外观。19链使用OneToOne旋转映射；SittingExact目标姿态对齐后保留目标脚部参考角度，避免不同脚骨几何造成下倾；不启用根运动重映射。替换人物或源骨架时须重新验姿态。

资源导入时完成落盘读回、关键骨骼全帧数值/循环接缝检查和动画编辑器预览。2026-09-07已将无编号Sitting_Idle接入轮椅静止/移动分支；普通Sitting及编号1/2动画仍未接状态播放。

## 已导入的跌倒资源（2026-09-07，导入时状态）

`/Game/OldManAnimset/Animations/FallingDown` 对应 `FallingDown.UE.fbx`，为69帧、30fps、2.2667秒的非叠加AnimSequence，使用现有 `UE4_Mannequin_Skeleton`。源参考姿势与目标不同，资源经 `/Game/OldManAnimset/Retarget/FallingDown` 的精确源骨架和19链OneToOne映射烘焙；未启用根运动，水平跌倒位移保留在pelvis轨道。该资源尚未填入 `FallAnimation` 或接入ABP，普通Falling仍按现有Timer逻辑运行。末帧手骨低于名义地面约6–13cm，实际老人蒙皮接地效果需在接入状态后做场景视觉验收。

### FallingDown运行接入（2026-09-08）

后续手穿地修正：仅在FallingDown_Laor烘焙双臂六骨旋转，用两段骨链求解抬起过低手部，局部骨长、root/腿/脊柱和手指原姿态保留；69帧手/指骨最低约6cm，为当前laor蒙皮留出间隙。当前平地PIE双手可见、未穿地，末帧保持正常；不是动态地面IK，不处理斜坡/家具避碰。源UE4 FallingDown仍保留原始数据，运行使用已修正的Laor版本。备份FallHands_20260908；证据FallHands_Correction.json、FallHands_PIE.json、FallHands_Open.png。完整构建Succeeded、BP_AI/ABP_AI_Laor编译0错误0警告，独立审查PASS。下段接入初期穿地描述为修正前记录。

BP_AI.FallAnimation指向`Character/Animations/FallingDown_Laor`，复用已有RTG并应用既有六骨手臂旋转校正；69帧与时长保留，root/腿/头轨道未改。ABP_AI_Laor新增一层Bool混合，False保留轮椅/移动，True使用非循环SequencePlayer；ResetChildOnActivate确保从站立再次跌倒重播。Falling与Fallen共用同一激活分支，直接Fallen的StartPosition为末帧。状态权威和Timer仍在Character，未增加第二套状态系统。

闭编辑器完整构建Succeeded，两蓝图编译0错误0警告；Showcase有渲染PIE委托点击测试验证开始/末帧保持/再次触发/禁止移动/轮椅进出/直接Fallen，骨盆98.47→9.49cm。独立接入范围审查PASS。证据`Saved/Tests/FallingDown_PIE.json`、`FallingDown_Graph.json`和`FallingDown_End.png`；备份`Saved/CodexBackups/FallingDown_Integration_20260908`。这是起身接入前的历史验收；源动画手部低于地面及靠家具穿插仍存在，无环境避碰或手脚IK。

### GettingUp运行接入（2026-09-08）

同日修复：最终 GettingUp Bool 必须将起身 Evaluator 接 `BlendPose_0`（True），原跌倒/轮椅/移动总输出接 `BlendPose_1`（False）。此前两线相反导致普通状态显示起身分支；当起身资源删除时更退化为参考姿势。已纠正、从确切备份恢复缺失 GettingUp_Laor，并重绑类/Showcase实例。运行验证以最终骨骼姿势与位移为准，不能只看状态 Enum、Active bool 或引脚 connected=true。两次起身循环与保存重载通过，见 `Saved/Tests/ElderAnimationRepair_*`。

`GettingUp.UE.fbx`导入为`/Game/OldManAnimset/Animations/GettingUp`，沿用UE4 Mannequin Skeleton；经既有`RTG_UE4_to_Laor`的18/18映射生成`/Game/Project_File/Model/Character/Animations/GettingUp_Laor`。目标动画251帧、30fps、8.3333秒，不启用Root Motion并锁定Root。导入器报告源骨架存在无效Bind Pose并以0时刻姿势重绑；目标骨骼采样及实际laor运行姿势有效，但不等同于全部场景接触视觉验收。

同日复核两个候选FBX：`MixamoToUE5/.../GettingUp.UE.fbx`可直接指定现有UE4 Mannequin Skeleton导入；`Downloads/Getting Up.fbx`缺少该骨架要求的`root`动画轨道，直接导入失败。转换版临时导入后的251帧、时长、骨架及五时刻八骨姿势与现有源动画逐值一致，因此维持转换版及其既有Laor重定向结果，不用原始Mixamo FBX覆盖。对照临时资产已删除；证据`Saved/Tests/GettingUp_Compare_*`，任务前资产备份`Saved/CodexBackups/GettingUp_Compare_20260908`。

状态新增GettingUp并保持追加枚举值以兼容既有序列化。StartGettingUp仅允许从Fallen进入且必须配置GetUpAnimation；期间禁移、隐藏手杖，按动画时长回到Standing。ABP_AI_Laor在原移动/轮椅及跌倒分支上新增非循环SequenceEvaluator，显式GetUpPoseTime由AnimInstance推进，激活首帧归零，避免第二次起身停在上次末帧。老人调试面板原“站立”按钮在Fallen时改为起身入口；轮椅退出语义不变。Showcase现有BP_AI实例与类默认值均绑定GettingUp_Laor。

闭编辑器完整构建`Result: Succeeded`，BP_AI/ABP_AI_Laor编译0错误0警告；18/18链映射、目标资源读回、五时刻骨骼采样和Showcase PIE两轮起身均通过。两轮开始时间都为0，期间CanElderMove=false，结束回Standing且恢复移动；Standing重入和GettingUp重入均拒绝。证据位于`Saved/Tests/GettingUp_*`，构建日志`Saved/Logs/GettingUp_ExplicitClockBuild.log`，精确备份`Saved/CodexBackups/GettingUp_Integration_20260908`。未覆盖斜坡、家具避碰、环境IK、物理鼠标与打包验收。

## laor 模型动画适配（2026-09-07，资源层）

`/Game/Project_File/Model/Character/laor` 保留自己的69骨骼 Skeleton；UE4骨架68骨骼，新模型多8掌骨且缺7个IK辅助骨，手指父级不同，不能直接Assign Skeleton或声明Compatible Skeletons。

`Character/Retarget/RTG_UE4_to_Laor` 使用同目录两个IK Rig、18条对应链与UE4_Aligned目标姿态，Root Motion Op明确source/target root=`root`、target pelvis=`pelvis`。`Character/Animations` 有原OldManAnimset/Animations下42段序列的 `_Laor` 副本；原骨架、动画及ABP未替换。以后新动画可经同一Retargeter转换，不能直接把UE4动画拖给laor。

42段资源加载、时长及根运动启用标记与源对应，资产校验42有效/0错误/0警告；4段代表动画全帧关键骨骼数值正常，坐姿蒙皮预览已核对。资源阶段证据 `Saved/Tests/LaorRetarget_Final.md`；后续接入见下节。材质和PhysicsAsset仍保持导入状态。

### BP_AI试接入（2026-09-07）

后续手臂修正：Character/Animations的42段与Runtime的14段动画已离线校正左右upperarm/lowerarm/hand旋转，目标骨长/缩放与其他轨道保留，资源路径不变。原RTG的自动前臂姿态产生系统肘角偏差；只换OneToOne/清偏移不充分，最终按源component旋转转为目标parent-local烘焙。此流程只针对已检查的同轴手臂，新增重定向动画必须补此步骤并验证，不能以原RTG直接导出当作已修复。

56资产16357帧手臂数值读回有效；五点抽样最大肘角误差由40.50°降至0.0051°，同采样root/腿不变，长度/帧数/根运动标记保留。Showcase实际PIE双手回膝上、行走与转身完成；未提供全动作布料/手指接触IK验收。证据Saved/Tests/LaorArms_Verification.md，原资源精确备份LaorArms_20260907。

额外筛查发现非运行用TIP_R180的源动画第70帧右前臂约81°突跳；目标仍继承此源缺陷，不算全动画平滑通过。运行TurnInPlace继续使用TIP_R180_Alt，勿未经修整改用该问题版本。

BP_AI默认Mesh及MaleMesh/FemaleMesh、Showcase原实例均使用laor；AnimClass为`Character/Runtime/ABP_AI_Laor`，通过原ABP_AI批量重定向保留UElderAnimInstance、嵌入ABS_Move、非循环根转身播放器和轮椅输出分支，非第二套角色架构。原ABP_AI文件未改。

Runtime目录16资产含重定向ABP/BlendSpace及所引用移动/TIP序列。Character的TurnInPlaceBlendSpace与AnimGraph均指向Runtime/TurnInPlace_Laor；两轮椅动画槽位用Character/Animations/Sitting_Idle_Laor。laor新增mesh-local Socket_Wheel_Chair复制源root插槽变换，保留18.15cm轮椅补偿；head视觉挂点未改。

有渲染PIE核对站姿、轮椅切入退出、行走足部变化、根转身结束与性别切换后实例兼容；BP_AI/ABP_AI_Laor编译0错误0警告，Runtime16资产校验通过。不是完整动作/脚底或脚踏IK验收；通用坐姿脚踏接触仍需精调，男女暂同一外观，缺失普通坐下/跌倒表现未补。证据`Saved/Tests/LaorBPIntegration_Verification.md`。
## 2026-10-27：主界面路线按钮

底部第三项对当前选中老人的已有 `ElderNavigation` 调用配置好的样条路线；若正被玩家接管，先归还原AI控制器。无选中老人、无有效样条或状态不允许移动时不会生成临时路线。重复点击重新从路线起点请求，非暂停。Showcase PIE验证归还AI及两次实际位移，证据 `Saved/CodexBackups/TargetNavLabels_20261027/route_camera.json`。


### 2026-10-27：LivingGrove 样条验证

`L_LivingGrove_Social` 加入一名复用 BP_AI 的老人、现有 BP_ElderSplinePath 和导航边界，路径沿中央通道从 Y=-3000 至 -900；重启后 PIE 记录走完全程 2100 cm，状态为 Completed，且无预选情况下可触发切换和路线重播。此项不代表所有行走区域或打包版本均已验收。
