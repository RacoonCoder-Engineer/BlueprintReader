// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Extractors/BPR_Extractor_Base.h"

class UMaterialFunction;
class UMaterialFunctionInstance;
class UMaterialExpression;
class UMaterialExpressionFunctionInput;
class UMaterialExpressionFunctionOutput;

/**
* Extractor for Material Function and Material Function Instance.
* Converts a Function Graph into a text representation:
* - function signature (Inputs / Outputs)
* - parameters and default values
* - computation graph (Expression DAG)
*/
class BLUEPRINTREADER_API BPR_Extractor_MaterialFunction : public BPR_Extractor_Base
{
public:

    BPR_Extractor_MaterialFunction();
    ~BPR_Extractor_MaterialFunction();

    // -------------------------------
    // Main entry point
    // -------------------------------
    /** Processes the selected object (MaterialFunction or MaterialFunctionInstance) */
    void ProcessMaterialFunction(UObject* SelectedObject, FBPR_ExtractedData& OutData);

    // Extractor contract (BPR_Extractor_Base)
    virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) override;
    virtual bool CanHandleAsset(UObject* Asset) const override;
    virtual int32 GetPriority() const override { return 30; }

private:

    // Logging (LogMessage/LogWarning/LogError) inherited from BPR_Extractor_Base.

    // -------------------------------
    // Material Function Structure (Declarative Part)
    // --------------------------------
    /** General Information about the Material Function */
    void AppendFunctionInfo(UMaterialFunction* Function, FString& OutText);

    /** Function Inputs */
    void AppendFunctionInputs(UMaterialFunction* Function, FString& OutText);

    /** Function Outputs */
    void AppendFunctionOutputs(UMaterialFunction* Function, FString& OutText);

    /** Information about the Function Instance and the parent function */
    void AppendFunctionInstanceInfo(UMaterialFunctionInstance* Instance, FString& OutText);

    /** Function Instance Overrides */
    void AppendFunctionInstanceOverrides(UMaterialFunctionInstance* Instance, FString& OutText);

    // -------------------------------
    // Material Function Graph (computational part)
    // -------------------------------
    /** Graph traversal entry point — all Function Outputs */
    void AppendFunctionGraph(UMaterialFunction* Function, FString& OutText);

    /** Handles a specific Function Output */
    void AppendFunctionOutput(
        const FString& OutputName,
        const TArray<UMaterialExpression*>& DirectExpressions,
        FString& OutText,
        TMap<UMaterialExpression*, int32>& NodeIds,
        TMap<int32, FString>& NodeTexts,
        int32& NextId
    );

    /** Recursive traversal of the Expression graph (data-flow) */
    void ProcessExpressionDAG(
        UMaterialExpression* Expression,
        TMap<UMaterialExpression*, int32>& NodeIds,
        TMap<int32, FString>& NodeTexts,
        int32& NextId
    );

    // -------------------------------
    // Expression / Input helpers
    // -------------------------------
    /** Expression human-readable name (type + keywords) */
    FString GetReadableExpressionName(UMaterialExpression* Expression);

    /** Expression Input Pin Details */
    FString GetExpressionInputs(UMaterialExpression* Expression, int32 IndentLevel);

    /** Input value: link or default */
    FString GetInputValueDescription(const struct FExpressionInput& Input);

    /** Check if Expression has any incoming relationships */
    bool HasAnyInputs(UMaterialExpression* Expression);

    /** Returns readable node name with ID for graph output */
    FString GetReadableNodeName(UMaterialExpression* Expr, int32 NodeId);

    // -------------------------------
    // Helper Methods
    // -------------------------------
    /** Clearing names of technical suffixes */
    FString CleanName(const FString& RawName);

    /** Indents for visual display of the graph */
    FString MakeIndent(int32 Level);

    /** Check whether the Expression is technically transparent */
    bool IsTransparentExpression(UMaterialExpression* Expr);

    /** Check if Expression is a logical data source */
    bool IsLogicalSourceExpression(UMaterialExpression* Expr);

    /** Returns the first non-transparent expression up the chain */
    UMaterialExpression* ResolveExpression(UMaterialExpression* Expr);
};
