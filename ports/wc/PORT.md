# Port: wc

## Overview

| Field | Value |
|-------|-------|
| Program | wc |
| Version | 1.32 |
| Source | OpenBSD wc v1.32 (BSD 3-Clause) |
| Category | 1 — CLI tool |
| License | BSD 3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-22 |

## Description

wc counts the number of lines, words, and characters (bytes) in files or standard input. Supports flags -l (lines), -w (words), -c (bytes), -h (human-readable sizes). The -m (multibyte characters) flag is accepted but equivalent to -c on AmigaOS (no multibyte locale support).

## Prior Art on Aminet

**WordCount v37.10** (by Torsten Poulin, 2021) exists on Aminet as `util/cli/WordCount`. It's a standalone 68k binary with full wc functionality (-l, -w, -c modes). BSD 2-Clause licensed, no ixemul dependency.

This port provides the actual OpenBSD wc v1.32 as part of a cohesive OpenBSD toolset (alongside grep, sed, cut, head, tail, etc.) and exercises additional shim functions (fstat, read, getopt, argv expansion).

## Portability Analysis

Verdict: **EASY** — All Tier 1. Single-file CLI tool with standard file I/O.

| Issue | Tier | Resolution |
|-------|------|------------|
| `open()`, `read()`, `close()`, `fstat()` | 1 | posix-shim wrappers |
| `getopt()` | 1 | posix-shim |
| `err()`/`warn()` | 1 | posix-shim |
| `getline()` | 1 | posix-shim |
| `pledge()` | 1 | Stub macro |
| `setlocale()` | 1 | Stub (always "C") |
| `<util.h>` / `fmt_scaled()` | 1 | Inline implementation |
| `<wchar.h>` / `<wctype.h>` / multibyte path | 1 | Guard with `#ifdef __AMIGA__` — unreachable (MB_CUR_MAX==1) |
| Exit codes (1 → 10) | 1 | Amiga exit code convention |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| | | | To be filled by code-transformer |

## Shim Functions Exercised

- `amiport_open()`
- `amiport_read()`
- `amiport_close()`
- `amiport_fstat()`
- `amiport_getopt()` (via macro)
- `amiport_err()` / `amiport_warn()` (via macros)
- `amiport_getline()` (via macro)
- `amiport_expand_argv()` / `amiport_free_argv()`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | |

## Test Results

<!-- To be filled after testing -->

| Test | Command | Input | Expected | Result |
|------|---------|-------|----------|--------|
| | | | | |

## Known Limitations

- `-m` flag accepted but equivalent to `-c` (no multibyte locale on AmigaOS 3.x)
- `-h` flag uses a simplified `fmt_scaled()` implementation (inlined, not OpenBSD libutil)
- `fstat()` on directories returns `st_size=0` (AmigaOS FFS/SFS behaviour)

## Review

<!-- /review-amiga score summary -->
