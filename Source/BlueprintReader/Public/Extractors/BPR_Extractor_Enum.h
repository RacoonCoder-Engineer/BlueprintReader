// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Core.h"

class BLUEPRINTREADER_API BPR_Extractor_Enum
{
public:
	BPR_Extractor_Enum();
	~BPR_Extractor_Enum();

	/** Main method: converts Enum to FBPR_ExtractedData */
	void ProcessEnum(UObject* Object, FBPR_ExtractedData& OutData);

private:
	/** Assembling a list of Enum elements */
	void AppendEnumEntries(UEnum* Enum, FString& OutText);
};
