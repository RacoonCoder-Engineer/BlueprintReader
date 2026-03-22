// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Core/BPR_Core.h"

class SBPR_TabSwitcher : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBPR_TabSwitcher)
		: _InitialData()
	{}
	SLATE_ARGUMENT(TOptional<FBPR_ExtractedData>, InitialData)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Sets the data after the widget is created */
	void SetData(const FBPR_ExtractedData& InData);

private:
	FReply OnTabClicked(int32 TabIndex);
	FReply OnStructureTabClicked(); //To Remove
	 FReply OnGraphTabClicked(); //To Remove
	void SwitchToIndex(int32 Index);
	void RebuildTabBarAndContent();

	/** Applies data if widgets are ready */
	void ApplyPendingData();
	
	TSharedPtr<SWidgetSwitcher> TabSwitcher;
	TSharedPtr<SBPR_TextWidget> StructureTextWidget;
	TSharedPtr<SBPR_TextWidget> GraphTextWidget;
	TSharedPtr<SBPR_TextWidget> DesignTextWidget;
	TSharedPtr<SHorizontalBox> TabBarBox;

	/** Data to be applied */
	TOptional<FBPR_ExtractedData> PendingData;
	EAssetType CurrentAssetType = EAssetType::Unknown;
};
