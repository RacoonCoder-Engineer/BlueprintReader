// Copyright (c) 2026 Racoon Coder. All rights reserved.



#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Core.h"
#include "WidgetBlueprint.h"
#include "Components/Widget.h"
#include "Styling/SlateColor.h"         
#include "Layout/Visibility.h"          
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
#include "Components/RichTextBlock.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/SpinBox.h"
#include "Components/ComboBoxString.h"
#include "Components/CanvasPanel.h"
#include "Components/Overlay.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/GridPanel.h"
#include "Components/ListView.h"
#include "Components/TileView.h"
#include "Components/TreeView.h"
#include "Components/Throbber.h"
#include "Components/CircularThrobber.h"
#include "Components/ExpandableArea.h"
#include "Components/BackgroundBlur.h"
#include "Components/UniformGridPanel.h"
#include "Containers/ScriptArray.h"
#include "Styling/SlateTypes.h"      
#include "UObject/UnrealType.h"  
#include "Styling/SlateTypes.h"


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
    
    //==================================================================
    // Widget Property Handlers
    //==================================================================


    /** ====================== Common Widgets ====================== */
    void HandleTextBlockProperties(UTextBlock* TextBlock, FString& OutText, int32 Indent);
    void HandleRichTextBlockProperties(URichTextBlock* RichTextBlock, FString& OutText, int32 Indent);
    void HandleImageProperties(UImage* Image, FString& OutText, int32 Indent);
    void HandleButtonProperties(UButton* Button, FString& OutText, int32 Indent);
    void HandleBorderProperties(UBorder* Border, FString& OutText, int32 Indent);
    void HandleProgressBarProperties(UProgressBar* ProgressBar, FString& OutText, int32 Indent);
    void HandleSliderProperties(USlider* Slider, FString& OutText, int32 Indent);
    void HandleCheckBoxProperties(UCheckBox* CheckBox, FString& OutText, int32 Indent);

    /** ====================== Input Widgets ====================== */
    void HandleEditableTextProperties(UEditableText* EditableText, FString& OutText, int32 Indent);
    void HandleEditableTextBoxProperties(UEditableTextBox* EditableTextBox, FString& OutText, int32 Indent);
    void HandleSpinBoxProperties(USpinBox* SpinBox, FString& OutText, int32 Indent);
    void HandleComboBoxStringProperties(UComboBoxString* ComboBoxString, FString& OutText, int32 Indent);

    /** ====================== Panel & Layout Widgets ====================== */
    void HandleCanvasPanelProperties(UCanvasPanel* CanvasPanel, FString& OutText, int32 Indent);
    void HandleOverlayProperties(UOverlay* Overlay, FString& OutText, int32 Indent);
    void HandleHorizontalBoxProperties(UHorizontalBox* HorizontalBox, FString& OutText, int32 Indent);
    void HandleVerticalBoxProperties(UVerticalBox* VerticalBox, FString& OutText, int32 Indent);
    void HandleScrollBoxProperties(UScrollBox* ScrollBox, FString& OutText, int32 Indent);
    void HandleSizeBoxProperties(USizeBox* SizeBox, FString& OutText, int32 Indent);
    void HandleGridPanelProperties(UGridPanel* GridPanel, FString& OutText, int32 Indent);
    void HandleUniformGridPanelProperties(UUniformGridPanel* UniformGridPanel, FString& OutText, int32 Indent);

    /** ====================== List & View Widgets ====================== */
    void HandleListViewProperties(UListView* ListView, FString& OutText, int32 Indent);
    void HandleTileViewProperties(UTileView* TileView, FString& OutText, int32 Indent);
    void HandleTreeViewProperties(UTreeView* TreeView, FString& OutText, int32 Indent);

    /** ====================== Special & Misc ====================== */
    void HandleThrobberProperties(UThrobber* Throbber, FString& OutText, int32 Indent);
    void HandleCircularThrobberProperties(UCircularThrobber* CircularThrobber, FString& OutText, int32 Indent);
    void HandleExpandableAreaProperties(UExpandableArea* ExpandableArea, FString& OutText, int32 Indent);
    void HandleBackgroundBlurProperties(UBackgroundBlur* BackgroundBlur, FString& OutText, int32 Indent);

    /** ====================== Fallback ====================== */
    /** Обработка неизвестных или кастомных виджетов (fallback) */
    void HandleUnknownWidget(UWidget* Widget, FString& OutText, int32 Indent);
    
};