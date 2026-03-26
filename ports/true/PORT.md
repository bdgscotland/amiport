# Port: true

## Overview

| Field | Value |
|-------|-------|
| Program | true |
| Version | 1.1 |
| Source | OpenBSD usr.bin/true (v1.1, November 2015) |
| Category | 1 -- CLI tool |
| License | Public Domain |
| Original Author | Theo de Raadt |
| Port Date | 2026-03-26 |

## Description

true is a shell utility that does nothing and exits with a zero (success) status. It is used in shell scripts as a no-op command, as a conditional that always succeeds, and as a placeholder in loop constructs. On AmigaOS, RC=0 means RETURN_OK, so `IF WARN`, `IF ERROR`, and `IF FAIL` tests all pass correctly (no branch fires).

## Prior Art on Aminet

No existing true port found on Aminet for AmigaOS 3.x.

## Portability Analysis

Verdict: **TRIVIAL** -- The original source is 4 lines of C with no POSIX dependencies beyond `return 0`.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| No version string | Amiga | Added `$VER` string |
| No stack cookie | Amiga | Added `__stack = 8192` |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| true.c | 6 | (none) | `#include <amiport/stdlib.h>` | Activates exit() -> amiport_exit() macro |
| true.c | 9 | (none) | `static const char *verstag = "$VER: true 1.1 (26.03.2026)";` | Amiga version string |
| true.c | 12 | (none) | `long __stack = 8192;` | Stack cookie for AmigaOS |

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

13 tests, all passing. Tested via FS-UAE (Category 1 CLI tool).

| Category | Count | Description |
|----------|-------|-------------|
| Functional | 5 | Basic invocation, single/multiple args, --help, --version |
| Output verification | 2 | No stdout output with or without args |
| Edge cases | 3 | Double-dash, empty string arg, file path arg |
| Stress | 3 | Numeric arg, special chars, 10 simultaneous args |

All tests verify RC=0 and no stdout output. The program ignores all arguments silently.

## Memory Safety

Verdict: **CLEAN** -- No heap allocations. No dynamic memory. The program consists of a single `return 0` statement. No cleanup needed.

## Performance Notes

N/A -- Trivial program. Executes a single return instruction.

## Known Limitations

None. The program is functionally identical to the OpenBSD original.
