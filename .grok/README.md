# `.grok/` — Project Documentation

This folder contains project knowledge for **BlueprintReader**, split by audience:

| Folder | Language | In Git | Audience |
|--------|----------|--------|----------|
| **[Public/](Public/)** | English | Yes | Open-source visitors, contributors, portfolio |
| **[Private/](Private/)** | Russian | No (gitignored) | Author + local AI workspace |

## Public (English)

Showcases the developer's **approach, values, and technical vision**:

- [VISION.md](Public/VISION.md) — product story and goals
- [PRINCIPLES.md](Public/PRINCIPLES.md) — engineering values
- [ARCHITECTURE.md](Public/ARCHITECTURE.md) — system design
- [ROADMAP.md](Public/ROADMAP.md) — high-level milestones (M0–M5)

## Private (Russian, local only)

Operational instructions, tactical roadmaps, skills, and session notes.  
**Not published** — clone the repo and maintain your own `Private/` if needed.

## For AI assistants

Read **only** [Private/AGENTS.md](Private/AGENTS.md) (Russian).  
Do **not** use Public docs as instructions.

Root [AGENTS.md](../AGENTS.md) redirects to Private.

## Product documentation (`Docs/`)

| Folder | Language | In Git | Purpose |
|--------|----------|--------|---------|
| `Docs/RU/` | Russian | No | Internal docs (author, Obsidian) — written on request |
| `Docs/EN/` | English | Yes | Full mirror of `Docs/RU/` — updated on request |