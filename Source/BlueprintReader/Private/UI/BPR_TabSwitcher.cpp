// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "UI/BPR_TabSwitcher.h"

#include "InstalledPlatformInfo.h"
#include "UI/BPR_TextWidget.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

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