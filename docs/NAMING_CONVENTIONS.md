# 命名约定

- 保留现有 BP_、BGM_、BGS_、UBP_、ABP_、ABS_、BPI_、E_、ST_、DT_、SM_、SK_、M_、MI_、IA_、IMC_ 命名体系；不为统一前缀重命名已有资产。
- 中文 UI 名称是当前工程事实；新增内容放在现有分类目录。Widget 实例名可能保留旧名，不等于资产名，也不证明重复功能。
- C++ 使用 UE 类前缀 A/U/F/E/I；当前老人枚举叫 `EElderState`、`EElderGender`，不是必须创建同名内容资产。
- 既有 `BP_AI` 是老人，`BP_Camera` 是监控，`BP_Character_Base` 是玩家观察角色；名字较泛也不能误换职责。
- 新公共函数用动作语义（Set、Start、Stop、Pause、Resume）；新增状态字段明确 Owner 与可写范围，不再用一组互斥 Boolean 代替 ElderState。
- 数据表拼写 `Vlaue`、行名 `Moon`、旧 Widget 实例名及重定向器视为兼容契约；仅在单独迁移任务中检查全部引用、备份、修复并验证。
- 文档统一引用已查到的真实包路径；找不到时写 UNKNOWN，不拼出看似合理的 Asset Reference。
