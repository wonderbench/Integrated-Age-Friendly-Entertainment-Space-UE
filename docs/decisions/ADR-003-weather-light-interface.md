# ADR-003：继续复用现有天气灯光接口

状态：记录已实现通信与本轮保留决定，2026-09-03。

## Context

天气/时间按钮写BGS_State，灯通过标签和BPI_Actor_Call.Light_Show接收；已有Timeline渐变。删除原图会切断功能。

## Decision / Reason

BGS保存选择，灯按完整时间/天气组合算目标，weather_temp仅兼容；目标变化才Play/Reverse，保留单灯过渡。选择立即广播，天空过渡不阻塞灯光。

## Consequences

保留原接口、标签和Timeline；UI目前直接写GameState，这是现状，不是新统一选择服务已完成。BGS不被描述为持久存档。

## Do Not Change Unless

确认调用方、保留按钮顺序无关及中途反向规则，完成全部组合/多灯测试；大规模灯注册优化不能借机重建天气系统。
