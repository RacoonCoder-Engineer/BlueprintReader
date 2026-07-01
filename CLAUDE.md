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
| M3 — Material/MaterialFunction/ActorComponent in hierarchy | ✅ Done (2026-06-30, editor-validated) |
| M4 — UX: settings, file export | ✅ Done (2026-07-01, editor-validated, merged) |
| M5 — CI, smoke tests, Fab v0.2 | ⏳ |
| M6 — UE 5.7 + 5.8 compatibility | 🔄 in progress |

**Current branch:** `M6-EngineCompat` (off `main` @ `6d35ab3`)  
**M4 scope (done):** 4.1 Plugin Settings + 4.2 Export to file — merged to `main`. Dropped: 4.5 README URLs (Patreon/Boosty plans paused). Deferred: 4.3 partial-graph filter, 4.4 progress bar.  
**M6 — UE 5.8 compat (in progress).** Full risk-classified touchpoint table + phased plan: [`.grok/Private/M6_MIGRATION_PLAN.md`](.grok/Private/M6_MIGRATION_PLAN.md) *(RU, not git-tracked by design — see note below; this CLAUDE.md section is the durable summary)*. Approach **A+C** = single source tree + version guards via `UE_VERSION_OLDER_THAN`, centralized in `BPR_Compat.h`; CI compile-matrix 5.7+5.8 is the reliability gate (shared with M5.2). Separate-branches-per-version rejected (drift risk).
- **Exactly 2 CONFIRMED action items, both free fixes (no `#if`), task 6.0:** (a) `USizeBox::bOverride_*` direct field reads in `BPR_Extractor_Widget.cpp` (`HandleSizeBoxProperties`, ~line 2068-2084) — replace with `FindFProperty<FBoolProperty>` reflection, mirroring the existing `AppendBoolFlag` pattern for ListView/TreeView in the same file (~line 2263); (b) remove vestigial `StateTreeModule`/`GameplayStateTreeModule`/`StateTreeEditorModule` from `BlueprintReader.Build.cs` editor deps (unused, link-risk on a 5.8 host without the StateTree plugin enabled).
- Everything else is either confirmed-stable (the whole UI/Slate/export layer — signatures checked against both installed 5.7 and 5.8 engine headers, identical) or MUST-VERIFY-on-5.8 (highest risk: the Material/MaterialFunction expression-DAG walk, duplicated across both extractors — route any 5.8 fix through one `BPR_Compat.h` inline). Epic publishes no 5.7→5.8 C++ deprecation list, so the first real 5.8 compile is the discovery mechanism, not something to pre-guess.
- **Dev workflow (actual, not hypothetical):** user backed up the whole host project to `D:\...\PluginsProject57` (real git checkout of the same remote/branch, frozen 5.7 reference/rollback — NOT edited directly, re-synced via `git pull` if ever needed) before starting migration work on the live `PluginsProject`. Fastest 5.8 compile signal needs no host-project changes at all: run UE 5.8's own `Build.bat`/`RunUAT BuildPlugin` against the plugin — see M6_MIGRATION_PLAN.md §5.
- **Phase 1 result (2026-07-01): plugin now builds clean on BOTH 5.7 and 5.8, `BPR_Compat.h` still empty — zero `#if` needed.** First real `RunUAT BuildPlugin` attempt against 5.8 hit a toolchain artifact (output path >260 chars, `CheckPathLengths` — not a code issue, fixed by using a short `-Package` path). The one real 5.8 break found: `Engine/UserDefinedStruct.h` (used by `BPR_Extractor_Structure.h`/`.cpp`) — its forwarding header was removed in 5.8; canonical path `StructUtils/UserDefinedStruct.h` (CoreUObject) exists unchanged in both versions → free fix, no guard. Everything else — including the material expression-DAG walk, predicted as highest-risk — compiled clean on 5.8 with only engine-header noise warnings. Re-verified 5.7 still builds 0-error after the fix (no regression). Full details: M6_MIGRATION_PLAN.md §7.
- **Remaining for M6:** task 6.3 runtime smoke-diff (real assets, 5.7 vs 5.8 editor output — catches silent reflection-by-name renames that compiling can't) — manual, needs the user in-editor. Then CI compile-matrix + Fab per-version packaging.  
**M4 progress (code done, build-clean on UE 5.7, pending editor validation):** 4.1 ✅ — new `UBPR_Settings : UDeveloperSettings` (`Public/Core/BPR_Settings.h`, config=Editor, category Plugins → "Blueprint Reader"): `ExportFormat` (`EBPR_ExportFormat` Markdown/PlainText), `bRestrictWidgetRecursionDepth`, `WidgetRecursionDepth` (1–50). `DeveloperSettings` added to Build.cs. `ProcessWidget` now reads settings → `SetRecursionSettings()` (was hardcoded `MaxDepth=10`). First UObject in the module — UHT now runs. 4.2 ✅ — `FString AssetName` added to `FBPR_ExtractedData`, set centrally in `BPR_Core::ExtractAsset`. "Export to file…" button in `SBPR_TabSwitcher` footer (persists across tab rebuilds) → `IDesktopPlatform::SaveFileDialog` (default name = asset, ext from `ExportFormat`) → `FFileHelper` UTF-8; success/fail editor notification. `DesktopPlatform` added to Build.cs. Gotcha fixed: `FindBestParentWindowHandleForDialogs` returns `const void*` in 5.7.  
**M3 ✅ (editor-validated, merged to `main`):** `Material` → `BPR_Extractor_Base` (priority 40), `MaterialFunction` → `Base` (30), `ActorComponent` → `BPR_Extractor_Object` (90). Each implements `Process`/`CanHandleAsset`/`GetPriority`, sets `OutData.AssetType` (the systemic bug), logs via base. ActorComponent dropped its duplicated Blueprint/graph stack (733 → 76 lines, M2.2-style). All three re-enabled in `BPR_Core.cpp`. Material graphs (Expression DAG) kept as-is — not shared with Object. Phase 3 (generic `BPR_Extractor_Blueprint` fallback) optional/deferred.  
**M2 progress:** M2.1 ✅ (Widget inherits `BPR_Extractor_Object`), M2.6 ✅ (WidgetAdapter removed), M2.2 ✅ (graph/variable dup removed, ~730 lines), M2.3 ✅ (event→handler bindings via `AppendWidgetEventBindings`/`K2Node_ComponentBoundEvent`, already wired; dead old `AppendWidgetBindings` removed), M2.5 ✅ (Widget logging now via inherited base → `LogBlueprintReader`). Widget.cpp ~3445 → 2660 lines. All builds verified on UE 5.7.
**M2.4 ✅ (editor-validated):** Custom `WBP_*` (UUserWidget) extraction. Nested WBP instances have null `WidgetTree` at asset level, so `ProcessWidgetHierarchy`'s recursion (~line 248) doesn't fire. Approach A in `HandleUnknownWidget` (~line 1020): for a `UUserWidget`, iterate its own exposed user variables (`IsUserVariable`) and print non-default instance values (`GetPropertyDefaultValue(Prop, Widget)`) under "Exposed Properties (set on this instance)". Validated: nested labels now surface their actual text (e.g. `In Text: Clan`) and config (`MaxCharToEnter`, ...) — the per-instance values an LLM needs.

**M2, M3 & M4 are complete and merged to `main`.** Current milestone: **M6** (UE 5.7+5.8 compatibility) — see above. (M5 — CI/smoke/Fab — comes after or in parallel with M6's CI-matrix work.)

**Bugfixes (editor-validated):** `ProcessWidget` now sets `OutData.AssetType = EAssetType::Widget` — was unset (Unknown=0), so the UI `RebuildTabsFromData()` switch fell through to "unsupported, type 0" for every widget. `BPR_Extractor_Interface` now sets `InterfaceBP` (was `Blueprint`, which has no switch case → same "unsupported" symptom). Duplicate Design title header removed.

**Follow-ups:** `CleanName` base-parity dedup (only widget-local duplicate left); module-wide `LogTemp`→`LogBlueprintReader` in non-Widget files (UI, Material, ActorComponent); in Design output, resolve brush/image `ResourceObject` to an asset name instead of a raw GUID, and suppress all-zero/transparent color values like `BackgroundColor:(0,0,0,0)` (M4 output-quality).

## Working rules

- **Git commits/push — user only.** Never commit or push autonomously.
- Read current files before editing; propose a plan for significant changes.
- After a logical group of edits, suggest a build check.
- Prefer minimal, safe changes. Quality over speed.
- **This file is the durable source of truth.** `.grok/Private/` is NOT git-tracked and NOT covered by the user's project backup tooling — its prior contents (`ROADMAP.md`, `NOTES.md`, `CURRENT_PLAN.md`, `ARCHITECTURE.md`, etc.) were lost on 2026-07-01 during a backup/migration operation and were deliberately not reconstructed (user call: CLAUDE.md is now the single source of truth going forward). Only `.grok/Private/M6_MIGRATION_PLAN.md` currently exists there (detailed RU reference for the active M6 milestone) — treat anything placed in `.grok/Private/` as disposable; durable facts belong in this file.
