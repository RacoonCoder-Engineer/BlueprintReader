// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Extractors/BPR_Extractor_Enum.h"
#include "UObject/UnrealType.h"

BPR_Extractor_Enum::BPR_Extractor_Enum()
{
	SetExtractorName(TEXT("Enum"));
}

void BPR_Extractor_Enum::Process(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
	FString StructureText;
	FString GraphText = TEXT("## Graphs\n\nThis asset type has no graphs.\n");

	UEnum* Enum = Cast<UEnum>(SelectedObject);
	if (!Enum)
	{
		StructureText = TEXT("Error: Object is not a valid UEnum.");
		
		OutData.Structure = FText::FromString(StructureText);
		OutData.Graph     = FText::FromString(GraphText);
		OutData.Design    = FText::FromString(TEXT("N/A"));
		OutData.AssetType = EAssetType::Enum;
		return;
	}

	// Structure Section
	AppendSectionHeader(StructureText, TEXT("ENUM"));

	StructureText += FString::Printf(TEXT("**Name:** %s\n"), *Enum->GetName());

	FString EnumDescription = Enum->GetDisplayNameText().ToString();
	if (!EnumDescription.IsEmpty() && EnumDescription != Enum->GetName())
	{
		StructureText += FString::Printf(TEXT("**Description:** %s\n"), *EnumDescription);
	}
	StructureText += TEXT("\n");

	// Enumerators Table
	AppendSectionHeader(StructureText, TEXT("Enumerators"));

	BeginMarkdownTable(StructureText, {
		TEXT("Index"),
		TEXT("Name"),
		TEXT("Value"),
		TEXT("Description")
	});

	AppendEnumEntries(Enum, StructureText);

	OutData.Structure = FText::FromString(StructureText);
	OutData.Graph     = FText::FromString(GraphText);
	OutData.Design    = FText::FromString(TEXT("N/A"));
	OutData.AssetType = EAssetType::Enum;
}

void BPR_Extractor_Enum::AppendEnumEntries(const UEnum* Enum, FString& OutText)
{
	if (!Enum)
	{
		return;
	}

	// Eliminate the artificial _MAX element
	const int32 Num = Enum->NumEnums() - 1;

	for (int32 i = 0; i < Num; ++i)
	{
		FString Name        = Enum->GetNameStringByIndex(i);
		int64   Value       = Enum->GetValueByIndex(i);
		FString Description = Enum->GetToolTipTextByIndex(i).ToString();

		// If the description is empty, put "-"
		if (Description.IsEmpty())
		{
			Description = TEXT("-");
		}

		AppendTableRow(OutText, {
			FString::FromInt(i),           // Index
			Name,                          // Name
			FString::Printf(TEXT("%lld"), Value), // Value
			Description                    // Description
		});
	}

	OutText += TEXT("\n");
}