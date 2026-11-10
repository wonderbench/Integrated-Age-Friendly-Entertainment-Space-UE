using UnrealBuildTool;
using System.Collections.Generic;

public class Old_Man_JiangEditorTarget : TargetRules
{
	public Old_Man_JiangEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "Old_Man_Jiang" });
	}
}
