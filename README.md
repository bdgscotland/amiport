# amiport

AI-powered toolkit for porting Linux/POSIX C software to the Commodore Amiga. Uses Claude Code skills and agents to automate the analysis, transformation, cross-compilation, and testing pipeline.

## How It Works

```
Source Code → Research → Analyze → Transform → Build → Test → Package
                │          │          │          │        │        │
            check if    portability  POSIX→    cross-   vamos    LHA for
            already on   report     Amiga    compile  emulator  Aminet
            Aminet
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

Ported programs live in `ports/` — each packages independently for [Aminet](https://aminet.net) upload.

| Port | Description | Status |
|------|-------------|--------|
| `cal` | Unix calendar display (OpenBSD) | Built & tested |

```bash
make list-ports                      # Show all ports and status
make build TARGET=ports/cal          # Build a specific port
make test TARGET=ports/cal           # Test in vamos emulator
make -C ports/cal TARGET=cal package # Create LHA for Aminet
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

### POSIX Shim Library

`lib/posix-shim/` maps POSIX functions to AmigaOS equivalents (37 functions, 100% unit tested):

- File I/O: `open`, `close`, `read`, `write`, `lseek`, `stat`, `fstat`
- Directories: `opendir`, `readdir`, `closedir`, `mkdir`, `getcwd`, `chdir`
- Process: `sleep`, `usleep`, `getenv`, `getpid`
- Parsing: `getopt`, `strtok_r`
- Errors: `err`, `errx`, `warn`, `warnx`, `strtonum` (BSD compat)
- Signals: `signal`, `raise` (SIGINT via Ctrl-C only)
- Time: `time`, `gettimeofday`
- Misc: `tmpfile`, `isatty`, `access`

**Not supported** (requires redesign): `fork`/`exec`, `mmap`, `pthreads`, BSD sockets

See [docs/architecture.md](docs/architecture.md) for full details.

## All Make Targets

```bash
# Setup
make doctor             # Check prerequisites
make setup-toolchain    # Pull/build cross-compiler
make setup-emu          # Install FS-UAE emulator
make fetch-ndk          # Download AmigaOS NDK

# Build
make build-shim         # Cross-compile POSIX shim library
make build TARGET=...   # Build a specific port or example
make build-ports        # Build all ports

# Test
make test TARGET=...    # Test a build via vamos
make test-shim          # Run shim unit tests (80 tests)
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
