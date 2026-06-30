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
| M2 — Widget as first-class `BPR_Extractor_Object` child | 🔄 In progress |
| M3 — Material/MaterialFunction in hierarchy | ⏳ |
| M4 — UX: settings, file export | ⏳ |
| M5 — CI, smoke tests, Fab v0.2 | ⏳ |

**Current branch:** `WidgetRefactoring`  
**M2 progress:** M2.1 ✅ (Widget inherits `BPR_Extractor_Object`), M2.6 ✅ (WidgetAdapter removed), M2.2 ✅ (graph/variable dup removed, ~730 lines), M2.3 ✅ (event→handler bindings via `AppendWidgetEventBindings`/`K2Node_ComponentBoundEvent`, already wired; dead old `AppendWidgetBindings` removed), M2.5 ✅ (Widget logging now via inherited base → `LogBlueprintReader`). Widget.cpp ~3445 → 2660 lines. All builds verified on UE 5.7.
**M2.4 🔄 (remaining):** Custom `WBP_*` (UUserWidget) extraction. Recursion into a nested `UserWidget->WidgetTree` already exists (`ProcessWidgetHierarchy`, ~line 247) BUT only fires when the nested instance's `WidgetTree` is populated — for many nested WBP instances it's null at asset level, so only the type prints. Closing the gap needs resolving the child's WidgetBlueprint from its GeneratedClass + **editor validation on a real nested WBP**.
**Bugfixes (editor-validated):** `ProcessWidget` now sets `OutData.AssetType = EAssetType::Widget` — was unset (Unknown=0), so the UI `RebuildTabsFromData()` switch fell through to "unsupported, type 0" for every widget. Duplicate Design title header removed (`TmpDesign` no longer pre-seeds a `#` header; `AppendWidgetTree` emits the single `##` one). Design tab confirmed populated in-editor.

**Follow-ups:** `CleanName` base-parity dedup (only widget-local duplicate left); module-wide `LogTemp`→`LogBlueprintReader` in non-Widget files (UI, Material, ActorComponent); resolve brush/image `ResourceObject` to an asset name instead of a raw GUID in Design output (M4 output-quality); same `AssetType` mis-wire latent in `BPR_Extractor_Interface` (sets `Blueprint` instead of `InterfaceBP`).

## Working rules

- **Git commits/push — user only.** Never commit or push autonomously.
- Read current files before editing; propose a plan for significant changes.
- After a logical group of edits, suggest a build check.
- Prefer minimal, safe changes. Quality over speed.
- Full project context in `.grok/Private/` (Russian). Key docs: `CURRENT_PLAN.md`, `NOTES.md`, `ROADMAP.md`, `ARCHITECTURE.md`.
