// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Extractors/BPR_Extractor_ActorComponent.h"
#include "Engine/Blueprint.h"
#include "UObject/UnrealType.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Switch.h"
#include "K2Node_Tunnel.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_Knot.h"
#include "K2Node_MathExpression.h"
#include "K2Node_Select.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_CastByteToEnum.h"
#include "EdGraphSchema_K2.h"


BPR_Extractor_ActorComponent::BPR_Extractor_ActorComponent() {} //ToDo инициализируй ExtractorName = TEXT("Actor");
BPR_Extractor_ActorComponent::~BPR_Extractor_ActorComponent() {}

//---------------------------------------------
// Main entry point
//---------------------------------------------
void BPR_Extractor_ActorComponent::ProcessComponent(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
    if (!SelectedObject)
    {
        LogError(TEXT("SelectedObject is null!"));
        OutData.Structure = FText::FromString("Error: SelectedObject is null.");
        OutData.Graph = FText::FromString("Error: SelectedObject is null.");
        return;
    }

    UBlueprint* Blueprint = Cast<UBlueprint>(SelectedObject);
    if (!Blueprint)
    {
        LogWarning(TEXT("Selected object is not a Blueprint asset."));
        OutData.Structure = FText::FromString("Warning: Selected object is not a Blueprint.");
        OutData.Graph = FText::FromString("Warning: Selected object is not a Blueprint.");
        return;
    }

    FString TmpStructure = FString::Printf(TEXT("# Blueprint Structure: %s\n\n"), *Blueprint->GetName());
    FString TmpGraph = FString::Printf(TEXT("# Graphs Export for Component: %s\n\n"), *Blueprint->GetName());

    AppendBlueprintInfo(Blueprint, TmpStructure);
    AppendVariables(Blueprint, TmpStructure);
    AppendReplicationInfo(Blueprint->GeneratedClass, TmpStructure);
    AppendGraphs(Blueprint, TmpGraph);

    OutData.Structure = FText::FromString(TmpStructure);
    OutData.Graph = FText::FromString(TmpGraph);
}

//---------------------------------------------
// Logging
//---------------------------------------------
void BPR_Extractor_ActorComponent::LogMessage(const FString& Msg) { UE_LOG(LogTemp, Log, TEXT("[BPR_Extractor_ActorComponent] %s"), *Msg); }
void BPR_Extractor_ActorComponent::LogWarning(const FString& Msg) { UE_LOG(LogTemp, Warning, TEXT("[BPR_Extractor_ActorComponent] %s"), *Msg); }
void BPR_Extractor_ActorComponent::LogError(const FString& Msg) { UE_LOG(LogTemp, Error, TEXT("[BPR_Extractor_ActorComponent] %s"), *Msg); }

//---------------------------------------------
// General information about Blueprint
//---------------------------------------------
void BPR_Extractor_ActorComponent::AppendBlueprintInfo(UBlueprint* Blueprint, FString& OutText)
{
    if (!Blueprint || !Blueprint->ParentClass) return;

    FString ParentClassName = Blueprint->ParentClass->GetName();
        
    OutText += FString::Printf(TEXT("## Blueprint Info: %s\n"), *Blueprint->GetName());
    OutText += FString::Printf(TEXT("Parent Class: %s (Native)\n"), *ParentClassName);
    OutText += TEXT("Replication: ");


    if (UClass* GenClass = Blueprint->GeneratedClass)
    {
        if (UActorComponent* CDO = Cast<UActorComponent>(GenClass->GetDefaultObject()))
        {
            OutText += CDO->GetIsReplicated() ? TEXT("Replicating\n\n") : TEXT("Not replicating\n\n");
        }
        else
        {
            OutText += TEXT("N/A (not a component)\n\n");
        }
    }
}

//---------------------------------------------
// Variables
//---------------------------------------------
void BPR_Extractor_ActorComponent::AppendVariables(UBlueprint* Blueprint, FString& OutText)
{
    if (!Blueprint || !Blueprint->GeneratedClass) return;

    OutText += TEXT("## Custom Variables\n");
    OutText += TEXT("| Name | Type | Default Value | Flags | Category | Description |\n");
    OutText += TEXT("|------|------|---------------|-------|----------|-------------|\n");

    UClass* Class = Blueprint->GeneratedClass;
    UObject* CDO = Class->GetDefaultObject();

    for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (!Property || !IsUserVariable(Property)) continue;

        FString PropName = CleanName(Property->GetName());
        FString PropType = GetPropertyTypeDetailed(Property);
        FString DefaultVal = GetPropertyDefaultValue(Property, CDO);
        FString Description = Property->GetToolTipText().ToString();
        FString Category = Property->GetMetaData(TEXT("Category"));

        FString Flags;
        if (Property->HasAnyPropertyFlags(CPF_Edit)) Flags += TEXT("Edit ");
        if (Property->HasAnyPropertyFlags(CPF_BlueprintVisible)) Flags += TEXT("BlueprintVisible ");
        if (Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly)) Flags += TEXT("BlueprintReadOnly ");
        Flags = Flags.TrimEnd();

        OutText += FString::Printf(TEXT("| %s | %s | %s | %s | %s | %s |\n"),
            *PropName, *PropType, *DefaultVal, *Flags, *Category, *Description);

        if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            OutText += TEXT("\n### Struct: ") + PropName + TEXT("\n");
            AppendStructFields(StructProp, OutText);
        }
    }

    OutText += TEXT("\n");
}


FString BPR_Extractor_ActorComponent::GetPropertyTypeDetailed(FProperty* Property)
{
    if (!Property) return TEXT("Unknown");

    FString Type = Property->GetCPPType();

    if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
    {
        FString KeyType = MapProp->KeyProp ? MapProp->KeyProp->GetCPPType() : TEXT("Unknown");
        FString ValueType = MapProp->ValueProp ? MapProp->ValueProp->GetCPPType() : TEXT("Unknown");
        Type = FString::Printf(TEXT("TMap<%s, %s>"), *KeyType, *ValueType);
    }
    else if (FSetProperty* SetProp = CastField<FSetProperty>(Property))
    {
        FString ElemType = SetProp->ElementProp ? SetProp->ElementProp->GetCPPType() : TEXT("Unknown");
        Type = FString::Printf(TEXT("TSet<%s>"), *ElemType);
    }
    else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        FString ElemType = ArrayProp->Inner ? ArrayProp->Inner->GetCPPType() : TEXT("Unknown");
        Type = FString::Printf(TEXT("TArray<%s>"), *ElemType);
    }
    else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        if (EnumProp->GetEnum())
            Type = EnumProp->GetEnum()->GetName();
    }

    return Type;
}

void BPR_Extractor_ActorComponent::AppendStructFields(FStructProperty* StructProp, FString& OutText, int32 Indent)
{
    if (!StructProp) return;

    UScriptStruct* Struct = StructProp->Struct;
    if (!Struct) return;

    OutText += TEXT("| Field | Type | Description |\n");
    OutText += TEXT("|-------|------|-------------|\n");

    for (TFieldIterator<FProperty> FieldIt(Struct); FieldIt; ++FieldIt)
    {
        FProperty* Field = *FieldIt;
        FString FieldName = CleanName(Field->GetName());
        FString FieldType = GetPropertyTypeDetailed(Field);
        FString Desc = Field->GetToolTipText().ToString();

        OutText += FString::Printf(TEXT("| %s | %s | %s |\n"), *FieldName, *FieldType, *Desc);
    }

    OutText += TEXT("\n");
}

FString BPR_Extractor_ActorComponent::GetPropertyDefaultValue(FProperty* Property, UObject* Object)
{
    if (!Property || !Object) return TEXT("None");

    FString ValueStr;
    Property->ExportText_Direct(ValueStr, Property->ContainerPtrToValuePtr<void>(Object), nullptr, Object, PPF_PropertyWindow);
    return ValueStr.IsEmpty() ? TEXT("None") : ValueStr;
}

bool BPR_Extractor_ActorComponent::IsUserVariable(FProperty* Property)
{
    if (!Property) return false;
    if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated)) return false;
    FString Name = Property->GetName();
    if (Name.StartsWith(TEXT("UberGraphFrame")) || Name.StartsWith(TEXT("PrimaryComponentTick")) || Name.StartsWith(TEXT("bReplicates"))) return false;
    return true;
}

//---------------------------------------------
// Replication
//---------------------------------------------
void BPR_Extractor_ActorComponent::AppendReplicationInfo(UClass* Class, FString& OutText)
{
    if (!Class) return;

    OutText += TEXT("## Replicated Variables\n");

    bool HasReplicated = false;
    for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->HasAnyPropertyFlags(CPF_Net))
        {
            FString PropName = CleanName(Property->GetName());
            FString RepNotify = Property->GetMetaData(TEXT("ReplicatedUsing"));
            FString Condition = Property->GetMetaData(TEXT("RepCondition"));

            OutText += FString::Printf(TEXT("- %s (Replicated"), *PropName);
            if (!RepNotify.IsEmpty()) OutText += FString::Printf(TEXT(", Using: %s"), *RepNotify);
            if (!Condition.IsEmpty()) OutText += FString::Printf(TEXT(", Condition: %s"), *Condition);
            OutText += TEXT(")\n");

            HasReplicated = true;
        }
    }

    if (!HasReplicated) OutText += TEXT("- None\n\n");
}

//---------------------------------------------
// Graphs
//---------------------------------------------
void BPR_Extractor_ActorComponent::AppendGraphs(UBlueprint* Blueprint, FString& OutText)
{
    if (!Blueprint) return;

    OutText += TEXT("\n## Graphs\n");

    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (!Graph) continue;
        FString ExecFlow, DataFlow;
        OutText += FString::Printf(TEXT("### Event Graph: %s\n"), *Graph->GetName());
        AppendGraphSequence(Graph, ExecFlow, DataFlow);
        OutText += ExecFlow + TEXT("\n") + DataFlow + TEXT("\n\n");

    }

    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (!Graph) continue;
        FString Signature = GetFunctionSignature(Graph);
        FString ExecFlow, DataFlow;
        OutText += FString::Printf(TEXT("### Function: %s\n- Signature: %s\n"), *Graph->GetName(), *Signature);
        AppendGraphSequence(Graph, ExecFlow, DataFlow);
        OutText += ExecFlow + TEXT("\n") + DataFlow + TEXT("\n\n");

    }

    for (UEdGraph* Graph : Blueprint->MacroGraphs)
    {
        if (!Graph) continue;
        FString Signature = GetMacroSignature(Graph);
        FString ExecFlow, DataFlow;
        OutText += FString::Printf(TEXT("### Macro: %s\n- Signature: %s\n"), *Graph->GetName(), *Signature);
        AppendGraphSequence(Graph, ExecFlow, DataFlow);
        OutText += ExecFlow + TEXT("\n") + DataFlow + TEXT("\n\n");

    }
}

//---------------------------------------------
// Find Entry/Result Nodes
//---------------------------------------------
UK2Node_FunctionEntry* BPR_Extractor_ActorComponent::FindFunctionEntryNodeInGraph(UEdGraph* Graph)
{
    if (!Graph) return nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (auto* Entry = Cast<UK2Node_FunctionEntry>(Node))
            return Entry;
    }
    return nullptr;
}

UK2Node_FunctionResult* BPR_Extractor_ActorComponent::FindFunctionResultNodeInGraph(UEdGraph* Graph)
{
    if (!Graph) return nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UK2Node_FunctionResult* Casted = Cast<UK2Node_FunctionResult>(Node))
            return Casted;
    }
    return nullptr;
}

//---------------------------------------------
// Function/Macro signatures
//---------------------------------------------
FString BPR_Extractor_ActorComponent::GetFunctionSignature(UEdGraph* Graph)
{
    if (!Graph) return TEXT("None");

    UK2Node_FunctionEntry* Entry = FindFunctionEntryNodeInGraph(Graph);
    UK2Node_FunctionResult* Result = FindFunctionResultNodeInGraph(Graph);

    TArray<FString> Inputs, Outputs;

    auto GetPinLabel = [](UEdGraphPin* Pin)
    {
        const FString Display = Pin->GetDisplayName().ToString();
        return Display.IsEmpty() ? Pin->PinName.ToString() : Display;
    };

    if (Entry)
    {
        for (UEdGraphPin* Pin : Entry->Pins)
        {
            if (!Pin) continue;
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                Inputs.Add(FString::Printf(TEXT("%s: %s"),
                    *GetPinLabel(Pin),
                    *Pin->PinType.PinCategory.ToString()));
            }
        }
    }

    if (Result)
    {
        for (UEdGraphPin* Pin : Result->Pins)
        {
            if (!Pin) continue;
            if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                Outputs.Add(FString::Printf(TEXT("%s: %s"),
                    *GetPinLabel(Pin),
                    *Pin->PinType.PinCategory.ToString()));
            }
        }
    }

    return FString::Printf(TEXT("Inputs: (%s) Outputs: (%s)"),
        *FString::Join(Inputs, TEXT(", ")),
        *FString::Join(Outputs, TEXT(", ")));
}


FString BPR_Extractor_ActorComponent::GetMacroSignature(UEdGraph* Graph)
{
    if (!Graph) return TEXT("None");

    TArray<FString> Inputs, Outputs;

    auto GetPinLabel = [](UEdGraphPin* Pin)
    {
        const FString Display = Pin->GetDisplayName().ToString();
        return Display.IsEmpty() ? Pin->PinName.ToString() : Display;
    };

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Tunnel* Tunnel = Cast<UK2Node_Tunnel>(Node);
        if (!Tunnel) continue;

        for (UEdGraphPin* Pin : Tunnel->Pins)
        {
            if (!Pin) continue;

            FString PinDesc = FString::Printf(
                TEXT("%s: %s"),
                *GetPinLabel(Pin),
                *Pin->PinType.PinCategory.ToString()
            );

            if (Pin->Direction == EGPD_Input)
                Inputs.Add(PinDesc);
            else
                Outputs.Add(PinDesc);
        }
    }

    return FString::Printf(
        TEXT("Inputs: (%s) Outputs: (%s)"),
        *FString::Join(Inputs, TEXT(", ")),
        *FString::Join(Outputs, TEXT(", "))
    );
}


//---------------------------------------------
// Graph Sequence
//---------------------------------------------
void BPR_Extractor_ActorComponent::AppendGraphSequence(UEdGraph* Graph, FString& OutExecText, FString& OutDataText)
{
    if (!Graph || Graph->Nodes.Num() == 0) return;

    TSet<UEdGraphNode*> Visited;

    // 1. We are looking for the input node of the function
    UEdGraphNode* StartNode = FindFunctionEntryNodeInGraph(Graph);

    // 2. If this is not a function, start from the first node
    if (!StartNode)
        StartNode = Graph->Nodes.Num() > 0 ? Graph->Nodes[0] : nullptr;

    // 3. EXEC chain bypass
    if (StartNode)
    {
        ProcessNodeSequence(StartNode, 0, Visited, OutExecText, OutDataText);
    }

    // 4. Processing of computing (pure) nodes that are not included in the exec chain
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node || Visited.Contains(Node)) 
            continue;

        if (IsComputationalNode(Node))
        {
            FString NodeTitle = GetReadableNodeName(Node);
            if (!Node->NodeComment.IsEmpty())
                NodeTitle += FString::Printf(TEXT(" // %s"), *Node->NodeComment);

            OutDataText += FString::Printf(TEXT("[pure] %s (no exec)\n"), *NodeTitle);
            Visited.Add(Node);

            // 4.1 Обход всех data-пинов
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (!Pin || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) 
                    continue;

                FString PinName = GetPinDisplayName(Pin);

                for (UEdGraphPin* LinkedTo : Pin->LinkedTo)
                {
                    if (!LinkedTo || !LinkedTo->GetOwningNode()) continue;

                    FString TargetNodeName = GetReadableNodeName(LinkedTo->GetOwningNode());
                    FString TargetPinName = GetPinDisplayName(LinkedTo);

                    // special handling для reroute
                    if (LinkedTo->GetOwningNode()->IsA(UK2Node_Knot::StaticClass()))
                    {
                        TargetNodeName = TEXT("Reroute") + TargetNodeName;
                    }

                    OutDataText += FString::Printf(TEXT("   [data] %s.%s → %s.%s\n"),
                        *NodeTitle, *PinName, *TargetNodeName, *TargetPinName);

                    // recursive traversal for data-flow chain
                    if (!Visited.Contains(LinkedTo->GetOwningNode()))
                    {
                        ProcessNodeSequence(LinkedTo->GetOwningNode(), 1, Visited, OutExecText, OutDataText);
                    }
                }
            }
        }
    }
}



void BPR_Extractor_ActorComponent::ProcessNodeSequence(
    UEdGraphNode* Node, 
    int32 IndentLevel, 
    TSet<UEdGraphNode*>& Visited, 
    FString& OutExecText,
    FString& OutDataText)
{
    if (!Node || Visited.Contains(Node))
    {
        if (Node)
            OutExecText += FString::Printf(TEXT("%*s[Loop Detected: %s]\n"), 
                IndentLevel * 2, TEXT(""), *Node->GetName());
        return;
    }

    Visited.Add(Node);

    // ----------------------------
    // 1. Readable host name
    // ----------------------------
    FString NodeTitle = GetReadableNodeName(Node);
    if (!Node->NodeComment.IsEmpty())
        NodeTitle += FString::Printf(TEXT(" // %s"), *Node->NodeComment);

    // ----------------------------
    // 2. Exec path or pure node
    // ----------------------------
    if (HasExecInput(Node))
    {
        OutExecText += FString::Printf(TEXT("%*s- %s\n"), 
            IndentLevel * 2, TEXT(""), *NodeTitle);
    }
    else if (IsComputationalNode(Node))
    {
        // Node without exec - output to data-flow
        OutDataText += FString::Printf(TEXT("%*s[pure] %s\n"), 
            IndentLevel * 2, TEXT(""), *NodeTitle);
    }
    else
    {
        // Node without exec, does not calculate (for example, Knot) - displayed for readability
        OutExecText += FString::Printf(TEXT("%*s- %s\n"), 
            IndentLevel * 2, TEXT(""), *NodeTitle);
    }

    // ----------------------------
    // 3. Data-flow — non-exec pins
    // ----------------------------
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) 
            continue;

        FString PinName = GetPinDisplayName(Pin);

        for (UEdGraphPin* LinkedTo : Pin->LinkedTo)
        {
            if (!LinkedTo || !LinkedTo->GetOwningNode()) continue;

            FString TargetNodeName = GetReadableNodeName(LinkedTo->GetOwningNode());
            FString TargetPinName = GetPinDisplayName(LinkedTo);

            if (LinkedTo->GetOwningNode()->IsA(UK2Node_Knot::StaticClass()))
                TargetNodeName = TEXT("Reroute ") + TargetNodeName;

            OutDataText += FString::Printf(TEXT("%*s[data] %s.%s → %s.%s\n"),
                (IndentLevel + 1) * 2, TEXT(""), *NodeTitle, *PinName, *TargetNodeName, *TargetPinName);

            // Recursive traversal for data-flow
            if (!Visited.Contains(LinkedTo->GetOwningNode()) &&
                IsComputationalNode(LinkedTo->GetOwningNode()))
            {
                ProcessNodeSequence(LinkedTo->GetOwningNode(), 
                    IndentLevel + 1, Visited, OutExecText, OutDataText);
            }
        }
    }

    // ----------------------------
    // 4. Exec recursion
    // ----------------------------
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin || Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec) 
            continue;

        FString Label = Pin->PinFriendlyName.IsEmpty() 
            ? TEXT("then") 
            : Pin->PinFriendlyName.ToString();

        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
        {
            if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;

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

//---------------------------------------------
// Node & Pin Helpers
//---------------------------------------------
FString BPR_Extractor_ActorComponent::GetReadableNodeName(UEdGraphNode* Node)
{
    if (!Node) return TEXT("None");

    FString BaseName = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();

    if (UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
    {
        FString Params;
        for (UEdGraphPin* Pin : CallFunc->Pins)
        {
            if (!Pin) continue;
            if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                Params += FString::Printf(TEXT("%s=%s "), *Pin->PinName.ToString(), *GetPinDetails(Pin));
            }
        }
        return FString::Printf(TEXT("CallFunction: %s(%s)"), *CallFunc->GetFunctionName().ToString(), *Params.TrimEnd());
    }

    if (UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
    {
        return FString::Printf(TEXT("VariableSet: %s = <value>"), *VarSet->VariableReference.GetMemberName().ToString());
    }

    if (UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(Node))
    {
        return FString::Printf(TEXT("VariableGet: %s"), *VarGet->VariableReference.GetMemberName().ToString());
    }

    if (UK2Node_IfThenElse* Branch = Cast<UK2Node_IfThenElse>(Node))
    {
        return FString::Printf(TEXT("Branch (Condition: %s)"), *GetPinDetails(Branch->GetConditionPin()));
    }

    if (UK2Node_Switch* Switch = Cast<UK2Node_Switch>(Node))
    {
        return FString::Printf(TEXT("Switch (Selection: %s)"), *GetPinDetails(Switch->GetSelectionPin()));
    }

    if (Cast<UK2Node_CallDelegate>(Node)) return TEXT("CallDelegate");
    if (Cast<UK2Node_Tunnel>(Node)) return TEXT("Tunnel");
    if (Cast<UK2Node_FunctionEntry>(Node)) return TEXT("FunctionEntry");
    if (Cast<UK2Node_FunctionResult>(Node)) return TEXT("FunctionResult");

    return BaseName.IsEmpty() ? Node->GetName() : BaseName;
}

FString BPR_Extractor_ActorComponent::GetPinDetails(UEdGraphPin* Pin)
{
    if (!Pin) return TEXT("None");
    if (!Pin->DefaultValue.IsEmpty()) return Pin->DefaultValue;
    if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0] && Pin->LinkedTo[0]->GetOwningNode())
    {
        return FString::Printf(TEXT("<linked from %s>"), *Pin->LinkedTo[0]->GetOwningNode()->GetName());
    }
    return TEXT("default");
}

FString BPR_Extractor_ActorComponent::GetPinDisplayName(UEdGraphPin* Pin)
{
    if (!Pin) return TEXT("None");
    if (!Pin->PinFriendlyName.IsEmpty()) return Pin->PinFriendlyName.ToString();
    if (!Pin->PinName.IsNone()) return Pin->PinName.ToString();
    return TEXT("Unknown");
}

FString BPR_Extractor_ActorComponent::CleanName(const FString& RawName)
{
    FString Result = RawName;
    int32 UnderscoreIndex;
    if (RawName.FindLastChar('_', UnderscoreIndex))
    {
        FString Tail = RawName.Mid(UnderscoreIndex + 1);
        // If the tail is similar to a GUID (32+ characters), cut it off
        if (Tail.Len() >= 32)
        {
            Result = RawName.Left(UnderscoreIndex);
        }
    }
    return Result;
}

bool BPR_Extractor_ActorComponent::HasExecInput(UEdGraphNode* Node)
{
    if (!Node) return false;

    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin) continue;
        if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            return true;
    }
    return false;
}

bool BPR_Extractor_ActorComponent::IsComputationalNode(UEdGraphNode* Node)
{
    if (!Node) return false;

    // 1) Reroute/knot — proxy for data-flow
    if (Node->IsA(UK2Node_Knot::StaticClass()))
        return true;

    // 2) MathExpression (if you have such nodes in your project)
    if (Node->IsA(UK2Node_MathExpression::StaticClass()))
        return true;

    // 3) Casts / dynamic cast / byte->enum
    if (Node->IsA(UK2Node_DynamicCast::StaticClass()) ||
        Node->IsA(UK2Node_CastByteToEnum::StaticClass()))
        return true;

    // 4) Select - usually pure selection by condition
    if (Node->IsA(UK2Node_Select::StaticClass()))
        return true;

    // 5) VariableGet is data source
    if (Node->IsA(UK2Node_VariableGet::StaticClass()))
        return true;

    // 6) VariableSet without exec — can be data-only
    if (UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
    {
        if (!HasExecInput(Node))
            return true;
    }

    // 7) CallFunction — check if the target UFunction is BlueprintPure,
    //    or if the node has no exec input -> treated as computational
    if (UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
    {
        // Try to get UFunction* reference (works for most cases)
        if (UFunction* Func = CallFunc->GetTargetFunction())
        {
            if (Func->HasAnyFunctionFlags(FUNC_BlueprintPure))
                return true;
        }
        // Fallback: if node has no exec input, it's likely a pure/data node
        if (!HasExecInput(Node))
            return true;
    }

    return false;
}