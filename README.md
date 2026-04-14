<p align="center">
  <img src="docs/media/readme-header.svg" alt="amiport — Modern Unix tools for the Commodore Amiga" width="100%">
</p>

[![CI](https://github.com/bdgscotland/amiport/actions/workflows/ci.yml/badge.svg)](https://github.com/bdgscotland/amiport/actions/workflows/ci.yml)
[![Ports](https://img.shields.io/badge/ports-61-8B6914)](https://amiport.platesteel.net/packages.html)
[![POSIX shim](https://img.shields.io/badge/POSIX_shim-150+_functions-554433)](docs/posix-tiers.md)
[![Aminet](https://img.shields.io/badge/Aminet-7_live-8B6914)](https://aminet.net)
[![amiport.platesteel.net](https://img.shields.io/badge/amiport-60_packages-554433)](https://amiport.platesteel.net)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/bdgscotland/amiport?style=social)](https://github.com/bdgscotland/amiport/stargazers)

amiport ports POSIX/Linux C programs to AmigaOS 3.x. It provides POSIX compatibility libraries, a cross-compilation toolchain, and an AI-powered pipeline that takes a C source file from analysis through to a tested, [Aminet](https://aminet.net)-ready binary.

**61 programs shipped. 1,419 test cases. 7 live on Aminet. First package manager for classic AmigaOS.**

![End-to-end port of OpenBSD tee to AmigaOS 3.x](docs/media/teeport-demo.gif)

## Why This Is Hard

AmigaOS predates POSIX. There is no `stat()`, no `opendir()`, no `getopt()`, no `regex`, no `fork()`, no signals, no `/tmp`. A typical Unix utility calls dozens of functions that simply do not exist. The Amiga has a 4KB default stack, no memory protection, and an epoch that starts in 1978 instead of 1970. Every ported program needs its POSIX assumptions systematically replaced with AmigaOS equivalents — or the machine crashes.

## Ports

### CLI Utilities

| Port | Version | Source | Status |
|------|---------|--------|--------|
| [amigit](ports/amigit/) | 0.1-2 | amiport-native (libgit2) | Built & tested (82/82 FS-UAE) |
| [basename](ports/basename/) | 1.14 | OpenBSD | Built & tested |
| [bc](ports/bc/) | 1.07.1 | GNU | [Aminet](https://aminet.net) |
| [cal](ports/cal/) | 1.32 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/cal-1.0) |
| [cat](ports/cat/) | 1.34 | OpenBSD | Built & tested |
| [cksum](ports/cksum/) | 1.0 | FreeBSD | Built & tested |
| [cmp](ports/cmp/) | 1.19 | OpenBSD | Published on amiport |
| [col](ports/col/) | 1.20 | OpenBSD | Built & tested |
| [colrm](ports/colrm/) | 1.14 | OpenBSD | Built & tested |
| [column](ports/column/) | 1.27-2 | OpenBSD | Built & tested |
| [comm](ports/comm/) | 1.11 | OpenBSD | Built & tested |
| [cut](ports/cut/) | 1.28 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/cut-1.0) |
| [diff](ports/diff/) | 1.95 | OpenBSD | Submitted to Aminet |
| [dirname](ports/dirname/) | 1.17 | OpenBSD | Built & tested |
| [echo](ports/echo/) | 1.12 | OpenBSD | Built & tested |
| [expand](ports/expand/) | 1.15 | OpenBSD | Built & tested |
| [expr](ports/expr/) | 1.28-2 | OpenBSD | Built & tested |
| [factor](ports/factor/) | 1.30 | OpenBSD | Built & tested |
| [false](ports/false/) | 1.1 | OpenBSD | Built & tested |
| [fold](ports/fold/) | 1.18-2 | OpenBSD | Built & tested |
| [getopt](ports/getopt/) | 1.10 | OpenBSD | Built & tested |
| [grep](ports/grep/) | 1.68 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/grep-1.68) |
| [head](ports/head/) | 1.24 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/head-1.24) |
| [join](ports/join/) | 1.34-2 | OpenBSD | Built & tested |
| [jot](ports/jot/) | 1.56 | OpenBSD | Built & tested |
| [jq](ports/jq/) | 1.7.1-2 | jqlang | Submitted to Aminet |
| [ln](ports/ln/) | 1.25 | OpenBSD | Built & tested |
| [logname](ports/logname/) | 1.10 | OpenBSD | Built & tested |
| [look](ports/look/) | 1.27-2 | OpenBSD | Built & tested |
| [mkdir](ports/mkdir/) | 1.31 | OpenBSD | Built & tested |
| [mv](ports/mv/) | 1.47 | OpenBSD | Published on amiport |
| [nl](ports/nl/) | 1.8-2 | OpenBSD | Built & tested |
| [paste](ports/paste/) | 1.27 | OpenBSD | Built & tested |
| [patch](ports/patch/) | 1.78 | OpenBSD | Built & tested |
| [pr](ports/pr/) | 1.46 | OpenBSD | Built & tested |
| [printenv](ports/printenv/) | 1.8 | OpenBSD | Built & tested |
| [printf](ports/printf/) | 1.28 | OpenBSD | Built & tested |
| [rev](ports/rev/) | 1.16 | OpenBSD | Built & tested |
| [rm](ports/rm/) | 1.45 | OpenBSD | Built & tested |
| [rmdir](ports/rmdir/) | 1.15 | OpenBSD | Built & tested |
| [rs](ports/rs/) | 1.30 | OpenBSD | Built & tested |
| [sed](ports/sed/) | 1.47 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/sed-1.47) |
| [seq](ports/seq/) | 1.8 | OpenBSD | Published on amiport |
| [sleep](ports/sleep/) | 1.29 | OpenBSD | Built & tested |
| [sort](ports/sort/) | 1.0 | Plan 9 | Built & tested |
| [sponge](ports/sponge/) | 0.1 | sbase | Built & tested |
| [strings](ports/strings/) | 1.0 | Custom | Published on amiport |
| [tail](ports/tail/) | 1.24 | OpenBSD | Built & tested |
| [tee](ports/tee/) | 1.15 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/tee-1.15) |
| [touch](ports/touch/) | 1.27 | OpenBSD | Built & tested |
| [test](ports/test/) | 1.23 | OpenBSD | Built & tested |
| [tr](ports/tr/) | 1.22-2 | OpenBSD | Built & tested |
| [true](ports/true/) | 1.1 | OpenBSD | Built & tested |
| [tsort](ports/tsort/) | 1.38-2 | OpenBSD | Built & tested |
| [tty](ports/tty/) | 1.14 | OpenBSD | Built & tested |
| [unexpand](ports/unexpand/) | 1.13 | OpenBSD | Built & tested |
| [uniq](ports/uniq/) | 1.33 | OpenBSD | Built & tested |
| [wc](ports/wc/) | 1.32 | OpenBSD | Built & tested |
| [which](ports/which/) | 1.27 | OpenBSD | Published on amiport |
| [yes](ports/yes/) | 1.9 | OpenBSD | [Aminet](https://aminet.net/package/util/cli/yes-1.9) |

### Scripting Languages

| Port | Version | Source | Status |
|------|---------|--------|--------|
| [awk](ports/awk/) | 2024.12.25 | BWK "One True Awk" | Built & tested |
| [lua](ports/lua/) | 5.4.7 | PUC-Rio | Submitted to Aminet |
| [python3](ports/python3/) | 3.11.12 | CPython | Built & tested |

### Console UI (Interactive)

| Port | Version | Source | Status |
|------|---------|--------|--------|
| [less](ports/less/) | 692 | GNU | Built & tested |
| [mg](ports/mg/) | 3.7 | troglobit/OpenBSD | Built & tested |
| [tetris](ports/tetris/) | 1.35 | OpenBSD | Built & tested |
| [vim](ports/vim/) | 9.1 | Vim | Built & tested |

### Network

| Port | Version | Source | Status |
|------|---------|--------|--------|
| [amiport](ports/amiport/) | 1.0 | Original | Built & tested |
| [wget](ports/wget/) | 1.20.3-2 | GNU | Published on amiport |

### amiport — Package Manager for AmigaOS

The first package manager for classic 68k AmigaOS. `amiport install jq` downloads, verifies (SHA-256), and installs packages from the amiport repository — one command, on the Amiga itself.

![amiport running on Coffin R65 — list, search, doctor, and Vim](ports/amiport/screenshots/amiport-coffin-r65.png)

9 commands: `list`, `search`, `info`, `install`, `upgrade`, `remove`, `installed`, `doctor`, `help`. Written in C89 from scratch (not a port). 42KB binary. Requires a TCP/IP stack (Roadshow, Miami, or AmiTCP).

Version format: `upstream[-portrev]` — port revision shown when > 1 (e.g., `1.68-2` = upstream 1.68, second port revision).

Pre-built Amiga binaries are included in each port directory. See [PORTS.md](PORTS.md) for the full catalog with test counts, shim coverage, and Aminet tracking.

## For Amiga Users

Download pre-built binaries from [amiport.platesteel.net](https://amiport.platesteel.net) or directly from [Aminet](https://aminet.net). Each `.lha` archive includes the binary and documentation — extract to `C:` or any directory in your path.

**amiport package manager:** Install `amiport` and then use `amiport install <name>` to download and install packages directly on your Amiga. Requires a TCP/IP stack (Roadshow, Miami, or AmiTCP).

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

For **batch porting** multiple programs in parallel:

```bash
/batch-port-parallel 5          # 5 Cat 1 ports simultaneously
```

This dispatches `port-worker` agents in isolated git worktrees — each runs the full pipeline independently. FS-UAE testing and reviews run serially after all workers complete.

## Compatibility Libraries

Most porting failures come from the POSIX gap. amiport bridges it with a three-tier model:

| Library | Purpose | Link Flag |
|---------|---------|-----------|
| [posix-shim](lib/posix-shim/) | Direct POSIX-to-AmigaOS wrappers (~90 functions) | `-lamiport` |
| [posix-emu](lib/posix-emu/) | Approximate emulation (regex, pipe, mmap, select) | `-lamiport-emu` |
| [console-shim](lib/console-shim/) | ncurses/termcap API via console.device | `-lamiport-console` |
| [bsdsocket-shim](lib/bsdsocket-shim/) | BSD socket API via bsdsocket.library | `-lamiport-net` |
| [oniguruma](lib/oniguruma/) | Perl-compatible regex engine (for jq) | `-loniguruma` |
| [zlib](lib/zlib/) | DEFLATE/gzip/zlib compression (zlib 1.3.1, for libgit2) | `-lz` |
| [libgit2](lib/libgit2/) | Embedded git library (libgit2 1.8.5, pruned, for amigit) | `-lgit2 -lz -lamiport -lm` |

**Tier 1** (posix-shim) covers functions where POSIX and AmigaOS semantics map cleanly: `open`, `read`, `stat`, `opendir`, `getopt`, `glob`, `fnmatch`, `scandir`, etc. Drop-in replacements, no caveats.

**Tier 2** (posix-emu) covers functions that can be approximated but not perfectly emulated. Each comes with documented limitations.

**Tier 3** means redesign required: `fork`/`exec`, pthreads, X11/GTK. The pipeline flags these during analysis so you know up front what can't be ported.

See [posix-tiers.md](docs/posix-tiers.md) for the complete function classification.

## AI Pipeline

The pipeline uses 20 specialized [Claude Code](https://claude.ai/claude-code) agents, each constrained to a specific role:

```
analyze → transform → build → test → memory-check → perf-optimize → publish
```

| Agent | Role |
|-------|------|
| aminet-researcher | Check if a tool already exists on Aminet before porting |
| source-analyzer | Scan for POSIX dependencies and classify by tier |
| code-transformer | Systematically replace POSIX calls with shim equivalents |
| build-manager | Cross-compile and iterate on build errors |
| test-runner | Validate in vamos (fast, headless) and FS-UAE (real AmigaOS 3.1) |
| test-designer | Design comprehensive FS-UAE test suites from source analysis |
| memory-checker | Audit for leaks, double-free, allocation safety (mandatory) |
| perf-optimizer | 68k-specific static analysis and optimization (mandatory) |
| profiler | Empirical ReadEClock-based runtime measurement (optional) |
| debug-agent | Autonomous Enforcer-based crash diagnosis and fix loop |
| dependency-auditor | Audit external library dependencies before complex ports |
| hardware-expert | Hardware architecture validation (CPU variants, chipset, address space) |
| visual-test-expert | Visual test authoring and debugging (SCRAPE/SCREEN_READ) |
| regression-checker | Rebuild all affected ports after shim library changes |
| catalog-engineer | Catalog management, candidate scoring, batch dispatch |
| site-manager | Website deployment, manifest generation, security scanning |
| aminet-publisher | Aminet package preparation and publishing (curated, never automatic) |
| amiport-publisher | Publish to amiport.platesteel.net (test-gated, never automatic) |
| port-coordinator | **Deprecated** -- cannot dispatch subagents |
| port-worker | Self-contained batch worker for parallel dispatch in worktrees |

Safety hooks enforce discipline: upstream source is read-only, direct compiler calls are blocked, and the pipeline won't proceed past a failing stage.

See [architecture.md](docs/architecture.md) for the full agent breakdown, [ADRs](docs/adr/) for architectural decisions, and [PDRs](docs/pdr/) for product decisions.

## Testing

Every port is tested at two levels:

- **vamos** — fast headless smoke tests (milliseconds, no emulator setup)
- **FS-UAE** — automated testing inside real AmigaOS 3.1 via ARexx harness, with TAP output and automatic emulator shutdown

Interactive programs (less, mg, vim, tetris) get automated keystroke injection ([ADR-023](docs/adr/023-automated-interactive-testing.md)) and visual screen verification with character-level assertions ([ADR-024](docs/adr/024-visual-verification.md), [ADR-025](docs/adr/025-screen-read-trap.md)). 1,384 test cases across all ports, all automated.

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

[amiport.platesteel.net](https://amiport.platesteel.net) — package browser, stats dashboard, news, and download index. Built with an Amiga MUI design system (warm gray, amber/brown/red accents, 1px bevels, no rounded corners). No CSS frameworks, no JS charting libraries, no CDN dependencies. See [DESIGN.md](DESIGN.md).

Release announcements and project updates are published via the `/post-news` skill, which appends to `site/data/news.json`, validates ASCII + schema, dispatches the `site-manager` agent to deploy, and clears the activity feed cache. News items surface on the homepage activity feed, in the News archive (`news.html`), and in the [RSS feed](https://amiport.platesteel.net/feed.php) alongside package releases.

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
