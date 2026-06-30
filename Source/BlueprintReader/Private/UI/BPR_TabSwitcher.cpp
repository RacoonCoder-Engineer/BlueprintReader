// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "UI/BPR_TabSwitcher.h"

#include "InstalledPlatformInfo.h"
#include "UI/BPR_TextWidget.h"
#include "Core/BPR_Types.h"
#include "Core/BPR_Settings.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// M4.2: export to file
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void SBPR_TabSwitcher::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SAssignNew(TabBarBox, SHorizontalBox)
        ]

        + SVerticalBox::Slot()
        .FillHeight(1.f)
        [
            SAssignNew(TabSwitcher, SWidgetSwitcher)
        ]

        // M4.2: persistent export footer
        + SVerticalBox::Slot()
        .AutoHeight()
        .HAlign(HAlign_Right)
        .Padding(5)
        [
            SNew(SButton)
            .Text(FText::FromString("Export to file…"))
            .ToolTipText(FText::FromString("Save the extracted data to a .md/.txt file (format from plugin settings)"))
            .OnClicked(this, &SBPR_TabSwitcher::OnExportClicked)
        ]
    ];

    if (InArgs._InitialData.IsSet())
    {
        SetData(InArgs._InitialData.GetValue());
    }
    else
    {
        // Дефолт — пустой или ошибка
        CurrentAssetType = EAssetType::Unknown;
        RebuildTabsFromData();
        SwitchToTab(0);
    }
}

void SBPR_TabSwitcher::SetData(const FBPR_ExtractedData& InData)
{
    CurrentData = InData;
    CurrentAssetType = InData.AssetType;

    RebuildTabsFromData();
    SwitchToTab(0);  // или сохраняй предыдущий индекс, если нужно
}

void SBPR_TabSwitcher::RebuildTabsFromData()
{
    ClearTabs();

    int32 TabIndex = 0;

    switch (CurrentAssetType)
    {
    case EAssetType::Widget:
        AddTab(
            FText::FromString("Design"),
            CreateDesignTabContent(CurrentData.Design)
        );
        TabIndex++;

        AddTab(
            FText::FromString("Structure"),
            CreateStructureTabContent(CurrentData.Structure)
        );
        TabIndex++;

        AddTab(
            FText::FromString("Graph"),
            CreateGraphTabContent(CurrentData.Graph)
        );
        break;

    case EAssetType::Actor:
    case EAssetType::ActorComponent:
    case EAssetType::Material:
    case EAssetType::MaterialFunction:
    case EAssetType::InterfaceBP:
        AddTab(
            FText::FromString("Structure"),
            CreateStructureTabContent(CurrentData.Structure)
        );
        TabIndex++;

        AddTab(
            FText::FromString("Graph"),
            CreateGraphTabContent(CurrentData.Graph)
        );
        break;

    case EAssetType::Enum:
    case EAssetType::Structure:
        AddTab(
            FText::FromString("Structure"),
            CreateStructureTabContent(CurrentData.Structure)
        );
        break;

    default:  // Unknown или неподдержанный
        AddTab(
            FText::FromString("Info"),
            CreateErrorTabContent()
        );
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("BPR_TabSwitcher: Rebuilt %d tabs for type %d"), TabIndex + 1, (int32)CurrentAssetType);
}

void SBPR_TabSwitcher::ClearTabs()
{
    if (TabBarBox.IsValid())
    {
        TabBarBox->ClearChildren();
    }

    if (TabSwitcher.IsValid())
    {
        // SWidgetSwitcher не имеет ClearChildren, поэтому пересоздаём
        TabSwitcher.Reset();
        ChildSlot
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(5)
            [
                TabBarBox.ToSharedRef()
            ]
            + SVerticalBox::Slot()
            .FillHeight(1.f)
            [
                SAssignNew(TabSwitcher, SWidgetSwitcher)
            ]

            // M4.2: keep the export footer present after a tab rebuild
            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Right)
            .Padding(5)
            [
                SNew(SButton)
                .Text(FText::FromString("Export to file…"))
                .ToolTipText(FText::FromString("Save the extracted data to a .md/.txt file (format from plugin settings)"))
                .OnClicked(this, &SBPR_TabSwitcher::OnExportClicked)
            ]
        ];
    }

    // Сбрасываем кэшированные виджеты
    DesignTextWidget.Reset();
    StructureTextWidget.Reset();
    GraphTextWidget.Reset();
    ErrorTextWidget.Reset();
}

void SBPR_TabSwitcher::AddTab(const FText& Label, TSharedRef<SWidget> ContentWidget)
{
    if (!TabBarBox.IsValid() || !TabSwitcher.IsValid()) return;

    int32 Index = TabSwitcher->GetNumWidgets();

    // Кнопка
    TabBarBox->AddSlot()
        .AutoWidth()
        .Padding(2)
        [
            SNew(SButton)
            .Text(Label)
            .OnClicked(this, &SBPR_TabSwitcher::OnTabClicked, Index)
        ];

    // Контент
    TabSwitcher->AddSlot()
    [
        ContentWidget
    ];
}

TSharedRef<SWidget> SBPR_TabSwitcher::CreateDesignTabContent(const FText& Content) const
{
    TSharedRef<SBPR_TextWidget> Widget = SNew(SBPR_TextWidget);
    Widget->SetText(Content.IsEmpty() ? FText::FromString("No Design") : Content);
    // Widget->SetText(FText::FromString(
    //     "### Design (Widget Hierarchy)\n\n"
    //     "Иерархия виджетов, layout, anchors, bindings и анимации появятся здесь после реализации экстрактора."
    // ));
    return Widget;
}

TSharedRef<SWidget> SBPR_TabSwitcher::CreateStructureTabContent(const FText& Content) const
{
    TSharedRef<SBPR_TextWidget> Widget = SNew(SBPR_TextWidget);
    Widget->SetText(Content.IsEmpty() ? FText::FromString("No Structure Data") : Content);
    return Widget;
}

TSharedRef<SWidget> SBPR_TabSwitcher::CreateGraphTabContent(const FText& Content) const
{
    TSharedRef<SBPR_TextWidget> Widget = SNew(SBPR_TextWidget);
    Widget->SetText(Content.IsEmpty() ? FText::FromString("No Graph Data") : Content);
    return Widget;
}

TSharedRef<SWidget> SBPR_TabSwitcher::CreateErrorTabContent() const
{
    TSharedRef<SBPR_TextWidget> Widget = SNew(SBPR_TextWidget);

    FString ErrorMessage = FString::Printf(
        TEXT("### Blueprint ещё не поддерживается\n\n")
        TEXT("Тип ассета: %d\n")
        TEXT("Добавь case в RebuildTabsFromData() и экстрактор."),
        static_cast<int32>(CurrentAssetType)
    );

    Widget->SetText(FText::FromString(ErrorMessage));

    return Widget;
}

FReply SBPR_TabSwitcher::OnTabClicked(int32 TabIndex)
{
    SwitchToTab(TabIndex);
    return FReply::Handled();
}

void SBPR_TabSwitcher::SwitchToTab(int32 Index)
{
    if (TabSwitcher.IsValid() && Index >= 0 && Index < TabSwitcher->GetNumWidgets())
    {
        TabSwitcher->SetActiveWidgetIndex(Index);
    }
}

//==============================================================================
// M4.2 — Export to file
//==============================================================================
FString SBPR_TabSwitcher::BuildExportDocument() const
{
    // Keep only sections that carry real content (extractors emit "N/A" / "No Data found" placeholders).
    auto IsMeaningful = [](const FText& InText) -> bool
    {
        const FString S = InText.ToString().TrimStartAndEnd();
        return !S.IsEmpty() && S != TEXT("N/A") && S != TEXT("No Data found");
    };

    FString Document;
    if (IsMeaningful(CurrentData.Structure)) { Document += CurrentData.Structure.ToString(); Document += TEXT("\n\n"); }
    if (IsMeaningful(CurrentData.Graph))     { Document += CurrentData.Graph.ToString();     Document += TEXT("\n\n"); }
    if (IsMeaningful(CurrentData.Design))    { Document += CurrentData.Design.ToString();     Document += TEXT("\n\n"); }
    return Document.TrimEnd();
}

FReply SBPR_TabSwitcher::OnExportClicked()
{
    const FString Document = BuildExportDocument();
    if (Document.IsEmpty())
    {
        ShowNotification(FText::FromString("Nothing to export for this asset."), false);
        return FReply::Handled();
    }

    const UBPR_Settings* Settings = GetDefault<UBPR_Settings>();
    const bool bMarkdown = (Settings == nullptr) || (Settings->ExportFormat == EBPR_ExportFormat::Markdown);
    const FString Extension = bMarkdown ? TEXT("md") : TEXT("txt");
    const FString FileTypes = bMarkdown
        ? TEXT("Markdown Document (*.md)|*.md")
        : TEXT("Text Document (*.txt)|*.txt");

    const FString BaseName = CurrentData.AssetName.IsEmpty() ? TEXT("BlueprintExport") : CurrentData.AssetName;
    const FString DefaultFileName = FString::Printf(TEXT("%s.%s"), *BaseName, *Extension);

    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
    {
        ShowNotification(FText::FromString("Export failed (desktop platform unavailable)."), false);
        return FReply::Handled();
    }

    const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

    TArray<FString> OutFilenames;
    const bool bPicked = DesktopPlatform->SaveFileDialog(
        ParentWindowHandle,
        TEXT("Export Blueprint Data"),
        FPaths::ProjectSavedDir(),
        DefaultFileName,
        FileTypes,
        EFileDialogFlags::None,
        OutFilenames);

    if (!bPicked || OutFilenames.Num() == 0)
    {
        return FReply::Handled();   // user cancelled
    }

    const FString TargetPath = OutFilenames[0];
    const bool bWritten = FFileHelper::SaveStringToFile(
        Document, *TargetPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    if (bWritten)
    {
        ShowNotification(FText::FromString(
            FString::Printf(TEXT("Exported to %s"), *FPaths::GetCleanFilename(TargetPath))), true);
        UE_LOG(LogBlueprintReader, Log, TEXT("BPR: Exported data to %s"), *TargetPath);
    }
    else
    {
        ShowNotification(FText::FromString("Export failed (could not write file)."), false);
        UE_LOG(LogBlueprintReader, Warning, TEXT("BPR: Export failed for %s"), *TargetPath);
    }

    return FReply::Handled();
}

void SBPR_TabSwitcher::ShowNotification(const FText& Message, bool bSuccess) const
{
    FNotificationInfo Info(Message);
    Info.ExpireDuration = 4.0f;
    Info.bUseSuccessFailIcons = true;

    TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info);
    if (Item.IsValid())
    {
        Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
    }
}