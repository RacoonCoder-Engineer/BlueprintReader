// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Extractors/BPR_Extractor_Base.h"

/**
 * Extractor for UScriptStruct (structures)
 */
class BLUEPRINTREADER_API BPR_Extractor_Structure : public BPR_Extractor_Base
{
public:
	BPR_Extractor_Structure();

	/** Main entry point used by BPR_Core */
	virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) override;

private:
	/** Appends detailed information about the structure fields as a Markdown table */
	void AppendStructInfo(const UScriptStruct* Struct, FString& OutText) const;
};