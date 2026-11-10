# 天气、时间与灯光

### 2026-11-02：LivingGrove 柔影颗粒降噪

为减少大光源角、动态遮罩树叶形成的 VSM 半影时序噪声，项目 `DefaultEngine.ini` 将平行光 SMRT 射线/每射线采样固定为 64/8；保留自适应提前结束、局部灯采样、太阳角度、曝光和 WPO。这是全项目平行光质量下限，低画质也有相应成本；不改变 BGS_State/UDS 的天气状态归属。当前 LivingGrove 同机位实录明显减轻颗粒，短 GPU 样本约 11.73→17.43 ms；不能称全场景零闪烁。新进程已读回配置来源，原编辑器 8 个脏包未保存。方法、恢复边界与验收范围见 [阴影稳定性记录](../GROVE_SHADOW_STABILITY_20261102.md)。

2026-10-27两图接入：适老空间复用本图既有UDS/UDW；苔原按用户批准添加现有UDS/UDW，保留但在游戏中隐藏原HDRIEnvironment、关闭其碰撞，原HDRISkyLight与SoftSun停光。天气和时段仍由BGS_State及原UI驱动。当前既有晴天按钮对应Cloudy预设、阴天对应Overcast、雨天对应Rain_Thunderstorm；本次保留既有映射。早晨/正午/晚上分别驱动620/1200/2400（午夜可能归零）。本轮未将目标图所有装饰灯改造成Showcase的天气灯接口。

Owner：`BGS_State` 保存选择；UDS 表现天空；`BP_LightBase_Box` 表现单灯。天气/时间 UI 仍在原 `UBP_天气选择` / `UBP_时间选择`。

## 现有通信不可整套删除

按钮读取 DT_Weather / DT_Time → 写 GameState → A_CallLightShow → B_Call_LightShow → 查询 BP_Light 标签 → **BPI_Actor_Call.Light_Show** → 单灯 Timeline。

所以原蓝图接口仍在使用。灯里的 Light_Show、规则判断、IsOpenLighting? 去重、Timeline、SpotLight 更新、BeginPlay 同步不是废代码。Light_Show1 手动入口保留；只有确认无调用的旧节点才可另行清理。

## 当前规则

| 时段 | 晴 | 阴 | 雨 |
| --- | --- | --- | --- |
| 早晨 | 亮 | 亮 | 亮 |
| 正午 | 灭 | 灭 | 亮 |
| 晚上 | 亮 | 亮 | 亮 |

每次基于完整组合计算，点击天气或时间的先后顺序不影响结果。雨天全天亮沿用原设计。weather_temp 参数继续存在以兼容调用，不再决定灯规则。

上轮实现保留1.5秒/0→30亮度 Timeline，Play/Reverse 从当前值渐变，目标相同不重播；BeginPlay 先归零再同步当前组合。天空时间用 PlayFromStart，灯广播立即执行，不等天空过渡结束。历史18组合/47灯和中途反向证据在 Saved/Tests/WeatherLights_Verified.json。

## 当前版本边界

2026-09-04 GPU崩溃规避：Showcase后处理体当前仍保存`TranslucencyType=RayTraced_Deprecated`且覆盖开启；项目`DefaultEngine.ini`以`[ConsoleVariables] r.RayTracing.Translucency=0`统一使用光栅透明，覆盖该旧模式。依据为09:27崩溃的Aftermath显存地址读取故障与光追透明路径记录，不是显存不足的结论。玻璃折射外观可能变化；天气状态、灯光接口、Lumen及其他光追设置不变。已验证当前会话参数与有渲染PIE截图，尚未重启读回或长期稳定性验收；详见`Saved/Tests/GPUTranslucency_20260904.md`。

本轮读取时 BP_LightBase_Box 是**未保存的编辑器版本**，未编译、保存或运行验证。历史通过不能覆盖这些新修改，下一次功能改动前先与用户当前图核对。

`Showcase` 关卡的 `Ultra_Dynamic_Weather` 已启用低强度的 UDW 屏幕雨滴：中心强度 0.05、边缘强度 0.4、纹理平铺 1.25、清除速度 2.0，并关闭“离开水体后保持湿润”。这是同一份 UDW 天气状态的相机后处理表现，不增加第二套天气或常驻手动雨滴；只有当前天气的降雨与相机暴露共同生效，晴/阴预设会将目标和当前屏幕雨滴强度归零。室内完整遮蔽时它会自然减弱，不能把它描述为室内持续降雨。

DT_Time 的 Moon 行为早晨620、Noon为1200、Night为2400，另有Morn=None/0；保持既有键名。BGS 时段选择、UDS 当前天空时间、UI 系统日历时钟是不同数据；时间框末尾“阴”仍写死，不能当作 GameState 当前天气的可信显示。

后续先统一选择/展示入口，再考虑大规模灯注册或事件订阅优化；当前是按钮事件时查标签，不是每帧全场景轮询，无需为架构理想迁移到新 Subsystem。

### 2026-10-27：LivingGrove 动态天气

`L_LivingGrove_Social` 复用现有 UDS/UDW 与天气时段 UI；场景原天空灯、定向日光和大气 Actor 保留但游戏中停用，避免和动态天空重复照明，原有室内灯具保留。PIE 中三项天气和三个时段的状态切换经读回验证；动态天空带来的背景与光照变化不等同于与原静态场景完全一致。

### 2026-10-27：LivingGrove 动态日光局部调校

针对动态天空替换原光照后运行画面偏亮，已将本关卡 UDS 太阳强度 3000 调为 500、日间曝光偏移设为 -0.75；保留动态天气和时间。重启后天气/时段状态及 UI/路线相关脚本测试通过，同视角试验图道路过曝有所缓解。原静态光照仍未恢复，太阳角度、阴影及整体观感与旧图不同，不能称完整视觉复原或所有天气视觉验收通过。

### 2026-10-28：LivingGrove 曝光统一及扫描石板

仅修改 LivingGrove 地图实例和新增独立材质/贴图：关闭该关卡 UDS Apply Exposure Settings，由原 RF_DesaturatedFilm priority10 管理 EV100 3.5–4.8、补偿-0.5、清空补偿曲线；太阳250、天光2、Sun Source Angle Scale5，白平衡6500K、饱和度.86、低泛光。保留现有 Lumen 与天气时段链。大跨度自动曝光导致的运行时补亮改为有限适应，日景高光收敛、阴影柔化；北侧深阴影及夜景仍偏暗，非完整美术重制。

10处原 QuietRubber 步道/平台改用 Poly Haven Stone Tiles02 的4K颜色/DirectX法线/粗糙度，新增 M_LG_ScannedStonePaving，世界XY按240cm循环。仅材质覆盖，未新增逐块石板几何、真实位移或碰撞；原网格/导航地形保留。下载文件按官方API MD5验证，来源与CC0说明见 SourceArt/LivingGrove/StoneTiles02/README.md。备份、固定视角及天气截图见 Saved/CodexBackups/LivingLighting_20261028。

最终复核：曝光范围3.5–4.8的保存版本已做第二轮天气/时段截图；独立只读审查确认日景过曝控制、石板细节及柔和阴影改善通过，夜景/北侧深阴影仍偏暗。验收为限定本关卡日景和已捕获天气画面，非商业游戏品质等同或全面性能测试。
最终功能回归：本轮保存版本在PIE中运行严格功能测试，老人接管/归还、姿态/跌倒起立、路线实际位移、视线与四监控摄像机显示、三天气属性和三时段读回均PASS；证据 L_LivingGrove_Social_Runtime.json。Readback.json确认10处覆盖与三张4096贴图。没有修改任何已有蓝图资源或共享原材质。当前PIE停止、脏包0。性能/打包未验收。
