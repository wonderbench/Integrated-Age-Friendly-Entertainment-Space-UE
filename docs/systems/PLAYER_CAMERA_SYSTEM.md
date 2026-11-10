# 玩家、相机与临时接管


2026-10-30：第二关三个 EW_Pendant 吊灯网格加 `ElderControlCeiling`，跟随天花板在观察时隐藏、接管老人时显示；独立 RectLight 保留。保存重载及 PIE 按钮事件验证三个网格显隐切换。
2026-10-28：观察模式鼠标旋转经 Controller 的 `ObservationOrbitInputScale`（默认 0.5）统一缩放，Slate、Viewport 和 InputComponent 复用相同入口；`OrbitYawSpeed` 和 `OrbitPitchSpeed` 保持原值。老人接管鼠标视角和滚轮缩放不经过该倍率。三关 PIE 等输入比较见 `Saved/CodexBackups/OrbitSpeed_20261028/SpeedPIE.json`；实际鼠标手感仍需检验。

2026-10-28：`Showcase`、`L_ElderWellness_Main`、`L_LivingGrove_Social` 各配置一个无碰撞且带 `OrbitCameraBounds` 标签的轴对齐 TriggerBox。观察相机以其水平包围盒限制镜头中心，保留俯仰和原目标距离；位置处于框外的旋转中心会夹到框内。它不是碰撞或遮挡检测，近裁剪面和不规则墙体仍需视觉验收。第二关额外28个顶棚、风口和灯具模型加 `ElderControlCeiling`，沿用既有接管显隐；真实灯光不受影响。

2026-10-27底部导航入口：从主UI视角按钮接管时保留可见光标和GameAndUI，老人视角无捕获时由Slate焦点内鼠标移动驱动，避免重复鼠标轴；视线按钮及Alt释放不再强制隐藏该模式光标，再按视角按钮恢复原观察Pawn。此前其他显式接管入口仍使用原GameOnly/Alt行为。自动与实际点击测试范围见`Saved/CodexBackups/ElderNavButtons_20261027/REVIEW.md`。

Owner：`BP_Controller → AOrbitCameraController`；默认 Pawn：`BP_Player → BP_Character_Base → AOrbitCameraCharacter`。源码分别在 `Source/Old_Man_Jiang/Private/OrbitCameraController*.cpp` 与 `OrbitCameraCharacter.cpp`。

## 现状与契约

- Blueprint 创建原 `UBP_用户界面`；原生控制器还提供老人测试控件。后者是已有调试入口，不是可任意新建第二套 MainUI 的理由。
- `SelectedElder` 管选择，私有 `ControlledElder` 管实际接管，`ObservationPawn` 保存原观察对象。不要用选择变化暗中切换 Possess。
- TakeControlOfElder 接管同一老人；AIController/Sight/路线暂停；ReleaseElderControl 恢复原观察 Pawn 和 AI。老人姓名/状态/动画实例不复制成替身。
- 接管时 WASD 移动、鼠标环绕/俯仰、不需按右键；Tab 返回。看镜头不直接转身体，移动转向由现有角色逻辑决定。
- WASD复用原`IA_Move`（Axis2D），`IMC_ElderControl`的Swizzle/Negate映射不变；Cumulative合成双键、相反键相消。Controller仅转交二维向量，兼容参数ViewYaw传0且不参与移动，见[老人系统](ELDER_AI_SYSTEM.md)。
- 普通步行新按键/组合按当前Forward/Right锁目标，Walking边走边平滑转弯、静止轻点保留TIP；松键停平移但完成普通尾转。轮椅在同一老人入口解释为W/S沿实时Forward前进/倒退、A/D持续左右转，松键停转，组合边走边转；不再锁固定角度。两模式均不用相机或接管时缓存，Controller仍仅转交原二维输入。
- 接管时按住左/右 Alt 临时显示光标并切换 GameAndUI/NoCapture，可点击原 UI；松开最后一个 Alt 恢复隐藏光标、GameOnly 和鼠标视角。Alt 操作 UI 期间暂停移动输入与鼠标转向，不改变老人状态 Owner 或 AI 接管链。
- 临时光标状态由 Controller 的 `OrbitCameraController_CursorInput.cpp` 维护；`IsElderUICursorActive` 是只读展示入口。Slate 键释放路由覆盖 UMG 按钮拿走焦点的情况；输入预处理器仅在临时光标/待恢复期间校验游戏焦点，切到其他窗口不主动抢回焦点，回到游戏后协调丢失的 Alt 松键。退出接管/UnPossess 清除锁存，不影响观察模式光标。
- 观察模式右键拖拽与接管自由视角是两个分支；接管中松右键不能重置为 UI/NoCapture。
- 全局 Slate 输入预处理器只允许处理游戏视口路径；不得干扰编辑器蓝图/材质图右键拖动。Enhanced Input/视口备用路由不可未经测试同时删改。

## 接管时的天花板显隐

- `AOrbitCameraController` 以实际 `IsControllingElder()` 为准，在 BeginPlay、Possess/UnPossess 和老人销毁时同步天花板；EndPlay 隐藏。返回失败而仍接管老人时保持显示，选中对象变化不会改变显隐。
- `Showcase` 中用户的 `GroupActor_4` 共115个静态模型（67块顶板、48个SM_DownLight），已添加 Actor Tag `ElderControlCeiling`；原分组、初始游戏隐藏、编辑器隐藏与碰撞设置保持。运行时只使用标签，不依赖仅编辑器存在的 GroupActor/ActorLabel。
- 只切换这些 StaticMeshActor 的游戏隐藏，不修改 BP_LightBase_Box、天气接口、灯光强度、组件可见性、碰撞或导航。当前单机方案为全场景游戏隐藏，不是仅玩家视图遮罩，因此 SceneCapture 也遵循同一显隐。
- 后续新增顶板/灯具模型须给对应 Actor 添加同一标签，单纯加入编辑器组不会自动同步。运行中新增或重新标记后调用 `RefreshElderCeilingVisibility()`；无需 Tick 查询。标签可在 BP_Controller 的 Elder/Ceiling 配置。

## 扩展与验证入口

视线按钮通过 `ToggleSelectedElderSightCone` 统一入口，下一帧在仍接管老人、未按住 Alt 且焦点仍属于游戏时恢复 GameOnly/视口捕获；不抢编辑器或外部窗口焦点。鼠标点击回包处理完后再恢复，避免按钮重新持有键盘焦点。未改动移动、转向或 AI 导航。

新增选人/按钮通过 ProjectUserWidgetBase 与 Controller API；选择列表允许按需查询，不改成每帧寻人。实际控制以 Controller/Possession 为准，UI 缓存只展示。

本地拥有权限的控制器方案；无联网接管 RPC。测试覆盖 CameraLook/Takeover，并联跑 AdjacentTurn、TurnPlayback；还需人工 PIE↔编辑器图表焦点切换。见 [TESTING](../TESTING.md)。

### 2026-10-27：老人视角鼠标 Y 方向

底部视角按钮进入老人接管时，NoCapture 的 Slate 鼠标 Y 输入方向已反转；观察相机分支及其他接管路径未改。完整关闭当前项目编辑器后构建成功、重启；运行窗口中实际注入鼠标上下各 100 px，Pitch -5.025 → 12.475 → -5.025。未据此推断其他输入模式或所有设备的方向。
