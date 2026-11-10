# 吊挂导航牌与关卡目录整理（2026-10-27）

后续已新增可开关微光蓝图包装，原模型/材质保持；默认仍为本页原外观。拖入蓝图与调节方式见 [微光蓝图说明](WAYFINDING_GLOW_BLUEPRINTS_20261027.md)。

## 交付范围

用户最终明确“先把模型做出来，我来放”。因此只交付六类吊牌模型与材质，**未向三个关卡放置任何吊牌**，也未新建卫生间或重命名场景分区。三个指定地图通过编辑器资产迁移进入 `/Game/Project_File/Maps`，保留原名称与陈设。

| 内容 | UE静态网格 |
| --- | --- |
| 休息区 / REST AREA | `/Game/Project_File/Props/Wayfinding/Meshes/SM_WF_Hanging_Rest` |
| 娱乐区 / RECREATION | `/Game/Project_File/Props/Wayfinding/Meshes/SM_WF_Hanging_Play` |
| 卫生间 / RESTROOM | `/Game/Project_File/Props/Wayfinding/Meshes/SM_WF_Hanging_WC` |
| 出口 / EXIT | `/Game/Project_File/Props/Wayfinding/Meshes/SM_WF_Hanging_Exit` |
| 运动区 / EXERCISE | `/Game/Project_File/Props/Wayfinding/Meshes/SM_WF_Hanging_Fitness` |
| 阅读区 / READING | `/Game/Project_File/Props/Wayfinding/Meshes/SM_WF_Hanging_Read` |

每件为完整吊挂装配：180×42×5.4 cm牌体、两根不锈钢吊杆、连接套、顶装底座、紧固件、凹缝、倒角、立体双面中英文与图标。含吊杆底座总尺寸180×8.8×97 cm，**枢轴位于顶装中心，底部在枢轴下97 cm**。正反面文字分别建模，反面箭头视觉朝向相反，保持同一世界方向；摆放者需按实际路线旋转整牌。单个网格保留完整LOD0几何与五个材质槽；不含碰撞，作为装饰导视使用。

## 材质与制作源

- 六个材质位于同目录 `Materials`：细砂炭灰喷涂、拉丝古铜边框、不锈钢、浅色嵌字、凹缝垫层、出口绿色。字面只带低强度自发光，不新增灯光Actor。
- `Textures` 内七张2048×2048自制PBR贴图，包含底色、线性粗糙度与切线法线；不锈钢复用拉丝粗糙度/法线。正常颜色与数据贴图分开设置sRGB/压缩。
- 本机 `E:/Ksoftware/blender-5.2.1-windows-x64/blender.exe`，已安装 `blender_mcp.py` 的本地9876端口提供 `execute_code`。实际建模、FBX导出和Cycles预览通过该MCP插件执行Python；未下载外部素材或改变全局MCP配置。
- 可编辑源：[Wayfinding_HangingSigns.blend](../SourceArt/Wayfinding/Wayfinding_HangingSigns.blend)，贴图、六份FBX和可复建脚本均在同目录。源工程内七张PBR图片已打包，六件可见模型按整套排开。

## 地图与引用

- `/Game/Project_File/Maps/L_ElderWellness_Main`
- `/Game/Project_File/Maps/Showcase`
- `/Game/Project_File/Maps/L_LivingGrove_Social`

`Config/DefaultEngine.ini` 的EditorStartupMap/GameDefaultMap改指新Showcase路径；`LS_LivingGrove_SocialLife` 的地图依赖已改为新LivingGrove路径。旧路径保留小型重定向包以兼容旧引用，未做全项目重定向器清理。LivingGrove Social原本使用EmptySceneMode；它不是本月另做功能迁移的TundraSanctuary，未向本图追加AI/UI。

## 检查与证据边界

- 迁移前后逐Actor核对标签、类、位置、旋转：ElderWellness 1104→1104，Showcase 440→440，LivingGrove 1691→1691；无缺失/新增项，三图均无新增吊牌。直接项目依赖缺失为0。
- 六网格、六材质、七贴图共19资产读回；尺寸、材质槽、完整LOD0、贴图色彩空间和项目依赖检查通过。最终网格关闭Nanite，以保留细字、饰条和准确包围盒。
- Blender Cycles整套/近景/反面预览已检查六类文字；UE临时预览世界实渲染确认资源可显示，临时世界未保存。单帧SceneCapture存在噪点与曝光差异，不作为三图最终灯光或安装效果验收。
- 编辑器重启后从新路径加载三图，Actor清单及19资产读回检查通过；三图各进行一次5秒PIE简短运行，ElderWellness/Showcase检查唯一老人、观察Pawn、MainUI和四台监控，LivingGrove检查其原LevelSequence，未将Social误当功能迁移地图。结果 `Saved/Wayfinding/pie_smoke.json` 三项PASS。首次检查脚本对Python字典返回值的JSON解析失败，修正后完整重跑；该早期脚本失败不算运行PASS。
- UE离屏近景最初过曝并出现细饰条断续；尝试完整Nanite回退/精度后，最终尺寸检查发现Nanite构建扩张包围盒，未接受该中间结果。最终关闭Nanite并保留原始LOD0，六网格读回恢复精确180×8.8×97 cm，见 `Saved/Wayfinding/full_geometry_result.json`。以持久渲染状态与明确曝光复核 `Saved/Wayfinding/UE_Accumulated_Close.png`，文字和吊杆连续可读。极细金属线仍有斜线采样/反射变化，不声称消除所有像素噪点。
- 最终独立审查和最终状态见 `Saved/CodexBackups/HangingSigns_20261027/REVIEW.md`、`Saved/Wayfinding/final_world_state.json`；审查不等于实际安装、全部业务或打包验收。

精确修改前文件及哈希在 `Saved/CodexBackups/HangingSigns_20261027/Before`；制作与检查结果在 `Saved/Wayfinding`。本次不改持久C++或Blueprint逻辑，因此未运行原生完整构建、全量业务回归、物理输入或打包验收。
