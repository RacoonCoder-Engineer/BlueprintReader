// Copyright (c) 2026 Racoon Coder. All rights reserved.



#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Core.h"

struct FWidgetRecursionSettings
{
    int32 MaxDepth = 10;           
    bool bRestrictDepth = true;
};

// Forward declarations
class UWidgetBlueprint;
class UWidgetTree;
class UWidget;
class UPanelSlot;
class UUserWidget;
class UWidgetAnimation;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_FunctionEntry;
class UK2Node_FunctionResult;

/**
 * Extractor for Widget Blueprint (UMG).
 * Collects hierarchy (WidgetTree), slots, properties, bindings, animations, variables and graphs.
 * Output goes to FBPR_ExtractedData (Structure, Graph, Design).
 */
class BLUEPRINTREADER_API BPR_Extractor_Widget
{
public:
    BPR_Extractor_Widget();
    ~BPR_Extractor_Widget();

    // --------------------------------
    // Main entry point
    // --------------------------------
    /** Processes the selected Widget Blueprint, fills Structure/Graph/Design */
    void ProcessWidget(UObject* SelectedObject, FBPR_ExtractedData& OutData);
    
    /** Устанавливает настройки рекурсии для обработки виджетов */
    void SetRecursionSettings(const FWidgetRecursionSettings& InSettings);

    /** Возвращает текущие настройки рекурсии (для отладки или чтения) */
    const FWidgetRecursionSettings& GetRecursionSettings() const;

private:
    // -------------------------------
    // Logging
    // -------------------------------
    void LogMessage(const FString& Msg);
    void LogWarning(const FString& Msg);
    void LogError(const FString& Msg);

    // -------------------------------
    // Общие методы для любого Blueprint (копируем/адаптируем из Actor)
    // -------------------------------
    void AppendBlueprintInfo(UBlueprint* Blueprint, FString& OutText);
    void AppendVariables(UBlueprint* Blueprint, FString& OutText);
    void AppendGraphs(UBlueprint* Blueprint, FString& OutText);

    // Helpers для свойств
    FString GetPropertyDefaultValue(FProperty* Property, UObject* Object);
    FString GetPropertyTypeDetailed(FProperty* Property);
    void AppendStructFields(FStructProperty* StructProp, FString& OutText, int32 Indent = 0);
    bool IsUserVariable(FProperty* Property);

    // Графы и ноды 
    void AppendGraphSequence(UEdGraph* Graph, FString& OutExecText, FString& OutDataText);
    void ProcessNodeSequence(
        UEdGraphNode* Node,
        int32 IndentLevel,
        TSet<UEdGraphNode*>& Visited,
        FString& OutExecText,
        FString& OutDataText);

    UK2Node_FunctionEntry* FindFunctionEntryNodeInGraph(UEdGraph* Graph);
    UK2Node_FunctionResult* FindFunctionResultNodeInGraph(UEdGraph* Graph);
    FString GetFunctionSignature(UEdGraph* Graph);
    FString GetMacroSignature(UEdGraph* Graph);

    FString GetReadableNodeName(UEdGraphNode* Node);
    FString GetPinDetails(UEdGraphPin* Pin);
    FString GetPinDisplayName(UEdGraphPin* Pin);
    FString CleanName(const FString& RawName);
    bool IsComputationalNode(UEdGraphNode* Node);
    bool HasExecInput(UEdGraphNode* Node);

    // -------------------------------
    // Специфичные для Widget новые методы
    // -------------------------------
    /** Главный метод для обработки виджет-дерева */
    void AppendWidgetTree(UWidgetBlueprint* WidgetBP, FString& OutDesignText);

    /** Рекурсивный обход иерархии виджетов */
    void ProcessWidgetHierarchy(
        UWidget* Widget,
        UWidgetBlueprint* WidgetBP,
        UPanelSlot* Slot,
        int32 Indent,
        FString& OutText,
        TSet<UWidget*>& Visited);

    /** Форматирование одного виджета + его слота */
    FString FormatWidgetInfo(UWidget* Widget, UPanelSlot* Slot, int32 Indent);

    /** Свойства слота (Anchors, Offsets, Alignment, Padding) */
    void AppendSlotProperties(UPanelSlot* Slot, FString& OutText, int32 Indent);

    /** Ключевые визуальные свойства виджета */
    void AppendWidgetProperties(UWidget* Widget, FString& OutText, int32 Indent);

    /** Bindings (OnClicked, OnTextChanged и т.д.) */
    void AppendWidgetBindings(UWidget* Widget, FString& OutText, int32 Indent);

    /** Анимации виджета */
    void AppendAnimations(UWidgetBlueprint* WidgetBP, FString& OutText);

    /** Получение читаемого имени класса виджета */
    FString GetWidgetTypeName(UWidget* Widget);
    
    /** Event Bindings с указанием, к каким функциям они привязаны */
    void AppendWidgetEventBindings(UWidget* Widget, UWidgetBlueprint* WidgetBP, FString& OutText, int32 Indent);
    
    FWidgetRecursionSettings RecursionSettings;
    
};