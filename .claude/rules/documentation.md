# Documentation Completeness Rules

**A change is not complete until ALL affected documentation is updated.**

## When Adding/Changing Skills, Agents, Pipeline Stages, Libraries, or ADRs

Update ALL of these:

1. **CLAUDE.md** — Pipeline, agent table, affected sections
2. **README.md** — Skills table, agents table, pipeline diagram, make targets
3. **docs/architecture.md** — Pipeline ASCII diagram, component tables
4. **docs/porting-guide.md** — Step-by-step workflow
5. **.claude/skills/port-project/SKILL.md** — Pipeline stages if affected
6. **docs/adr/** — New ADR for architectural decisions; update README.md index
7. **docs/pdr/** — New PDR for product/scope decisions; update README.md index

## When Completing a Port or Publishing to Aminet

8. **PORTS.md** — Add to catalog table (name, version, description, category, source, status)
9. **README.md** — Add to Ports summary table

## When Completing a Category 3+ Port

10. **test-fsemu-visual-cases.txt** — Visual verification tests in a SEPARATE file from functional tests (ADR-024). Functional and visual MUST be separate FS-UAE passes.

## When Making Cross-Cutting Convention Changes

A convention change (versioning, naming, coding standards, etc.) touches many files beyond the standard checklist above. Before claiming completion, audit ALL of these:

| Category | Files to check |
|----------|---------------|
| **Project docs** | CLAUDE.md, README.md, PORTS.md |
| **Rules** | `.claude/rules/` — any rule that references the convention |
| **Templates** | `ports/templates/` — Makefile.template, PORT.md.template, readme.template, STRUCTURE.md |
| **Skills** | `.claude/skills/port-project/`, `transform-source/references/`, and any skill that touches the convention |
| **Agents** | `.claude/agents/` — any agent that produces artifacts affected by the convention |
| **Site files** | `site/js/packages.js`, `site/js/stats.js`, `site/amiga.html`, `site/feed.php`, `site/api/v1/` |
| **Scripts** | `scripts/check-port-metadata.sh`, `scripts/publish-aminet.sh`, and any validation scripts |

**Method:** Before starting edits, use an Explore agent to find ALL references to the convention being changed. Edit from that list — don't rely on memory to enumerate touchpoints.

## Enforcement

- A new skill without README/architecture/porting-guide references is **incomplete**.
- An ADR without an index entry is **lost**.
- A port without a PORTS.md entry is **invisible**.
- A Category 3+ port without `test-fsemu-visual-cases.txt` has **no visual verification**.
- A cross-cutting change without a full audit of all touchpoints is **incomplete**.
- Do not ask the user if they want docs updated — just do it.
