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
    // Создаём Core как shared (чтобы TWeakPtr в actions мог его держать)
    CoreInstance = MakeShared<BPR_Core>();

#if WITH_EDITOR
    // Окно вывода
    OutputWindow = MakeShared<BPR_OutputWindow>();

    // Actions в Content Browser
    ContentBrowserActions = MakeShared<FBPR_ContentBrowserAssetActions>();

    // Передаём зависимости в actions (это решает проблему "invalid or expired")
    ContentBrowserActions->SetCore(CoreInstance);
    ContentBrowserActions->SetOutputWindow(OutputWindow);

    // Регистрируем меню
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

    CoreInstance.Reset();
}

//==============================================================================
// OpenOutputWindow (оставляем, но можно будет убрать позже)
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
// HandleMenuClick — УДАЛЁН полностью
// Теперь вся логика в FBPR_ContentBrowserAssetActions::ExecuteForObject
//==============================================================================

//==============================================================================
// Module registration
//==============================================================================
IMPLEMENT_MODULE(FBlueprintReaderModule, BlueprintReader);