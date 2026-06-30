// Copyright (c) 2026 Racoon Coder. All rights reserved.
#include "Extractors/BPR_Extractor_Base.h"

#include "Math/Vector.h"
#include "Math/Rotator.h"
#include "Math/TransformNonVectorized.h"
#include "UObject/UnrealType.h"
#include "EdGraph/EdGraphPin.h"
#include "Containers/Array.h"
#include "Editor/BlueprintGraph/Classes/EdGraphSchema_K2.h"



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
// String Utilities
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

FString BPR_Extractor_Base::GetPinDisplayName(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return TEXT("None");
	}

	// Предпочитаем пользовательское дружественное имя
	if (!Pin->PinFriendlyName.IsEmpty())
	{
		return Pin->PinFriendlyName.ToString();
	}

	// Затем техническое имя пина
	if (!Pin->PinName.IsNone())
	{
		return Pin->PinName.ToString();
	}

	return TEXT("Unknown");
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
// Error processing
// ===================================================================

void BPR_Extractor_Base::SetErrorData(FBPR_ExtractedData& OutData, const FString& ErrorMessage) const
{
	LogError(ErrorMessage);
	OutData.Structure = FText::FromString(ErrorMessage);
	OutData.Graph     = FText::FromString(TEXT("N/A"));
	OutData.Design    = FText::FromString(TEXT("N/A"));
	OutData.AssetType = EAssetType::Blueprint;
}

// ===================================================================
// Property Helpers
// ===================================================================

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

FString BPR_Extractor_Base::GetPropertyTypeDetailed(const FProperty* Property, bool bIncludeObjectFlags /*= false*/)
{
    if (!Property)
    {
        return TEXT("Unknown");
    }

    // ===================================================================
    // Специфические типы объектов и ссылок (самое важное для читаемости)
    // ===================================================================
    if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
    {
        FString Result = FString::Printf(TEXT("%s*"), *ObjProp->PropertyClass->GetName());

        if (bIncludeObjectFlags)
        {
            if (ObjProp->HasAnyPropertyFlags(CPF_InstancedReference))
                Result += TEXT(" (Instanced)");
            if (ObjProp->HasAnyPropertyFlags(CPF_ExportObject))
                Result += TEXT(" (Export)");
        }
        return Result;
    }

    if (const FClassProperty* ClassProp = CastField<FClassProperty>(Property))
    {
        return FString::Printf(TEXT("TSubclassOf<%s>"), *ClassProp->MetaClass->GetName());
    }

    if (const FSoftObjectProperty* SoftProp = CastField<FSoftObjectProperty>(Property))
    {
        FString Result = FString::Printf(TEXT("TSoftObjectPtr<%s>"), *SoftProp->PropertyClass->GetName());
        if (bIncludeObjectFlags && SoftProp->HasAnyPropertyFlags(CPF_AssetRegistrySearchable))
            Result += TEXT(" (Asset)");
        return Result;
    }

    if (const FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Property))
    {
        return FString::Printf(TEXT("TSoftClassPtr<%s>"), *SoftClassProp->MetaClass->GetName());
    }

    if (const FWeakObjectProperty* WeakProp = CastField<FWeakObjectProperty>(Property))
    {
        return FString::Printf(TEXT("TWeakObjectPtr<%s>"), *WeakProp->PropertyClass->GetName());
    }

    if (const FInterfaceProperty* InterfaceProp = CastField<FInterfaceProperty>(Property))
    {
        return FString::Printf(TEXT("TScriptInterface<%s>"), *InterfaceProp->InterfaceClass->GetName());
    }

    // ===================================================================
    // Структуры
    // ===================================================================
    if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        return StructProp->Struct->GetName();  // FVector, FRotator, FTransform, FMyCustomStruct и т.д.
    }

    // ===================================================================
    // Контейнеры (рекурсивно используем эту же функцию)
    // ===================================================================
    if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        return FString::Printf(TEXT("TArray<%s>"), 
            *GetPropertyTypeDetailed(ArrayProp->Inner, bIncludeObjectFlags));
    }

    if (const FMapProperty* MapProp = CastField<FMapProperty>(Property))
    {
        return FString::Printf(TEXT("TMap<%s, %s>"), 
            *GetPropertyTypeDetailed(MapProp->KeyProp, bIncludeObjectFlags),
            *GetPropertyTypeDetailed(MapProp->ValueProp, bIncludeObjectFlags));
    }

    if (const FSetProperty* SetProp = CastField<FSetProperty>(Property))
    {
        return FString::Printf(TEXT("TSet<%s>"), 
            *GetPropertyTypeDetailed(SetProp->ElementProp, bIncludeObjectFlags));
    }

    // ===================================================================
    // Enum
    // ===================================================================
    if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        if (UEnum* Enum = EnumProp->GetEnum())
        {
            return Enum->GetName();
        }
        return TEXT("Enum");
    }

    if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        if (UEnum* Enum = ByteProp->GetIntPropertyEnum())
        {
            return Enum->GetName();
        }
        return TEXT("uint8");
    }

    // ===================================================================
    // Делегиаты (часто встречаются в Blueprint)
    // ===================================================================
    if (Property->IsA<FMulticastDelegateProperty>() || Property->IsA<FDelegateProperty>())
    {
        return TEXT("Delegate");
    }

    // ===================================================================
    // Базовые примитивы и fallback
    // ===================================================================
    return Property->GetCPPType();
}

FString BPR_Extractor_Base::GetPropertyTypeDetailedFromPin(const UEdGraphPin* Pin, bool bIncludeObjectFlags /*= false*/)
{
    if (!Pin)
    {
        return TEXT("Unknown");
    }

    const FEdGraphPinType& PinType = Pin->PinType;

    // Exec пин
    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
    {
        return TEXT("Exec");
    }

    // Редкий случай с FProperty* (пока отключён)
    // TODO: Реализовать правильно, когда будет время
    // if (FProperty* Property = CastField<FProperty>(PinType.PinSubCategoryObject.Get()))
    // {
    //     return GetPropertyTypeDetailed(Property, bIncludeObjectFlags);
    // }

    // ====================== Основная логика ======================
    FString TypeStr;

    if (PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
        TypeStr = TEXT("bool");
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Byte)
    {
        if (UEnum* Enum = Cast<UEnum>(PinType.PinSubCategoryObject.Get()))
            TypeStr = Enum->GetName();
        else
            TypeStr = TEXT("uint8");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
        TypeStr = TEXT("int32");
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Int64)
        TypeStr = TEXT("int64");
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Float)
        TypeStr = TEXT("float");
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Double)
        TypeStr = TEXT("double");
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
        TypeStr = TEXT("FName");
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_String)
        TypeStr = TEXT("FString");
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
        TypeStr = TEXT("FText");
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
    {
    	if (UScriptStruct* Struct = Cast<UScriptStruct>(PinType.PinSubCategoryObject.Get()))
    	{
    		if (Struct == TBaseStructure<FVector>::Get())
    			TypeStr = TEXT("FVector");
    		else if (Struct == TBaseStructure<FRotator>::Get())
    			TypeStr = TEXT("FRotator");
    		else if (Struct == TBaseStructure<FTransform>::Get())
    			TypeStr = TEXT("FTransform");
    		else
    			TypeStr = Struct->GetName();
    	}
    	else
    	{
    		TypeStr = TEXT("FStruct");
    	}
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
    {
        if (UClass* Class = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
            TypeStr = FString::Printf(TEXT("%s*"), *Class->GetName());
        else
            TypeStr = TEXT("UObject*");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Interface)
    {
        if (UClass* Class = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
            TypeStr = FString::Printf(TEXT("TScriptInterface<%s>"), *Class->GetName());
        else
            TypeStr = TEXT("Interface");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
    {
        if (UClass* Class = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
            TypeStr = FString::Printf(TEXT("TSubclassOf<%s>"), *Class->GetName());
        else
            TypeStr = TEXT("UClass*");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject)
    {
        if (UClass* Class = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
            TypeStr = FString::Printf(TEXT("TSoftObjectPtr<%s>"), *Class->GetName());
        else
            TypeStr = TEXT("TSoftObjectPtr<UObject>");
    }
    else if (PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass)
    {
        if (UClass* Class = Cast<UClass>(PinType.PinSubCategoryObject.Get()))
            TypeStr = FString::Printf(TEXT("TSoftClassPtr<%s>"), *Class->GetName());
        else
            TypeStr = TEXT("TSoftClassPtr<UClass>");
    }
    else
    {
        TypeStr = PinType.PinCategory.ToString();   // fallback
    }

    // Контейнеры (Array, Map, Set) — применяем после определения базового типа
    if (PinType.IsArray())
    {
        TypeStr = FString::Printf(TEXT("TArray<%s>"), *TypeStr);
    }
    else if (PinType.IsMap())
    {
    	// Для Value-типа в TMap<> используем TerminalCategory + TerminalSubCategoryObject
    	FString ValueTypeStr;

    	const FEdGraphTerminalType& ValueTerminal = PinType.PinValueType;

    	if (ValueTerminal.TerminalCategory == UEdGraphSchema_K2::PC_Struct)
    	{
    		if (UScriptStruct* Struct = Cast<UScriptStruct>(ValueTerminal.TerminalSubCategoryObject.Get()))
    		{
    			if (Struct == TBaseStructure<FVector>::Get())
    				ValueTypeStr = TEXT("FVector");
    			else if (Struct == TBaseStructure<FRotator>::Get())
    				ValueTypeStr = TEXT("FRotator");
    			else if (Struct == TBaseStructure<FTransform>::Get())
    				ValueTypeStr = TEXT("FTransform");
    			else
    				ValueTypeStr = Struct->GetName();
    		}
    		else
    		{
    			ValueTypeStr = TEXT("FStruct");
    		}
    	}
    	else if (ValueTerminal.TerminalCategory == UEdGraphSchema_K2::PC_Object)
    	{
    		if (UClass* Class = Cast<UClass>(ValueTerminal.TerminalSubCategoryObject.Get()))
    			ValueTypeStr = FString::Printf(TEXT("%s*"), *Class->GetName());
    		else
    			ValueTypeStr = TEXT("UObject*");
    	}
    	else if (ValueTerminal.TerminalCategory == UEdGraphSchema_K2::PC_Class)
    	{
    		if (UClass* Class = Cast<UClass>(ValueTerminal.TerminalSubCategoryObject.Get()))
    			ValueTypeStr = FString::Printf(TEXT("TSubclassOf<%s>"), *Class->GetName());
    		else
    			ValueTypeStr = TEXT("UClass*");
    	}
    	// ... можно добавить другие типы по мере необходимости
    	else
    	{
    		ValueTypeStr = ValueTerminal.TerminalCategory.ToString();
    	}

    	if (ValueTypeStr.IsEmpty())
    		ValueTypeStr = TEXT("Unknown");

    	TypeStr = FString::Printf(TEXT("TMap<%s, %s>"), *TypeStr, *ValueTypeStr);
    }
    else if (PinType.IsSet())
    {
        TypeStr = FString::Printf(TEXT("TSet<%s>"), *TypeStr);
    }

    return TypeStr;
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

// CanHandleAsset now has default impl in header (return false).