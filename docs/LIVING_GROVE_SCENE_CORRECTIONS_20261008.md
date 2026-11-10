# 森活庭五处场景修正（2026-10-08）

本次只修改独立目录 `E:/Jiang_Work/Old_Man_Jiang_TundraGarden` 的 `/Game/TundraGarden/LivingGrove/Maps/L_LivingGrove_Social` 与两个新步道网格；主工程和老人、玩家、UI、天气逻辑未改。本目录不是 Git 仓库。

| 用户指出的问题 | 已保存的场景修正 |
| --- | --- |
| `GENTLE MOVEMENT` 木牌悬空 | 六块同类 `CR_LandmarkBacking` 各加两根落地木柱，底部进入地面，顶部嵌入牌体。 |
| 栏杆之间是泥土 | 旧 `LG_LoopCarpet` 网格顶面朝下；替换为朝上的封闭环线网格，沿原双侧栏杆连续铺设，材质与十字步道统一。 |
| 两侧椅子背对桌子且过远 | 三组茶座共六把 `LG_SupportChair` 旋转至面向各自茶几，中心距从 205 cm 调至 125 cm；桌沿与椅近侧约 38.5 cm。 |
| 喷泉挡住十字步道 | 增加内半径 265 cm、外半径 470 cm 的闭合绕行步道，接回四个方向的原十字路；两张沙发移至外侧休息垫，退出主要通路。喷泉池体保持原位。 |
| `RF_EntryVestibuleWall` 重叠闪烁 | 四面入口侧墙靠大厅端缩短 50 cm，内端停在 `y=±3550 cm`，与原端墙端部对接，避开端部岩壁的重叠段。 |

两个新网格在 `Content/TundraGarden/SceneCorrections/Meshes`，可重建的 OBJ 生成脚本、OBJ 源文件、修改及检查脚本在 `Saved/SceneCorrections20261008`。步道采用 Complex as Simple 碰撞，使环内花床不被整块凸包覆盖；这仅是资产碰撞设置读回，未进行真人或轮椅通行操作测试。

修改前地图精确副本在 `Saved/SceneCorrections20261008/Backup/L_LivingGrove_Social.umap`。任务中地图曾出现新未保存状态，先安全保存并另留 `Backup/L_LivingGrove_Social_after_user_dirty_save.umap`，随后才修改目标。新步道导入时未覆盖旧网格资产。最终地图保存于磁盘，独立 UE 游戏进程从该地图加载并正常退出，六张运行截图在 `Saved/SceneCorrections20261008/Views`，记录见 `GameCapture.log`。截图分别检查木牌、环线、茶座、喷泉眼高与俯视、入口近景；静态图无法证明运动过程中的零闪烁。另一个独立 UE 游戏进程对磁盘地图完成结构读回，`runtime_validation.json` 与 `GameValidation.log` 记录牌柱、步道网格、六把椅子及四面入口墙的实际运行值并正常退出。两个 OBJ 的全顶面朝向与闭合段数见 `geometry_validation.json`。最后一次编辑器只读校验的结构断言已通过至保存状态检查，之后独立编辑器退出，故不把该次检查称为完整 PASS，也不声称当前编辑器没有新脏内容。未修改 Blueprint 或 C++，未进行构建、PIE 行走或打包。

本轮独立审查结论另见 `Saved/SceneCorrections20261008/independent_review.json`。
