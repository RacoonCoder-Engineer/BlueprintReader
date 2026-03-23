// Copyright (c) 2026 Racoon Coder. All rights reserved.


#include "BlueprintReader.h"
#include "Core/BPR_Core.h"
#include "UI/BPR_OutputWindow.h"
#include "UI/BPR_ContentBrowserAssetActions.h"
#include "UI/BPR_TabSwitcher.h"
#include "UI/BPR_InfoWindow.h"

#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Framework/Application/SlateApplication.h"
#endif

//==============================================================================
// StartupModule
//==============================================================================
void FBlueprintReaderModule::StartupModule()
{
	// Creating Core
	CoreInstance = MakeUnique<BPR_Core>();

#if WITH_EDITOR
	// Creating an output window with TabSwitcher
	OutputWindow = MakeShared<BPR_OutputWindow>();

	// Registering actions in the Content Browser
	ContentBrowserActions = MakeShared<FBPR_ContentBrowserAssetActions>();
	ContentBrowserActions->Register();
#endif
}

//==============================================================================
// ShutdownModule
//==============================================================================
void FBlueprintReaderModule::ShutdownModule()
{
#if WITH_EDITOR
	if (ContentBrowserActions.IsValid())
	{
		ContentBrowserActions->Unregister();
		ContentBrowserActions.Reset();
	}

	OutputWindow.Reset();
#endif

	if (CoreInstance)
	{
		CoreInstance.Reset();
	}
}

//==============================================================================
// OpenOutputWindow
//==============================================================================
#if WITH_EDITOR
void FBlueprintReaderModule::OpenOutputWindow()
{
	if (OutputWindow.IsValid())
	{
		OutputWindow->Open();
	}
}
#endif

//==============================================================================
// HandleMenuClick
//==============================================================================
#if WITH_EDITOR
void FBlueprintReaderModule::HandleMenuClick(UObject* SelectedObject)
{
	if (!SelectedObject || !CoreInstance.IsValid() || !OutputWindow.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("BPR: Invalid state - Object=%d, Core=%d, Window=%d"), 
			SelectedObject != nullptr, CoreInstance != nullptr, OutputWindow.IsValid());
		return;
	}

	// Checking if the asset is supported
	if (!CoreInstance->IsSupportedAsset(SelectedObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("BPR: The selected asset is not supported!"));

		InfoWindow = MakeShared<BPR_InfoWindow>();
		InfoWindow->Open({
			FText::FromString("Blueprint Reader"),                                    // Title
			FText::FromString("Warning! This asset is not supported yet!"),           // Main warning
			FText::FromString(
				"The Blueprint Reader plugin cannot display this asset type in the current version.\n"
				"This is not an engine error, it's just support not yet implemented."), // SubMessage
			FString("https://github.com/RacoonCoder-Engineer/BlueprintReader"),             // URL
			FText::FromString("Check for updates")                                    // Button text
		});

		return;
	}

	// ВАЖНО: Сначала открываем окно и даём виджетам инициализироваться
	OutputWindow->Open();

	// Getting TabSwitcher AFTER opening the window
	TSharedPtr<SBPR_TabSwitcher> TabSwitcher = OutputWindow->GetTabSwitcher();
	if (!TabSwitcher.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("BPR: TabSwitcher is invalid after opening window!"));
		return;
	}

	// Launching the extractor
	CoreInstance->ExtractorSelector(SelectedObject);

	// Getting data from Core
	const FBPR_ExtractedData& ExtractedData = CoreInstance->GetTextData();

	// Logging for debugging
	UE_LOG(LogTemp, Log, TEXT("BPR: Setting data - Structure length: %d, Graph length: %d"),
		ExtractedData.Structure.ToString().Len(),
		ExtractedData.Graph.ToString().Len());

	// Passing data to TabSwitcher
	TabSwitcher->SetData(ExtractedData);
	
	UE_LOG(LogTemp, Log, TEXT("BPR: Data successfully set to TabSwitcher"));
}
#endif

//==============================================================================
// Module registration
//==============================================================================
IMPLEMENT_MODULE(FBlueprintReaderModule, BlueprintReader);