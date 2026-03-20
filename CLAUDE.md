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
- `.claude/agents/` — Agent definitions (source-analyzer, code-transformer, build-manager, test-runner, port-coordinator)
- `lib/posix-shim/` — C compatibility library: POSIX function wrappers using AmigaOS calls
- `toolchain/` — Cross-compiler Docker images, build scripts, target profiles
- `docs/` — Architecture docs, API mapping tables, porting guide
- `ports/` — Output directory for real ports (each port gets original/, ported/, Makefile, PORT.md)
- `examples/` — Reference ports for testing the pipeline (wc, head, mini-find)

## Coding Conventions for Amiga C

- Use **ANSI C89** for maximum compatibility with classic AmigaOS (no C99 unless targeting OS 4.x)
- Include `<proto/*.h>` headers for Amiga system calls (not `<clib/*.h>` pragmas)
- Use Amiga types (`LONG`, `ULONG`, `STRPTR`, `BPTR`, `APTR`) when interfacing with OS libraries
- Use `amiport_*` shim wrappers from `lib/posix-shim/` rather than raw AmigaOS calls in ported code
- Always include Amiga version string: `static const char *verstag = "$VER: progname 1.0 (DD.MM.YYYY)";` (use current date)

## Using the Pipeline — IMPORTANT

**Always use the skills and agents for porting work. Do not do their jobs manually.**

- To port a project: use `/port-project <path>` — it orchestrates the full pipeline
- To analyze source: dispatch the `source-analyzer` agent or use `/analyze-source`
- To transform source: dispatch the `code-transformer` agent or use `/transform-source`
- To build: dispatch the `build-manager` agent or use `/build-amiga`
- To test: dispatch the `test-runner` agent or use `/test-amiga`
- To check Aminet for existing ports: dispatch the `aminet-researcher` agent
- To publish to Aminet: dispatch the `aminet-publisher` agent

The `/port-project` skill runs Stage 0 (Aminet research) through Stage 6 (packaging) automatically, dispatching the appropriate agents at each step. Use it as the entry point for all porting work.

**Available agents:**
| Agent | When to dispatch |
|-------|-----------------|
| `aminet-researcher` | Before any port — check if it already exists |
| `source-analyzer` | Stage 1 — portability analysis |
| `code-transformer` | Stage 3 — source transformation |
| `build-manager` | Stage 4 — cross-compilation and error fixing |
| `test-runner` | Stage 5 — vamos testing |
| `port-coordinator` | Full pipeline orchestration for complex ports |
| `aminet-publisher` | Publishing — curated, never automatic |

## Porting Rules

- **Verify shim availability** — check `lib/posix-shim/include/amiport/` headers before assuming an `amiport_*` function exists. The mapping docs list planned functions that may not be implemented yet.
- **Prefer shim wrappers** over inline AmigaDOS rewrites — keeps ported code clean
- **Never remove functionality** — stub it with a clear message if no Amiga equivalent exists
- **Preserve original source** as reference in `original/` directory alongside the port in `ported/`
- **Document every transformation** with a comment: `/* amiport: replaced POSIX open() with amiport_open() */`
- Use `#ifdef __AMIGA__` blocks when the code should remain cross-platform
- Target **AmigaOS 3.x on 68020+** as the default. Use `#ifdef` for other OS versions.

## Build Instructions

```bash
make help              # Show all available targets
make setup-toolchain   # Install/pull cross-compiler (Docker)
make fetch-ndk         # Download AmigaOS NDK 3.2 R4
make build-shim        # Build the POSIX shim library
make build TARGET=examples/wc   # Build a specific port
make test TARGET=examples/wc    # Test via vamos
make test-shim         # Run POSIX shim library tests via vamos
make package TARGET=examples/wc # Create LHA archive
make clean             # Remove build artifacts
```

**Prerequisites:** Docker (for cross-compiler), Python + amitools (`pip install amitools`) for vamos testing.

## Toolchain

Primary: **bebbo/amiga-gcc** (`m68k-amigaos-gcc`) in Docker
Secondary: **VBCC** (`vc`) in Docker

The toolchain scripts in `toolchain/scripts/` handle detection and invocation. Always use these scripts rather than calling compilers directly.

## Testing

Use **vamos** (from amitools) for CLI program testing — it provides a virtual AmigaOS runtime without needing a full emulator. The `test-amiga` skill handles this.

For GUI programs or hardware-dependent code, use **FS-UAE** with a configured AmigaOS 3.x installation.

## Key References

- `docs/api-mapping.md` — Master POSIX-to-AmigaOS function mapping
- `docs/architecture.md` — System architecture overview
- `docs/porting-guide.md` — Step-by-step porting guide
- `.claude/skills/transform-source/references/transformation-rules.md` — How to transform each pattern
- `.claude/skills/analyze-source/references/posix-to-amiga-map.md` — Portability classification
