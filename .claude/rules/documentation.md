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

## Enforcement

- A new skill without README/architecture/porting-guide references is **incomplete**.
- An ADR without an index entry is **lost**.
- A port without a PORTS.md entry is **invisible**.
- Do not ask the user if they want docs updated — just do it.
