// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Extractors/BPR_Extractor_Material.h"
#include "Core/BPR_Core.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "MaterialExpressionIO.h"
#include "Materials/MaterialExpressionReroute.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h" // for StaticSwitch
#include "UObject/UnrealType.h" // FProperty, FStructProperty
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialexpressionTexturesampleparameter.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionStaticSwitch.h"

BPR_Extractor_Material::BPR_Extractor_Material()
{
	SetExtractorName(TEXT("Material"));
}
BPR_Extractor_Material::~BPR_Extractor_Material() {}

void BPR_Extractor_Material::Process(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
	ProcessMaterial(SelectedObject, OutData);
}

bool BPR_Extractor_Material::CanHandleAsset(UObject* Asset) const
{
	return Cast<UMaterial>(Asset) != nullptr || Cast<UMaterialInstance>(Asset) != nullptr;
}

void BPR_Extractor_Material::ProcessMaterial(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
	if (!SelectedObject)
	{
		LogError(TEXT("SelectedObject is null!"));
		OutData.Structure = FText::FromString("Error: SelectedObject is null.");
		OutData.Graph = FText::FromString("Error: SelectedObject is null.");
		return;
	}

	UMaterial* Material = Cast<UMaterial>(SelectedObject);
	UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(SelectedObject);

	if (!Material && !MaterialInstance)
	{
		LogWarning(TEXT("Selected object is not a Material or MaterialInstance."));
		OutData.Structure = FText::FromString("Warning: Selected object is not a Material or MaterialInstance.");
		OutData.Graph = FText::FromString("Warning: Selected object is not a Material or MaterialInstance.");
		return;
	}

	FString TmpStructure;
	FString TmpGraph;

	// For normal material
	if (Material)
	{
		AppendMaterialInfo(Material, TmpStructure);
		AppendMaterialParameters(Material, TmpStructure);
		AppendMaterialGraph(Material, TmpGraph);
	}

	// For material instance
	if (MaterialInstance)
	{
		AppendMaterialInstanceInfo(MaterialInstance, TmpStructure);
		AppendMaterialInstanceOverrides(MaterialInstance, TmpStructure);

		UMaterial* ParentMaterial = MaterialInstance->GetMaterial();
		if (ParentMaterial)
		{
			AppendMaterialGraph(ParentMaterial, TmpGraph);
		}
	}

	OutData.Structure = FText::FromString(TmpStructure);
	OutData.Graph = FText::FromString(TmpGraph);
	OutData.Design = FText::FromString(TEXT("N/A"));
	OutData.AssetType = EAssetType::Material;
}


//==============================================================================
// Material Information
//==============================================================================
void BPR_Extractor_Material::AppendMaterialInfo(UMaterial* Material, FString& OutText)
{
	if (!Material) return;

	OutText += FString::Printf(TEXT("Material: %s\n"), *CleanName(Material->GetName()));
	OutText += FString::Printf(TEXT("Domain: %s\n"), *UEnum::GetValueAsString(Material->MaterialDomain));
	OutText += FString::Printf(TEXT("BlendMode: %s\n"), *UEnum::GetValueAsString(Material->BlendMode));
	OutText += FString::Printf(TEXT("ShadingModel: %s\n"), *UEnum::GetValueAsString(Material->GetShadingModels().GetFirstShadingModel()));
	OutText += FString::Printf(TEXT("TwoSided: %s\n"), Material->TwoSided ? TEXT("True") : TEXT("False"));
	OutText += FString::Printf(TEXT("DitheredLODTransition: %s\n"), Material->DitheredLODTransition ? TEXT("True") : TEXT("False"));
	OutText += TEXT("\n");
}

//==============================================================================
// Material parameters
//==============================================================================
void BPR_Extractor_Material::AppendMaterialParameters(UMaterial* Material, FString& OutText)
{
	if (!Material) return;

	OutText += TEXT("## Material Parameters\n");

	// -----------------
	// Scalar
	// -----------------
	TArray<FMaterialParameterInfo> ScalarInfos;
	TArray<FGuid> ScalarIds;
	Material->GetAllScalarParameterInfo(ScalarInfos, ScalarIds);

	for (const FMaterialParameterInfo& Info : ScalarInfos)
	{
		float Value = 0.f;
		Material->GetScalarParameterValue(Info, Value);

		OutText += FString::Printf(TEXT("ScalarParam_%s = %.3f\n"),
			*Info.Name.ToString(), Value);
	}

	// -----------------
	// Vector
	// -----------------
	TArray<FMaterialParameterInfo> VectorInfos;
	TArray<FGuid> VectorIds;
	Material->GetAllVectorParameterInfo(VectorInfos, VectorIds);

	for (const FMaterialParameterInfo& Info : VectorInfos)
	{
		FLinearColor Value;
		Material->GetVectorParameterValue(Info, Value);

		OutText += FString::Printf(TEXT("VectorParam_%s = (%f,%f,%f,%f)\n"),
			*Info.Name.ToString(),
			Value.R, Value.G, Value.B, Value.A);
	}

	// -----------------
	// Texture
	// -----------------
	TArray<FMaterialParameterInfo> TextureInfos;
	TArray<FGuid> TextureIds;
	Material->GetAllTextureParameterInfo(TextureInfos, TextureIds);

	for (const FMaterialParameterInfo& Info : TextureInfos)
	{
		UTexture* Tex = nullptr;
		Material->GetTextureParameterValue(Info, Tex);

		FString TexName = Tex ? CleanName(Tex->GetName()) : TEXT("None");
		OutText += FString::Printf(TEXT("Texture_%s = %s\n"),
			*Info.Name.ToString(), *TexName);
	}

	// -----------------
	// StaticSwitch
	// -----------------
	TArray<FStaticSwitchParameter> SwitchParams;
	FStaticParameterSet DummySet;
	Material->GetStaticParameterValues(DummySet);
	SwitchParams = DummySet.StaticSwitchParameters;

	for (const FStaticSwitchParameter& Param : SwitchParams)
	{
		OutText += FString::Printf(TEXT("StaticSwitch_%s = %s\n"),
			*Param.ParameterInfo.Name.ToString(),
			Param.Value ? TEXT("True") : TEXT("False"));
	}

	OutText += TEXT("\n");
}



//==============================================================================
// Traversing the material graph
//==============================================================================
void BPR_Extractor_Material::AppendMaterialGraph(UMaterial* Material, FString& OutText)
{
	if (!Material) return;

	OutText += TEXT("## Material Graph\n");

	struct FMatOutput
	{
		FString Name;
		EMaterialProperty Property;
	};

	const TArray<FMatOutput> Outputs = {
		{TEXT("BaseColor"), MP_BaseColor},
		{TEXT("Metallic"), MP_Metallic},
		{TEXT("Specular"), MP_Specular},
		{TEXT("Roughness"), MP_Roughness},
		{TEXT("EmissiveColor"), MP_EmissiveColor},
		{TEXT("Opacity"), MP_Opacity},
		{TEXT("OpacityMask"), MP_OpacityMask},
		{TEXT("Normal"), MP_Normal},
		{TEXT("WorldPositionOffset"), MP_WorldPositionOffset}
	};

	// DAG structures
	TMap<UMaterialExpression*, int32> NodeIds;
	TMap<int32, FString> NodeTexts;
	int32 NextId = 1;

	// ----------------------
	// Phase 1: Outputs
	// ----------------------
	for (const FMatOutput& Out : Outputs)
	{
		TArray<UMaterialExpression*> DirectExpressions;
		Material->GetExpressionsInPropertyChain(
			Out.Property,
			DirectExpressions,
			nullptr
		);

		if (DirectExpressions.Num() == 0)
		{
			OutText += FString::Printf(TEXT("%s → None\n"), *Out.Name);
			continue;
		}

		// ❗ IMPORTANT: one call per Output
		AppendMaterialOutput(
			Out.Name,
			DirectExpressions,
			OutText,
			NodeIds,
			NodeTexts,
			NextId
		);
	}

	// ----------------------
	// Phase 2: Nodes
	// ----------------------
	OutText += TEXT("\n## Nodes\n");
	for (auto& Pair : NodeTexts)
	{
		OutText += Pair.Value;
	}
}


//==============================================================================
// MaterialInstance Information
//==============================================================================
void BPR_Extractor_Material::AppendMaterialInstanceInfo(UMaterialInstance* Instance, FString& OutText)
{
	if (!Instance) return;

	OutText += FString::Printf(TEXT("MaterialInstance: %s\n"), *CleanName(Instance->GetName()));
	UMaterial* Parent = Instance->GetMaterial();
	if (Parent)
	{
		OutText += FString::Printf(TEXT("Parent Material: %s\n"), *CleanName(Parent->GetName()));
	}
	OutText += TEXT("\n");
}


//==============================================================================
// Recursive traversal of Expression
//==============================================================================
void BPR_Extractor_Material::ProcessExpressionDAG(
	UMaterialExpression* Expression,
	TMap<UMaterialExpression*, int32>& NodeIds,
	TMap<int32, FString>& NodeTexts,
	int32& NextId)
{
	if (!Expression)
		return;

	// If the node has already been processed, we do nothing
	if (NodeIds.Contains(Expression))
		return;

	// We assign a unique ID
	int32 NodeId = NextId++;
	NodeIds.Add(Expression, NodeId);

	// Readable node name
	FString NodeDisplayName = GetReadableNodeName(Expression, NodeId);

	// Building the node text
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

			// Process child recursively
			ProcessExpressionDAG(ChildExpr, NodeIds, NodeTexts, NextId);

			// Add a link to child using a readable name
			int32 ChildId = NodeIds[ChildExpr];
			FString ChildDisplayName = GetReadableNodeName(ChildExpr, ChildId);
			NodeText += FString::Printf(TEXT("  Input: %s -> %s\n"), *InputName, *ChildDisplayName);
		}
		else
		{
			NodeText += FString::Printf(TEXT("  Input: %s = Unconnected\n"), *InputName);
		}
	}

	// Save the node text once
	NodeTexts.Add(NodeId, NodeText);
}

//==============================================================================
// Bypass Material Output
//==============================================================================
void BPR_Extractor_Material::AppendMaterialOutput(
	const FString& OutputName,
	const TArray<UMaterialExpression*>& DirectExpressions,
	FString& OutText,
	TMap<UMaterialExpression*, int32>& NodeIds,
	TMap<int32, FString>& NodeTexts,
	int32& NextId)
{
	if (DirectExpressions.Num() == 0)
	{
		OutText += FString::Printf(TEXT("%s → None\n"), *OutputName);
		return;
	}

	TArray<FString> DirectNames;
	TSet<UMaterialExpression*> AddedExpressions; // protection from duplicates

	for (UMaterialExpression* Expr : DirectExpressions)
	{
		if (!Expr)
			continue;

		// 1️⃣ We allow technical nodes (Reroute, Transparent, etc.)
		UMaterialExpression* ResolvedExpr = ResolveExpression(Expr);
		if (!ResolvedExpr)
			continue;

		// 2️⃣ Filtering: leaving only logical sources
		if (!IsLogicalSourceExpression(ResolvedExpr))
			continue;

		// 3️⃣ Replay protection
		if (AddedExpressions.Contains(ResolvedExpr))
			continue;

		AddedExpressions.Add(ResolvedExpr);

		// 4️⃣ Add to DAG (with all dependencies)
		if (!NodeIds.Contains(ResolvedExpr))
		{
			ProcessExpressionDAG(ResolvedExpr, NodeIds, NodeTexts, NextId);
		}

		// 5️⃣ Adding a readable name to the output
		DirectNames.Add(
			GetReadableNodeName(ResolvedExpr, NodeIds[ResolvedExpr])
		);
	}

	if (DirectNames.Num() == 0)
	{
		OutText += FString::Printf(TEXT("%s → None\n"), *OutputName);
		return;
	}

	FString Joined = FString::Join(DirectNames, TEXT(", "));
	OutText += FString::Printf(TEXT("%s → %s\n"), *OutputName, *Joined);
}


void BPR_Extractor_Material::AppendMaterialInstanceOverrides(UMaterialInstance* Instance, FString& OutText)
{
    if (!Instance) return;

    OutText += TEXT("## MaterialInstance Parameters\n");

    UMaterial* Parent = Instance->GetMaterial();
    if (!Parent)
    {
        OutText += TEXT("No parent material.\n\n");
        return;
    }

    auto AppendScalar = [&](const FMaterialParameterInfo& Info)
    {
        float ParentValue = 0.f;
        Parent->GetScalarParameterValue(Info, ParentValue);

        float InstValue = 0.f;
        Instance->GetScalarParameterValue(Info.Name, InstValue, false);

        bool bOverridden = !FMath::IsNearlyEqual(ParentValue, InstValue);
        OutText += FString::Printf(TEXT("Scalar: %s = %.3f (%s)\n"),
            *Info.Name.ToString(),
            InstValue,
            bOverridden ? TEXT("Overridden") : TEXT("Default"));
    };

    auto AppendVector = [&](const FMaterialParameterInfo& Info)
    {
        FLinearColor ParentValue;
        Parent->GetVectorParameterValue(Info, ParentValue);

        FLinearColor InstValue;
        Instance->GetVectorParameterValue(Info.Name, InstValue, false);

        bool bOverridden = !ParentValue.Equals(InstValue);
        OutText += FString::Printf(TEXT("Vector: %s = (%f,%f,%f,%f) (%s)\n"),
            *Info.Name.ToString(),
            InstValue.R, InstValue.G, InstValue.B, InstValue.A,
            bOverridden ? TEXT("Overridden") : TEXT("Default"));
    };

    auto AppendTexture = [&](const FMaterialParameterInfo& Info)
    {
        UTexture* ParentTex = nullptr;
        Parent->GetTextureParameterValue(Info, ParentTex);

        UTexture* InstTex = nullptr;
        Instance->GetTextureParameterValue(Info.Name, InstTex, false);

        bool bOverridden = ParentTex != InstTex;
        FString InstName = InstTex ? CleanName(InstTex->GetName()) : TEXT("None");
        FString ParentName = ParentTex ? CleanName(ParentTex->GetName()) : TEXT("None");

        OutText += FString::Printf(TEXT("Texture: %s = %s (%s, Parent=%s)\n"),
            *Info.Name.ToString(),
            *InstName,
            bOverridden ? TEXT("Overridden") : TEXT("Default"),
            *ParentName);
    };

    // -----------------
    // Scalar
    TArray<FMaterialParameterInfo> ScalarInfos;
    TArray<FGuid> ScalarIds;
    Parent->GetAllScalarParameterInfo(ScalarInfos, ScalarIds);
    for (const auto& Info : ScalarInfos) AppendScalar(Info);

    // -----------------
    // Vector
    TArray<FMaterialParameterInfo> VectorInfos;
    TArray<FGuid> VectorIds;
    Parent->GetAllVectorParameterInfo(VectorInfos, VectorIds);
    for (const auto& Info : VectorInfos) AppendVector(Info);

    // -----------------
    // Texture
    TArray<FMaterialParameterInfo> TextureInfos;
    TArray<FGuid> TextureIds;
    Parent->GetAllTextureParameterInfo(TextureInfos, TextureIds);
    for (const auto& Info : TextureInfos) AppendTexture(Info);

    // -----------------
    // StaticSwitch
    FStaticParameterSet StaticParams;
    Instance->GetStaticParameterValues(StaticParams);
    for (const FStaticSwitchParameter& Param : StaticParams.StaticSwitchParameters)
    {
        OutText += FString::Printf(TEXT("StaticSwitch: %s = %s (%s)\n"),
            *Param.ParameterInfo.Name.ToString(),
            Param.Value ? TEXT("True") : TEXT("False"),
            Param.bOverride ? TEXT("Overridden") : TEXT("Default"));
    }

    OutText += TEXT("\n");
}





// Helpers
FString BPR_Extractor_Material::CleanName(const FString& RawName)
{
	FString Result = RawName;

	int32 UnderscoreIndex;
	if (RawName.FindLastChar('_', UnderscoreIndex))
	{
		FString Tail = RawName.Mid(UnderscoreIndex + 1);

		// Material часто генерит *_GUID или *_NUMBER
		if (Tail.Len() >= 8 && Tail.IsNumeric())
		{
			Result = RawName.Left(UnderscoreIndex);
		}
	}

	return Result;
}

bool BPR_Extractor_Material::HasAnyInputs(UMaterialExpression* Expression)
{
	if (!Expression) return false;

	// For now we consider that if this is not Constant - 
	// it potentially has inputs
	return !Expression->IsA<UMaterialExpressionConstant>()
		&& !Expression->IsA<UMaterialExpressionScalarParameter>()
		&& !Expression->IsA<UMaterialExpressionVectorParameter>();
}


FString BPR_Extractor_Material::GetInputValueDescription(const FExpressionInput& Input)
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


FString BPR_Extractor_Material::GetReadableExpressionName(UMaterialExpression* Expression)
{
	if (!Expression)
		return TEXT("None");

	// 1. Basic type
	FString ClassName = Expression->GetClass()->GetName();
	ClassName.RemoveFromStart(TEXT("MaterialExpression"));

	FString Result = ClassName;

	// 2. Special cases (the most important)
	if (auto* Tex = Cast<UMaterialExpressionTextureSample>(Expression))
	{
		FString TexName = Tex->Texture
			? CleanName(Tex->Texture->GetName())
			: TEXT("None");

		Result = FString::Printf(TEXT("TextureSample(%s)"), *TexName);
	}
	else if (auto* Scalar = Cast<UMaterialExpressionScalarParameter>(Expression))
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

	// 3. Comment
	if (!Expression->Desc.IsEmpty())
	{
		Result += FString::Printf(TEXT(" // %s"), *Expression->Desc);
	}

	return Result;
}

FString BPR_Extractor_Material::MakeIndent(int32 Level)
{
	return FString::ChrN(Level * 2, ' '); // 2 spaces per level
}

bool BPR_Extractor_Material::IsTransparentExpression(UMaterialExpression* Expression)
{
	if (!Expression) return false;

	// Reroute — Always
	if (Expression->IsA<UMaterialExpressionReroute>()) return true;
	if (Expression->IsA<UMaterialExpressionNamedRerouteUsage>()) return true;

	// Declaration is a special case (logically transparent)
	if (Expression->IsA<UMaterialExpressionNamedRerouteDeclaration>()) return true;

	return false;
}

UMaterialExpression* BPR_Extractor_Material::ResolveExpression(UMaterialExpression* Expr)
{
	if (!Expr)
	{
		return nullptr;
	}

	TSet<UMaterialExpression*> Visited;
	UMaterialExpression* Current = Expr;

	while (Current && IsTransparentExpression(Current))
	{
		// защита от циклов
		if (Visited.Contains(Current))
		{
			LogWarning(FString::Printf(
				TEXT("ResolveExpression: loop detected at %s"),
				*Current->GetName()
			));
			return nullptr;
		}

		Visited.Add(Current);

		// Getting expression inputs (dereferenced)
		TArray<UMaterialExpression*> InputExpressions;
		Current->GetAllInputExpressions(InputExpressions);

		if (InputExpressions.Num() == 0)
		{
			// transparent node without inputs
			return nullptr;
		}

		// transparent node without inputs
		Current = InputExpressions[0];
	}

	return Current;
}

FString BPR_Extractor_Material::GetReadableNodeName(UMaterialExpression* Expr, int32 NodeId)
{
	if (!Expr)
		return FString::Printf(TEXT("None_%d"), NodeId);

	FString TypeName = Expr->GetClass()->GetName();
	TypeName.RemoveFromStart(TEXT("MaterialExpression"));

	// 1. Parameters
	if (auto* Scalar = Cast<UMaterialExpressionScalarParameter>(Expr))
	{
		if (!Scalar->ParameterName.IsNone())
			return FString::Printf(TEXT("ScalarParam_%s"), *Scalar->ParameterName.ToString());
	}
	else if (auto* Vector = Cast<UMaterialExpressionVectorParameter>(Expr))
	{
		if (!Vector->ParameterName.IsNone())
			return FString::Printf(TEXT("VectorParam_%s"), *Vector->ParameterName.ToString());
	}
	else if (auto* Switch = Cast<UMaterialExpressionStaticSwitchParameter>(Expr))
	{
		if (!Switch->ParameterName.IsNone())
			return FString::Printf(TEXT("StaticSwitch_%s"), *Switch->ParameterName.ToString());
	}
	// 2.FunctionCall
	else if (auto* FuncCall = Cast<UMaterialExpressionMaterialFunctionCall>(Expr))
	{
		if (FuncCall->MaterialFunction)
			return FString::Printf(TEXT("MF_%s"), *CleanName(FuncCall->MaterialFunction->GetName()));
	}
	// 3. Reroute
	else if (auto* Decl = Cast<UMaterialExpressionNamedRerouteDeclaration>(Expr))
	{
		if (!Decl->Name.IsNone())
			return FString::Printf(TEXT("Reroute_%s"), *Decl->Name.ToString());
	}

	// Fallback - type + number
	return FString::Printf(TEXT("%s_%d"), *TypeName, NodeId);
}

bool BPR_Extractor_Material::IsLogicalSourceExpression(
	UMaterialExpression* Expr
)
{
	if (!Expr)
		return false;

	if (Cast<UMaterialExpressionScalarParameter>(Expr)) return true;
	if (Cast<UMaterialExpressionVectorParameter>(Expr)) return true;
	if (Cast<UMaterialExpressionTextureSampleParameter>(Expr)) return true;
	if (Cast<UMaterialExpressionStaticSwitchParameter>(Expr)) return true;

	if (Cast<UMaterialExpressionConstant>(Expr)) return true;
	if (Cast<UMaterialExpressionConstant2Vector>(Expr)) return true;
	if (Cast<UMaterialExpressionConstant3Vector>(Expr)) return true;
	if (Cast<UMaterialExpressionConstant4Vector>(Expr)) return true;

	if (Cast<UMaterialExpressionStaticSwitch>(Expr)) return true;

	if (auto* Func = Cast<UMaterialExpressionMaterialFunctionCall>(Expr))
	{
		// later you can specify the function name
		return true;
	}

	return false;
}


