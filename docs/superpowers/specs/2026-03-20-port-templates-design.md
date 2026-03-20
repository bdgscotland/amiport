# Port Templates as Data Models

## Problem

Per-port artifacts (Makefile, PORT.md, .readme, directory structure) follow implicit patterns that have drifted across ports. Each new port reinvents the structure, leading to inconsistencies:
- `SOURCE` vs `SOURCES` in Makefiles
- Varying PORT.md section order and naming
- Auto-generated .readme is bare-bones vs. hand-curated cal .readme
- Directory conventions are documented only in prose

Claude needs explicit, machine-readable templates to produce consistent ports.

## Solution

Create `ports/templates/` containing four template files that serve as the canonical "data models" for port artifacts. The `port-project` skill reads these at Stage 2 and fills in placeholders.

## Template Files

### 1. `Makefile.template`
Replaces the existing `ports/Makefile.template`. Uses `__DUNDER__` placeholder syntax.

**Placeholders:** `__TARGET__`, `__DESCRIPTION__`, `__AUTHOR__`, `__AMINET_CAT__`

**Changes from current template:**
- Uses `SOURCES` (plural) consistently
- Includes stub `test:` and `compare:` targets with TODO markers
- Documents all available variables from `common.mk`

### 2. `PORT.md.template`
Standardized documentation with 10 sections in fixed order.

**Sections:**
1. Overview — metadata table
2. Description — what the program does
3. Prior Art on Aminet — Stage 0 research results
4. Portability Analysis — verdict, tier breakdown
5. Transformations Applied — line-level change table
6. Shim Functions Exercised — bulleted list
7. Build Configuration — compiler, flags, size
8. Test Results — test case table
9. Known Limitations — gaps with rationale
10. Review — `/review-amiga` score

**Placeholders:** `__TARGET__`, `__VERSION__`, `__SOURCE_URL__`, `__SOURCE_VERSION__`, `__LICENSE__`, `__AUTHOR__`, `__DATE_ISO__`, `__CATEGORY__`, `__CATEGORY_NAME__`

### 3. `readme.template`
Aminet .readme format matching the quality of the hand-curated `cal-1.0.readme`.

**Placeholders:** `__TARGET__`, `__VERSION__`, `__DESCRIPTION__`, `__AUTHOR__`, `__AMINET_CAT__`, `__DATE_ISO__`

**Includes:** Short description, uploader, author, type, architecture, version, long description with usage examples, build info, attribution, and project link.

### 4. `STRUCTURE.md`
Reference document (not a template to be copied) describing the directory layout, file purposes, and naming conventions. Consumed by Claude as context, not as a fill-in-the-blanks template.

## Placeholder Syntax

All templates use `__DUNDER__` style (double underscores), consistent with the existing `Makefile.template`.

| Variable | Source | Example |
|----------|--------|---------|
| `__TARGET__` | Program name | `cut` |
| `__VERSION__` | Version string | `1.0` |
| `__DESCRIPTION__` | One-line summary | `Extract fields/columns from text` |
| `__AUTHOR__` | Original author | `University of California, Berkeley` |
| `__LICENSE__` | License name | `BSD 3-Clause` |
| `__SOURCE_URL__` | Source origin | `OpenBSD usr.bin/cut` |
| `__SOURCE_VERSION__` | Original version | `v1.28` |
| `__DATE__` | Port date DD.MM.YYYY | `20.03.2026` |
| `__DATE_ISO__` | Port date YYYY-MM-DD | `2026-03-20` |
| `__CATEGORY__` | Port category number | `1` |
| `__CATEGORY_NAME__` | Category label | `CLI tool` |
| `__AMINET_CAT__` | Aminet category | `util/cli` |

## Consumption

- `port-project` skill reads templates at Stage 2 (Set Up Port Directory)
- Claude copies each template, replaces placeholders, fills in content sections
- `STRUCTURE.md` is read as context, not copied
- Old `ports/Makefile.template` is removed (replaced by `ports/templates/Makefile.template`)

## Scope

- Templates apply to `ports/` only, not `examples/`
- Existing ports are not retroactively updated (they already work)
- `common.mk` is unchanged — the .readme auto-generation rule stays as a fallback, but the template produces a richer result when used directly
