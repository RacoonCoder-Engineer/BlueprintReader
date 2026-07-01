// Copyright (c) 2026 Racoon Coder. All rights reserved.

#pragma once

// Centralizes ALL engine-version branching for BlueprintReader (M6: UE 5.7 + 5.8 support).
//
// Rules:
// - No other file in this plugin uses UE_VERSION_* or ENGINE_*_VERSION directly.
// - Branch older-version-first: #if UE_VERSION_OLDER_THAN(5, 8, 0) ... #else ... #endif
//   (there is no UE_VERSION_NEWER_OR_EQUAL_THAN macro).
// - Add a shim here ONLY after a real UE 5.8 compile error proves it is needed.
//   Do not add speculative guards for touchpoints that merely might change.
#include "Misc/EngineVersionComparison.h"

namespace BPR::Compat
{
	// Intentionally empty. See .grok/Private/M6_MIGRATION_PLAN.md for the reactive-shim policy
	// and the current list of MUST-VERIFY-on-5.8 touchpoints (material expression DAG walk is
	// the most likely first tenant of this namespace).
}
