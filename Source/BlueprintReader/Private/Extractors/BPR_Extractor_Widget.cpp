// Copyright (c) 2026 Racoon Coder. All rights reserved.

// TODO: Улучшения и доработки экстрактора виджетов
// =====================================================================
// 1. Bindings — расширить вывод событий (OnClicked, OnTextChanged и т.д.)
//    Сейчас выводятся только имена делегатов (OnTextChangedDispatcher),
//    но не показывается, к какой функции/ивенту они привязаны.
//    Решение: в графе Event Graph искать K2Node_ComponentBoundEvent,
//    где ComponentName == Widget->GetName() и DelegatePropertyName == имя делегата.
//    Добавить вывод: "OnTextChanged → CustomEvent_MyTextChanged"
//    Приоритет: высокий (самая частая причина "почему не работает")

// 2. Текст в TextBlock'ах / лейблах
//    Сейчас для WBP_TextLabel_C и подобных выводится только "Custom Widget",
//    текст не извлекается.
//    Причина: AppendWidgetProperties обрабатывает только нативные UTextBlock.
//    Решение: 
//      - Если WBP_* наследует UUserWidget → получить WidgetTree → найти UTextBlock внутри
//      - Или добавить поддержку типичных кастомных виджетов через Cast к UUserWidget + GetWidgetFromName
//    Приоритет: средний (важно для понимания содержимого UI)

// 3. Свойства кастомных виджетов (WBP_*)
//    Сейчас: "Custom Widget: WBP_EditableUnderlinedField_C"
//    Решение: 
//      - Если виджет — UUserWidget, рекурсивно вызвать AppendWidgetProperties на его RootWidget
//      - Или добавить специальную обработку часто используемых WBP_* (через if (Widget->GetClass()->GetName().StartsWith("WBP_")))
//    Приоритет: средний

// 4. Анимации
//    Уже обрабатывается в AppendAnimations — если их нет, выводит "None"
//    Доработка: добавить длительность, имена треков, события (если нужно)
//    Приоритет: низкий

// 5. Общие улучшения
//    - Добавить обработку Visibility Binding (если Visibility привязан к переменной)
//    - Добавить Render Transform (Scale, Rotation, Translation), если не Identity
//    - Опционально: выводить ZOrder, IsFocusable, Navigation
//    Приоритет: низкий-средний

// =====================================================================

#include "Extractors/BPR_Extractor_Widget.h"

// Общие инклюды из Actor-экстрактора
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
#include "K2Node_Knot.h"
#include "K2Node_MathExpression.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_CastByteToEnum.h"
#include "K2Node_Select.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_Variable.h"
#include "K2Node_ComponentBoundEvent.h"

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

BPR_Extractor_Widget::BPR_Extractor_Widget() {}
BPR_Extractor_Widget::~BPR_Extractor_Widget() {}

//==============================================================================
// Logging (переносим как есть)
//==============================================================================
void BPR_Extractor_Widget::LogMessage(const FString& Msg) 
{ 
    UE_LOG(LogTemp, Log, TEXT("[BPR_Extractor_Widget] %s"), *Msg); 
}

void BPR_Extractor_Widget::LogWarning(const FString& Msg) 
{ 
    UE_LOG(LogTemp, Warning, TEXT("[BPR_Extractor_Widget] %s"), *Msg); 
}

void BPR_Extractor_Widget::LogError(const FString& Msg) 
{ 
    UE_LOG(LogTemp, Error, TEXT("[BPR_Extractor_Widget] %s"), *Msg); 
}

//==============================================================================
// Main entry point — адаптируем под виджет
//==============================================================================
void BPR_Extractor_Widget::ProcessWidget(UObject* SelectedObject, FBPR_ExtractedData& OutData)
{
    if (!SelectedObject)
    {
        LogError(TEXT("SelectedObject is null!"));
        OutData.Structure = FText::FromString("Error: SelectedObject is null.");
        OutData.Graph     = FText::FromString("Error: SelectedObject is null.");
        OutData.Design    = FText::FromString("Error: SelectedObject is null.");
        return;
    }

    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(SelectedObject);
    if (!WidgetBP)
    {
        LogWarning(TEXT("Selected object is not a Widget Blueprint."));
        OutData.Structure = FText::FromString("Warning: Not a Widget Blueprint.");
        OutData.Graph     = FText::FromString("Warning: Not a Widget Blueprint.");
        OutData.Design    = FText::FromString("Warning: Not a Widget Blueprint.");
        return;
    }

    FString TmpStructure = FString::Printf(TEXT("# Structure Export for Widget: %s\n\n"), *WidgetBP->GetName());
    FString TmpGraph     = FString::Printf(TEXT("# Graphs Export for Widget: %s\n\n"), *WidgetBP->GetName());
    FString TmpDesign    = FString::Printf(TEXT("# Design (Widget Tree) for %s\n\n"), *WidgetBP->GetName());

    // Общие части — переносим из Actor
    AppendBlueprintInfo(WidgetBP, TmpStructure);
    AppendVariables(WidgetBP, TmpStructure);
    AppendGraphs(WidgetBP, TmpGraph);

    // Новое для виджетов
    AppendWidgetTree(WidgetBP, TmpDesign);
    AppendAnimations(WidgetBP, TmpDesign);

    OutData.Structure = FText::FromString(TmpStructure);
    OutData.Graph     = FText::FromString(TmpGraph);
    OutData.Design    = FText::FromString(TmpDesign);

    LogMessage(FString::Printf(TEXT("Widget extraction finished for %s"), *WidgetBP->GetName()));
}

//==============================================================================
// Общие методы (копируем из Actor-экстрактора без изменений)
//==============================================================================
void BPR_Extractor_Widget::AppendBlueprintInfo(UBlueprint* Blueprint, FString& OutText)
{
    if (!Blueprint || !Blueprint->ParentClass) return;

    FString ParentClassName = Blueprint->ParentClass->GetName();
    OutText += FString::Printf(TEXT("## Blueprint Info: %s\n"), *Blueprint->GetName());
    OutText += FString::Printf(TEXT("Parent Class: %s (UUserWidget)\n\n"), *ParentClassName);
}

void BPR_Extractor_Widget::AppendVariables(UBlueprint* Blueprint, FString& OutText)
{
    // Копируем как есть из Actor — работает одинаково
    if (!Blueprint || !Blueprint->GeneratedClass) return;

    OutText += TEXT("## Custom Variables\n");
    OutText += TEXT("| Name | Type | Default Value | Flags | Category | Description |\n");
    OutText += TEXT("|------|------|---------------|-------|----------|-------------|\n");

    UClass* Class = Blueprint->GeneratedClass;
    if (!Class) return;
    UObject* CDO = Class->GetDefaultObject();
    if (!CDO) return;

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
        if (Property->HasAnyPropertyFlags(CPF_BlueprintAssignable)) Flags += TEXT("Assignable ");
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

// Остальные общие методы (GetPropertyDefaultValue, GetPropertyTypeDetailed, AppendStructFields, IsUserVariable,
// AppendGraphs, AppendGraphSequence, ProcessNodeSequence, Find*NodeInGraph, Get*Signature, GetReadableNodeName,
// GetPinDetails, GetPinDisplayName, CleanName, IsComputationalNode, HasExecInput) — копируй из Actor-экстрактора
// Они работают идентично, потому что используют UBlueprint и EdGraph.

//==============================================================================
// Специфичные для виджета — новые методы (пока пустые каркасы)
//==============================================================================
void BPR_Extractor_Widget::AppendWidgetTree(UWidgetBlueprint* WidgetBP, FString& OutDesignText)
{
    if (!WidgetBP || !WidgetBP->WidgetTree) 
    {
        OutDesignText += TEXT("No Widget Tree found.\n");
        return;
    }

    OutDesignText += TEXT("## Widget Hierarchy\n");
    OutDesignText += TEXT("Root: ") + WidgetBP->WidgetTree->RootWidget->GetName() + TEXT("\n");

    // Рекурсивный обход — пока заглушка
    TSet<UWidget*> VisitedWidgets;
    ProcessWidgetHierarchy(WidgetBP->WidgetTree->RootWidget, WidgetBP, nullptr,  0, OutDesignText, VisitedWidgets);
}

void BPR_Extractor_Widget::ProcessWidgetHierarchy(
    UWidget* Widget,
    UWidgetBlueprint* WidgetBP,
    UPanelSlot* Slot,
    int32 Indent,
    FString& OutText,
    TSet<UWidget*>& Visited)
{
    if (!Widget || Visited.Contains(Widget)) return;

    Visited.Add(Widget);

    FString IndentStr = FString::ChrN(Indent * 2, ' ');
    OutText += IndentStr + TEXT("- ") + GetWidgetTypeName(Widget) + TEXT(" (") + Widget->GetName() + TEXT(")\n");

    AppendSlotProperties(Slot, OutText, Indent + 1);
    AppendWidgetProperties(Widget, OutText, Indent + 1);
    
    // ← Вот сюда добавляем новый метод!
    AppendWidgetEventBindings(Widget, WidgetBP, OutText, Indent + 1);   // ← Добавь эту строку

    // Рекурсия по детям    
    if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
    {
        for (UWidget* Child : Panel->GetAllChildren())
        {
            if (!Child) continue;

            UPanelSlot* ChildSlot = Child->Slot;
            ProcessWidgetHierarchy(Child, WidgetBP, ChildSlot,  Indent + 1, OutText, Visited);
        }
    }
}

void BPR_Extractor_Widget::AppendSlotProperties(UPanelSlot* Slot, FString& OutText, int32 Indent)
{
    if (!Slot)
    {
        FString IndentStr = FString::ChrN(Indent * 2, ' ');
        OutText += IndentStr + TEXT("- Slot: None (root widget or direct child)\n\n");
        return;
    }

    FString IndentStr = FString::ChrN(Indent * 2, ' ');
    OutText += IndentStr + TEXT("Slot Properties:\n");

    // ==================== CanvasPanelSlot (самый частый) ====================
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
    {
        FAnchors Anchors = CanvasSlot->GetAnchors();
        FMargin Offsets = CanvasSlot->GetOffsets();

        OutText += IndentStr + FString::Printf(TEXT("  - Type: CanvasPanelSlot\n"));
        OutText += IndentStr + FString::Printf(TEXT("  - Anchors: Min(%.2f,%.2f) → Max(%.2f,%.2f)\n"),
            Anchors.Minimum.X, Anchors.Minimum.Y, Anchors.Maximum.X, Anchors.Maximum.Y);
        OutText += IndentStr + FString::Printf(TEXT("  - Offsets: L:%.0f T:%.0f R:%.0f B:%.0f\n"),
            Offsets.Left, Offsets.Top, Offsets.Right, Offsets.Bottom);
        OutText += IndentStr + FString::Printf(TEXT("  - AutoSize: %s | ZOrder: %d\n"),
            CanvasSlot->GetAutoSize() ? TEXT("True") : TEXT("False"), CanvasSlot->GetZOrder());
    }
    // ==================== HorizontalBoxSlot ====================
    else if (UHorizontalBoxSlot* HBoxSlot = Cast<UHorizontalBoxSlot>(Slot))
    {
        OutText += IndentStr + TEXT("  - Type: HorizontalBoxSlot\n");
        OutText += IndentStr + FString::Printf(TEXT("  - Horizontal Alignment: %s\n"),
            *UEnum::GetValueAsString(HBoxSlot->GetHorizontalAlignment()));
        OutText += IndentStr + FString::Printf(TEXT("  - Vertical Alignment: %s\n"),
            *UEnum::GetValueAsString(HBoxSlot->GetVerticalAlignment()));
        OutText += IndentStr + FString::Printf(TEXT("  - Size Rule: %s (Value: %.2f)\n"),
            *UEnum::GetValueAsString(HBoxSlot->GetSize().SizeRule), HBoxSlot->GetSize().Value);
    }
    // ==================== VerticalBoxSlot ====================
    else if (UVerticalBoxSlot* VBoxSlot = Cast<UVerticalBoxSlot>(Slot))
    {
        OutText += IndentStr + TEXT("  - Type: VerticalBoxSlot\n");
        OutText += IndentStr + FString::Printf(TEXT("  - Horizontal Alignment: %s\n"),
            *UEnum::GetValueAsString(VBoxSlot->GetHorizontalAlignment()));
        OutText += IndentStr + FString::Printf(TEXT("  - Vertical Alignment: %s\n"),
            *UEnum::GetValueAsString(VBoxSlot->GetVerticalAlignment()));
        OutText += IndentStr + FString::Printf(TEXT("  - Size Rule: %s (Value: %.2f)\n"),
            *UEnum::GetValueAsString(VBoxSlot->GetSize().SizeRule), VBoxSlot->GetSize().Value);
    }
    // ==================== OverlaySlot ====================
    else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Slot))
    {
        OutText += IndentStr + TEXT("  - Type: OverlaySlot\n");
        OutText += IndentStr + FString::Printf(TEXT("  - Padding: L:%.0f T:%.0f R:%.0f B:%.0f\n"),
            OverlaySlot->GetPadding().Left, OverlaySlot->GetPadding().Top,
            OverlaySlot->GetPadding().Right, OverlaySlot->GetPadding().Bottom);
    }
    // ==================== SizeBoxSlot ====================
    else if (USizeBoxSlot* SizeBoxSlot = Cast<USizeBoxSlot>(Slot))
    {
        OutText += IndentStr + TEXT("  - Type: SizeBoxSlot\n");
        OutText += IndentStr + FString::Printf(TEXT("  - Horizontal Alignment: %s\n"),
            *UEnum::GetValueAsString(SizeBoxSlot->GetHorizontalAlignment()));
        OutText += IndentStr + FString::Printf(TEXT("  - Vertical Alignment: %s\n"),
            *UEnum::GetValueAsString(SizeBoxSlot->GetVerticalAlignment()));
    }
    else
    {
        OutText += IndentStr + FString::Printf(TEXT("  - Type: %s (custom slot)\n"), 
            *Slot->GetClass()->GetName());
    }

    OutText += TEXT("\n");
}

void BPR_Extractor_Widget::AppendWidgetProperties(UWidget* Widget, FString& OutText, int32 Indent)
{
    if (!Widget)
    {
        FString IndentStr = FString::ChrN(Indent * 2, ' ');
        OutText += IndentStr + TEXT("- Properties: None (null widget)\n");
        LogWarning(TEXT("AppendWidgetProperties: Widget is null"));
        return;
    }

    FString IndentStr = FString::ChrN(Indent * 2, ' ');
    OutText += IndentStr + TEXT("Properties:\n");

    // Общие свойства (всегда выводим)
    OutText += IndentStr + FString::Printf(TEXT("  - Visibility: %s\n"),
        *UEnum::GetValueAsString(Widget->GetVisibility()));

    OutText += IndentStr + FString::Printf(TEXT("  - Is Enabled: %s\n"),
        Widget->GetIsEnabled() ? TEXT("True") : TEXT("False"));

    float Opacity = Widget->GetRenderOpacity();
    OutText += IndentStr + FString::Printf(TEXT("  - Render Opacity: %.2f\n"), Opacity);

    FWidgetTransform Transform = Widget->GetRenderTransform();
    if (!Transform.IsIdentity())
    {
        OutText += IndentStr + TEXT("  - Render Transform:\n");
        OutText += IndentStr + FString::Printf(TEXT("    - Scale: %.2f x %.2f\n"),
            Transform.Scale.X, Transform.Scale.Y);
        OutText += IndentStr + FString::Printf(TEXT("    - Rotation: %.1f deg\n"),
            Transform.Angle);
        OutText += IndentStr + FString::Printf(TEXT("    - Translation: X:%.0f Y:%.0f\n"),
            Transform.Translation.X, Transform.Translation.Y);
    }

    // Специфичные свойства — только через Cast
    if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
    {
        OutText += IndentStr + TEXT("  - TextBlock Properties:\n");
        OutText += IndentStr + FString::Printf(TEXT("    - Text: \"%s\"\n"),
            *TextBlock->GetText().ToString());
        FSlateFontInfo Font = TextBlock->GetFont();
        OutText += IndentStr + FString::Printf(TEXT("    - Font Size: %d\n"), (int32)Font.Size);

        // Justification через reflection (без спама логов, только если не нашли)
        static const FName JustificationPropName(TEXT("Justification"));
        if (FProperty* Prop = TextBlock->GetClass()->FindPropertyByName(JustificationPropName))
        {
            if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
            {
                uint8 RawValue = 0;
                ByteProp->GetValue_InContainer(TextBlock, &RawValue);
                ETextJustify::Type Justification = static_cast<ETextJustify::Type>(RawValue);

                // Выводим всегда, чтобы видеть явно (можно оставить только != Left, как было)
                OutText += IndentStr + FString::Printf(TEXT("    - Justification: %s\n"),
                    *UEnum::GetValueAsString(Justification));
            }
            else
            {
                LogWarning(FString::Printf(TEXT("Justification property in TextBlock is not ByteProperty! Type: %s"),
                    *Prop->GetClass()->GetName()));
            }
        }
        else
        {
            LogWarning(TEXT("Justification property not found in UTextBlock"));
        }

        FSlateColor TextColor = TextBlock->GetColorAndOpacity();
        OutText += IndentStr + FString::Printf(TEXT("    - Text Color: R:%.2f G:%.2f B:%.2f A:%.2f\n"),
            TextColor.GetSpecifiedColor().R, TextColor.GetSpecifiedColor().G,
            TextColor.GetSpecifiedColor().B, TextColor.GetSpecifiedColor().A);
    }
    else if (UImage* Image = Cast<UImage>(Widget))
    {
        OutText += IndentStr + TEXT("  - Image Properties:\n");
        FSlateBrush Brush = Image->GetBrush();
        if (Brush.GetResourceObject())
        {
            OutText += IndentStr + FString::Printf(TEXT("    - Brush: %s (Size: %.0fx%.0f)\n"),
                *Brush.GetResourceObject()->GetName(),
                Brush.ImageSize.X, Brush.ImageSize.Y);
        }
        FSlateColor BrushColor = Brush.TintColor;
        OutText += IndentStr + FString::Printf(TEXT("    - Brush Color: R:%.2f G:%.2f B:%.2f A:%.2f\n"),
            BrushColor.GetSpecifiedColor().R,
            BrushColor.GetSpecifiedColor().G,
            BrushColor.GetSpecifiedColor().B,
            BrushColor.GetSpecifiedColor().A);
    }
    else if (UButton* Button = Cast<UButton>(Widget))
    {
        OutText += IndentStr + TEXT("  - Button Properties:\n");
        FSlateColor BtnColor = Button->GetColorAndOpacity();
        OutText += IndentStr + FString::Printf(TEXT("    - Color & Opacity: R:%.2f G:%.2f B:%.2f A:%.2f\n"),
            BtnColor.GetSpecifiedColor().R, BtnColor.GetSpecifiedColor().G,
            BtnColor.GetSpecifiedColor().B, BtnColor.GetSpecifiedColor().A);
    }
    else if (UBorder* Border = Cast<UBorder>(Widget))
    {
        OutText += IndentStr + TEXT("  - Border Properties:\n");
        FLinearColor BrushColor = Border->GetBrushColor();
        OutText += IndentStr + FString::Printf(TEXT("    - Brush Color: R:%.2f G:%.2f B:%.2f A:%.2f\n"),
            BrushColor.R, BrushColor.G, BrushColor.B, BrushColor.A);
    }
    else if (UProgressBar* Progress = Cast<UProgressBar>(Widget))
    {
        OutText += IndentStr + TEXT("  - ProgressBar Properties:\n");
        OutText += IndentStr + FString::Printf(TEXT("    - Percent: %.2f\n"), Progress->GetPercent());
        FSlateColor FillColor = Progress->GetFillColorAndOpacity();
        OutText += IndentStr + FString::Printf(TEXT("    - Fill Color: R:%.2f G:%.2f B:%.2f A:%.2f\n"),
            FillColor.GetSpecifiedColor().R,
            FillColor.GetSpecifiedColor().G,
            FillColor.GetSpecifiedColor().B,
            FillColor.GetSpecifiedColor().A);
    }
    else
    {
        // Только здесь лог — если тип неизвестен
        LogWarning(FString::Printf(TEXT("Unknown widget type for properties: %s"),
            *Widget->GetClass()->GetName()));
        OutText += IndentStr + FString::Printf(TEXT("  - Custom Widget: %s\n"),
            *Widget->GetClass()->GetName());
    }

    OutText += TEXT("\n");
}

void BPR_Extractor_Widget::AppendWidgetBindings(UWidget* Widget, FString& OutText, int32 Indent)
{
    if (!Widget) return;

    FString IndentStr = FString::ChrN(Indent * 2, ' ');
    OutText += IndentStr + TEXT("Event Bindings (OnXXX):\n");

    bool bHasAny = false;

    for (TFieldIterator<FProperty> It(Widget->GetClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        FProperty* Prop = *It;
        if (FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(Prop))
        {
            FName Name = Prop->GetFName();
            FString NameStr = Name.ToString();

            // Фильтр: только типичные события виджетов
            if (NameStr.StartsWith(TEXT("On")))
            {
                bHasAny = true;
                OutText += IndentStr + FString::Printf(TEXT("  - %s (dynamic multicast delegate)\n"), *CleanName(NameStr));
            }
        }
    }

    if (!bHasAny)
    {
        OutText += IndentStr + TEXT("  - No event delegates found on this widget type\n");
    }

    OutText += TEXT("\n");
}

void BPR_Extractor_Widget::AppendAnimations(UWidgetBlueprint* WidgetBP, FString& OutText)
{
    if (!WidgetBP) return;

    OutText += TEXT("## Animations\n");

    if (WidgetBP->Animations.Num() == 0)
    {
        OutText += TEXT("- None\n\n");
        return;
    }

    for (UWidgetAnimation* Anim : WidgetBP->Animations)
    {
        if (!Anim) continue;

        float Duration = Anim->GetEndTime() - Anim->GetStartTime();

        OutText += FString::Printf(TEXT("- %s (Duration: %.2f sec)\n"), 
            *Anim->GetName(), Duration);
    }

    OutText += TEXT("\n");
}

FString BPR_Extractor_Widget::GetWidgetTypeName(UWidget* Widget)
{
    if (!Widget) return TEXT("Unknown");

    FString ClassName = Widget->GetClass()->GetName();
    // Убираем "Widget" или "UserWidget" из конца, если есть
    if (ClassName.EndsWith(TEXT("Widget"))) ClassName = ClassName.LeftChop(6);
    return ClassName;
}

// В конец BPR_Extractor_Widget.cpp

void BPR_Extractor_Widget::AppendGraphs(UBlueprint* Blueprint, FString& OutText)
{
    if (!Blueprint) return;

    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint);
    if (!WidgetBP)
    {
        OutText += TEXT("## Graphs\nError: Not a Widget Blueprint\n\n");
        return;
    }

    OutText += FString::Printf(TEXT("## Graphs for Widget Blueprint: %s\n\n"), *Blueprint->GetName());

    // Event Graph (обычно Event Construct + другие события)
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (!Graph) continue;

        FString ExecFlow, DataFlow;
        OutText += FString::Printf(TEXT("### Event Graph: %s\n"), *Graph->GetName());

        AppendGraphSequence(Graph, ExecFlow, DataFlow);
        OutText += ExecFlow + TEXT("\n") + DataFlow + TEXT("\n\n");
    }

    // Обычные функции (включая Event Tick, если есть)
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (!Graph) continue;

        FString Signature = GetFunctionSignature(Graph);
        FString ExecFlow, DataFlow;

        OutText += FString::Printf(TEXT("### Function: %s\n- Signature: %s\n"), *Graph->GetName(), *Signature);

        AppendGraphSequence(Graph, ExecFlow, DataFlow);
        OutText += ExecFlow + TEXT("\n") + DataFlow + TEXT("\n\n");
    }

    // Макросы (если используются)
    for (UEdGraph* Graph : Blueprint->MacroGraphs)
    {
        if (!Graph) continue;

        FString Signature = GetMacroSignature(Graph);
        FString ExecFlow, DataFlow;

        OutText += FString::Printf(TEXT("### Macro: %s\n- Signature: %s\n"), *Graph->GetName(), *Signature);

        AppendGraphSequence(Graph, ExecFlow, DataFlow);
        OutText += ExecFlow + TEXT("\n") + DataFlow + TEXT("\n\n");
    }

    // Опционально: предупреждение, если UbergraphPages пустое (часто в виджетах)
    if (Blueprint->UbergraphPages.Num() == 0)
    {
        OutText += TEXT("Note: No Event Graph found (typical for simple widgets)\n\n");
    }
}

FString BPR_Extractor_Widget::GetPropertyDefaultValue(FProperty* Property, UObject* Object)
{
    if (!Property || !Object)
    {
        return TEXT("None");
    }

    // Специальная обработка часто встречающихся структур в UMG
    if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        FName StructName = StructProp->Struct->GetFName();

        // FSlateColor (ColorAndOpacity, BrushColor и т.д.)
        if (StructName == FName("SlateColor"))
        {
            FSlateColor Color;
            StructProp->CopyCompleteValue_InContainer(&Color, Object);
            FLinearColor Linear = Color.GetSpecifiedColor();
            return FString::Printf(TEXT("R:%.2f G:%.2f B:%.2f A:%.2f"), 
                Linear.R, Linear.G, Linear.B, Linear.A);
        }

        // FLinearColor (ShadowColorAndOpacity, BrushColor в некоторых случаях)
        if (StructName == FName("LinearColor"))
        {
            FLinearColor Color;
            StructProp->CopyCompleteValue_InContainer(&Color, Object);
            return FString::Printf(TEXT("R:%.2f G:%.2f B:%.2f A:%.2f"), 
                Color.R, Color.G, Color.B, Color.A);
        }

        // FMargin (Padding, Offsets)
        if (StructName == FName("Margin"))
        {
            FMargin Margin;
            StructProp->CopyCompleteValue_InContainer(&Margin, Object);
            return FString::Printf(TEXT("L:%.0f T:%.0f R:%.0f B:%.0f"), 
                Margin.Left, Margin.Top, Margin.Right, Margin.Bottom);
        }

        // FVector2D (ShadowOffset, Translation)
        if (StructName == FName("Vector2D"))
        {
            FVector2D Vec;
            StructProp->CopyCompleteValue_InContainer(&Vec, Object);
            return FString::Printf(TEXT("X:%.1f Y:%.1f"), Vec.X, Vec.Y);
        }
    }

    // Общий случай через ExportText
    FString ValueStr;
    Property->ExportText_Direct(
        ValueStr,
        Property->ContainerPtrToValuePtr<void>(Object),
        nullptr,
        Object,
        PPF_PropertyWindow | 0x20 | 0x20000   // PPF_Localized (1<<5) + PPF_ConsoleVariables (1<<17)
    );

    ValueStr.TrimStartInline();
    ValueStr.TrimEndInline();

    if (ValueStr.IsEmpty() || 
        ValueStr == TEXT("None") || 
        ValueStr == TEXT("0") || 
        ValueStr == TEXT("false") || 
        ValueStr == TEXT("()"))
    {
        return TEXT("None");
    }

    return ValueStr;
}

FString BPR_Extractor_Widget::GetPropertyTypeDetailed(FProperty* Property)
{
    if (!Property) return TEXT("Unknown");

    FString Type = Property->GetCPPType();

    // Контейнеры — TArray, TMap, TSet
    if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        FString ElemType = ArrayProp->Inner ? ArrayProp->Inner->GetCPPType() : TEXT("Unknown");
        Type = FString::Printf(TEXT("TArray<%s>"), *ElemType);
    }
    else if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
    {
        FString KeyType   = MapProp->KeyProp   ? MapProp->KeyProp->GetCPPType()   : TEXT("Unknown");
        FString ValueType = MapProp->ValueProp ? MapProp->ValueProp->GetCPPType() : TEXT("Unknown");
        Type = FString::Printf(TEXT("TMap<%s, %s>"), *KeyType, *ValueType);
    }
    else if (FSetProperty* SetProp = CastField<FSetProperty>(Property))
    {
        FString ElemType = SetProp->ElementProp ? SetProp->ElementProp->GetCPPType() : TEXT("Unknown");
        Type = FString::Printf(TEXT("TSet<%s>"), *ElemType);
    }

    // Enum — показываем имя enum-класса
    else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        UEnum* Enum = EnumProp->GetEnum();
        if (Enum)
        {
            Type = Enum->GetName();
            // Можно добавить ::Type для ясности, но в UMG это редко нужно
        }
    }

    // Специфичные для UMG типы — делаем более читаемыми
    else if (Property->GetCPPType() == TEXT("FSlateColor"))
    {
        Type = TEXT("FSlateColor");
    }
    else if (Property->GetCPPType() == TEXT("FLinearColor"))
    {
        Type = TEXT("FLinearColor");
    }
    else if (Property->GetCPPType() == TEXT("FMargin"))
    {
        Type = TEXT("FMargin (Padding)");
    }
    else if (Property->GetCPPType() == TEXT("FAnchors"))
    {
        Type = TEXT("FAnchors");
    }
    else if (Property->GetCPPType() == TEXT("FWidgetTransform"))
    {
        Type = TEXT("FWidgetTransform");
    }
    else if (Property->GetCPPType() == TEXT("ETextJustify::Type"))
    {
        Type = TEXT("ETextJustify");
    }

    return Type;
}

void BPR_Extractor_Widget::AppendStructFields(FStructProperty* StructProp, FString& OutText, int32 Indent)
{
    if (!StructProp) return;

    UScriptStruct* Struct = StructProp->Struct;
    if (!Struct) return;

    FString IndentStr = FString::ChrN(Indent * 2, ' ');

    OutText += IndentStr + FString::Printf(TEXT("### Struct: %s\n"), *Struct->GetName());
    OutText += IndentStr + TEXT("| Field | Type | Description |\n");
    OutText += IndentStr + TEXT("|-------|------|-------------|\n");

    for (TFieldIterator<FProperty> FieldIt(Struct); FieldIt; ++FieldIt)
    {
        FProperty* Field = *FieldIt;
        if (!Field) continue;

        FString FieldName = CleanName(Field->GetName());
        FString FieldType = GetPropertyTypeDetailed(Field);
        FString Desc = Field->GetToolTipText().ToString();

        // Убираем лишние пробелы и переносы в описании
        Desc.ReplaceInline(TEXT("\r\n"), TEXT(" "));
        Desc.ReplaceInline(TEXT("\n"), TEXT(" "));
        Desc.TrimStartInline();
        Desc.TrimEndInline();

        OutText += IndentStr + FString::Printf(
            TEXT("| %s | %s | %s |\n"),
            *FieldName,
            *FieldType,
            Desc.IsEmpty() ? TEXT("-") : *Desc
        );
    }

    OutText += IndentStr + TEXT("\n");
}

bool BPR_Extractor_Widget::IsUserVariable(FProperty* Property)
{
    if (!Property) return false;

    // Отсекаем transient, deprecated, config и т.п.
    if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_Config))
    {
        return false;
    }

    FString Name = Property->GetName();

    // Общие для всех блюпринтов (UberGraphFrame, Tick)
    if (Name.StartsWith(TEXT("UberGraphFrame")) ||
        Name.StartsWith(TEXT("PrimaryComponentTick")) ||
        Name.StartsWith(TEXT("bReplicates")) ||
        Name.StartsWith(TEXT("bGeneratedClassIsAuthoritative")))
    {
        return false;
    }

    // Специфично для Widget Blueprint
    if (Name.StartsWith(TEXT("WidgetTree")) ||
        Name.StartsWith(TEXT("Animations")) ||
        Name.StartsWith(TEXT("NamedSlotBindings")) ||
        Name.StartsWith(TEXT("DesignTimePreview")) ||
        Name == TEXT("bCanEverAffectNavigation") ||
        Name == TEXT("bIsVariable") ||           // внутренний флаг
        Name.StartsWith(TEXT("On")) &&           // OnXXX delegates уже отображаются отдельно
            Property->IsA<FMulticastDelegateProperty>())
    {
        return false;
    }

    // Оставляем только те, что помечены как EditAnywhere / BlueprintVisible
    // (самый надёжный фильтр для пользовательских переменных)
    return Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible);
}

FString BPR_Extractor_Widget::CleanName(const FString& RawName)
{
    FString Result = RawName;

    // Убираем хвост после последнего '_', если он выглядит как GUID (32+ символов)
    int32 UnderscoreIndex = INDEX_NONE;
    if (RawName.FindLastChar('_', UnderscoreIndex))
    {
        FString Tail = RawName.Mid(UnderscoreIndex + 1);
        if (Tail.Len() >= 32 && Tail.Len() <= 40) // 32 hex — стандартный GUID без скобок
        {
            Result = RawName.Left(UnderscoreIndex);
        }
    }

    // Дополнительно: убираем префиксы, которые часто бывают в UMG (K2Node_, etc.)
    // Но осторожно — не переусердствуем
    if (Result.StartsWith(TEXT("K2Node_")))
    {
        Result = Result.Mid(7);
    }
    else if (Result.StartsWith(TEXT("bp_")) || Result.StartsWith(TEXT("BP_")))
    {
        Result = Result.Mid(3);
    }

    // Убираем лишние пробелы/символы, если есть
    Result.TrimStartInline();
    Result.TrimEndInline();

    return Result.IsEmpty() ? RawName : Result; // fallback на оригинал
}

void BPR_Extractor_Widget::AppendGraphSequence(UEdGraph* Graph, FString& OutExecText, FString& OutDataText)
{
    if (!Graph || Graph->Nodes.Num() == 0) return;

    TSet<UEdGraphNode*> Visited;

    // 1. Ищем точку входа (Function Entry / Event)
    UEdGraphNode* StartNode = FindFunctionEntryNodeInGraph(Graph);

    // 2. Если точки входа нет — берём первую ноду (для Event Graph без entry)
    if (!StartNode)
    {
        StartNode = Graph->Nodes.Num() > 0 ? Graph->Nodes[0] : nullptr;
    }

    // 3. Обход exec-цепочки
    if (StartNode)
    {
        ProcessNodeSequence(StartNode, 0, Visited, OutExecText, OutDataText);
    }

    // 4. Отдельный проход по чистым (pure/computational) нодам, которые не попали в exec
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node || Visited.Contains(Node)) 
            continue;

        if (IsComputationalNode(Node))
        {
            FString NodeTitle = GetReadableNodeName(Node);
            if (!Node->NodeComment.IsEmpty())
            {
                NodeTitle += FString::Printf(TEXT(" // %s"), *Node->NodeComment);
            }

            OutDataText += FString::Printf(TEXT("[pure] %s (no exec)\n"), *NodeTitle);
            Visited.Add(Node);

            // Обход data-пинов
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
                    {
                        TargetNodeName = TEXT("Reroute ") + TargetNodeName;
                    }

                    OutDataText += FString::Printf(TEXT("   [data] %s.%s → %s.%s\n"),
                        *NodeTitle, *PinName, *TargetNodeName, *TargetPinName);

                    if (!Visited.Contains(LinkedTo->GetOwningNode()))
                    {
                        ProcessNodeSequence(LinkedTo->GetOwningNode(), 1, Visited, OutExecText, OutDataText);
                    }
                }
            }
        }
    }
}

FString BPR_Extractor_Widget::GetFunctionSignature(UEdGraph* Graph)
{
    if (!Graph) return TEXT("None");

    UK2Node_FunctionEntry* Entry = FindFunctionEntryNodeInGraph(Graph);
    UK2Node_FunctionResult* Result = FindFunctionResultNodeInGraph(Graph);

    TArray<FString> Inputs, Outputs;

    auto GetPinLabel = [](UEdGraphPin* Pin) -> FString
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

    FString InputsStr = Inputs.Num() > 0 ? FString::Join(Inputs, TEXT(", ")) : TEXT("void");
    FString OutputsStr = Outputs.Num() > 0 ? FString::Join(Outputs, TEXT(", ")) : TEXT("void");

    return FString::Printf(TEXT("(%s) → %s"),
        *InputsStr,
        *OutputsStr);
}

FString BPR_Extractor_Widget::GetMacroSignature(UEdGraph* Graph)
{
    if (!Graph) return TEXT("None");

    TArray<FString> Inputs, Outputs;

    auto GetPinLabel = [](UEdGraphPin* Pin) -> FString
    {
        const FString Display = Pin->GetDisplayName().ToString();
        return Display.IsEmpty() ? Pin->PinName.ToString() : Display;
    };

    // Макросы определяются через UK2Node_Tunnel (входные/выходные туннели)
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
            {
                Inputs.Add(PinDesc);
            }
            else if (Pin->Direction == EGPD_Output)
            {
                Outputs.Add(PinDesc);
            }
        }
    }

    // Более читаемый формат: (входы) → (выходы)
    FString InputsStr = Inputs.Num() > 0 ? FString::Join(Inputs, TEXT(", ")) : TEXT("void");
    FString OutputsStr = Outputs.Num() > 0 ? FString::Join(Outputs, TEXT(", ")) : TEXT("void");

    return FString::Printf(TEXT("(%s) → %s"), *InputsStr, *OutputsStr);
}

void BPR_Extractor_Widget::ProcessNodeSequence(
    UEdGraphNode* Node, 
    int32 IndentLevel, 
    TSet<UEdGraphNode*>& Visited, 
    FString& OutExecText, 
    FString& OutDataText)
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

    // 1. Читаемое имя ноды + комментарий
    FString NodeTitle = GetReadableNodeName(Node);
    if (!Node->NodeComment.IsEmpty())
    {
        NodeTitle += FString::Printf(TEXT(" // %s"), *Node->NodeComment);
    }

    // 2. Вывод в exec или data в зависимости от типа ноды
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

    // 3. Data-flow — не-exec пины
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
            {
                TargetNodeName = TEXT("Reroute ") + TargetNodeName;
            }

            OutDataText += FString::Printf(TEXT("%*s[data] %s.%s → %s.%s\n"),
                (IndentLevel + 1) * 2, TEXT(""), *NodeTitle, *PinName, *TargetNodeName, *TargetPinName);

            // Рекурсия только для pure-нод (чтобы не ходить по exec-цепочке повторно)
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

    // 4. Exec-ветвление (then, else, completed и т.д.)
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin || Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec) 
            continue;

        FString Label = Pin->PinFriendlyName.IsEmpty() ? TEXT("then") : Pin->PinFriendlyName.ToString();

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

UK2Node_FunctionEntry* BPR_Extractor_Widget::FindFunctionEntryNodeInGraph(UEdGraph* Graph)
{
    if (!Graph) return nullptr;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
        {
            return Entry;
        }
    }

    return nullptr;
}

UK2Node_FunctionResult* BPR_Extractor_Widget::FindFunctionResultNodeInGraph(UEdGraph* Graph)
{
    if (!Graph) return nullptr;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UK2Node_FunctionResult* Result = Cast<UK2Node_FunctionResult>(Node))
        {
            return Result;
        }
    }

    return nullptr;
}

FString BPR_Extractor_Widget::GetReadableNodeName(UEdGraphNode* Node)
{
    if (!Node) return TEXT("None");

    // Custom Events
    if (UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(Node))
    {
        if (CustomEvent->CustomFunctionName != NAME_None)
        {
            return CustomEvent->CustomFunctionName.ToString();
        }
    }

    // Events (Event Construct, OnClicked и т.д.)
    if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
    {
        FName EventName = EventNode->EventReference.GetMemberName();
        if (EventName != NAME_None)
        {
            return EventName.ToString();
        }
    }

    // Variable Set
    if (UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
    {
        return FString::Printf(TEXT("Set %s"), 
            *VarSet->VariableReference.GetMemberName().ToString());
    }

    // Variable Get
    if (UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(Node))
    {
        return FString::Printf(TEXT("Get %s"), 
            *VarGet->VariableReference.GetMemberName().ToString());
    }

    // Call Function
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
        Params.TrimEndInline();

        return FString::Printf(TEXT("Call %s(%s)"), 
            *CallFunc->FunctionReference.GetMemberName().ToString(), 
            *Params);
    }

    // Branch (If Then Else)
    if (UK2Node_IfThenElse* Branch = Cast<UK2Node_IfThenElse>(Node))
    {
        return FString::Printf(TEXT("If %s"), *GetPinDetails(Branch->GetConditionPin()));
    }

    // Switch
    if (UK2Node_Switch* Switch = Cast<UK2Node_Switch>(Node))
    {
        return FString::Printf(TEXT("Switch on %s"), *GetPinDetails(Switch->GetSelectionPin()));
    }

    // Common node types
    if (Cast<UK2Node_CallDelegate>(Node)) return TEXT("Call Delegate");
    if (Cast<UK2Node_Tunnel>(Node)) return TEXT("Macro Tunnel");
    if (Cast<UK2Node_FunctionEntry>(Node)) return TEXT("Function Entry");
    if (Cast<UK2Node_FunctionResult>(Node)) return TEXT("Function Result");

    // Widget-specific additions (optional, but useful)
    if (Cast<UK2Node_ComponentBoundEvent>(Node))
    {
        return TEXT("Component Bound Event");
    }

    // Fallback to node title or name
    FString Title = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
    if (!Title.IsEmpty())
    {
        return Title;
    }

    return Node->GetName();
}

FString BPR_Extractor_Widget::GetPinDisplayName(UEdGraphPin* Pin)
{
    if (!Pin) return TEXT("None");

    if (!Pin->PinFriendlyName.IsEmpty())
    {
        return Pin->PinFriendlyName.ToString();
    }

    if (!Pin->PinName.IsNone())
    {
        return Pin->PinName.ToString();
    }

    return TEXT("Unknown");
}

bool BPR_Extractor_Widget::IsComputationalNode(UEdGraphNode* Node)
{
    if (!Node) return false;

    // 1) Reroute / Knot — чистый прокси для data-flow
    if (Node->IsA(UK2Node_Knot::StaticClass()))
    {
        return true;
    }

    // 2) Math Expression
    if (Node->IsA(UK2Node_MathExpression::StaticClass()))
    {
        return true;
    }

    // 3) Casts (DynamicCast, CastByteToEnum)
    if (Node->IsA(UK2Node_DynamicCast::StaticClass()) ||
        Node->IsA(UK2Node_CastByteToEnum::StaticClass()))
    {
        return true;
    }

    // 4) Select (switch on value)
    if (Node->IsA(UK2Node_Select::StaticClass()))
    {
        return true;
    }

    // 5) Variable Get — чистый источник данных
    if (Node->IsA(UK2Node_VariableGet::StaticClass()))
    {
        return true;
    }

    // 6) Variable Set без exec-входа — тоже чисто data
    if (UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
    {
        if (!HasExecInput(Node))
        {
            return true;
        }
    }

    // 7) Call Function — если помечена BlueprintPure или без exec-входа
    if (UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
    {
        if (UFunction* Func = CallFunc->GetTargetFunction())
        {
            if (Func->HasAnyFunctionFlags(FUNC_BlueprintPure))
            {
                return true;
            }
        }
        // Также считаем чистой, если нет exec-входа (редко, но бывает)
        if (!HasExecInput(Node))
        {
            return true;
        }
    }

    return false;
}

FString BPR_Extractor_Widget::GetPinDetails(UEdGraphPin* Pin)
{
    if (!Pin) return TEXT("None");

    // Значение по умолчанию (если задано в редакторе)
    if (!Pin->DefaultValue.IsEmpty())
    {
        return Pin->DefaultValue;
    }

    // Связь с другим пином (linked from...)
    if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0] && Pin->LinkedTo[0]->GetOwningNode())
    {
        FString SourceNode = GetReadableNodeName(Pin->LinkedTo[0]->GetOwningNode());
        return FString::Printf(TEXT("<linked from %s>"), *SourceNode);
    }

    return TEXT("default");
}

bool BPR_Extractor_Widget::HasExecInput(UEdGraphNode* Node)
{
    if (!Node) return false;

    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin) continue;

        if (Pin->Direction == EGPD_Input && 
            Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
        {
            return true;
        }
    }

    return false;
}

void BPR_Extractor_Widget::AppendWidgetEventBindings(UWidget* Widget, UWidgetBlueprint* WidgetBP, FString& OutText, int32 Indent)
{
    if (!Widget || !WidgetBP) return;

    FString IndentStr = FString::ChrN(Indent * 2, ' ');
    OutText += IndentStr + TEXT("Event Bindings:\n");

    bool bHasAnyBinding = false;

    for (UEdGraph* Graph : WidgetBP->UbergraphPages)
    {
        if (!Graph) continue;

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            UK2Node_ComponentBoundEvent* BoundEvent = Cast<UK2Node_ComponentBoundEvent>(Node);
            if (!BoundEvent) continue;

            if (BoundEvent->GetComponentPropertyName().ToString().Equals(Widget->GetName(), ESearchCase::CaseSensitive))
            {
                bHasAnyBinding = true;

                // Имя события
                FString EventName = TEXT("UnknownEvent");
                if (FText DisplayName = BoundEvent->GetTargetDelegateDisplayName(); !DisplayName.IsEmpty())
                {
                    EventName = DisplayName.ToString();
                }
                else if (FMulticastDelegateProperty* DelegateProp = BoundEvent->GetTargetDelegateProperty())
                {
                    EventName = DelegateProp->GetName();   // исправлено
                }

                // Имя функции-обработчика
                FString FunctionName = BoundEvent->EventReference.GetMemberName().ToString();
                if (FunctionName.IsEmpty())
                {
                    FunctionName = TEXT("UnknownFunction");
                }
                else
                {
                    FunctionName = CleanName(FunctionName);   // рекомендуется
                }

                OutText += IndentStr + FString::Printf(TEXT("  - %s → %s\n"), *EventName, *FunctionName);
            }
        }
    }

    if (!bHasAnyBinding)
    {
        OutText += IndentStr + TEXT("  - No event bindings found\n");
    }

    OutText += TEXT("\n");
}