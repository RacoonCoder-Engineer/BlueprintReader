// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Extractors/BPR_Extractor_Base.h"

class UMaterial;
class UMaterialInstance;
class UMaterialExpression;

/**
* Extractor for Material and MaterialInstance.
* Converts the Material Graph into a text representation:
* - material structure
* - parameters
* - dependency graph of Material Outputs
*/
class BLUEPRINTREADER_API BPR_Extractor_Material : public BPR_Extractor_Base
{
public:

    BPR_Extractor_Material();
    ~BPR_Extractor_Material();

    // -------------------------------
    // Main entry point
    // -------------------------------
    /** Processes the selected object (Material or MaterialInstance) */
    void ProcessMaterial(UObject* SelectedObject, FBPR_ExtractedData& OutData);

    // Extractor contract (BPR_Extractor_Base)
    virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) override;
    virtual bool CanHandleAsset(UObject* Asset) const override;
    virtual int32 GetPriority() const override { return 40; }

private:

    // Logging (LogMessage/LogWarning/LogError) inherited from BPR_Extractor_Base.

    // -------------------------------
    // Material structure (declarative part)
    // -------------------------------
    /** General information about the material (Domain, BlendMode, ShadingModel, etc.) */
    void AppendMaterialInfo(UMaterial* Material, FString& OutText);

    /** Material Parameters (Scalar / Vector / Texture / Static Switch) */
    void AppendMaterialParameters(UMaterial* Material, FString& OutText);

    /** Information about the MaterialInstance and its parent */
    void AppendMaterialInstanceInfo(UMaterialInstance* Instance, FString& OutText);

    /** Overridden instance parameters */
    void AppendMaterialInstanceOverrides(UMaterialInstance* Instance, FString& OutText);

    // -------------------------------
    // Material graph (computational part)
    // -------------------------------
    /** Graph traversal entry point — all Material Outputs */
    void AppendMaterialGraph(UMaterial* Material, FString& OutText);

    /** Handles a specific Material Output (BaseColor, Normal, etc.) */
    void AppendMaterialOutput(
    const FString& OutputName,
    const TArray<UMaterialExpression*>& DirectExpressions,
    FString& OutText,
    TMap<UMaterialExpression*, int32>& NodeIds,
    TMap<int32, FString>& NodeTexts,
    int32& NextId);
    
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

    /** Checks if expression is transparent (pass-through) node like Reroute */
    bool IsTransparentExpression(UMaterialExpression* Expr);

    /** Checks if expression is a logical data source (TextureSample, Parameter, etc.) */
    bool IsLogicalSourceExpression(UMaterialExpression* Expr);
    
    // Returns the first non-transparent expression up the chain
    // May return nullptr if the chain is broken or loops
    UMaterialExpression* ResolveExpression(UMaterialExpression* Expr);

};
