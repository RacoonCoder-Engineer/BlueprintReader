# CLAUDE.md — BlueprintReader Plugin

UE5.7+ Editor plugin. Right-click an asset in Content Browser → "Read Blueprint for AI-Assistant" → Slate window with Structure / Graph / Design tabs (Markdown output for LLM consumption). MIT, target: Fab marketplace.

## Architecture

```
UI (Slate)  →  BPR_Core (factory)  →  Extractor hierarchy
               BPR_Types.h            BPR_Extractor_Base
                                        ├── BPR_Extractor_Object
                                        │     ├── BPR_Extractor_Actor        (priority 100)
                                        │     ├── BPR_Extractor_ActorComponent (priority 90)
                                        │     └── BPR_Extractor_Widget        (priority 80)
                                        ├── BPR_Extractor_Interface           (priority 70)
                                        ├── BPR_Extractor_Enum/Structure      (priority 50)
                                        ├── BPR_Extractor_Material            (priority 40)
                                        └── BPR_Extractor_MaterialFunction    (priority 30)
```

**Data contract:** `FBPR_ExtractedData { FText Structure, Graph, Design; EAssetType AssetType; }`  
**Extractor contract:** `Extract()`, `Process()`, `CanHandleAsset()`, `GetPriority()`  
**Core API:** `RegisterAllExtractors()`, `IsSupportedAsset()`, `ExtractAsset()`, `GetUnsupportedAssetInfo()`  
Extractors are plain C++ (not UObject), owned via `TSharedPtr` in Core.

## Key files

| File | Role |
|------|------|
| `Source/.../Private/BlueprintReader.cpp` | Module lifecycle, creates Core |
| `Source/.../Private/Core/BPR_Core.cpp` | Extractor factory and registration |
| `Source/.../Public/Core/BPR_Types.h` | Shared data types |
| `Source/.../Private/UI/BPR_ContentBrowserAssetActions.cpp` | UX entry point |
| `Source/.../Private/Extractors/BPR_Extractor_Widget.cpp` | Largest file (~3400 lines) |

## Migration status

| Milestone | Status |
|-----------|--------|
| M1 — Core factory wired, build fixed | ✅ Done (2026-06-27) |
| M2 — Widget as first-class `BPR_Extractor_Object` child | ✅ Done (2026-06-30) |
| M3 — Material/MaterialFunction/ActorComponent in hierarchy | 🔄 code done, validating |
| M4 — UX: settings, file export | ⏳ |
| M5 — CI, smoke tests, Fab v0.2 | ⏳ |

**Current branch:** `M3-AssetCoverage` (off `main` after M2 merge)  
**M3 progress (code done, build-clean on UE 5.7, pending editor validation):** `Material` → `BPR_Extractor_Base` (priority 40), `MaterialFunction` → `Base` (30), `ActorComponent` → `BPR_Extractor_Object` (90). Each implements `Process`/`CanHandleAsset`/`GetPriority`, sets `OutData.AssetType` (the systemic bug), logs via base. ActorComponent dropped its duplicated Blueprint/graph stack (733 → 76 lines, M2.2-style). All three re-enabled in `BPR_Core.cpp`. Material graphs (Expression DAG) kept as-is — not shared with Object. Phase 3 (generic `BPR_Extractor_Blueprint` fallback + README sync) optional/deferred.  
**M2 progress:** M2.1 ✅ (Widget inherits `BPR_Extractor_Object`), M2.6 ✅ (WidgetAdapter removed), M2.2 ✅ (graph/variable dup removed, ~730 lines), M2.3 ✅ (event→handler bindings via `AppendWidgetEventBindings`/`K2Node_ComponentBoundEvent`, already wired; dead old `AppendWidgetBindings` removed), M2.5 ✅ (Widget logging now via inherited base → `LogBlueprintReader`). Widget.cpp ~3445 → 2660 lines. All builds verified on UE 5.7.
**M2.4 ✅ (editor-validated):** Custom `WBP_*` (UUserWidget) extraction. Nested WBP instances have null `WidgetTree` at asset level, so `ProcessWidgetHierarchy`'s recursion (~line 248) doesn't fire. Approach A in `HandleUnknownWidget` (~line 1020): for a `UUserWidget`, iterate its own exposed user variables (`IsUserVariable`) and print non-default instance values (`GetPropertyDefaultValue(Prop, Widget)`) under "Exposed Properties (set on this instance)". Validated: nested labels now surface their actual text (e.g. `In Text: Clan`) and config (`MaxCharToEnter`, ...) — the per-instance values an LLM needs.

**M2 is complete.** Next milestone: **M3** (Material / MaterialFunction into the hierarchy; re-enable their registration in `BPR_Core.cpp` — currently commented out, along with ActorComponent).

**Bugfixes (editor-validated):** `ProcessWidget` now sets `OutData.AssetType = EAssetType::Widget` — was unset (Unknown=0), so the UI `RebuildTabsFromData()` switch fell through to "unsupported, type 0" for every widget. `BPR_Extractor_Interface` now sets `InterfaceBP` (was `Blueprint`, which has no switch case → same "unsupported" symptom). Duplicate Design title header removed.

**Follow-ups:** `CleanName` base-parity dedup (only widget-local duplicate left); module-wide `LogTemp`→`LogBlueprintReader` in non-Widget files (UI, Material, ActorComponent); in Design output, resolve brush/image `ResourceObject` to an asset name instead of a raw GUID, and suppress all-zero/transparent color values like `BackgroundColor:(0,0,0,0)` (M4 output-quality).

## Working rules

- **Git commits/push — user only.** Never commit or push autonomously.
- Read current files before editing; propose a plan for significant changes.
- After a logical group of edits, suggest a build check.
- Prefer minimal, safe changes. Quality over speed.
- Full project context in `.grok/Private/` (Russian). Key docs: `CURRENT_PLAN.md`, `NOTES.md`, `ROADMAP.md`, `ARCHITECTURE.md`.
