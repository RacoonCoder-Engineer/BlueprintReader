// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Extractors/BPR_Extractor_ActorComponent.h"
#include "Engine/Blueprint.h"
#include "Components/ActorComponent.h"
#include "UObject/UnrealType.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Switch.h"
#include "K2Node_Tunnel.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_Knot.h"
#include "K2Node_MathExpression.h"
#include "K2Node_Select.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_CastByteToEnum.h"
#include "EdGraphSchema_K2.h"


BPR_Extractor_ActorComponent::BPR_Extractor_ActorComponent()
{
    SetExtractorName(TEXT("ActorComponent"));
}
BPR_Extractor_ActorComponent::~BPR_Extractor_ActorComponent() {}

void BPR_Extractor_ActorComponent::Process(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
    ProcessComponent(SelectedObject, OutData);
}

bool BPR_Extractor_ActorComponent::CanHandleAsset(UObject* Asset) const
{
    UBlueprint* BP = Cast<UBlueprint>(Asset);
    return BP && BP->GeneratedClass && BP->GeneratedClass->IsChildOf(UActorComponent::StaticClass());
}

//---------------------------------------------
// Main entry point
//---------------------------------------------
void BPR_Extractor_ActorComponent::ProcessComponent(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
    if (!SelectedObject)
    {
        LogError(TEXT("SelectedObject is null!"));
        OutData.Structure = FText::FromString("Error: SelectedObject is null.");
        OutData.Graph = FText::FromString("Error: SelectedObject is null.");
        return;
    }

    UBlueprint* Blueprint = Cast<UBlueprint>(SelectedObject);
    if (!Blueprint)
    {
        LogWarning(TEXT("Selected object is not a Blueprint asset."));
        OutData.Structure = FText::FromString("Warning: Selected object is not a Blueprint.");
        OutData.Graph = FText::FromString("Warning: Selected object is not a Blueprint.");
        return;
    }

    FString TmpStructure = FString::Printf(TEXT("# Blueprint Structure: %s\n\n"), *Blueprint->GetName());
    FString TmpGraph = FString::Printf(TEXT("# Graphs Export for Component: %s\n\n"), *Blueprint->GetName());

    AppendBlueprintInfo(Blueprint, TmpStructure);
    AppendVariables(Blueprint, TmpStructure);
    AppendReplicationInfo(Blueprint->GeneratedClass, TmpStructure);
    AppendGraphs(Blueprint, TmpGraph);

    OutData.Structure = FText::FromString(TmpStructure);
    OutData.Graph = FText::FromString(TmpGraph);
    OutData.Design = FText::FromString(TEXT("N/A"));
    OutData.AssetType = EAssetType::ActorComponent;
}