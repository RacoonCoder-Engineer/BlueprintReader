// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "Extractors/BPR_Extractor_Base.h"



// Forward declarations
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_FunctionEntry;
class UK2Node_FunctionResult;


/**
 * Base class for all Blueprint-based assets (Actor, Widget, Component, Interface, etc.).
 * 
 * Inherits from BPR_Extractor_Base and adds common logic for:
 *   - Blueprint Info
 *   - User Variables + Struct expansion
 *   - Graphs (Construction Script, Event Graphs, Functions, Macros)
 *   - Node traversal and readable representation
 * 
 * Specific extractors (BPR_Extractor_Actor, BPR_Extractor_Widget, etc.)
 * should inherit from this class and override/extend only what they need.
 */
class BLUEPRINTREADER_API BPR_Extractor_Object : public BPR_Extractor_Base
{
public:
    BPR_Extractor_Object();
    virtual ~BPR_Extractor_Object() override = default;

    /** Main entry point */
    virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) override;

    /** Cleaner API (recommended for future) */
    virtual FString Extract(const UObject* Asset) override;

protected:
    

    
    // ===================================================================
    // Core Extraction Methods (can be overridden by derived classes)
    // ===================================================================
    virtual void ExtractStructure(FString& OutText, UBlueprint* Blueprint);
    virtual void ExtractGraphs(FString& OutText, UBlueprint* Blueprint);

    // ===================================================================
    // Blueprint Structure
    // ===================================================================
    virtual void AppendBlueprintInfo(UBlueprint* Blueprint, FString& OutText) const;
    virtual void AppendVariables(UBlueprint* Blueprint, FString& OutText) const;
    virtual void AppendReplicationInfo(const UClass* Class, FString& OutText) const;

    // ===================================================================
    // Property Helpers (extended)
    // ===================================================================
    virtual FString GetPropertyDefaultValue(FProperty* Property, UObject* Object) const;
    virtual bool IsUserVariable(FProperty* Property) const;
    virtual void AppendStructFields(FStructProperty* StructProp, FString& OutText, int32 Indent = 0) const;

    // ===================================================================
    // Graph Processing
    // ===================================================================
    virtual void AppendGraphs(UBlueprint* Blueprint, FString& OutText) const;
    virtual void AppendGraphSequence(const UEdGraph* Graph, FString& OutExecText, FString& OutDataText) const;
    virtual void ProcessNodeSequence(UEdGraphNode* Node, int32 IndentLevel, TSet<UEdGraphNode*>& Visited,
                                     FString& OutExecText, FString& OutDataText) const;

    //virtual UEdGraph* FindConstructionScriptGraph(UBlueprint* Blueprint) const; //Надо перенести в Актора

    virtual FString GetFunctionSignature(const UEdGraph* Graph) const;
    virtual FString GetMacroSignature(const UEdGraph* Graph) const;

    // ===================================================================
    // Node & Pin Helpers (can be overridden for better readability)
    // ===================================================================
    virtual FString GetReadableNodeName(const UEdGraphNode* Node) const;
    virtual FString GetPinDetails(const UEdGraphPin* Pin) const;

    static UK2Node_FunctionEntry* FindFunctionEntryNodeInGraph(const UEdGraph* Graph);
    static UK2Node_FunctionResult* FindFunctionResultNodeInGraph(const UEdGraph* Graph);
    static bool IsComputationalNode(const UEdGraphNode* Node);
    static bool HasExecInput(const UEdGraphNode* Node);

private:
    /** Currently processed Blueprint (cached for internal use during extraction) */
    UBlueprint* CurrentBlueprint = nullptr;
};