# 参考界面复原记录（2026-09-03）

## 2026-09-04：右侧紧凑间距（用户新布局要求，独立复审PASS）

最终独立复审PASS：坐标、尺寸、组中心及收展截图符合本轮要求；按钮命中X1295–1337，未与右栏重叠。PIE停止、无脏包。下段“待回”为记录当时状态，已由本结果取代；不代表物理点击或高分辨率复测通过。

工具条MainUI槽X1240→1270，Y484及92×249不变；贴图可见右缘距右侧面板约8px（透明边缘导致可见间隙不同于槽）。四模块宽552、高228/134/146/377保持，屏幕Y为127/359/497/647，三个布局间距均4px；组上下界127/1024，中心575.5与原112/1039相同，向中间收拢。MainUI右侧栏容器552×897；子槽Y为0/232/370/520。此新要求取代下方旧坐标表，不再将原12/15/15px间距作为当前目标。

只修改UBP_用户界面、UBP_右侧栏的5个Canvas槽，不改纹理、正文、按钮、图表业务或动画轨道。备份Saved/CodexBackups/UICompactSpacing_20260904。两蓝图编译保存0错误0警告；1920截图CompactSpacing_1920_Final.png、CompactSpacing_Collapsed.png、CompactSpacing_Reexpanded.png位于Saved/Screenshots。通过原Button_434委托触发收起/展开，末态完整、工具条未出屏；展开后运行几何X1270/Y484、变换归零、裁切诊断0。此为运行布局回归，不是物理鼠标/2K4K/打包验证。独立审查待回。

## 2026-09-04：空间说明原图恢复

用户指定15_53_29确为原空间说明素材。此前T_UI_InfoPanel被预处理清空内部、由材质补底，造成不匹配；本次保留原内部渐变、内框、青色装饰，只局部去烘焙标题及裁透明边。原图未写入，能耗不改。精确备份Saved/CodexBackups/InfoRestore_20260904。

仅现有UBP_右侧栏调整：停用PanelFill/TitleRule/TitleAccent；IMG_SpaceFrame沿用T_UI_InfoPanel，Box边距0.065/0.20/0.065/0.12，外框552×228不变。独立标题内距35/21；正文32/72、488×123、字号10、换行488，保留全文。首图文字压边，调整后最新1920截图完整显示且不压装饰。

Texture保存成功stillDirty=false；右侧栏编译保存0错误0警告。实际PIE截图Saved/Screenshots/InfoRestore_1920_Final.png（10:42:48）。PIE已停止，未改逻辑或进行本轮按钮交互/2K4K复测；本任务没有保存或丢弃其他包的操作。独立复审PASS仅覆盖本轮空间说明素材修正，确认导入哈希、属性、全文安全区及能耗未改；未重启重载，不代表原始全任务验收通过。最后实时dirty为空，不推断其他包状态改变的原因。

## 2026-09-04：原素材恢复与验收补测（局部修正落盘，整体仍有边界）

续跑结果：用户确认无其他任务后，停止PIE并重导最新PNG，两Texture保存success=true/stillDirty=false，Weather99851字节、Time99162字节，导入MD5与Prepared一致；两选择Widget编译0错误0警告。未保存/覆盖灯、轮椅及MainUI的其他工作。前述PIE中保存失败已解除，保留以下过程事实供追溯。

最终截图改用Saved/Screenshots/UISourceRestore_1920_Final.png（实际PIE），UISourceRestore_2560_Final.png、UISourceRestore_3840_Final.png（新建临时WidgetComponent独立渲染；原生尺寸UI-only，非场景/窗口，非旧图复制；顶部时钟10:26:10/10:26:33）。临时Actor已销毁。4K正文换行和离屏混合颜色仍不同，不能称严格像素一致。空间/能耗仍是上一轮材质近似，尚未恢复这两原图内部。

补完主要入口鼠标点击：三关卡有hover反馈但不切地图；+/-确实缩放场景；Move仅提示右键旋转；收展按钮实际收起再展开，动画结束布局完整恢复；地区箭头不展开；Exit实际结束PIE。六天气/时段及Camera/Alarm/Person证据保留上轮，SOS业务用户豁免，不冒称全部业务可用。结构/状态证据Saved/UISourceRestore_FinalVerification.json。最终PIE停止，dirty仅灯、轮椅、MainUI；不是全项目clean，也未重启编辑器来丢弃这些改动。

独立复审PASS仅覆盖天气/时段原素材恢复及持久化：两源MD5与导入记录一致、目标包非脏、Widget引用和衬底停用正确、最新截图无重复标题或按钮越框。未独立比较备份蓝图接线，未重启重载，不代表整个原任务验收通过。

### 前一阶段记录（以下待办状态已由上述续跑结果更新）

用户指出正确：天气15_53_43和时段15_54_01原PNG内部均有渐变与内框；空心Prepared是此前处理脚本造成，不是源素材缺失。本次Prepared恢复原填充、装饰与内框，仅在标题小区域按同行背景插值移除烘焙文字；保留独立TextBlock。原始PNG未写入。天气/时段的PanelFill、TitleRule、TitleAccent停用，Image_237九宫格上下边距调整以保留原头部/底边。空间说明/能耗尚使用上一轮材质近似，不能称所有原图均已恢复。

精确备份Saved/CodexBackups/UISourceRestore_20260904；脚本Saved/PrepareUIReferenceAssets.ps1新增Only过滤，此次仅生成WeatherPanel/TimePanel。两Widget编译0错误0警告。重要：两Texture重导成功但当前PIE期间保存返回false/stillDirty；Prepared已落盘，Texture包持久化与重启读回尚未完成。

补测证据：UISourceRestore_1920.png为实际1920×1080 PIE截图；UISourceRestore_2560.png和UISourceRestore_3840.png分别为UE WidgetComponent在2560×1440/3840×2160独立绘制的完整UI，非放大1920图、非静态拼图；仅用于布局核验，不包含3D场景，不冒充高分辨率桌面游戏窗口验收。临时PIE测试Actor已销毁，无新增Widget Blueprint。三图均早于最后标题背景插值微调，最终截图仍需刷新；离屏图表混合/颜色和实际PIE不同，不能据此证明颜色像素完全一致。

实际鼠标：阴/雨/晴、正午/晚上/早晨六按钮均经sky.click，互斥亮度及BGS_State读回正确，记录Saved/UISourceRestore_PhysicalTests.json（阴天是已观测结果摘要，其余保留完整读回）。摄像头/警报实际点击有高亮，定位实际展开/收起原选人面板，圆形无重叠/裁边。没有把委托调用冒充物理点击。三关卡、缩放工具、地区与退出本轮物理验收仍待继续；SOS业务按用户要求不补。

测试中检测到外部窗口输入，预览被切到编辑器，轮椅与主UI出现其他未保存改动；已暂停窗口操作并询问是否另一个任务正在使用同编辑器，不强停PIE或覆盖其他工作。独立初审发现天气标题背景采样碰到青色斜杠，已缩小去字范围并重新生成Prepared，横线消除；最新PNG仍待再次重导、保存与最终截图。原长任务不能宣称全部完成。

## 修改范围

在原 UBP_用户界面上重构，没有新建第二套 MainUI，也没有新增 Widget Blueprint 或 C++。创建入口保持 BP_Controller.BeginPlay → CreateWidget → AddToViewport。

修改的 11 个 Widget（均在 Content/Project_File/BP/UBP）：UBP_用户界面、UBP_左侧栏、UBP_右侧栏、UBP_小菜单、UBP_当前区域、UBP_地点、UBP_天气选择、UBP_时间选择、UBP_能耗曲线、UBP_缩放、UBP_时间框。

主层级：RootCanvas → DesignScale（ScaleToFit）→ DesignSize_1920x1080（固定尺寸）→ 原 CanvasPanel_38。Designer 使用 Custom 1920×1080。组件内部以原生 Canvas/Overlay/HorizontalBox/VerticalBox/Image/Button/TextBlock 组成，选中覆盖层拉满各自按钮，不单独重复硬编码宽度。

## 1920×1080 布局基准

| 模块 | X | Y | W | H |
| --- | ---: | ---: | ---: | ---: |
| 顶栏 | 14 | 16 | 1889 | 79 |
| 地区 | 21 | 116 | 280 | 47 |
| 摄像机 | 29 | 198 | 90 | 90 |
| 警报 | 29 | 289 | 90 | 90 |
| 定位/选人 | 29 | 380 | 90 | 90 |
| SOS | 29 | 471 | 90 | 90 |
| 关卡框 | 24 | 593 | 300 | 391 |
| 空间说明 | 1349 | 112 | 552 | 228 |
| 天气 | 1349 | 352 | 552 | 134 |
| 时段 | 1349 | 501 | 552 | 146 |
| 能耗 | 1349 | 662 | 552 | 377 |
| 工具条 | 1240 | 484 | 92 | 249 |

右栏子模块只保存相对位置；四框共用父栏宽度552。天气/时段以HorizontalBox三等分，真实按钮162.667×58，间隔16；全部位于框内。三个关卡按钮共用同一Texture，256×80，间隔20。布局以UMG内的Designer值为权威，本表是审查记录，不是第二份运行状态。

## PNG 对应与导入

原图均位于 Material/ChatGPT Image 2026年9月3日 <时间>.png。先实际看图，原文件未覆盖。导入目录：/Game/Project_File/BP/UBP/Textures/ReferenceUI。派生图放 Material/Prepared；Saved/PrepareUIReferenceAssets.ps1 可重复处理已核对的14个源文件，逐组件裁透明边、等比缩小、提取图标、清理框内烘焙内容及暗色矩形底，未生成整屏合成图。

| 时间 | Texture（T_UI_前缀） | 使用 |
| --- | --- | --- |
| 15_52_45 | LevelButton | 三个关卡按钮共同背景 |
| 15_52_51 | LevelPanel | 左下外框 |
| 15_52_58 | SOS | SOS圆形视觉 |
| 15_53_03 | Location | 定位/选人圆形视觉 |
| 15_53_09 | Alarm | 警报圆形视觉 |
| 15_53_19 | Camera | 摄像机圆形视觉 |
| 15_53_25 | EnergyPanel | 能耗外框，九宫格 |
| 15_53_29 | InfoPanel | 空间说明外框，九宫格 |
| 15_53_43 | WeatherPanel | 天气外框，九宫格 |
| 15_53_53 | Sunny、IconSunny | 整图保留导入，界面仅使用独立太阳图标 |
| 15_53_57 | Cloudy、IconCloudy | 整图保留导入，云图标保持36×22.65比例 |
| 15_54_01 | TimePanel | 时段外框，九宫格 |
| 15_54_05 | Rainy、IconRainy | 整图保留导入，界面只使用图标、没有双箭头 |
| 15_54_12 | WideButton | 备用导入，未作为天气/时段整图使用 |

共17个Texture。大型源图先等比降至组件适用像素尺寸，再使用Slate九宫格；圆形素材透明补齐90×90，未硬拉伸。原有顶栏、地区框与工具条底板继续复用原素材。

## 原生表现与交互边界

所有标题、地区、关卡名、天气/时段名、正文、单位、日期/时间/星期与退出为独立TextBlock；原图不可避免的SOS图标字样保留。天气按钮底色及Normal/Hovered/Pressed/Selected为UMG原生样式；时段图标、工具符号和退出符号由独立UMG形状组成。

| 视觉入口 | 保留的控件名 | 当前实际执行 |
| --- | --- | --- |
| 摄像机 | B_摄像机视角 | 原SetIsActive，补可见高亮；未发现摄像机业务切换执行链 |
| 警报 | B_报警灯 | 原SetIsActive，补可见高亮；不是跌倒告警闭环 |
| 定位/选人 | B_人称切换 | 原SetIsActive，并显示/隐藏既有ElderDebugControlWidget；原选人、接管、Tab返回仍由Controller负责 |
| SOS | B_紧急呼救系统 | 2026-09-04已接BP_alarm逐实例Light_Toggle；默认关，再点关。悬停即时1.01倍、移出1倍；无外部呼救服务 |
| 三关卡 | Button_45 / Button / Button_1 | 真Button；原EventGraph无关卡打开逻辑，未臆造地图映射 |
| 天气 | Button_SunnyDay / Button_CloudDay / Button_RainDay | 原BGS_State写入、A_CallLightShow，之后更新单选高亮 |
| 时段 | Button_Moon / Button_Noon / Button_Night | 原早晨/正午/晚上状态写入及A_CallLightShow，之后更新高亮 |
| + / − | Button_67 / Button | 已接既有ProcessOrbitZoomInput，未新增相机 |
| 适配图标 | Button_434 | 保留原BPI_用户界面.点击缩放，即UI收展动画，不伪称相机重置 |
| 移动符号 | BTN_Move | 真Button和右键旋转说明，尚无独立点击切换模式 |
| 退出 | Button_2 | QuitGame；旧未连执行线的Bad cast节点保留为既有技术债 |

Weather/Time新增RefreshSelectionVisuals与RefreshSelectionFromOwner。主界面在原GameState注入后恢复高亮；按钮执行原业务后更新表现。不另建权威状态、不Tick轮询。仍使用原枚举；新增类型须扩展原枚举/业务，再复制一列控件及更新高亮映射，不能只加一个图标便称业务支持。新增关卡须用户提供地图路径后明确绑定。

## 图表

复用UBP_能耗曲线及WebInterface_116，不新建图表Widget。Content/Echarts/line-stack.html 改为本地动态Canvas，保留 ue.interface.ue2js 对象/JSON入口和旧12个月electric/water数据兼容；没有外部CDN。三点测试及空数据实际改变了PIE图表。默认25个小时点、0–100、约中午88峰值是示例，鼠标提示标示示例，不是实测业务数据。

## 验证与差异

- 11个Widget编译保存成功；关卡旧动画目标Overlay_44恢复为实际内容父组，0警告。时间框仅保留原有1条未执行坏转换警告。
- 9组天气/时段实际OnClicked委托、Owner读回、互斥高亮通过。物理鼠标Sunny点击通过；不是所有按钮的物理点击验收。
- +/-实际距离2250→2000→2250；既有选人面板单实例开关；Camera/Alarm可见高亮已读回。
- 1920×1080实际PIE截图：Saved/Screenshots/UIReference_Final1920.png；追加动效修正后的三个同时放大截图为UIReference_IconHover1920.png。
- 1600×1200实际PIE中设计内容为1600×900，上下各150安全边距，保持16:9，未压扁。
- 2560×1440/3840×2160使用同一ScaleToFit设计结构，但本机原生窗口受工作区钳制，未取得可信的全尺寸像素截图。虚拟窗口尺寸探测有缓存/屏外剔除影响，不作为完整通过证据。
- 原PNG比参考更粗亮，摄像机/定位图案也不同；这些素材差异依然可见。运行背景为原3D场景，不把参考图格线烘焙成整屏UI。
- 原场景仍有VSM灯光超量及导航警告，不属于本轮UI编译错误；未更改灯/导航系统来隐藏这些问题。
- 独立审查与当前变更过程见Saved/UIReference_STATUS.md；不得把图表单项PASS冒充整套通过。

备份：Saved/CodexBackups/UIReference_20260903，保留11个精确原Widget和原图表HTML。未修改C++、地图或其他任务的轮椅/模型改动。

## 追加：圆形图标放大与裁剪

用户要求减小放大并防止重叠/裁边。实际来源是UBP_小菜单的三个OnHovered/OnUnhovered动画，不是OnClicked。原Scale=1→1.3，90像素图标变117，而中心间距仅91。现有三个动画改为1→1.01、0.1秒线性插值，移出恢复1；最大90.9像素，三个同时放大仍保留约0.1像素几何间隙。SOS原无放大动画，保持90像素。没有更改按钮点击执行线、位置、命中区域或选中亮度。

裁边来源是UBP_左侧栏中的UBP_小菜单实例Clipping=ClipToBounds，已改Inherit；其余父级与全屏裁剪不扩大修改。内部Image/Button/Canvas本来即Inherit。外层左栏范围能够容纳最大动画边界。

两项持久改动仅涉及UBP_小菜单、UBP_左侧栏；主界面作为依赖重新编译保存，三者均0错误0警告。已在1920×1080 PIE触发三个同时悬停、移出恢复，记录实际RenderTransform/边界/裁剪，见Saved/UIReference_IconMotionTests.json；几何不是原生Slate绘制裁剪栈，截图另行验证可见圆形完整。定位按钮仍开关同一个选人控件。不是全部物理鼠标事件验收。

局部修改前备份：Saved/CodexBackups/UIIconMotion_20260903。独立复审结论单列在Saved/UIReference_STATUS.md，不扩大为2K/4K或全部业务完成。

局部独立复审PASS：资产裁剪设置、三条动画绑定/时长、实际委托几何与截图、EventGraph接线不变及备份均已核对。最终11Widget编译保存0错误，仅原时间框1警告；退出委托实测结束PIE。Designer为Custom1920×1080，已截图但仍是局部放大视图；尝试缩放全览时用户Escape停止电脑操作，因此未继续桌面输入。PIE停留在停止状态。

## 2026-09-04：去除右侧纯色硬衬底观感

用户指出天气/时段标题旁的深蓝方块不协调。它来自上轮PanelFill纯色衬底；上轮为分离文字而清掉PNG内部渐变后，该衬底显得像一块遮盖板。不是新业务窗口或点击区域。

本轮在原四个PanelFill上使用同一份M_UI_PanelRecess（/Game/Project_File/BP/UBP/Materials），UI域、半透明材质，提供圆角抗锯齿、轻微顶部渐变与内侧蓝光。没有新建Widget或PNG、没有合成整屏图。PNG在Z=-2，衬底在Z=-3且HitTestInvisible，文字和按钮在上方。天气/时段衬底内缩6，空间/能耗内缩14以避开其斜角，组件外框及原布局坐标未变。渐变是可维护的程序化近似，不是恢复原PNG被清理掉的精确像素。

持久修改：UBP_右侧栏、UBP_天气选择、UBP_时间选择、UBP_能耗曲线；新增共享材质M_UI_PanelRecess。仅修改PanelFill的Brush与CanvasSlot，没有改业务图、图表HTML、原Texture、C++或地图。精确备份Saved/CodexBackups/UIPanelFill_20260904；保存后属性与材质快照UIPanelFill_After.json。UIPanelFill_Before.json记录的是退出期间连接失败，不能作为有效属性基线；未完成备份与最终包的独立二进制业务接线差异比对。

验证：四Widget编译保存0错误0警告；9组天气/时段实际委托、GameState读回与互斥高亮通过（UIPanelFill_SelectionTests.json）。实际1920截图UIPanelFill_1920_Final.png；实际1600×1200截图UIPanelFill_1600x1200.png及几何快照，保持1600×900设计空间、上下150安全边距。已补上一轮Designer全览截图UIPanelFill_Designer1920.png，设计尺寸Custom1920×1080，编辑器缩放显示并非1920像素导出；WebUI图表在Designer占位、PIE正常绘制。独立审查结果见Saved/UIReference_STATUS.md。2K/4K全尺寸、全部物理点击、打包仍未验收，不能将本轮局部通过扩展为原长任务全部完成。

独立局部审查PASS：四面板/材质读回、三种截图、9组合和当前状态/灯光调用链均核对；不包含备份包的完整接线差异、全部物理交互或高分辨率验收。

执行期间在首次材质创建成功前发生DX12光追GPU PageFault；已保留UIPanelFill_GPUCrash_20260904.log并只重启本项目。重启前无未保存包，目标备份完好。本UI任务未改GPU设置；同期另一任务添加了光追透明规避，见项目指南对应条目，不把UI复测算作GPU长期稳定性验收。最终PIE停止，预览配置恢复1920宽度。
