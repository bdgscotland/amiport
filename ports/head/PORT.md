# Port: head

## Overview

| Field | Value |
|-------|-------|
| Program | head |
| Version | 1.24 |
| Source | OpenBSD src (head.c,v 1.24 2022/02/07) |
| Category | 1 — CLI tool |
| License | BSD 3-clause |
| Original Author | Bill Joy / UC Berkeley |
| Port Date | 2026-03-22 |

## Description

Print the first N lines (default 10) of each file to standard output. Supports `-n count` and legacy `-count` syntax. Multiple files get `==> filename <==` headers.

## Prior Art on Aminet

UnixCmd (1994) includes a `head` but is 32 years old, limited flags, SAS/C compiled. No modern standalone head exists for 68k AmigaOS 3.x. Upgrade justified.

## Portability Analysis

Verdict: **EASY** — pure stdio/file I/O, single file, 130 lines, all Tier 1 issues.

| Issue | Tier | Resolution |
|-------|------|------------|
| `pledge()` | 1 | Macro stub `#define pledge(p,e) (0)` |
| `getopt()` | 1 | `<amiport/unistd.h>` provides `amiport_getopt()` |
| `err()`/`errx()`/`warn()` | 1 | `<amiport/err.h>` provides shim macros |
| `strtonum()` | 1 | `<amiport/err.h>` provides `amiport_strtonum()` |
| Exit code 1 | — | Changed to 10 (RETURN_ERROR) |
| No wildcard expansion | — | Added `amiport_expand_argv()` |
| Missing stack cookie | — | Added `long __stack = 16384` |
| Missing version string | — | Added `$VER: head 1.24 (22.03.2026)` |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|----------|----------|-------------|---------|
| — | `<err.h>` | `<amiport/err.h>` | Shim for err/errx/warn/strtonum |
| — | `<unistd.h>` | `<amiport/unistd.h>` | Shim for getopt |
| — | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit() macro |
| — | `pledge(...)` | `#define pledge(p,e) (0)` | OpenBSD sandbox — no Amiga equivalent |
| — | `exit(1)`, `err(1,...)`, `return 1` | `exit(10)`, `err(10,...)`, `return 10` | Amiga exit code convention |
| — | — | `long __stack = 16384` | Stack cookie |
| — | — | `$VER: head 1.24` | AmigaOS version string |

## Shim Functions Exercised

- `amiport_err()` / `amiport_errx()` / `amiport_warn()` — error reporting
- `amiport_strtonum()` — safe string-to-number conversion
- `amiport_getopt()` — POSIX option parsing
- `amiport_exit()` — clean exit (via stdlib.h macro)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | 37,600 bytes |

## FS-UAE Crash Investigation (2026-03-22)

**Symptom:** ACPU_LineF (alert 0x8000000B) on FS-UAE. Vamos tests passed.

**Root cause:** A stale `examples/head/head` binary (from March 19, `$VER: head 1.0`) was shadowing our port binary during `install-emu`. The install script copied ports first, then examples overwrote them — so FS-UAE was running the old crashing example, not our port.

**Fix:** Deleted stale `examples/head/`. Fixed `install-emu.sh` to skip examples when a port with the same name exists (ports take priority).

**Defensive changes kept:** The debug-agent made `char buf[4096]` static and increased `__stack` to 65536 during investigation. These are good practice even though they weren't the crash cause — large stack buffers are genuinely dangerous on real AmigaOS (see crash-patterns.md #10).

## Test Results

Tested via vamos and FS-UAE. **21/21 FS-UAE tests passed.**

Test suite covers: functional (8), error path (4), exit code (2), edge case (4), Amiga-specific (3).

## Known Limitations

- No `-c` (byte count) flag — OpenBSD head v1.24 doesn't implement it either
- AmigaOS shells don't expand wildcards — `amiport_expand_argv()` handles `#?` patterns

## Review

Reviewed with `/review-amiga`. Score: **READY**. No critical issues. All 6 automated logic-bug checks clean. Memory audit found 2 leaks on err()/exit() paths — fixed with atexit(cleanup). fclose(stdin) crash — fixed with guard. Perf-optimized: getc/putchar loop replaced with fgets/fputs (~40x fewer branches per line). Crash-fixed: buf[4096] moved to static after FS-UAE ACPU_LineF crash (2026-03-22).
