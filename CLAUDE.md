# amiport — Claude Code Project Instructions

## What This Project Is

amiport is an AI-powered toolkit for porting POSIX/Linux C software to the Commodore Amiga. Claude is the primary porting agent — this project provides the skills, agents, reference docs, and shim libraries that enable automated porting.

## Architecture

The porting pipeline has 4 stages, each backed by a Claude skill:

1. **Analyze** (`/analyze-source`) — Scan source for portability issues
2. **Transform** (`/transform-source`) — Replace POSIX calls with Amiga equivalents
3. **Build** (`/build-amiga`) — Cross-compile with bebbo-gcc or VBCC
4. **Test** (`/test-amiga`) — Run in vamos emulator, verify output

**Orchestration modes:**
- `/port-project` — Run the full pipeline end-to-end (single port)
- `/batch-port-parallel` — Dispatch N ports simultaneously in isolated worktrees

## Codebase Map

- `.claude/skills/` — Skill definitions for each pipeline stage
- `.claude/agents/` — 20 agent definitions (see agent table below for full list)
- `lib/posix-shim/` — Tier 1: Direct POSIX-to-AmigaOS wrappers (`amiport_*` functions)
- `lib/posix-emu/` — Tier 2: Approximate POSIX emulation with documented caveats (`amiport_emu_*` functions)
- `lib/posix-shim/include/amiport/compat.h` — Platform compatibility fixes for 68k quirks (alignment, byte order). Not a function library — a header with macros for issues that break correct C on 68k. See crash-patterns #15, #16.
- `lib/console-shim/` — Minimal ncurses + termcap API mapped to Amiga console.device ANSI escapes (ADR-009). Includes termcap (tgetent/tgetstr/tgoto/tparm) for programs like less, and curses (initscr/getch/addch) for ncurses programs.
- `lib/posix-shim/include/amiport/termios.h` — Minimal termios shim mapping tcgetattr/tcsetattr to AmigaOS SetMode() for raw/cooked console mode. Used by terminal programs (less, nano, vim).
- `lib/bsdsocket-shim/` — BSD socket API via bsdsocket.library with auto lifecycle (ADR-010)
- `lib/http-shim/` — Reusable HTTP/1.0 GET client library on bsdsocket-shim. Used by amiport CLI. Handles redirects, Content-Length validation, progress callbacks, 30s socket timeout.
- `lib/oniguruma/` — Oniguruma 6.9.9 regex engine (ASCII-only build, 156 KB). Perl-compatible regex with named captures. Used by jq for test/match/sub/gsub. Unicode data tables replaced with stubs to save 312 KB.
- `lib/zlib/` — zlib 1.3.1 DEFLATE/gzip/zlib compression library (~90 KB). Pure C89, no amiport shim dependencies, uses libnix fd calls directly for gz* file I/O. Built `-O0` default with per-file `-O1` on hot-path files (`inffast.c`, `adler32.c`, `crc32.c`). `-DNO_DIVIDE` eliminates software divides in adler32. Required by libgit2/amigit (PDR-010) and available for any future compression-dependent ports.
- `lib/libgit2/` — libgit2 1.8.5 embedded git library (~1.44 MB). Dual-flavor build: `libgit2.a` (-m68000, default, used by every 68000-compatible port and by the Stage 5 test suite) and `libgit2-020.a` (-m68020, used by `ports/amigit/`). Clone/fetch/remote/push + smart-HTTP transport framing (clone.c, fetch.c, remote.c, transport.c, transports/smart.c + smart_pkt.c + smart_protocol.c) are PRESENT as of PDR-012 Phase 1. Still pruned: `transports/http.c`, `transports/git.c`, `transports/ssh_*.c`, `transports/local.c`, entire `streams/` subtree, threads, mmap, failalloc, win32. `transport_stubs.c` provides link-time stubs for the static dispatch table (git_smart_subtransport_http/_git/_ssh and git_transport_local return GIT_ERROR); amigit registers its own smart-HTTP subtransport via `git_transport_register` before any remote lookup so its backend preempts the stubs at runtime. Depends on `lib/zlib/` and `lib/posix-shim/`. Force-include `src/util/amigaos_compat.h` retargets `pread`/`pwrite`/`realpath`/`symlink`/`getpwuid_r`/`utimes`/`futimes` to the amiport shim; all other file I/O uses libnix native. Built `-O0` default with per-file `-O1 -fno-strict-aliasing` on 9 audited hot files (SHA1DC, xdiff core, wildmatch). `-DGIT_LEGACY_HASH` avoids a latent bus error in MurmurHash3 (68k alignment). Required by the `ports/amigit/` CLI (PDR-010 + PDR-012). Stage 5 tests at `tests/libgit2/` pass 79/79 on vamos with `-DAMIPORT_VAMOS_LIMITED` gating 6 directory-enumeration tests that need FS-UAE. Audit reports: `lib/libgit2/PATCHES.md`, `lib/libgit2/MEMORY-AUDIT.md`, `lib/libgit2/PERF-REPORT.md`.
- `lib/amissl-sdk/` — AmiSSL SDK headers and stub libraries for optional HTTPS support (used by wget). See known-pitfalls re: libamisslauto.a hard dependency.
- `site/` — Website source for amiport.platesteel.net
  - `site/css/style.css` — MUI warm gray design system (see DESIGN.md)
  - `site/index.html` — Landing page (hero terminal animation, featured packages, getting started, port request form)
  - `site/packages.html` — Package browser with search/filter/sort, rich detail view (porting notes, test gauge, limitations)
  - `site/stats.html` — Stats dashboard with SVG bar charts, category breakdown, publication timeline
  - `site/news.html` — News archive page — release announcements, project updates, behind-the-scenes notes. JS-driven from `site/data/news.json`, rendered by `site/js/news.js`.
  - `site/amiga.html` — HTML 3.2 page for classic Amiga browsers (IBrowse/AWeb). PHP-generated, table layout, <30KB, 640x480
  - `site/feed.php` — RSS 2.0 feed combining published packages **and news entries** (`site/data/news.json`), sorted by date. `?category=<cat>` filter narrows to packages only (news is project-wide).
  - `site/data/news.json` — Source of truth for site news. Flat JSON array of `{id, date, title, body, tags, url}`. ASCII-only. Appended via `/post-news` skill — do not hand-edit when the skill applies. `site/api/v1/activity.php` and `site/feed.php` read this file server-side; the browser reaches it via `site/api/v1/news.php` (see below) because `.htaccess` blocks direct `/data/` access.
  - `site/api/v1/news.php` — Public JSON proxy for `site/data/news.json`. Matches the same pattern as `packages.php` proxying `data/packages/*.json`. Validates JSON parses before echoing.
  - `site/js/packages.js` — Package browser logic + keyboard shortcuts (P/S/Esc//)
  - `site/js/stats.js` — Stats rendering with SVG chart generation (no charting library)
  - `site/js/news.js` — News archive renderer. Supports a markdown-lite subset in `body`: paragraphs (blank line), links (`[text](url)`), bold (`**x**`), inline code (`` `x` ``).
  - `site/js/terminal-anim.js` — Hero typing animation (respects prefers-reduced-motion)
  - `site/api/v1/` — PHP API endpoints (packages, stats, download, vote, request, activity — the activity endpoint merges news.json entries into the feed)
- `toolchain/` — Cross-compiler Docker images, build scripts, target profiles
- `toolchain/keyinject/` — KeyInject: keyboard event injector for functional interactive testing via AddIEvents() (ADR-023)
- `toolchain/screenread/` — ScreenRead: ConUnit cursor reader for visual test cursor verification (ADR-025)
- `scripts/inject-keys.sh` — Host-side key injection via macOS osascript for visual tests (ADR-025). Batches all keystrokes into a single osascript call (~1.7s overhead)
- `docs/` — Architecture docs, API mapping tables, porting guide, tier classification
- `docs/references/adcd/` — Complete ADCD 2.1 in agent-optimized markdown (Libraries, Devices, Hardware, Amiga Mail, Autodocs)
- `docs/references/amiga-intern/` — "Amiga Intern" (1992) converted to markdown — 68030 CPU internals, custom chip architecture, memory map, hardware programming
- `tests/` — Unit tests (shim/, emu/, console/, net/, common/, zlib/)
- `ports/` — Output directory for real ports (each port gets original/, ported/, Makefile, PORT.md)
- `ports/templates/` — Canonical templates for per-port artifacts (Makefile, PORT.md, .readme, directory structure)
- `data/catalog.json` — Porting Tech Tree: candidate inventory, readiness scoring, shim unlock index, hardware profiles
- `scripts/catalog-score.py` — Catalog scoring, validation, status dashboard, next-candidate selection, diff reports

## Using the Pipeline — CRITICAL (ENFORCED BY HOOKS)

**Agent dispatch is MANDATORY.** A PreToolUse hook (`enforce-agents.sh`) warns on direct edits to `ported/*.c` files as a reminder to use pipeline agents. The real enforcement is via CLAUDE.md rules and `/port-project` GATE checks.

The `/port-project` skill has GATE checks — it will not proceed to the next stage until the current stage's agent has returned successfully.

**Library ports (`lib/<name>/`) use the same pipeline discipline as port ports.** There is no separate `/port-library` skill — apply the stages manually per `.claude/rules/library-pipeline.md`. Mandatory: KB query → source-analyzer → build → test-designer (library mode) → test-runner → memory-checker → perf-optimizer → docs. Do NOT `make -C lib/<name>` before source-analyzer has returned.

**Post-port quality skills:**
- `/extend-shim <function-name>` — Add a missing POSIX function to the shim library
- `/review-amiga <path>` — Amiga-specific code review (stack safety, BPTR handling, conventions)

**Site skills:**
- `/post-news` — Publish a news entry to `site/data/news.json` and deploy. Use for release announcements, project updates, and behind-the-scenes notes. Validates JSON + ASCII, dispatches `site-manager` to deploy, clears the activity cache. Do not hand-edit `site/data/news.json` when this skill applies — use the skill so deploy + cache clear happen atomically.

**Available agents:**
| Agent | When to dispatch |
|-------|-----------------|
| `aminet-researcher` | Before any port — check if it already exists |
| `source-analyzer` | Stage 1 — portability analysis |
| `code-transformer` | Stage 3 — source transformation |
| `build-manager` | Stage 4 — cross-compilation and error fixing |
| `test-runner` | Stage 5 — vamos testing |
| `port-coordinator` | **DEPRECATED** — cannot dispatch subagents. Orchestrate from main session instead, dispatching specialized agents directly. |
| `dependency-auditor` | Before complex ports — audit external library dependencies |
| `debug-agent` | When a port crashes at runtime — autonomous Enforcer-based crash diagnosis and fix loop |
| `memory-checker` | **Mandatory** Stage 6b — memory leak detection, double-free, allocation safety |
| `perf-optimizer` | **Mandatory** Stage 6c — 68k static analysis and optimization recommendations |
| `profiler` | Optional Stage 6d — empirical ReadEClock-based runtime measurement. Validates perf-optimizer findings |
| `hardware-expert` | Hardware architecture validation — on-demand consultant + proactive doc auditor. Dispatch when agents need hardware facts (address space, CPU variants, chipset capabilities). |
| `test-designer` | Two modes: **port mode** — designs FS-UAE test suites (`test-fsemu-cases.txt`); **library mode** — designs C unit test plans for `lib/<name>/` using `tests/shim/test_framework.h`. Both enforce the test-coverage-standard. |
| `aminet-publisher` | Publishing — curated, never automatic |
| `site-manager` | Website operations — deployment, manifest generation, security scanning, testing |
| `visual-test-expert` | Visual test authoring and debugging — SCRAPE/SCREEN_READ/EXPECT_TRAP_CURSOR (ADR-024/025) |
| `amiport-publisher` | Publish ports to amiport.platesteel.net — test-gated, never automatic |
| `catalog-engineer` | Catalog management — candidate enumeration, dry-run analysis, scoring, batch dispatch |
| `port-worker` | **Draft mode only** — self-contained porting worker for quick-pass batch dispatch in worktrees. Use specialized agents via `/batch-port-parallel` for production quality. |
| `regression-checker` | After shim/library changes — rebuild and test all affected ports to detect regressions |

## Documentation Rules — IMPORTANT

**A change is not complete until all affected documentation is updated.** Full checklist with all 13 touchpoints is in `.claude/rules/documentation.md`. Key points:

- Skills/agents/libraries/ADRs: update CLAUDE.md, README.md, architecture.md, porting-guide.md, port-project skill
- Completed ports: update PORTS.md, README.md ports table
- Port updates (version/deps/tests): update catalog.json, site packages.json
- Cross-cutting changes: audit ALL touchpoints upfront (see rule for full matrix)

## First-Time Setup — MANDATORY

**Run this immediately after cloning.** It configures git hooks that enforce documentation consistency and port directory hygiene. Without this, commits will not be validated.

```bash
make setup             # Configure git hooks (REQUIRED — run first)
make setup-toolchain   # Install cross-compiler Docker image
```

## Build Instructions

```bash
make help              # Show all available targets
make setup             # Configure git hooks (run after cloning)
make setup-toolchain   # Install/pull cross-compiler (Docker)
make build-shim        # Build the POSIX shim library (Tier 1)
make build TARGET=examples/wc   # Build a specific port
make test TARGET=examples/wc    # Test via vamos
make test-shim         # Run POSIX shim library tests via vamos
make test-ports        # Test all production ports via vamos
make check-docs        # Validate agent references across all docs
make check-port-metadata  # Validate port metadata consistency (version, files, PORTS.md)
make check-arexx       # Validate ARexx files (non-ASCII, compound vars, syntax)
make build-keyinject   # Build KeyInject (keystroke injector for interactive tests)
make build-screenread  # Build ScreenRead (screen state reader for visual tests)
make clean             # Remove build artifacts
```

**Prerequisites:** Docker (for cross-compiler), Python + amitools (`pip install amitools`) for vamos testing.

## Versioning

Each port has two version components defined in its Makefile:

- **VERSION** — upstream version (e.g., `1.68`). Only changes when pulling new upstream source.
- **REVISION** — port revision (default `1`). Increment when `ported/`, Makefile, shim deps, or tests change but upstream version stays the same.

`common.mk` computes **DISPLAY_VERSION**: `VERSION` for revision 1, `VERSION-REVISION` for revision 2+ (e.g., `1.68-2`). DISPLAY_VERSION flows to:

- `$VER` string in source code
- `.readme` `Version:` field
- LHA filename (e.g., `grep-1.68-2.lha`)
- Website package display
- PORTS.md catalog

Revision 1 is implicit — never shown. Run `make check-port-metadata` to validate version consistency across all touchpoints.

## Toolchain

Primary: **bebbo/amiga-gcc** (`m68k-amigaos-gcc`) in Docker
Secondary: **VBCC** (`vc`) in Docker

The toolchain scripts in `toolchain/scripts/` handle detection and invocation. Always use these scripts rather than calling compilers directly.

## Testing

Use **vamos** (from amitools) for CLI program testing (Categories 1-2) — it provides a virtual AmigaOS runtime without needing a full emulator. The `test-amiga` skill handles this.

For console UI apps (Category 3), network apps (Category 4), GUI programs, or hardware-dependent code, use **FS-UAE** with a configured AmigaOS 3.x installation. See ADR-014 for automated FS-UAE testing design.

For interactive console programs (Category 3+), the test harness supports `ITEST:` blocks that use **KeyInject** (`toolchain/keyinject/`) to inject keystrokes via `commodities.library/AddIEvents()` for functional tests (exit code verification). Interactive tests are skipped on vamos (KeyInject requires AmigaOS). See ADR-023.

For **visual verification** (ADR-024), use a **separate test file** (`test-fsemu-visual-cases.txt`) with `SCRAPE`, `EXPECT_AT row,col,text`, and `EXPECT_CURSOR row,col` directives. **Functional and visual tests MUST be separate FS-UAE passes** -- never mix them in one suite. Resource exhaustion at ~13 ITESTs is a hard wall. Run visual tests with `make test-fsemu TARGET=ports/<name> VISUAL=1` (passes `--visual` to `scripts/test-fsemu.sh`). The forked FS-UAE (`~/Developer/fs-uae/`) captures per-unit ANSI output; host-side `scripts/verify-screen.py` uses pyte for screen reconstruction. ARexx syntax validated by `scripts/check-arexx-syntax.py` / `make check-arexx`. Note: `CMD_WRITE` captures static display (file load, help text) but NOT interactive echo (typed chars, cursor movement).

For **visual test key injection** (ADR-025), visual ITEST blocks use **host-side injection** via `scripts/inject-keys.sh` instead of Amiga-side KeyInject. This sends keystrokes through macOS `osascript` (System Events) into FS-UAE's SDL input path -- the same path as physical keypresses. AddIEvents() does not reliably deliver to RAW mode programs in visual tests. The ARexx harness coordinates via sentinel files (`keys-request-N` / `keys-done-N`). The `CLEANUP:` directive sends quit keys after SCRAPE capture.

For **cursor position verification** (ADR-025), use `EXPECT_TRAP_CURSOR row,col` in visual test files for COOKED mode programs. This reads cursor position directly from the ConUnit struct via a custom FS-UAE trap (mode 150). Requires `SCREEN_READ` directive and the ScreenRead binary (`toolchain/screenread/`). For RAW mode programs (mg, less, nano), ConUnit cursor stays at (0,0) -- verify cursor via the program's status line using `EXPECT_AT` instead.

## Design System

Always read `DESIGN.md` before making any visual or UI decisions for the website (`site/`).
All font choices, colors, spacing, and aesthetic direction are defined there.
Do not deviate without explicit user approval.
In QA mode, flag any code that doesn't match DESIGN.md.

## Key References

**Shared Knowledge Base (amiga-kb via MCP):**

General Amiga reference docs are in the shared amiga-kb knowledge base (~44K vectors, ~2.6K graph nodes, ~3.5K edges). Use MCP tools instead of reading local files. All queries are tagged with `source_project: "amiport"` for cross-project analytics.

Query tools (read-only):
- `amiga_search` — hybrid vector+keyword search across all docs (RRF fusion)
- `amiga_api_lookup` — function/struct lookup with graph traversal and pitfall warnings
- `amiga_pitfalls_for` — known pitfalls for an API or concept
- `amiga_crash_diagnosis` — crash diagnosis from Guru codes
- `amiga_techniques_for` — demo/game techniques by topic, filterable by chipset (OCS/ECS/AGA)
- `amiga_recipe_lookup` — buildable code recipe with dependency chain and technique links
- `amiga_register_lookup` — custom chip register details + which techniques use it
- `amiga_check_compatibility` — check register overlap and chipset compatibility between techniques
- `amiga_port_briefing` — structured porting briefing from a project description (surfaces pitfalls, required libs, relevant docs)

Hydration tools (write — route universal knowledge here):
- `amiga_ingest_doc` — add/update a document (disk + vectors + graph)
- `amiga_add_pitfall` — add pitfall with auto-edge extraction
- `amiga_add_crash_pattern` — add crash pattern with graph links

Project intelligence:
- `amiga_add_plan` / `amiga_add_todo` / `amiga_update_status` — plan and track work across projects
- `amiga_add_dependency` / `amiga_get_work` / `amiga_get_blockers` — dependency chains and outstanding work

Intelligence (self-improvement):
- `amiga_coverage` — coverage report, query analytics, gap detection
- `amiga_suggest_links` — find orphan nodes, suggest missing graph edges
- `amiga_report_gap` — report missing knowledge for future enrichment
- `amiga_health` — health check all backing services

The amiga-kb MCP server must be running (`docker compose up -d` in the amiga-kb repo).

**Critical (project-specific, consult during every port):**
- `docs/posix-tiers.md` — Master POSIX tier classification (Tier 1/2/3 for every function)
- `docs/references/adcd/` — Complete ADCD 2.1 in markdown (also indexed in amiga-kb vectors)
- `docs/references/amiga-intern/` — "Amiga Intern" (1992) — 68030 internals, custom chip architecture
- `docs/references/m68000-prm/` — Motorola M68000 Family Programmer's Reference Manual (646 pages)
- `docs/test-coverage-standard.md` — **Mandatory** test coverage requirements
- `.claude/skills/transform-source/references/transformation-rules.md` — Tier 1 transformation rules

**Skills for on-demand context loading:**
- `/amiga-api-lookup` — Loads ADCD reference. For simple lookups, prefer `amiga_api_lookup` MCP tool (faster, includes graph data and pitfall warnings).
- `/c89-reference` — C89/ANSI C constraints. No MCP equivalent (project-specific).
- `/write-arexx` — ARexx syntax reference. Also available via `amiga_search "arexx ..."`.
- `/crash-patterns` — Crash KB loader. For quick diagnosis, prefer `amiga_crash_diagnosis` MCP tool.
- `/libnix-reference` — libnix function list. Also available via `amiga_search "libnix ..."`.

**Skill injection:** Knowledge base skills are injected into agent definitions via the `skills:` frontmatter field. When an agent is dispatched, its injected skills are loaded into context automatically. See individual agent definitions in `.claude/agents/` for the injection matrix.
- `/extend-shim` — Invoke when adding new POSIX functions to the shim library.
- `/review-amiga` — Invoke for Amiga-specific code review.
- `/capture-learning` — Invoke when a bug, mistake, or process failure occurs. **Dual-writes** by default: project-local enforcement (hook/rule/agent/skill/memory — strongest) AND amiga-kb via `amiga_add_pitfall` / `amiga_add_crash_pattern` / `amiga_report_gap` when the learning is universal AmigaOS/68k/libnix/bebbo knowledge. Project-local knowledge (pipeline mechanics, site architecture, amiport conventions) stays local only. The skill file has the full YES/NO classification.

**Architecture & guides:** `docs/architecture.md`, `docs/porting-guide.md`, `docs/api-mapping.md`

**ADRs:** `docs/adr/008` (tiers), `009` (console), `010` (bsdsocket), `011` (categories), `014` (FS-UAE testing), `015` (CI/quality), `016` (debug agent), `017` (hooks enforcement), `018` (ADCD knowledge base), `019` (agent persona matrix), `020` (git hooks validation), `021` (design system — MUI warm gray), `022` (C99 compiler support), `023` (automated interactive testing), `024` (visual verification), `025` (screen read trap — interactive cursor verification), `026` (CPython port)

**PDRs:** `docs/pdr/001` (target audience), `002` (MVP wc port), `003` (ports directory + Aminet), `004` (Aminet research first), `005` (committed binaries), `006` (FS-UAE mandatory for all categories), `007` (design system redesign), `008` (SDL2 AmigaOS3 vision), `009` (hardware expansion + SDL)

**Shim references (in amiga-kb):** Use `amiga_search "bsd socket mapping"`, `amiga_search "newlib availability"` etc. ADCD FUNCTIONS.md and TYPES.md still local at `docs/references/adcd/`.

## Safety Hooks

The project enforces structural safety via hooks in `.claude/settings.json`:

- **`block-original-edits.sh`** — Blocks Edit/Write to `/original/`. Upstream source is read-only.
- **`block-root-files.sh`** — Blocks Edit/Write of non-config files in the project root.
- **`block-direct-gcc.sh`** — Blocks direct `m68k-amigaos-gcc`/`ld`/`as` calls. Forces use of `make` or toolchain scripts.
- **`warn-direct-port-build.sh`** — Warns on `make -C ports/` or `make TARGET=ports/` without using the build-manager agent. Allows `make test`, `make clean`, and `make -C lib/` (library builds).
- **`enforce-agents.sh`** — Warns on Edit/Write to `ported/*.c` files. Reminds to use code-transformer or debug-agent (warn-only — subagents use the same tools, so blocking would break the pipeline).
- **`verify-before-stop.sh`** — Reminds Claude to verify work before stopping.
- **`save-port-context.sh`** — On auto-compaction, injects active port names into context.
- **`check-toolchain.sh`** — Warns if Docker, vamos, lha, or jq are missing at session start.
- **`check-c89-comments.sh`** — Warns on C++ style `//` comments in C source files under `ports/` and `lib/`. bebbo-gcc with `-ansi` rejects them.
- **`auto-sync-catalog.sh`** — Auto-copies `data/catalog.json` to `site/data/catalog.json` after edits, preventing catalog drift.

## Git Hooks

The repo uses `.githooks/` for git hooks, configured by `make setup` (which runs `git config core.hooksPath .githooks`):

- **commit-msg**: Enforces conventional commit prefixes (`feat:`, `fix:`, `docs:`, `test:`, `refactor:`, `chore:`, `ci:`, `perf:`, `style:`, `build:`). Allows merge commits.
- **pre-commit**: Runs `make check-docs`, `make check-port-metadata`, and `make check-arexx` (when .rexx files are staged) to validate agent references, port metadata consistency, and ARexx syntax. Also checks for stray root files, port directory hygiene, and non-ASCII in C source. Blocks commits that would introduce doc drift, metadata drift, or violate hygiene rules.
- **pre-push**: Builds the shim library and compiles all shim tests. Catches build/link breakage before it reaches origin. Runs expensive Docker cross-compilation (~10-15s).

**`make setup` is mandatory after cloning.** Without it, hook validation is skipped.

## Continuous Integration

CI runs on every push to main: builds all libs, runs all tests via vamos, validates docs and agent frontmatter, builds and tests all ports. See `.github/workflows/ci.yml`. Toolchain Docker image is cached on GHCR (`ghcr.io/bdgscotland/amiport-toolchain:latest`).
