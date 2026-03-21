# README Overhaul Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restructure the README from a technical reference manual into a narrative that serves three audiences: Amiga enthusiasts, developer tooling people, and contributors — in that priority order.

**Architecture:** Single-file rewrite of README.md. The new structure follows "Vision → Proof → How" — hero statement, ports table as proof, then four pillars (libraries, pipeline, testing, knowledge base). One Mermaid pipeline diagram. Target ~200-250 lines.

**Tech Stack:** Markdown, Mermaid (GitHub-native rendering)

**Spec:** `docs/superpowers/specs/2026-03-21-readme-overhaul-design.md`

**Verified facts (2026-03-21):**
- 6 ports: cal (1.32), cut (1.28), diff (1.95), grep (1.68), sed (1.47), lua (5.4.7)
- 2 live on Aminet: cal, cut (submitted 2026-03-20)
- 4 libraries: posix-shim, posix-emu, console-shim, bsdsocket-shim
- ~50 function declarations in posix-shim headers, ~88 amiport_ symbols total
- 10 agents in .claude/agents/
- ~3,600 ADCD markdown files across 5 directories (amiga-mail, autodocs-2.0, devices, hardware, libraries)
- ADCD index files (FUNCTIONS.md, TYPES.md, INCLUDES.json, examples/) do NOT exist yet — omit from README
- 22 autodoc markdown files in docs/references/autodocs/
- 15 ADRs (001-015 but 17 files including README), 6 PDR files
- CI configured in .github/workflows/ci.yml

---

### Task 1: Write the hero section

**Files:**
- Modify: `README.md` (full rewrite — work in a scratch file first)
- Create: `README.new.md` (working draft, will replace README.md in final task)

Working in a scratch file avoids broken intermediate states on the real README.

- [ ] **Step 1: Create README.new.md with hero section**

Write the top of the file:

```markdown
# amiport

A porting platform for bringing modern software to the classic Amiga.

amiport combines POSIX compatibility libraries, AI-powered build agents, and a complete AmigaOS developer knowledge base to port Linux/POSIX C programs to AmigaOS 3.x — from source analysis through to tested, Aminet-ready binaries.

<!-- badges -->
[![CI](https://github.com/bdgscotland/amiport/actions/workflows/ci.yml/badge.svg)](https://github.com/bdgscotland/amiport/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Aminet](https://img.shields.io/badge/Aminet-2_ports_live-blue)](https://aminet.net)
```

**Tone note:** The one-liner is the vision. The paragraph grounds it. No exclamation marks, no "amazing", no emoji. The badges provide credibility signals without words.

- [ ] **Step 2: Review tone** — read it aloud. Does it sound like a person who respects the platform, or like generated marketing? Adjust.

---

### Task 2: Write the ports table

**Files:**
- Modify: `README.new.md`

**Reference:** `PORTS.md` for exact versions and Aminet URLs.

- [ ] **Step 1: Add ports section**

```markdown
## Ports

| Port | Version | Description | Status |
|------|---------|-------------|--------|
| [cal](ports/cal/) | 1.32 | Unix calendar display (OpenBSD) | [Live on Aminet](https://aminet.net/package/util/cli/cal-1.0) |
| [cut](ports/cut/) | 1.28 | Extract fields/columns from text (OpenBSD) | [Live on Aminet](https://aminet.net/package/util/cli/cut-1.0) |
| [diff](ports/diff/) | 1.95 | File comparison utility (OpenBSD) | Built & tested |
| [grep](ports/grep/) | 1.68 | Pattern search — regex, fixed, recursive (OpenBSD) | Built & tested |
| [sed](ports/sed/) | 1.47 | Stream editor — text transformation (OpenBSD) | Built & tested |
| [lua](ports/lua/) | 5.4.7 | Lua 5.4 scripting language (PUC-Rio) | Built & tested |

Pre-built Amiga binaries are included in each port directory. See **[PORTS.md](PORTS.md)** for the full catalog.
```

- [ ] **Step 2: Add build/test commands below the table**

```markdown
\```bash
make build TARGET=ports/grep    # Build a specific port
make test TARGET=ports/grep     # Test in vamos emulator
make list-ports                 # Show all ports and status
\```
```

- [ ] **Step 3: Verify Aminet URLs** — check that the URLs in PORTS.md match what's in the table.

---

### Task 3: Write the Quick Start section

**Files:**
- Modify: `README.new.md`

- [ ] **Step 1: Add Quick Start**

Retain the current README's Quick Start content — it's already good. Copy it verbatim from the current `README.md:16-33`.

```markdown
## Quick Start

\```bash
git clone https://github.com/bdgscotland/amiport.git
cd amiport
git config core.hooksPath .githooks   # Enable pre-commit validation

# Check prerequisites and set up toolchain
make doctor             # Check what's installed
make setup-toolchain    # Pull cross-compiler Docker image

# Validate everything works
make smoke-test         # Full end-to-end: build shim → build examples → test in vamos

# Port a project (from within Claude Code)
/port-project /path/to/source.c
\```

**Prerequisites:** Docker, Python 3, pip (`pip install amitools` for vamos)
```

---

### Task 4: Write Pillar 1 — Compatibility Libraries

**Files:**
- Modify: `README.new.md`

**References to check:**
- `lib/posix-shim/include/amiport/*.h` — for actual function count
- `docs/posix-tiers.md` — tier classification
- `docs/architecture.md` — library descriptions

- [ ] **Step 1: Add the Compatibility Libraries section**

```markdown
## How It Works

### Compatibility Libraries

Most porting failures come from the POSIX gap — the hundreds of Unix system calls and C library functions that don't exist on AmigaOS. amiport bridges this gap with a tiered compatibility model:

- **Tier 1 — Direct wrappers** (`lib/posix-shim/`): One-to-one POSIX-to-AmigaOS mappings. `open()` becomes `amiport_open()`, which calls `Open()` with mode translation. ~50 functions covering file I/O, directory operations, string utilities, error handling, and option parsing.
- **Tier 2 — Emulation** (`lib/posix-emu/`): Approximate implementations for functions with no direct AmigaOS equivalent — regex, pipes, select. Documented caveats on each.
- **Tier 3 — Redesign required**: `fork`/`exec`, `pthreads`, GUI toolkits (X11, GTK, Qt). These need human-guided architectural changes.

| Library | Purpose | Link Flag |
|---------|---------|-----------|
| `lib/posix-shim/` | Tier 1: Direct POSIX→AmigaOS wrappers | `-lamiport` |
| `lib/posix-emu/` | Tier 2: Approximate POSIX emulation (regex, pipes, select) | `-lamiport-emu` |
| `lib/console-shim/` | Minimal ncurses API via Amiga console.device ANSI escapes | `-lamiport-console` |
| `lib/bsdsocket-shim/` | BSD socket API via bsdsocket.library | `-lamiport-net` |

See [docs/posix-tiers.md](docs/posix-tiers.md) for the full tier classification and [docs/architecture.md](docs/architecture.md) for library internals.
```

---

### Task 5: Write Pillar 2 — AI Pipeline

**Files:**
- Modify: `README.new.md`

**References to check:**
- `.claude/agents/*.md` — agent names and roles
- `.claude/skills/` — skill names
- `.claude/settings.json` — safety hooks

- [ ] **Step 1: Add the AI Pipeline section with Mermaid diagram**

```markdown
### AI Pipeline

The porting pipeline is driven by 10 specialized AI agents, each with a defined role and constrained tools — not one monolithic prompt, but a staged workflow with safety rails.

\```mermaid
graph TD
    A[Research] --> B[Analyze]
    B --> C[Transform]
    C --> D[Build]
    D --> E[Test]
    E --> F[Review]
    F --> G[Package]

    A -.- a1[aminet-researcher]
    B -.- b1[source-analyzer]
    C -.- c1[code-transformer]
    D -.- d1[build-manager]
    E -.- e1[test-runner]
    F -.- f1["memory-checker\n(mandatory)"]
    G -.- g1[aminet-publisher]
\```

**Safety hooks** block agents from editing upstream source or calling the compiler directly — changes go through `make` and the toolchain wrappers. A **mandatory memory checker** runs on every port because AmigaOS has no memory protection: a leaked allocation persists until reboot.

| Agent | Role |
|-------|------|
| `aminet-researcher` | Check Aminet for existing ports before starting |
| `source-analyzer` | Portability analysis and tier classification |
| `code-transformer` | Systematic POSIX→Amiga source transformation |
| `build-manager` | Cross-compilation, error diagnosis, iterative fixing |
| `test-runner` | Emulator test execution and output comparison |
| `port-coordinator` | Multi-file port orchestration |
| `memory-checker` | **Mandatory** — leak detection, double-free, allocation safety |
| `perf-optimizer` | 68k instruction timing and hardware optimization |
| `dependency-auditor` | External library dependency analysis |
| `aminet-publisher` | Aminet package preparation and submission |

Entry point: `/port-project` orchestrates the full pipeline. Individual stages available via `/analyze-source`, `/transform-source`, `/build-amiga`, `/test-amiga`, `/review-amiga`.
```

**Note on Mermaid:** GitHub renders this natively. The diagram shows the linear flow with agent associations. Test by previewing on GitHub after push.

---

### Task 6: Write Pillar 3 — Testing

**Files:**
- Modify: `README.new.md`

**References to check:**
- `docs/adr/014-fs-uae-automated-testing.md` — FS-UAE pipeline design
- `toolchain/templates/test-runner.rexx` — ARexx harness template
- Current README lines 56-68 — interactive emulator section

- [ ] **Step 1: Add the Testing section**

```markdown
### Testing

Two testing paths, both automated:

**vamos** (CLI tools, scripting interpreters): A virtual AmigaOS runtime that runs Amiga binaries on the host. Fast, headless, no emulator needed. Used for all current ports.

\```bash
make test TARGET=ports/grep      # Test a single port
make smoke-test                  # Full pipeline validation
\```

**FS-UAE** (console UI, network apps, or full-system verification): A complete Amiga emulator with automated test execution. A custom Startup-Sequence boots directly into an [ARexx](docs/references/arexx-reference.md) test harness that runs test cases, captures TAP-format results, and signals the emulator to exit — no manual interaction required.

\```bash
make test-fsemu TARGET=ports/grep   # Automated FS-UAE test
\```

See [ADR-014](docs/adr/014-fs-uae-automated-testing.md) for the testing pipeline design.

**Interactive testing** — to explore ports on a real Amiga desktop:

\```bash
make setup-emu        # Install FS-UAE, check for Kickstart ROM
make install-emu      # Copy binaries to emulator directory
make emu              # Launch FS-UAE — ports mounted as WORK:
\```

Requires [FS-UAE](https://fs-uae.net) and a Kickstart 3.1 ROM (~$10 from [amigaforever.com](https://www.amigaforever.com)).
```

---

### Task 7: Write Pillar 4 — AmigaOS Knowledge Base

**Files:**
- Modify: `README.new.md`

**References to check:**
- `docs/references/adcd/raw/` — verify 5 directories and ~3,600 files
- `docs/references/autodocs/` — verify 22 files

- [ ] **Step 1: Add the Knowledge Base section**

```markdown
### AmigaOS Knowledge Base

The project includes the complete **Amiga Developer CD v2.1** converted to searchable markdown — 3,600+ pages across 5 volumes (Libraries, Devices, Hardware, Amiga Mail, Autodocs), plus parsed API function signatures.

Agents use this to reason about AmigaOS APIs with full prose context, code examples, and cross-references — not guessing from function names alone.

This reference material is independently useful for anyone writing AmigaOS software, not just for porting.

Regenerate with `make scrape-adcd` (requires internet, ~20 minutes).
```

**Note:** Do NOT mention FUNCTIONS.md, TYPES.md, INCLUDES.json, or the examples directory — these don't exist yet. Update this section when they land.

---

### Task 8: Write Make Targets, Contributing, and footer sections

**Files:**
- Modify: `README.new.md`

- [ ] **Step 1: Add Make Targets section**

```markdown
## Make Targets

\```bash
# Setup
make doctor             # Check prerequisites
make setup-toolchain    # Pull cross-compiler Docker image
make fetch-ndk          # Download AmigaOS NDK 3.2 R4

# Build & Test
make build-shim         # Build POSIX compatibility libraries
make build TARGET=...   # Build a specific port
make test TARGET=...    # Test via vamos emulator
make smoke-test         # Full end-to-end validation

# Emulator
make setup-emu          # Install FS-UAE
make emu                # Launch FS-UAE with ports mounted
\```

Run `make help` for the full list.
```

- [ ] **Step 2: Add Contributing section**

```markdown
## Contributing

Contributions welcome — especially:

- **Port something new** — check [Aminet](https://aminet.net) first to avoid duplicating existing work
- **Expand the POSIX shim** — see [docs/posix-tiers.md](docs/posix-tiers.md) for unimplemented functions
- **Test on real Amiga hardware** — vamos and FS-UAE catch most issues, but nothing replaces the real thing
- **Improve the knowledge base** — better ADCD cross-references, more code examples, corrections

See [CLAUDE.md](CLAUDE.md) for the full contributor guide.
```

- [ ] **Step 3: Add Acknowledgments and License**

Copy from current README.md lines 199-210 (Acknowledgments + License). Add ADCD attribution:

```markdown
- [Amiga Developer CD v2.1](https://wiki.amigaos.net/wiki/Amiga_Developer_Docs) — Commodore/Amiga developer documentation (converted to markdown)
```

---

### Task 9: Replace README.md and verify

**Files:**
- Delete: `README.new.md` (after copying)
- Modify: `README.md` (replace contents)

- [ ] **Step 1: Check line count**

Run: `wc -l README.new.md` — target is 200-250 lines. If over 250, trim the Make Targets or agent table. If under 200, the content may be too thin.

- [ ] **Step 2: Review the complete draft** — read `README.new.md` end to end. Check:
  - All internal links point to files that exist
  - No aspirational features described (ADCD indexes, debug agent)
  - Tone is consistent throughout
  - Mermaid diagram syntax is valid

- [ ] **Step 3: Replace README.md** — copy contents of `README.new.md` to `README.md`

- [ ] **Step 4: Delete README.new.md**

- [ ] **Step 5: Run `make check-docs`** — ensure the rewrite doesn't break doc-consistency validation.

- [ ] **Step 6: Verify links**

Run: Check that all markdown links in README.md point to existing files:
```bash
grep -oP '\[.*?\]\(((?!https?://).*?)\)' README.md | grep -oP '\(.*?\)' | tr -d '()' | while read f; do [ ! -e "$f" ] && echo "BROKEN: $f"; done
```

- [ ] **Step 7: Commit**

```bash
git add README.md
git commit -m "Overhaul README: vision-first narrative with four pillars

Restructure from technical reference to audience-aware narrative.
Lead with vision and shipped ports, then explain how via four
pillars: compatibility libraries, AI pipeline, testing, and
AmigaOS knowledge base. Add Mermaid pipeline diagram, badges,
and Aminet links."
```
