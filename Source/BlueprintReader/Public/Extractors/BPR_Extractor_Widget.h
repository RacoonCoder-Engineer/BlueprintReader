// Copyright (c) 2026 Racoon Coder. All rights reserved.



#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Core.h"

// Новые инклюды для виджетов
#include "Blueprint/UserWidget.h"
#include "WidgetBlueprint.h"
#include "Components/Widget.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/WidgetAnimation.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBoxSlot.h"
#include "Styling/SlateColor.h"         
#include "Layout/Visibility.h"          
#include "Slate/WidgetTransform.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Styling/SlateColor.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/UnrealType.h"
#include "Internationalization/Text.h"
#include "Styling/SlateTypes.h"
#include "Layout/Margin.h"
#include "UObject/ObjectMacros.h"

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

    /** Рекурсивный обход иерархии виджетов с контролем глубины рекурсии */
    void ProcessWidgetHierarchy(
        UWidget* Widget,
        UPanelSlot* Slot,
        UWidgetBlueprint* WidgetBP,   // Добавили
        int32 CurrentDepth,
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
    
    /** Добавляет общие свойства, которые есть у любого UWidget */
    void AppendCommonProperties(UWidget* Widget, FString& OutText, int32 Indent);

    /** Добавляет специфические свойства в зависимости от типа виджета */
    void AppendWidgetTypeProperties(UWidget* Widget, FString& OutText, int32 Indent);
    
    // Handlers
    
    /** Обрабатывает специфические свойства UTextBlock */
    void HandleTextBlockProperties(UTextBlock* TextBlock, FString& OutText, int32 Indent);
    /** Обрабатывает специфические свойства UImage */
    void HandleImageProperties(UImage* Image, FString& OutText, int32 Indent);
    /** Обрабатывает специфические свойства UButton */
    void HandleButtonProperties(UButton* Button, FString& OutText, int32 Indent);
    /** Обрабатывает специфические свойства UBorder */
    void HandleBorderProperties(UBorder* Border, FString& OutText, int32 Indent);
    /** Обрабатывает специфические свойства UProgressBar */
    void HandleProgressBarProperties(UProgressBar* ProgressBar, FString& OutText, int32 Indent);
    /** Обработка неизвестных или кастомных виджетов (fallback) */
    void HandleUnknownWidget(UWidget* Widget, FString& OutText, int32 Indent);
    
};