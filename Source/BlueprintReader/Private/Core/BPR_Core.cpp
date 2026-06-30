// Copyright (c) 2026 Racoon Coder. All rights reserved.

//ToDo:
// Что нужно сделать дальше:
//
// Создать BPR_Extractor_Blueprint.h/.cpp (для обычных Blueprint Class) — если ещё нет.
// В каждом существующем экстракторе реализовать CanHandleAsset() и GetPriority().
// Обновить BPR_Extractor_Base (добавить GetPriority() и CalculateInheritanceDepth()).
// Вызвать Core->RegisterAllExtractors() в StartupModule() плагина.
//
// Хочешь, я сразу напишу обновлённый BPR_Extractor_Base.h/.cpp с поддержкой приоритетов?
// Или сначала посмотрим на этот Core и поправим, если нужно?
// Говори, что делаем дальше.

#include "Core/BPR_Core.h"
#include "Extractors/BPR_Extractor_Base.h"

// === Concrete extractors (add new ones here) ===
#include "Extractors/BPR_Extractor_Actor.h"
#include "Extractors/BPR_Extractor_ActorComponent.h"
#include "Extractors/BPR_Extractor_Widget.h"
#include "Extractors/BPR_Extractor_Interface.h"
#include "Extractors/BPR_Extractor_Material.h"
#include "Extractors/BPR_Extractor_MaterialFunction.h"
#include "Extractors/BPR_Extractor_Enum.h"
#include "Extractors/BPR_Extractor_Structure.h"

// M2: Widget is now proper BPR_Extractor_Object subclass (no adapter needed)
//#include "Extractors/BPR_Extractor_Blueprint.h"   // ← для обычных Blueprint Class

BPR_Core::BPR_Core() = default;
BPR_Core::~BPR_Core() = default;

void BPR_Core::RegisterAllExtractors()
{
    Extractors.Empty();

    // Register from most specific to most general (priority will also be considered).
    // NOTE (M1): Only register extractors that properly inherit BPR_Extractor_Base.
    // Legacy (Widget, Material, ActorComponent, MaterialFunction) will be added via adapters in Phase 3.
    Extractors.Add(MakeShared<BPR_Extractor_Actor>());
    Extractors.Add(MakeShared<BPR_Extractor_ActorComponent>());    // M3: now inherits Object
    Extractors.Add(MakeShared<BPR_Extractor_Widget>());   // M2: now properly inherits from Object
    Extractors.Add(MakeShared<BPR_Extractor_Interface>());
    Extractors.Add(MakeShared<BPR_Extractor_Material>());          // M3: Base subclass
    Extractors.Add(MakeShared<BPR_Extractor_MaterialFunction>());  // M3: Base subclass
    Extractors.Add(MakeShared<BPR_Extractor_Enum>());
    Extractors.Add(MakeShared<BPR_Extractor_Structure>());
    //Extractors.Add(MakeUnique<BPR_Extractor_Blueprint>());   // обычные Blueprint Class

    // Sort by priority (highest first)
    Extractors.Sort([](const TSharedPtr<BPR_Extractor_Base>& A, const TSharedPtr<BPR_Extractor_Base>& B)
    {
        return A->GetPriority() > B->GetPriority();
    });

    UE_LOG(LogBlueprintReader, Log, TEXT("BPR_Core: Registered %d extractors"), Extractors.Num());
    for (const auto& Ex : Extractors)
    {
        UE_LOG(LogBlueprintReader, Verbose, TEXT("  - %s (priority %d)"), 
               *Ex->GetExtractorName(), Ex->GetPriority());
    }
}

bool BPR_Core::IsSupportedAsset(UObject* Asset) const
{
    bool result = FindSuitableExtractor(Asset) != nullptr;
    UE_LOG(LogBlueprintReader, Verbose, TEXT("IsSupportedAsset(%s) -> %s"), 
           Asset ? *Asset->GetName() : TEXT("NULL"), 
           result ? TEXT("true") : TEXT("false"));
    return result;
}

void BPR_Core::ExtractAsset(UObject* Asset, FBPR_ExtractedData& OutData)
{
    if (!Asset)
    {
        OutData.Structure = FText::FromString(TEXT("Error: Null asset"));
        OutData.AssetType = EAssetType::Unknown;
        return;
    }

    if (BPR_Extractor_Base* Extractor = FindSuitableExtractor(Asset))
    {
        Extractor->Extract(Asset, OutData);
        UE_LOG(LogBlueprintReader, Log, TEXT("BPR_Core: Used extractor: %s"), *Extractor->GetExtractorName());
    }
    else
    {
        // Unsupported asset
        OutData.AssetType = EAssetType::Unknown;
        OutData.Structure = FText::FromString(TEXT("No Data found"));
        OutData.Graph     = FText::FromString(TEXT("N/A"));
        OutData.Design    = FText::FromString(TEXT("N/A"));

        UE_LOG(LogBlueprintReader, Warning, TEXT("BPR_Core: No extractor found for asset: %s"), *Asset->GetName());
    }

    // M4.2: centrally record the asset name so the UI can offer a sensible export filename.
    OutData.AssetName = Asset->GetName();
}

BPR_Extractor_Base* BPR_Core::FindSuitableExtractor(UObject* Asset) const
{
    for (const TSharedPtr<BPR_Extractor_Base>& Extractor : Extractors)
    {
        if (Extractor->CanHandleAsset(Asset))
        {
            UE_LOG(LogBlueprintReader, Log, TEXT("FindSuitableExtractor chose %s for %s"), 
                   *Extractor->GetExtractorName(), Asset ? *Asset->GetName() : TEXT("NULL"));
            return Extractor.Get();
        }
    }
    return nullptr;
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