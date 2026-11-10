using UnrealBuildTool;
using System.Collections.Generic;

public class Old_Man_JiangTarget : TargetRules
{
	public Old_Man_JiangTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "Old_Man_Jiang" });
	}
}
