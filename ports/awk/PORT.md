# Port: awk

## Overview

| Field | Value |
|-------|-------|
| Program | awk |
| Version | 2024.12.25 |
| Source | Brian W. Kernighan's "One True Awk" (github.com/onetrueawk/awk) |
| Category | 2 -- Scripting Interpreter |
| License | MIT-like |
| Original Author | Brian W. Kernighan |
| Port Date | 2026-03-25 |

## Description

AWK is a pattern scanning and processing language. This is Brian Kernighan's "One True Awk" -- the canonical implementation by the "K" in AWK, actively maintained. Replaces the 30-year-old ATT-awk-1_0 on Aminet (1994).

## Prior Art on Aminet

ATT-awk-1_0 (1994) exists on Aminet -- compiled with SAS/C 6.51, unmodified AT&T source from April 1994. Missing 30 years of features, bug fixes, and enhancements. No Unicode, no CSV support, no modern error handling. A fresh port is warranted.

## Portability Analysis

Verdict: **MODERATE** -- 11 source files, ~12K LOC, Category 2 scripting interpreter.

| Issue | Tier | Resolution |
|-------|------|------------|
| `popen()`/`pclose()` | 2 | `amiport_emu_popen()` (temp file emulation) |
| `system()` + W* macros | 2 | `amiport_emu_system()` + W* macro stubs |
| `MB_CUR_MAX` | pitfall | Force `awk_mb_cur_max = 1` on AmigaOS |
| `random()`/`srandom()` | 1 | `#define random() rand()` / `srand()` |
| `/dev/null` | arch | Replace with `NIL:` |
| `stat()` / `S_ISDIR()` | 1 | `amiport_stat()` |
| `fcntl(FD_CLOEXEC)` | 1 | Stub as no-op |
| `signal(SIGFPE)` | 1 | libnix ignores silently |
| `%zu` format | arch | Replace with `%lu` + cast |
| C99 features | -- | `-std=gnu99` (ADR-022) |

## Transformations Applied

<!-- Populated after Stage 3 -->

## Shim Functions Exercised

- `amiport_emu_popen()` / `amiport_emu_pclose()` from `<amiport-emu/popen.h>`
- `amiport_emu_system()` from `<amiport-emu/popen.h>`
- `amiport_stat()` / `S_ISDIR()` from `<amiport/stat.h>`
- `err()` from `<amiport/err.h>` (if applicable)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -std=gnu99` |
| Libraries | `-lamiport` (posix-shim), `-lamiport-emu` (popen), `-lm` (math) |
| Binary size | |

## Test Results

<!-- Populated after Stage 5 -->

## Platform Compatibility Notes

- `MB_CUR_MAX` forced to 1 -- multibyte/UTF-8 disabled (AmigaOS has no wchar support)
- `popen` uses temp files, not concurrent pipes -- awk `print | "cmd"` works but is not concurrent
- `system()` returns AmigaDOS RC directly (0/5/10/20), not Unix wait-status

## Known Bugs (WIP -- not yet resolved)

- **libnix `%g` format bug**: `print` outputs integers as `1.0000000000` instead of `1`. The OFMT is set to `"%.6g"` which should suppress trailing zeros, but libnix's printf does not implement `%g` correctly. This affects all numeric output via `print` (not `printf "%d"`). Root cause needs investigation on FS-UAE.
- **Floating point accumulation drift**: `fib(20)` returns `6764.999...` instead of `6765`. Integer arithmetic via `for (i=1; i<=10000; i++) sum+=i` returns `50004999.999...`. This may be related to the `%g` issue or a genuine 68k FPU precision problem.
- **split() array access**: `split($0, parts, ":")` returns correct count but `parts[1]` appears empty in output. Needs investigation.
- **Test suite status**: 44 tests with strict expectations. ~12 tests expected to fail due to the above bugs. Tests are NOT weakened -- they fail honestly.

## Known Limitations

- Pipe commands (`print | "cmd"`, `"cmd" | getline`) use temp files -- not concurrent
- Shell syntax in pipe commands limited to what AmigaDOS supports (no `&&`, `||`, backticks)
- Multibyte/UTF-8 disabled -- operates on raw bytes (correct for ISO 8859-1)
- SIGFPE not delivered -- math errors caught via errno checking instead
- ENVIRON array is empty (AmigaOS has no environ)
- exit code for syntax errors is 2 (below AmigaDOS RETURN_WARN threshold of 5)

## Review

<!-- Populated after Stage 6 -->
