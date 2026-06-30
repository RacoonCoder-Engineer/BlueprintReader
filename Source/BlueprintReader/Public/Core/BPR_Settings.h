// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BPR_Settings.generated.h"

/** File format used when exporting extracted data to disk (M4.2). */
UENUM()
enum class EBPR_ExportFormat : uint8
{
	Markdown  UMETA(DisplayName = "Markdown (.md)"),
	PlainText UMETA(DisplayName = "Plain Text (.txt)")
};

/**
 * Project-level settings for the Blueprint Reader plugin.
 * Appears under Project Settings -> Plugins -> Blueprint Reader.
 * Plain C++ extractors read these via GetDefault<UBPR_Settings>().
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Blueprint Reader"))
class BLUEPRINTREADER_API UBPR_Settings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UBPR_Settings();

	/** Settings tree location: groups this under the "Plugins" section. */
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** File format chosen by the "Export to file" action. */
	UPROPERTY(config, EditAnywhere, Category = "Output", meta = (DisplayName = "Export Format"))
	EBPR_ExportFormat ExportFormat = EBPR_ExportFormat::Markdown;

	/** When enabled, the widget-tree traversal stops at WidgetRecursionDepth. */
	UPROPERTY(config, EditAnywhere, Category = "Widget", meta = (DisplayName = "Restrict Widget Recursion Depth"))
	bool bRestrictWidgetRecursionDepth = true;

	/** Maximum widget-tree recursion depth (used only when the restriction is on). */
	UPROPERTY(config, EditAnywhere, Category = "Widget", meta = (DisplayName = "Widget Recursion Depth", ClampMin = "1", ClampMax = "50", EditCondition = "bRestrictWidgetRecursionDepth"))
	int32 WidgetRecursionDepth = 10;
};
