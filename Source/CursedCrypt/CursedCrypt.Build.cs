// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CursedCrypt : ModuleRules
{
    public CursedCrypt(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "AIModule",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "UMG",
            "Slate",
            "Json",
            "JsonUtilities"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        PublicIncludePaths.AddRange(new string[] {
            "CursedCrypt",
            "CursedCrypt/Variant_Platforming",
            "CursedCrypt/Variant_Platforming/Animation",
            "CursedCrypt/Variant_Combat",
            "CursedCrypt/Variant_Combat/AI",
            "CursedCrypt/Variant_Combat/Animation",
            "CursedCrypt/Variant_Combat/Gameplay",
            "CursedCrypt/Variant_Combat/Interfaces",
            "CursedCrypt/Variant_Combat/UI",
            "CursedCrypt/Variant_SideScrolling",
            "CursedCrypt/Variant_SideScrolling/AI",
            "CursedCrypt/Variant_SideScrolling/Gameplay",
            "CursedCrypt/Variant_SideScrolling/Interfaces",
            "CursedCrypt/Variant_SideScrolling/UI"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}