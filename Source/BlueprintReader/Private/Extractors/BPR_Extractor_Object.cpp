// Copyright (c) 2026 Racoon Coder. All rights reserved.

#include "Extractors/BPR_Extractor_Object.h"

#include "Engine/Blueprint.h"
#include "UObject/UnrealType.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Tunnel.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Switch.h"
#include "K2Node_Knot.h"
#include "K2Node_MathExpression.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_CastByteToEnum.h"
#include "K2Node_Select.h"
#include "K2Node_InputActionEvent.h"
#include "K2Node_InputAxisEvent.h"
#include "K2Node_InputKeyEvent.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_MacroInstance.h"

// ===================================================================
// Construction / Destruction
// ===================================================================

BPR_Extractor_Object::BPR_Extractor_Object()
{
    SetExtractorName(TEXT("Blueprint Object"));
}

// ===================================================================
// Main Entry Points
// ===================================================================

void BPR_Extractor_Object::Process(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
    UBlueprint* Blueprint = Cast<UBlueprint>(SelectedObject);
    if (!Blueprint)
    {
        SetErrorData(OutData, TEXT("Selected object is not a Blueprint (UBlueprint cast failed)"));
        return;
    }

    if (!Blueprint->GeneratedClass)
    {
        SetErrorData(OutData, TEXT("Blueprint has no GeneratedClass"));
        return;
    }

    // Безопасно устанавливаем текущий Blueprint
    CurrentBlueprint = Blueprint;

    FString StructureText;
    FString GraphText;

    // Structure part
    ExtractStructure(StructureText, Blueprint);

    // Graphs part
    ExtractGraphs(GraphText, Blueprint);

    OutData.Structure = FText::FromString(StructureText);
    OutData.Graph     = FText::FromString(GraphText);
    OutData.Design    = FText::FromString(TEXT("N/A"));        // пока не используем
    OutData.AssetType = EAssetType::Blueprint;

    // Обнуляем, чтобы не держать указатель дольше необходимого
    CurrentBlueprint = nullptr;
}

FString BPR_Extractor_Object::Extract(const UObject* Asset)
{
    // Пока просто заглушка, чтобы не ломать интерфейс
    return FString();
}

// ===================================================================
// Structure Extraction
// ===================================================================

void BPR_Extractor_Object::ExtractStructure(FString& OutText, UBlueprint* Blueprint)
{
    if (!Blueprint) return;

    AppendBlueprintInfo(Blueprint, OutText);
    AppendVariables(Blueprint, OutText);
}

void BPR_Extractor_Object::AppendBlueprintInfo(UBlueprint* Blueprint, FString& OutText) const
{
    if (!Blueprint) return;

    AppendSectionHeader(OutText, TEXT("Blueprint Info"));

    OutText += FString::Printf(TEXT("**Name:** %s\n"), *Blueprint->GetName());

    if (Blueprint->ParentClass)
    {
        OutText += FString::Printf(TEXT("**Parent Class:** %s\n"), *Blueprint->ParentClass->GetName());
    }
    else
    {
        OutText += TEXT("**Parent Class:** None\n");
    }

    OutText += TEXT("\n");
}

void BPR_Extractor_Object::AppendVariables(UBlueprint* Blueprint, FString& OutText) const
{
    if (!Blueprint || !Blueprint->GeneratedClass)
        return;

    UClass* Class = Blueprint->GeneratedClass;
    UObject* CDO = Class->GetDefaultObject();
    if (!CDO) return;

    TArray<FProperty*> UserVariables;

    for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property && IsUserVariable(Property))
        {
            UserVariables.Add(Property);
        }
    }

    AppendSectionHeader(OutText, TEXT("Custom Variables"));

    if (UserVariables.Num() == 0)
    {
        OutText += TEXT("No custom variables defined.\n\n");
        return;
    }

    // Table header
    BeginMarkdownTable(OutText, {
        TEXT("Name"), TEXT("Type"), TEXT("Default Value"), TEXT("Category"), TEXT("Description")
    });

    for (FProperty* Property : UserVariables)
    {
        FString PropName     = CleanName(Property->GetName());
        FString PropType     = GetPropertyTypeDetailed(Property);
        FString DefaultVal   = GetPropertyDefaultValue(Property, CDO);
        FString Category     = Property->GetMetaData(TEXT("Category"));
        FString Description  = GetPropertyDescription(Property);

        if (Category.IsEmpty()) Category = TEXT("-");

        AppendTableRow(OutText, { PropName, PropType, DefaultVal, Category, Description });

        // Expand nested structs
        if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
        {
            OutText += TEXT("\n");
            AppendStructFields(StructProp, OutText);
        }
    }

    OutText += TEXT("\n");
}

// ===================================================================
// Graph Extraction (stub for now)
// ===================================================================

void BPR_Extractor_Object::ExtractGraphs(FString& OutText, UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return;
    }

    AppendSectionHeader(OutText, TEXT("Graphs"));
    AppendGraphs(Blueprint, OutText);   // ← Теперь реальная логика включается
}

// ===================================================================
// Property Helpers
// ===================================================================

FString BPR_Extractor_Object::GetPropertyDefaultValue(FProperty* Property, UObject* Object) const
{
    if (!Property || !Object)
    {
        return TEXT("None");
    }

    FString ValueStr;

    // Экспортируем значение свойства через стандартный механизм Unreal
    Property->ExportText_Direct(
        ValueStr,
        Property->ContainerPtrToValuePtr<void>(Object),
        nullptr,
        Object,
        PPF_PropertyWindow
    );

    ValueStr.TrimStartAndEndInline();

    // Нормализация пустых/тривиальных значений — чтобы вывод был понятнее
    if (ValueStr.IsEmpty() ||
        ValueStr == TEXT("None") ||
        ValueStr == TEXT("0") ||
        ValueStr == TEXT("false") ||
        ValueStr == TEXT("()") ||
        ValueStr == TEXT("(0,0,0,0)") ||
        ValueStr == TEXT("(0,0)") ||
        ValueStr == TEXT("0.0") ||
        ValueStr == TEXT("0.000000"))
    {
        return TEXT("None");
    }

    return ValueStr;
}

bool BPR_Extractor_Object::IsUserVariable(FProperty* Property) const
{
    if (!Property)
    {
        return false;
    }

    // Отбрасываем системные и технические свойства
    if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_Config))
    {
        return false;
    }

    const FString Name = Property->GetName();

    // Отбрасываем хорошо известные внутренние переменные Blueprint
    if (Name.StartsWith(TEXT("UberGraphFrame")) ||
        Name.StartsWith(TEXT("PrimaryComponentTick")) ||
        Name.StartsWith(TEXT("bReplicates")) ||
        Name.StartsWith(TEXT("bGeneratedClassIsAuthoritative")) ||
        Name == TEXT("bCanEverAffectNavigation") ||
        Name == TEXT("bIsVariable") ||
        Name == TEXT("bNetAddressable"))
    {
        return false;
    }

    // Отбрасываем multicast делегаты, начинающиеся на "On" (обычно используются для Bindings)
    if (Name.StartsWith(TEXT("On")) && Property->IsA<FMulticastDelegateProperty>())
    {
        return false;
    }

    // Основное правило: переменная должна быть либо редактируемой, либо видимой в Blueprint
    return Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible);
}

void BPR_Extractor_Object::AppendStructFields(FStructProperty* StructProp, FString& OutText, int32 Indent /*= 0*/) const
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

    // Заголовок структуры
    OutText += IndentStr + FString::Printf(TEXT("### Struct: %s\n\n"), *Struct->GetName());

    // Используем общую таблицу из Base (это важно для единого стиля)
    BeginMarkdownTable(OutText, 
        { TEXT("Field"), TEXT("Type"), TEXT("Description") }, 
        Indent, 
        true);   // true = жирные заголовки

    // Перебираем все поля структуры
    for (TFieldIterator<FProperty> FieldIt(Struct); FieldIt; ++FieldIt)
    {
        FProperty* Field = *FieldIt;
        if (!Field)
        {
            continue;
        }

        FString FieldName   = CleanName(Field->GetName());
        FString FieldType   = GetPropertyTypeDetailed(Field);
        FString Description = GetPropertyDescription(Field);

        AppendTableRow(OutText, { FieldName, FieldType, Description }, Indent);
    }

    OutText += TEXT("\n");
}

void BPR_Extractor_Object::AppendReplicationInfo(const UClass* Class, FString& OutText) const
{
    if (!Class)
    {
        return;
    }

    AppendSectionHeader(OutText, TEXT("Replicated Variables"));

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

UK2Node_FunctionEntry* BPR_Extractor_Object::FindFunctionEntryNodeInGraph(const UEdGraph* Graph)
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

UK2Node_FunctionResult* BPR_Extractor_Object::FindFunctionResultNodeInGraph(const UEdGraph* Graph)
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

FString BPR_Extractor_Object::GetFunctionSignature(const UEdGraph* Graph) const
{
    if (!Graph)
    {
        return TEXT("None");
    }

    UK2Node_FunctionEntry* Entry   = FindFunctionEntryNodeInGraph(Graph);
    UK2Node_FunctionResult* Result = FindFunctionResultNodeInGraph(Graph);

    TArray<FString> Inputs;
    TArray<FString> Outputs;

    auto GetPinLabel = [](const UEdGraphPin* Pin) -> FString
    {
        if (!Pin) return TEXT("Unknown");

        const FString Display = Pin->GetDisplayName().ToString();
        return Display.IsEmpty() ? Pin->PinName.ToString() : Display;
    };

    // Собираем входные параметры из Function Entry
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

    // Собираем выходные параметры из Function Result
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

    FString InputsStr  = Inputs.Num()  > 0 ? FString::Join(Inputs, TEXT(", "))  : TEXT("void");
    FString OutputsStr = Outputs.Num() > 0 ? FString::Join(Outputs, TEXT(", ")) : TEXT("void");

    return FString::Printf(TEXT("(%s) → %s"), *InputsStr, *OutputsStr);
}

FString BPR_Extractor_Object::GetMacroSignature(const UEdGraph* Graph) const
{
    if (!Graph)
    {
        return TEXT("None");
    }

    TArray<FString> Inputs;
    TArray<FString> Outputs;

    auto GetPinLabel = [](const UEdGraphPin* Pin) -> FString
    {
        if (!Pin) return TEXT("Unknown");

        const FString Display = Pin->GetDisplayName().ToString();
        return Display.IsEmpty() ? Pin->PinName.ToString() : Display;
    };

    // Макросы в Blueprint используют UK2Node_Tunnel
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Tunnel* Tunnel = Cast<UK2Node_Tunnel>(Node);
        if (!Tunnel) continue;

        for (UEdGraphPin* Pin : Tunnel->Pins)
        {
            if (!Pin) continue;

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

    FString InputsStr  = Inputs.Num()  > 0 ? FString::Join(Inputs, TEXT(", "))  : TEXT("void");
    FString OutputsStr = Outputs.Num() > 0 ? FString::Join(Outputs, TEXT(", ")) : TEXT("void");

    return FString::Printf(TEXT("(%s) → %s"), *InputsStr, *OutputsStr);
}

bool BPR_Extractor_Object::HasExecInput(const UEdGraphNode* Node)
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

bool BPR_Extractor_Object::IsComputationalNode(const UEdGraphNode* Node)
{
    if (!Node)
    {
        return false;
    }

    // 1. Чистые прокси и математические узлы
    if (Node->IsA(UK2Node_Knot::StaticClass()) ||
        Node->IsA(UK2Node_MathExpression::StaticClass()))
    {
        return true;
    }

    // 2. Касты и преобразования
    if (Node->IsA(UK2Node_DynamicCast::StaticClass()) ||
        Node->IsA(UK2Node_CastByteToEnum::StaticClass()))
    {
        return true;
    }

    // 3. Select node
    if (Node->IsA(UK2Node_Select::StaticClass()))
    {
        return true;
    }

    // 4. Получение переменной — всегда computational (data source)
    if (Node->IsA(UK2Node_VariableGet::StaticClass()))
    {
        return true;
    }

    // 5. Установка переменной без Exec-пинов
    if (Node->IsA(UK2Node_VariableSet::StaticClass()))
    {
        if (!HasExecInput(Node))
        {
            return true;
        }
    }

    // 6. Вызов функции
    if (Node->IsA(UK2Node_CallFunction::StaticClass()))
    {
        // Проверяем, является ли функция BlueprintPure
        if (const UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
        {
            if (UFunction* Func = CallFunc->GetTargetFunction())
            {
                if (Func->HasAnyFunctionFlags(FUNC_BlueprintPure))
                {
                    return true;
                }
            }
        }

        // Если у ноды нет Exec-входа — считаем её чисто data-нодой
        if (!HasExecInput(Node))
        {
            return true;
        }
    }

    return false;
}

FString BPR_Extractor_Object::GetPinDetails(const UEdGraphPin* Pin) const
{
    if (!Pin)
    {
        return TEXT("None");
    }

    // 1. Если у пина явно задано DefaultValue — используем его
    if (!Pin->DefaultValue.IsEmpty())
    {
        return Pin->DefaultValue;
    }

    // 2. Если пин связан с другим нодом — показываем связь
    if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0] && Pin->LinkedTo[0]->GetOwningNode())
    {
        FString SourceNodeName = GetReadableNodeName(Pin->LinkedTo[0]->GetOwningNode());
        
        // Добавляем небольшую защиту от очень длинных имён
        if (SourceNodeName.Len() > 60)
        {
            SourceNodeName = SourceNodeName.Left(57) + TEXT("...");
        }

        return FString::Printf(TEXT("<linked from %s>"), *SourceNodeName);
    }

    // 3. Если ничего не задано и нет связи — считаем дефолтным
    return TEXT("default");
}

FString BPR_Extractor_Object::GetReadableNodeName(const UEdGraphNode* Node) const
{
    if (!Node)
    {
        return TEXT("None");
    }

    // 1. Custom Events — один из самых частых случаев
    if (const UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(Node))
    {
        if (CustomEvent->CustomFunctionName != NAME_None)
        {
            return CustomEvent->CustomFunctionName.ToString();
        }
    }

    // 2. Стандартные события (BeginPlay, Tick, ConstructionScript, OnClicked и т.д.)
    if (const UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
    {
        FName EventName = EventNode->EventReference.GetMemberName();
        if (EventName != NAME_None)
        {
            return EventName.ToString();
        }
    }

    // 3. Enhanced Input Events (IA_Move, IA_Look и т.п.)
    if (Node->IsA(UK2Node_InputActionEvent::StaticClass()) ||
        Node->IsA(UK2Node_InputAxisEvent::StaticClass()) ||
        Node->IsA(UK2Node_InputKeyEvent::StaticClass()))
    {
        return Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
    }

    // 4. Component Bound Events (OnComponentBeginOverlap, OnActorHit и т.д.)
    if (const UK2Node_ComponentBoundEvent* BoundEvent = Cast<UK2Node_ComponentBoundEvent>(Node))
    {
        FName EventName = BoundEvent->EventReference.GetMemberName();
        if (EventName != NAME_None)
        {
            return EventName.ToString();
        }
    }

    // 5. Call Function — самый важный и частый тип ноды
    if (const UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
    {
        FString Params;

        for (UEdGraphPin* Pin : CallFunc->Pins)
        {
            if (!Pin || Pin->Direction != EGPD_Input || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            {
                continue;
            }

            Params += FString::Printf(TEXT("%s=%s "), *Pin->PinName.ToString(), *GetPinDetails(Pin));
        }

        Params.TrimEndInline();

        FString FuncName = CallFunc->FunctionReference.GetMemberName().ToString();
        if (FuncName.IsEmpty())
        {
            FuncName = CallFunc->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
        }

        return FString::Printf(TEXT("Call %s(%s)"), *FuncName, *Params);
    }

    // 6. Variable Get / Set
    if (const UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(Node))
    {
        return FString::Printf(TEXT("Get %s"), *VarGet->VariableReference.GetMemberName().ToString());
    }

    if (const UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
    {
        return FString::Printf(TEXT("Set %s"), *VarSet->VariableReference.GetMemberName().ToString());
    }

    // 7. Control Flow nodes
    if (const UK2Node_IfThenElse* Branch = Cast<UK2Node_IfThenElse>(Node))
    {
        return FString::Printf(TEXT("Branch (Condition: %s)"), *GetPinDetails(Branch->GetConditionPin()));
    }

    if (const UK2Node_Switch* Switch = Cast<UK2Node_Switch>(Node))
    {
        return FString::Printf(TEXT("Switch (Selection: %s)"), *GetPinDetails(Switch->GetSelectionPin()));
    }

    // 8. Специальные узлы
    if (Node->IsA(UK2Node_CallDelegate::StaticClass())) return TEXT("Call Delegate");
    if (Node->IsA(UK2Node_Tunnel::StaticClass()))       return TEXT("Tunnel");
    if (Node->IsA(UK2Node_FunctionEntry::StaticClass())) return TEXT("Function Entry");
    if (Node->IsA(UK2Node_FunctionResult::StaticClass())) return TEXT("Function Result");
    if (Node->IsA(UK2Node_MacroInstance::StaticClass()))
    {
        return Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
    }

    // 9. Fallback — используем заголовок ноды Unreal
    FString Title = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
    if (Title.IsEmpty())
    {
        Title = Node->GetName();
    }

    // Добавляем комментарий пользователя, если он есть
    if (!Node->NodeComment.IsEmpty())
    {
        Title += FString::Printf(TEXT(" // %s"), *Node->NodeComment);
    }

    return Title;
}

void BPR_Extractor_Object::ProcessNodeSequence(
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
            OutExecText += FString::Printf(TEXT("%*s[Loop Detected: %s]\n"),
                IndentLevel * 2, TEXT(""), *Node->GetName());
        }
        return;
    }

    Visited.Add(Node);

    // 1. Получаем читаемое имя ноды
    FString NodeTitle = GetReadableNodeName(Node);
    if (!Node->NodeComment.IsEmpty())
    {
        NodeTitle += FString::Printf(TEXT(" // %s"), *Node->NodeComment);
    }

    // 2. Определяем, куда выводить ноду (Exec flow или Data flow)
    if (HasExecInput(Node))
    {
        // Exec-нода — идёт в основной поток выполнения
        OutExecText += FString::Printf(TEXT("%*s- %s\n"),
            IndentLevel * 2, TEXT(""), *NodeTitle);
    }
    else if (IsComputationalNode(Node))
    {
        // Pure/computational нода без Exec — выводим в Data section
        OutDataText += FString::Printf(TEXT("%*s[pure] %s\n"),
            IndentLevel * 2, TEXT(""), *NodeTitle);
    }
    else
    {
        // Остальные ноды без Exec (например, некоторые Knot's) — выводим в Exec для читаемости
        OutExecText += FString::Printf(TEXT("%*s- %s\n"),
            IndentLevel * 2, TEXT(""), *NodeTitle);
    }

    // 3. Обработка Data-flow (non-exec пины)
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
            FString TargetPinName  = GetPinDisplayName(LinkedTo);

            // Специальная метка для Knot (Reroute)
            if (LinkedTo->GetOwningNode()->IsA(UK2Node_Knot::StaticClass()))
            {
                TargetNodeName = TEXT("Reroute ") + TargetNodeName;
            }

            OutDataText += FString::Printf(TEXT("%*s[data] %s.%s → %s.%s\n"),
                (IndentLevel + 1) * 2, TEXT(""), *NodeTitle, *PinName,
                *TargetNodeName, *TargetPinName);

            // Рекурсия только для computational нод, чтобы не уходить слишком глубоко
            if (!Visited.Contains(LinkedTo->GetOwningNode()) &&
                IsComputationalNode(LinkedTo->GetOwningNode()))
            {
                ProcessNodeSequence(LinkedTo->GetOwningNode(),
                    IndentLevel + 1, Visited, OutExecText, OutDataText);
            }
        }
    }

    // 4. Рекурсия по Exec-пину (основной поток выполнения)
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

void BPR_Extractor_Object::AppendGraphSequence(
    const UEdGraph* Graph,
    FString& OutExecText,
    FString& OutDataText) const
{
    if (!Graph || Graph->Nodes.Num() == 0)
    {
        return;
    }

    TSet<UEdGraphNode*> Visited;

    // 1. Ищем стартовую точку (приоритет: Function Entry → Event → первый нод)
    UEdGraphNode* StartNode = FindFunctionEntryNodeInGraph(Graph);

    // Если это не функция, пытаемся найти любой Event нод (BeginPlay, Tick и т.д.)
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

    // Fallback — просто первый нод в графе
    if (!StartNode && !Graph->Nodes.IsEmpty())
    {
        StartNode = Graph->Nodes[0];
    }

    // 2. Основной проход по Exec-потоку
    if (StartNode)
    {
        ProcessNodeSequence(StartNode, 0, Visited, OutExecText, OutDataText);
    }

    // 3. Обработка чистых (computational/pure) нод, которые не попали в Exec-цепочку
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node || Visited.Contains(Node) || !IsComputationalNode(Node))
        {
            continue;
        }

        FString NodeTitle = GetReadableNodeName(Node);
        if (!Node->NodeComment.IsEmpty())
        {
            NodeTitle += FString::Printf(TEXT(" // %s"), *Node->NodeComment);
        }

        OutDataText += FString::Printf(TEXT("[pure] %s (no exec)\n"), *NodeTitle);
        Visited.Add(Node);

        // Обход всех data-пинов этой ноды
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
                FString TargetPinName  = GetPinDisplayName(LinkedTo);

                if (LinkedTo->GetOwningNode()->IsA(UK2Node_Knot::StaticClass()))
                {
                    TargetNodeName = TEXT("Reroute ") + TargetNodeName;
                }

                OutDataText += FString::Printf(TEXT(" [data] %s.%s → %s.%s\n"),
                    *NodeTitle, *PinName, *TargetNodeName, *TargetPinName);

                // Рекурсивный обход только для computational нод
                if (!Visited.Contains(LinkedTo->GetOwningNode()))
                {
                    ProcessNodeSequence(LinkedTo->GetOwningNode(), 
                        1, Visited, OutExecText, OutDataText);
                }
            }
        }
    }
}

void BPR_Extractor_Object::AppendGraphs(UBlueprint* Blueprint, FString& OutText) const
{
    if (!Blueprint)
    {
        return;
    }

    AppendSectionHeader(OutText, TEXT("Graphs"));

    // ================================================
    // 1. Event Graphs (UbergraphPages)
    // ================================================
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
        OutText += TEXT("No Event Graphs found.\n\n");
    }

    // ================================================
    // 2. Function Graphs
    // ================================================
    bool bHasAnyFunction = false;

    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (!Graph) continue;

        // Пропускаем ConstructionScript — он будет обработан в Actor, если нужно
        if (Graph->GetName() == TEXT("ConstructionScript"))
            continue;

        bHasAnyFunction = true;

        FString Signature = GetFunctionSignature(Graph);
        FString GraphName = CleanName(Graph->GetName());

        AppendSectionHeader(OutText, FString::Printf(TEXT("Function: %s"), *GraphName));

        if (!Signature.IsEmpty() && Signature != TEXT("None"))
        {
            OutText += FString::Printf(TEXT("- Signature: %s\n\n"), *Signature);
        }
        else
        {
            OutText += TEXT("\n");
        }

        FString ExecFlow, DataFlow;
        AppendGraphSequence(Graph, ExecFlow, DataFlow);
        OutText += ExecFlow + DataFlow + TEXT("\n\n");
    }

    // ================================================
    // 3. Macro Graphs
    // ================================================
    for (UEdGraph* Graph : Blueprint->MacroGraphs)
    {
        if (!Graph) continue;

        FString Signature = GetMacroSignature(Graph);
        FString GraphName = CleanName(Graph->GetName());

        AppendSectionHeader(OutText, FString::Printf(TEXT("Macro: %s"), *GraphName));

        if (!Signature.IsEmpty() && Signature != TEXT("None"))
        {
            OutText += FString::Printf(TEXT("- Signature: %s\n\n"), *Signature);
        }
        else
        {
            OutText += TEXT("\n");
        }

        FString ExecFlow, DataFlow;
        AppendGraphSequence(Graph, ExecFlow, DataFlow);
        OutText += ExecFlow + DataFlow + TEXT("\n\n");
    }

    if (!bHasAnyFunction && Blueprint->FunctionGraphs.Num() == 0 && Blueprint->MacroGraphs.Num() == 0)
    {
        OutText += TEXT("No custom functions or macros found.\n\n");
    }
}