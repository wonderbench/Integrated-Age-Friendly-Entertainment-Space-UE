# LivingGrove 林间路灯 · 2026-10-28

独立审查限定 PASS：模型、蓝图灯光组件与稀疏摆放成果通过；前置备份流程失败不在此PASS内。详见 `Saved/GroveLamps_20261028/REVIEW.md`。

自制 Blender 路灯约高 319 cm，深色金属主体、暖木嵌条、古铜收边、乳白灯罩、八根护条、帽檐及底座锚栓。可编辑源文件和生成脚本在 `SourceArt/LivingGrove/PathLantern`；没有下载外部模型。

蓝图：`/Game/Project_File/Props/GroveLantern/Blueprints/BP_LG_PathLantern`。目录内另含1个网格和4个PBR材质。蓝图组件包括 LanternModel、PathLight 和 LanternGlow；组件参数可在蓝图中编辑。聚光灯1800 lm、2850 K、半径650 cm、外锥68度，出光点(12,0,246)避开底盘与灯杆；补光点灯90 lm、半径160 cm且不投影。灯罩低强度自发光。当前默认常亮，未接天气/时间自动开关，也不写入另一份天气状态。

在 `/Game/Project_File/Maps/L_LivingGrove_Social` 放置8个实例，沿环线外缘与活动平台边缘疏朗布置，最近中心间距约12.9m。标签`GroveLamps_20261028`，文件夹`LivingGrove/PathLanterns_20261028`，可按专用标签单独移除。位置见 `Saved/GroveLamps_20261028/DiskReload.json`。未为路灯移动旧家具、扶手、路线或植物。

## 验证与证据

- 蓝图编译0错误0警告，保存后切到Showcase再返回，8网格和16灯光组件世界位置/参数读回一致，PIE已停且脏包0。初次Static实例的组件未随Actor正确移动；改Movable并重新创建本轮8实例后复测，最终读回位置正确。初次底盘自遮光已通过出光偏移修正，最终夜景有实际照地光。
- 最终实际PIE截图：`Saved/GroveLamps_20261028/Day_Corrected.png`、`Night_Lit_Final.png`、`Night_Overview.png`。Night测试只在PIE将UDS时刻设2200，未保存夜间测试状态；临时相机/UI隐藏/Controller暂停Tick均只影响PIE副本。`Model_Framed.png`是过曝的诊断SceneCapture，不作交付图。
- 满盘期间的前置备份失败：`Map_preserved_before_lamps.umap`及C盘同名副本经独立检查均仅242字节、内容为资产元数据片段，不是可恢复地图；即使两份SHA256相同也不能认定有效。`Map_disk_before.umap`第一次复制失败同样禁止用于恢复。前置精确备份要求未达成，不能补称成功。完整修改后副本为`Saved/CodexBackups/GroveLamps_20261028/Map_after_lamps.umap`（3924477字节，UE包头C1-83-2A-9E），保存时SHA256与工作地图相同。优先回退方式为按`GroveLamps_20261028`标签及8个确切名称移除本轮灯实例，保留其他线程工作；未实跑删除。早期`LivingLighting_20261028/After`有完整旧图，但早于本轮和树预览，不是精确前置快照。
- 开始时目标地图已有用户未保存内容，保全时包含另一树线程的V2候选；已告知树线程仅恢复`BG_TallCanopyTree_0`组件，不用整图覆盖路灯。`Readback.json`中的该树差异归属另一线程；旧树预览不是路灯验收成果。
- E盘满时，用户明确授权清理可再生成缓存。自动审批拒绝删除，于是将项目`Intermediate/Build`完整移到`D:/CodexCacheArchive/Old_Man_Jiang_IntermediateBuild_20261028`暂存，保留恢复能力。未清理源码、模型、关卡、用户备份或其他项目。

独立审查记录：`Saved/GroveLamps_20261028/REVIEW.md`。本轮未测试全部天气、全程导航、照度规范、性能或打包；常亮路灯不代表自动时段控制已接入。
