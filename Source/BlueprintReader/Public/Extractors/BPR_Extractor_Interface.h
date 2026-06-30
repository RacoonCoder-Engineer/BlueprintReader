// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "Extractors/BPR_Extractor_Base.h"

// Forward declarations
class UBlueprint;
class UK2Node_FunctionEntry;
class UK2Node_FunctionResult;

/**
 * Extractor for Blueprint Interfaces (BPI).
 * Inherits directly from BPR_Extractor_Base because interfaces have
 * significantly different structure than regular Blueprint Classes.
 */
class BLUEPRINTREADER_API BPR_Extractor_Interface : public BPR_Extractor_Base
{
public:
	BPR_Extractor_Interface();
	virtual ~BPR_Extractor_Interface() override = default;

	/** Main entry point used by current menu system */
	virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) override;

	/** Cleaner API for future use - now matches Base via bridge */
	virtual void Extract(UObject* Asset, FBPR_ExtractedData& OutData) override;

	virtual bool CanHandleAsset(UObject* Asset) const override;
	virtual int32 GetPriority() const override { return 70; }

protected:
	/** Extracts interface functions and appends them to OutText */
	virtual void AppendInterfaceFunctions(UBlueprint* BP, FString& OutText) const;

private:
	/** Helper to find Function Entry and Result nodes */
	static UK2Node_FunctionEntry* FindFunctionEntryNode(const UEdGraph* Graph);
	static UK2Node_FunctionResult* FindFunctionResultNode(const UEdGraph* Graph);
};