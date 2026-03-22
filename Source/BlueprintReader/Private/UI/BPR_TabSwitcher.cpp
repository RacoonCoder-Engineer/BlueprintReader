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

        // Tab Button Bar (будет динамически заполняться в Rebuild)
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
        UE_LOG(LogTemp, Warning, TEXT("BPR_TabSwitcher: TabBarBox invalid"));
        return;
    }

    TabBarBox->ClearChildren();

    // Полностью пересоздаём TabSwitcher, чтобы очистить старые слоты
    TabSwitcher.Reset();  // старый уничтожается

    bool bIsWidget = (CurrentAssetType == EAssetType::Widget);
    int32 TabIndex = 0;

    // Создаём новый switcher
    SAssignNew(TabSwitcher, SWidgetSwitcher);

    // Design (только для виджетов)
    if (bIsWidget)
    {
        TabBarBox->AddSlot()
            .AutoWidth().Padding(2)
            [
                SNew(SButton)
                .Text(FText::FromString("Design"))
                .OnClicked(this, &SBPR_TabSwitcher::OnTabClicked, TabIndex)
            ];

        TabSwitcher->AddSlot()
            [
                SAssignNew(DesignTextWidget, SBPR_TextWidget)
            ];

        ++TabIndex;
    }

    // Structure
    TabBarBox->AddSlot()
        .AutoWidth().Padding(2)
        [
            SNew(SButton)
            .Text(FText::FromString("Structure"))
            .OnClicked(this, &SBPR_TabSwitcher::OnTabClicked, TabIndex)
        ];

    TabSwitcher->AddSlot()
        [
            SAssignNew(StructureTextWidget, SBPR_TextWidget)
        ];

    ++TabIndex;

    // Graph
    TabBarBox->AddSlot()
        .AutoWidth().Padding(2)
        [
            SNew(SButton)
            .Text(FText::FromString("Graph"))
            .OnClicked(this, &SBPR_TabSwitcher::OnTabClicked, TabIndex)
        ];

    TabSwitcher->AddSlot()
        [
            SAssignNew(GraphTextWidget, SBPR_TextWidget)
        ];

    UE_LOG(LogTemp, Log, TEXT("BPR_TabSwitcher: Rebuilt %d tabs (Widget mode: %d)"), TabIndex + 1, bIsWidget);

    // Важно! Обновляем слот в ChildSlot, чтобы новый TabSwitcher отобразился
    // Но поскольку ChildSlot уже содержит SVerticalBox, нужно перестроить весь ChildSlot
    // Поэтому лучше сделать так:
    // Пересоздаём весь контент в ChildSlot
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
}

void SBPR_TabSwitcher::SetData(const FBPR_ExtractedData& InData)
{
    PendingData = InData;

    // Обновляем тип и перестраиваем табы сразу
    if (InData.AssetType != CurrentAssetType)
    {
        CurrentAssetType = InData.AssetType;
        RebuildTabBarAndContent();
    }

    // Теперь проверяем, готовы ли все нужные виджеты
    bool WidgetsReady = StructureTextWidget.IsValid() && GraphTextWidget.IsValid();
    bool DesignReadyOrNotNeeded = (CurrentAssetType != EAssetType::Widget) || DesignTextWidget.IsValid();

    if (WidgetsReady && DesignReadyOrNotNeeded)
    {
        UE_LOG(LogTemp, Log, TEXT("BPR_TabSwitcher: Widgets ready, applying immediately"));
        ApplyPendingData();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("BPR_TabSwitcher: Widgets not ready yet, deferring apply"));
    }
}

void SBPR_TabSwitcher::ApplyPendingData()
{
    if (!PendingData.IsSet())
    {
        UE_LOG(LogTemp, Warning, TEXT("BPR_TabSwitcher: No pending data to apply"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("BPR_TabSwitcher: Applying pending data NOW"));

    if (StructureTextWidget.IsValid())
        StructureTextWidget->SetText(PendingData->Structure);

    if (GraphTextWidget.IsValid())
        GraphTextWidget->SetText(PendingData->Graph);

    if (DesignTextWidget.IsValid())
    {
        // Заглушка для Design
        FText DesignText = FText::FromString(
            "### Design (Widget Hierarchy)\n\n"
            "Иерархия виджетов (CanvasPanel → VerticalBox → Button/Text/Image и т.д.),\n"
            "anchors, offsets, padding, alignment, visibility, анимации, bindings\n"
            "появятся здесь после реализации экстрактора для UWidgetBlueprint.\n\n"
            "Сейчас используется Actor-экстрактор как временная заглушка."
        );
        DesignTextWidget->SetText(DesignText);
    }

    PendingData.Reset();

    UE_LOG(LogTemp, Log, TEXT("BPR_TabSwitcher: Data applied successfully"));
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