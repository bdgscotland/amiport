# Architecture

## Pipeline Overview

```
┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
│ Research  │──▶│ Dep Audit│──▶│ Analyze  │──▶│Transform │──▶│  Build   │──▶│  Test    │──▶│ Review & │
│          │   │(if deps) │   │          │   │          │   │          │   │          │   │ Package  │
│ Stage 0  │   │Stage 0b  │   │ Stage 1  │   │ Stage 3  │   │ Stage 4  │   │ Stage 5  │   │ Stage 6-7│
└──────────┘   └──────────┘   └──────────┘   └──────────┘   └──────────┘   └──────────┘   └──────────┘
      │              │              │              │    │          │              │              │
 aminet-        dependency-    source-        code-  hardware-  build-       test-runner   memory-checker
 researcher     auditor        analyzer       trans- expert     manager      test-designer (mandatory, 6b)
  (agent)      (conditional)   (agent)        former (Cat 3+)  (agent)      (agents)      perf-optimizer
                                              (agent)                                    (optional, 6c)

                                   ▲                   ▲
                                   │                   │
                              ┌─────────────────────────────┐
                              │     lib/posix-shim/          │  Tier 1: Direct POSIX wrappers
                              │     lib/posix-emu/           │  Tier 2: Approximate emulation
                              │     lib/console-shim/        │  termcap + ncurses → ANSI escapes
                              │     lib/bsdsocket-shim/      │  BSD sockets → bsdsocket.library
                              └─────────────────────────────┘

For complex multi-file ports (5+ source files, multiple Tier 3 issues):
  Orchestrate from main session — dispatch each specialized agent directly.
  (port-coordinator is deprecated — cannot dispatch subagents.)
  debug-agent is dispatched automatically on FS-UAE crash (Guru Meditation).
  hardware-expert is available on-demand at any stage for hardware questions.
```

## Components

### Skills (Pipeline Stages)

Skills define **what** to do — the instructions, rules, and reference material for each stage.

| Skill | Location | Trigger |
|-------|----------|---------|
| `analyze-source` | `.claude/skills/analyze-source/` | User runs `/analyze-source <path>` |
| `transform-source` | `.claude/skills/transform-source/` | User runs `/transform-source <path>` or called by port-project |
| `build-amiga` | `.claude/skills/build-amiga/` | User runs `/build-amiga` or called by port-project |
| `test-amiga` | `.claude/skills/test-amiga/` | User runs `/test-amiga` or called by port-project |
| `review-amiga` | `.claude/skills/review-amiga/` | User runs `/review-amiga` or called by port-project |
| `debug-amiga` | `.claude/skills/debug-amiga/` | User runs `/debug-amiga <path>` to debug a crashed port |
| `port-project` | `.claude/skills/port-project/` | User runs `/port-project <path>` for full pipeline |

### Agents (Specialized Roles)

Agents define **who** does the work — model selection, tool access, persona, and memory.

- **aminet-researcher**: Uses Sonnet for web research. Checks if a tool already exists on Aminet before porting.
- **source-analyzer**: Uses Sonnet for fast, thorough code scanning. Read-only tools.
- **code-transformer**: Uses Sonnet for systematic source transformation. Full edit access.
- **build-manager**: Uses Sonnet for iterative build-fix cycles. Bash + edit access.
- **test-runner**: Uses Haiku for lightweight test execution. Bash + read only.
- **memory-checker**: Uses Sonnet for memory safety analysis. **Mandatory** (Stage 6b). Read-only tools.
- **perf-optimizer**: Uses Sonnet for 68k hardware performance optimization. **Mandatory** (Stage 6c). Read + analysis tools.
- **profiler**: Uses Sonnet for empirical runtime measurement. Instruments code with ReadEClock-based AMIPORT_PROFILE_BEGIN/END macros, builds profiled binary, runs on vamos/FS-UAE, analyzes timing output. Optional (Stage 6d). Full tool access.
- **hardware-expert**: Uses Sonnet for Amiga system architecture validation. Dual-role: on-demand consultant (other agents escalate hardware questions) + proactive auditor (reviews reference docs for hardware accuracy). Baked-in knowledge covers CPU variants, chipset generations, address space, bus arbitration. Read + edit tools.
- **port-coordinator**: **Deprecated** — cannot dispatch subagents. Orchestrate from main session instead.
- **debug-agent**: Uses Sonnet for autonomous crash debugging. Parses Enforcer hits, maps to source, classifies crashes, applies fixes, and iterates until clean (max 5 iterations). Bash + edit + agent access (can escalate to hardware-expert).
- **dependency-auditor**: Uses Sonnet for auditing external library dependencies. Research + read tools.
- **test-designer**: Uses Sonnet for comprehensive FS-UAE test suite design. Analyzes ported source code (flags, exit codes, error paths) to generate test-fsemu-cases.txt files meeting the project's test coverage standard. Read + write tools.
- **aminet-publisher**: Uses Sonnet for Aminet package preparation and publishing. Curated, never automatic.
- **site-manager**: Uses Sonnet for website deployment, manifest generation, security scanning, and testing. Dispatched by /deploy-site skill.
- **amiport-publisher**: Uses Sonnet for test-gated publishing to amiport.platesteel.net. Validates vamos/FS-UAE tests pass before allowing downloads. Sets package status (stable/testing/hidden). Never publishes without explicit approval.

### Context-Loading Skills

Skills load reference documentation on demand (not always in context):

- `/amiga-api-lookup` — ADCD 2.1 reference library (exec/dos/timer/intuition/graphics APIs)
- `/c89-reference` — C89 constraints, libnix function availability, format specifiers, common mistakes
- `/write-arexx` — ARexx language reference, FS-UAE test harness patterns

### Libraries

The project provides four libraries, corresponding to the tiered compatibility model (ADR-008) and port categories (ADR-011):

| Library | Purpose | Link Flag | API Prefix |
|---------|---------|-----------|------------|
| `lib/posix-shim/` | Tier 1: Direct POSIX-to-AmigaOS wrappers. Includes termios shim (tcgetattr/tcsetattr → SetMode) for terminal programs. | `-lamiport` | `amiport_*` |
| `lib/posix-emu/` | Tier 2: Approximate POSIX emulation with caveats | `-lamiport-emu` | `amiport_emu_*` |
| `lib/console-shim/` | Termcap + ncurses subset via Amiga console.device ANSI escapes (ADR-009). Termcap API (tgetent/tgetstr/tgoto/tparm) for programs like less; curses API (initscr/getch/addch) for ncurses programs. | `-lamiport-console` | termcap + ncurses-compatible |
| `lib/bsdsocket-shim/` | BSD socket API via bsdsocket.library with auto lifecycle (ADR-010) | `-lamiport-net` | `amiport_*` |
| `lib/posix-shim/include/amiport/compat.h` | Platform compatibility layer: alignment macros, compiler workarounds (not a tier — cross-cutting) | included via `<amiport/compat.h>` | `AMIPORT_ALIGN()` |

Key design principle: **one wrapper per POSIX function**, so transformations are simple function renames rather than complex inline rewrites.

### Port Categories (ADR-011)

| Category | Libraries Needed | Test Strategy |
|----------|-----------------|---------------|
| 1. CLI tools | posix-shim [+ posix-emu] | vamos (headless, fast) |
| 2. Scripting interpreters | posix-shim + minor stubs | vamos (headless, fast) |
| 3. Console UI apps | posix-shim + console-shim | FS-UAE + ARexx harness (`make test-fsemu`) |
| 4. Network apps | posix-shim + bsdsocket-shim | FS-UAE + ARexx harness + TCP/IP |
| 5. GUI apps (future) | Intuition/MUI | FS-UAE |

#### FS-UAE Automated Testing (ADR-014)

Categories 3-4 use an automated FS-UAE testing pipeline instead of manual interactive testing:

```
┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│  test-fsemu  │───▶│   FS-UAE     │───▶│ ARexx test   │───▶│ TEST-REPORT  │
│  (host)      │    │  (boots      │    │ harness      │    │ .md (TAP     │
│              │    │   AmigaOS)   │    │ (runs cases) │    │  results)    │
└──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘
                          │                    │
                     Startup-Sequence     UAEQuit signals
                     launches harness     FS-UAE to exit
```

- **vamos** path: fast, headless, no GUI — used for Categories 1-2. All ports tested in CI via `make test-ports`.
- **FS-UAE** path: full AmigaOS boot, ARexx harness executes test cases, TAP output captured via shared RESULTS: volume, UAEQuit terminates the emulator — used for all categories pre-publication.
- **Test assertions**: `EXPECT:` for exact first-line match, `EXPECT_CONTAINS:` for substring match (useful for multi-line command output like diff).
- **Interactive tests** (Category 3+): `ITEST:` blocks use KeyInject (`toolchain/keyinject/`) to inject keystrokes via `commodities.library/AddIEvents()`. The test-runner.rexx harness supports `ITEST:` blocks alongside existing `TEST:` blocks. Interactive tests are skipped on vamos. See ADR-023.
- **Visual verification** (ADR-024): `SCRAPE` + `EXPECT_AT row,col,text` + `EXPECT_CURSOR row,col` directives in ITEST blocks. Forked FS-UAE captures raw ANSI output from console.device; host-side `verify-screen.py` (pyte) reconstructs screen state for character-level assertions.

### Toolchain

Docker containers provide reproducible cross-compilation environments:

- `toolchain/docker/Dockerfile.bebbo-gcc` — Primary: m68k-amigaos-gcc
- `toolchain/docker/Dockerfile.vbcc` — Secondary: VBCC
- `toolchain/scripts/` — Setup, detection, and compilation wrappers
- `toolchain/configs/target-profiles/` — CPU-specific compiler flags

### Reference Documentation

| Resource | Location | Description |
|----------|----------|-------------|
| ADCD 2.1 (full) | `docs/references/adcd/` | Complete Amiga Developer CD in agent-optimized markdown |
| Function index | `docs/references/adcd/FUNCTIONS.md` | Cross-reference: function → all ADCD pages discussing it |
| Type index | `docs/references/adcd/TYPES.md` | All structs, typedefs, enums across ADCD manuals |
| Include mapping | `docs/references/adcd/INCLUDES.json` | `#include` path → documentation chapter |
| Code examples | `docs/references/adcd/examples/` | Extracted AmigaOS code examples by library |
| Autodocs | `docs/references/autodocs/` | API function signatures (896 functions, 21 libraries) |
| Amiga Intern (1992) | `docs/references/amiga-intern/` | Hardware internals: 68030 CPU, custom chips, memory map, DMA, Blitter |

## Data Flow

1. **Input**: Path to a C source tree
2. **Analysis**: Source files scanned, portability report generated (JSON)
3. **Transformation**: Source copied to `ported/` directory, POSIX calls replaced with shim wrappers
4. **Build**: Cross-compiled using Docker toolchain, linked with posix-shim [+ console-shim/bsdsocket-shim as needed]
5. **Test**: Binary executed in vamos (CLI/scripting) or FS-UAE (console UI/network)
6. **Package**: Binary + docs packed into LHA archive

## Governance

Decision records track the evolution of the project:

- `docs/adr/` — Architecture Decision Records (technical choices)
- `docs/pdr/` — Product Decision Records (scope, priorities, users)
- `docs/cidr/` — Conceptual Idea Records (early-stage explorations)
