// Copyright (c) 2026 Racoon Coder. All rights reserved.

//ToDo:
// Минимальный план на ближайшую итерацию:
// Вынести все общие типы (EAssetType, FBPR_ExtractedData, FUnsupportedAssetInfo, EOutputFormat) в отдельный файл BPR_Types.h.
// Убрать зависимость Base от Core.
// Решить, какой API будет основным (я голосую за FString Extract(const UObject*) + отдельно void FillExtractedData(...) если нужно).
// Начать постепенное вынесение хелперов (PropertyHelpers, MarkdownWriter, StringUtils и т.д.) в отдельные классы/пространства имён, как ты и планировал.

#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Core.h"
#include "Math/MathFwd.h"

// Forward declarations
class UObject;
class FProperty;
class UEdGraphPin;

enum class EOutputFormat : uint8
{
    HumanReadable,     // Beautiful, multi-line output with indentation (for humans)
    Compact,           // Balanced format, saves tokens for LLMs
    Minimal            // Most compact format (best for LLMs)
};

/**
 * Base class for ALL asset extractors.
 * Plain C++ class (no UObject inheritance).
 *
 * Contains only common infrastructure that is shared between:
 * - Simple extractors (Enum, Structure, Material...)
 * - Complex extractors (Blueprint Object, Actor, Widget...)
 */
class BLUEPRINTREADER_API BPR_Extractor_Base
{
public:
    BPR_Extractor_Base();
    virtual ~BPR_Extractor_Base() = default;

    /** Returns short name of this extractor (used in logs) */
    FString GetExtractorName() const;

    /** Main entry point used by current menu system (ToDo: remove in future)*/
    virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) = 0;

    /** Cleaner API for future use (recommended to migrate to gradually) */
    virtual FString Extract(const UObject* Asset) { return FString(); }

protected:
    // ===================================================================
    // Logging
    // ===================================================================
    void LogMessage(const FString& Msg) const;
    void LogWarning(const FString& Msg) const;
    void LogError(const FString& Msg) const;
    
    // ===================================================================
    // Error processing
    // ===================================================================
    /** Fills OutData with error message when cast fails */
    void SetErrorData(FBPR_ExtractedData& OutData, const FString& ErrorMessage) const;
    
    

    // ===================================================================
    // Formatting Helpers
    // ===================================================================
    void AppendSectionHeader(FString& OutText, const FString& Title) const;
    void AppendKeyValue(FString& OutText, const FString& Key, const FString& Value, int32 Indent = 0) const;

    EOutputFormat GetOutputFormat() const { return CurrentOutputFormat; }
    void SetOutputFormat(EOutputFormat Format) { CurrentOutputFormat = Format; }

    // ===================================================================
    // Markdown Table Helpers
    // ===================================================================
    static void BeginMarkdownTable(FString& OutText,
                                   const TArray<FString>& Headers,
                                   int32 Indent = 0,
                                   bool bBoldHeaders = true);

    static void AppendTableRow(FString& OutText, const TArray<FString>& Columns, int32 Indent = 0);

    // ===================================================================
    // String Utilities
    // ===================================================================
    static FString GetIndent(int32 Level);
    static FString CleanName(const FString& RawName);
    
    /** Returns a human-readable name for a pin (prefers PinFriendlyName, then PinName) */
    static FString GetPinDisplayName(const UEdGraphPin* Pin);

    // ===================================================================
    // Property Helpers (shared by Structure, Object, Actor, Widget etc.)
    // ===================================================================
    /**
     * Returns a detailed, human-readable string representation of a property's type.
     * This is one of the most commonly used helpers across all extractors.
     *
     * Handles the following cases intelligently:
     * - Object pointers (UTexture2D*)
     * - Class references (TSubclassOf<AActor>)
     * - Soft references (TSoftObjectPtr<UStaticMesh>)
     * - Weak pointers (TWeakObjectPtr<...>)
     * - Interfaces (TScriptInterface<IMyInterface>)
     * - Structures (FVector, FRotator, FMyCustomStruct, etc.)
     * - Containers: TArray<>, TMap<>, TSet<> (recursively)
     * - Enums and byte enums
     * - Delegates
     *
     * @param Property              The property to describe. Can be nullptr.
     * @param bIncludeObjectFlags   If true, adds extra flags for object properties 
     *                              (e.g. "(Instanced)", "(Export)", "(Asset)"). 
     *                              Useful mainly for complex Blueprint/Object extractors.
     *                              Default: false (clean output).
     *
     * @return Detailed type string. Never empty. Returns "Unknown" if Property is nullptr.
     */
    static FString GetPropertyTypeDetailed(const FProperty* Property, bool bIncludeObjectFlags = false);
    
    static FString GetPropertyDescription(const FProperty* Property);

    /** Should be called from derived class constructors */
    void SetExtractorName(const FString& InName);
    
    /**
     * Returns a detailed, human-readable string representation of a pin's type.
     * Useful for Interface functions, Blueprint graphs, nodes etc.
     *
     * Internally converts FEdGraphPinType to the closest FProperty representation
     * and then calls GetPropertyTypeDetailed().
     *
     * @param Pin                  The EdGraphPin to describe. Can be nullptr.
     * @param bIncludeObjectFlags  Passed through to GetPropertyTypeDetailed.
     *
     * @return Detailed type string (e.g. "int32", "FVector", "TSoftObjectPtr<UTexture2D>", "AActor*")
     */
    static FString GetPropertyTypeDetailedFromPin(const UEdGraphPin* Pin, bool bIncludeObjectFlags = false);
    



private:
    FString ExtractorName = TEXT("Base");
    EOutputFormat CurrentOutputFormat = EOutputFormat::Minimal;
};