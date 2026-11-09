using UnrealBuildTool;

public class ActionRPGEditor : ModuleRules
{
    public ActionRPGEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "GameplayAbilities", "GameplayTags",
            "BlueprintGraph", "KismetCompiler", "NavigationSystem", "UMG", "ActionRPG", "MovieScene", "MovieSceneTracks", "AssetRegistry"
        });
    }
}
