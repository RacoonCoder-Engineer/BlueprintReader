// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Extractors/BPR_Extractor_Base.h"

// Forward declarations
class UActorComponent;
class AActor;

/**
 * Extractor for Actor Blueprints (AActor derived classes).
 * Handles class info, variables, components, tags, replication and graphs.
 */
class BLUEPRINTREADER_API BPR_Extractor_Actor : public BPR_Extractor_Base
{
public:
    BPR_Extractor_Actor();

    /** Main entry point used by BPR_Core */
    virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) override;

private:
    // ===================================================================
    // Actor-specific Structure
    // ===================================================================

    /** Appends general class and Blueprint information */
    void AppendClassInfo(UBlueprint* Blueprint, FString& OutText) const;

    /** Appends information about Actor Components */
    void AppendActorComponents(UBlueprint* Blueprint, FString& OutText) const;

    /** Appends Actor Tags */
    void AppendActorTags(UBlueprint* Blueprint, FString& OutText) const;

    /** Formats component information for output */
    FString FormatComponentInfo(UActorComponent* Component) const;

    // ===================================================================
    // Graph Processing (Actor-specific overrides if needed)
    // ===================================================================

    /** Generates readable node name with Actor-specific enhancements */
    FString GetReadableNodeName(UEdGraphNode* Node) const;

    // Note: Most graph-related methods (AppendGraphs, ProcessNodeSequence, etc.)
    // will be inherited from BPR_Extractor_Base. We only override if Actor needs
    // special behavior.
};
