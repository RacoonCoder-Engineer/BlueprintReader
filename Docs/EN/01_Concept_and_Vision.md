# Concept and Vision — BlueprintReader

## The problem this product solves

Unreal Engine development increasingly relies on LLM assistants (ChatGPT, Claude, Gemini, and others) as a source of help for debugging, refactoring, writing C++ equivalents of Blueprint logic, and simply as a "second developer" to discuss architecture with. But this approach has a systemic bottleneck: **Blueprint is a visual, graph-based format, while LLMs work with text.** Between "what's happening in the graph" and "what can be pasted into a chat" stands a wall of manual labor.

In practice, this results in several concrete pain points:

- **Screenshots are a poor information channel.** A graph with dozens of nodes, collapsed macros, and hovering pins isn't reliably readable by a model: part of the text in a screenshot becomes illegible when scaled, connections between nodes get visually lost, and the token cost of an image is higher than that of equivalent text, while comprehension quality is lower.
- **Manually retelling Blueprint logic wastes time and introduces errors.** When a developer explains to an assistant what an actor, component, or widget does, they inevitably miss details — call order, default values, event bindings — simply because they're recounting from memory rather than dumping the entire structure.
- **Different asset types require different context, which makes retelling even harder.** A widget's Structure (UI tree, anchors, padding) is fundamentally different from an Actor Blueprint's structure (components, variables, event graphs) or from a material expression DAG. Holding all of this in your head and describing it correctly in words is a realistic but costly task that distracts from actually solving the problem.
- **Existing Blueprint export tools are either not free or not designed for the manual, ad hoc scenario** of "select an asset → get text → paste into any AI chat of your choice" (more on this in "Differentiation from existing solutions").

As a result, many developers either don't use AI assistants for Blueprint work at all (missing out on real value) or spend significant time preparing context manually before every interaction with a model. BlueprintReader exists to remove this preparatory step: to turn any supported Blueprint asset into clean, structured Markdown with one click, directly from the Content Browser.

## The idea and value proposition

BlueprintReader is an editor plugin for Unreal Engine that adds a **"Read Blueprint for AI-Assistant"** entry to the Content Browser's context menu. By selecting an asset and invoking this command, a developer gets a Slate window with **Structure / Graph / Design** tabs, where the entire essence of the asset — variables, components, event graphs, the widget's UI hierarchy, the material's expression DAG — is presented as readable Markdown text, ready to be copied into any AI chat or saved to a file.

The value proposition can be summarized as a single chain:

> **Select an asset → get Markdown → paste it into your favorite AI assistant → get a meaningful answer.**

The key components of this value:

- **Speed.** Extraction happens in one click, with no intermediate steps, no need to open the Blueprint graph itself or manually collapse/expand nodes.
- **Completeness.** The extractors cover the entire lineup of Blueprint-like assets — Actor, ActorComponent, Widget (including nested `WBP_*` instances), Interface, Enum, Structure, Material, MaterialFunction — so the same workflow works uniformly across the whole project, not just for "ordinary" Blueprint classes.
- **A format optimized for LLMs.** The output is clean Markdown with a clear hierarchy of headings and lists, not a serialized dump of internal engine data. A model parses this kind of text far more reliably than a graph image or raw JSON.
- **No vendor lock-in.** The plugin doesn't embed calls to any LLM API, doesn't require an API key, and doesn't tie the user to a specific provider. It stops at generating context and leaves the choice of assistant (Claude, ChatGPT, Gemini, a local model — anything) entirely up to the developer.
- **Transparency of the distribution model.** The product is freely distributed under the MIT license and is free for personal use on Fab; for commercial/studio use there's a nominal fee ($10) — details and rationale are in "Differentiation from existing solutions."

## Target audience

The plugin is aimed primarily at developers who write and maintain Blueprint logic themselves while also using LLMs as part of their everyday workflow. More specifically, these are the following groups:

- **Indie developers and small teams working in Unreal Engine.** Typically constrained in time and human resources for code review or pair programming; an AI assistant often serves as a second opinion. For this audience, the free personal license and zero entry barrier (nothing to configure, no keys required) are especially important.
- **Blueprint programmers porting logic to C++.** The classic "rewrite this Blueprint function in C++" scenario requires the model to see the full structure of the graph — variables, pin types, call order. Exporting to Markdown removes the need to describe this manually in text.
- **UI/UMG developers.** The dedicated **Design** tab for Widget Blueprints is a specialized case: it describes precisely the visual/layout structure (widget hierarchy, anchors, padding, SizeBox properties, and so on), which isn't needed when working with an Actor or Material but is critical when debugging and refactoring interfaces.
- **Technical artists and shader developers.** The Material and MaterialFunction extractors turn the material expression DAG into a text description — useful when discussing material graph optimization or explaining complex shader logic with an assistant.
- **Developers maintaining or updating a project's technical documentation.** Even without using AI directly in the moment, a Markdown dump of a Blueprint's structure is a convenient way to capture "how this actually works" for a team's knowledge base or for onboarding new members.
- **Technical leads and code reviewers** who need to quickly understand the structure of someone else's Blueprint asset without opening it in the editor and manually clicking through nodes.

The common denominator across all these groups is working inside the Unreal Engine editor (5.7 or 5.8) and having the habit of reaching for external AI assistants in a browser or a separate app, rather than through an agentic workflow built into the engine.

## Typical use cases

- **Debugging confusing Actor or ActorComponent behavior.** A developer notices a character isn't behaving as expected, exports the Blueprint via BlueprintReader, and pastes the resulting Markdown (Structure + Graph) into a conversation with an assistant, asking "why doesn't this component respond to this event."
- **Porting Blueprint logic to C++.** Before migrating a function or an entire class, a developer exports the structure and graph to give the model full context on types, variables, and call order — instead of an incomplete retelling in their own words.
- **Making sense of someone else's or a legacy project.** When onboarding onto an existing project (for example, inherited from another developer or purchased as an asset), quickly exporting all key Blueprints to Markdown speeds up building a mental model of how the project works — both independently and with the help of an assistant.
- **Working with UMG widgets.** A UI developer exports a `WBP_*` asset, gets a Design tab with the widget tree, anchors, and properties of the specific instance (including nested custom widgets — see the `IsUserVariable`/`GetPropertyDefaultValue` support), and discusses layout or layout mismatches with an assistant.
- **Reviewing and refactoring materials.** A technical artist exports a Material or MaterialFunction to get a text representation of the expression DAG and discusses simplifying the node chain or optimizing instruction count with an assistant.
- **Documenting Blueprint Interfaces, Enums, and Structures.** Before a code review or during API design, a developer exports an interface contract or a custom data structure to Markdown for use in documentation or in discussion with the team/assistant.
- **Quickly checking an unsupported asset.** If a user selects an asset the plugin can't yet parse (for example, an Animation Blueprint or Data Table), instead of a "silent" refusal they get an informative window (`BPR_InfoWindow`) explaining that this is a limitation of the current plugin version rather than an engine error, along with a link to the repository for tracking updates.
- **Saving context for later use.** Via the "Export to file…" button, the result is saved as `.md` or `.txt` (the format is set in Project Settings → Plugins → Blueprint Reader) — convenient when you need to attach a file to a ticket, an email to a colleague, or save the context "for later" rather than pasting it into a chat right away.

## Differentiation from existing solutions

Before stating the differentiation, it's worth honestly acknowledging: BlueprintReader isn't inventing this niche from scratch. The niche of "converting Blueprint to text for an AI assistant" has already been identified by the market, and at least one product solves it in almost the same way as BlueprintReader. The differentiation rests on several concrete, verifiable points.

**1. Free and open source (MIT).** BlueprintReader is distributed entirely under the MIT license — the source code is open, and it can be studied, forked, and modified without legal risk. This fundamentally distinguishes the product from typical paid plugins on Fab/Marketplace, where the source code is either closed or available only in compiled form, and the license doesn't permit free use or modification.

**2. Fab distribution model: free for personal use, $10 for business.** On Fab, the plugin is available completely free for individual/personal use and costs a nominal $10 when used in a commercial/studio context. This is a deliberate compromise between open-source philosophy (personal use shouldn't be a barrier for anyone) and sustainability for the maintainer (studios extracting commercial value from the tool contribute a small amount toward its continued support). This combination of "open source + free/paid-business model" is itself a point of differentiation from the vast majority of paid marketplace plugins, which never publish their source under any conditions.

**3. An honest look at the competitive landscape.** Research conducted showed that the niche is thin but not empty:
  - The closest direct competitor — **BP2AI - Blueprint Translator** (Fab) — offers practically the same UX (right-click a Blueprint → export to Markdown) and the same "translate and paste into your favorite AI" philosophy. It has broader format coverage (Anim Blueprint, GAS, PCG graphs) and CLI export for CI/pipelines, but its license and pricing aren't publicly stated as open or free — it looks like an ordinary paid marketplace product, unlike BlueprintReader's explicitly stated MIT license.
  - **UE-Blueprint-to-Markdown-Doc** (GitHub, wuzhineihan) — free with visible source code, but oriented toward batch-processing an entire folder, documentation is in Chinese, there's no interactive viewer inside the editor, and there's no explicit license with clear usage terms.
  - **BlueprintSerializer** and **ue-blueprint-extractor** solve adjacent but different problems: the former exports JSON for programmatic processing and C++ generation, the latter is an MCP server for autonomous AI agents (Claude Code, Codex, and the like), requiring a substantially higher setup barrier than "right-click — get text."
  - **Node to Code** and **Blueprint Generator AI (Kibibyte Labs)** solve the inverse or an adjacent problem: they themselves call an LLM API (requiring the user's own key) to generate C++ code or explain nodes inside the editor, rather than exporting portable text that the user pastes into any chat of their choice.
  - Tools like **Blueprint Assist** (graph ergonomics) and **Kantan Doc Gen** (static HTML documentation) don't address the AI-context problem at all.

  Conclusion on the landscape: demand for the category is confirmed by a real competitor (BP2AI), but the segment of "free, open source, no API key, for manual pasting into any chat" remains largely unoccupied.

**4. Technical and product differences.**
  - **Full coverage of the extractor hierarchy in one click.** BlueprintReader uniformly handles the entire lineup of supported types — Actor, ActorComponent, Widget, Material, MaterialFunction, Blueprint Interface, Enum, Structure — through a shared factory (`BPR_Core`) and a unified data contract (`FBPR_ExtractedData`), rather than through a set of disparate tools for different asset types.
  - **Works with any AI assistant, with no vendor lock-in.** The result is plain Markdown text with no embedded calls to any LLM API and no need for an API key. The user decides where to paste the text: into Claude, ChatGPT, Gemini, a local model, or anywhere else. This is the direct opposite of the approach taken by tools like Node to Code or Blueprint Generator AI, which hard-wire specific LLM providers into the plugin itself.
  - **A dedicated Design tab for widgets.** In addition to Structure and Graph, a specialized view of the UI hierarchy is generated for Widget Blueprints (widget tree, anchors, padding, instance properties of nested `WBP_*` widgets) — an emphasis absent from BP2AI's public materials and missing from most JSON-oriented extractors.
  - **Support for two engine versions from a single source tree.** The plugin supports UE 5.7 and 5.8 without separate branches (a "single source + version guards via `BPR_Compat.h`" approach), which reduces the risk of code drift between versions and simplifies maintenance for the end user — they don't need to manually hunt for the "right" plugin version for their engine outside of Fab's standard mechanism (Project Version per engine version).

## Principles and deliberate limitations

BlueprintReader deliberately remains a narrowly specialized tool and doesn't try to cover every adjacent task. Key principles and what the product **deliberately does not do**:

- **Extraction and presentation only, never generation or automation.** The plugin doesn't call any LLM API, doesn't generate code, and doesn't offer "smart" answers on its own. All analytical work stays on the side of whichever AI assistant the user chooses. This is a deliberate rejection of the Node to Code/Blueprint Generator AI model: the moment the plugin adds its own LLM call, a dependency on an API key, token costs, and a specific provider appears — exactly what the product is protecting the user from.
- **A manual, not agentic, workflow.** BlueprintReader is an on-demand tool: the user explicitly selects an asset and explicitly triggers extraction. The plugin doesn't run as an MCP server and doesn't give an AI agent the ability to independently and in real time query or modify the project's Blueprint assets (unlike `ue-blueprint-extractor`). This is a deliberate limitation of the audience: the product is for a person who decides for themselves when and what to show the assistant, not for an autonomous agent.
- **Reasonable, not exhaustive, asset type coverage.** Actor, ActorComponent, Widget, Blueprint Interface, Enum, Structure, Material, and MaterialFunction are supported. Animation Blueprint, StateTree, BehaviorTree, Blackboard, DataAsset/DataTable, Niagara systems, and other asset types are deliberately not supported (at least at this stage) — expanding coverage is possible in the future, but not at the expense of the quality and correctness of the types already supported.
- **Exporting text, not reproducing behavior.** The Markdown output describes the asset's structure and logic for a human and for a model, but it isn't an executable representation and doesn't guarantee byte-for-byte completeness of every possible engine property — the goal is to provide enough context for meaningful work with an assistant, not to create an alternative Blueprint serialization format.
- **Priority on a single distribution platform.** At launch, the plugin is deliberately focused on Fab as the sole distribution channel; this aligns with the decision to defer the plugin's own monetization channels (see the README/Patreon/Boosty decision history — deliberately paused).
- **Compatibility through a single source tree, not through rushed support for every new engine version "at any cost."** The approach to version compatibility (single codebase + `BPR_Compat.h`) assumes that guards are added reactively, based on an actual break discovered during real compilation, rather than proactively/speculatively for any theoretical possibility of incompatibility. This is a deliberate choice favoring maintenance simplicity over premature defense against hypothetical problems.
- **The plugin doesn't replace reading the graph in the editor.** For fine-grained visual debugging (node placement, canvas annotation comments, breakpoints), the standard Blueprint editor remains the primary tool; BlueprintReader complements it for a specific task — preparing textual context for AI — rather than replacing the entire process of working with graphs.

## Success criteria

Since the product is at the stage of preparing for its first full release (`IsBetaVersion: true`, version `0.2.0`, milestone M5 — CI/smoke/Fab release — not yet complete), success criteria are split into technical (release readiness) and product (market acceptance after release).

**Technical criteria (readiness to ship v0.2 on Fab):**

- The plugin builds without errors on both stated engine versions — UE 5.7 and 5.8 — from a single source tree, which has already been confirmed manually; the next step is automating this check via a CI compile matrix, so that a regression on either version is caught automatically rather than only during manual pre-release checking.
- A manual comparison (smoke-diff) of the plugin's output on real assets between 5.7 and 5.8 shows identical results — already performed and confirmed for the current state of the code; the success criterion is maintaining this parity through further changes.
- All 8 supported asset types (Actor, ActorComponent, Widget, Blueprint Interface, Enum, Structure, Material, MaterialFunction) are extracted without editor crashes and without "Unknown"/unset `AssetType` messages — a class of bugs already identified and fixed in earlier stages must not reappear.
- Both versions — 5.7.0 and 5.8.0 — are successfully packaged and uploaded to Fab as separate Project Versions with the correct `EngineVersion` stamped in each package.

**Product criteria (post-release):**

- The presence of organic installs and at least minimal feedback (issues, reviews, questions) on Fab and/or in the GitHub repository, confirming that the tool genuinely solves the problem described in this document for real users, not just for the author.
- No serious recurring complaints about incompatibility with a specific engine version or about incorrect/unreadable Markdown output for supported asset types.
- The formation of a small but growing user base for whom "one-click" export has become part of their everyday workflow when working with AI assistants — that is, a transition from a one-time "let's try it" install to regular use.
- The appearance of at least a handful of cases where users with a commercial/studio profile pay the $10 business license — a signal that the "free for personal use / paid for business" model is viable and perceived as fair rather than as an artificial restriction.
- Further out — expanding asset type coverage (for example, Animation Blueprint) and moving from `IsBetaVersion: true` status to a stable `1.0.0` release only after the CI/smoke-testing criteria (M5) are closed, which will confirm the product's maturity for both personal and commercial use.
