# amiport

AI-powered toolkit for porting Linux/POSIX C software to the Commodore Amiga. Uses Claude Code skills and agents to automate the analysis, transformation, cross-compilation, and testing pipeline.

## How It Works

```
Source Code → Analyze → Transform → Build → Test → Package
               │          │          │        │        │
          portability   POSIX→    cross-   vamos    ADF/LHA
           report      Amiga    compile  emulator  archive
```

**amiport** provides:

- **Claude skills** that form an automated porting pipeline
- **A POSIX shim library** (`lib/posix-shim/`) bridging Linux system calls to AmigaOS equivalents
- **Docker-based cross-compilation** using [bebbo's amiga-gcc](https://github.com/bebbo/amiga-gcc) or [VBCC](http://sun.hasenbraten.de/vbcc/)
- **Emulator-based testing** via [vamos](https://github.com/cnvogelg/amitools) (virtual AmigaOS runtime)

## Quick Start

```bash
# Clone the repo
git clone https://github.com/duncanmackinnon/amiport.git
cd amiport

# Set up the cross-compilation toolchain (requires Docker)
make setup-toolchain

# Port a project end-to-end
# (from within Claude Code)
/port-project examples/wc/original/wc.c
```

## Supported Targets

| Target | Status | Notes |
|--------|--------|-------|
| AmigaOS 3.x (68020+) | Primary | Most capable classic OS, best toolchain support |
| AmigaOS 2.0 (68000+) | Supported | Reduced API surface |
| AmigaOS 1.3 (68000) | Limited | Minimal dos.library, significant restrictions |
| AmigaOS 4.x (PPC) | Future | Has native POSIX via newlib |
| MorphOS / AROS | Future | Amiga-compatible, broader POSIX support |

## Architecture

The porting pipeline is implemented as Claude Code skills and agents:

### Skills (pipeline stages)

| Skill | Purpose |
|-------|---------|
| `/analyze-source` | Scan C code for portability issues, produce structured report |
| `/transform-source` | Replace POSIX calls with Amiga/shim equivalents |
| `/build-amiga` | Cross-compile using bebbo-gcc or VBCC |
| `/test-amiga` | Run binaries in vamos emulator, verify output |
| `/port-project` | Orchestrate the full pipeline end-to-end |

### Agents (specialized roles)

| Agent | Role |
|-------|------|
| `source-analyzer` | Deep portability analysis |
| `code-transformer` | Systematic source transformation |
| `build-manager` | Compiler error diagnosis and fixing |
| `test-runner` | Emulator test execution |
| `port-coordinator` | Full pipeline orchestration |

See [docs/architecture.md](docs/architecture.md) for details.

## The POSIX Shim

`lib/posix-shim/` provides a compatibility layer mapping common POSIX functions to AmigaOS equivalents:

- File I/O (`open`, `close`, `read`, `write`, `lseek`)
- Directory operations (`opendir`, `readdir`, `closedir`)
- Command-line parsing (`getopt`)
- Error mapping (AmigaDOS IoErr → POSIX errno)
- Signal stubs (SIGINT via `SetSignal()`)

**Not supported** (requires redesign, not just shimming):
- `fork` / `exec` (no process model equivalent)
- `mmap` (no memory-mapped files)
- `pthreads` (no preemptive threading — Amiga uses cooperative tasks)
- BSD sockets (requires bsdsocket.library, separate effort)

## Toolchain

The recommended setup uses Docker for reproducible builds:

```bash
make setup-toolchain    # Pull/build Docker images
make build-shim         # Cross-compile the POSIX shim library
make build TARGET=examples/wc   # Build a specific project
make test TARGET=examples/wc    # Test via vamos
make package TARGET=examples/wc # Create LHA archive
```

See [docs/toolchain-setup.md](docs/toolchain-setup.md) for native installation options.

## Contributing

Contributions welcome! Areas where help is especially appreciated:

- Expanding the POSIX shim (more functions, better OS version support)
- Adding example ports of real-world utilities
- Testing on real Amiga hardware
- AmigaOS 4.x / MorphOS / AROS target support

## License

MIT License. See [LICENSE](LICENSE).

## Acknowledgments

- [bebbo/amiga-gcc](https://github.com/bebbo/amiga-gcc) — m68k cross-compiler
- [VBCC](http://sun.hasenbraten.de/vbcc/) — portable C compiler with Amiga targets
- [amitools/vamos](https://github.com/cnvogelg/amitools) — virtual AmigaOS runtime
- [clib2](https://github.com/adtools/clib2) — C runtime library for AmigaOS
- [AmigaPorts](https://github.com/amigaports) — community porting efforts
