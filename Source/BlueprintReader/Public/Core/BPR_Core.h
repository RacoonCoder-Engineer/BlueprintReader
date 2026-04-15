// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Components/ActorComponent.h"

BLUEPRINTREADER_API DECLARE_LOG_CATEGORY_EXTERN(LogBlueprintReader, Log, All);

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
    FText Design   = FText::FromString(TEXT("No Data found"));
    EAssetType AssetType = EAssetType::Unknown;
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
    //  General logic
    //---------------------------------------------------------------------- 
    bool IsSupportedAsset(UObject* Object);
    void ExtractorSelector(UObject* Object);

    // --- Data access for UI ---
    const FBPR_ExtractedData& GetTextData() const { return TextData; }
    
    //==============================================================================
    // Logic for unsupported assets
    // (minimum, only for ActorComponent and Actor in the first step)
    //==============================================================================   
    
    

    FUnsupportedAssetInfo GetUnsupportedAssetInfo() const;

private:

    //---------------------------------------------------------------------- 
    //  State Core
    //---------------------------------------------------------------------- 
    EAssetType CachedType = EAssetType::Unknown;    
    FBPR_ExtractedData TextData;
};
