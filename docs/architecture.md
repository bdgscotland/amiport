# Architecture

## Pipeline Overview

```
┌─────────────┐    ┌──────────────┐    ┌─────────────┐    ┌────────────┐    ┌───────────┐
│   Analyze    │───▶│  Transform   │───▶│    Build     │───▶│    Test     │───▶│  Package   │
│              │    │              │    │              │    │            │    │           │
│ /analyze-    │    │ /transform-  │    │ /build-      │    │ /test-     │    │ (in build │
│  source      │    │  source      │    │  amiga       │    │  amiga     │    │  skill)   │
└─────────────┘    └──────────────┘    └─────────────┘    └────────────┘    └───────────┘
       │                  │                   │                  │
  source-analyzer   code-transformer    build-manager       test-runner
    (agent)            (agent)            (agent)            (agent)

                         ▲                   ▲
                         │                   │
                    ┌─────────────────────────────┐
                    │     lib/posix-shim/          │  Tier 1: Direct POSIX wrappers
                    │     lib/posix-emu/           │  Tier 2: Approximate emulation
                    │     lib/console-shim/        │  ncurses → ANSI escapes
                    │     lib/bsdsocket-shim/      │  BSD sockets → bsdsocket.library
                    └─────────────────────────────┘
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
| `port-project` | `.claude/skills/port-project/` | User runs `/port-project <path>` for full pipeline |

### Agents (Specialized Roles)

Agents define **who** does the work — model selection, tool access, persona, and memory.

- **source-analyzer**: Uses Sonnet for fast, thorough code scanning. Read-only tools.
- **code-transformer**: Uses parent model for high-quality code edits. Full edit access.
- **build-manager**: Uses Sonnet for iterative build-fix cycles. Bash + edit access.
- **test-runner**: Uses Haiku for lightweight test execution. Bash + read only.
- **port-coordinator**: Uses parent model for orchestration decisions. Full tool access.

### Libraries

The project provides four libraries, corresponding to the tiered compatibility model (ADR-008) and port categories (ADR-011):

| Library | Purpose | Link Flag | API Prefix |
|---------|---------|-----------|------------|
| `lib/posix-shim/` | Tier 1: Direct POSIX-to-AmigaOS wrappers | `-lamiport` | `amiport_*` |
| `lib/posix-emu/` | Tier 2: Approximate POSIX emulation with caveats | `-lamiport-emu` | `amiport_emu_*` |
| `lib/console-shim/` | ncurses subset via Amiga console.device ANSI escapes (ADR-009) | `-lamiport-console` | ncurses-compatible |
| `lib/bsdsocket-shim/` | BSD socket API via bsdsocket.library with auto lifecycle (ADR-010) | `-lamiport-net` | `amiport_*` |

Key design principle: **one wrapper per POSIX function**, so transformations are simple function renames rather than complex inline rewrites.

### Port Categories (ADR-011)

| Category | Libraries Needed | Test Strategy |
|----------|-----------------|---------------|
| 1. CLI tools | posix-shim [+ posix-emu] | vamos |
| 2. Scripting interpreters | posix-shim + minor stubs | vamos |
| 3. Console UI apps | posix-shim + console-shim | FS-UAE |
| 4. Network apps | posix-shim + bsdsocket-shim | FS-UAE + TCP/IP |
| 5. GUI apps (future) | Intuition/MUI | FS-UAE |

### Toolchain

Docker containers provide reproducible cross-compilation environments:

- `toolchain/docker/Dockerfile.bebbo-gcc` — Primary: m68k-amigaos-gcc
- `toolchain/docker/Dockerfile.vbcc` — Secondary: VBCC
- `toolchain/scripts/` — Setup, detection, and compilation wrappers
- `toolchain/configs/target-profiles/` — CPU-specific compiler flags

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
