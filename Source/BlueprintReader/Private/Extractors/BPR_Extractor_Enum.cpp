// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Extractors/BPR_Extractor_Enum.h"
#include "UObject/UnrealType.h"

BPR_Extractor_Enum::BPR_Extractor_Enum()
{
    // Set short name for logging
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

    // === Structure Section ===
    AppendSectionHeader(StructureText, TEXT("Enum"));
    StructureText += FString::Printf(TEXT("**Name:** %s\n\n"), *Enum->GetName());

    AppendEnumEntries(Enum, StructureText);

    OutData.Structure = FText::FromString(StructureText);
    OutData.Graph     = FText::FromString(GraphText);
    OutData.Design    = FText::FromString(TEXT("N/A"));        // Enums have no Design
    OutData.AssetType = EAssetType::Enum;
}

void BPR_Extractor_Enum::AppendEnumEntries(UEnum* Enum, FString& OutText) const
{
    if (!Enum)
    {
        return;
    }

    const int32 NumEnums = Enum->NumEnums();

    for (int32 i = 0; i < NumEnums; ++i)
    {
        FString Name       = Enum->GetNameStringByIndex(i);
        int64   Value      = Enum->GetValueByIndex(i);
        FString DisplayName = Enum->GetDisplayNameTextByIndex(i).ToString();

        // Clean up display name if it's the same as the name
        if (DisplayName.IsEmpty() || DisplayName == Name)
        {
            DisplayName = TEXT("-");
        }

        OutText += FString::Printf(TEXT("%d) %s = %lld    %s\n"),
            i, *Name, Value, *DisplayName);
    }

    OutText += TEXT("\n");
}