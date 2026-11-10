# 森活庭顶棚接边与日光调色（2026-10-09）

范围仅为独立目录 `E:/Jiang_Work/Old_Man_Jiang_TundraGarden` 的 `/Game/TundraGarden/LivingGrove/Maps/L_LivingGrove_Social`。主工程及老人、玩家、UI、天气功能均未修改；本目录不是 Git 仓库。

## 当前场景

原 `SM_LG_NestCanopy` 保留。新 `Content/TundraGarden/CanopyLighting/Meshes/SM_CL_AnchoredBirdNest.uasset` 从原网格复制，保持编织拓扑、材质与整体波浪形态；沿横向和纵向分别小幅延展枝条，并对每根枝条的端部九圈平顺修形，使末端伸入两侧岩壁/承托及南北端壁，高度约为 16.38 米。地图中原 `BG_RestoredBirdNest` Actor 改用新网格，没有新增第二片顶棚。网格读回共 46 个左右末端、32 个南北末端落在边缘接合范围，见 `Saved/CanopyLighting20261009/validation.json`。这是场景几何接边，不是已验算的建筑结构连接。

日光从原偏冷、较高角度改为偏暖的斜射光；降低原来均匀铺满场景的屋顶补光、林下补光与浓雾，再回调天光和活动区灯具，使茶座、林下植物及岩壁仍能辨认。后期只调整本地图全局 `RF_DesaturatedFilm` 的曝光补偿、对比、饱和度和轻微暗角。相关对象和最终读回参数见 `Saved/CanopyLighting20261009/lighting_settings.json`、`lighting_refinement.json` 与 `validation.json`。这是为当前场景制作的电影感日光版本；画面风格仍受现有模型、材质和实时渲染条件约束，不将它称为摄影级成片。

## 备份与证据

修改前地图及指南的精确副本在 `Saved/CanopyLighting20261009/Backup`。新顶棚网格第二次端部修形前的资产副本也在此目录。没有覆盖原 `SM_LG_NestCanopy`。制作和校验脚本保存在同一 `Saved/CanopyLighting20261009` 目录。

同机位实际 UE 独立游戏进程截图保留 `Before`、`Geometry`、`After`、`Refined` 四组；**最终采用 `Refined`**。`After` 是暗部不可读而被放弃的第一轮光照，不能用作最终效果。`Refined/SideConnection.png` 核对侧边枝条接入边缘；`Refined/Canopy.png`、`Grove.png`、`Tea.png` 核对顶棚、中心和茶座光影。独立进程日志 `Refined.log` 正常退出；编辑器结构检查 `validation.json` 为 PASS，保存后地图和内容包均无脏包。运行截图不等于人工漫游或轮椅通行，也不能证明所有视角的光影一致；没有 C++、Blueprint、构建或打包改动。

独立审查结论见 `Saved/CanopyLighting20261009/independent_review.json`。
