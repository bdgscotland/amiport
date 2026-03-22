# amiport — Claude Code Project Instructions

## What This Project Is

amiport is an AI-powered toolkit for porting POSIX/Linux C software to the Commodore Amiga. Claude is the primary porting agent — this project provides the skills, agents, reference docs, and shim libraries that enable automated porting.

## Architecture

The porting pipeline has 5 stages, each backed by a Claude skill:

1. **Analyze** (`/analyze-source`) — Scan source for portability issues
2. **Transform** (`/transform-source`) — Replace POSIX calls with Amiga equivalents
3. **Build** (`/build-amiga`) — Cross-compile with bebbo-gcc or VBCC
4. **Test** (`/test-amiga`) — Run in vamos emulator, verify output
5. **Orchestrate** (`/port-project`) — Run the full pipeline end-to-end

## Codebase Map

- `.claude/skills/` — Skill definitions for each pipeline stage
- `.claude/agents/` — Agent definitions (source-analyzer, code-transformer, build-manager, test-runner, port-coordinator, dependency-auditor)
- `lib/posix-shim/` — Tier 1: Direct POSIX-to-AmigaOS wrappers (`amiport_*` functions)
- `lib/posix-emu/` — Tier 2: Approximate POSIX emulation with documented caveats (`amiport_emu_*` functions)
- `lib/console-shim/` — Minimal ncurses API mapped to Amiga console.device ANSI escapes (ADR-009)
- `lib/bsdsocket-shim/` — BSD socket API via bsdsocket.library with auto lifecycle (ADR-010)
- `site/` — Website source (PHP API, static HTML, CSS, JS) for amiport.platesteel.net
- `toolchain/` — Cross-compiler Docker images, build scripts, target profiles
- `docs/` — Architecture docs, API mapping tables, porting guide, tier classification
- `docs/references/adcd/` — Complete ADCD 2.1 in agent-optimized markdown (Libraries, Devices, Hardware, Amiga Mail, Autodocs)
- `docs/references/amiga-intern/` — "Amiga Intern" (1992) converted to markdown — 68030 CPU internals, custom chip architecture, memory map, hardware programming
- `tests/` — Unit tests (shim/, emu/, console/, net/, common/)
- `ports/` — Output directory for real ports (each port gets original/, ported/, Makefile, PORT.md)
- `ports/templates/` — Canonical templates for per-port artifacts (Makefile, PORT.md, .readme, directory structure)

## Using the Pipeline — CRITICAL (ENFORCED BY HOOKS)

**Agent dispatch is MANDATORY.** A PreToolUse hook (`enforce-agents.sh`) warns on direct edits to `ported/*.c` files as a reminder to use pipeline agents. The real enforcement is via CLAUDE.md rules and `/port-project` GATE checks.

The `/port-project` skill has GATE checks — it will not proceed to the next stage until the current stage's agent has returned successfully.

**Post-port quality skills:**
- `/extend-shim <function-name>` — Add a missing POSIX function to the shim library
- `/review-amiga <path>` — Amiga-specific code review (stack safety, BPTR handling, conventions)

**Available agents:**
| Agent | When to dispatch |
|-------|-----------------|
| `aminet-researcher` | Before any port — check if it already exists |
| `source-analyzer` | Stage 1 — portability analysis |
| `code-transformer` | Stage 3 — source transformation |
| `build-manager` | Stage 4 — cross-compilation and error fixing |
| `test-runner` | Stage 5 — vamos testing |
| `port-coordinator` | Dispatched by /port-project for complex multi-file ports requiring judgment calls. Not invoked directly. |
| `dependency-auditor` | Before complex ports — audit external library dependencies |
| `debug-agent` | When a port crashes at runtime — autonomous Enforcer-based crash diagnosis and fix loop |
| `memory-checker` | **Mandatory** Stage 6b — memory leak detection, double-free, allocation safety |
| `perf-optimizer` | **Mandatory** Stage 6c — 68k static analysis and optimization recommendations |
| `profiler` | Optional Stage 6d — empirical ReadEClock-based runtime measurement. Validates perf-optimizer findings |
| `hardware-expert` | Hardware architecture validation — on-demand consultant + proactive doc auditor. Dispatch when agents need hardware facts (address space, CPU variants, chipset capabilities). |
| `test-designer` | Designs comprehensive FS-UAE test suites by analyzing source code, flags, exit codes, and error paths |
| `aminet-publisher` | Publishing — curated, never automatic |
| `site-manager` | Website operations — deployment, manifest generation, security scanning, testing |
| `amiport-publisher` | Publish ports to amiport.platesteel.net — test-gated, never automatic |

## Documentation Rules — IMPORTANT

**When adding or changing any skill, agent, pipeline stage, library, or architectural decision:**

1. **CLAUDE.md** — Update the pipeline, agent table, and any affected sections
2. **README.md** — Update the skills table, agents table, pipeline diagram, and make targets
3. **docs/architecture.md** — Update the pipeline ASCII diagram and component tables
4. **docs/porting-guide.md** — Update the step-by-step workflow
5. **.claude/skills/port-project/SKILL.md** — Update pipeline stages if affected
6. **docs/adr/** — Create a new ADR for architectural decisions; update README.md index
7. **docs/pdr/** — Create a new PDR for product/scope decisions; update README.md index

**When completing a port (`/port-project`) or publishing to Aminet:**

8. **PORTS.md** — Add the port to the catalog table (name, version, description, category, source, status). Update Aminet tracking table after publishing.
9. **README.md** — Add the port to the Ports summary table

**Do not consider a change complete until all affected documentation is updated.** A new skill without README/architecture/porting-guide references is incomplete. An ADR without an index entry is lost. A port without a PORTS.md entry is invisible.

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
make clean             # Remove build artifacts
```

**Prerequisites:** Docker (for cross-compiler), Python + amitools (`pip install amitools`) for vamos testing.

## Toolchain

Primary: **bebbo/amiga-gcc** (`m68k-amigaos-gcc`) in Docker
Secondary: **VBCC** (`vc`) in Docker

The toolchain scripts in `toolchain/scripts/` handle detection and invocation. Always use these scripts rather than calling compilers directly.

## Testing

Use **vamos** (from amitools) for CLI program testing (Categories 1-2) — it provides a virtual AmigaOS runtime without needing a full emulator. The `test-amiga` skill handles this.

For console UI apps (Category 3), network apps (Category 4), GUI programs, or hardware-dependent code, use **FS-UAE** with a configured AmigaOS 3.x installation. See ADR-014 for automated FS-UAE testing design.

## Design System

Always read `DESIGN.md` before making any visual or UI decisions for the website (`site/`).
All font choices, colors, spacing, and aesthetic direction are defined there.
Do not deviate without explicit user approval.
In QA mode, flag any code that doesn't match DESIGN.md.

## Key References

**Critical (consult during every port):**
- `docs/posix-tiers.md` — Master POSIX tier classification (Tier 1/2/3 for every function)
- `docs/references/crash-patterns.md` — Crash pattern KB — check BEFORE fixing any bug
- `docs/references/68k-hardware.md` — 68k hardware reference — memory map, addressing modes, crash signatures, vamos differences
- `docs/references/adcd/` — Complete ADCD 2.1 in markdown — HOW to use AmigaOS functions, not just signatures
- `docs/references/amiga-intern/` — "Amiga Intern" (1992) — 68030 internals, custom chip architecture, memory map, DMA timing, hardware programming
- `docs/references/libnix-reference.md` — Complete libnix function list (700+ functions) extracted from libc.a, plus runtime behavior docs from libnix.texi
- `docs/references/m68000-prm/` — Motorola M68000 Family Programmer's Reference Manual (646 pages, converted to searchable markdown) — instruction set, addressing modes, timing. Section 4 (integer instructions) split into four files (A-B, B-E, E-N, O-U) for fast lookup.
- `docs/test-coverage-standard.md` — **Mandatory** test coverage requirements (no happy-path-only testing)
- `.claude/skills/transform-source/references/transformation-rules.md` — Tier 1 transformation rules

**Skills for on-demand context loading:**
- `/amiga-api-lookup` — **Invoke this skill** when writing or reviewing code that uses AmigaOS APIs (exec.library, dos.library, timer.device, etc.). Loads the ADCD reference library with function signatures, struct layouts, usage patterns, and code examples. Do NOT guess at AmigaOS APIs — look them up via this skill.
- `/c89-reference` — **Invoke this skill** when writing or reviewing C code. Loads the C89/ANSI C constraint set — what C99+ features are NOT available, libnix function availability, printf format restrictions, and common agent mistakes. Prevents generating code that won't compile.
- `/write-arexx` — Invoke when writing or modifying ARexx scripts. Loads ARexx syntax reference and known gotchas.
- `/extend-shim` — Invoke when adding new POSIX functions to the shim library.
- `/review-amiga` — Invoke for Amiga-specific code review.

**Architecture & guides:** `docs/architecture.md`, `docs/porting-guide.md`, `docs/api-mapping.md`

**ADRs:** `docs/adr/008` (tiers), `009` (console), `010` (bsdsocket), `011` (categories), `014` (FS-UAE testing), `015` (CI/quality), `016` (debug agent), `017` (hooks enforcement), `018` (ADCD knowledge base), `019` (agent persona matrix), `020` (git hooks validation), `021` (design system — MUI warm gray)

**Shim references:** `docs/references/bsd-isms.md`, `docs/references/newlib-availability.md`, `docs/references/adcd/FUNCTIONS.md`, `docs/references/adcd/TYPES.md`

## Safety Hooks

The project enforces structural safety via hooks in `.claude/settings.json`:

- **`block-original-edits.sh`** — Blocks Edit/Write to `/original/`. Upstream source is read-only.
- **`block-root-files.sh`** — Blocks Edit/Write of non-config files in the project root.
- **`block-direct-gcc.sh`** — Blocks direct `m68k-amigaos-gcc`/`ld`/`as` calls. Forces use of `make` or toolchain scripts.
- **`enforce-agents.sh`** — Warns on Edit/Write to `ported/*.c` files. Reminds to use code-transformer or debug-agent (warn-only — subagents use the same tools, so blocking would break the pipeline).
- **`verify-before-stop.sh`** — Reminds Claude to verify work before stopping.
- **`save-port-context.sh`** — On auto-compaction, injects active port names into context.
- **`check-toolchain.sh`** — Warns if Docker, vamos, lha, or jq are missing at session start.

## Git Hooks

The repo uses `.githooks/` for git hooks, configured by `make setup` (which runs `git config core.hooksPath .githooks`):

- **commit-msg**: Enforces conventional commit prefixes (`feat:`, `fix:`, `docs:`, `test:`, `refactor:`, `chore:`, `ci:`, `perf:`, `style:`, `build:`). Allows merge commits.
- **pre-commit**: Runs `make check-docs` and `make check-port-metadata` to validate agent references and port metadata consistency (version alignment, required files, no template placeholders, PORTS.md entries, stray artifacts). Also checks for stray root files and port directory hygiene. Blocks commits that would introduce doc drift, metadata drift, or violate hygiene rules.
- **pre-push**: Builds the shim library and compiles all shim tests. Catches build/link breakage before it reaches origin. Runs expensive Docker cross-compilation (~10-15s).

**`make setup` is mandatory after cloning.** Without it, hook validation is skipped.

## Continuous Integration

CI runs on every push to main: builds all libs, runs all tests via vamos, validates docs and agent frontmatter, builds and tests all ports. See `.github/workflows/ci.yml`. Toolchain Docker image is cached on GHCR (`ghcr.io/bdgscotland/amiport-toolchain:latest`).
