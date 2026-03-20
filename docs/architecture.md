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
                    │     lib/posix-shim/          │
                    │  POSIX compatibility layer   │
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

### POSIX Shim Library

`lib/posix-shim/` is a C library that provides POSIX-compatible wrapper functions implemented using AmigaOS system calls. Ported code links against this library.

Key design principle: **one wrapper per POSIX function**, so transformations are simple function renames rather than complex inline rewrites.

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
4. **Build**: Cross-compiled using Docker toolchain, linked with posix-shim
5. **Test**: Binary executed in vamos, output compared to expected results
6. **Package**: Binary + docs packed into LHA archive

## Governance

Decision records track the evolution of the project:

- `docs/adr/` — Architecture Decision Records (technical choices)
- `docs/pdr/` — Product Decision Records (scope, priorities, users)
- `docs/cidr/` — Conceptual Idea Records (early-stage explorations)
