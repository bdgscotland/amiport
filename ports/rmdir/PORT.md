# Port: rmdir

## Overview

| Field | Value |
|-------|-------|
| Program | rmdir |
| Version | 1.15 (port revision: 1) |
| Source | OpenBSD (GitHub mirror) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-26 |

## Description

rmdir removes empty directories specified as arguments. With the `-p` flag, it also removes each parent directory component of the given path, walking backward from the leaf to the root (stopping at the volume separator). This is useful for cleaning up nested directory trees after their contents have been removed.

## Prior Art on Aminet

No existing standalone 68k port of OpenBSD rmdir found on Aminet. AmigaDOS has a built-in `Delete` command that can remove directories, but it lacks the `-p` recursive parent-removal feature. This port provides the POSIX rmdir interface with parent directory stripping.

## Portability Analysis

Verdict: **EASY** -- Single file (147 lines), zero Tier 2/3 blockers. Only 5 POSIX functions beyond stdio: rmdir(), getopt(), warn(), err(), pledge().

| Issue | Tier | Resolution |
|-------|------|------------|
| `rmdir()` | Tier 1 | `amiport_rmdir()` via `<amiport/dirent.h>` (uses AmigaDOS DeleteFile) |
| `getopt()` | Tier 1 | `<amiport/getopt.h>` (libnix getopt_long is broken) |
| `warn()` / `err()` | Tier 1 | `<amiport/err.h>` |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `__dead` macro | BSD-ism | `#define __dead __attribute__((__noreturn__))` |
| `__progname` | Tier 1 | Provided by `amiport_expand_argv()` -- extern declaration only |
| Exit codes | Tier 1 | `exit(1)` changed to `exit(10)` (RETURN_ERROR) |
| Wildcard globbing | Tier 1 | `amiport_expand_argv()` added |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| rmdir.c | 34 | (none) | `#define __dead __attribute__((__noreturn__))` | BSD macro stub |
| rmdir.c | 37 | (none) | `#define pledge(p, e) (0)` | OpenBSD sandbox stub |
| rmdir.c | 41 | `#include <err.h>` | `#include <amiport/err.h>` | amiport shim |
| rmdir.c | 45 | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | exit() macro |
| rmdir.c | 48 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | amiport shim |
| rmdir.c | 50 | (none) | `#include <amiport/dirent.h>` | rmdir() macro |
| rmdir.c | 52 | (none) | `#include <amiport/glob.h>` | argv expansion + __progname |
| rmdir.c | 54 | (none) | `#include <amiport/getopt.h>` | Working getopt |
| rmdir.c | 57 | (none) | `$VER: rmdir 1.15` | Amiga version string |
| rmdir.c | 60 | (none) | `long __stack = 8192` | Stack cookie |
| rmdir.c | 69-74 | (none) | `cleanup()` function | atexit handler |
| rmdir.c | 83 | (none) | `amiport_expand_argv()` | Wildcard expansion |
| rmdir.c | 85 | (none) | `atexit(cleanup)` | Register cleanup |
| rmdir.c | 88 | `err(1, "pledge")` | `err(10, "pledge")` | Amiga RETURN_ERROR |
| rmdir.c | 145 | `exit(1)` | `exit(10)` | Amiga RETURN_ERROR in usage() |

## Shim Functions Exercised

- `amiport_rmdir()` -- remove directory via AmigaDOS DeleteFile
- `amiport_expand_argv()` -- wildcard expansion
- `amiport_free_argv()` -- free expanded argv in atexit cleanup
- `amiport_exit()` -- via exit macro
- `amiport_err()`, `amiport_warn()` -- error reporting
- `amiport_getopt()` -- option parsing (replaces broken libnix getopt_long)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Stack size | 8192 bytes |
| Binary size | 34 KB |

## Test Results

Tested via FS-UAE (A1200, Kickstart 3.1). ARexx harness, TAP output.

| Metric | Value |
|--------|-------|
| Total tests | 38 |
| Passed | 38 |
| Failed | 0 |
| Pass rate | 100% |

Categories tested: basic removal, nonexistent directory error, non-empty directory error, usage/invalid flag errors (RC=10), `-p` flag (2-level, 3-level, single-level), multiple directories, partial failure, Amiga T: volume paths, edge cases (volume removal attempt, double-slash path), real-world scenarios (build cleanup, deep hierarchy), stress tests (5-dir removal, partial failure with 5 args), precision exit code verification.

## Memory Safety

**Verdict: CLEAN** -- Approved for shipping (memory-checker agent, 2026-03-26).

All dynamic allocations (argv expansion) paired with atexit cleanup. No manual free() calls, no double-free risk. rm_path() operates on the passed-in string (modifies in place), no allocations. Correct extern __progname usage (not redefined).

## Performance Notes

No perf-optimizer report generated. The program makes a small number of system calls (rmdir per argument) and exits. No hot loops or optimization opportunities.

## Known Limitations

1. **rm_path() uses `/` separator only** -- The `-p` flag walks backward looking for `/` separators. AmigaDOS volume-rooted paths like `T:dirname` contain no `/`, so rm_path() correctly does nothing after removing the leaf directory. This is correct behavior but means `-p` only removes POSIX-style nested paths (e.g., `T:a/b/c`), not AmigaDOS-style ones.
2. **Return code 1 on partial failure** -- When some removals succeed and others fail, the program returns 1 (not an Amiga-standard code). This is inherited from OpenBSD upstream. The `usage()` path correctly returns 10 (RETURN_ERROR).
