// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Components/ActorComponent.h"

struct FBPR_ExtractedData
{
    FText Structure = FText::FromString(TEXT("No Data found"));
    FText Graph     = FText::FromString(TEXT("No Data found"));
};

class SBPR_TextWidget;

//==============================================================================
// BPR_Core - central data retrieval logic
// (minimum, only for ActorComponent and Actor in the first step)
//==============================================================================
class BPR_Core
{
public:

    BPR_Core() = default;
    ~BPR_Core() = default;

    //---------------------------------------------------------------------- 
    //  Types of Assets Supported
    //---------------------------------------------------------------------- 
    enum class EAssetType : uint8
    {
        Unknown,
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
    //  General logic
    //---------------------------------------------------------------------- 
    bool IsSupportedAsset(UObject* Object);
    void ExtractorSelector(UObject* Object);
  

    // --- Data access for UI ---
    const FBPR_ExtractedData& GetTextData() const { return TextData; }

private:

    //---------------------------------------------------------------------- 
    //  State Core
    //---------------------------------------------------------------------- 
    EAssetType CachedType = EAssetType::Unknown;

    TWeakPtr<SBPR_TextWidget> OutputWindow;
    TWeakPtr<SBPR_TextWidget> StructureTab;
    TWeakPtr<SBPR_TextWidget> GraphTab;

    FBPR_ExtractedData TextData;
};
