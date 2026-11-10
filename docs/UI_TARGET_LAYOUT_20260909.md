# 目标图布局替换（2026-09-09）

## 当前实现

复用 `UBP_用户界面` 的 Canvas → ScaleBox → 1920×1080 SizeBox；没有第二套 MainUI。按用户参考图的1672×941坐标换算左右区域和底部按钮，素材来自 Desktop/2，包括后补的环境与时段整框。底部五按钮按用户补充要求保持无文字、无图标。

- 修改原9个 Widget：主界面、左侧栏、右侧栏、当前区域、天气选择、时间选择、时间框、能耗曲线、小菜单。
- 37张处理纹理位于 `/Game/UI/TargetLayout20260909`，由 Widget 的 Brush 硬引用；不覆盖旧 NewGoldStyle 纹理或用户 PNG。没有新增绘制边框的 Border/材质。
- Top 裁去不透明外围，沿原边界保留标题/房屋/叶片，仅延展空白横段；退出按钮拆分二图集，天气/时间拆分六图集。面板只伸展空白内段以保留标题和角部；底部高亮 Alpha 校准到普通轮廓。处理不是 AI 重绘，用户已授权代码裁剪。
- 新环境整框替代原天气、时间两个外框；原按钮与 BGS_State 写入、Light_Show 调用、Owner 恢复链保留。仅将 RefreshSelectionVisuals 的未选中按钮透明度0.78改为1，Selected_0/1/2继续互斥显示高亮图片。
- 关卡3项、底部5项的持久高亮由原 `UProjectUserWidgetBase` 增加两个默认关闭的开关控制，仅目标 Widget 开启。SelectTargetLevel/SelectTargetNavigation及对应事件只管理展示选择，不跳转地图、不修改业务 Owner。样式缓存有GC引用，Construct/Destruct成对绑定/解绑，无Tick。
- 左侧概览、设备数字显示 `--`：没有假造目标图示例人数、在线率或安全状态。新增文字为参考布局占位，不是统计系统接入。
- 原小菜单4个按钮保留，改为设备面板4个图标热区，Tooltip说明实际功能；仍是监控、报警灯、选人接管、SOS，不代表新增设备统计页面。
- 原地区选择和缩放工具 Widget 在目标主画面中 Collapsed，资产与业务图保留。能耗继续使用原本地 WebInterface/ue2js 与示例曲线，仅调整显示区域以消除重复图例；没有改HTML数据协议。原相机、建筑、天气/老人逻辑不改。

## 点击高亮规则（2026-09-09 增量）

- 目标导航、关卡、天气、时段和退出按钮默认使用普通素材；首次打开界面不主动选中，悬停不会产生持久高亮。
- 目标导航与关卡索引默认 `INDEX_NONE`；点击后仅当前项使用高亮材质并互斥清除同组其他项。已选项 Hovered 保持高亮，避免闪回普通态。
- 天气/时段 `RefreshSelectionFromOwner` 由 `bReflectOwnerSelection=false` 门控，保留原 GameState 写入、灯光调用和点击后的互斥 Selected 图片；同一 Widget 移除重加保留已点击视觉状态。
- 增量备份：`Saved/CodexBackups/ClickOnlyHighlight_20260909/`。完整关闭编辑器构建 `Result: Succeeded`；重启后 PIE 委托/反射断言 20 项通过，截图：`Saved/ClickOnlyHighlight_20260909/Initial1920.png`。验证边界不等同于人工物理鼠标命中或打包测试。

## 验证与边界

完整闭编辑器构建 `Result: Succeeded`。9个 Widget 编译保存检查为0错误；`UBP_时间框` 保留既有 `Reset Create Main UI` 警告1条，没有借换布局删除历史节点。运行19项断言覆盖三关卡/五底部项互斥和持久高亮、天气与时段实际GameState字段、同一主界面移除重加后再选择；这是PIE委托调用和属性断言，不是全部物理鼠标命中、打包或全DPI验收。

独立复审为限定范围 PASS。1920×1080实际截图已检查标题完整、正文无裁切、环境整框、无底部文字图标、图例不重复。**不是100%像素复刻**：用户素材的标题/边框细节与目标不同，能耗框沿用旧资源，统计未接入，场景视角没有为了截图改动。

首次安全重启后同19项行为断言PASS，但PIE_Reopened1920.png发现空间正文字号持久化错误，不能作为最终视觉PASS。原因是统计标签处理共用了正文模板字体数据；已改独立副本并恢复正文字号11。再次重启后磁盘读回与PIE断言字号11，连同原19项共20项PASS；最终PIE_Verified1920.png正文末句完整、曲线正常、底部留白。未为复测改动相机或关卡；编辑器保持打开，结束测试停止PIE。

证据：`Saved/UILayoutTarget_20260909/` 下的 PreparedManifest.json、AppliedLayout.json、FullBuild.log、FinalBlueprintValidation.json、RuntimeTests.json、PIE_Final1920.png。首轮测试误取子Widget继承属性，已修正为实例根路径；早期Python Struct文本只证明读取成功，最终改为真实字段枚举/时间数值断言，不沿用旧弱断言作为Owner验证。

备份：`Saved/CodexBackups/UILayoutTarget_20260909/`。UBP_BeforeWrite为写入前UI副本，父类两源码另存；PIE后BP_AI出现脏包，关闭前依项目协议保全保存前/后两个副本，未有意修改其逻辑/模型。恢复时仅选择明确目标，不整目录覆盖其他后续修改。
