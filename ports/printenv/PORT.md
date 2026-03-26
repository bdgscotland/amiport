# Port: printenv

## Overview

| Field | Value |
|-------|-------|
| Program | printenv |
| Version | 1.8 (port revision: 1) |
| Source | OpenBSD (GitHub mirror) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-26 |

## Description

printenv prints environment variables. With no arguments, it enumerates all variables in `NAME=VALUE` format. With a variable name as an argument, it prints that variable's value. On AmigaOS, the POSIX `environ[]` array does not exist under `-noixemul`, so the port uses AmigaDOS `ExAll()` on `ENV:` to enumerate variables and `GetVar()` with `GVF_GLOBAL_ONLY` to look up individual values.

## Prior Art on Aminet

No existing standalone 68k port of OpenBSD printenv found on Aminet. AmigaDOS provides `GetEnv` and `SetEnv` commands but no equivalent that enumerates all environment variables in POSIX `NAME=VALUE` format. This port bridges that gap for scripts ported from Unix.

## Portability Analysis

Verdict: **EASY** -- Single file (201 lines), but required a significant architectural change: the `environ[]` global does not exist on AmigaOS `-noixemul`, so the entire enumeration path was rewritten using AmigaDOS `ExAll()` on `ENV:` and `GetVar()`.

| Issue | Tier | Resolution |
|-------|------|------------|
| `extern char **environ` | Tier 3 | Replaced entirely with AmigaDOS ExAll() on ENV: + GetVar() |
| `<err.h>` | Tier 1 | Replaced with `<amiport/err.h>` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `exit(1)` | Tier 1 | Changed to `exit(10)` (RETURN_ERROR) |
| `err(1,...)` | Tier 1 | Changed to `err(10,...)` |
| Wildcard globbing | Tier 1 | `amiport_expand_argv()` added |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| printenv.c | 37-46 | `#include <stdlib.h>`, `<unistd.h>`, `<err.h>` | amiport equivalents | Standard header replacement |
| printenv.c | 48-52 | (none) | `#include <proto/dos.h>`, `<proto/exec.h>`, `<dos/exall.h>` | AmigaOS headers for ExAll/GetVar |
| printenv.c | 55 | `pledge()` call | `#define pledge(p, e) (0)` | OpenBSD sandbox stub |
| printenv.c | 58 | (none) | `$VER: printenv 1.8` | Amiga version string |
| printenv.c | 61 | (none) | `long __stack = 8192` | Stack cookie |
| printenv.c | 85-155 | `for (ep = environ; *ep; ep++) puts(*ep)` | `print_all_env()` using ExAll()+GetVar() | Complete rewrite of enumeration |
| printenv.c | 173 | (none) | `amiport_expand_argv()` | Wildcard expansion |
| printenv.c | 174 | (none) | `atexit(amiport_free_argv)` | Cleanup on all exit paths |
| printenv.c | 178 | `err(1, "pledge")` | `err(10, "pledge")` | Amiga RETURN_ERROR |
| printenv.c | 180-184 | `for (ep = environ; *ep; ep++)` iterate | `print_all_env()` call | AmigaOS ExAll enumeration |
| printenv.c | 191 | `strncmp(*ep, *argv, len)` | `GetVar(*argv, val, ...)` | Direct AmigaDOS variable lookup |
| printenv.c | 200 | `exit(1)` | `exit(10)` | Amiga RETURN_ERROR |

## Shim Functions Exercised

- `amiport_expand_argv()` -- wildcard expansion
- `amiport_free_argv()` -- free expanded argv via atexit
- `amiport_exit()` -- via exit macro
- `amiport_err()` -- error reporting

## AmigaOS APIs Used Directly

- `Lock()` / `UnLock()` -- lock ENV: directory for ExAll enumeration
- `AllocDosObject()` / `FreeDosObject()` -- ExAllControl struct per ADCD requirements
- `ExAll()` -- enumerate files in ENV: (each file is an environment variable)
- `GetVar()` with `GVF_GLOBAL_ONLY` -- read variable values from ENV:

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Stack size | 8192 bytes |
| Binary size | 27 KB |

## Test Results

Tested via FS-UAE (A1200, Kickstart 3.1). ARexx harness, TAP output. Tests use ARexx wrapper scripts to set/query environment variables via `SetEnv`/`GetEnv` within the test harness.

| Metric | Value |
|--------|-------|
| Total tests | 17 |
| Passed | 17 |
| Failed | 0 |
| Pass rate | 100% |

Categories tested: single variable lookup, multiple arguments (only first processed), special characters in values, missing variable error (RC=10), exit code verification, no-args enumeration mode (ExAll on ENV:), NAME=VALUE format verification, Amiga-specific (Language variable from Startup-Sequence, GVF_GLOBAL_ONLY), edge cases (200-char value near ENV_VAL_BUF limit), stress tests (20 simultaneous variables, buffer boundary).

## Memory Safety

**Verdict: CLEAN** -- Approved for shipping (memory-checker agent, 2026-03-26).

All allocations properly paired with cleanup:
- argv expansion freed via `atexit(amiport_free_argv)`
- `Lock()` / `UnLock()` paired on all paths (including early error returns)
- `AllocDosObject()` / `FreeDosObject()` paired on all paths
- `exall_buf[1024]` is `static` (not stack-allocated)
- No amiport_getenv() usage (uses GetVar() directly)
- Stack usage conservative: `val[256]` on stack + ~300 bytes locals, within 8KB `__stack`

## Performance Notes

No perf-optimizer report generated. The program performs a small number of AmigaDOS calls (ExAll loop or single GetVar) and exits. No hot loops or optimization opportunities.

## Known Limitations

1. **ENV_VAL_BUF is 256 bytes** -- Variable values longer than 255 characters are silently truncated by GetVar(). This covers the vast majority of environment variables but could truncate very long PATH-like values.
2. **No-args mode only shows top-level ENV: files** -- Subdirectories in ENV: (like `ENV:Sys/`) are not recursed. This matches POSIX behavior (shell variables, not system configuration).
3. **Only first argument processed** -- `printenv NAME1 NAME2` only prints NAME1, ignoring NAME2. This matches OpenBSD behavior but differs from GNU printenv which prints all named variables.
