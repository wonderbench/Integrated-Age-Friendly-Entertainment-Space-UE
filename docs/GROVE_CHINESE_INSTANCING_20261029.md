# LivingGrove 中文标识和场景优化（2026-10-29）

地图 `/Game/Project_File/Maps/L_LivingGrove_Social` 的29个原 TextRender 英文/里程文字在编辑及游戏中隐藏；29个独立中文字形网格代替，含求助、迎宾、出口、六活动区及14处里程标记。旧文字仍保留以便回退；新对象位于 `LivingGrove/ChineseSigns_20261029`，带 `GroveChineseSigns_20261029` 标签。求助牌附着在原立柱顶部，迎宾牌有服务台支撑；里程牌回到原文字中心的 XY，置于环道地面。14处中心射线均命中 z=3.5cm 的 `LG_LoopCarpet`，底板中心 z=3.75、厚 0.5cm。52个新对象的组件存盘重载后均为 NoCollision 且不影响导航。字形由 `SourceArt/LivingGrove/ChineseSigns/build_text.py` 及本机微软雅黑粗体制作；字体使用权应随项目发行自行确认。场景可见文字中的贴图或外部模型内嵌英文尚未逐资产全面审计。

将88个 `LG_VentSlat` 和520个 `LG_DrainCoverSlot` 静态对象分别替换为一个 HISM 组件，共608实例；逐项位置、旋转及缩放与事前记录一致，材质、BlockAll 碰撞、静态移动性和阴影设置保持。Vent 代表性碰撞射线命中，未对每一个实例逐一实测碰撞或导航。原项目材质 `M_R_Oak` 与 `M_LG_MossGranite` 补存 InstancedStaticMeshes 用途；原资产位于备份中。其他构件不强制合并。

24个路灯均保留光源，6盏 PathLight 实例关闭投影，其余18盏保留投影；不修改共享路灯蓝图或天气链。无阴影灯附近可能出现照亮遮挡物背面的变化；夜间实际对照证据见 `Saved/GroveChinesePerf_20261029/night_allshadows_road.png` 与 `night_6shadows_off.png`，抽样视角并非完整场景验收。结构上减少独立对象与投影灯，但未取得可靠的前后性能基线，不能断言帧率提升或卡顿已解决。

中文和地面牌视图：`activity_sign.png`、`help_fitted.png`、`welcome_glow.png`、`exit.png`、`distance_floor_fixed.png`；为编辑器 SceneCapture，并非完整人工游览。原世界变换清单 `inventory.json`，安装和试验脚本在同目录。完整前后地图和两种材质副本在 `D:/CodexProjectBackups/GroveChinesePerf_20261029`；回退全图会覆盖后续其他更改，优先按对象/属性选择性恢复。字体网格、原文显隐和里程牌可按标签及 `signs.json` 单独还原；608实例回退需依据 inventory.json 或精确前置地图重建，未实跑选择性回退。

E盘满曾导致一次地图文件截断（尽管保存报告成功）；已保存损坏文件，将之前保存且哈希核对的地图从D盘恢复，重启编辑器后再补无碰撞设置、保存并切图重载成功，最终地图与 `after_collision_fixed.umap` 哈希一致。构建缓存转存到 `D:/CodexCacheArchive/Old_Man_Jiang_IntermediateBuild_20261029B`。为安全重启保全并保存当时未保存的 BP_AI、BP_Player、BP_Character_Base，磁盘前后副本在同备份目录。最终 PIE 启动后交互打开另一地图出现重复终止对话并触发 PlayWorld 断言，编辑器崩溃；当次不能作为运行验收；此后已在重载的最终地图单独启动并正常退出 PIE，但未做完整游览或交互验收。非完整导航/人工轮椅/打包验证。
