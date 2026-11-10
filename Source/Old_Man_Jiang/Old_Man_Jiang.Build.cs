using UnrealBuildTool;

public class Old_Man_Jiang : ModuleRules
{
	public Old_Man_Jiang(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "AIModule", "GameplayTasks", "NavigationSystem" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "ApplicationCore", "EnhancedInput", "AnimGraphRuntime" });
		if (Target.bBuildEditor) PrivateDependencyModuleNames.Add("UnrealEd");
	}
}
