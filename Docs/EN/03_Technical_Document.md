# Technical Document — BlueprintReader

## Module Architecture

BlueprintReader is built as a classic three-layer pipeline: **UI (Slate) → `BPR_Core` (factory) → extractor hierarchy**, with all three layers connected by a single data contract defined in `BPR_Types.h`. This separation of responsibilities is deliberately simple and free of unnecessary abstractions — the plugin does not try to be a universal framework; it solves one task (turning an arbitrary Blueprint asset into Markdown text for an LLM), and the code structure directly reflects that.

The **UI layer** contains no data-extraction business logic whatsoever. Its sole concern is to show the user what `BPR_Core` returned and to pass the selected asset down the pipeline. The entry point is a Content Browser context-menu item, followed by non-modal Slate windows (`BPR_InfoWindow` for unsupported assets, `BPR_OutputWindow`/`SBPR_TabSwitcher` for the extraction result); both are created via `FSlateApplication::AddWindow` rather than `AddModalWindow`, so they do not block the rest of the editor.

**`BPR_Core`** is a plain C++ class (`BLUEPRINTREADER_API BPR_Core`, `Public/Core/BPR_Core.h:20`), deliberately not a `UObject`. It holds no scene state and does not participate in engine reflection — it is a pure factory/service object at module scope, instantiated once when the plugin loads (`BlueprintReader.cpp`, module lifecycle). `BPR_Core` owns the array of extractors (`TArray<TSharedPtr<BPR_Extractor_Base>> Extractors`, `BPR_Core.h:47`) and exposes four public methods:

- `RegisterAllExtractors()` — constructs one instance of each of the 8 concrete extractors and sorts the array in descending order of priority;
- `IsSupportedAsset(UObject*) const` — a quick check of "is there even an extractor for this asset," used by the UI before the result window is opened;
- `ExtractAsset(UObject*, FBPR_ExtractedData&)` — the actual extraction;
- `GetUnsupportedAssetInfo() const` — data for the popup window shown when no suitable extractor was found.

The private method `FindSuitableExtractor(UObject*) const` (`BPR_Core.h:44`) encapsulates the extractor-selection algorithm itself and does not leak outward — all other code works through the two public methods (`IsSupportedAsset`, `ExtractAsset`), which gives freedom to change the internal selection algorithm without consequences for the UI.

The **extractor hierarchy** is a classic inheritance tree rooted at the common base `BPR_Extractor_Base` (also plain C++, not a `UObject`), with an intermediate layer, `BPR_Extractor_Object`, into which all logic common to "regular" Blueprint classes (variables, graphs, component structure) is factored out. Extractors whose internal structure fundamentally differs from a Blueprint class (interfaces, enums, structures, materials) inherit from `BPR_Extractor_Base` directly, bypassing `BPR_Extractor_Object` — this is a deliberate architectural decision, not an oversight (see the explicit comment in `BPR_Extractor_Interface.h`: "Inherits directly from BPR_Extractor_Base because interfaces have significantly different structure than regular Blueprint Classes"). Details of the hierarchy and priorities are in a separate section below.

This architecture gives the plugin two key properties: first, adding support for a new asset type means creating a new subclass and adding a single registration line in `BPR_Core::RegisterAllExtractors()`, without modifying existing code (open for extension, closed for modification); second, the UI is fully decoupled from extraction details — it works with a single data type, `FBPR_ExtractedData`, regardless of whether the asset is a Widget, an Actor, or a Material.

## Data Contract

The data contract lives in a single file — `Public/Core/BPR_Types.h` — and is the only point of contact between the extractor layer and the UI layer. This centralization is not accidental: any change to the output format goes through one header that both layers include, which makes cross-layer incompatibility practically impossible at compile time.

The central structure is **`FBPR_ExtractedData`** (`BPR_Types.h:41-48`):

```cpp
struct FBPR_ExtractedData
{
    FText Structure = FText::FromString(TEXT("No Data found"));
    FText Graph     = FText::FromString(TEXT("No Data found"));
    FText Design    = FText::FromString(TEXT("No Data found"));
    EAssetType AssetType = EAssetType::Unknown;
    FString AssetName;   // M4.2: source asset name, used as the default file name on export
};
```

The three text "tab" fields (`Structure`, `Graph`, `Design`) are already fully-formed, Markdown-formatted text (by default in `EOutputFormat::Minimal` format, see below), which the UI simply drops into a text widget with no further processing. All three fields share the same default value — the string `"No Data found"` — which is both a safe default in case an extractor forgot to fill in a field, and a recognizable "empty" marker that the file-export logic relies on (see the section on the UI layer): a section with this content does not make it into the exported document.

`EAssetType` (`BPR_Types.h:12-24`, `enum class : uint8`) is a tag the UI uses to decide which tabs to show and how to label them:

```
Unknown, Blueprint, Actor, Widget, Material, MaterialFunction, ActorComponent, Enum, Structure, InterfaceBP
```

An important detail for anyone reading or extending the code: the value `Blueprint` is present in the enum, but **as of today no registered extractor ever assigns it** — the system has no universal extractor for "just a Blueprint class with no specific parent" (see Phase 3 / `BPR_Extractor_Blueprint` in the hierarchy section). Historically, this same field was tied to a bug: `BPR_Extractor_Interface` mistakenly assigned `AssetType::Blueprint` instead of `InterfaceBP`, causing the UI to fail to find a matching `case` in its `switch` and fall back to the "Info" fallback tab — the bug has been fixed, but the fact itself underlines just how tightly `EAssetType` and the tab switch in `SBPR_TabSwitcher` are coupled: a mismatch between what an extractor assigns and what the UI expects is the one class of error to watch out for when working on this field.

The `AssetName` field (`FString`, no default value, i.e. an empty string) was added in M4.2 and is populated not by the extractor but centrally, at the very end of `BPR_Core::ExtractAsset` (`BPR_Core.cpp:100`, `OutData.AssetName = Asset->GetName()`). This design relieves each of the 8 extractors of the responsibility of remembering this field and guarantees it will be correctly filled for any supported asset type — the only point where this field is not populated is the early `return` in `ExtractAsset` when `Asset == nullptr` (see below).

The helper structure **`FUnsupportedAssetInfo`** (`BPR_Types.h:29-36`) serves the opposite scenario — when an asset is not supported at all:

```cpp
struct FUnsupportedAssetInfo
{
    FText Title;
    FText MainMessage;
    FText SubMessage;
    FString GitHubURL;
    FText ButtonText;
};
```

It is returned from `BPR_Core::GetUnsupportedAssetInfo()` and is unpacked directly into the parameters of `BPR_InfoWindow` — that is, it is essentially a ViewModel for one specific non-modal window, not part of the main extraction pipeline.

Finally, **`EOutputFormat`** (`BPR_Types.h:50-55`, `enum class : uint8`: `HumanReadable`, `Compact`, `Minimal`) is an internal, code-level (not a UPROPERTY, not user-configurable) switch controlling formatting density, used via `BPR_Extractor_Base::GetOutputFormat()`/`SetOutputFormat()` (default value is `Minimal`, `BPR_Extractor_Base.h:102`). It is important not to confuse it with the user-facing setting `EBPR_ExportFormat` (Markdown/PlainText) from `BPR_Settings.h` — they solve different problems: `EOutputFormat` controls how "densely" an extractor formats text inside `Structure`/`Graph`/`Design` (aimed at saving LLM tokens), while `EBPR_ExportFormat` controls solely the file extension and file type when exporting to disk.

## Extractor Hierarchy and Priorities

All extractors implement a single three-method contract: `Process()`, `CanHandleAsset()`, `GetPriority()`. In addition, the base class has a bridge method, `Extract()`, which by default simply calls `Process()` — the one exception to this rule is `BPR_Extractor_Widget`, which overrides `Extract()` directly (`BPR_Extractor_Widget.h:98`) rather than relying on the base class's bridge, due to the specifics of building the widget tree.

The inheritance tree (all classes are plain C++, not `UObject`):

- **`BPR_Extractor_Base`** (`Public/Extractors/BPR_Extractor_Base.h:18`) — root of the hierarchy, shared logging infrastructure (`LogBlueprintReader`) and `EOutputFormat`.
  - **`BPR_Extractor_Object`** (`BPR_Extractor_Object.h:30`) — adds logic common to all Blueprint classes: variables, graphs, class structure.
    - **`BPR_Extractor_Actor`** — priority **100**
    - **`BPR_Extractor_ActorComponent`** — priority **90**
    - **`BPR_Extractor_Widget`** — priority **80**
  - **`BPR_Extractor_Interface`** — priority **70** (directly from `Base`)
  - **`BPR_Extractor_Enum`** — priority **50** (directly from `Base`)
  - **`BPR_Extractor_Structure`** — priority **50** (directly from `Base`)
  - **`BPR_Extractor_Material`** — priority **40** (directly from `Base`)
  - **`BPR_Extractor_MaterialFunction`** — priority **30** (directly from `Base`)

Registration of all eight happens in `BPR_Core::RegisterAllExtractors()` (`BPR_Core.cpp:34-63`): one `TSharedPtr` of each concrete class is created, the array is populated, then sorted in descending order of `GetPriority()`. There is no universal "Blueprint fallback" in the system (`BPR_Extractor_Blueprint`, which would pick up any `UBlueprint` not matched by any of the specialized extractors) — this idea is recorded in the code as a commented-out future Phase 3 and is deliberately outside the scope of any completed milestone so far.

Selecting an extractor for a given asset is done by `FindSuitableExtractor` (`BPR_Core.cpp:103-115`): the method walks the priority-sorted array and returns the first extractor whose `CanHandleAsset(Asset)` returned `true`. Priority here works precisely as a mechanism for resolving ambiguity between potentially overlapping checks, not as an arbitrary label — for instance, any `AActor` subclass and any `UActorComponent` subclass are both `UBlueprint`, so the check order (Actor before ActorComponent before Widget) is critical for correctness.

Table of exact `CanHandleAsset()` conditions:

| Extractor | Priority | Base class | `CanHandleAsset()` condition |
|---|---|---|---|
| `BPR_Extractor_Actor` | 100 | `BPR_Extractor_Object` | `Cast<UBlueprint>(Asset)` and `Blueprint->GeneratedClass->IsChildOf(AActor::StaticClass())` |
| `BPR_Extractor_ActorComponent` | 90 | `BPR_Extractor_Object` | `Cast<UBlueprint>(Asset)` and `GeneratedClass->IsChildOf(UActorComponent::StaticClass())` |
| `BPR_Extractor_Widget` | 80 | `BPR_Extractor_Object` | `Cast<UWidgetBlueprint>(Asset) != nullptr` |
| `BPR_Extractor_Interface` | 70 | `BPR_Extractor_Base` | `Cast<UBlueprint>(Asset)` and `BP->BlueprintType == BPTYPE_Interface` |
| `BPR_Extractor_Enum` | 50 | `BPR_Extractor_Base` | `Cast<UUserDefinedEnum>(Asset) != nullptr` |
| `BPR_Extractor_Structure` | 50 | `BPR_Extractor_Base` | `Asset->GetClass()->GetName() == TEXT("UserDefinedStruct")` (comparison by class name, not `Cast`) |
| `BPR_Extractor_Material` | 40 | `BPR_Extractor_Base` | `Cast<UMaterial>(Asset) != nullptr \|\| Cast<UMaterialInstance>(Asset) != nullptr` |
| `BPR_Extractor_MaterialFunction` | 30 | `BPR_Extractor_Base` | `Cast<UMaterialFunction>(Asset) != nullptr \|\| Cast<UMaterialFunctionInstance>(Asset) != nullptr` |

One nuance is worth flagging separately, important for future work: **`Enum` and `Structure` share the same priority — 50**. Their `CanHandleAsset()` conditions are mutually exclusive (a type check against `UUserDefinedEnum` versus a class-name comparison against the string `"UserDefinedStruct"`), so in practice no ambiguous selection ever arises. Nevertheless, the relative order of these two extractors with respect to each other after `Extractors.Sort()` is not guaranteed — `TArray::Sort` is not documented as a stable sort, and the comparator treats equal priorities as equivalent. If a third extractor with priority 50 and an overlapping `CanHandleAsset()` condition is ever added, this is the one place in the architecture where priorities will need to be explicitly separated to make the behavior deterministic.

`BPR_Extractor_Structure` deserves separate mention for another reason as well: it is the only extractor that determines the asset type not via `Cast<T>`, but via a string comparison of the class name. The reason is pragmatic — `UUserDefinedStruct` is not always available as a clean C++ type across all engine build configurations, and comparing by class name turned out to be a more reliable solution than a direct cast. As real-world experience migrating to UE 5.8 showed (see the "Engine Version Compatibility" section), even this type's own header file changed location between engine versions — which further confirms the wisdom of decoupling the recognition logic from a specific include path.

## UI Layer (Slate)

The entry point into the plugin is a Content Browser context-menu item, added via `FBPR_ContentBrowserAssetActions::Register()` (`Private/UI/BPR_ContentBrowserAssetActions.cpp:13-27`). The registration extends the `"GetAssetActions"` section of the `"ContentBrowser.AssetContextMenu"` menu and adds an entry with the identifier `"ShowBPAsMD"`, the visible text **"Read Blueprint for AI-Assistant"**, and the tooltip "Convert Blueprint to Markdown via BPR Core".

The click handler, `OnShowBPAsMDClicked()`, retrieves the selected asset via `ContentBrowserModule` and passes it to `ExecuteForObject(SelectedObject)` (`BPR_ContentBrowserAssetActions.cpp:71-134`) — this is the central orchestrator function of the entire UI layer, and its algorithm is linear:

1. Ask `Core->IsSupportedAsset(SelectedObject)` whether the asset is supported at all.
2. If not — build `BPR_InfoWindow::FParams` from `Core->GetUnsupportedAssetInfo()` and open (or, if already open, bring to front) the `BPR_InfoWindow` window.
3. If yes — open `BPR_OutputWindow`, get its `TabSwitcher`, call `Core->ExtractAsset(SelectedObject, Data)`, and pass the result via `TabSwitcher->SetData(Data)`.

**`BPR_InfoWindow`** (`Public/UI/BPR_InfoWindow.h`, `Private/UI/BPR_InfoWindow.cpp`) is a deliberately simple plain C++ class (not a subclass of `SWindow`/`SCompoundWidget`), holding a `TWeakPtr<SWindow>` to the created window. This ownership approach — via `TWeakPtr` plus an "already open → `BringToFront()`" check — prevents duplicate windows when the user repeatedly clicks on an unsupported asset. The window has a fixed size of 600×300, is not maximizable, not minimizable, is topmost, and contains: large yellow main-message text (font size 16), a white word-wrapped subtitle (font size 14), a "Check for updates" button (visible only if a non-empty URL is supplied — opens the link via `FPlatformProcess::LaunchURL`), a static gray hint line, and an "OK" button that closes the window via `RequestDestroyWindow()`. For the unsupported-asset scenario, all of these fields are populated from `BPR_Core::GetUnsupportedAssetInfo()` — a fixed set of texts hardcoded in `BPR_Core.cpp` (Title "Blueprint Reader", MainMessage "Warning! This asset is not supported yet!", etc., including a link to the project's GitHub repository).

**`SBPR_TabSwitcher`** is the workhorse of the result window, responsible for building the set of tabs for a specific `EAssetType`. The method `RebuildTabsFromData()` (`Private/UI/BPR_TabSwitcher.cpp:77-138`) implements a `switch` on the asset type:

- **`Widget`** → 3 tabs, in the order **Design, Structure, Graph** (Design is first and the default tab for widgets, which makes sense: for a UMG widget, the visual layout is usually more important for quick understanding than the event graph).
- **`Actor`, `ActorComponent`, `Material`, `MaterialFunction`, `InterfaceBP`** → 2 tabs: **Structure, Graph**.
- **`Enum`, `Structure`** → 1 tab: **Structure** (these asset types by definition have neither an execution graph nor a visual design).
- **`default`** (covers `Unknown` and the unused value `Blueprint`) → 1 **Info** tab, built via `CreateErrorTabContent()`, which outputs Russian-language text ("### Blueprint ещё не поддерживается..." — "Blueprint is not yet supported...") along with the numeric value of `EAssetType`. This is a defensive, secondary path — the primary UX for unsupported assets is implemented earlier, via the `BPR_InfoWindow` popup, before the result window is even opened; the `default` branch in `SBPR_TabSwitcher` is a safety net in case `IsSupportedAsset()` and the actual contents of the `switch` fall out of sync.

Starting with M4.2, the footer of `SBPR_TabSwitcher` (bottom-right corner, `BPR_TabSwitcher.cpp:43-52`, and re-added identically after every tab rebuild at lines 166-176, so it survives `ClearTabs()`) contains a persistent **"Export to file…"** button with the tooltip "Save the extracted data to a .md/.txt file (format from plugin settings)". The export logic is described in detail in the context of the plugin settings (`UBPR_Settings`) and is not duplicated here — from a UI architecture standpoint, what matters is that the button lives outside the lifecycle of any particular tab set and survives an asset change.

## Engine Version Compatibility

The compatibility strategy for UE 5.7/5.8 adopted in M6 is referred to in the project documentation as the **"A+C"** approach: a single source tree (not separate branches per engine version — the separate-branches option was considered and rejected due to the risk of code drift between versions over time) plus version guards via the `UE_VERSION_OLDER_THAN` macro, centralized in a single header — `Public/Core/BPR_Compat.h`.

The rules for using this header are recorded directly in its comments and are mandatory for any future compatibility work:

- **No other plugin file uses `UE_VERSION_*`/`ENGINE_*_VERSION` directly.** All version branching is pulled into `BPR_Compat.h` — the single place to look for and maintain such code.
- **Branching is always "from the older version":** `#if UE_VERSION_OLDER_THAN(5, 8, 0) ... #else ... #endif` — the engine has no inverse macro `UE_VERSION_NEWER_OR_EQUAL_THAN`, so the condition is always phrased as a negation, "version older than X."
- **A shim is added only reactively** — after a real compilation error on UE 5.8 has proven its necessity. Speculative guards on code points that "might" change but currently compile identically on both versions are forbidden.

As of the time of writing this document, `BPR_Compat.h` contains only the declaration of the `BPR::Compat` namespace, which is **empty** — not a single `#if` guard has been needed. This is not unfinished work but the actual outcome of the first real attempt to compile the plugin under UE 5.8: the overwhelming majority of code points originally classified as "needs verification" (first and foremost, the material expression DAG walk, duplicated between `BPR_Extractor_Material` and `BPR_Extractor_MaterialFunction`, pre-flagged as the highest-risk code) compiled without a single change, with only noisy engine warnings.

The one real finding from the first 5.8 compilation concerned not a version guard but a direct include replacement: the header `Engine/UserDefinedStruct.h`, used by `BPR_Extractor_Structure.h`/`.cpp`, was removed as a forwarding header in UE 5.8. The canonical path — `StructUtils/UserDefinedStruct.h` (module `CoreUObject`) — exists unchanged in both engine versions, so the fix came down to replacing the `#include` line and required no `#if`/`#else` at all: this is a "free" fix in M6 terminology (works identically on 5.7 and 5.8, no branching logic needed).

Beyond this single `#include`, two more targeted fixes were made as part of task 6.0, also requiring no version guards:

- In `BPR_Extractor_Widget.cpp` (`HandleSizeBoxProperties`), direct reads of the `USizeBox::bOverride_*` fields were replaced with reflection-based reads (`FindFProperty<FBoolProperty>` plus retrieving the value by field name), mirroring the existing `AppendBoolFlag` pattern already used for ListView/TreeView in the same file. Direct access to these fields relied on an unstable implementation detail of `USizeBox` flagged as subject to change, so the move to reflection eliminates the risk of a silent break in future engine versions rather than being a reaction to a specific compilation error.
- The unused dependencies `StateTreeModule`, `GameplayStateTreeModule`, `StateTreeEditorModule` were removed from `BlueprintReader.Build.cs` — they were not used anywhere in the plugin's codebase and posed a linking risk on a UE 5.8 host where the StateTree plugin is not enabled.

The final state as of the completion of M6's technical portion: the plugin builds without a single error on both engine versions (5.7 and 5.8), the `.uplugin` file contains `EngineVersion: "5.7.0"` as the "floor" version for local development, and `Description` explicitly states the support model: "Single-source, supports Unreal Engine 5.7 and 5.8 (EngineVersion managed at packaging time)." For actual publication on Fab, this additionally means the need for separate packaging per engine version (Fab requires a single `EngineVersion` value per "Project Version" in a listing) — this is a separate, not-yet-completed stage as of this document (Phase 4 of the migration plan), which does not affect the structure of the source code itself.

## Build and Dependencies

The build is described in `BlueprintReader.Build.cs` (`public class BlueprintReader : ModuleRules`). The module uses `PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs` and explicitly disables Unity builds (`bUseUnity = false`) — the latter is a second deliberate decision for a plugin of this size: disabling Unity builds increases full compile time but simplifies diagnosing compilation errors in individual extractor files during active development and engine-version migration.

**`PublicDependencyModuleNames`** is kept to a minimum — just `["Core"]` (a code comment states the rule directly: "Public is always only Core"). This means that no external module that might potentially depend on `BlueprintReader` transitively inherits anything beyond `Core` — the module's entire functional weight is encapsulated as private dependencies, which is correct for a tooling plugin that does not expose its own public API to other modules.

**`PrivateDependencyModuleNames`** (shared across all targets): `Core`, `CoreUObject`, `Engine`, `UnrealEd`, `Slate`, `SlateCore`, `UMG`, `UMGEditor`, `DeveloperSettings`. The last one was added in M4.1 specifically for `UBPR_Settings : UDeveloperSettings` — the first `UObject` class in the module, which is why UnrealHeaderTool (UHT) started running for the plugin, something worth keeping in mind when adding new `UCLASS`/`USTRUCT` types in the future (requirements around `GENERATED_BODY()`, include order, etc. only became relevant from this point in the project's history onward).

For editor targets (`if (Target.Type == TargetType.Editor)`), a separate block of dependencies is added: `UnrealEd`, `ContentBrowser`, `AssetTools`, `ToolMenus`, `BlueprintGraph`, `GraphEditor`, `KismetCompiler`, `WorkspaceMenuStructure`, `UMG`, `EditorStyle`, `DesktopPlatform`. The last one, `DesktopPlatform`, was added in M4.2 for the native `SaveFileDialog` used by the export-to-file feature. It is explicitly noted that this list no longer contains `StateTreeModule`/`GameplayStateTreeModule`/`StateTreeEditorModule` — they were removed as part of task M6.0(b) as unused baggage posing a linking risk on a "clean" UE 5.8 host (see the section on engine version compatibility).

Plugin metadata in `BlueprintReader.uplugin`: a single module, `"BlueprintReader"`, `Type: "Editor"` (the plugin only works in the editor, it does not participate in the runtime of a packaged game), `LoadingPhase: "Default"`, `PlatformAllowList: ["Win64", "Mac", "Linux"]`. The version as of this document's writing is `VersionName: "0.2.0"` (`Version` (integer): 2, `FileVersion`: 3), deliberately not `1.0.0` — per the versioning semantics adopted in the project, the move to major version 1.0 is reserved for the completion of milestone M5 (CI, smoke tests, final release on Fab), and the `IsBetaVersion: true` field reflects precisely this state of QA-process incompleteness, not incompleteness of the technical core of the plugin.

## Testing and Validation

There is no formal automated test suite (C++ unit tests, engine automation tests) in the plugin as of this document's writing — this is an open item of milestone M5. Nevertheless, functional validation is performed, and several genuinely different, practically applied mechanisms should be distinguished.

**Manual editor validation per milestone.** Each of milestones M2–M4 is marked in the project history as "editor-validated": after implementing changes, the plugin was launched in a real UE editor, and actual output was checked on real assets (including nested `WBP_*` widgets with non-empty user variables) — for example, M2.4 validation confirmed that nested labels and configuration fields (`In Text: Clan`, `MaxCharToEnter`, etc.) do actually show the current instance values rather than the class defaults. This is the primary, most substantively exercised means of correctness checking to date.

**Compile validation as a compatibility-problem discovery mechanism.** In the absence of a public list of breaking changes between UE 5.7 and 5.8 for the C++ API from Epic Games, actually compiling the plugin under both engine versions became the primary tool for *discovering* migration problems, rather than merely confirming their absence. In practice this means: instead of guessing in advance which parts of the code might break on 5.8 and wrapping them in speculative `#if` guards, the team first tried compiling the existing code as-is under 5.8 (including via a direct call to `RunUAT BuildPlugin` using the 5.8 engine's own tools, with no changes to the host project — the fastest signal about compilability) and only then made targeted fixes based on actual compilation errors. This approach proved effective: the one genuine finding — the stale include path `Engine/UserDefinedStruct.h` — was discovered exactly this way, rather than anticipated in advance.

**Manual smoke-diff between engine versions (task 6.3).** A separate, specifically conducted step consisted of running the same real assets through the plugin built under UE 5.7 and under UE 5.8, with a manual side-by-side comparison of the output. This is a critically important check precisely because it catches a class of error unavailable to the compiler: silent renames of reflected engine fields (access by property name, as in `BPR_Extractor_Structure`'s class-name comparison, or potentially in places that read widget properties by string name) do not cause a compilation error — they simply start silently returning an empty value or `nullptr` at runtime. Diffing the output between versions is the only practical way to catch this class of regression on a plugin of this profile. Result of the check: the output matches between engine versions.

**Open items for milestone M5** (not yet done, recorded as explicit process debt rather than code debt): automating the 5.7+5.8 compile matrix in CI (currently the compile check is performed manually, locally), formal smoke tests beyond the manually performed task 6.3, and the release preparation for Fab itself (packaging per engine version separately, publishing a Project Version).

## Known Limitations and Technical Debt

Below are open items, recorded as-is, as not yet done, without any attempt to make them look better or worse than they are.

- **No automated testing.** As described in the validation section, all current correctness checking is manual (per-milestone editor validation, manual smoke-diff between engine versions). The 5.7+5.8 CI compile matrix and formal automated smoke tests are the open part of milestone M5, not yet started as a separate process as of this document's writing.
- **No universal fallback extractor for arbitrary Blueprint classes.** Assets of type `UBlueprint` that are neither an `AActor` nor a `UActorComponent` subclass, nor an interface (for example, an arbitrary utility Blueprint class with no specific parent, as well as `UAnimBlueprint` — which is technically a `UBlueprint` subclass but does not fall under any current `CanHandleAsset()` condition), are considered unsupported and handled via the `BPR_InfoWindow` popup rather than through substantive extraction. The idea of a generic `BPR_Extractor_Blueprint` is recorded in the codebase as a commented-out future Phase 3, status deliberately deferred, not in the scope of any completed milestone.
- **The material expression DAG walk is not shared between `BPR_Extractor_Material` and `BPR_Extractor_MaterialFunction`.** The logic is duplicated between the two extractors for historical reasons and was originally flagged as the highest-risk code during migration to a new engine version. In practice, when moving to UE 5.8, this code compiled without a single change, but the duplication itself remains technical debt: any future fix or improvement to the expression-graph walk logic will have to be made in two places in sync, unless a separate refactor is done to extract the shared part.
- **`CleanName`: residual duplication of name-normalization logic.** After the completion of milestones M2/M3 this logic was unified almost everywhere in the base classes; the one known remaining duplication is a widget-local copy in `BPR_Extractor_Widget`, which has not yet been reduced to the shared implementation from the base class.
- **Logging is not fully unified.** Some non-Widget files (the UI layer, `BPR_Extractor_Material`, `BPR_Extractor_ActorComponent`) still use the engine's standard logging category `LogTemp` instead of the plugin's own category, `LogBlueprintReader`, defined in `BPR_Types.h`. This does not affect functionality but reduces the convenience of filtering the plugin's logs in the Output Log when users and developers are diagnosing problems.
- **Design-output quality for images and color.** In the Design tab's output, references to visual resources (`ResourceObject` on `Brush`/`Image`) are currently output as a raw object GUID rather than a readable asset name — for an LLM consumer this is substantially less useful information than the texture/material name. Similarly, fully zero/transparent color values (e.g. `BackgroundColor:(0,0,0,0)`) are not currently filtered out and clutter the output with uninformative lines. Both items are recorded as an open output-quality item of milestone M4, not blocking functionality but reducing the usefulness of the output for the end use case (pasting into a chat with an AI assistant).
- **Deferred UX items (M4.3, M4.4).** A partial-graph filter (the ability to extract a selected part of the graph rather than the whole thing) and a progress indicator for extracting large assets were deliberately moved out of M4's scope and remain in the backlog with no assignment to a specific milestone.
- **Publishing on Fab requires a separate packaging step per engine version.** Since the `EngineVersion` field in the `.uplugin` file can hold only a single value rather than a list of supported versions, full publication on Fab will require packaging the plugin twice — once with `EngineVersion: "5.7.0"` and once with `"5.8.0"` — as separate Project Versions within a single listing. The source code itself does not change; this is purely process debt, not code debt, assigned to Phase 4 of the migration plan and milestone M5.
