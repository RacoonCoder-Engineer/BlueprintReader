// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "UI/BPR_TabSwitcher.h"
#include "UI/BPR_TextWidget.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

void SBPR_TabSwitcher::Construct(const FArguments& InArgs)
{
    UE_LOG(LogTemp, Log, TEXT("BPR_TabSwitcher: Construct starting"));

    if (InArgs._InitialData.IsSet())
    {
        PendingData = InArgs._InitialData.GetValue();
        UE_LOG(LogTemp, Log, TEXT("BPR_TabSwitcher: Initial data provided"));
        if (PendingData.IsSet())
        {
            CurrentAssetType = PendingData->AssetType;
        }
    }

    ChildSlot
    [
        SNew(SVerticalBox)

        // Tab Button Bar (will be dynamically populated on Rebuild)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SAssignNew(TabBarBox, SHorizontalBox)
        ]

        // Tabs Content
        + SVerticalBox::Slot()
        .FillHeight(1.f)
        [
            SAssignNew(TabSwitcher, SWidgetSwitcher)
        ]
    ];

    RebuildTabBarAndContent();

    SwitchToIndex(0);

    UE_LOG(LogTemp, Log, TEXT("BPR_TabSwitcher: Construct finished"));

    if (PendingData.IsSet())
    {
        ApplyPendingData();
    }
}

void SBPR_TabSwitcher::RebuildTabBarAndContent()
{
    if (!TabBarBox.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("TabBarBox invalid"));
        return;
    }

    TabBarBox->ClearChildren();

    // Полностью пересоздаём TabSwitcher
    TabSwitcher.Reset();  // уничтожаем старый

    SAssignNew(TabSwitcher, SWidgetSwitcher);

    int32 Index = 0;

    switch (CurrentAssetType)
    {
    case EAssetType::Enum:
    case EAssetType::Structure:
        AddTabButton("Structure", Index);
        TabSwitcher->AddSlot()[SAssignNew(StructureTextWidget, SBPR_TextWidget)];
        break;

    case EAssetType::Actor:
    case EAssetType::ActorComponent:
    case EAssetType::Material:
    case EAssetType::MaterialFunction:
    case EAssetType::InterfaceBP:
        AddTabButton("Structure", Index++);
        TabSwitcher->AddSlot()[SAssignNew(StructureTextWidget, SBPR_TextWidget)];

        AddTabButton("Graph", Index);
        TabSwitcher->AddSlot()[SAssignNew(GraphTextWidget, SBPR_TextWidget)];
        break;

    case EAssetType::Widget:
        AddTabButton("Design", Index++);
        TabSwitcher->AddSlot()[SAssignNew(DesignTextWidget, SBPR_TextWidget)];

        AddTabButton("Structure", Index++);
        TabSwitcher->AddSlot()[SAssignNew(StructureTextWidget, SBPR_TextWidget)];

        AddTabButton("Graph", Index);
        TabSwitcher->AddSlot()[SAssignNew(GraphTextWidget, SBPR_TextWidget)];
        break;

    default:  // Unknown
        AddTabButton("Info", Index);
        TabSwitcher->AddSlot()[SAssignNew(ErrorTextWidget, SBPR_TextWidget)];

        if (ErrorTextWidget.IsValid())
        {
            ErrorTextWidget->SetText(FText::FromString(
                "### Blueprint ещё не добавлен на поддержку\n\n"
                "Тип: " + FString::FromInt(static_cast<int32>(CurrentAssetType)) + "\n\n"
                "Добавь case в RebuildTabBarAndContent()"
            ));
        }
        break;
    }

    // Перестраиваем весь ChildSlot с новым TabSwitcher
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
            TabSwitcher.ToSharedRef()
        ]
    ];

    UE_LOG(LogTemp, Log, TEXT("Rebuilt tabs for type %d"), static_cast<int32>(CurrentAssetType));

    // После перестройки можно сразу применить данные
    if (PendingData.IsSet())
    {
        ApplyPendingData();
    }
}

void SBPR_TabSwitcher::SetData(const FBPR_ExtractedData& InData)
{
    PendingData = InData;

    if (InData.AssetType != CurrentAssetType)
    {
        CurrentAssetType = InData.AssetType;
        RebuildTabBarAndContent();
    }

    // Применяем данные сразу, если всё готово
    if (StructureTextWidget.IsValid() && GraphTextWidget.IsValid() &&
        (CurrentAssetType != EAssetType::Widget || DesignTextWidget.IsValid()) &&
        (CurrentAssetType != EAssetType::Unknown || ErrorTextWidget.IsValid()))
    {
        ApplyPendingData();
    }
}

void SBPR_TabSwitcher::ApplyPendingData()
{
    if (!PendingData.IsSet()) return;

    if (CurrentAssetType == EAssetType::Unknown && ErrorTextWidget.IsValid())
    {
        PendingData.Reset();
        return;
    }

    if (StructureTextWidget.IsValid())
        StructureTextWidget->SetText(PendingData->Structure);

    if (GraphTextWidget.IsValid())
        GraphTextWidget->SetText(PendingData->Graph);

    if (DesignTextWidget.IsValid())
    {
        DesignTextWidget->SetText(FText::FromString(
            "### Design (Widget Hierarchy)\n\n"
            "Иерархия виджетов появится здесь после реализации Widget-экстрактора..."
        ));
    }

    PendingData.Reset();
}

FReply SBPR_TabSwitcher::OnTabClicked(int32 Index)
{
    SwitchToIndex(Index);
    return FReply::Handled();
}

void SBPR_TabSwitcher::SwitchToIndex(int32 Index)
{
    if (TabSwitcher.IsValid())
    {
        TabSwitcher->SetActiveWidgetIndex(Index);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("BPR_TabSwitcher: Cannot switch - TabSwitcher is invalid"));
    }
}

void SBPR_TabSwitcher::AddTabButton(const FString& Label, int32 Index)
{
    TabBarBox->AddSlot()
        .AutoWidth()
        .Padding(2)
        [
            SNew(SButton)
            .Text(FText::FromString(Label))
            .OnClicked(this, &SBPR_TabSwitcher::OnTabClicked, Index)
        ];
}