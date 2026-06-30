// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Extractors/BPR_Extractor_Actor.h"
#include "Engine/Blueprint.h"
#include "UObject/UnrealType.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

BPR_Extractor_Actor::BPR_Extractor_Actor()
{
	SetExtractorName(TEXT("Actor"));
}

void BPR_Extractor_Actor::Process(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
	FString StructureText;
	FString GraphText = TEXT("## Graphs\n\n");

	UBlueprint* Blueprint = Cast<UBlueprint>(SelectedObject);
	if (!Blueprint)
	{
		StructureText = TEXT("Error: Selected object is not a Blueprint.");
		GraphText += TEXT("Error: Selected object is not a Blueprint.\n");

		OutData.Structure = FText::FromString(StructureText);
		OutData.Graph     = FText::FromString(GraphText);
		OutData.Design    = FText::FromString(TEXT("N/A"));
		OutData.AssetType = EAssetType::Actor;
		return;
	}

	// === Structure Section ===
	AppendSectionHeader(StructureText, TEXT("ACTOR BLUEPRINT"));

	AppendClassInfo(Blueprint, StructureText);
	AppendBlueprintInfo(Blueprint, StructureText);     // from base
	AppendVariables(Blueprint, StructureText);         // from base
	AppendActorComponents(Blueprint, StructureText);
	AppendActorTags(Blueprint, StructureText);
	AppendReplicationInfo(Blueprint->GeneratedClass, StructureText); // from base

	// === Graphs Section ===
	AppendGraphs(Blueprint, GraphText);                // from base

	OutData.Structure = FText::FromString(StructureText);
	OutData.Graph     = FText::FromString(GraphText);
	OutData.Design    = FText::FromString(TEXT("N/A"));
	OutData.AssetType = EAssetType::Actor;

	LogMessage(FString::Printf(TEXT("Actor extraction completed for %s"), *Blueprint->GetName()));
}

bool BPR_Extractor_Actor::CanHandleAsset(UObject* Asset) const
{
    if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
    {
        if (UClass* Generated = Blueprint->GeneratedClass)
        {
            return Generated->IsChildOf(AActor::StaticClass());
        }
    }
    return false;
}

// ===================================================================
// Actor-specific Structure
// ===================================================================

void BPR_Extractor_Actor::AppendClassInfo(const UBlueprint* Blueprint, FString& OutText) const
{
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return;
	}

	UClass* Class = Blueprint->GeneratedClass;

	AppendSectionHeader(OutText, TEXT("Class Info"));

	FString ParentClass = Class->GetSuperClass() ? Class->GetSuperClass()->GetName() : TEXT("None");
	FString NativeParent = Class->GetSuperClass() ? Class->GetSuperClass()->GetPathName() : TEXT("None");

	OutText += FString::Printf(TEXT("**Class:** %s\n"), *Class->GetName());
	OutText += FString::Printf(TEXT("**Parent Class:** %s\n"), *ParentClass);
	OutText += FString::Printf(TEXT("**Native Parent Path:** %s\n"), *NativeParent);

	bool bIsPlaceable = (Class->ClassFlags & CLASS_NotPlaceable) == 0;
	OutText += FString::Printf(TEXT("**Placeable:** %s\n"), bIsPlaceable ? TEXT("Yes") : TEXT("No"));

	OutText += TEXT("\n");
}

void BPR_Extractor_Actor::AppendActorComponents(const UBlueprint* Blueprint, FString& OutText) const
{
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return;
	}

	AppendSectionHeader(OutText, TEXT("Actor Components"));

	UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
	AActor* ActorCDO = Cast<AActor>(CDO);

	if (!ActorCDO)
	{
		OutText += TEXT("No Actor CDO available.\n\n");
		return;
	}

	TArray<UActorComponent*> Components;
	ActorCDO->GetComponents(Components);

	for (UActorComponent* Comp : Components)
	{
		if (!Comp) continue;

		FString CompInfo = FormatComponentInfo(Comp);
		OutText += FString::Printf(TEXT("- **%s**: %s\n"), *Comp->GetName(), *CompInfo);
	}

	OutText += TEXT("\n");
}

void BPR_Extractor_Actor::AppendActorTags(const UBlueprint* Blueprint, FString& OutText) const
{
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return;
	}

	UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
	AActor* ActorCDO = Cast<AActor>(CDO);

	if (!ActorCDO || ActorCDO->Tags.Num() == 0)
	{
		return;
	}

	AppendSectionHeader(OutText, TEXT("Actor Tags"));

	for (const FName& Tag : ActorCDO->Tags)
	{
		OutText += FString::Printf(TEXT("- %s\n"), *Tag.ToString());
	}

	OutText += TEXT("\n");
}

FString BPR_Extractor_Actor::FormatComponentInfo(UActorComponent* Component)
{
	if (!Component)
	{
		return TEXT("None");
	}

	FString Result;

	if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
	{
		FString AttachParent = SceneComp->GetAttachParent() 
			? SceneComp->GetAttachParent()->GetName() 
			: TEXT("None");

		Result += FString::Printf(TEXT("AttachParent: %s, "), *AttachParent);

		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(SceneComp))
		{
			Result += FString::Printf(TEXT("Collision: %s, SimPhysics: %s, Mobility: %s"),
				*UEnum::GetValueAsString(Prim->GetCollisionEnabled()),
				Prim->IsSimulatingPhysics() ? TEXT("true") : TEXT("false"),
				*UEnum::GetValueAsString(Prim->Mobility));
		}
	}

	return Result.IsEmpty() ? TEXT("None") : Result;
}

// ===================================================================
// Node Name Override (Actor-specific)
// ===================================================================

FString BPR_Extractor_Actor::GetReadableNodeName(const UEdGraphNode* Node) const
{
	if (!Node)
	{
		return TEXT("None");
	}

	// You can add Actor-specific enhancements here in the future
	// For now we use the base implementation
	return BPR_Extractor_Object::GetReadableNodeName(Node);
}