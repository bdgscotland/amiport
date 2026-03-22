# PDR-005: Compiled Binaries as Committed Artifacts

## Status

Accepted

## Date

2026-03-21

## Problem

Most software projects `.gitignore` compiled binaries — they're ephemeral build output, reproducible from source. But amiport's target audience includes Amiga enthusiasts who want to run ported Unix utilities on their Amiga without setting up a cross-compilation toolchain. The toolchain requires Docker, bebbo-gcc, and amitools — a significant barrier for someone who just wants `grep` on their A1200.

## Target Users

- **Amiga enthusiasts** who want pre-built binaries they can copy to their Amiga and run immediately
- **Retro computing hobbyists** who may not have Docker or a modern development environment
- **Contributors** who want to test existing ports without building the toolchain first

## Decision

Compiled Amiga binaries (`ports/<name>/<name>`) are committed to git and treated as distributable artifacts, not build ephemera. The git repository serves dual purpose: source archive and binary distribution channel.

Rules:
- The binary at `ports/<name>/<name>` is always committed after a successful build+test
- Only the final stripped binary is committed — no `.o` files, no `gmon.out`, no `*_native` comparison binaries
- Debug builds (with STABS info) are committed during development; stripped binaries replace them at `make package` time
- `.gitignore` explicitly does NOT exclude `ports/**/<name>` binaries

## Rationale

- **Zero-friction distribution** — `git clone` gives you working Amiga binaries, no build step required
- **Aminet alignment** — Aminet distributes pre-built binaries alongside source. Having binaries in the repo means the packaging step (`make package`) just bundles what's already there
- **Reproducibility is preserved** — source is in `ported/`, Makefile is in the port directory, anyone CAN rebuild from source if they want to
- **Git handles it fine** — Amiga CLI binaries are typically 10-50KB. Even with dozens of ports, binary size is negligible compared to the ADCD reference docs

## Success Criteria

- Users can clone the repo, copy a binary to their Amiga (via SD card, network, or floppy), and run it
- `git clone` + `ls ports/*/` shows all available pre-built ports
- Binary in repo matches what `make build TARGET=ports/<name>` produces

## Alternatives Considered

- **GitHub Releases for binaries**: Adds a separate distribution channel to manage. Users must know to check Releases, not just the repo. Loses the simplicity of "everything in one place."
- **Build-on-demand only**: Users must install Docker + bebbo-gcc + amitools before they can use any port. High barrier for the target audience.
- **Separate binary repo**: Doubles the maintenance burden. Source and binary drift out of sync. Two repos to clone instead of one.
