// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/BPR_Types.h"

// Forward declarations
class BPR_Extractor_Base;

/**
 * Central service and extractor factory.
 * 
 * Responsibilities:
 *   - Holds and registers all available extractors
 *   - Finds the most suitable extractor for a given asset using priority + CanHandleAsset()
 *   - Serves as the single entry point for UI layer
 *   - Provides unsupported asset handling
 */
class BLUEPRINTREADER_API BPR_Core
{
public:
	BPR_Core();
	~BPR_Core();

	/** Must be called once during module startup to register all extractors */
	void RegisterAllExtractors();

	/** Returns true if any extractor can handle this asset */
	bool IsSupportedAsset(UObject* Asset) const;

	/**
	 * Main extraction entry point.
	 * Finds the best extractor by priority and calls its Extract() method.
	 * If no extractor found → fills OutData with unsupported state.
	 */
	void ExtractAsset(UObject* Asset, FBPR_ExtractedData& OutData);

	/** Returns information for "unsupported asset" UI dialog */
	FUnsupportedAssetInfo GetUnsupportedAssetInfo() const;

private:
	/** Finds the highest priority extractor that can handle the asset */
	BPR_Extractor_Base* FindSuitableExtractor(UObject* Asset) const;

	/** List of all registered extractors, sorted by priority (highest first) */
	TArray<TUniquePtr<BPR_Extractor_Base>> Extractors;
};