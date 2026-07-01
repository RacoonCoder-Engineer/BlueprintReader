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
				"UMG",
				"UMGEditor",
				"DeveloperSettings"   // M4.1: UBPR_Settings (UDeveloperSettings)
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
					"DesktopPlatform"   // M4.2: native SaveFileDialog for export
					// M6.0b: StateTreeModule/GameplayStateTreeModule/StateTreeEditorModule removed —
					// unused in the codebase, vestigial dependency, link-risk on a 5.8 host without
					// the StateTree plugin enabled.
				}
			);
		}
	}
}