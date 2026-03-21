# README Overhaul Design

**Date:** 2026-03-21
**Status:** Draft

## Problem

The current README (210 lines) reads like a technical reference manual for someone already working in the project. It leads with pipeline internals and make targets, burying the most compelling aspects — shipped ports on Aminet, the compatibility library suite, the ADCD knowledge base, the automated testing infrastructure. It doesn't communicate what the project *is* to any of its three audiences.

## Audiences (Priority Order)

1. **Amiga enthusiasts** — want to know what exists, how to get it running, what's on Aminet
2. **Developer tooling people** — interested in the AI pipeline, the agent architecture, the engineering rigor
3. **Potential contributors** — want to port more software, expand the shim, test on real hardware

## Design Approach

**Vision → Proof → How.** Open with a framing statement, immediately prove it's real with the ports table, then unfold four pillars explaining how it works. Details pushed to linked docs.

## Structure

```
1. Hero (vision + badges)
2. Ports table (proof it ships, with Aminet links + binary download note)
3. Quick Start
4. How It Works — four pillars:
   4a. Compatibility Libraries (tiered model)
   4b. AI Pipeline (agents, safety hooks, quality gates)
   4c. Testing (vamos + automated FS-UAE + interactive emulator setup)
   4d. AmigaOS Knowledge Base (ADCD + Autodocs)
5. Make Targets (top 8-10, then `make help`)
6. Contributing (four paths + CLAUDE.md pointer)
7. Acknowledgments + License
```

### Diagrams

One Mermaid diagram rendered natively by GitHub:

- **Pipeline flow** — Research → Analyze → Transform → Build → Test → Review → Package, showing which agent handles each stage

The architecture layering is conveyed by the four-pillar prose — a second diagram would add visual weight without new information.

### Badges

Top of README: CI status, license, count of Aminet-live ports.

## Section Details

### 1. Hero

One-line vision statement framing the project as a porting platform for bringing modern software to classic Amiga. Short paragraph (~2-3 sentences): ports POSIX C to AmigaOS 3.x using compatibility libraries, AI agents, and a complete AmigaOS reference knowledge base. Mention ports already live on Aminet.

Tone: warm but restrained. One sentence that shows care for the platform, otherwise technical.

### 2. Ports Table

Promoted from position 4 to position 2. Answers "is this real?" in 5 seconds.

Columns: Port | Version | Category | Description | Status.

Status column uses hyperlinked text where applicable: "[Live on Aminet](https://aminet.net/package/...)" for published ports, "Built & tested" for others.

One-liner below the table: "Download pre-built binaries from the port directories, or grab them from Aminet."

Keep the 4-line `make` block below that (build, test, list-ports, publish).

### 3. Quick Start

Retain current content — it works well. Clone, doctor, setup-toolchain, smoke-test, /port-project.

### 4a. Compatibility Libraries

Lead with the insight: porting fails because of the POSIX gap, not the compiler. Explain the tiered model:
- **Tier 1** (posix-shim): Direct POSIX→AmigaOS wrappers, automated transformation (verify actual count at implementation time)
- **Tier 2** (posix-emu): Approximate emulation (regex, pipes, select), documented caveats
- **Tier 3**: Requires human-guided redesign

Table of four libraries (posix-shim, posix-emu, console-shim, bsdsocket-shim) with link flags.

One line about explicit non-support: fork/exec, pthreads, X11/GTK/Qt.

Link to architecture docs and posix-tiers.md for full classification.

### 4b. AI Pipeline

Frame: 10 specialized agents with defined roles and constrained tools. Not one monolithic LLM — a staged pipeline with safety rails.

Key points to surface:
- Safety hooks (block editing upstream source, block direct compiler calls)
- Mandatory quality gates (memory checker on every port — AmigaOS has no memory protection)
- Governance via ADRs/PDRs (verify counts at implementation time)

Compact agent table (name + one-line role). Mention skill entry points for contributors.

**Pipeline Mermaid diagram** lives in this section.

### 4c. Testing & Debugging

Two testing paths:
- **vamos** — fast, headless, for CLI tools (Categories 1-2)
- **FS-UAE** — automated via ARexx test harnesses, TAP output, UAEQuit for unattended exit (Categories 3+)

Note: FS-UAE testing is automated, not manual. Custom Startup-Sequence → ARexx harness → TAP results → TEST-REPORT.md. No human interaction.

Link to ADR-014 for FS-UAE testing design.

Also fold in the interactive FS-UAE setup commands (boot a real Amiga desktop, type commands) as a subsection here rather than a separate top-level section. Keeps all testing in one place.

### 4d. AmigaOS Knowledge Base

The complete ADCD 2.1 converted to searchable markdown (verify file count at implementation time). Not just raw text — enriched with:
- Function cross-references (FUNCTIONS.md)
- Type/struct/enum index (TYPES.md)
- Include path → documentation mapping (INCLUDES.json)
- Extracted code examples by library

Additionally: API function signatures from the Autodocs (896 functions across 21 libraries) in `docs/references/autodocs/`.

Agents reason about AmigaOS APIs with full prose, signatures, and context.

Note: independently useful for anyone writing AmigaOS code, not just porters.

Regeneratable: `make scrape-adcd`.

**Implementation note:** Verify which ADCD index files (FUNCTIONS.md, TYPES.md, INCLUDES.json, examples/) have landed on main before writing the README. Only describe what's shipped.

### 5. Make Targets

Top 8-10 most-used targets, grouped by purpose. End with: "Run `make help` for the full list."

### 6. Contributing

Warmer than current. Four paths in:
1. Port something new (check Aminet first)
2. Expand the POSIX shim
3. Test on real Amiga hardware
4. Improve the ADCD knowledge base

Pointer: "See CLAUDE.md for the full contributor guide."

### 7. Acknowledgments + License

Same as current. Add ADCD source attribution.

## What Gets Cut

- **"Example Ports" section** — pipeline validation internals, not for the reader. Examples still exist in the repo.
- **"Port Categories" table** — moved to architecture docs. The README doesn't need the full taxonomy.
- **"Reference Documentation" as standalone section** — folded into the Knowledge Base pillar.
- **Standalone "Specialized Agents" and "Pipeline Skills" tables** — consolidated into the AI Pipeline pillar.

## Target Length

~200-250 lines. Same length as current but restructured for narrative flow.

## Tone

Warm but restrained. One or two sentences that show care for the Amiga platform. Otherwise technical and direct. Not generated-docs voice, not community-newsletter voice. The work speaks for itself.
