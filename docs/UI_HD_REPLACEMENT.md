# 高清 UI 贴图替换（2026-09-08）

用户提供 Desktop/2 的22张PNG，确认替换20张。沿用 `/Game/UI/NewGoldStyle/T_UI_*` 路径，未改Widget、C++、HTML、布局、命中框或事件。三个老人状态按钮仍为66×66，未包含在这批素材中。

## 映射

文件统一前缀 `ChatGPT Image 2026年9月8日 09_`，后缀 `.png`。

| 文件时间部分 | Texture后缀 |
| --- | --- |
| 54_23 | SpacePanel |
| 54_33 | LevelPanel |
| 54_37 | EnergyPanel |
| 54_48 | Location |
| 54_55 | SOS |
| 55_01 | Monitor |
| 55_07 | FirstPerson |
| 55_19 | Alert |
| 55_30 | Exit |
| 55_44 | WeatherSunny |
| 55_49 | WeatherRainy |
| 55_55 | WeatherCloudy |
| 56_11 | TimeMorning |
| 56_16 | TimeNight |
| 56_20 | TimeNoon |
| 56_25 | ActionBlank |
| 56_47 | ReturnObserve |
| 56_51 | Level03 |
| 56_54 | Level02 |
| 56_58 | Level01 |

55_38的右端是圆点，不对应当前下拉箭头；56_00空白框对应不明确，二者未替换。顶部、天气/时段外框、工具条和老人状态按钮无本批对应资源，保持原版。关卡标题叶饰由左变右、部分框线造型改变，是高清源图自带设计，不是布局节点移动。

## 备份与处理

原33个NewGoldStyle资产完整备份到 `Saved/CodexBackups/HD_UI_20260908/NewGoldStyle`。恢复时只选择目标纹理，先保全后续改动，不整目录盲目覆盖。

用户明确允许本地脚本清除外围白底/留白。仅删除边缘连通近白像素的Alpha并裁有效范围；内部文字符号不重绘，源PNG不覆盖。处理副本与完整映射在 `Saved/HD_UI_20260908/Prepared`、`Manifest.json`。原UI组、RGBA压缩、sRGB、NoMipmaps、Clamp、NeverStream等11项设置逐项保持；没有按高清像素尺寸改变Brush DesiredSize。

## 验证边界

20纹理保存并读回尺寸及设置，见 `ImportResults.json`。全部原UI蓝图哈希与替换前一致；没有蓝图图表改动，本轮不重编译/保存用户蓝图。1920×1080实际PIE截图 `After1920.png`、`AfterPanel1920.png` 已检查，包含原生操作面板和动态能耗图，没有白色外框。未重新执行全部物理点击、其他DPI或打包测试。NoMipmaps保留，高清细线大幅缩小后仍可能出现高频锯齿。

PIE已停止，20纹理无脏包；Showcase原有脏标记保留，未保存地图。

独立审查PASS，限20项映射、备份、导入设置、100个UI目录文件哈希零变化和两张1920运行截图；不扩展为所有按钮功能回归。
