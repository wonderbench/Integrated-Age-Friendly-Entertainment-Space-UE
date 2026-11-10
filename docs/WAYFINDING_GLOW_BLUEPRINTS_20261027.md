# 吊牌微光蓝图（2026-10-27）

本轮仅把既有六件吊牌包装为可调微光Actor蓝图，沿用原网格/PBR资源；不在三个场景中摆牌，不新增天气/时间系统或灯光Actor。旧19个资源与交接时三个地图的文件哈希均保持一致。

## 使用入口

`/Game/Project_File/Props/Wayfinding/Blueprints` 内六个可拖入关卡的子蓝图：

| 蓝图 | 内容 |
| --- | --- |
| `BP_WF_Hanging_Rest` | 休息区 |
| `BP_WF_Hanging_Play` | 娱乐区 |
| `BP_WF_Hanging_WC` | 卫生间 |
| `BP_WF_Hanging_Exit` | 出口 |
| `BP_WF_Hanging_Fitness` | 运动区 |
| `BP_WF_Hanging_Read` | 阅读区 |

共用父蓝图为 `BP_WF_Hanging_Base`。枢轴、180×8.8×97 cm总尺寸、双面文字、原始LOD0与无碰撞设定沿用原模型，摆放方式见[模型交付记录](WAYFINDING_SIGNS_20261027.md)。

## 开关与默认外观

用户明确默认保持原预览图外观，因此 **“启用微光”默认不勾选**。关闭时直接恢复原 `M_WF_IvoryInlay` 和原炭灰/出口绿底板材质，并非把动态材质亮度归零。原材质字面本身已有0.12低强度自发光，属于原始外观，不把关闭“额外微光”说成所有发光为零。

选中场景实例，在“吊牌微光”分类调整：

- **启用微光**：总开关，默认关闭。
- **文字图标亮度**：默认0.5，仅开启总开关后生效；控制文字、图标、箭头。建议0.1–0.8，0表示该通道不额外发光。
- **底板亮度**：默认0；需要底板微光时可从0.03–0.2试调。
- **文字图标颜色 / 底板微光颜色**：分别控制两条自发光通道，原有底色、法线、粗糙度仍保留。

编辑状态修改实例参数会经Construction Script刷新；运行时使用 `SetGlowEnabled(Enabled)` 开关并立即刷新，或使用 `ToggleGlow()` 切换。运行中通过其他逻辑改亮度/颜色后显式调用 `ApplyGlowSettings()`；不依赖Tick轮询。`ToggleGlow` 与 `ApplyGlowSettings` 也可在编辑器对选中实例执行。

## 实现边界

- 实例配置是微光状态唯一Owner；Construction Script与BeginPlay调用同一 `ApplyGlowSettings`。没有每帧查找、自动跨场景同步或对BGS_State/UDS写入。
- 开启后槽3使用专用字面MID、槽2使用专用底板MID；各Actor独立。槽0/1/4的金属/凹缝材质沿用原网格，不改变。
- 新增三种参数化材质位于 `Materials/Glow`，不修改旧六材质。Emissive链路在材质内部限制GlowStrength到0–3；属性值本身不会因UI元数据自动在运行时钳制。
- 仅模拟柔和自发光，没有蓄光充能/衰减逻辑，也没有永久场景灯光。

## 验证记录

父蓝图及六子蓝图创建后编译均为0错误、0警告并保存；工具回执在 `Saved/WayfindingGlow/build_log.json`、`controls_log.json`。旧19资产哈希与三图交接后地图哈希均一致。

为避免与按钮任务争用主编辑器，将全部29项Wayfinding资产按原包路径复制到独立临时验证工程，SHA-256逐一相同（`copy_manifest.json`差异0）。以同版本UE5.8.3无窗口加载、保存数据读回及SIE模拟运行：七个蓝图默认关闭、六种网格/原材质、六类各自开关、Rest五轮状态切换、ToggleGlow及两个实例的亮度/MID隔离全部通过。`isolated_validation.json`最终PASS，实际结果见 `readback_assets.json`、`construction_validation.json`、`runtime_validation.json`。

测试脚本最初有Python反射方法拼写、启动入口自动退出和Slate重入问题；隔离进程曾在夜景准备时因重入切图触发原生World Memory Leaks崩溃。已修正并加重入守卫，完整流程复跑PASS且正常退出；历史attempt01–04与原始日志保留，不将中间失败当通过。主工程/地图未被这些测试加载保存。

夜景对照使用临时灯光和固定曝光，展示原外观OFF、仅字面ON、仅底板ON；读回确认实际MID参数。该证据不等于三个正式关卡的最终光照或安装验收。独立审查报告见 `Saved/CodexBackups/WayfindingGlow_20261027/REVIEW.md`。

精确修改前备份在 `Saved/CodexBackups/WayfindingGlow_20261027/Before`；本轮未修改C++，不把另一个按钮任务的原生构建算作本功能的独立构建证据。
