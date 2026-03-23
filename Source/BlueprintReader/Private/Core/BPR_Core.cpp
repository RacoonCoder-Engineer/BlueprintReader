// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Core/BPR_Core.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Blueprint.h"
#include "Extractors/BPR_Extractor_Actor.h"
#include "Extractors/BPR_Extractor_ActorComponent.h"
#include "Extractors/BPR_Extractor_Material.h"
#include "Extractors/BPR_Extractor_MaterialFunction.h"
#include "Extractors/BPR_Extractor_Enum.h"
#include "Extractors/BPR_Extractor_Structure.h"
#include "Extractors/BPR_Extractor_InterfaceBP.h"
#include "Extractors/BPR_Extractor_Widget.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Logging/LogMacros.h"

//==============================================================================
//  IsSupportedAsset
//==============================================================================
// Sets CachedType and returns true if supported.
// Beware: this method exposes CachedType as a side effect.
//==============================================================================
bool BPR_Core::IsSupportedAsset(UObject* Object)
{
    CachedType = EAssetType::Unknown;
    if (!Object) return false;

    if (UBlueprint* BP = Cast<UBlueprint>(Object))
    {
        if (BP->GeneratedClass)
        {
            if (BP->GeneratedClass->IsChildOf(AActor::StaticClass()))
            {
                CachedType = EAssetType::Actor;
                return true;
            }
            
            else if (BP->GeneratedClass->IsChildOf(UUserWidget::StaticClass()))  // New String
            {
                CachedType = EAssetType::Widget;
                return true;
            }
            
            else if (BP->GeneratedClass->IsChildOf(UActorComponent::StaticClass()))
            {
                CachedType = EAssetType::ActorComponent;
                return true;
            }
                     
        }
    }

    if (Cast<UMaterial>(Object) || Cast<UMaterialInstance>(Object))
    {
        CachedType = EAssetType::Material;
        return true;
    }
    if (Cast<UMaterialFunction>(Object)) { CachedType = EAssetType::MaterialFunction; return true; }
    if (Cast<UEnum>(Object)) { CachedType = EAssetType::Enum; return true; }
    if (Cast<UScriptStruct>(Object)) { CachedType = EAssetType::Structure; return true; }
    if (Cast<UBlueprint>(Object)) { CachedType = EAssetType::InterfaceBP; return true; }

    return false;
}

//==============================================================================
//  ExtractorSelector
//==============================================================================
void BPR_Core::ExtractorSelector(UObject* Object)
{
    if (!Object) return;

    if (CachedType == EAssetType::Unknown)
    {
        IsSupportedAsset(Object);
    }

    switch (CachedType)
    {
    case EAssetType::ActorComponent:
        {
            UE_LOG(LogTemp, Log, TEXT("BPR_Core: Using ActorComponent extractor"));
            BPR_Extractor_ActorComponent Extractor;
            Extractor.ProcessComponent(Object, TextData);
            break;
        }

    case EAssetType::Actor:
        {
            UE_LOG(LogTemp, Log, TEXT("BPR_Core: Using Actor extractor"));
            BPR_Extractor_Actor Extractor;
            Extractor.ProcessActor(Object, TextData);
            break;
        }
    case EAssetType::Widget:
        {
            UE_LOG(LogTemp, Log, TEXT("BPR_Core: Using Widget extractor"));
            BPR_Extractor_Widget Extractor;         
            Extractor.ProcessWidget(Object, TextData);
            break;
        }

    case EAssetType::Enum:
        {
            UE_LOG(LogTemp, Log, TEXT("BPR_Core: Using Enum extractor"));
            BPR_Extractor_Enum Extractor;
            Extractor.ProcessEnum(Object, TextData);
            break;
        }
        
    case EAssetType::Structure:
        {
            UE_LOG(LogTemp, Log, TEXT("BPR_Core: Using Structure extractor"));
            BPR_Extractor_Structure Extractor;
            Extractor.ProcessStructure(Object, TextData);
            break;
        }
        
    case EAssetType::InterfaceBP:
        {
            UE_LOG(LogTemp, Log, TEXT("BPR_Core: Using InterfaceBP extractor"));
            BPR_Extractor_InterfaceBP Extractor;
            Extractor.ProcessInterfaceBP(Object, TextData);      
            break;
        }
       
    case EAssetType::Material:
        {
            UE_LOG(LogTemp, Log, TEXT("BPR_Core: Using Material extractor"));
            BPR_Extractor_Material Extractor;
            Extractor.ProcessMaterial(Object, TextData);
            break;
        }
        
    case EAssetType::MaterialFunction:
        {
            UE_LOG(LogTemp, Log, TEXT("BPR_Core: Using MaterialFunction extractor"));
            BPR_Extractor_MaterialFunction Extractor;
            Extractor.ProcessMaterialFunction(Object, TextData);
            break;
        }
    
    default:
        UE_LOG(LogTemp, Warning, TEXT("BPR_Core: No extractor for CachedType %d"), static_cast<int32>(CachedType));
        break;
    }
    
    TextData.AssetType = CachedType;
}

FUnsupportedAssetInfo BPR_Core::GetUnsupportedAssetInfo() const
{
    return {
        FText::FromString("Blueprint Reader"),
        FText::FromString("Warning! This asset is not supported yet!"),
        FText::FromString(
            "The Blueprint Reader plugin cannot display this asset type in the current version.\n"
            "This is not an engine error, it's just support not yet implemented."),
        TEXT("https://github.com/RacoonCoder-Engineer/BlueprintReader"),
        FText::FromString("Check for updates")
    };
}

