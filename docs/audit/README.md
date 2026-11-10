# UE5.8 只读审计资料

审计日期为 2026-11-10（北京时间）。源码、核心蓝图、三张地图、图表网页、项目入口和脱敏后的 DefaultEngine.ini 来自审计包。仓库补收 DefaultEditor.ini、DefaultGame.ini 和 DefaultInput.ini。更新后的项目状态与技术债位于本包的 docs 目录。此包按磁盘资料交付，保留旧包目录结构。运行相关结果见独立验证证据表。

- [功能、风险和下一步建议](../AUDIT_SUMMARY.md)
- [当前状态](../CURRENT_STATE.md)
- [技术债](../TECH_DEBT.md)
- [构建、蓝图编译、自动 PIE 和打包证据](VALIDATION_EVIDENCE.md)
- [文件清单与 SHA-256](FILE_MANIFEST.csv)

## 证据与覆盖范围

| 文件 | 证据与限制 |
| --- | --- |
| [blueprint_index.json](exports/blueprint_index.json) 和 exports/blueprints | UE-MCP 编辑器插件 export_blueprint_batch 对 15 个当前磁盘蓝图读回。包括指定六个蓝图及运行用 ABP_AI_Laor。JSON 包含父类、组件、变量、函数与图。节点引脚 linkedTo 记录连线端点。组件和变量列表按导出器口径列出蓝图声明，原生继承成员还需查 C++ 父类。 |
| [assets.csv](exports/assets.csv) | UE Asset Registry 同步全扫描，导出全部已挂载的磁盘资产索引行，包括路径、类、重定向标记和范围。计 11,587 个对象，其中 /Game 2,375 个对象、2,359 个包。磁盘枚举另有 2 个 809 字节 UE 包未出现在注册表，具体路径见统计表。 |
| [references.csv](exports/references.csv) | UE Asset Registry 的直接包依赖，逐次分别查询硬引用、软引用。从 /Game 及已挂载插件包出发，保留指向 /Engine 和 /Script 的引用。/Engine 包自身的出边、运行时拼出的路径、外部网页和原生代码字符串不在这个引用表中。 |
| [unresolved.csv](exports/unresolved.csv) | 201 条未匹配资产行的硬软引用，其中 26 条源自 /Game，192 条在推定挂载目录下无同名包文件，9 条虽未入索引但包文件存在。按候选保留，不将插件示例、重定向、索引差异或软引用一律视作运行故障。/Script 是原生模块命名空间，不按资产缺失计数。 |
| [plugins.json](exports/plugins.json) | 899 个本机引擎或项目插件描述符及挂载记录。版本来自 .uplugin 的 Version 与 VersionName，mounted 来自隔离编辑器启动日志，project_declared 来自 .uproject。表含未挂载插件。 |
| [registry_summary.json](exports/registry_summary.json) 和 [map_bundle_dependency_gap.json](exports/map_bundle_dependency_gap.json) | 统计与三图硬引用递归结果。资料包保留核心资产，其余引用仅列路径。完整运行需在原项目环境验证。 |
| exports/scenes 与 [editor_readback.json](exports/editor_readback.json) | 隔离编辑器逐图加载磁盘场景，读取 GameMode、已加载 Actor 类型计数和关键实例。World Partition 未加载 Actor 不在该场景统计中。只记录编辑态结构。 |

原项目桥接在启动前未连通，进程检查未发现该项目编辑器。隔离读取使用独立项目外壳和 Saved 目录，原 Content 与 Plugins 通过目录联接读取，未设置文件系统只读。使用只读导出和场景加载入口，关闭自动保存。起止分别读回零脏包，最终 PIE 为 false。首次 NullRHI 读取器启动失败，第二次成功完成注册表读取，但插件描述符的严格 JSON 解析失败。随后从描述符与启动日志补齐版本表，并由正常启动的读取器完成蓝图和地图导出。这些属于读取工具的过程记录，不能用于判定项目构建或玩法通过。

仓库中的 DefaultEngine.ini 为原 GithubStage 的脱敏副本。插件和第三方资源只保留审计所需名称、版本及依赖路径。缓存、日志原文、构建产物、完整第三方资源库及原始未脱敏配置不随包交付。原 Stage 中 16.5 MB 的思源黑体资产在本包中只登记引用路径。

[原生组件构造定位](exports/native_component_construction.txt)来自当前源码中 CreateDefaultSubobject 调用的搜索结果，可补查父类中创建的组件。UMG 设计器 WidgetTree 与运行时动态添加的组件不属于蓝图本地 SCS 清单。

