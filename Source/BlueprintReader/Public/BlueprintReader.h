// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "UI/BPR_ContentBrowserAssetActions.h"
#endif

class BPR_Core;
class BPR_OutputWindow;
class BPR_InfoWindow;

class FBlueprintReaderModule : public IModuleInterface
{
public:
	// Module life cycle
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Access to Core */
	BPR_Core* GetCoreInstance() const { return CoreInstance.Get(); }


	/** Window access */
	TSharedPtr<BPR_OutputWindow> GetOutputWindow() const { return OutputWindow; }

#if WITH_EDITOR
	/** Open a window manually */
	void OpenOutputWindow();

	/** Processes a click on a context menu item */
	void HandleMenuClick(UObject* SelectedObject);
#endif

private:
	/** Central plugin logic */
	TSharedPtr<BPR_Core> CoreInstance;


	/** Output window with TabSwitcher */
	TSharedPtr<BPR_OutputWindow> OutputWindow;
	TSharedPtr<BPR_InfoWindow> InfoWindow;

#if WITH_EDITOR
	/** Context menu in Content Browser */
	TSharedPtr<FBPR_ContentBrowserAssetActions> ContentBrowserActions;
#endif
};
