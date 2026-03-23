// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "UI/BPR_ContentBrowserAssetActions.h"
#include "ToolMenus.h"
#include "Modules/ModuleManager.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "UI/BPR_TabSwitcher.h"

//==============================================================================
// Register / Unregister (без изменений)
//==============================================================================
void FBPR_ContentBrowserAssetActions::Register()
{
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
    FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

    Section.AddMenuEntry(
        "ShowBPAsMD",
        FText::FromString("Read Blueprint for AI-Assistant"),
        FText::FromString("Convert Blueprint to Markdown via BPR Core"),
        FSlateIcon(),
        FUIAction(
            FExecuteAction::CreateRaw(this, &FBPR_ContentBrowserAssetActions::OnShowBPAsMDClicked)
        )
    );
}

void FBPR_ContentBrowserAssetActions::Unregister()
{
    UToolMenus::UnregisterOwner(this);
}

//==============================================================================
// Новые сеттеры (без изменений)
//==============================================================================
void FBPR_ContentBrowserAssetActions::SetCore(TSharedPtr<BPR_Core> InCore)
{
    CoreInstance = InCore;
}

void FBPR_ContentBrowserAssetActions::SetOutputWindow(TSharedPtr<BPR_OutputWindow> InOutputWindow)
{
    OutputWindow = InOutputWindow;
}

//==============================================================================
// OnShowBPAsMDClicked (без изменений)
//==============================================================================
void FBPR_ContentBrowserAssetActions::OnShowBPAsMDClicked()
{
#if WITH_EDITOR
    UObject* SelectedObject = GetSelectedAsset();
    if (!SelectedObject) return;

    ExecuteForObject(SelectedObject);
#endif
}

//==============================================================================
// ExecuteForObject — исправленная версия с Pin()
//==============================================================================
void FBPR_ContentBrowserAssetActions::ExecuteForObject(UObject* SelectedObject)
{
    // Получаем сильную ссылку на Core
    TSharedPtr<BPR_Core> Core = CoreInstance.Pin();
    if (!Core.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("BPR: CoreInstance is invalid or expired"));
        return;
    }

    if (!Core->IsSupportedAsset(SelectedObject))
    {
        auto Info = Core->GetUnsupportedAssetInfo();

        BPR_InfoWindow::FParams Params;
        Params.Title              = Info.Title;
        Params.Message            = Info.MainMessage;
        Params.SubMessage         = Info.SubMessage;
        Params.OptionalURL        = Info.GitHubURL;
        Params.OptionalButtonText = Info.ButtonText;

        auto InfoWin = MakeShared<BPR_InfoWindow>();
        InfoWin->Open(Params);

        return;
    }

    // Получаем сильную ссылку на OutputWindow
    TSharedPtr<BPR_OutputWindow> Window = OutputWindow.Pin();
    if (!Window.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("BPR: OutputWindow is invalid or expired"));
        return;
    }

    // Открываем окно
    Window->Open();

    auto TabSwitcher = Window->GetTabSwitcher();
    if (!TabSwitcher.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("BPR: TabSwitcher not valid after Open()"));
        return;
    }

    // Экстракция и передача данных
    Core->ExtractorSelector(SelectedObject);

    const auto& Data = Core->GetTextData();
    TabSwitcher->SetData(Data);
}

//==============================================================================
// GetSelectedAsset (без изменений)
//==============================================================================
UObject* FBPR_ContentBrowserAssetActions::GetSelectedAsset() const
{
#if WITH_EDITOR
    FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    TArray<FAssetData> SelectedAssets;
    CBModule.Get().GetSelectedAssets(SelectedAssets);
    return SelectedAssets.Num() > 0 ? SelectedAssets[0].GetAsset() : nullptr;
#else
    return nullptr;
#endif
}