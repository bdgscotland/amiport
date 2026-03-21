# amiport

AI-powered toolkit for porting Linux/POSIX C software to the Commodore Amiga. Uses Claude Code skills and agents to automate the analysis, transformation, cross-compilation, and testing pipeline.

## How It Works

```
Source Code → Research → Analyze → Transform → Build → Test → Review → Package
                │          │          │          │        │        │         │
            check if    portability  POSIX→    cross-   vamos   Amiga-    LHA for
            already on   report     Amiga    compile  emulator specific   Aminet
            Aminet                                             review
```

## Quick Start

```bash
git clone https://github.com/bdgscotland/amiport.git
cd amiport

# Check prerequisites and set up toolchain
make doctor             # Check what's installed
make setup-toolchain    # Pull cross-compiler Docker image

# Validate everything works
make smoke-test         # Full end-to-end: build shim → build examples → test in vamos

# Port a project (from within Claude Code)
/port-project /path/to/source.c
```

**Prerequisites:** Docker, Python 3, pip (`pip install amitools` for vamos)

## Ports

Ported programs live in `ports/` — each packages independently for [Aminet](https://aminet.net) upload. See **[PORTS.md](PORTS.md)** for the full catalog with versions, shim coverage, and Aminet status.

| Port | Description | Status |
|------|-------------|--------|
| [cal](ports/cal/) | Unix calendar display (OpenBSD) | Built & tested |
| [cut](ports/cut/) | Extract fields/columns from text (OpenBSD) | Built & tested |
| [diff](ports/diff/) | File comparison utility (OpenBSD) | Built & tested |
| [grep](ports/grep/) | Pattern search — regex, fixed string, recursive (OpenBSD) | Built & tested |
| [sed](ports/sed/) | Stream editor — text transformation (OpenBSD) | Built & tested |

```bash
make list-ports                      # Show all ports and status
make build TARGET=ports/cal          # Build a specific port
make test TARGET=ports/cal           # Test in vamos emulator
make publish TARGET=ports/cal        # Package and upload to Aminet
```

## Interactive Emulator Testing

Test ports on a full Amiga desktop using FS-UAE:

```bash
make setup-emu          # Install FS-UAE, check for Kickstart ROM
make build-ports        # Build everything
make install-emu        # Copy binaries to emulator directory
make emu                # Launch FS-UAE — ports mounted as WORK:
```

In the Amiga shell: `WORK:cal 3 2026`

**Requires:** [FS-UAE](https://fs-uae.net), Kickstart 3.1 ROM (~$10 from [amigaforever.com](https://www.amigaforever.com))

## Example Ports (Pipeline Validation)

These exercise the shim library and validate the build/test pipeline:

| Example | Complexity | Shim Functions Exercised |
|---------|-----------|-------------------------|
| `wc` | Trivial | None (stdio only) |
| `head` | Moderate | `amiport_open`, `amiport_read`, `amiport_close`, `amiport_write`, `amiport_getopt` |
| `mini-find` | Complex | `amiport_opendir`, `amiport_readdir`, `amiport_closedir`, `amiport_stat`, `amiport_getopt` |

## Architecture

### Pipeline Skills

| Skill | Purpose |
|-------|---------|
| `/analyze-source` | Scan C code for portability issues |
| `/transform-source` | Replace POSIX calls with Amiga/shim equivalents |
| `/build-amiga` | Cross-compile using Docker toolchain |
| `/test-amiga` | Test binaries in vamos emulator |
| `/review-amiga` | Amiga-specific code review (stack, memory, BPTR, conventions) |
| `/port-project` | Orchestrate the full pipeline (including Aminet research) |

### Specialized Agents

| Agent | Role |
|-------|------|
| `aminet-researcher` | Check if a tool already exists for AmigaOS |
| `source-analyzer` | Deep portability analysis |
| `code-transformer` | Systematic source transformation |
| `build-manager` | Compiler error diagnosis and fixing |
| `test-runner` | Emulator test execution |
| `port-coordinator` | Full pipeline orchestration |
| `perf-optimizer` | 68k hardware performance optimization |
| `dependency-auditor` | Audit external library dependencies |
| `aminet-publisher` | Prepare and publish ports to Aminet |

### Port Categories (ADR-011)

Not all ports are CLI tools. The pipeline supports five categories:

| Category | Description | Libraries | Test Method |
|----------|-------------|-----------|-------------|
| 1. CLI tools | Pure POSIX utilities (wc, grep, sed) | posix-shim | vamos |
| 2. Scripting interpreters | Lua, bc, awk | posix-shim | vamos |
| 3. Console UI apps | less, vim, nano | posix-shim + console-shim | FS-UAE |
| 4. Network apps | curl, wget, irc | posix-shim + bsdsocket-shim | FS-UAE + TCP/IP |
| 5. GUI apps | (future) | Intuition/MUI | FS-UAE |

### Libraries

| Library | Purpose | Link Flag |
|---------|---------|-----------|
| `lib/posix-shim/` | Tier 1: Direct POSIX-to-AmigaOS wrappers (40+ functions) | `-lamiport` |
| `lib/posix-emu/` | Tier 2: Approximate POSIX emulation (regex, mmap, pipe, select) | `-lamiport-emu` |
| `lib/console-shim/` | Minimal ncurses API via Amiga console.device ANSI escapes | `-lamiport-console` |
| `lib/bsdsocket-shim/` | BSD socket API via bsdsocket.library with auto lifecycle | `-lamiport-net` |

**Not supported** (requires redesign): `fork`/`exec`, `pthreads`, GUI toolkits (X11, GTK, Qt)

See [docs/architecture.md](docs/architecture.md) for full details.

## All Make Targets

```bash
# Setup
make doctor             # Check prerequisites
make setup-toolchain    # Pull/build cross-compiler
make setup-emu          # Install FS-UAE emulator
make fetch-ndk          # Download AmigaOS NDK

# Build
make build-shim         # Cross-compile POSIX shim library (Tier 1)
make build-emu          # Cross-compile POSIX emulation library (Tier 2)
make build-console      # Cross-compile console shim (ncurses)
make build-net          # Cross-compile BSD socket shim
make build TARGET=...   # Build a specific port or example
make build-ports        # Build all ports

# Test
make test TARGET=...    # Test a build via vamos
make test-shim          # Run POSIX shim unit tests
make test-emu           # Run POSIX emulation tests
make test-console       # Run console shim tests
make test-net           # Run BSD socket shim tests
make compare TARGET=... # Compare native vs Amiga output
make smoke-test         # Full end-to-end validation

# Emulator
make install-emu        # Copy binaries to emulator directory
make emu                # Launch FS-UAE

# Package & Info
make package TARGET=... # Create LHA archive for Aminet
make list-ports         # Show all ports and status
make clean              # Remove build artifacts
```

## Contributing

Contributions welcome! Especially:

- Porting useful CLI tools (check Aminet first — use the `aminet-researcher` agent)
- Expanding the POSIX shim
- Testing on real Amiga hardware
- Improving vamos compatibility

## License

MIT License. See [LICENSE](LICENSE).

## Acknowledgments

- [amigadev/m68k-amigaos-gcc](https://hub.docker.com/r/amigadev/m68k-amigaos-gcc) — pre-built cross-compiler
- [bebbo/amiga-gcc](https://github.com/bebbo/amiga-gcc) — m68k cross-compiler (upstream)
- [VBCC](http://sun.hasenbraten.de/vbcc/) — portable C compiler with Amiga targets
- [amitools/vamos](https://github.com/cnvogelg/amitools) — virtual AmigaOS runtime
- [FS-UAE](https://fs-uae.net) — Amiga emulator for interactive testing
- [Aminet](https://aminet.net) — The Amiga software archive
