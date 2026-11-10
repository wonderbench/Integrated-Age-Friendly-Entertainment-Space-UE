# LivingGrove 柔和树影闪烁修复（2026-11-02）

## 现状与原因

`L_LivingGrove_Social` 的日光树影存在持续细颗粒跳动。当前太阳光源角约 2.673345°，叶片继续随风运动；本轮实际运行的 VSM SMRT 配置为 8 条平行光射线、每条 4 次采样。宽半影与复杂遮罩叶片使低采样的随机阴影噪声明显。提高采样后颗粒减弱，恢复原值后重新出现，支持这是主要噪声来源；不把所有植被运动或全场景闪烁归为同一种问题。

原 `CR_FilteredMorningSun` 的 LightComponent0 在 PIE 中 `bVisible=false`，不能仅凭其强度 34 判断重复照明。当前太阳、曝光、树叶材质和 WPO 不作永久修改。Epic 对 [VSM 的 SMRT 采样与半影噪声](https://dev.epicgames.com/documentation/en-us/unreal-engine/virtual-shadow-maps-in-unreal-engine) 的说明与本轮对照相符。

## 已保存的修改

仅在 `Config/DefaultEngine.ini` 的 `[ConsoleVariables]` 增加：

```ini
r.Shadow.Virtual.SMRT.RayCountDirectional=64
r.Shadow.Virtual.SMRT.SamplesPerRayDirectional=8
```

这是**项目全局平行光阴影质量下限**，也会影响其他地图的太阳/月亮；不是 LivingGrove 专属运行脚本。引擎渲染器拥有这些参数，不新增天气或状态 Owner。局部灯采样与 `AdaptiveRayCount=1` 保持原行为，未关闭投影、叶片动画、Lumen 或虚拟阴影。设置优先于普通 Scalability，低画质也会承担这两项固定采样成本。

没有持久 C++、Blueprint、材质、网格或关卡改动，不需要原生重编译。本轮开始时的 8 个未保存包在结束 PIE 后仍为同一组未保存包；未执行 Save All。测试相机和灯光试验仅存在于 PIE，停止后销毁。

## 本轮验证

证据目录：`D:/CodexProjectBackups/GroveShadowRepair_20261102`。`BeforeAfter.mp4` 为同机位源视频的并排裁切，按各自实际采集时间对齐，重复已有帧供播放、不生成插值帧；源采集约 12 帧/秒，不能充当 60 FPS 逐帧分析。

| 对照 | 地面 ROI 平均灰度 | 高频相邻帧差 |
| --- | ---: | ---: |
| 原值 A1：baseline_live | 103.774 | 2.534 |
| 原值复测 A2：baseline_A2 | 103.512 | 2.548 |
| 最终 64×8、自适应开启：adapt64x8_live | 104.945 | 1.227 |

指标取全帧录制的 x550–1300、y300–900 区域，去前 10 帧，灰度减去 sigma=2 高斯模糊后计算相邻帧绝对差均值。它同时含真实树影运动、采样和录制误差，不是感知闪烁百分比。肉眼多帧对照支持颗粒明显减轻、阴影形状/柔和度保留；**不是全场景零噪点或彻底消除所有闪烁的保证**。

最终参数另录 `final64_move` / `original_move_correct`（平移与转向）、`forest_final64` / `forest_original`（石板与近叶）、`forest_retreat_final64` / `forest_retreat_original`（连续拉远）。旋转使用明确 pitch/yaw/roll，运动日志确认 roll 约为 0。比较范围仅为各录像共同覆盖的运动区间，不以回调日志最终点冒充完整同程。

已舍弃：最初三个后台 3 FPS 录制不作正常性能证据；旧 `move_candidate` / `move_baseline` 使用错误的 Rotator 位置参数，不作最终移动验收；缩小太阳角改变阴影软硬，未采用；提高贴图分辨率、TAA/TSR 拒绝阈值、直接写光追开关的试验均未构成更优修复。所有非最终参数恢复本轮开始值，临时 Actor 随 PIE 销毁。恢复后的部分 CVar 仍为 Console 优先级，原编辑器未重启，不能称优先级也已恢复。

## 性能与持久化边界

同机位短 GPU 历史样本：原值约 11.73 ms，最终 64×8 约 17.43 ms。约增加 5.7 ms，属于明显质量/性能交换；这些是非独占编辑器短样本，不是稳定 FPS 或所有硬件预算。32×8/关闭自适应有近似降噪效果，但会影响局部灯提前结束采样，因此未采用。

新启动的独立 `-game` 进程从磁盘配置读到 64/8，`LastSetBy: SystemSettingsIni`；AdaptiveRayCount 为 1、来源 Constructor。`FreshGrove.log` 和 `FreshGrove.png` 证明保存配置后的加载/渲染与正常退出。该进程只读取磁盘地图，主编辑器未同时保存资产；它不包含主编辑器尚未保存的地图修改。启动截图只有数帧历史，不用于证明阴影最终画质或 UI 完整显示。

LivingGrove、Showcase、ElderWellness 三个独立游戏进程依次加载磁盘版本，均读到 64/8 的配置来源、生成启动渲染图并正常退出。对应 `FreshGrove`、`FreshShowcase`、`FreshElderWellness` 日志未检出 `Error:` 或 `Fatal`；仍有既有插件/UI等警告，不能称零警告。另两图仅做启动渲染兼容检查，没有它们的前后性能对照。原编辑器 PID 21128 保持运行。

最终独立审查 **PASS（限定降噪修复、阴影保留与配置持久化范围）**，记录位于证据目录 `REVIEW.md`；审查结论与上述运行证据分开看待。未做打包、全部天气/夜景、全园遍历、低端显卡或完整交互回归。

## 回退

精确原配置为证据目录中的 `DefaultEngine.before.ini`；原指南/天气文档也有 before 副本。回退时只删除本轮新增的两项及相邻说明，保留其他已有修改；重新启动后让 Scalability 重新接管。当前会话可临时恢复本轮基线 8/4，但这仍是 Console 优先级，不等同重启后的配置状态。
