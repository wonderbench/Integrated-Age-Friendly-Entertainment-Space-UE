# 三关卡辅助扶手试放 · 2026-10-27

状态：已摆放、保存及重载；独立复审 PASS，限三图辅助扶手可回退场景试放。审查逐张查看六张实景图，核对保存读回、原对象差异和备份哈希，无必须修复项；未把几何检查扩大为实际全程 AI/轮椅通行验收。审查记录见 `Saved/CodexBackups/AssistRails_20261027/REVIEW.md`。

## 试放内容

三图位于 `/Game/Project_File/Maps`。仅增加 15 个 StaticMeshActor，复用已有 `SM_EW_ContinuousRail` 与 Oak/Brass 材质，原模型与材质未修改。圆扶杆有向下回收端部，成品握持顶高约 86.3–90.4 cm，单段约 2.1–3.24 m。此为可回退的场景预览，不代表扶握动画或工程认证。

| 关卡 | 新增 | 布置依据 |
| --- | ---: | --- |
| L_ElderWellness_Main | 5 段 | 中央会客/棋牌家具外侧的通行边界，朝阅读与轻运动区的连接，保留横向开口 |
| Showcase | 4 段 | 中央走廊单侧与两翼会客区的横向连接，避开现有门口；按实际两种地面高度贴地 |
| L_LivingGrove_Social | 6 段 | 保留原有环线扶手，只补阅读、茶座及棋桌横向步道；环线交叉口留空 |

每段都带唯一试放标签 `AssistRailPreview_20261027`，名字以 `ARP_EW_`、`ARP_SC_`、`ARP_LG_` 开头，集中在同名 World Outliner 文件夹的三个子组。完整位置清单位于 `Saved/AssistRails_20261027/plan.json`。

## 保存与检查

- 修改前逐图精确备份，原文件与备份 SHA-256 相同；修改后另存 After 快照并核对一致。路径为 `Saved/CodexBackups/AssistRails_20261027/{Before,After}`，均含 manifest。
- 三图分别保存成功并从磁盘重新加载，专用标签读回数量为 5/4/6；全为 Static、BlockAll，依赖引用有效。原有 Actor 在本轮操作前后位置/旋转没有变化，同会话 Actor 增量恰为 5/4/6。读回见 `Saved/AssistRails_20261027/saved_verification.json` 及三份 `*_saved_readback.json`。
- 现有 ElderWellness 线性样条、Showcase 曲线样条每段采样 400 次，与扶手世界包围盒的 XY 间隔最小分别为 77.8、176.4 cm。按现有老人胶囊半径 34 cm，额外间隙为 43.8、142.4 cm。此为几何采样，不是实际轮椅外轮廓扫掠或全程 AI 行走证明。LivingGrove 未找到老人样条 Actor，按原 `CR_CrossPath_Node` 步道布置。
- 未发现新增段与原静态家具、结构柱或原扶手的直接包围盒重叠。整圈合并踢脚线、蓝图显示视锥、聚合植被盒不适合用作三角面穿插结论。六张实际 PIE 截图用于补充可见接触与开口检查。
- Showcase 首次保存因第二编辑器 PID49368 持有同图而失败（Error Code 32）。核对第二实例无脏包后由其原生桥接 requireClean=true 安全退出，再保存成功；无丢弃扶手或用户改动。最终交接时 PIE 已停、dirtyPackages=[]。
- 拍图时仅在 PIE 副本移除主 UI 遮挡，并使用临时相机；停止 PIE 后这些临时对象销毁，未保存界面改动。未改老人、控制器、样条或公共业务系统。

## 实景截图

均在 `Saved/AssistRails_20261027`：

- `ElderWellness_Aisle_Clean.png`、`ElderWellness_Reading.png`
- `Showcase_Corridor.png`、`Showcase_LoungeLink.png`
- `LivingGrove_ReadingLink.png`、`LivingGrove_ChessLink.png`

## 回退

优先只删除本轮 15 个新增对象，保留后续其他任务的地图修改。`Saved/AssistRails_20261027/rollback_preview.py` 提供默认只报告的回退函数；执行删除前要求 PIE 停止且无未保存包，逐一校验标签、清单中的名字及网格，再删除并保存。整图 Before 备份仅作灾难恢复兜底，不应覆盖后续的新工作。用户可直接要求撤回本轮扶手，由智能体按清单执行。

没有声称真实扶握支撑、全部轮椅通行、导航重建/路线运行回归、夜景或打包已验收。本轮不含 C++、Blueprint 或材质内容修改。
