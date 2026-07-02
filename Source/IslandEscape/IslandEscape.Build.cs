using UnrealBuildTool;
using System.IO;

public class IslandEscape : ModuleRules
{
    public IslandEscape(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // 모듈 루트 + 모든 Variant 서브폴더를 include 경로에 등록
        // "Variant_Combat/CombatGameMode.h" 형태와 "CombatAttacker.h" 형태 둘 다 커버
        PublicIncludePaths.AddRange(new string[]
        {
            ModuleDirectory,                                                        // "Variant_Combat/Foo.h"
            Path.Combine(ModuleDirectory, "Variant_Combat"),                        // "CombatCharacter.h"
            Path.Combine(ModuleDirectory, "Variant_Combat", "AI"),
            Path.Combine(ModuleDirectory, "Variant_Combat", "Animation"),
            Path.Combine(ModuleDirectory, "Variant_Combat", "Gameplay"),
            Path.Combine(ModuleDirectory, "Variant_Platforming"),                   // "PlatformingCharacter.h"
            Path.Combine(ModuleDirectory, "Variant_Platforming", "Animation"),
            Path.Combine(ModuleDirectory, "Variant_Platforming", "Gameplay"),
            Path.Combine(ModuleDirectory, "Variant_SideScrolling"),                 // "SideScrollingUI.h"
            Path.Combine(ModuleDirectory, "Variant_SideScrolling", "AI"),
            Path.Combine(ModuleDirectory, "Variant_SideScrolling", "Gameplay"),
            Path.Combine(ModuleDirectory, "Variant_SideScrolling", "Animation"),
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "UMG",
            "AdvancedWidgets",
            "Slate",
            "SlateCore",
            "AIModule",
            "NavigationSystem",
            "GameplayTasks",
            "Water",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "Niagara"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Chaos",
            "MoviePlayer",
            "PhysicsCore",
        });
    }
}
