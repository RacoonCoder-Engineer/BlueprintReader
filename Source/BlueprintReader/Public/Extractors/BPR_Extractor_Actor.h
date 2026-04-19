// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BPR_Extractor_Object.h"
#include "Extractors/BPR_Extractor_Base.h"

// Forward declarations
class UActorComponent;
class AActor;

/**
 * Extractor for Actor Blueprints (AActor derived classes).
 * Handles class info, variables, components, tags, replication and graphs.
 */
class BLUEPRINTREADER_API BPR_Extractor_Actor : public BPR_Extractor_Object
{
protected:
    /** Generates readable node name with Actor-specific enhancements */
    virtual FString GetReadableNodeName (const UEdGraphNode* Node) const;
    
    // ===================================================================
    // Actor-specific Structure
    // ===================================================================

    /** Appends general class and Blueprint information */
    void AppendClassInfo(const UBlueprint* Blueprint, FString& OutText) const;

    /** Appends information about Actor Components */
    void AppendActorComponents(const UBlueprint* Blueprint, FString& OutText) const;

    /** Appends Actor Tags */
    void AppendActorTags(const UBlueprint* Blueprint, FString& OutText) const;

    /** Formats component information for output */
    static FString FormatComponentInfo(UActorComponent* Component);

    // ===================================================================
    // Graph Processing (Actor-specific overrides if needed)
    // ===================================================================

    

    // Note: Most graph-related methods (AppendGraphs, ProcessNodeSequence, etc.)
    // will be inherited from BPR_Extractor_Base. We only override if Actor needs
    // special behavior.
    
public:
    BPR_Extractor_Actor();

    /** Main entry point used by BPR_Core */
    virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) override;

private:
    
};
