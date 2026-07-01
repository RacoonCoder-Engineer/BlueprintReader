# Roadmap — BlueprintReader

> Milestone numbering (`M1`–`M6`) follows the plugin's development history and matches the notation used in the project's technical log (`CLAUDE.md`). Statuses are current as of version `0.2.0` (beta).

## Completed Milestones

**M1 — Extractor factory and stable build.** *Done.*
The plugin's base architecture was established: `BPR_Core` acts as a factory that owns and registers all extractors (`RegisterAllExtractors`, `IsSupportedAsset`, `ExtractAsset`, `GetUnsupportedAssetInfo`), while `BPR_Types.h` defines a single data contract (`FBPR_ExtractedData` with the fields `Structure`, `Graph`, `Design`). A base class hierarchy was set up: `BPR_Extractor_Base` → `BPR_Extractor_Object` → `Actor` / `ActorComponent` / `Widget`, plus the independent branches `Interface`, `Enum` / `Structure`, `Material`, `MaterialFunction`. By the end of this milestone, the plugin build was brought to a clean state on the new structure.

**M2 — Widget as a full-fledged descendant of `BPR_Extractor_Object`.** *Done, validated in the editor.*
The widget extractor was moved from a standalone implementation onto the shared hierarchy, which eliminated duplicated logic and noticeably reduced the amount of code:

- Widget now inherits `BPR_Extractor_Object` instead of using a separate extraction path.
- The obsolete `WidgetAdapter` shim was removed.
- Duplicated graph- and variable-extraction logic shared with the base class was removed (~730 lines of code).
- Event → handler bindings (`AppendWidgetEventBindings` / `K2Node_ComponentBoundEvent`) were switched to a unified mechanism; the unused old `AppendWidgetBindings` was removed.
- Widget logging was switched to the inherited base category `LogBlueprintReader` instead of scattered ad-hoc logging.
- Separately, the problem of extracting nested custom widgets (`WBP_*`, `UUserWidget`) was solved: for nested instances, `WidgetTree` at the asset level is `null`, so the usual tree recursion (`ProcessWidgetHierarchy`) didn't fire. The solution ("Approach A" in `HandleUnknownWidget`) is to iterate the widget's own exposed user variables (`IsUserVariable`) and print their non-default instance values (`GetPropertyDefaultValue`) in a separate "Exposed Properties (set on this instance)" block. Manually validated: nested text labels and parameters (e.g., `In Text: Clan`, `MaxCharToEnter`) now correctly show the actual values of the specific instance.

Outcome of this milestone: `BPR_Extractor_Widget.cpp` shrank from roughly 3445 to 2660 lines. All builds verified on UE 5.7.

**M3 — Material / MaterialFunction / ActorComponent in the extractor hierarchy.** *Done, validated in the editor, merged into `main`.*
Three extractors that previously existed in isolation were folded into the shared hierarchy with correct priorities: `Material` → `BPR_Extractor_Base` (priority 40), `MaterialFunction` → `BPR_Extractor_Base` (priority 30), `ActorComponent` → `BPR_Extractor_Object` (priority 90). Each extractor implements the full contract (`Process` / `CanHandleAsset` / `GetPriority`) and — importantly — now correctly sets `OutData.AssetType`, whereas previously this field was left unset for all three types (a systemic bug that caused the interface to treat these assets as unsupported). The `ActorComponent` extractor was relieved of its duplicated Blueprint-graph extraction stack, shrinking it from 733 to 76 lines — following the same pattern as the M2.2 refactor. The Material Expression DAG walk was deliberately left unchanged and not unified with the `Object` branch — this is the riskiest piece of code, and it is separately flagged for review during future engine-version work. The optional "Phase 3" (a generic fallback extractor `BPR_Extractor_Blueprint` for arbitrary Blueprint classes with no more specific match) was deemed non-essential and was not part of M3's scope.

**M4 — UX: plugin settings and file export.** *Done, validated in the editor, merged into `main`.*
This milestone was limited to two sub-items (4.1 and 4.2); one item was dropped from the plan, and two were deliberately deferred (see the "Deferred and Optional Items" section).

- **4.1 — Plugin settings.** Added `UBPR_Settings : UDeveloperSettings` (`Public/Core/BPR_Settings.h`, `config=Editor`), available under Project Settings → Plugins → Blueprint Reader. It exposes the export format (`EBPR_ExportFormat`: Markdown/PlainText), a flag to restrict the widget-tree recursion depth (`bRestrictWidgetRecursionDepth`), and the depth itself (`WidgetRecursionDepth`, range 1–50). `ProcessWidget` now reads these settings via `SetRecursionSettings()` instead of the previously hardcoded `MaxDepth=10`. A technically significant fact: this is the first `UObject` in the module, which meant UnrealHeaderTool (UHT) ran for the plugin for the first time.
- **4.2 — Export to file.** An `FString AssetName` field was added to `FBPR_ExtractedData`, populated centrally in `BPR_Core::ExtractAsset`. An "Export to file…" button was added to the footer of `SBPR_TabSwitcher` (which survives tab rebuilds), invoking `IDesktopPlatform::SaveFileDialog` (default file name is the asset name, extension taken from the export-format setting), after which the data is written via `FFileHelper` in UTF-8 with a success/failure notification. Along the way, a UE 5.7 version quirk was resolved: `FindBestParentWindowHandleForDialogs` returns `const void*`.

**M6 — Compatibility with UE 5.7 and 5.8.** *Technical portion complete (see details in the "Current Milestone" section — the milestone's process loose ends are still open).*

## Current Milestone

The technical work for **M6 (UE 5.7 and 5.8 compatibility)** is, at this point, effectively complete, but the milestone formally remains open until associated process tasks are closed out, some of which overlap with M5. Detailed status below.

**Approach.** The chosen strategy is "A+C": a single source tree (not separate branches per engine version — that option was rejected due to the risk of code drift) with version forks via `UE_VERSION_OLDER_THAN`, centralized in the `BPR_Compat.h` header. The reliability gate is a CI build matrix for 5.7 and 5.8 simultaneously (shared with task M5.2).

**Phase 1 result.** The plugin builds without errors on both UE 5.7 and UE 5.8; meanwhile `BPR_Compat.h` remains **empty** — in the end, not a single version fork (`#if`) was needed. The first attempt to build via `RunUAT BuildPlugin` against 5.8 ran into not a code problem but a toolchain artifact (path length exceeded 260 characters, triggering `CheckPathLengths`) — resolved by using a shorter packaging path. The one real finding that required a code fix: the header `Engine/UserDefinedStruct.h` (used in `BPR_Extractor_Structure.h`/`.cpp`) was removed as a forwarding header in 5.8 — fixed for free, with no version fork, by switching to the canonical path `StructUtils/UserDefinedStruct.h` (CoreUObject module), which is unchanged in both engine versions. Notably, the Material Expression DAG walk — the piece of code flagged in advance as the riskiest — compiled on 5.8 without a single change, only with noisy engine-header warnings. A re-check confirmed that the 5.7 build remains error-free after this fix, with no regressions introduced.

**Task 6.0 — two confirmed free fixes (no `#if`):**

- **(a)** Direct reads of the `USizeBox::bOverride_*` fields in `BPR_Extractor_Widget.cpp` (`HandleSizeBoxProperties`, ~lines 2068–2084) were replaced with reflection via `FindFProperty<FBoolProperty>`, following the pattern already used by `AppendBoolFlag` for ListView/TreeView in the same file (~line 2263).
- **(b)** The unused dependencies `StateTreeModule` / `GameplayStateTreeModule` / `StateTreeEditorModule` were removed from `BlueprintReader.Build.cs` — vestigial modules that created a risk of a linker error on a UE 5.8 host without the StateTree plugin enabled.

Current status: both items are confirmed as done and present in the source code.

**Task 6.3 — runtime smoke-diff.** Performed manually: the user compared the plugin's output on real assets between the 5.7 and 5.8 editors — the results match, with no discrepancies found in the extracted data.

**Evolution of the development environment.** Initially, a live 5.7 host (`PluginsProject`) and its frozen backup copy (`PluginsProject57`) were maintained. After the technical part of M6 was completed, the main host `PluginsProject` was recreated from scratch on UE 5.8 and is confirmed to build cleanly as a full editor module; `PluginsProject57` has been kept as a reference host for rolling back to 5.7 on demand (an ordinary git checkout of the same repository) — also confirmed to build without errors. So the current topology is: `PluginsProject` — the primary development host on 5.8, `PluginsProject57` — the backup host for verifying 5.7 compatibility.

**What remains within M6 (process, not technical, tasks):**

- Re-verify the actual application of item 6.0(a).
- Roll out the 5.7+5.8 CI build matrix as an automated pipeline (a milestone shared with M5).
- Prepare Fab packaging separately for each engine version (see below, overlaps with M5).

## Planned Milestones

**M5 — CI, smoke tests, v0.2 release on Fab.** *Not yet started as standalone work; part of the related tasks have already been closed out as part of M6.*

As an independent stream of work, M5 has not yet started, but some of its logical prerequisites have already been fulfilled in passing while working on M6 (in particular, the manual runtime smoke-diff from task 6.3). What remains open specifically for M5:

- **CI build matrix (5.7 + 5.8).** Currently, builds against both engine versions are only checked manually and locally; an automated pipeline is needed, which will serve as a shared reliability gate with M6.
- **Formal smoke tests.** The manual check (M6.3) was a one-off and was not formalized into a repeatable test suite — formalization is required.
- **Preparing and packaging the v0.2 release for Fab**, including the mechanism for packaging separately per engine version: the `EngineVersion` field in `.uplugin` can only hold a single version, so Fab's actual mechanism is a separate uploadable "Project Version" for each supported engine version, each with its own copy of `.uplugin` in which `EngineVersion` is stamped specifically for it (5.7.0 / 5.8.0) at the packaging step itself. This is a separate phase of the migration plan (Phase 4), which has not yet been carried out.

The plugin version was deliberately not bumped to `1.0.0` — that step is reserved for the point when M5 (CI, smoke tests, release QA) is complete; until then, the plugin manifest keeps `IsBetaVersion: true`.

## Deferred and Optional Items

Listed below are items for which a deliberate decision was made to defer them or exclude them from the scope of completed milestones. These are not forgotten items, but a recorded backlog.

- **M4.3 — Partial graph filter.** *Deferred.* The ability to selectively limit the scope of the extracted graph (e.g., only certain functions/events) was not included in M4's scope and has been pushed to the future without being tied to a specific milestone.
- **M4.4 — Progress indicator.** *Deferred.* A UI indicator for the extraction process on large assets was not implemented in M4; it remains an open backlog item.
- **M4.5 — README links (Patreon/Boosty).** *Dropped from the plan, not merely deferred.* Monetization plans via Patreon/Boosty are on hold; the corresponding links were not added and are not planned within the current horizon.
- **Extractor Phase 3 (M3) — generic `BPR_Extractor_Blueprint`.** *Optional, not planned.* A universal fallback extractor for arbitrary Blueprint classes that don't fall under any specialized type (Actor/ActorComponent/Widget/Interface) was deemed a non-essential enhancement rather than a requirement of the current architecture.
- **Other open output-quality improvements (not tied to a specific milestone):**
  - Deduplicating `CleanName` at the base-class level (currently only a local duplicate remains in the Widget extractor).
  - A module-wide switch from `LogTemp` to the dedicated `LogBlueprintReader` category in files outside the Widget extractor (UI, Material, ActorComponent).
  - In the Design tab output, resolving brush/image `ResourceObject` to a human-readable asset name instead of a raw GUID.
  - Suppressing fully zero/transparent color values in the Design output (e.g., `BackgroundColor:(0,0,0,0)`), which carry no useful information for an LLM.

These items do not block the v0.2 release and are considered material for subsequent iterations after M5/M6 have stabilized.
