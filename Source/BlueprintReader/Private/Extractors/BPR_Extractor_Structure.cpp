// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Extractors/BPR_Extractor_Structure.h"
#include "UObject/UnrealType.h"

BPR_Extractor_Structure::BPR_Extractor_Structure()
{
	SetExtractorName(TEXT("Structure"));
}

void BPR_Extractor_Structure::Process(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
	FString StructureText;
	FString GraphText = TEXT("## Graphs\n\nThis asset type has no graphs.\n");

	UScriptStruct* Struct = Cast<UScriptStruct>(SelectedObject);
	if (!Struct)
	{
		// Try to get the struct from the object's class if it's not a direct UScriptStruct
		if (SelectedObject)
		{
			Struct = Cast<UScriptStruct>(SelectedObject->GetClass()->GetSuperStruct());
		}
	}

	if (!Struct)
	{
		StructureText = TEXT("Error: Object is not a valid UScriptStruct.");
		
		OutData.Structure = FText::FromString(StructureText);
		OutData.Graph     = FText::FromString(GraphText);
		OutData.Design    = FText::FromString(TEXT("N/A"));
		OutData.AssetType = EAssetType::Structure;
		return;
	}

	// Structure Section
	AppendSectionHeader(StructureText, TEXT("STRUCTURE"));

	StructureText += FString::Printf(TEXT("**Name:** %s\n"), *Struct->GetName());

	FString StructDescription = Struct->GetToolTipText().ToString();
	if (!StructDescription.IsEmpty())
	{
		StructureText += FString::Printf(TEXT("**Description:** %s\n"), *StructDescription);
	}
	StructureText += TEXT("\n");

	// Fields Table
	AppendSectionHeader(StructureText, TEXT("Fields"));

	BeginMarkdownTable(StructureText, {
		TEXT("Field"),
		TEXT("Type"),
		TEXT("Description")
	});

	AppendStructInfo(Struct, StructureText);

	OutData.Structure = FText::FromString(StructureText);
	OutData.Graph     = FText::FromString(GraphText);
	OutData.Design    = FText::FromString(TEXT("N/A"));
	OutData.AssetType = EAssetType::Structure;
}

void BPR_Extractor_Structure::AppendStructInfo(const UScriptStruct* Struct, FString& OutText)
{
	if (!Struct)
	{
		return;
	}

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property)
		{
			continue;
		}

		FString FieldName   = CleanName(Property->GetName());
		FString FieldType   = GetPropertyTypeDetailed(Property);
		FString Description = GetPropertyDescription(Property);

		AppendTableRow(OutText, { FieldName, FieldType, Description });
	}

	OutText += TEXT("\n");
}