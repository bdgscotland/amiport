# Port: touch

## Overview

| Field | Value |
|-------|-------|
| Program | touch |
| Version | 1.27 |
| Source | OpenBSD usr.bin/touch (v1.27) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-04-11 |
| Binary Size | 41 KB |
| Source Files | 1 (touch.c) |

## Description

Change file access and modification timestamps. Supports creating new files,
setting specific times via ISO 8601 (-d) or numeric (-t) formats, and copying
timestamps from a reference file (-r). On AmigaOS, only modification time is
stored -- access time is accepted but silently ignored since AmigaOS filesystems
(OFS/FFS) do not track access timestamps.

## Prior Art on Aminet

Touch13 (1994) exists on Aminet but is over 30 years old and lacks the full
POSIX flag set (-a, -c, -d, -m, -r, -t). No POSIX-compliant version is
available for OS 3.x/68k.

## Portability Analysis

Verdict: **MODERATE** -- Single-file but heavy use of POSIX time APIs
(utimensat, futimens, strptime, timegm) and stat. The stat-first pattern
required restructuring the main loop to work around an errno visibility issue.

| Issue | Tier | Resolution |
|-------|------|------------|
| `stat()` / `struct stat` | Tier 1 | `<amiport/sys/stat.h>` (amiport_stat) |
| `utimensat()` | Tier 1 | `<amiport/unistd.h>` shim |
| `futimens()` | Tier 1 | `<amiport/unistd.h>` shim |
| `strptime()` | Tier 1 | `amiport_strptime()` via `<amiport/sys/time.h>` |
| `timegm()` | Tier 1 | `amiport_timegm()` via `<amiport/unistd.h>` |
| `<err.h>` | Tier 1 | Replaced with `<amiport/err.h>` |
| `<sys/time.h>` | Tier 1 | Removed (time functions via `<time.h>`) |
| `getopt()` | Tier 1 | `<amiport/getopt.h>` |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `__dead` attribute | Stub | `#define __dead` (no-op) |
| `DEFFILEMODE` | Stub | `#define DEFFILEMODE 0666` |
| `UTIME_NOW` / `UTIME_OMIT` | Tier 1 | Provided by `<amiport/unistd.h>` |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)` (RETURN_ERROR) |
| errno after utimensat | N/A | Restructured to stat-first pattern |

## Transformations Applied

| File | Change | Comment |
|------|--------|---------|
| touch.c | `<sys/stat.h>` -> `<amiport/sys/stat.h>` | stat macro redirection |
| touch.c | `<sys/time.h>` removed | Not needed; time functions via `<time.h>` |
| touch.c | `<err.h>` -> `<amiport/err.h>` | libnix has no err.h |
| touch.c | `<stdlib.h>` -> `<amiport/stdlib.h>` | exit macro activation |
| touch.c | `<unistd.h>` -> `<amiport/unistd.h>` | utimensat, futimens, timegm |
| touch.c | Added `<amiport/getopt.h>` | libnix getopt_long broken |
| touch.c | Added `<amiport/sys/time.h>` | amiport_strptime() |
| touch.c | Added `<amiport/glob.h>` | Wildcard expansion |
| touch.c | `fcntl.h` moved before `amiport/sys/stat.h` | Prevents macro collision |
| touch.c | Main loop restructured to stat-first | errno from shim not reliably visible |
| touch.c | File creation uses libnix `open()`/`close()` | `#undef open`/`close` to bypass amiport macros (separate fd namespaces) |
| touch.c | All `err(1,...)` -> `err(10,...)` | Amiga error convention |
| touch.c | All `errx(1,...)` -> `errx(10,...)` | Amiga error convention |
| touch.c | `exit(1)` -> `exit(10)` | RETURN_ERROR for AmigaOS |
| touch.c | Added `__stack = 16384` | Stack cookie |
| touch.c | Added `$VER` string | AmigaOS version identification |
| touch.c | Added atexit cleanup | Free expanded argv on all exit paths |

## Build Configuration

| Setting | Value |
|---------|-------|
| Stack size | 16384 bytes |
| CFLAGS | Default (C89, -m68000) |
| Link libraries | libamiport.a (posix-shim) |
| Special flags | None |

## Test Results

- **Total tests:** 21
- **Passed:** 20
- **Failed:** 1
- **Pass rate:** 95%
- **Known failure:** `strptime` with `%F` format specifier (ISO 8601 YYYY-MM-DD).
  The amiport_strptime() shim does not support the `%F` shorthand. The `-d`
  flag works correctly with explicit format strings like `%Y-%m-%dT%H:%M:%S`.

## Memory Safety

**Verdict: CLEAN.** No dynamic allocations beyond argv expansion (atexit
cleanup). The stat-first pattern uses stack-allocated `struct amiport_stat`.
All `err()`/`errx()` exit paths go through atexit for cleanup. No getenv
usage, no linked lists, no growth buffers.

## Performance Notes

No performance concerns. File operations are one-shot per argument. No loops
over file content, no large buffers.

## Known Limitations

- **Access time not supported:** AmigaOS filesystems (OFS/FFS) only store
  modification time. The `-a` flag is accepted but has no effect.
- **strptime %F not supported:** The `%F` format specifier in `-d` timestamps
  is not recognized. Use `-d "2024-01-15T12:00:00"` with explicit format.
- **File permissions ignored:** The `DEFFILEMODE` (0666) is used for file
  creation but AmigaOS does not enforce Unix permission bits. All created
  files get standard AmigaOS protection bits.
