// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"

BLUEPRINTREADER_API DECLARE_LOG_CATEGORY_EXTERN(LogBlueprintReader, Log, All);

//---------------------------------------------------------------------- 
//  Types of Assets Supported
//---------------------------------------------------------------------- 
enum class EAssetType : uint8
{
	Unknown,
	Blueprint,
	Actor,
	Widget,
	Material,
	MaterialFunction,
	ActorComponent,
	Enum,
	Structure,
	InterfaceBP
};

//---------------------------------------------------------------------- 
//  Data for unsupported Assets
//---------------------------------------------------------------------- 
struct FUnsupportedAssetInfo
{
	FText Title;
	FText MainMessage;
	FText SubMessage;
	FString GitHubURL;
	FText ButtonText;
};

//---------------------------------------------------------------------- 
//  Structure for Data Output
//---------------------------------------------------------------------- 
struct FBPR_ExtractedData
{
	FText Structure = FText::FromString(TEXT("No Data found"));
	FText Graph     = FText::FromString(TEXT("No Data found"));
	FText Design    = FText::FromString(TEXT("No Data found"));
	EAssetType AssetType = EAssetType::Unknown;
	FString AssetName;   // M4.2: source asset name, used as the default export filename
};

enum class EOutputFormat : uint8
{
	HumanReadable,     // Beautiful, multi-line output with indentation (for humans)
	Compact,           // Balanced format, saves tokens for LLMs
	Minimal            // Most compact format (best for LLMs)
};