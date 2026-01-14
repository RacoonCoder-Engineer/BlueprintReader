// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Core.h"

/**
* Extractor for Blueprint Interface
*/
class BLUEPRINTREADER_API BPR_Extractor_InterfaceBP
{
public:
	BPR_Extractor_InterfaceBP();
	~BPR_Extractor_InterfaceBP();

	/** Main data retrieval method */
	void ProcessInterfaceBP(
	UObject* Object,
	FBPR_ExtractedData& OutData
);


private:

	/** Вспомогательный метод для сбора информации о функциях интерфейса */
	void AppendInterfaceFunctions(UBlueprint* BP, FString& OutText);
};
