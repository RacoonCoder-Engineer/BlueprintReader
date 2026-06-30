// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Extractors/BPR_Extractor_Interface.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"

BPR_Extractor_Interface::BPR_Extractor_Interface()
{
    SetExtractorName(TEXT("Blueprint Interface"));
}

void BPR_Extractor_Interface::Process(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
    UBlueprint* BP = Cast<UBlueprint>(SelectedObject);
    if (!BP || BP->BlueprintType != BPTYPE_Interface)
    {
        SetErrorData(OutData, TEXT("Object is not a Blueprint Interface."));
        return;
    }

    FString Result;

    AppendSectionHeader(Result, TEXT("Blueprint Interface"));

    AppendKeyValue(Result, TEXT("Name"), BP->GetName());
    AppendKeyValue(Result, TEXT("Generated Class"), 
        BP->GeneratedClass ? BP->GeneratedClass->GetName() : TEXT("None"));

    AppendInterfaceFunctions(BP, Result);

    OutData.Structure = FText::FromString(Result);
    OutData.Graph     = FText::FromString(TEXT("N/A"));   // Interfaces don't have graphs in the same way
    OutData.Design    = FText::FromString(TEXT("N/A"));
    OutData.AssetType = EAssetType::Blueprint;            // или можно добавить отдельный тип позже
}

void BPR_Extractor_Interface::Extract(UObject* Asset, FBPR_ExtractedData& OutData)
{
    // Delegate to Process (the real implementation fills OutData).
    Process(Asset, OutData);
}

bool BPR_Extractor_Interface::CanHandleAsset(UObject* Asset) const
{
    if (UBlueprint* BP = Cast<UBlueprint>(Asset))
    {
        return BP->BlueprintType == BPTYPE_Interface;
    }
    return false;
}

// ===================================================================
// Protected Implementation
// ===================================================================

void BPR_Extractor_Interface::AppendInterfaceFunctions(UBlueprint* BP, FString& OutText) const
{
    if (!BP)
        return;

    AppendSectionHeader(OutText, TEXT("Interface Functions"));

    if (BP->FunctionGraphs.Num() == 0)
    {
        OutText += TEXT("_No functions defined in this interface._\n\n");
        return;
    }

    for (UEdGraph* Graph : BP->FunctionGraphs)
    {
        if (!Graph)
            continue;

        FString FunctionName = CleanName(Graph->GetName());
        AppendSectionHeader(OutText, FunctionName);

        UK2Node_FunctionEntry* EntryNode = FindFunctionEntryNode(Graph);
        UK2Node_FunctionResult* ResultNode = FindFunctionResultNode(Graph);

        // Input parameters (from FunctionEntry)
        if (EntryNode)
        {
            bool bHasInputs = false;
            for (UEdGraphPin* Pin : EntryNode->Pins)
            {
                if (!Pin || Pin->bHidden || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                    continue;
                if (Pin->Direction != EGPD_Output)
                    continue;

                FString ParamName = GetPinDisplayName(Pin);
                FString ParamType = GetPropertyTypeDetailedFromPin(Pin);   // будем добавлять позже

                AppendKeyValue(OutText, TEXT("Input"), FString::Printf(TEXT("%s : %s"), *ParamName, *ParamType), 1);
                bHasInputs = true;
            }
            if (!bHasInputs)
            {
                OutText += TEXT("  No input parameters\n");
            }
        }

        // Output parameters (from FunctionResult)
        if (ResultNode)
        {
            bool bHasOutputs = false;
            for (UEdGraphPin* Pin : ResultNode->Pins)
            {
                if (!Pin || Pin->bHidden || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                    continue;
                if (Pin->Direction != EGPD_Input)
                    continue;

                FString ParamName = GetPinDisplayName(Pin);
                FString ParamType = GetPropertyTypeDetailedFromPin(Pin);

                AppendKeyValue(OutText, TEXT("Output"), FString::Printf(TEXT("%s : %s"), *ParamName, *ParamType), 1);
                bHasOutputs = true;
            }
            if (!bHasOutputs)
            {
                OutText += TEXT("  No output parameters (void)\n");
            }
        }

        OutText += TEXT("\n");
    }
}

UK2Node_FunctionEntry* BPR_Extractor_Interface::FindFunctionEntryNode(const UEdGraph* Graph)
{
    if (!Graph) return nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
            return Entry;
    }
    return nullptr;
}

UK2Node_FunctionResult* BPR_Extractor_Interface::FindFunctionResultNode(const UEdGraph* Graph)
{
    if (!Graph) return nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UK2Node_FunctionResult* Result = Cast<UK2Node_FunctionResult>(Node))
            return Result;
    }
    return nullptr;
}