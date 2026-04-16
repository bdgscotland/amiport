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

## When Changing a Port's Version, Binary, Tests, or Capabilities

Any change to ported source, dependencies, or test suites requires updating:

10. **PORTS.md** — Update version, test count, status
11. **README.md** — Ports table row for the port
12. **`data/catalog.json`** — Update version, `measured_binary_kb`, `test_count`, `test_pass_rate`
13. **`site/data/catalog.json`** — Copy from `data/catalog.json` (mirror)
14. **`site/data/packages/<name>.json`** — Update top-level `version`, `revision`, `size`, `sha256`, `machine_size`, `machine_sha256`, `download`, `published_at`, `updated_at`, and the embedded `readme` string
15. **`ports/<name>/<name>.readme`** — `Version:` line AND any "amigit 0.1-N -- DEVELOPER PREVIEW" style banner
16. **`ports/<name>/PORT.md`** — `| Version |` row, `| Last Update |` row, and a new Status block describing what changed
17. **`ports/<name>/ported/<name>.h`** or wherever `<NAME>_VERSION` is defined
18. **`ports/<name>/ported/<name>.c`** — `$VER` tag AND any `printf("<name> %s (built YYYY-MM-DD)\n", ...)` build-date strings in cmd_version or equivalent
19. **`ports/<name>/test-fsemu-cases.txt`** — any `EXPECT: <name> X.Y-N` line that pins the exact version string

This applies to version bumps (REVISION changes), dependency additions (e.g., adding Oniguruma), and test suite expansions. Do not consider a port update complete until catalog and site are updated.

### Mandatory pre-commit validation step

**After bumping `VERSION` or `REVISION` in any `ports/<name>/Makefile`, run `make check-port-metadata` IMMEDIATELY, BEFORE `git add`.**

The script validates all 10 touchpoints above (plus the catalog-revision-drift check, stray-artifact check, and site-mirror-integrity check) in a single pass and reports every mismatch in one output. Running it after bumping but before staging lets you fix every drift in one edit loop instead of iterating through per-check failures at pre-commit hook time.

**Failure mode this prevents:** the pre-commit hook is designed to catch exactly these drifts (and it does — it's the last line of defense). But if you stage + commit without running `make check-port-metadata` first, the hook fires, fails, and you end up fixing one file, re-staging, retrying the commit, getting the next failure, fixing that, re-staging, retrying the commit, etc. The hook catches everything eventually, but the interactive thrash wastes time and fragments your attention. One explicit `make check-port-metadata` call up-front shows all the drifts in one report so you can fix them atomically.

**Also run it after any change that touches:**
- `ports/<name>/original/` or `ports/<name>/ported/` (to catch stray `.o` files — run `make -C ports/<name> clean` first)
- `lib/<name>/` if any port links against that library (the revision check may flag dependent ports)
- `site/data/packages/<name>.json` (to catch file/sha mismatches)

### Canonical incident: PDR-012 Phase 7 (2026-04-15)

The Phase 7 commit attempted to land with `REVISION = 7` bumped in the Makefile but the downstream `ports/amigit/amigit.readme` `Version:` line, `ports/amigit/PORT.md` `| Version |` row, `site/data/packages/amigit.json` `revision` field, and three stray `.o` files in `ported/` were all out of sync. Pre-commit hook caught them — correctly — but surfaced them across multiple iterations. Running `make check-port-metadata` once after the REVISION bump would have reported all four drifts + the stray artifacts in one pass and the commit would have landed on the first try.

## When Completing a Category 3+ Port

13. **test-fsemu-visual-cases.txt** — Visual verification tests in a SEPARATE file from functional tests (ADR-024). Functional and visual MUST be separate FS-UAE passes.

## When Announcing News / Updates

Never hand-edit `site/data/news.json`. Use the `/post-news` skill. It validates JSON + ASCII, appends the entry, dispatches `site-manager` to deploy, and clears `/tmp/amiport-activity-cache.json` so the homepage activity feed refreshes immediately. News surfaces in three places automatically once deployed:

14. **site/news.html** (rendered from news.json)
15. **site/feed.php** (RSS 2.0, merged with package items)
16. **Homepage activity feed** (via `site/api/v1/activity.php`)

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
- A port update without catalog.json and packages.json changes is **invisible to users**.
- Do not ask the user if they want docs updated — just do it.
