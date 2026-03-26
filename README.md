# amiport

Modern Unix tools for the Commodore Amiga.

[![CI](https://github.com/bdgscotland/amiport/actions/workflows/ci.yml/badge.svg)](https://github.com/bdgscotland/amiport/actions/workflows/ci.yml)
[![Ports](https://img.shields.io/badge/ports-22-8B6914)](https://amiport.platesteel.net/packages.html)
[![POSIX shim](https://img.shields.io/badge/POSIX_shim-150+_functions-554433)](docs/posix-tiers.md)
[![Aminet](https://img.shields.io/badge/Aminet-7_live-8B6914)](https://aminet.net)
[![amiport.platesteel.net](https://img.shields.io/badge/amiport-16_packages-554433)](https://amiport.platesteel.net)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/bdgscotland/amiport?style=social)](https://github.com/bdgscotland/amiport/stargazers)

amiport ports POSIX/Linux C programs to AmigaOS 3.x. It provides POSIX compatibility libraries, a cross-compilation toolchain, and an AI-powered pipeline that takes a C source file from analysis through to a tested, [Aminet](https://aminet.net)-ready binary.

**22 ports shipped. 784 test cases. 7 live on Aminet.**

![End-to-end port of OpenBSD tee to AmigaOS 3.x](docs/media/teeport-demo.gif)

## Why This Is Hard

AmigaOS predates POSIX. There is no `stat()`, no `opendir()`, no `getopt()`, no `regex`, no `fork()`, no signals, no `/tmp`. A typical Unix utility calls dozens of functions that simply do not exist. The Amiga has a 4KB default stack, no memory protection, and an epoch that starts in 1978 instead of 1970. Every ported program needs its POSIX assumptions systematically replaced with AmigaOS equivalents — or the machine crashes.

## Ports

### CLI Utilities

| Port | Version | Source | Status |
|------|---------|--------|--------|
| [bc](ports/bc/) | 1.07.1 | GNU | Built & tested |
| [cal](ports/cal/) | 1.32 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/cal-1.0) |
| [comm](ports/comm/) | 1.11 | OpenBSD | Built & tested |
| [cut](ports/cut/) | 1.28 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/cut-1.0) |
| [diff](ports/diff/) | 1.95 | OpenBSD | Submitted to Aminet |
| [expand](ports/expand/) | 1.15 | OpenBSD | Built & tested |
| [grep](ports/grep/) | 1.68 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/grep-1.68) |
| [head](ports/head/) | 1.24 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/head-1.24) |
| [jq](ports/jq/) | 1.7.1 | jqlang | Built & tested |
| [patch](ports/patch/) | 1.78 | OpenBSD | Built & tested |
| [rev](ports/rev/) | 1.16 | OpenBSD | Built & tested |
| [sed](ports/sed/) | 1.47 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/sed-1.47) |
| [sort](ports/sort/) | 1.0 | Plan 9 | Built & tested |
| [tail](ports/tail/) | 1.24 | OpenBSD | Built & tested |
| [tee](ports/tee/) | 1.15 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/tee-1.15) |
| [uniq](ports/uniq/) | 1.33 | OpenBSD | Built & tested |
| [wc](ports/wc/) | 1.32 | OpenBSD | Built & tested |
| [yes](ports/yes/) | 1.9 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/yes-1.9) |

### Scripting Languages

| Port | Version | Source | Status |
|------|---------|--------|--------|
| [awk](ports/awk/) | 2024.12.25 | BWK "One True Awk" | Built & tested |
| [lua](ports/lua/) | 5.4.7 | PUC-Rio | Submitted to Aminet |

### Console UI (Interactive)

| Port | Version | Source | Status |
|------|---------|--------|--------|
| [less](ports/less/) | 692 | GNU | Built & tested |
| [mg](ports/mg/) | 3.7 | troglobit/OpenBSD | Built & tested |

Version format: `upstream[-portrev]` — port revision shown when > 1 (e.g., `1.68-2` = upstream 1.68, second port revision).

Pre-built Amiga binaries are included in each port directory. See [PORTS.md](PORTS.md) for the full catalog with test counts, shim coverage, and Aminet tracking.

## For Amiga Users

Download pre-built binaries from [amiport.platesteel.net](https://amiport.platesteel.net) or directly from [Aminet](https://aminet.net). Each `.lha` archive includes the binary and documentation — extract to `C:` or any directory in your path.

The website has a [dedicated page for classic Amiga browsers](https://amiport.platesteel.net/amiga.html) (IBrowse, AWeb) — HTML 3.2, under 30KB, 640x480.

## Quick Start

```bash
git clone https://github.com/bdgscotland/amiport.git
cd amiport
make setup              # Configure git hooks (required)
make setup-toolchain    # Pull cross-compiler Docker image
make smoke-test         # Verify everything works

# Port a project (from within Claude Code)
/port-project /path/to/source.c
```

**Prerequisites:** Docker, Python 3, pip (`pip install amitools` for vamos emulator).

The `/port-project` command runs the full pipeline: analyze source for POSIX dependencies, transform calls to Amiga equivalents, cross-compile, test in emulator, review for memory safety and performance. See the [porting guide](docs/porting-guide.md) for details.

## Compatibility Libraries

Most porting failures come from the POSIX gap. amiport bridges it with a three-tier model:

| Library | Purpose | Link Flag |
|---------|---------|-----------|
| [posix-shim](lib/posix-shim/) | Direct POSIX-to-AmigaOS wrappers (~90 functions) | `-lamiport` |
| [posix-emu](lib/posix-emu/) | Approximate emulation (regex, pipe, mmap, select) | `-lamiport-emu` |
| [console-shim](lib/console-shim/) | ncurses/termcap API via console.device | `-lamiport-console` |
| [bsdsocket-shim](lib/bsdsocket-shim/) | BSD socket API via bsdsocket.library | `-lamiport-net` |
| [oniguruma](lib/oniguruma/) | Perl-compatible regex engine (for jq) | `-loniguruma` |

**Tier 1** (posix-shim) covers functions where POSIX and AmigaOS semantics map cleanly: `open`, `read`, `stat`, `opendir`, `getopt`, `glob`, `fnmatch`, `scandir`, etc. Drop-in replacements, no caveats.

**Tier 2** (posix-emu) covers functions that can be approximated but not perfectly emulated. Each comes with documented limitations.

**Tier 3** means redesign required: `fork`/`exec`, pthreads, X11/GTK. The pipeline flags these during analysis so you know up front what can't be ported.

See [posix-tiers.md](docs/posix-tiers.md) for the complete function classification.

## AI Pipeline

The pipeline uses 16 specialized [Claude Code](https://claude.ai/claude-code) agents, each constrained to a specific role:

```
analyze → transform → build → test → memory-check → perf-optimize → publish
```

The source-analyzer scans for POSIX dependencies and classifies them by tier. The code-transformer systematically replaces POSIX calls with shim equivalents. The build-manager cross-compiles and iterates on errors. The test-runner validates in both vamos (fast, headless) and FS-UAE (real AmigaOS 3.1 with ARexx test harness). The memory-checker audits for leaks — mandatory on every port, because AmigaOS has no memory protection or garbage collector. The perf-optimizer applies 68k-specific static analysis.

Safety hooks enforce discipline: upstream source is read-only, direct compiler calls are blocked, and the pipeline won't proceed past a failing stage.

See [architecture.md](docs/architecture.md) for the full agent breakdown, [ADRs](docs/adr/) for architectural decisions, and [PDRs](docs/pdr/) for product decisions.

## Testing

Every port is tested at two levels:

- **vamos** — fast headless smoke tests (milliseconds, no emulator setup)
- **FS-UAE** — automated testing inside real AmigaOS 3.1 via ARexx harness, with TAP output and automatic emulator shutdown

Interactive programs (less, mg) get automated keystroke injection ([ADR-023](docs/adr/023-automated-interactive-testing.md)) and visual screen verification with character-level assertions ([ADR-024](docs/adr/024-visual-verification.md), [ADR-025](docs/adr/025-screen-read-trap.md)). 784 test cases across all ports, all automated.

```bash
make test TARGET=ports/grep             # Quick vamos smoke test
make test-fsemu TARGET=ports/grep       # Full FS-UAE test
make install-emu && make emu            # Manual testing on Amiga desktop
```

Requires [FS-UAE](https://fs-uae.net) and a Kickstart 3.1 ROM (~$10 from [amigaforever.com](https://www.amigaforever.com)) for full-system testing.

## Knowledge Base

The project includes two complete Amiga developer references converted to searchable markdown:

- **[Amiga Developer CD v2.1](docs/references/adcd/)** — Commodore's official reference: 3,600+ pages covering every system library, device, and hardware register. 896 parsed API function signatures across 21 libraries.
- **[Amiga Intern](docs/references/amiga-intern/)** (1992, Abacus) — 42 chapters on 68030 internals, custom chip architecture, memory maps, DMA timing, and hardware programming. Converted from [Internet Archive OCR](https://archive.org/details/Amiga_Intern_1992_Abacus).

These are the references the AI agents reason with when making porting decisions. They're also independently useful as a modern, searchable version of the classic Commodore documentation.

## Build Targets

```bash
make help                   # Show all targets
make build TARGET=ports/... # Build a specific port
make test TARGET=ports/...  # Test via vamos
make test-fsemu TARGET=...  # Test via FS-UAE
make build-shim             # Build POSIX shim library
make test-shim              # Run shim library tests
make build-ports            # Build all ports
make test-ports             # Test all ports via vamos
make doctor                 # Check prerequisites
make list-ports             # Show all ports and status
make check-docs             # Validate documentation consistency
make check-port-metadata    # Validate port metadata
make publish TARGET=...     # Package and upload to Aminet
```

## Website

[amiport.platesteel.net](https://amiport.platesteel.net) — package browser, stats dashboard, and download index. Built with an Amiga MUI design system (warm gray, amber/brown/red accents, 1px bevels, no rounded corners). No CSS frameworks, no JS charting libraries, no CDN dependencies. See [DESIGN.md](DESIGN.md).

## Contributing

- **Port something new** — pick a Unix utility and run `/port-project`. Check Aminet first (use the `aminet-researcher` agent) to avoid duplicating existing work.
- **Expand the POSIX shim** — add missing functions with `/extend-shim`. The skill handles research, classification, implementation, and testing.
- **Test on real hardware** — vamos and FS-UAE catch most issues, but nothing replaces a real A1200. Hardware test reports are valuable.
- **Improve the knowledge base** — better ADCD coverage, more Autodoc parsing, additional cross-references.

See [CLAUDE.md](CLAUDE.md) for coding conventions, pipeline rules, and the full contributor guide.

## Acknowledgments

- [bebbo/amiga-gcc](https://codeberg.org/bebbo/amiga-gcc) — m68k cross-compiler (GCC 6.5.0b)
- [amigadev/m68k-amigaos-gcc](https://hub.docker.com/r/amigadev/m68k-amigaos-gcc) — pre-built cross-compiler Docker image
- [amitools/vamos](https://github.com/cnvogelg/amitools) — virtual AmigaOS runtime for headless testing
- [FS-UAE](https://fs-uae.net) — Amiga emulator for full-system testing
- [Aminet](https://aminet.net) — the Amiga software archive
- [Amiga Developer CD v2.1](https://wiki.amigaos.net/wiki/Amiga_Developer_Docs) — Commodore developer documentation

## License

MIT License. See [LICENSE](LICENSE).
