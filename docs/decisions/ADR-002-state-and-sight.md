# ADR-002：状态单入口、事件通知与视觉显示分离

状态：记录当前核心契约，2026-09-03。

## Context

多老人、摄像机和UI需要独立数据；线框显示不具备真正视觉检测能力。

## Decision / Reason

每老人EElderState通过SetElderState修改并广播，消费者读事件/接口，不Tick轮询。AI真实视觉在独立AIController的Perception；SightCone只显示，身份不由全局变量共享。

## Consequences

摄像机历史/UI投影可以存在，但不是第二权威。当前监控仍具体类型注册且只有球形Overlap，此决策不等于它已实现通用接口或真实视锥；待补项必须另记。

## Do Not Change Unless

明确新的所有权/通知模型、生命周期和多实例验收；不能用多个互斥Boolean或Mesh碰撞代替现有状态/AI感知。
