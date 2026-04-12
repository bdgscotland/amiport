# Port: logname

## Overview

| Field | Value |
|-------|-------|
| Program | logname |
| Version | 1.10 |
| Source | OpenBSD usr.bin/logname (v1.10) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-04-11 |
| Binary Size | 34 KB |
| Source Files | 1 (logname.c) |

## Description

Prints the login name of the current user. On AmigaOS, which has no multi-user
login system, the amiport_getlogin() shim always returns "amiga". This utility
exists for script compatibility -- POSIX shell scripts that call `logname` or
check `$LOGNAME` will get a reasonable value instead of an error.

## Prior Art on Aminet

No standalone logname utility found on Aminet. The adtools/coreutils collection
includes logname in source form but no distributed 68k binary exists for OS 3.x.

## Portability Analysis

Verdict: **TRIVIAL** -- Single-file, minimal POSIX dependencies. The only
non-trivial issue is `getlogin()` which has no AmigaOS equivalent and is
shimmed to return a constant.

| Issue | Tier | Resolution |
|-------|------|------------|
| `getlogin()` | Tier 1 | `amiport_getlogin()` returns "amiga" |
| `<err.h>` | Tier 1 | Replaced with `<amiport/err.h>` |
| `<stdlib.h>` exit() | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `getopt()` | Tier 1 | `<amiport/getopt.h>` (libnix getopt_long broken) |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `__dead` attribute | Stub | `#define __dead` (no-op) |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)` (RETURN_ERROR) |
| `__progname` | Tier 1 | Provided by `amiport_expand_argv()` |

## Transformations Applied

| File | Change | Comment |
|------|--------|---------|
| logname.c | `<err.h>` -> `<amiport/err.h>` | libnix has no err.h |
| logname.c | `<stdlib.h>` -> `<amiport/stdlib.h>` | Activates exit->amiport_exit macro |
| logname.c | Added `<amiport/getopt.h>` | libnix getopt_long broken (crash-patterns #17) |
| logname.c | Added `<amiport/glob.h>` | amiport_expand_argv/amiport_free_argv |
| logname.c | Added `<amiport/pwd.h>` | getlogin() -> amiport_getlogin() shim |
| logname.c | Added `__stack = 4096` | Amiga stack cookie |
| logname.c | Added `$VER` string | AmigaOS version identification |
| logname.c | `__dead` attribute removed | Not supported by bebbo-gcc |
| logname.c | `pledge()` stubbed | No-op macro |
| logname.c | `err(1,...)` -> `err(10,...)` | Amiga error convention (RETURN_ERROR=10) |
| logname.c | `exit(1)` -> `exit(10)` | Amiga error convention |
| logname.c | Added atexit cleanup | Free expanded argv, flush stdout |

## Build Configuration

| Setting | Value |
|---------|-------|
| Stack size | 4096 bytes |
| CFLAGS | Default (C89, -m68000) |
| Link libraries | libamiport.a (posix-shim) |
| Special flags | None |

## Test Results

- **Total tests:** 13
- **Passed:** 13
- **Pass rate:** 100%
- **Notable findings:** All tests pass including the --version/--help flags,
  getlogin output verification, and error path tests. The output is always
  "amiga" on AmigaOS regardless of context.

## Memory Safety

**Verdict: CLEAN.** No dynamic allocations beyond argv expansion, which is
properly freed via atexit cleanup. No getenv leaks, no getline buffers, no
linked list allocations. The program is essentially a single printf call.

## Performance Notes

No performance concerns. The program executes a single function call and
prints one line. No loops, no file I/O, no recursion.

## Known Limitations

- Always returns "amiga" -- AmigaOS has no multi-user login system and no
  equivalent of utmp/wtmp databases.
- The `LOGNAME` environment variable is not consulted (matches POSIX spec,
  which says logname returns the login name, not the environment variable).
