// Copyright (c) 2026 Racoon Coder. All rights reserved.
#include "Extractors/BPR_Extractor_Base.h"

#include "Engine/Blueprint.h"
#include "UObject/UnrealType.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Switch.h"
#include "K2Node_Tunnel.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Knot.h"
#include "K2Node_MathExpression.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_CastByteToEnum.h"
#include "K2Node_Select.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogBlueprintReader);

// ===================================================================
// Construction / Destruction
// ===================================================================

BPR_Extractor_Base::BPR_Extractor_Base()
    : ExtractorName(TEXT("Base"))
{
    // Derived classes should set ExtractorName in their constructor
}

void BPR_Extractor_Base::SetExtractorName(const FString& InName)
{
	ExtractorName = InName;
}

FString BPR_Extractor_Base::GetExtractorName() const
{
	return ExtractorName;
}

// ===================================================================
// Formatting Helpers
// ===================================================================

FString BPR_Extractor_Base::GetIndent(int32 Level)
{
	if (Level <= 0)
	{
		return FString();
	}

	// Limit maximum indentation to prevent excessive memory usage or malformed output
	Level = FMath::Clamp(Level, 0, 20);

	static TArray<FString> CachedIndents;

	if (CachedIndents.Num() <= Level)
	{
		CachedIndents.SetNum(Level + 1);

		for (int32 i = CachedIndents.Num() - 1; i >= 0; --i)
		{
			if (CachedIndents[i].IsEmpty())
			{
				CachedIndents[i] = FString::ChrN(i * 4, TEXT(' '));
			}
		}
	}

	return CachedIndents[Level];
}

void BPR_Extractor_Base::AppendSectionHeader(FString& OutText, const FString& Title) const
{
	OutText += TEXT("\n");

	switch (CurrentOutputFormat)
	{
	case EOutputFormat::HumanReadable:
		// Double-line header for human-readable output
		OutText += FString::ChrN(80, TEXT('═')) + TEXT("\n");
		{
			constexpr int32 LineWidth = 80;
			const int32 TitleLen = Title.Len();
			const int32 Padding = FMath::Max(0, (LineWidth - TitleLen) / 2);

			FString Centered = FString::ChrN(Padding, TEXT(' ')) + Title.ToUpper();
			if ((LineWidth - TitleLen) % 2 != 0)
			{
				Centered += TEXT(" ");
			}

			OutText += Centered + TEXT("\n");
		}
		OutText += FString::ChrN(80, TEXT('═')) + TEXT("\n\n");
		break;

	case EOutputFormat::Compact:
		// Single-line header for compact output
		OutText += FString::ChrN(60, TEXT('─')) + TEXT("\n");
		OutText += TEXT("  ") + Title.ToUpper() + TEXT("\n");
		OutText += FString::ChrN(60, TEXT('─')) + TEXT("\n\n");
		break;

	case EOutputFormat::Minimal:
	default:
		// Markdown-style header optimized for LLMs
		OutText += FString::Printf(TEXT("## %s\n\n"), *Title.ToUpper());
		break;
	}
}

void BPR_Extractor_Base::AppendKeyValue(FString& OutText, const FString& Key, const FString& Value, int32 Indent /*= 0*/) const
{
	if (Key.IsEmpty())
	{
		return;
	}

	const FString DisplayValue = Value.IsEmpty() ? TEXT("—") : Value;
	constexpr int32 KeyPadding = 28;
	
	switch (CurrentOutputFormat)
	{
	case EOutputFormat::HumanReadable:
		OutText += GetIndent(Indent);
		OutText += Key + TEXT(": ");

		
		if (Key.Len() < KeyPadding)
		{
			OutText += FString::ChrN(KeyPadding - Key.Len(), TEXT(' '));
		}

		OutText += DisplayValue + TEXT("\n");
		break;

	case EOutputFormat::Compact:
		OutText += GetIndent(Indent);
		OutText += Key + TEXT(": ") + DisplayValue + TEXT("; ");
		break;

	case EOutputFormat::Minimal:
	default:
		// Most compact format: Key=Value,
		OutText += Key + TEXT("=") + DisplayValue + TEXT(", ");
		break;
	}
}

// ===================================================================
// Name & String Helpers
// ===================================================================
FString BPR_Extractor_Base::CleanName(const FString& RawName)
{
    if (RawName.IsEmpty())
    {
        return RawName;
    }

    FString Result = RawName;

    // Remove GUID-like or numeric suffix after the last underscore
    int32 UnderscoreIndex = INDEX_NONE;
    if (RawName.FindLastChar('_', UnderscoreIndex))
    {
        FString Tail = RawName.Mid(UnderscoreIndex + 1);

        const bool bShouldCutTail = 
            (Tail.Len() >= 32 && Tail.Len() <= 40) ||	// Long GUID-style suffix
            (Tail.Len() >= 8 && Tail.IsNumeric());		// Numeric suffix (common in Materials, etc.)

        if (bShouldCutTail)
        {
            Result = RawName.Left(UnderscoreIndex);
        }
    }

    // Remove common technical prefixes
    if (Result.StartsWith(TEXT("K2Node_")))
    {
        Result = Result.Mid(8);
    }
    else if (Result.StartsWith(TEXT("K2_")))
    {
        Result = Result.Mid(3);
    }
    else if (Result.StartsWith(TEXT("bp_")) || Result.StartsWith(TEXT("BP_")))
    {
        Result = Result.Mid(3);
    }

    // Final cleanup
    Result.TrimStartAndEndInline();

    // If cleaning removed everything meaningful, fall back to original name
    return Result.IsEmpty() ? RawName : Result;
}

// ===================================================================
// Logging
// ===================================================================

void BPR_Extractor_Base::LogMessage(const FString& Msg) const
{
    if (Msg.IsEmpty())
    {
        return;
    }

    UE_LOG(LogBlueprintReader, Log, TEXT("[BPR_Extractor_%s] %s"), *ExtractorName, *Msg);
}

void BPR_Extractor_Base::LogWarning(const FString& Msg) const
{
    if (Msg.IsEmpty())
    {
        return;
    }

    UE_LOG(LogBlueprintReader, Warning, TEXT("[BPR_Extractor_%s] %s"), *ExtractorName, *Msg);
}

void BPR_Extractor_Base::LogError(const FString& Msg) const
{
    if (Msg.IsEmpty())
    {
        return;
    }

    UE_LOG(LogBlueprintReader, Error, TEXT("[BPR_Extractor_%s] %s"), *ExtractorName, *Msg);
}

// ===================================================================
// Property Helpers
// ===================================================================

bool BPR_Extractor_Base::IsUserVariable(FProperty* Property) const
{
	if (!Property)
	{
		return false;
	}

	// Skip system and internal properties
	if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_Config))
	{
		return false;
	}

	const FString Name = Property->GetName();

	// Skip well-known internal variables that are rarely useful to users
	if (Name.StartsWith(TEXT("UberGraphFrame")) ||
		Name.StartsWith(TEXT("PrimaryComponentTick")) ||
		Name.StartsWith(TEXT("bReplicates")) ||
		Name.StartsWith(TEXT("bGeneratedClassIsAuthoritative")) ||
		Name == TEXT("bCanEverAffectNavigation") ||
		Name == TEXT("bIsVariable"))
	{
		return false;
	}

	// Skip multicast delegates starting with "On" (usually shown in Bindings section)
	if (Name.StartsWith(TEXT("On")) && Property->IsA<FMulticastDelegateProperty>())
	{
		return false;
	}

	// Keep only properties that are editable or visible in Blueprint
	return Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible);
}

FString BPR_Extractor_Base::GetPropertyTypeDetailed(FProperty* Property) const
{
	if (!Property)
	{
		return TEXT("Unknown");
	}

	FString TypeStr = Property->GetCPPType();

	// Handle container types
	if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		FString ElemType = ArrayProp->Inner ? ArrayProp->Inner->GetCPPType() : TEXT("Unknown");
		TypeStr = FString::Printf(TEXT("TArray<%s>"), *ElemType);
	}
	else if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		FString KeyType   = MapProp->KeyProp   ? MapProp->KeyProp->GetCPPType()   : TEXT("Unknown");
		FString ValueType = MapProp->ValueProp ? MapProp->ValueProp->GetCPPType() : TEXT("Unknown");
		TypeStr = FString::Printf(TEXT("TMap<%s, %s>"), *KeyType, *ValueType);
	}
	else if (FSetProperty* SetProp = CastField<FSetProperty>(Property))
	{
		FString ElemType = SetProp->ElementProp ? SetProp->ElementProp->GetCPPType() : TEXT("Unknown");
		TypeStr = FString::Printf(TEXT("TSet<%s>"), *ElemType);
	}
	else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		if (UEnum* Enum = EnumProp->GetEnum())
		{
			TypeStr = Enum->GetName();
		}
	}

	return TypeStr;
}

FString BPR_Extractor_Base::GetPropertyDefaultValue(FProperty* Property, UObject* Object) const
{
	if (!Property || !Object)
	{
		return TEXT("None");
	}

	FString ValueStr;

	// Export the default value using Unreal's built-in system
	Property->ExportText_Direct(
		ValueStr,
		Property->ContainerPtrToValuePtr<void>(Object),
		nullptr,
		Object,
		PPF_PropertyWindow
	);

	ValueStr.TrimStartInline();
	ValueStr.TrimEndInline();

	// Normalize common "empty" values to a consistent "None"
	if (ValueStr.IsEmpty() ||
		ValueStr == TEXT("None") ||
		ValueStr == TEXT("0") ||
		ValueStr == TEXT("false") ||
		ValueStr == TEXT("()") ||
		ValueStr == TEXT("(0,0,0,0)") ||
		ValueStr == TEXT("(0,0)"))
	{
		return TEXT("None");
	}

	return ValueStr;
}

void BPR_Extractor_Base::AppendStructFields(FStructProperty* StructProp, FString& OutText, int32 Indent) const
{
	if (!StructProp)
	{
		return;
	}

	UScriptStruct* Struct = StructProp->Struct;
	if (!Struct)
	{
		return;
	}

	const FString IndentStr = GetIndent(Indent);

	OutText += IndentStr + FString::Printf(TEXT("### Struct: %s\n\n"), *Struct->GetName());

	// Creating a table using new helpers
	BeginMarkdownTable(OutText, { TEXT("Field"), TEXT("Type"), TEXT("Description") }, Indent);

	for (TFieldIterator<FProperty> FieldIt(Struct); FieldIt; ++FieldIt)
	{
		FProperty* Field = *FieldIt;
		if (!Field)
		{
			continue;
		}

		FString FieldName = CleanName(Field->GetName());
		FString FieldType = GetPropertyTypeDetailed(Field);
		FString Description = GetPropertyDescription(Field);   // static вызов

		AppendTableRow(OutText, { FieldName, FieldType, Description }, Indent);
	}

	OutText += TEXT("\n");
}

FString BPR_Extractor_Base::GetPropertyDescription(const FProperty* Property)
{
	if (!Property)
	{
		return TEXT("-");
	}

	FString Desc = Property->GetToolTipText().ToString();

	// Clean up newlines and trim
	Desc.ReplaceInline(TEXT("\r\n"), TEXT(" "));
	Desc.ReplaceInline(TEXT("\n"), TEXT(" "));
	Desc.TrimStartAndEndInline();

	return Desc.IsEmpty() ? TEXT("-") : Desc;
}

// ===================================================================
// Pin & Node Helpers
// ===================================================================

FString BPR_Extractor_Base::GetPinDisplayName(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return TEXT("None");
	}

	// Prefer user-defined friendly name
	if (!Pin->PinFriendlyName.IsEmpty())
	{
		return Pin->PinFriendlyName.ToString();
	}

	// Fall back to technical pin name
	if (!Pin->PinName.IsNone())
	{
		return Pin->PinName.ToString();
	}

	return TEXT("Unknown");
}

FString BPR_Extractor_Base::GetPinDetails(const UEdGraphPin* Pin) const
{
	if (!Pin)
	{
		return TEXT("None");
	}

	// Use explicit default value if set
	if (!Pin->DefaultValue.IsEmpty())
	{
		return Pin->DefaultValue;
	}

	// Show link information if connected
	if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0] && Pin->LinkedTo[0]->GetOwningNode())
	{
		FString SourceNodeName = GetReadableNodeName(Pin->LinkedTo[0]->GetOwningNode());
		return FString::Printf(TEXT("<linked from %s>"), *SourceNodeName);
	}

	// No value or connection
	return TEXT("default");
}

FString BPR_Extractor_Base::GetReadableNodeName(const UEdGraphNode* Node) const
{
	if (!Node)
	{
		return TEXT("None");
	}

	// Custom Event
	if (const UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(Node))
	{
		if (CustomEvent->CustomFunctionName != NAME_None)
		{
			return CustomEvent->CustomFunctionName.ToString();
		}
	}

	// Standard Events (BeginPlay, Tick, OnClicked, etc.)
	if (const UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
	{
		FName EventName = EventNode->EventReference.GetMemberName();
		if (EventName != NAME_None)
		{
			return EventName.ToString();
		}
	}

	// Variable Get / Set
	if (const UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(Node))
	{
		return FString::Printf(TEXT("Get %s"), *VarGet->VariableReference.GetMemberName().ToString());
	}

	if (const UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
	{
		return FString::Printf(TEXT("Set %s"), *VarSet->VariableReference.GetMemberName().ToString());
	}

	// Call Function
	if (const UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
	{
		FString Params;
		for (UEdGraphPin* Pin : CallFunc->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				continue;

			Params += FString::Printf(TEXT("%s=%s "), *Pin->PinName.ToString(), *GetPinDetails(Pin));
		}
		Params.TrimEndInline();

		FString FuncName = CallFunc->FunctionReference.GetMemberName().ToString();
		return FString::Printf(TEXT("Call %s(%s)"), *FuncName, *Params);
	}

	// Control flow
	if (const UK2Node_IfThenElse* Branch = Cast<UK2Node_IfThenElse>(Node))
	{
		return FString::Printf(TEXT("If %s"), *GetPinDetails(Branch->GetConditionPin()));
	}

	if (const UK2Node_Switch* Switch = Cast<UK2Node_Switch>(Node))
	{
		return FString::Printf(TEXT("Switch on %s"), *GetPinDetails(Switch->GetSelectionPin()));
	}

	// Special nodes
	if (Node->IsA(UK2Node_CallDelegate::StaticClass()))     return TEXT("Call Delegate");
	if (Node->IsA(UK2Node_Tunnel::StaticClass()))           return TEXT("Macro Tunnel");
	if (Node->IsA(UK2Node_FunctionEntry::StaticClass()))    return TEXT("Function Entry");
	if (Node->IsA(UK2Node_FunctionResult::StaticClass()))   return TEXT("Function Result");
	if (Node->IsA(UK2Node_ComponentBoundEvent::StaticClass())) return TEXT("Component Bound Event");

	// Fallback
	FString Title = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
	return Title.IsEmpty() ? Node->GetName() : Title;
}

bool BPR_Extractor_Base::IsComputationalNode(const UEdGraphNode* Node)
{
	if (!Node)
	{
		return false;
	}

	// Pure data-flow nodes
	if (Node->IsA(UK2Node_Knot::StaticClass()) ||
		Node->IsA(UK2Node_MathExpression::StaticClass()) ||
		Node->IsA(UK2Node_DynamicCast::StaticClass()) ||
		Node->IsA(UK2Node_CastByteToEnum::StaticClass()) ||
		Node->IsA(UK2Node_Select::StaticClass()) ||
		Node->IsA(UK2Node_VariableGet::StaticClass()))
	{
		return true;
	}

	// Variable Set without execution pin is treated as data-only
	if (Node->IsA(UK2Node_VariableSet::StaticClass()))
	{
		if (!HasExecInput(Node))
		{
			return true;
		}
	}

	// CallFunction is computational if it's BlueprintPure or has no exec input
	if (const UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
	{
		if (UFunction* Func = CallFunc->GetTargetFunction())
		{
			if (Func->HasAnyFunctionFlags(FUNC_BlueprintPure))
			{
				return true;
			}
		}

		if (!HasExecInput(Node))
		{
			return true;
		}
	}

	return false;
}

bool BPR_Extractor_Base::HasExecInput(const UEdGraphNode* Node)
{
	if (!Node)
	{
		return false;
	}

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin && 
			Pin->Direction == EGPD_Input && 
			Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
		{
			return true;
		}
	}

	return false;
}

// ===================================================================
// Graph Traversal Helpers
// ===================================================================

UK2Node_FunctionEntry* BPR_Extractor_Base::FindFunctionEntryNodeInGraph(const UEdGraph* Graph)
{
	if (!Graph)
	{
		return nullptr;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
		{
			return Entry;
		}
	}

	return nullptr;
}

UK2Node_FunctionResult* BPR_Extractor_Base::FindFunctionResultNodeInGraph(const UEdGraph* Graph)
{
	if (!Graph)
	{
		return nullptr;
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UK2Node_FunctionResult* Result = Cast<UK2Node_FunctionResult>(Node))
		{
			return Result;
		}
	}

	return nullptr;
}

FString BPR_Extractor_Base::GetFunctionSignature(const UEdGraph* Graph) const
{
	if (!Graph)
	{
		return TEXT("None");
	}

	UK2Node_FunctionEntry* Entry = FindFunctionEntryNodeInGraph(Graph);
	UK2Node_FunctionResult* Result = FindFunctionResultNodeInGraph(Graph);

	TArray<FString> Inputs;
	TArray<FString> Outputs;

	auto GetPinLabel = [](const UEdGraphPin* Pin) -> FString
	{
		if (!Pin)
		{
			return TEXT("Unknown");
		}

		const FString Display = Pin->GetDisplayName().ToString();
		return Display.IsEmpty() ? Pin->PinName.ToString() : Display;
	};

	// Collect input parameters from Function Entry node
	if (Entry)
	{
		for (UEdGraphPin* Pin : Entry->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				continue;
			}

			Inputs.Add(FString::Printf(TEXT("%s: %s"),
				*GetPinLabel(Pin),
				*Pin->PinType.PinCategory.ToString()));
		}
	}

	// Collect output parameters from Function Result node
	if (Result)
	{
		for (UEdGraphPin* Pin : Result->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Input || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				continue;
			}

			Outputs.Add(FString::Printf(TEXT("%s: %s"),
				*GetPinLabel(Pin),
				*Pin->PinType.PinCategory.ToString()));
		}
	}

	FString InputsStr = Inputs.Num() > 0 ? FString::Join(Inputs, TEXT(", ")) : TEXT("void");
	FString OutputsStr = Outputs.Num() > 0 ? FString::Join(Outputs, TEXT(", ")) : TEXT("void");

	return FString::Printf(TEXT("(%s) → %s"), *InputsStr, *OutputsStr);
}

FString BPR_Extractor_Base::GetMacroSignature(const UEdGraph* Graph) const
{
	if (!Graph)
	{
		return TEXT("None");
	}

	TArray<FString> Inputs;
	TArray<FString> Outputs;

	auto GetPinLabel = [](const UEdGraphPin* Pin) -> FString
	{
		if (!Pin)
		{
			return TEXT("Unknown");
		}

		const FString Display = Pin->GetDisplayName().ToString();
		return Display.IsEmpty() ? Pin->PinName.ToString() : Display;
	};

	// Macros are represented using UK2Node_Tunnel nodes
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UK2Node_Tunnel* Tunnel = Cast<UK2Node_Tunnel>(Node);
		if (!Tunnel)
		{
			continue;
		}

		for (UEdGraphPin* Pin : Tunnel->Pins)
		{
			if (!Pin)
			{
				continue;
			}

			FString PinDesc = FString::Printf(TEXT("%s: %s"),
				*GetPinLabel(Pin),
				*Pin->PinType.PinCategory.ToString());

			if (Pin->Direction == EGPD_Input)
			{
				Inputs.Add(PinDesc);
			}
			else if (Pin->Direction == EGPD_Output)
			{
				Outputs.Add(PinDesc);
			}
		}
	}

	FString InputsStr = Inputs.Num() > 0 ? FString::Join(Inputs, TEXT(", ")) : TEXT("void");
	FString OutputsStr = Outputs.Num() > 0 ? FString::Join(Outputs, TEXT(", ")) : TEXT("void");

	return FString::Printf(TEXT("(%s) → %s"), *InputsStr, *OutputsStr);
}

// ===================================================================
// Graph Processing
// ===================================================================

void BPR_Extractor_Base::ProcessNodeSequence(
    UEdGraphNode* Node,
    int32 IndentLevel,
    TSet<UEdGraphNode*>& Visited,
    FString& OutExecText,
    FString& OutDataText) const
{
	if (!Node || Visited.Contains(Node))
	{
		if (Node)
		{
			OutExecText += FString::Printf(TEXT("%*s[Loop detected: %s]\n"),
				IndentLevel * 2, TEXT(""), *Node->GetName());
		}
		return;
	}

	Visited.Add(Node);

	// Build readable node title with optional comment
	FString NodeTitle = GetReadableNodeName(Node);
	if (!Node->NodeComment.IsEmpty())
	{
		NodeTitle += FString::Printf(TEXT(" // %s"), *Node->NodeComment);
	}

	// Determine output section
	if (HasExecInput(Node))
	{
		OutExecText += FString::Printf(TEXT("%*s- %s\n"),
			IndentLevel * 2, TEXT(""), *NodeTitle);
	}
	else if (IsComputationalNode(Node))
	{
		OutDataText += FString::Printf(TEXT("%*s[pure] %s\n"),
			IndentLevel * 2, TEXT(""), *NodeTitle);
	}
	else
	{
		OutExecText += FString::Printf(TEXT("%*s- %s\n"),
			IndentLevel * 2, TEXT(""), *NodeTitle);
	}

	// 3. Data-flow (non-exec pins)
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			continue;

		FString PinName = GetPinDisplayName(Pin);

		for (UEdGraphPin* LinkedTo : Pin->LinkedTo)
		{
			if (!LinkedTo || !LinkedTo->GetOwningNode())
				continue;

			FString TargetNodeName = GetReadableNodeName(LinkedTo->GetOwningNode());
			FString TargetPinName = GetPinDisplayName(LinkedTo);

			if (LinkedTo->GetOwningNode()->IsA(UK2Node_Knot::StaticClass()))
			{
				TargetNodeName = TEXT("Reroute ") + TargetNodeName;
			}

			OutDataText += FString::Printf(TEXT("%*s[data] %s.%s → %s.%s\n"),
				(IndentLevel + 1) * 2, TEXT(""), *NodeTitle, *PinName,
				*TargetNodeName, *TargetPinName);

			// Recurse only into computational (pure) nodes for data flow
			if (!Visited.Contains(LinkedTo->GetOwningNode()) &&
				IsComputationalNode(LinkedTo->GetOwningNode()))
			{
				ProcessNodeSequence(
					LinkedTo->GetOwningNode(),
					IndentLevel + 1,
					Visited,
					OutExecText,
					OutDataText
				);
			}
		}
	}

	// 4. Exec flow recursion
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin || Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
			continue;

		FString Label = Pin->PinFriendlyName.IsEmpty() 
			? TEXT("then") 
			: Pin->PinFriendlyName.ToString();

		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			if (!LinkedPin || !LinkedPin->GetOwningNode())
				continue;

			OutExecText += FString::Printf(TEXT("%*s  [%s] →\n"),
				IndentLevel * 2, TEXT(""), *Label);

			ProcessNodeSequence(
				LinkedPin->GetOwningNode(),
				IndentLevel + 1,
				Visited,
				OutExecText,
				OutDataText
			);
		}
	}
}

void BPR_Extractor_Base::AppendGraphSequence(
	const UEdGraph* Graph,
	FString& OutExecText,
	FString& OutDataText) const
{
	if (!Graph || Graph->Nodes.Num() == 0)
	{
		return;
	}

	TSet<UEdGraphNode*> Visited;

	// Try to find a good starting point
	UEdGraphNode* StartNode = FindFunctionEntryNodeInGraph(Graph);

	// For Event Graphs without FunctionEntry (common case), start from the first Event node
	if (!StartNode)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node && Node->IsA(UK2Node_Event::StaticClass()))
			{
				StartNode = Node;
				break;
			}
		}
	}

	// Fallback to first node
	if (!StartNode && !Graph->Nodes.IsEmpty())
	{
		StartNode = Graph->Nodes[0];
	}

	// Main execution flow
	if (StartNode)
	{
		ProcessNodeSequence(StartNode, 0, Visited, OutExecText, OutDataText);
	}

	// Pure/computational nodes not reached by exec flow
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node || Visited.Contains(Node) || !IsComputationalNode(Node))
			continue;

		FString NodeTitle = GetReadableNodeName(Node);
		if (!Node->NodeComment.IsEmpty())
		{
			NodeTitle += FString::Printf(TEXT(" // %s"), *Node->NodeComment);
		}

		OutDataText += FString::Printf(TEXT("[pure] %s (no exec)\n"), *NodeTitle);
		Visited.Add(Node);

		// Process data connections
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				continue;

			FString PinName = GetPinDisplayName(Pin);

			for (UEdGraphPin* LinkedTo : Pin->LinkedTo)
			{
				if (!LinkedTo || !LinkedTo->GetOwningNode())
					continue;

				FString TargetNodeName = GetReadableNodeName(LinkedTo->GetOwningNode());
				FString TargetPinName = GetPinDisplayName(LinkedTo);

				if (LinkedTo->GetOwningNode()->IsA(UK2Node_Knot::StaticClass()))
				{
					TargetNodeName = TEXT("Reroute ") + TargetNodeName;
				}

				OutDataText += FString::Printf(TEXT("   [data] %s.%s → %s.%s\n"),
					*NodeTitle, *PinName, *TargetNodeName, *TargetPinName);

				if (!Visited.Contains(LinkedTo->GetOwningNode()))
				{
					ProcessNodeSequence(
						LinkedTo->GetOwningNode(),
						1,
						Visited,
						OutExecText,
						OutDataText
					);
				}
			}
		}
	}
}

void BPR_Extractor_Base::AppendGraphs(UBlueprint* Blueprint, FString& OutText) const
{
    if (!Blueprint)
    {
        return;
    }

    OutText += TEXT("## Graphs\n\n");

    // === Construction Script ===
    {
        UEdGraph* ConstructionGraph = FindConstructionScriptGraph(Blueprint);

        AppendSectionHeader(OutText, TEXT("Construction Script"));

        if (ConstructionGraph)
        {
            FString ExecFlow, DataFlow;
            AppendGraphSequence(ConstructionGraph, ExecFlow, DataFlow);
            OutText += ExecFlow + DataFlow + TEXT("\n\n");
        }
        else
        {
            OutText += TEXT("No Construction Script defined.\n\n");
        }
    }

    // === Event Graphs (UbergraphPages) ===
    if (Blueprint->UbergraphPages.Num() > 0)
    {
        for (UEdGraph* Graph : Blueprint->UbergraphPages)
        {
            if (!Graph) continue;

            FString GraphName = CleanName(Graph->GetName());
            AppendSectionHeader(OutText, FString::Printf(TEXT("Event Graph: %s"), *GraphName));

            FString ExecFlow, DataFlow;
            AppendGraphSequence(Graph, ExecFlow, DataFlow);
            OutText += ExecFlow + DataFlow + TEXT("\n\n");
        }
    }
    else
    {
        OutText += TEXT("Note: No Event Graphs found.\n\n");
    }

    // === Function Graphs (кроме ConstructionScript) ===
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (!Graph || Graph->GetName() == TEXT("ConstructionScript"))
            continue;

        FString Signature = GetFunctionSignature(Graph);
        FString GraphName = CleanName(Graph->GetName());

        OutText += FString::Printf(TEXT("### Function: %s\n- Signature: %s\n\n"),
            *GraphName, *Signature);

        FString ExecFlow, DataFlow;
        AppendGraphSequence(Graph, ExecFlow, DataFlow);
        OutText += ExecFlow + DataFlow + TEXT("\n\n");
    }

    // === Macro Graphs ===
    for (UEdGraph* Graph : Blueprint->MacroGraphs)
    {
        if (!Graph) continue;

        FString Signature = GetMacroSignature(Graph);
        FString GraphName = CleanName(Graph->GetName());

        OutText += FString::Printf(TEXT("### Macro: %s\n- Signature: %s\n\n"),
            *GraphName, *Signature);

        FString ExecFlow, DataFlow;
        AppendGraphSequence(Graph, ExecFlow, DataFlow);
        OutText += ExecFlow + DataFlow + TEXT("\n\n");
    }
}

UEdGraph* BPR_Extractor_Base::FindConstructionScriptGraph(UBlueprint* Blueprint) const
{
	if (!Blueprint)
	{
		return nullptr;
	}

	// Самый надёжный и актуальный способ в современных версиях UE
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph && Graph->GetName() == TEXT("ConstructionScript"))
		{
			return Graph;
		}
	}

	// Fallback (на всякий случай, хотя в UE 5.4+ практически не нужен)
	if (Blueprint->SimpleConstructionScript)
	{
		// В старых версиях иногда граф лежал здесь, но сейчас это не используется
		// Если в будущем Epic снова изменит поведение — можно будет доработать
		UE_LOG(LogBlueprintReader, Verbose, TEXT("[%s] SimpleConstructionScript found, but no ConstructionScript graph in FunctionGraphs"), 
			   *GetExtractorName());
	}

	return nullptr;
}

// ===================================================================
// Blueprint Structure
// ===================================================================

void BPR_Extractor_Base::AppendBlueprintInfo(UBlueprint* Blueprint, FString& OutText) const
{
	if (!Blueprint)
	{
		return;
	}

	OutText += FString::Printf(TEXT("## Blueprint Info: %s\n"), *Blueprint->GetName());

	// Parent class
	if (Blueprint->ParentClass)
	{
		FString ParentName = Blueprint->ParentClass->GetName();
		OutText += FString::Printf(TEXT("Parent Class: %s\n"), *ParentName);
	}
	else
	{
		OutText += TEXT("Parent Class: None\n");
	}

	// Replication info from CDO (relevant for Actors and ActorComponents)
	if (UClass* GenClass = Blueprint->GeneratedClass)
	{
		if (UObject* CDO = GenClass->GetDefaultObject())
		{
			if (AActor* ActorCDO = Cast<AActor>(CDO))
			{
				OutText += ActorCDO->GetIsReplicated()
					? TEXT("Replicates: Yes\n")
					: TEXT("Replicates: No\n");
			}
			else if (UActorComponent* ComponentCDO = Cast<UActorComponent>(CDO))
			{
				OutText += ComponentCDO->GetIsReplicated()
					? TEXT("Replicates: Yes\n")
					: TEXT("Replicates: No\n");
			}
		}
	}

	OutText += TEXT("\n");
}

void BPR_Extractor_Base::AppendVariables(UBlueprint* Blueprint, FString& OutText) const
{
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return;
	}

	UClass* Class = Blueprint->GeneratedClass;
	UObject* CDO = Class->GetDefaultObject();
	if (!CDO)
	{
		return;
	}

	// Collect all user variables
	TArray<FProperty*> UserVariables;
	for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (Property && IsUserVariable(Property))
		{
			UserVariables.Add(Property);
		}
	}

	if (UserVariables.Num() == 0)
	{
		AppendSectionHeader(OutText, TEXT("Custom Variables"));
		OutText += TEXT("No custom variables defined.\n\n");
		return;
	}

	AppendSectionHeader(OutText, TEXT("Custom Variables"));

	// Wide table with important property flags
	BeginMarkdownTable(OutText, {
		TEXT("Name"),
		TEXT("Type"),
		TEXT("Default Value"),
		TEXT("Edit"),
		TEXT("Visible"),
		TEXT("ExposeOnSpawn"),
		TEXT("InstanceEditable"),
		TEXT("Private"),
		TEXT("SaveGame"),
		TEXT("Category"),
		TEXT("Description")
	});

	for (FProperty* Property : UserVariables)
	{
		FString PropName     = CleanName(Property->GetName());
		FString PropType     = GetPropertyTypeDetailed(Property);
		FString DefaultVal   = GetPropertyDefaultValue(Property, CDO);
		FString Description  = GetPropertyDescription(Property);
		FString Category     = Property->GetMetaData(TEXT("Category"));

		// Simple boolean flags for better readability
		FString EditFlag         = Property->HasAnyPropertyFlags(CPF_Edit) ? TEXT("Yes") : TEXT("-");
		FString VisibleFlag      = Property->HasAnyPropertyFlags(CPF_BlueprintVisible) ? TEXT("Yes") : TEXT("-");
		FString ExposeOnSpawn    = Property->HasAnyPropertyFlags(CPF_ExposeOnSpawn) ? TEXT("Yes") : TEXT("-");
		FString InstanceEditable = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance) ? TEXT("Yes") : TEXT("-");
		FString PrivateFlag      = Property->GetBoolMetaData(TEXT("Private")) ? TEXT("Yes") : TEXT("-");
		FString SaveGameFlag     = Property->HasAnyPropertyFlags(CPF_SaveGame) ? TEXT("Yes") : TEXT("-");

		if (Category.IsEmpty()) Category = TEXT("-");

		AppendTableRow(OutText, {
			PropName,
			PropType,
			DefaultVal,
			EditFlag,
			VisibleFlag,
			ExposeOnSpawn,
			InstanceEditable,
			PrivateFlag,
			SaveGameFlag,
			Category,
			Description
		});

		// Expand nested structs
		if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
		{
			OutText += TEXT("\n");
			AppendStructFields(StructProp, OutText);
		}
	}

	OutText += TEXT("\n");
}

void BPR_Extractor_Base::AppendReplicationInfo(const UClass* Class, FString& OutText)
{
	if (!Class)
	{
		return;
	}

	OutText += TEXT("## Replicated Variables\n");

	bool HasReplicated = false;

	for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (!Property || !Property->HasAnyPropertyFlags(CPF_Net))
		{
			continue;
		}

		FString PropName = CleanName(Property->GetName());
		FString RepNotify = Property->GetMetaData(TEXT("ReplicatedUsing"));
		FString Condition = Property->GetMetaData(TEXT("RepCondition"));

		OutText += FString::Printf(TEXT("- %s (Replicated"), *PropName);

		if (!RepNotify.IsEmpty())
		{
			OutText += FString::Printf(TEXT(", Using: %s"), *RepNotify);
		}

		if (!Condition.IsEmpty())
		{
			OutText += FString::Printf(TEXT(", Condition: %s"), *Condition);
		}

		OutText += TEXT(")\n");

		HasReplicated = true;
	}

	if (!HasReplicated)
	{
		OutText += TEXT("- None\n");
	}

	OutText += TEXT("\n");
}

// ===================================================================
// Table Helpers (Markdown)
// ===================================================================

void BPR_Extractor_Base::BeginMarkdownTable(FString& OutText, 
											const TArray<FString>& Headers, 
											int32 Indent /*= 0*/, 
											bool bBoldHeaders /*= true*/)
{
	if (Headers.Num() == 0)
	{
		return;
	}

	const FString IndentStr = GetIndent(Indent);

	// Header row with optional bold
	OutText += IndentStr;
	for (int32 i = 0; i < Headers.Num(); ++i)
	{
		if (bBoldHeaders)
		{
			OutText += TEXT("**") + Headers[i] + TEXT("**");
		}
		else
		{
			OutText += Headers[i];
		}

		if (i < Headers.Num() - 1)
		{
			OutText += TEXT(" | ");
		}
	}
	OutText += TEXT("\n");

	// Separator row (left aligned)
	OutText += IndentStr;
	for (int32 i = 0; i < Headers.Num(); ++i)
	{
		OutText += TEXT(":---");
		if (i < Headers.Num() - 1)
		{
			OutText += TEXT(" | ");
		}
	}
	OutText += TEXT("\n");
}

void BPR_Extractor_Base::AppendTableRow(FString& OutText, const TArray<FString>& Columns, int32 Indent /*= 0*/)
{
	if (Columns.Num() == 0)
	{
		return;
	}

	const FString IndentStr = GetIndent(Indent);

	OutText += IndentStr;
	for (int32 i = 0; i < Columns.Num(); ++i)
	{
		// Escape the vertical bar inside the cell, if there is one.
		FString Cell = Columns[i];
		Cell.ReplaceInline(TEXT("|"), TEXT("\\|"));

		OutText += Cell;

		if (i < Columns.Num() - 1)
		{
			OutText += TEXT(" | ");
		}
	}
	OutText += TEXT("\n");
}