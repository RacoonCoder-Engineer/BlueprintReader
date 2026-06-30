# Vision — BlueprintReader

## What it is

**BlueprintReader** is an Unreal Engine 5.7+ Editor plugin that turns Blueprints and other assets into **readable Markdown text** — structure, graphs, and (for Widgets) UI hierarchy.

One right-click in the Content Browser → **"Read Blueprint for AI-Assistant"** → a window with **Structure**, **Graph**, and **Design** tabs.

## Who it is for

| Audience | Value |
|----------|-------|
| Unreal developers | Understand complex Blueprints without opening every graph |
| AI-assisted workflows | Give LLMs structured context instead of screenshots |
| Code review & docs | Textual representation of logic and structure |
| Teams | Shareable, diff-friendly asset descriptions |

## Product vision

Blueprint text representation should be a **first-class artifact** in the UE toolchain — as natural as opening the Blueprint editor, but optimized for reading, review, and AI consumption.

Long term:

- Broad asset coverage through a **unified extractor factory**
- Three output layers: **Structure** / **Graph** / **Design**
- Output formats tuned for humans and for LLMs
- Plugin settings, file export, and marketplace-ready quality (Fab)

## Author approach

This project reflects how I build software:

- **Quality and maintainability over speed** — especially during architectural migrations
- **Layered architecture** — UI → Core factory → specialized extractors
- **Plain C++ extractors** — no unnecessary UObject overhead
- **Documentation-driven** — architecture, decisions, and roadmap stay close to the code
- **Honest status** — v0.1 beta; Core/extractor refactor (milestone M1) in progress

## Current status (high level)

- **Version:** 0.1 (beta), UE 5.7
- **License:** MIT
- **Marketplace:** [Fab](https://www.epicgames.com/fab)
- **GitHub:** https://github.com/RacoonCoder-Engineer/BlueprintReader
- **Active work:** Migrating to `BPR_Core` extractor factory and completing Widget integration (see [ROADMAP.md](ROADMAP.md))

## Author

**Racoon Coder** — developer, game designer, systems thinker.  
This repository is open source and also serves as a **portfolio** of engineering approach and product thinking.