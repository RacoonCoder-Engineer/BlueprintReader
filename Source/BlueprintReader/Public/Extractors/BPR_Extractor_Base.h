// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Core.h"

// Forward declarations for types used in function signatures
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_FunctionEntry;
class UK2Node_FunctionResult;
class FProperty;
class FStructProperty;
class UObject;

enum class EOutputFormat : uint8
{
    HumanReadable,     // Beautiful, multi-line output with indentation (for humans)
    Compact,           // Balanced format, saves tokens for LLMs
    Minimal            // Most compact format (best for LLMs)
};

/**
 * Base class for all asset extractors.
 * Provides common functionality for extracting Blueprint structure and graphs.
 */
class BLUEPRINTREADER_API BPR_Extractor_Base
{
public:
    BPR_Extractor_Base();
    virtual ~BPR_Extractor_Base() = default;
    
    /** Returns the short name of this extractor (used in logs and debugging) */
    FString GetExtractorName() const;

    /** Main entry point. Must be overridden by derived classes. */
    virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) = 0;

protected:
    // ===================================================================
    // Logging
    // ===================================================================
    void LogMessage(const FString& Msg) const;
    void LogWarning(const FString& Msg) const;
    void LogError(const FString& Msg) const;

    // ===================================================================
    // Blueprint Structure
    // ===================================================================
    virtual void AppendBlueprintInfo(UBlueprint* Blueprint, FString& OutText) const;
    virtual void AppendVariables(UBlueprint* Blueprint, FString& OutText) const;
    static void AppendReplicationInfo(const UClass* Class, FString& OutText);

    // ===================================================================
    // Property Helpers
    // ===================================================================
    virtual FString GetPropertyDefaultValue(FProperty* Property, UObject* Object) const;
    virtual FString GetPropertyTypeDetailed(FProperty* Property) const;
    virtual void AppendStructFields(FStructProperty* StructProp, FString& OutText, int32 Indent = 0) const;
    virtual bool IsUserVariable(FProperty* Property) const;
    static FString GetPropertyDescription(const FProperty* Property);

    // ===================================================================
    // Graph Processing
    // ===================================================================
    virtual void AppendGraphs(UBlueprint* Blueprint, FString& OutText) const;
    virtual void AppendGraphSequence(const UEdGraph* Graph, FString& OutExecText, FString& OutDataText) const;
    virtual void ProcessNodeSequence(UEdGraphNode* Node, int32 IndentLevel, TSet<UEdGraphNode*>& Visited, FString& OutExecText, FString& OutDataText) const;

    static UK2Node_FunctionEntry* FindFunctionEntryNodeInGraph(const UEdGraph* Graph);
    static UK2Node_FunctionResult* FindFunctionResultNodeInGraph(const UEdGraph* Graph);

    virtual FString GetFunctionSignature(const UEdGraph* Graph) const;
    virtual FString GetMacroSignature(const UEdGraph* Graph) const;

    // ===================================================================
    // Node & Pin Helpers
    // ===================================================================
    static FString GetReadableNodeName(const UEdGraphNode* Node);
    static FString GetPinDetails(const UEdGraphPin* Pin);
    static FString GetPinDisplayName(const UEdGraphPin* Pin);
    static FString CleanName(const FString& RawName);
    static bool IsComputationalNode(const UEdGraphNode* Node);
    static bool HasExecInput(const UEdGraphNode* Node);

    // ===================================================================
    // Formatting Helpers
    // ===================================================================
    void AppendSectionHeader(FString& OutText, const FString& Title) const;
    void AppendKeyValue(FString& OutText, const FString& Key, const FString& Value, int32 Indent = 0) const;
    static FString GetIndent(int32 Level);

    /** Controls output style (HumanReadable / Compact / Minimal) */
    EOutputFormat CurrentOutputFormat = EOutputFormat::Minimal;
    
    // ===================================================================
    // Table Helpers (Markdown)
    // ===================================================================

    /** Begins a markdown table with specified headers */
    static void BeginMarkdownTable(FString& OutText, const TArray<FString>& Headers, int32 Indent = 0);

    /** Appends a single row to the currently active markdown table */
    static void AppendTableRow(FString& OutText, const TArray<FString>& Columns, int32 Indent = 0);
        
    // ===================================================================
    // Utilities
    // ===================================================================
    /** Sets the extractor name for logging. Should be called from derived class constructors. */
    void SetExtractorName(const FString& InName);

private:
    /** Short name of the extractor used in logs (e.g. "Actor", "Widget") */
    FString ExtractorName;

    /** Currently processed Blueprint (optional, for future use) */
    UBlueprint* CurrentBlueprint = nullptr;
};