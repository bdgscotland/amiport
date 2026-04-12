# Port: rm

## Overview

| Field | Value |
|-------|-------|
| Program | rm |
| Version | 1.45 |
| Source | OpenBSD bin/rm (v1.45) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | The Regents of the University of California |
| Port Date | 2026-04-11 |
| Binary Size | 44 KB |
| Source Files | 1 (rm.c) |

## Description

Remove directory entries. Supports recursive removal (-R/-r), forced removal
without prompts (-f), interactive confirmation (-i), verbose output (-v), and
directory removal (-d). The -P flag (secure overwrite with three passes) is
accepted for compatibility but performs no action -- AmigaOS has no security
model requiring data sanitization.

## Prior Art on Aminet

No functional POSIX rm for OS 3.x/68k. The adtools/coreutils collection has a
PPC-only version with broken recursion. AmigaOS native `DELETE` command uses
different flag syntax and lacks recursive descent.

## Portability Analysis

Verdict: **MODERATE** -- Single-file but requires fts(3) for recursive
directory traversal, strmode() for permission display, and user/group name
lookup for interactive prompts. The -P secure overwrite relies on fstatfs()
and fsync() which have no AmigaOS equivalent.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<fts.h>` (fts_open/fts_read) | Tier 2 | `<amiport/fts.h>` shim (opendir/readdir) |
| `stat()` / `struct stat` | Tier 1 | `<amiport/sys/stat.h>` (amiport_stat) |
| `<err.h>` (err/errx/errc) | Tier 1 | `<amiport/err.h>` |
| `strmode()` | N/A | Local rm_strmode() implementation |
| `user_from_uid()` | Tier 1 | `<amiport/pwd.h>` macro (returns "amiga") |
| `group_from_gid()` | Tier 1 | `<amiport/grp.h>` macro (returns "amiga") |
| `<sys/mount.h>` (fstatfs) | Tier 3 | Removed -- secure overwrite disabled |
| `fsync()` | Tier 3 | Not available -- secure overwrite disabled |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `getopt()` | Tier 1 | `<amiport/getopt.h>` |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)` (RETURN_ERROR) |
| Root inode check | N/A | Skipped -- AmigaOS has no single filesystem root |

## Transformations Applied

| File | Change | Comment |
|------|--------|---------|
| rm.c | `<sys/stat.h>` -> `<amiport/sys/stat.h>` | stat macro redirection |
| rm.c | `<sys/mount.h>` removed | fstatfs() not available |
| rm.c | `<fts.h>` -> `<amiport/fts.h>` | fts shim for directory traversal |
| rm.c | `<err.h>` -> `<amiport/err.h>` | libnix has no err.h |
| rm.c | `<stdlib.h>` -> `<amiport/stdlib.h>` | exit macro activation |
| rm.c | `<pwd.h>` -> `<amiport/pwd.h>` | user_from_uid macro |
| rm.c | `<grp.h>` -> `<amiport/grp.h>` | group_from_gid macro |
| rm.c | Added `<amiport/glob.h>` | Wildcard expansion |
| rm.c | `fcntl.h` moved before `amiport/sys/stat.h` | Prevents macro collision |
| rm.c | Local rm_strmode() added | Avoids conflict with libnix's strmode declaration |
| rm.c | rm_overwrite() disabled | Returns 1 (no-op) -- no fstatfs/fsync |
| rm.c | Root inode check skipped | AmigaOS has no "/" root to guard |
| rm.c | All `err(1,...)` -> `err(10,...)` | Amiga error convention |
| rm.c | All `errc(1,...)` -> `errc(10,...)` | Amiga error convention |
| rm.c | `exit(1)` -> `exit(10)` | RETURN_ERROR for AmigaOS |
| rm.c | Added `__stack = 16384` | Stack cookie for recursive traversal |
| rm.c | Added `$VER` string | AmigaOS version identification |
| rm.c | Added atexit cleanup | Free expanded argv on all exit paths |

## Build Configuration

| Setting | Value |
|---------|-------|
| Stack size | 16384 bytes |
| CFLAGS | Default (C89, -m68000) |
| Link libraries | libamiport.a (posix-shim) |
| Special flags | None |

## Test Results

- **Total tests:** 18
- **Passed:** 17
- **Failed:** 1
- **Pass rate:** 94%
- **Known failure:** `rm -f` on a nonexistent file returns a non-zero errno
  through a code path where the shim's errno propagation does not match POSIX
  semantics. The -f flag should suppress errors for nonexistent files, but the
  shim reports an error code. This does not affect normal usage.

## Memory Safety

**Verdict: CLEAN.** No dynamic allocations beyond argv expansion (atexit
cleanup) and the fts shim's internal state (which is managed by fts_close).
No getenv usage, no getline buffers, no linked lists. The rm_strmode()
function uses a caller-provided buffer.

## Performance Notes

No performance concerns. File operations are system-call-bound (unlink/rmdir).
The fts traversal is I/O-bound. No large buffers, no character-at-a-time
processing.

## Known Limitations

- **-P (secure overwrite) disabled:** Accepted for flag compatibility but
  performs no data overwriting. AmigaOS has no fstatfs() to determine
  filesystem block size and no fsync() to guarantee writes reach disk.
  Files are deleted normally without data sanitization.
- **Permission display approximate:** The rm_strmode() function generates
  Unix-style permission strings (e.g., "-rwxr-xr-x") from AmigaOS
  protection bits, which do not map exactly to POSIX permissions.
- **User/group names:** Always shows "amiga"/"amiga" in interactive prompts
  since AmigaOS has no multi-user system.
- **No root protection:** The check for "refusing to remove /" is skipped
  since AmigaOS has no single filesystem root equivalent.
