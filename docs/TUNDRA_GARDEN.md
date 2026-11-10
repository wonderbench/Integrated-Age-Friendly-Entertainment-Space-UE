# 苔林 · 室内生态康养园

独立工作区：`E:/Jiang_Work/Old_Man_Jiang_TundraGarden`。基于2026-09-10主项目磁盘文件的物理复制；不是持续同步的Git worktree，不包含当时主编辑器尚未保存的改动。依赖本机UE5.8及现有引擎插件。

## 当前场景

打开根目录 `Old_Man_Jiang.uproject`，默认地图为 `/Game/TundraGarden/Rainforest/Maps/L_RainforestSanctuary`。旧 `/Game/TundraGarden/Maps/L_TundraSanctuary` 仅保留作早期方案，不代表当前交付。

主厅内部50×70米，约3500平方米，另有两端封闭前室。四周50厘米厚实体不透明墙，顶高约11米，漫射采光顶与室内面光提供柔和光照。HDR仅参与天光照明，当前地图没有可见HDR天空球。灰绿低饱和色调配合苔石、深木和亚麻休息设施。

植物共5330个实例：65棵大树、65棵小树、900株蕨类、460株宽叶植物、470株灌木、270株高地被、1900簇酢浆草、1200簇草。以7组实例组件保存，环绕曲线主步道、三条横向通道和6个休息口，非仅中央绿岛。115块苔石丰富林下层。

六个休息区共6张沙发、12把有靠背与扶手的椅子、6张茶桌。主路径扶手中心高87厘米、直径4.4厘米，沿曲线连接横向通道至休息口；交叉处保留3.4米横向通道，末端向下回收。两端门洞净宽3米，地面无抬高休息平台。以上为场景几何设计，不是无障碍规范认证；尚未验收轮椅操控、坐姿、交互或AI导航。

## 素材与职责

- 本机Blender建模：蕨类、宽叶植物、起伏苔地、旧版苔石与草簇。可编辑源文件在 `SourceArt/TundraGarden/Rainforest` 及其上级目录。
- [Poly Haven Mossy Rock](https://polyhaven.com/a/mossy_rock)：2K底色、法线、粗糙度，许可见 [CC0](https://polyhaven.com/license)。源数据校验记录见 `SourceArt/TundraGarden/sources.json`。
- 树、灌木、酢浆草与家具复用项目 ElderWellness / MedaFurniturePack，保留各自既有许可边界。当前照明HDR复用 `MedaFurniturePack/HDRI/T_approaching_storm_4k`；先前下载的Mossy Forest保留为源素材，未作本地图背景。
- 针对独立副本内六个复用植物材质补齐实例化使用标记；修改前精确备份在 `Saved/Tundra/Backup_BeforePolish/InstancedMaterials`。未修改主工作区相同资产。
- 地图灯光与后期只归本场景所有，复用既有 `BP_EW_EmptySceneMode` 展示模式。未新建老人、玩家Controller、MainUI或天气状态系统。

## 验证与边界

真实UE独立游戏进程按地图内四个CameraActor出图，输出在 `Saved/Tundra/FinalViews`，非生成式概念图。早期默认材质回退截图不代表修正后结果；修正后日志为 `Saved/Tundra/RainforestGameCapture_Final.log`，最终独立审查证据另存该目录。

本次为环境资产任务，无C++变更，没有为此进行原生构建。未进行打包、老人AI接入、人工PIE轮椅通行或康复效果验证。画面质量以实际截图判断，资产加载或审查不替代视觉验收。

地形FBX导入的Y轴镜像通过场景内 `(1,-1,1)` 缩放校正；不要直接重置地形Actor缩放。主路67个中心射线样本均命中平地，证据 `Saved/Tundra/ground_alignment_verified.json`；此项不等于全路径宽度或轮椅碰撞验收。

最终独立复审：环境修复PASS，地形对齐小范围复审PASS。对应 `Saved/Tundra/independent_final_review.json`、`independent_final_geometry.json`、`independent_ground_review.json`。展示截图副本在 `Documentation/TundraGarden`。
