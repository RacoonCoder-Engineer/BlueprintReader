// Copyright (c) 2026 Racoon Coder. All rights reserved.

using UnrealBuildTool;
public class BlueprintReader : ModuleRules
{
	public BlueprintReader(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;


		// Public is always only Core
		PublicDependencyModuleNames.AddRange(
			new string[] 
			{
				"Core"
			}
		);

		// Everything else is Private
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"Slate",
				"SlateCore",
				"UMG"
			}
		);

		// Editor-only dependencies
		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
					"ContentBrowser",
					"AssetTools",
					"ToolMenus",
					"BlueprintGraph",
					"GraphEditor",
					"KismetCompiler",
					"WorkspaceMenuStructure",
					"UMG",
					"EditorStyle",
					"StateTreeModule",
					"GameplayStateTreeModule",
					"StateTreeEditorModule"
				}
			);
		}
	}
}