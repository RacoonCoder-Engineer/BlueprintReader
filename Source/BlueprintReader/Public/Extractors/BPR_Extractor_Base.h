// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Types.h"
#include "Math/MathFwd.h"

// Forward declarations
class UObject;
class FProperty;
class UEdGraphPin;

/**
 * Base class for ALL asset extractors.
 * Plain C++ class (no UObject inheritance).
 */
class BLUEPRINTREADER_API BPR_Extractor_Base
{
public:
    BPR_Extractor_Base();
    virtual ~BPR_Extractor_Base() = default;

    /** Returns short name of this extractor (used in logs) */
    FString GetExtractorName() const;

    /** 
     * Старый API (пока оставляем для совместимости с Core)
     * ToDo: будет удалён после полной миграции 
     */
    virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) = 0;

    /** 
     * Новый рекомендуемый API 
     */
    virtual void Extract(UObject* Asset, FBPR_ExtractedData& OutData) = 0;

    /** 
     * Determines if this extractor can handle the given asset.
     * Override in derived classes to implement type-specific logic.
     * 
     * @return true if this extractor supports the asset
     */
    virtual bool CanHandleAsset(UObject* Asset) const = 0;

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
    static FString GetPinDisplayName(const UEdGraphPin* Pin);

    // ===================================================================
    // Property Helpers
    // ===================================================================
    static FString GetPropertyTypeDetailed(const FProperty* Property, bool bIncludeObjectFlags = false);
    static FString GetPropertyDescription(const FProperty* Property);
    static FString GetPropertyTypeDetailedFromPin(const UEdGraphPin* Pin, bool bIncludeObjectFlags = false);

    /** Should be called from derived class constructors */
    void SetExtractorName(const FString& InName);

private:
    FString ExtractorName = TEXT("Base");
    EOutputFormat CurrentOutputFormat = EOutputFormat::Minimal;
};