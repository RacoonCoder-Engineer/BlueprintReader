// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Extractors/BPR_Extractor_Base.h"

class BLUEPRINTREADER_API BPR_Extractor_Enum : public BPR_Extractor_Base
{
public:
	BPR_Extractor_Enum();

	/** Main entry point used by BPR_Core */
	virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) override;

private:
	/** Appends list of all enum entries as a markdown table */
	static void AppendEnumEntries(const UEnum* Enum, FString& OutText);
};