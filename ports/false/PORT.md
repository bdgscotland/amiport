# Port: false

## Overview

| Field | Value |
|-------|-------|
| Program | false |
| Version | 1.1 |
| Source | OpenBSD usr.bin/false (v1.1, November 2015) |
| Category | 1 -- CLI tool |
| License | Public Domain |
| Original Author | Theo de Raadt |
| Port Date | 2026-03-26 |

## Description

false is a shell utility that does nothing and exits with a non-zero (failure) status. It is used in shell scripts to force failure conditions, as a conditional that always fails, and to test error-handling paths. The exit code mapping from POSIX to AmigaOS is the entire substance of this port: POSIX `exit(1)` is invisible to AmigaDOS scripts, so false must return RETURN_ERROR (10) to trigger `IF ERROR` correctly.

## Prior Art on Aminet

No existing false port found on Aminet for AmigaOS 3.x.

## Portability Analysis

Verdict: **TRIVIAL** -- The original source is 4 lines of C. The only porting concern is the exit code mapping.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `return (1)` exit code | Tier 1 | Changed to `return (10)` -- RETURN_ERROR for AmigaDOS |
| No version string | Amiga | Added `$VER` string |
| No stack cookie | Amiga | Added `__stack = 8192` |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| false.c | 5 | (none) | `#include <amiport/stdlib.h>` | Activates exit() -> amiport_exit() macro |
| false.c | 9 | (none) | `static const char *verstag = "$VER: false 1.1 (26.03.2026)";` | Amiga version string with `__attribute__((used))` |
| false.c | 12 | (none) | `long __stack = 8192;` | Stack cookie for AmigaOS |
| false.c | 19 | `return (1);` | `return (10);` | POSIX EXIT_FAILURE (1) is invisible to AmigaDOS; RETURN_ERROR (10) triggers `IF ERROR` |

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (Tier 1 posix-shim) |
| Stack size | 8192 bytes |
| Binary size | ~3 KB |

## Test Results

14 tests, all passing. Tested via FS-UAE (Category 1 CLI tool).

| Category | Count | Description |
|----------|-------|-------------|
| Functional | 5 | Basic invocation, single/multiple args, --help, --version |
| Output verification | 2 | No stdout output with or without args |
| AmigaDOS correctness | 2 | RC is exactly 10 (not 1 or 5) -- validates the exit code mapping |
| Edge cases | 1 | Double-dash argument ignored |
| Stress | 4 | Numeric arg, flag-style arg, 10 simultaneous args, file path arg |

The AmigaDOS correctness tests are the most important: they verify that the exit code is exactly 10 (RETURN_ERROR), not 1 (POSIX EXIT_FAILURE, invisible to AmigaDOS) or 5 (RETURN_WARN, wrong semantics).

## Memory Safety

Verdict: **CLEAN** -- No heap allocations. No dynamic memory. The program consists of a single `return 10` statement. No cleanup needed.

## Performance Notes

N/A -- Trivial program. Executes a single return instruction.

## Known Limitations

None. The program is functionally identical to the OpenBSD original, with the exit code correctly mapped to AmigaDOS semantics.
