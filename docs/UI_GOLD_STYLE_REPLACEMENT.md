# 主 UI 暖金换肤记录

更新：2026-09-07。**暖金换肤已完成并通过1920×1080限定范围独立审查。** 原 `UBP_用户界面` 继续作为主 UI；没有新增第二套主界面。用户同意继续后，仅将主标题 `TextBlock_128` 从X=53移到X=118，解决新Header左端房屋图标重叠；其余标题Slot字段和所有模块布局不变。

## 范围与回滚

- 仅换现有 UI 的框体、按钮 Brush、装饰图标与颜色；保留原功能对象、业务状态 Owner、绑定、事件与布局。未为换肤修改 3D 场景材质、模型或相机配置。
- 修改前精确备份：`Saved/CodexBackups/GoldStyle_20260904`，包含15个原 UBP、`line-stack.html`、老人控制 Widget 源码和配置；旧 Texture 未覆盖或删除。保留当前未提交版本，不以 Git 旧提交替代用户当前工作。
- 布局/属性/图基线：`Saved/GoldStyle/Baseline`；逐控件审查见 [UI_BEFORE_AUDIT.md](../Saved/GoldStyle/UI_BEFORE_AUDIT.md)。整包恢复前必须再次检查后续用户改动，不能盲目覆盖。
- 新纹理统一导入 `/Game/UI/NewGoldStyle/`，`ImportManifest.json` 的28条均为 `replaceExisting=false`。已有 C++ 老人面板仍只创建一个实例，没有另建老人控制 Widget Blueprint。

## 实际素材来源与28项映射

先审查用户 `Image` 目录和三批素材，实际选用以 [ImportManifest.json](../Saved/GoldStyle/ImportManifest.json) 为准，而非随机名称推断。当前19项来自用户补充的候选素材目录、9项来自第三批 ZIP 解压目录；不将候选图误写成第一/第二批原图。

- **A**：`output/UI_Reference_Assets_20260904/assets/`，顶部使用用户明确指定的 `Top_Header_Frame.png`。
- **B**：第三批 `UI_Assets_Batch3.zip` 解压到项目外临时目录 `C:/Users/Administrator/AppData/Local/Temp/GoldUI_AssetAudit_20260904/UI_Assets_Batch3/`。

表内 Widget Blueprint 均位于 `/Game/Project_File/BP/UBP/`。纹理资产名均加前缀 `/Game/UI/NewGoldStyle/`。

| Texture | 来源 PNG | 原有承载控件 |
| --- | --- | --- |
| T_UI_Header | A / Top_Header_Frame.png | UBP_用户界面 / Image_118 |
| T_UI_Exit | A / Exit_Button.png | UBP_时间框 / Button_2 |
| T_UI_Location | A / Location_Frame_Empty.png | UBP_地点 / Image_57 |
| T_UI_Monitor | A / Left_Icon_Monitor.png | UBP_小菜单 / Image_326，原 B_摄像机视角 内 |
| T_UI_Alert | A / Left_Icon_Alert.png | UBP_小菜单 / Image_208，原 B_报警灯 内 |
| T_UI_FirstPerson | A / Left_Icon_Location.png | UBP_小菜单 / Image_92，原 B_人称切换 内 |
| T_UI_SOS | A / Left_Icon_SOS.png | UBP_小菜单 / Image，原 B_紧急呼救系统 内 |
| T_UI_ElderDropdown | A / Dropdown_Elder_Empty.png | UElderDebugControlWidget / ElderPicker |
| T_UI_ActionBlank | A / Button_TakeControl.png | UElderDebugControlWidget / ToggleElderSightCone、TakeControl |
| T_UI_ReturnObserve | A / Button_ReturnObserve.png | UElderDebugControlWidget / ReleaseControl |
| T_UI_SpacePanel | A / Panel_SpaceDescription_Empty.png | UBP_右侧栏 / IMG_SpaceFrame |
| T_UI_WeatherPanel | A / Panel_Weather_Outer.png | UBP_天气选择 / Image_237 |
| T_UI_WeatherSunny | A / Button_Weather_Sunny.png | UBP_天气选择 / Button_SunnyDay |
| T_UI_WeatherCloudy | A / Button_Weather_Cloudy.png | UBP_天气选择 / Button_CloudDay |
| T_UI_WeatherRainy | A / Button_Weather_Rainy.png | UBP_天气选择 / Button_RainDay |
| T_UI_TimePanel | A / Panel_Time_Outer.png | UBP_时间选择 / Image_237 |
| T_UI_TimeMorning | A / Button_Time_Morning.png | UBP_时间选择 / Button_Moon（既有早晨按钮名） |
| T_UI_TimeNoon | A / Button_Time_Noon.png | UBP_时间选择 / Button_Noon |
| T_UI_TimeNight | A / Button_Time_Night.png | UBP_时间选择 / Button_Night |
| T_UI_LevelPanel | B / Panel_Level_Outer.png | UBP_当前区域 / Image_237 |
| T_UI_Level01 | B / Button_Level_1.png | UBP_当前区域 / Button_45 |
| T_UI_Level02 | B / Button_Level_2.png | UBP_当前区域 / Button |
| T_UI_Level03 | B / Button_Level_3.png | UBP_当前区域 / Button_1 |
| T_UI_EnergyPanel | B / Panel_Energy_Base_NoLine.png | UBP_能耗曲线 / Image_237，原 WebInterface_116 仍在上层 |
| T_UI_ToolAim | B / ToolButton_Aim.png | UBP_缩放 / BTN_Move |
| T_UI_ToolPlus | B / ToolButton_Plus.png | UBP_缩放 / Button_67 |
| T_UI_ToolMinus | B / ToolButton_Minus.png | UBP_缩放 / Button |
| T_UI_ToolFullscreen | B / ToolButton_Fullscreen.png | UBP_缩放 / Button_434，仍调用原收展接口 |

28个文件均可找到；没有缺失而临时生成的新图。通用空白操作图复用于视线开关和接管，两按钮仍独立。A/B中的图片本身是独立组件，不是将整屏合为单图。

导入读回：28/28已保存，UI组、`TC_EditorIcon`（UI RGBA）、sRGB、NoMipmaps、Clamp X/Y、NeverStream，`CompressionNoAlpha=false`；见 [TextureSettings.json](../Saved/GoldStyle/TextureSettings.json)。原 C++ 的三张纹理通过运行时 `LoadObject` 加载，现有 `Config/DefaultGame.ini` 已将该专用目录加入 `DirectoriesToAlwaysCook`，防止遗漏；这不是打包验收通过。

## 布局与控件保持

直接换肤10个既有 Widget：`UBP_用户界面`、`UBP_时间框`、`UBP_地点`、`UBP_当前区域`、`UBP_小菜单`、`UBP_右侧栏`、`UBP_天气选择`、`UBP_时间选择`、`UBP_能耗曲线`、`UBP_缩放`。另把未改的 `UBP_左侧栏` 纳入结构对照。

独立 [LayoutReview.md](../Saved/GoldStyle/LayoutReview.md) 当前结论为**结构范围 PASS**：11个 Widget / 162个控件的名称、类、父子和顺序保持；151个 Slot 的位置、尺寸、锚点、对齐、Padding、ZOrder、AutoSize及关联保持。1920×1080设计空间、RootCanvas → ScaleToFit → SizeBox → 原Canvas、Font、Text、自动换行、Clipping和渲染变换未变；新 Brush 的 ImageSize 保持旧值，不取原 PNG 像素尺寸作为 Desired Size，Button Padding不变。

参考外部模块：顶栏 `(14,16)/1889×79`，左侧栏 `(21,116)/303×868`，右侧栏 `(1349,127)/552×897`，工具条 `(1270,484)/92×249`。右侧四框原屏幕 Y为127/359/497/647，间距4，顺序保持。此为本轮换肤基线，不恢复最初参考图的旧坐标。

原生老人面板固定 Slot 仍为 Toggle `(24,180)/340×42`、Picker `(24,228)/340×36`、Take `(24,270)/164×36`、Release `(200,270)/164×36`、Status `(24,312)/340×70`；只在既有 RebuildWidget 设置 Style/文字颜色，未改 Owner、选项数据、事件或层级。

## 隐藏的旧视觉与保留的动态内容

没有删除控件。已有重复视觉以 Hidden 停用，Button、动态内容和选中层保留：

| 区域 | 停用旧视觉 | 保留 |
| --- | --- | --- |
| 退出 | ExitContent 中旧退出文字及绘制图标 | 原 Button_2 / QuitGame |
| 地址 | Image_75、ArrowLeft、ArrowRight | 原地址 TextBlock_110 |
| 关卡 | 当前区域文本、TXT_Level1/2/3 | 原3个 Button、VerticalBox、间距及现有事件 |
| 空间说明 | TXT_SpaceTitle、旧 PanelFill/TitleRule/TitleAccent | TXT_SpaceBody 的内容、字体、换行和 Slot |
| 天气 | 旧标题、Icon_0/1/2和3个文本、旧装饰 | 原按钮、HorizontalBox、Content及Selected覆层，原 BGS_State 链 |
| 时段 | 旧标题、太阳/月亮/地平线拼图和3个文本、旧装饰 | 原按钮、HorizontalBox、Content及Selected覆层，原 BGS_State 链 |
| 能耗 | 旧标题、TXT_Unit、PanelFill/TitleRule/TitleAccent | 原 WebInterface_116、数据接口和实时 Canvas 绘制 |
| 工具条 | Glyph_1/2、IconCanvas_0/3 | 原4个 Button / 原调用入口 |
| 老人下拉 | 原箭头仅透明，不移除占位 | PNG箭头、动态姓名、Options、OnOpening、OnSelectionChanged |

9个原 Collapsed 装饰已恢复 Collapsed。61个停用旧视觉另设 `bHiddenInDesigner=true`，消除设计器预览的双文字/图标，运行 Visibility、布局和业务不变；两个 HorizontalBox_219 均保持显示。装饰 Image 和选中覆层为 HitTestInvisible，不新增阻挡点击层。

主标题、每秒日期时间、地址、空间正文、老人姓名/提示及接管状态消息均仍为原独立文字，未烘焙替代。PNG自带文字的模块使用图中文字，避免双标题/双图标/双箭头。顶部天气仍是原静态图标，不宣称已同步天气状态。

## 状态、交互与能耗

- 天气/时段仍由 BGS_State 决定。为使新 Button Brush 也随原选中变量减亮，`RefreshSelectionVisuals` 的6个表现目标由旧 Content 改为原 Button；比较、SelectedIndex及业务链不变。选中/未选中整按钮透明度1/0.78，原Selected覆层1/0；没有第二份权威状态。
- SOS仍调用 `BP_alarm.Light_Toggle`，默认关闭、再次点击取消；原摄像机/报警/定位动画与1.01倍反馈不变。老人操作继续转发 Controller，UI不拥有老人状态。
- 工具 `+/-`、原收展接口、退出及其他绑定不重写。**原关卡 EventGraph 为空，地区/Move没有完整点击业务链**，本次只保留其现状，不能将素材换上等同实现了关卡切换/地址选择/移动模式。
- 关卡一源PNG自带较亮金边，原关卡Widget又没有Selected状态Owner；这是素材与基线限制，不是“关卡一已选中”的运行证据，关卡单选验收未通过。未为换肤捏造关卡切换逻辑。
- 能耗仍为原 `UBP_能耗曲线.WebInterface_116`，Slot `(16,58)/520×303`。`line-stack.html` 保留 `ue.interface.ue2js`、electric/values/water/labels、输入校验、空态、动态Y范围、原plot坐标、ResizeObserver与鼠标提示。默认25小时点是示例，不是实际设备数据；本次不新建数据源。
- 仅将线、点、面积、网格、提示改暖金色。为不让NoLine中的静态坐标与真实动态坐标重复，网页在原范围内使用不透明暖棕渐变背景 `#29241d → #18130d`，底部12px圆角仅作用于不参与命中的背景层，不裁Canvas或改变坐标。动态轴必须保留以兼容大于100/自定义标签/旧双序列。

## 当前验证与未完成项

| 证据层级 | 当前结论 / 限制 |
| --- | --- |
| 原生构建 | [Build_Succeeded.log](../Saved/GoldStyle/Build_Succeeded.log) 为本次完整 Editor 构建，`Result: Succeeded`；不代替PIE或打包 |
| Texture导入 | 28/28保存成功、导入设置已读回；并不单独证明实际渲染完整 |
| Blueprint编译/保存 | [FinalCompile.json](../Saved/GoldStyle/FinalCompile.json)：10/10编译保存成功、0失败；时间框1条原有Reset Create Main UI警告，其余0。后续仅Designer隐藏标记修改的8个Widget再次编译保存成功，主Widget再编译0错误0警告；无未保存包 |
| 结构独立审查 | PASS，限上述11 Widget /162控件/151 Slot；图19张/292节点无增删，仅6个表现目标改变；历史基线无完整wire endpoints，不能声称全部历史接线二进制等价 |
| HTML数学/数据对照 | 23组Node与换肤前精确备份对照PASS：默认、空/单点/三点、>100、自定义标签、12月electric+water、JSON/values、错误数据、提示及DPR/尺寸变化；模型、返回值、绘制坐标和刷新一致；与下项UE运行验证分开计 |
| 实际PIE图表输入 | 主任务经原WebInterface.Execute/ue2js输入electric=[20,140,30]、labels=[A,B,C]，三点曲线、Y轴140和自定义ABC正常；再发空数组显示“暂无能耗数据”且没有烘焙旧轴，最后恢复原数据。截图 [Dynamic3Points](../Saved/GoldStyle/Gold1920_Dynamic3Points.png)、[DynamicEmpty](../Saved/GoldStyle/Gold1920_DynamicEmpty.png) 均为1920×1080 |
| 真实鼠标交互 | [PhysicalTests.json](../Saved/GoldStyle/PhysicalTests.json)：6个天气/时段按钮、SOS开关、老人选择与接管、工具+/-通过（TargetOrbitDistance 2250→2500）；Tab退出接管回原BP_Player及原位置/朝向；Return仅观察态物理点击验证，未物理按住Alt跨模式返回。全屏图标实际执行原UI收展，两次点击恢复；Exit物理点击结束PIE。关卡/地址/Move实际点击无完整业务反馈，不能授业务PASS |
| 1920×1080视觉 | [Gold1920_Current.png](../Saved/GoldStyle/Gold1920_Current.png) 为重开PIE后的完整初始观察视角1920×1080；绘图区渐变/一套坐标已获局部视觉认可；老人面板/下拉展开截图获局部视觉PASS。测试天气/时间和缩放状态已通过重开PIE复原，没有保存3D场景改动 |
| Designer预览 | [GoldDesigner1920.png](../Saved/GoldStyle/GoldDesigner1920.png)：Custom1920×1080设计区完整显示，已消除旧隐藏装饰在Designer中的双文字；实际整编辑器截图2560×1440，不是2K PIE验收。WebUI在Designer显示占位，不代替运行曲线证据 |
| 整体独立视觉审查 | **PASS（限定范围）**：最终 [Gold1920_TitleFixed.png](../Saved/GoldStyle/Gold1920_TitleFixed.png) 中房屋与标题不再重叠，窄缝合理，标题未裁切且未碰中央叶饰；资产只读确认仅标题Left 53→118，详见 [VisualReview.md](../Saved/GoldStyle/VisualReview.md) 与 [HeaderTitleChange.json](../Saved/GoldStyle/HeaderTitleChange.json) |
| 其他分辨率/打包 | 2K新窗口尝试被当前桌面钳制为2554×1402，内部16:9设计区保持比例并留左右安全边距，但不是原生2560×1440；4K和打包未验收，不沿用旧版测试背书 |

`Before1920.png` 与 `FirstGold.png` 实际为1920×1082，仅作同尺寸中间对照；`Gold1920_Working.png` 是启动过早的局部首帧，已弃用，不作为最终布局证据。稳定完整截图已补齐；旧失败及修复记录保留，不隐藏测试边界。

当前已停止PIE、恢复1920预览设置，目标UI包均已保存；只有用户原有 `BP_AI` 保持未保存且未触碰。顶部修复前磁盘状态和保存后/移动前状态分别备份在 `Saved/CodexBackups/GoldHeaderTitle_20260907`。原关卡状态与缺失业务入口仍作为明确限制；限定PASS不扩展到打包、原生2K/4K、非16:9全流程或缺失业务。
