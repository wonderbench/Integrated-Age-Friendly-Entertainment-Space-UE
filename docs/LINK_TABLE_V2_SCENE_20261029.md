# 悦邻一体桌 V2 场景替换（2026-10-29）

用户提供 `D:/适老娱乐一体桌_V2/03_模型/悦邻_LinkTable_V2_多场景模型.blend`。项目内保留源文件副本及导出脚本 `SourceArt/LinkTableV2`；从 `03_桌游模式` 导出桌体、五子棋盘、棋子、文字和曲线，排除棚拍场景和其他模式。导出时将桌体品牌文字临时改为“悦邻”，原 Blender 源文件保持不变。新资源 `/Game/Project_File/Props/LinkTableV2/SM_LinkTableV2_Game`，原概念桌面 120×120cm、高76cm；导入后按 77/76 等比缩放，使桌面与两关原棋桌顶面77cm一致。模型最高处约81.16cm包括棋子及附件，不是桌面高度。

`L_ElderWellness_Main` 中原两张 `EW_CommunityGameTable` 与其旧 `EW_ChessSet` 被各一张新桌替代；既有书、茶杯等保留。`L_LivingGrove_Social` 原 `LG_ChessTable` 与 `LG_ChessSet` 被新桌替代，原两把 `LG_ChessChair` 原地转向桌面。新桌原点分别位于(-45,-90,0)、(45,300,0)与(-1850,0,0)，缩放均约1.01316。五个自定义凸碰撞体覆盖桌面与四腿；第三关中心竖直射线命中桌面 z=77cm、桌下 z=60cm 中心短射线未命中。此为几何抽查，不是实物适老或轮椅认证。

两地图、资源的前后备份 `D:/CodexProjectBackups/LinkTableV2_20261029`；整图还原会覆盖后续用户更改，应先核对。原棋桌和棋盘 Actor 已移除，未完成逐对象回退实测。场景视图在 `Saved/LinkTableImport_20261029`，其中 `elder_view.png` 为两张室内桌的保存后视图、`grove_seats.png` 为第三关椅子转向后的视图；两关分别单独启动并正常退出 PIE；固定曝光编辑器截图及启动检查不等于人工就座、完整游览或导航验收。该模型是静态概念表现，模式切换、可调桌腿、电控及联锁没有被实现为运行功能。
