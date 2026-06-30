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
**M2 progress:** Widget inherits `BPR_Extractor_Object` (M2.1 ✅). `BPR_Extractor_WidgetAdapter` removed (M2.6 ✅). Duplicate graph/variable helper stack (15 dead methods, ~730 lines) removed from `BPR_Extractor_Widget.cpp`/`.h` (M2.2 ✅, build verified) — graph extraction now owned solely by `BPR_Extractor_Object`. Widget.cpp ~3445 → 2715 lines. `CleanName` kept widget-local (only live duplicate; base-parity dedup pending). Next: M2.3 Bindings, M2.4 custom WBP text, M2.5 `LogBlueprintReader` instead of `LogTemp`.

## Working rules

- **Git commits/push — user only.** Never commit or push autonomously.
- Read current files before editing; propose a plan for significant changes.
- After a logical group of edits, suggest a build check.
- Prefer minimal, safe changes. Quality over speed.
- Full project context in `.grok/Private/` (Russian). Key docs: `CURRENT_PLAN.md`, `NOTES.md`, `ROADMAP.md`, `ARCHITECTURE.md`.
