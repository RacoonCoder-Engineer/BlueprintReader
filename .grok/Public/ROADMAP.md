# Roadmap — BlueprintReader

High-level milestones from current refactor to Fab v0.2.  
*Tactical step-by-step checklists are maintained in the private workspace.*

**Updated:** 2026-06-26

```
M0 done ──→ M1 in progress ──→ M2 ──→ M3 ──→ M4 ──→ M5
```

---

## M0. Documentation and project context — done

**Goal:** Structured project knowledge and engineering principles documented.

- Project analysis and architecture documented
- Public / Private documentation split (EN portfolio + local RU workspace)
- Engineering principles and roles defined

---

## M1. Refactor: build and run again — in progress

**Goal:** Complete migration to `BPR_Core` extractor factory. Plugin compiles; context menu works for core asset types.

**Exit criteria:**
- Clean build on `WidgetRefactoring`
- Actor Blueprint → Structure + Graph via "Read Blueprint for AI-Assistant"
- Core registered at module startup and connected to UI

**Scope:** Bridge extractor API, connect Core to UI, temporary adapters for legacy Widget/Material extractors, implement `CanHandleAsset()` across handlers.

---

## M2. Widget Blueprint — full support

**Goal:** `BPR_Extractor_Widget` inherits `BPR_Extractor_Object`; no duplicated graph logic.

**Exit criteria:**
- Widget BP fills all three tabs (Structure / Graph / Design)
- README lists Widget as supported

---

## M3. Full asset coverage

**Goal:** Material, MaterialFunction, and generic Blueprint Class via unified factory with correct priorities.

**Exit criteria:**
- Specific extractors always beat generic fallback
- README matches implemented asset types

---

## M4. UX and product maturity

**Goal:** Daily-use polish beyond AI workflows.

- Plugin settings (output format, widget recursion depth)
- Export to file (.md / .txt)
- Partial graph output filters
- Progress feedback for large Blueprints (500+ nodes)

---

## M5. Quality, CI, Fab v0.2

**Goal:** Stable release with basic automation.

- Smoke automation tests
- CI compile check (UE 5.7)
- Updated Fab listing
- Version 0.2