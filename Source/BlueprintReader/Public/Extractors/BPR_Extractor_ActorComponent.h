// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Core.h"


class UActorComponent;
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_FunctionEntry;
class UK2Node_FunctionResult;

/**
* Extractor for the ActorComponent Blueprint.
* Collects the component's variables, functions, and macros.
* The result is returned to Core via FText.
*/
class BLUEPRINTREADER_API BPR_Extractor_ActorComponent
{
public:
	
	BPR_Extractor_ActorComponent();
	~BPR_Extractor_ActorComponent();
	// --------------------------------
	// Main entry point
	// -------------------------------
	/** Processes the selected object (Blueprint), generates the structure and graph */
	void ProcessComponent(UObject* SelectedObject, FBPR_ExtractedData& OutData);

private:

	// -------------------------------
	// Logging
	// -------------------------------
	// Simple methods for outputting messages of different levels
	void LogMessage(const FString& Msg);
	void LogWarning(const FString& Msg);
	void LogError(const FString& Msg);

	// -------------------------------
	// Working with the Blueprint structure
	// -------------------------------
	/** General information about Blueprint (parent class, replication) */
	void AppendBlueprintInfo(UBlueprint* Blueprint, FString& OutText);

	/** Collection of user variables, including type, default value, flags, category, description */
	void AppendVariables(UBlueprint* Blueprint, FString& OutText);
	
	/** Finds the function entry node in the given graph */
	UK2Node_FunctionEntry* FindFunctionEntryNodeInGraph(UEdGraph* Graph);
	
	/** Finds the function result node in the given graph */
	UK2Node_FunctionResult* FindFunctionResultNodeInGraph(UEdGraph* Graph);

	/** Returns the default value of the property */
	FString GetPropertyDefaultValue(FProperty* Property, UObject* Object);

	/** Returns the detailed type of the property, including TMap/TSet/TArray and enum */
	FString GetPropertyTypeDetailed(FProperty* Property);

	/** Processes structural property fields and outputs their information */
	void AppendStructFields(FStructProperty* StructProp, FString& OutText, int32 Indent = 0);

	/** Information about replicated variables */
	void AppendReplicationInfo(UClass* Class, FString& OutText);

	// -------------------------------
	// Working with Graphs (Event/Function/Macro)
	// -------------------------------
	/** Main point for traversing all Blueprint graphs */
	void AppendGraphs(UBlueprint* Blueprint, FString& OutText);

	/** Traverse a sequence of graph nodes (exec + data flow) */
	void AppendGraphSequence(UEdGraph* Graph, FString& OutExecText, FString& OutDataText);

	/** Recursive processing of graph nodes for exec- and data-flow */
	void ProcessNodeSequence(UEdGraphNode* Node, int32 IndentLevel, TSet<UEdGraphNode*>& Visited, FString& OutExecText, FString& OutDataText);

	/** Collect function signature by input/output pins */
	FString GetFunctionSignature(UEdGraph* Graph);

	/** Collecting macro signatures from Tunnel nodes */
	FString GetMacroSignature(UEdGraph* Graph);

	/** Generates a human-readable node name given the node type and parameters */
	FString GetReadableNodeName(UEdGraphNode* Node);

	/** Pin details (default value or reference to another node) */
	FString GetPinDetails(UEdGraphPin* Pin);

	/** Human-readable name of the pin to display */
	FString GetPinDisplayName(UEdGraphPin* Pin);

	// -------------------------------
	// Helper Methods
	// -------------------------------
	/** Strips the name of the trailing GUID, if any */
	FString CleanName(const FString& RawName);

	/** Determines whether the property is a user variable (not a system variable) */
	bool IsUserVariable(FProperty* Property);
	
	/** Checks if the node is a compute (data-flow) node, without exec */
	bool IsComputationalNode(UEdGraphNode* Node);

	/** Checks if the node has an Exec input */
	bool HasExecInput(UEdGraphNode* Node);

};