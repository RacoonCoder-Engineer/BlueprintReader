// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Core/BPR_Core.h"

class SBPR_TextWidget;

class SBPR_TabSwitcher : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBPR_TabSwitcher)
		: _InitialData()
	{}
	SLATE_ARGUMENT(TOptional<FBPR_ExtractedData>, InitialData)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Устанавливает данные и перестраивает табы */
	void SetData(const FBPR_ExtractedData& InData);

private:
	FReply OnTabClicked(int32 TabIndex);

	void SwitchToTab(int32 Index);

	void ClearTabs();

	void RebuildTabsFromData();

	// M4.2: Export to file
	FReply OnExportClicked();
	/** Concatenates the meaningful (non-N/A) sections of CurrentData into one document. */
	FString BuildExportDocument() const;
	/** Shows a transient editor notification (success/fail) after an export attempt. */
	void ShowNotification(const FText& Message, bool bSuccess) const;

	// Helper-фабрики для контента табов (расширяй по мере нужды)
	TSharedRef<SWidget> CreateDesignTabContent(const FText& Content) const;
	TSharedRef<SWidget> CreateStructureTabContent(const FText& Content) const;
	TSharedRef<SWidget> CreateGraphTabContent(const FText& Content) const;
	TSharedRef<SWidget> CreateErrorTabContent() const;

	// Добавляет кнопку + слот в switcher
	void AddTab(const FText& Label, TSharedRef<SWidget> ContentWidget);

	TSharedPtr<SWidgetSwitcher> TabSwitcher;
	TSharedPtr<SHorizontalBox> TabBarBox;

	TSharedPtr<SBPR_TextWidget> DesignTextWidget;   // кэшируем, если нужно обновлять позже
	TSharedPtr<SBPR_TextWidget> StructureTextWidget;
	TSharedPtr<SBPR_TextWidget> GraphTextWidget;
	TSharedPtr<SBPR_TextWidget> ErrorTextWidget;

	FBPR_ExtractedData CurrentData;
	EAssetType CurrentAssetType = EAssetType::Unknown;
};