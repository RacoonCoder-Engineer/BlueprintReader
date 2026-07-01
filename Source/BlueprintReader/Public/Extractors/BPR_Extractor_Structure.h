// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Extractors/BPR_Extractor_Base.h"
#include "StructUtils/UserDefinedStruct.h"   // M6: canonical CoreUObject path — the old
                                              // Engine/UserDefinedStruct.h forwarder was removed in UE 5.8;
                                              // this path exists unchanged in both 5.7 and 5.8 (free fix, no #if).

/**
 * Extractor for UScriptStruct (structures)
 */
class BLUEPRINTREADER_API BPR_Extractor_Structure : public BPR_Extractor_Base
{
public:
	BPR_Extractor_Structure();

	/** Main entry point used by BPR_Core */
	virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) override;

	virtual bool CanHandleAsset(UObject* Asset) const override;
	virtual int32 GetPriority() const override { return 50; }

private:
	/** Appends detailed information about the structure fields as a Markdown table */
	static void AppendStructInfo(const UScriptStruct* Struct, FString& OutText);
};