// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Core.h"

// Forward declarations
class UActorComponent;
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class K2Node_FunctionEntry;
class K2Node_FunctionResult;

class UK2Node_FunctionEntry;
class UK2Node_FunctionResult;

/**
* Extractor for Actor Blueprint.
* Collects information about classes, technologies, components, tags, and graphs.
* The result is returned to the core via FText.
*/
class BLUEPRINTREADER_API BPR_Extractor_Actor
{
public:
    BPR_Extractor_Actor();
    ~BPR_Extractor_Actor();

    // --------------------------------
    // Main entry point
    // -------------------------------
    /** Processes the selected object (Blueprint), generates the structure and graph */
    void ProcessActor(UObject* SelectedObject, FBPR_ExtractedData& OutData);

private:
    // ------------------------------- 
    // Logging 
    // -------------------------------
    void LogMessage(const FString& Msg);
    void LogWarning(const FString& Msg);
    void LogError(const FString& Msg);

    // -------------------------------
    // Working with the Blueprint structure
    // -------------------------------
    /** General information about the class (name, parent, placeable) */
    void AppendClassInfo(UBlueprint* Blueprint, FString& OutText);
    
    /** General information about Blueprint */
    void AppendBlueprintInfo(UBlueprint* Blueprint, FString& OutText);
    
    /** Information about replicated variables */
    void AppendReplicationInfo(UClass* Class, FString& OutText);

    // -------------------------------
    // Variables
    // -------------------------------
    /** Collection of user variables in table format */
    void AppendVariables(UBlueprint* Blueprint, FString& OutText);
    
    /** Returns the default value of the property */
    FString GetPropertyDefaultValue(FProperty* Property, UObject* Object);
    
    /** Returns the detailed type of the property */
    FString GetPropertyTypeDetailed(FProperty* Property);
    
    /** Processes structural property fields and outputs their information */
    void AppendStructFields(FStructProperty* StructProp, FString& OutText, int32 Indent = 0);
    
    /** Determines whether the property is a user variable */
    bool IsUserVariable(FProperty* Property);

    // -------------------------------
    // Components and Tags
    // -------------------------------
    /** Collecting information about actor components */
    void AppendActorComponents(UBlueprint* Blueprint, FString& OutText);
    
    /** Collect actor tags */
    void AppendActorTags(UBlueprint* Blueprint, FString& OutText);
    
    /** Formats information about the component */
    FString FormatComponentInfo(UActorComponent* Component);

    // -------------------------------
    // Working with Graphs
    // -------------------------------
    /** Main point for traversing all Blueprint graphs */
    void AppendGraphs(UBlueprint* Blueprint, FString& OutText);
    
    /** Traverse a sequence of graph nodes (exec + data flow) */
    void AppendGraphSequence(UEdGraph* Graph, FString& OutExecText, FString& OutDataText);
    
    /** Recursive processing of graph nodes */
    void ProcessNodeSequence(
        UEdGraphNode* Node, 
        int32 IndentLevel, 
        TSet<UEdGraphNode*>& Visited, 
        FString& OutExecText, 
        FString& OutDataText);

    /** Finds the function entry node in the given graph */
    UK2Node_FunctionEntry* FindFunctionEntryNodeInGraph(UEdGraph* Graph);
    
    /** Finds the function result node in the given graph */
    UK2Node_FunctionResult* FindFunctionResultNodeInGraph(UEdGraph* Graph);
    
    /** Collect function signature by input/output pins */
    FString GetFunctionSignature(UEdGraph* Graph);
    
    /** Collecting macro signatures from Tunnel nodes */
    FString GetMacroSignature(UEdGraph* Graph);

    // -------------------------------
    // Helper methods for nodes and pins
    // -------------------------------
    /** Generates a human-readable node name */
    FString GetReadableNodeName(UEdGraphNode* Node);
    
    /** Pin details (value or link) */
    FString GetPinDetails(UEdGraphPin* Pin);
    
    /** Human-readable name of the pin */
    FString GetPinDisplayName(UEdGraphPin* Pin);
    
    /** Strips the name of the GUID tail */
    FString CleanName(const FString& RawName);
    
    /** Checks if the node is a compute (data-flow) node */
    bool IsComputationalNode(UEdGraphNode* Node);
    
    /** Checks if the node has an Exec input */
    bool HasExecInput(UEdGraphNode* Node);
};