// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Extractors/BPR_Extractor_MaterialFunction.h"
#include "Core/BPR_Core.h"
#include "Core/BPR_Compat.h"   // M6: plumbing for future 5.7/5.8 shims (expression DAG walk below)
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "MaterialExpressionIO.h"
#include "Materials/MaterialExpressionReroute.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "UObject/UnrealType.h"
#include "Materials/MaterialexpressionTexturesampleparameter.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionStaticSwitch.h"
#include "Materials/MaterialExpressionFunctionOutput.h"


BPR_Extractor_MaterialFunction::BPR_Extractor_MaterialFunction()
{
	SetExtractorName(TEXT("MaterialFunction"));
}
BPR_Extractor_MaterialFunction::~BPR_Extractor_MaterialFunction() {}

void BPR_Extractor_MaterialFunction::Process(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
	ProcessMaterialFunction(SelectedObject, OutData);
}

bool BPR_Extractor_MaterialFunction::CanHandleAsset(UObject* Asset) const
{
	return Cast<UMaterialFunction>(Asset) != nullptr || Cast<UMaterialFunctionInstance>(Asset) != nullptr;
}

void BPR_Extractor_MaterialFunction::ProcessMaterialFunction(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
    // Empty for now, just checking logic
    if (!SelectedObject)
    {
        LogError(TEXT("SelectedObject is null!"));
        OutData.Structure = FText::FromString("Error: SelectedObject is null.");
        OutData.Graph = FText::FromString("Error: SelectedObject is null.");
        return;
    }

    UMaterialFunction* Function = Cast<UMaterialFunction>(SelectedObject);
    UMaterialFunctionInstance* Instance = Cast<UMaterialFunctionInstance>(SelectedObject);

    if (!Function && !Instance)
    {
        LogWarning(TEXT("Selected object is not a MaterialFunction or MaterialFunctionInstance."));
        OutData.Structure = FText::FromString("Warning: Selected object is not a MaterialFunction or MaterialFunctionInstance.");
        OutData.Graph = FText::FromString("Warning: Selected object is not a MaterialFunction or MaterialFunctionInstance.");
        return;
    }

    FString TmpStructure;
    FString TmpGraph;

    if (Function)
    {
        AppendFunctionInfo(Function, TmpStructure);
        AppendFunctionInputs(Function, TmpStructure);
        AppendFunctionOutputs(Function, TmpStructure);
        AppendFunctionGraph(Function, TmpGraph);
    }

    if (Instance)
    {
        AppendFunctionInstanceInfo(Instance, TmpStructure);
        AppendFunctionInstanceOverrides(Instance, TmpStructure);
        UMaterialFunction* ParentFunc = Cast<UMaterialFunction>(Instance->Base);

        if (ParentFunc)
        {
            AppendFunctionGraph(ParentFunc, TmpGraph);
        }
    }

    OutData.Structure = FText::FromString(TmpStructure);
    OutData.Graph = FText::FromString(TmpGraph);
    OutData.Design = FText::FromString(TEXT("N/A"));
    OutData.AssetType = EAssetType::MaterialFunction;
}

//==============================================================================
// Material Function structure
//==============================================================================
void BPR_Extractor_MaterialFunction::AppendFunctionInfo(UMaterialFunction* Function, FString& OutText)
{
    if (!Function) return;

    OutText += FString::Printf(TEXT("MaterialFunction: %s\n"), *CleanName(Function->GetName()));
    if (!Function->Description.IsEmpty())
        OutText += FString::Printf(TEXT("Description: %s\n"), *Function->Description);
    OutText += FString::Printf(TEXT("Exposed to Library: %s\n"), Function->bExposeToLibrary ? TEXT("True") : TEXT("False"));
    OutText += TEXT("\n");
}


void BPR_Extractor_MaterialFunction::AppendFunctionInputs(UMaterialFunction* Function, FString& OutText)
{
    if (!Function) return;

    OutText += TEXT("## Function Inputs\n");

    // Loop through all Expressions inside the function
    TArray<UMaterialExpression*> AllExpressions;
    for (UMaterialExpression* Expr : Function->GetExpressions())
    {
        if (Expr && Expr->IsA<UMaterialExpressionFunctionInput>())
        {
            AllExpressions.Add(Expr);
        }
    }


    for (UMaterialExpression* Expr : AllExpressions)
    {
        auto* Input = Cast<UMaterialExpressionFunctionInput>(Expr);
        if (!Input) continue;

        FString InputName = Input->InputName.IsNone() ? TEXT("Unnamed") : Input->InputName.ToString();
        FString InputType;

        switch (Input->InputType)
        {
        case FunctionInput_Scalar: InputType = TEXT("Scalar"); break;
        case FunctionInput_Vector2: InputType = TEXT("Vector2"); break;
        case FunctionInput_Vector3: InputType = TEXT("Vector3"); break;
        case FunctionInput_Vector4: InputType = TEXT("Vector4"); break;
        case FunctionInput_Texture2D: InputType = TEXT("Texture2D"); break;
        case FunctionInput_TextureCube: InputType = TEXT("TextureCube"); break;
        default: InputType = TEXT("Unknown"); break;
        }

        FString DefaultValueStr = TEXT("");
        if (Input->InputType == FunctionInput_Scalar)
        {
            DefaultValueStr = FString::Printf(TEXT("Default = %.3f"), Input->PreviewValue.X);
        }
        else if (Input->InputType == FunctionInput_Vector3 || Input->InputType == FunctionInput_Vector4)
        {
            DefaultValueStr = FString::Printf(TEXT("Default = (%.3f, %.3f, %.3f, %.3f)"),
                Input->PreviewValue.X, Input->PreviewValue.Y, Input->PreviewValue.Z, Input->PreviewValue.W);
        }

        OutText += FString::Printf(TEXT("%s (%s) %s\n"), *InputName, *InputType, *DefaultValueStr);
    }

    OutText += TEXT("\n");
}

void BPR_Extractor_MaterialFunction::AppendFunctionOutputs(UMaterialFunction* Function, FString& OutText)
{
    if (!Function) return;

    OutText += TEXT("## Function Outputs\n");

    // Get all ExpressionOutput nodes
    TArray<UMaterialExpression*> AllExpressions;
    for (UObject* Obj : Function->GetExpressions())
    {
        if (UMaterialExpression* Expr = Cast<UMaterialExpression>(Obj))
        {
            AllExpressions.Add(Expr);
        }
    }


    for (UMaterialExpression* Expr : AllExpressions)
    {
        if (!Expr) continue;

        // FunctionOutput only
        if (auto* Output = Cast<UMaterialExpressionFunctionOutput>(Expr))
        {
            FString OutputName = !Output->OutputName.IsNone() 
                                 ? Output->OutputName.ToString() 
                                 : TEXT("UnnamedOutput");

            // Get Expression connected to Output
            FString ConnectedExprName = TEXT("None");
            if (Output->A.Expression)
            {
                ConnectedExprName = GetReadableNodeName(Output->A.Expression, 0); // NodeId = 0, т.к. неважно здесь
            }

            OutText += FString::Printf(TEXT("Output: %s -> %s\n"), *OutputName, *ConnectedExprName);
        }
    }

    OutText += TEXT("\n");
}

void BPR_Extractor_MaterialFunction::AppendFunctionInstanceInfo(UMaterialFunctionInstance* Instance, FString& OutText)
{
    if (!Instance) return;

    OutText += FString::Printf(TEXT("MaterialFunctionInstance: %s\n"), *CleanName(Instance->GetName()));

    // Parent function
    UMaterialFunctionInterface* ParentFuncInterface = Instance->Parent;
    if (ParentFuncInterface)
    {
        // Пробуем каст к UMaterialFunction
        if (UMaterialFunction* ParentFunc = Cast<UMaterialFunction>(ParentFuncInterface))
        {
            OutText += FString::Printf(TEXT("Parent Function: %s\n"), *CleanName(ParentFunc->GetName()));
        }
    }

    OutText += TEXT("\n");
}

void BPR_Extractor_MaterialFunction::AppendFunctionInstanceOverrides(UMaterialFunctionInstance* Instance, FString& OutText)
{
    if (!Instance) return;

    OutText += TEXT("## FunctionInstance Parameters\n");

    UMaterialFunctionInterface* ParentInterface = Instance->Base.Get();
    if (!ParentInterface)
    {
        OutText += TEXT("No parent function.\n\n");
        return;
    }

    for (UMaterialExpression* Expr : ParentInterface->GetExpressions())
    {
        if (auto* Input = Cast<UMaterialExpressionFunctionInput>(Expr))
        {
            FString ParamName = Input->InputName.ToString();

            switch (Input->InputType)
            {
            case EFunctionInputType::FunctionInput_Scalar:
                OutText += FString::Printf(TEXT("Scalar: %s = %.3f\n"), *ParamName, Input->PreviewValue.X);
                break;

            case EFunctionInputType::FunctionInput_Vector2:
                OutText += FString::Printf(TEXT("Vector2: %s = (%f,%f)\n"),
                    *ParamName,
                    Input->PreviewValue.X,
                    Input->PreviewValue.Y);
                break;

            case EFunctionInputType::FunctionInput_Vector3:
                OutText += FString::Printf(TEXT("Vector3: %s = (%f,%f,%f)\n"),
                    *ParamName,
                    Input->PreviewValue.X,
                    Input->PreviewValue.Y,
                    Input->PreviewValue.Z);
                break;

            case EFunctionInputType::FunctionInput_Vector4:
                OutText += FString::Printf(TEXT("Vector4: %s = (%f,%f,%f,%f)\n"),
                    *ParamName,
                    Input->PreviewValue.X,
                    Input->PreviewValue.Y,
                    Input->PreviewValue.Z,
                    Input->PreviewValue.W);
                break;

            case EFunctionInputType::FunctionInput_Bool:
            case EFunctionInputType::FunctionInput_StaticBool:
                OutText += FString::Printf(TEXT("Bool: %s = %s\n"),
                    *ParamName,
                    Input->PreviewValue.X > 0.5f ? TEXT("True") : TEXT("False"));
                break;

            case EFunctionInputType::FunctionInput_Texture2D:
            case EFunctionInputType::FunctionInput_TextureCube:
            case EFunctionInputType::FunctionInput_Texture2DArray:
            case EFunctionInputType::FunctionInput_VolumeTexture:
            case EFunctionInputType::FunctionInput_TextureExternal:
                {
                    FString TexName = Input->GetReferencedTexture() ? Input->GetReferencedTexture()->GetName() : TEXT("None");
                    OutText += FString::Printf(TEXT("Texture: %s = %s\n"), *ParamName, *TexName);
                    break;
                }

            case EFunctionInputType::FunctionInput_MaterialAttributes:
                OutText += FString::Printf(TEXT("MaterialAttributes: %s\n"), *ParamName);
                break;

            case EFunctionInputType::FunctionInput_Substrate:
                OutText += FString::Printf(TEXT("Substrate: %s\n"), *ParamName);
                break;

            default:
                OutText += FString::Printf(TEXT("UnknownType: %s\n"), *ParamName);
                break;
            }
    
        }
    }

    OutText += TEXT("\n");
}


//==============================================================================
// Graph
//==============================================================================
void BPR_Extractor_MaterialFunction::AppendFunctionGraph(UMaterialFunction* Function, FString& OutText)
{
    if (!Function) return;

    OutText += TEXT("## Function Graph\n");

    // DAG structures
    TMap<UMaterialExpression*, int32> NodeIds;
    TMap<int32, FString> NodeTexts;
    int32 NextId = 1;

    // Get all output expressions of the function
    TArray<UMaterialExpression*> FunctionOutputs;
    for (UMaterialExpression* Expr : Function->GetExpressions())
    {
        if (Expr && Expr->IsA<UMaterialExpressionFunctionOutput>())
        {
            FunctionOutputs.Add(Expr);
        }
    }

    // Process each output
    for (UMaterialExpression* OutputExpr : FunctionOutputs)
    {
        if (!OutputExpr) continue;

        FString OutputName = Cast<UMaterialExpressionFunctionOutput>(OutputExpr)->GetName();

        TArray<UMaterialExpression*> DirectExpressions;

        // FunctionOutput usually only has one input
        if (OutputExpr->GetInput(0) && OutputExpr->GetInput(0)->Expression)
        {
            DirectExpressions.Add(OutputExpr->GetInput(0)->Expression);
        }

        AppendFunctionOutput(OutputName, DirectExpressions, OutText, NodeIds, NodeTexts, NextId);
    }

    // Add all nodes to the final text
    OutText += TEXT("\n## Nodes\n");
    for (auto& Pair : NodeTexts)
    {
        OutText += Pair.Value;
    }
}


void BPR_Extractor_MaterialFunction::AppendFunctionOutput(
    const FString& OutputName,
    const TArray<UMaterialExpression*>& DirectExpressions,
    FString& OutText,
    TMap<UMaterialExpression*, int32>& NodeIds,
    TMap<int32, FString>& NodeTexts,
    int32& NextId
)
{
    if (DirectExpressions.Num() == 0)
    {
        OutText += FString::Printf(TEXT("%s → None\n"), *OutputName);
        return;
    }

    TArray<FString> DirectNames;
    TSet<UMaterialExpression*> AddedExpressions;

    for (UMaterialExpression* Expr : DirectExpressions)
    {
        if (!Expr)
            continue;

        // 1️⃣ Allow transparent nodes
        UMaterialExpression* ResolvedExpr = ResolveExpression(Expr);
        if (!ResolvedExpr)
            continue;

        // 2️⃣ Filtering logical sources
        if (!IsLogicalSourceExpression(ResolvedExpr))
            continue;

        // 3️⃣ Replay protection
        if (AddedExpressions.Contains(ResolvedExpr))
            continue;

        AddedExpressions.Add(ResolvedExpr);

        // 4️⃣ Add to DAG
        if (!NodeIds.Contains(ResolvedExpr))
        {
            ProcessExpressionDAG(ResolvedExpr, NodeIds, NodeTexts, NextId);
        }

        // 5️⃣ Readable name
        DirectNames.Add(GetReadableNodeName(ResolvedExpr, NodeIds[ResolvedExpr]));
    }

    if (DirectNames.Num() == 0)
    {
        OutText += FString::Printf(TEXT("%s → None\n"), *OutputName);
        return;
    }

    FString Joined = FString::Join(DirectNames, TEXT(", "));
    OutText += FString::Printf(TEXT("%s → %s\n"), *OutputName, *Joined);
}


void BPR_Extractor_MaterialFunction::ProcessExpressionDAG(
    UMaterialExpression* Expression,
    TMap<UMaterialExpression*, int32>& NodeIds,
    TMap<int32, FString>& NodeTexts,
    int32& NextId)
{
    if (!Expression)
        return;

    // Already processed
    if (NodeIds.Contains(Expression))
        return;

    int32 NodeId = NextId++;
    NodeIds.Add(Expression, NodeId);

    FString NodeDisplayName = GetReadableNodeName(Expression, NodeId);
    FString NodeText = FString::Printf(TEXT("Node: %s\n"), *NodeDisplayName);

    int32 NumInputs = Expression->CountInputs();
    for (int32 i = 0; i < NumInputs; ++i)
    {
        FExpressionInput* Input = Expression->GetInput(i);
        if (!Input) continue;

        FString InputName = Expression->GetInputName(i).ToString();

        if (Input->Expression)
        {
            UMaterialExpression* ChildExpr = Input->Expression;

            // Recursive traversal
            ProcessExpressionDAG(ChildExpr, NodeIds, NodeTexts, NextId);

            int32 ChildId = NodeIds[ChildExpr];
            FString ChildDisplayName = GetReadableNodeName(ChildExpr, ChildId);
            NodeText += FString::Printf(TEXT("  Input: %s -> %s\n"), *InputName, *ChildDisplayName);
        }
        else
        {
            NodeText += FString::Printf(TEXT("  Input: %s = Unconnected\n"), *InputName);
        }
    }

    NodeTexts.Add(NodeId, NodeText);
}


//==============================================================================
// Helpers
//==============================================================================
FString BPR_Extractor_MaterialFunction::GetReadableExpressionName(UMaterialExpression* Expression)
{
    if (!Expression)
        return TEXT("None");

    // 1. Basic type
    FString ClassName = Expression->GetClass()->GetName();
    ClassName.RemoveFromStart(TEXT("MaterialExpression"));

    FString Result = ClassName;

    // 2. Special cases (important for MF)
    if (auto* Scalar = Cast<UMaterialExpressionScalarParameter>(Expression))
    {
        Result = FString::Printf(
            TEXT("ScalarParam(%s = %.3f)"),
            *Scalar->ParameterName.ToString(),
            Scalar->DefaultValue
        );
    }
    else if (auto* Vector = Cast<UMaterialExpressionVectorParameter>(Expression))
    {
        Result = FString::Printf(
            TEXT("VectorParam(%s)"),
            *Vector->ParameterName.ToString()
        );
    }
    else if (auto* Const = Cast<UMaterialExpressionConstant>(Expression))
    {
        Result = FString::Printf(
            TEXT("Constant(%.3f)"),
            Const->R
        );
    }
    else if (auto* FuncInput = Cast<UMaterialExpressionFunctionInput>(Expression))
    {
        Result = FString::Printf(
            TEXT("FunctionInput(%s)"),
            *FuncInput->InputName.ToString()
        );
    }
    else if (auto* FuncOutput = Cast<UMaterialExpressionFunctionOutput>(Expression))
    {
        Result = FString::Printf(
            TEXT("FunctionOutput(%s)"),
            *FuncOutput->OutputName.ToString()
        );
    }

    // 3. Comment
    if (!Expression->Desc.IsEmpty())
    {
        Result += FString::Printf(TEXT(" // %s"), *Expression->Desc);
    }

    return Result;
}



FString BPR_Extractor_MaterialFunction::GetExpressionInputs(
    UMaterialExpression* Expression,
    int32 IndentLevel
)
{
    if (!Expression) return TEXT("");

    FString Indent = MakeIndent(IndentLevel);
    FString Result;

    int32 NumInputs = Expression->CountInputs();
    for (int32 i = 0; i < NumInputs; ++i)
    {
        FExpressionInput* Input = Expression->GetInput(i);
        if (!Input) continue;

        FString InputName = Expression->GetInputName(i).ToString();
        FString InputDesc = GetInputValueDescription(*Input);

        Result += FString::Printf(TEXT("%sInput: %s -> %s\n"), 
            *Indent, *InputName, *InputDesc);
    }

    return Result;
}



FString BPR_Extractor_MaterialFunction::GetInputValueDescription(
    const FExpressionInput& Input
)
{
    if (Input.Expression)
    {
        return FString::Printf(
            TEXT("<linked: %s>"),
            *GetReadableExpressionName(Input.Expression)
        );
    }

    return TEXT("Unconnected");
}


bool BPR_Extractor_MaterialFunction::HasAnyInputs(UMaterialExpression* Expression)
{
    if (!Expression)
        return false;

    // Function Input - entry point, not a computational node
    if (Expression->IsA<UMaterialExpressionFunctionInput>())
        return false;

    // Function Output - always has an input
    if (Expression->IsA<UMaterialExpressionFunctionOutput>())
        return true;

    // Constants and parameters
    if (Expression->IsA<UMaterialExpressionConstant>() ||
        Expression->IsA<UMaterialExpressionConstant2Vector>() ||
        Expression->IsA<UMaterialExpressionConstant3Vector>() ||
        Expression->IsA<UMaterialExpressionConstant4Vector>())
        return false;

    // By default, we assume that inputs are possible
    return true;
}


FString BPR_Extractor_MaterialFunction::GetReadableNodeName(
    UMaterialExpression* Expr,
    int32 NodeId
)
{
    if (!Expr)
        return FString::Printf(TEXT("None_%d"), NodeId);

    FString TypeName = Expr->GetClass()->GetName();
    TypeName.RemoveFromStart(TEXT("MaterialExpression"));

    // 1️⃣ Function Input
    if (auto* Input = Cast<UMaterialExpressionFunctionInput>(Expr))
    {
        if (!Input->InputName.IsNone())
        {
            return FString::Printf(
                TEXT("Input_%s"),
                *Input->InputName.ToString()
            );
        }
    }
    // 2️⃣ Function Output
    else if (auto* Output = Cast<UMaterialExpressionFunctionOutput>(Expr))
    {
        if (!Output->OutputName.IsNone())
        {
            return FString::Printf(
                TEXT("Output_%s"),
                *Output->OutputName.ToString()
            );
        }
    }
    // 3️⃣ Nested Material Function
    else if (auto* FuncCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expr))
    {
        if (FuncCall->MaterialFunction)
        {
            return FString::Printf(
                TEXT("MF_%s"),
                *CleanName(FuncCall->MaterialFunction->GetName())
            );
        }
    }
    // 4️⃣ Named Reroute
    else if (auto* Decl = Cast<UMaterialExpressionNamedRerouteDeclaration>(Expr))
    {
        if (!Decl->Name.IsNone())
        {
            return FString::Printf(
                TEXT("Reroute_%s"),
                *Decl->Name.ToString()
            );
        }
    }

    // 5️⃣ Fallback
    return FString::Printf(TEXT("%s_%d"), *TypeName, NodeId);
}


FString BPR_Extractor_MaterialFunction::CleanName(const FString& RawName)
{
    FString Result = RawName;

    int32 UnderscoreIndex;
    if (RawName.FindLastChar('_', UnderscoreIndex))
    {
        FString Tail = RawName.Mid(UnderscoreIndex + 1);

        if (Tail.Len() >= 8 && Tail.IsNumeric())
        {
            Result = RawName.Left(UnderscoreIndex);
        }
    }

    return Result;
}



FString BPR_Extractor_MaterialFunction::MakeIndent(int32 Level)
{
    return FString::ChrN(Level * 2, ' ');
}



bool BPR_Extractor_MaterialFunction::IsTransparentExpression(
    UMaterialExpression* Expression
)
{
    if (!Expression) return false;

    if (Expression->IsA<UMaterialExpressionReroute>()) return true;
    if (Expression->IsA<UMaterialExpressionNamedRerouteUsage>()) return true;
    if (Expression->IsA<UMaterialExpressionNamedRerouteDeclaration>()) return true;

    return false;
}



bool BPR_Extractor_MaterialFunction::IsLogicalSourceExpression(
    UMaterialExpression* Expr
)
{
    if (!Expr)
        return false;

    // -------------------------
    // Function-specific
    // -------------------------
    if (Cast<UMaterialExpressionFunctionInput>(Expr)) return true;

    // -------------------------
    // Parameters
    // -------------------------
    if (Cast<UMaterialExpressionScalarParameter>(Expr)) return true;
    if (Cast<UMaterialExpressionVectorParameter>(Expr)) return true;
    if (Cast<UMaterialExpressionTextureSampleParameter>(Expr)) return true;
    if (Cast<UMaterialExpressionStaticSwitchParameter>(Expr)) return true;

    // -------------------------
    // Constants
    // -------------------------
    if (Cast<UMaterialExpressionConstant>(Expr)) return true;
    if (Cast<UMaterialExpressionConstant2Vector>(Expr)) return true;
    if (Cast<UMaterialExpressionConstant3Vector>(Expr)) return true;
    if (Cast<UMaterialExpressionConstant4Vector>(Expr)) return true;

    // -------------------------
    // Logic
    // -------------------------
    if (Cast<UMaterialExpressionStaticSwitch>(Expr)) return true;

    // -------------------------
    // Nested functions
    // -------------------------
    if (Cast<UMaterialExpressionMaterialFunctionCall>(Expr)) return true;

    return false;
}



UMaterialExpression* BPR_Extractor_MaterialFunction::ResolveExpression(
    UMaterialExpression* Expr
)
{
    if (!Expr)
    {
        return nullptr;
    }

    TSet<UMaterialExpression*> Visited;
    UMaterialExpression* Current = Expr;

    while (Current && IsTransparentExpression(Current))
    {
        if (Visited.Contains(Current))
        {
            LogWarning(FString::Printf(
                TEXT("ResolveExpression (MF): loop detected at %s"),
                *Current->GetName()
            ));
            return nullptr;
        }

        Visited.Add(Current);

        TArray<UMaterialExpression*> InputExpressions;
        Current->GetAllInputExpressions(InputExpressions);

        if (InputExpressions.Num() == 0)
        {
            return nullptr;
        }

        Current = InputExpressions[0];
    }

    return Current;
}

//==============================================================================
// Logging
//==============================================================================
// Logging (LogMessage/LogWarning/LogError) inherited from BPR_Extractor_Base (LogBlueprintReader).
