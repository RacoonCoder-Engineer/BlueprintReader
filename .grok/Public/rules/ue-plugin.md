# UE Plugin Conventions — BlueprintReader

## Build

- Module: `BlueprintReader`, type `Editor`, loading phase `Default`
- Built inside parent project `PluginsProject.uproject`
- `bUseUnity = false` — intentional; do not enable without a strong reason

## Code patterns

- Use `BLUEPRINTREADER_API` for exported classes
- Log with `UE_LOG(LogBlueprintReader, ...)` — not `LogTemp`
- Editor-only code: `#if WITH_EDITOR`
- Slate widgets: `SBPR_` prefix; manager classes without `S-` prefix
- Extractors: plain C++, owned by `TUniquePtr` in `BPR_Core`

## File layout

- Headers: `Source/BlueprintReader/Public/<Layer>/`
- Implementation: `Source/BlueprintReader/Private/<Layer>/`
- Layers: `Core/`, `Extractors/`, `UI/`

## Includes

```cpp
#include "Core/BPR_Core.h"
#include "Extractors/BPR_Extractor_Actor.h"
```

## Verifying changes

1. Build via Unreal Build Tool (parent project)
2. Open PluginsProject in UE 5.7 Editor
3. Right-click a test asset → "Read Blueprint for AI-Assistant"
4. Confirm Structure / Graph / Design tabs