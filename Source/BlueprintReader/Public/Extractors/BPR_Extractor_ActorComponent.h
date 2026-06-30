// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Extractors/BPR_Extractor_Object.h"

// Forward declarations
class UActorComponent;
class UBlueprint;

/**
 * Extractor for ActorComponent Blueprints (UActorComponent-derived classes).
 * Inherits BPR_Extractor_Object to reuse all Blueprint structure/variable/graph
 * logic; only adds the component-specific entry point and replication info.
 */
class BLUEPRINTREADER_API BPR_Extractor_ActorComponent : public BPR_Extractor_Object
{
public:
	BPR_Extractor_ActorComponent();
	~BPR_Extractor_ActorComponent();

	// --------------------------------
	// Main entry point
	// --------------------------------
	/** Processes the selected Component Blueprint, fills Structure/Graph */
	void ProcessComponent(UObject* SelectedObject, FBPR_ExtractedData& OutData);

	// Extractor contract (BPR_Extractor_Base / Object)
	virtual void Process(UObject* SelectedObject, FBPR_ExtractedData& OutData) override;
	virtual bool CanHandleAsset(UObject* Asset) const override;
	virtual int32 GetPriority() const override { return 90; }

	// Blueprint structure / variable / graph logic and logging are inherited
	// from BPR_Extractor_Object / BPR_Extractor_Base.
};
