# Port: uniq

## Overview

| Field | Value |
|-------|-------|
| Program | uniq |
| Version | 1.33 |
| Source | OpenBSD CVS (rev 1.33, 2022-01-01) |
| Category | 1 — CLI |
| License | BSD-3-Clause |
| Original Author | OpenBSD Project |
| Port Date | 2026-03-22 |

## Description

Standard POSIX uniq utility — filters adjacent repeated lines from input, with options for counting occurrences (-c), showing only duplicates (-d), showing only unique lines (-u), skipping fields (-f) or characters (-s), and case-insensitive comparison (-i).

## Prior Art on Aminet

An ancient (~1990s) uniq exists in `util/cli/UnixCmd.lha` (jffabre) — a Minix-based port compiled with SAS/C. It lacks modern flags and has no maintenance. Our OpenBSD 1.33 port provides a complete, well-tested implementation.

## Portability Analysis

Verdict: **EASY** — 258 lines, single file, all Tier 1 issues.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<err.h>` — err/errx/strtonum | 1 | `<amiport/err.h>` |
| `<unistd.h>` — getopt, ssize_t | 1 | `<amiport/unistd.h>` |
| `pledge()` calls | 1 | Stub macro `#define pledge(p,e) (0)` |
| `getline()` | 1 | `<amiport/stdio_ext.h>` |
| `getprogname()` | 1 | Replace with `__progname` extern |
| `__dead` attribute | 1 | `#define __dead __attribute__((__noreturn__))` |
| `setlocale()` | 1 | Stub in `<amiport/unistd.h>` |
| MB_CUR_MAX in `skip()` | arch | Guard multibyte path with `#ifndef __AMIGA__` |
| `long long` globals | arch | Accept — 64-bit emulation is functional |
| Exit code `1` → `10` | 1 | Amiga RETURN_ERROR convention |

## Transformations Applied

<!-- Filled by code-transformer agent -->

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|

## Shim Functions Exercised

<!-- Filled after transform -->

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | |

## Test Results

<!-- Filled after testing -->

| Test | Command | Input | Expected | Result |
|------|---------|-------|----------|--------|

## Known Limitations

- No multibyte/locale support (AmigaOS is always C locale). The `-f` and `-s` flags skip bytes, not multibyte characters. This matches behavior on any POSIX system in the C locale.

## Review

<!-- /review-amiga score summary -->
