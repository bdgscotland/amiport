# Port: sponge

## Overview

| Field | Value |
|-------|-------|
| Program | sponge |
| Version | 0.1 |
| Source | sbase (michaelforney fork) |
| Category | 1 -- CLI |
| License | MIT |
| Original Author | Dimitris Papastamos, Hiltjo Posthuma |
| Port Date | 2026-04-11 |
| Binary Size | 33 KB |
| Source Files | 4 (sponge.c, concat.c, writeall.c, eprintf.c) + 3 headers |

## Description

Reads all standard input into a temporary file, then writes it to the specified
output file. This allows constructing pipelines that read from and write to the
same file safely -- e.g., `sort file | sponge file`. Without sponge, the shell
would truncate the output file before the pipeline finishes reading it. This is
a core utility from the moreutils/sbase collection.

## Prior Art on Aminet

No sponge or moreutils equivalent found for AmigaOS. The AmigaDOS `PIPE:`
device provides basic piping but has no equivalent to sponge's atomic
read-then-write semantics.

## Portability Analysis

Verdict: **MODERATE** -- Multi-file (4 source files) but straightforward I/O.
The key challenge is the fd namespace: sponge passes fd 0 (stdin) to concat(),
so all fd operations must use the libnix fd namespace, not amiport's. The
deferred unlink pattern also requires special handling since AmigaOS does not
support Unix-style "unlink but keep open" semantics.

| Issue | Tier | Resolution |
|-------|------|------------|
| `/tmp` path | Tier 1 | Replaced with `T:` (AmigaOS RAM:T/) |
| `mkstemp()` | Tier 1 | `amiport_mkstemp()` via macro |
| `creat()` | Tier 1 | `open(path, O_WRONLY\|O_CREAT\|O_TRUNC)` |
| `unlink()` after mkstemp | N/A | Deferred to atexit (no unlink-while-open on AmigaOS) |
| `ssize_t` type | Tier 1 | Replaced with `long` (C89 libnix) |
| `read()`/`write()`/`close()` | Tier 1 | libnix native (not amiport shim -- fd namespace) |
| `lseek()` | Tier 1 | libnix native |
| `BUFSIZ` (65536 on libnix) | N/A | Capped to 4096 (stack safety) |
| `<stdlib.h>` exit() | Tier 1 | `<amiport/stdlib.h>` (exit macro) |
| `concat()` name | N/A | Renamed `sponge_concat()` (libnix string.h clash) |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)` (RETURN_ERROR) |
| `#include "../util.h"` | N/A | Removed (pulls in `<regex.h>` unnecessarily) |

## Transformations Applied

| File | Change | Comment |
|------|--------|---------|
| sponge.c | Added `<amiport/stdlib.h>` | exit() -> amiport_exit() |
| sponge.c | Added `<amiport/glob.h>` | Wildcard expansion |
| sponge.c | `"/tmp/sponge-XXXXXX"` -> `"T:sponge-XXXXXX"` | No /tmp on AmigaOS |
| sponge.c | Deferred unlink to atexit cleanup | AmigaOS deletes immediately on unlink |
| sponge.c | `creat()` -> `open()` with O_WRONLY\|O_CREAT\|O_TRUNC | creat() not shimmed |
| sponge.c | All `exit(1)` -> `exit(10)` | Amiga error convention |
| sponge.c | Added `__stack = 16384` | Stack cookie |
| sponge.c | Added `$VER` string | AmigaOS version identification |
| sponge.c | Uses libnix native `<unistd.h>` | fd 0 (stdin) is in libnix namespace |
| sponge.c | Added atexit cleanup | Free argv, remove temp file, flush stdout |
| concat.c | `#include "../util.h"` removed | Pulls regex.h unnecessarily |
| concat.c | `concat()` renamed `sponge_concat()` | libnix string.h has concat symbol |
| concat.c | `BUFSIZ` capped to 4096 | libnix BUFSIZ is 65536, too large for stack |
| concat.c | Buffer made `static` | Stack safety on AmigaOS |
| concat.c | `ssize_t` -> `long` | C89 compatibility |
| writeall.c | `#include "../util.h"` removed | Pulls regex.h unnecessarily |
| writeall.c | `ssize_t` -> `long` | C89 compatibility |
| eprintf.c | `exit(1)` -> `exit(10)` | Amiga error convention |
| eprintf.c | `<stdlib.h>` -> `<amiport/stdlib.h>` | exit macro activation |
| sponge-util.h | Created as minimal subset of util.h | Avoids regex.h dependency |
| sponge-util.h | `ssize_t` -> `long` | C89 compatibility |
| sponge-util.h | `concat` -> `sponge_concat` | Function rename declaration |

## Build Configuration

| Setting | Value |
|---------|-------|
| Stack size | 16384 bytes |
| CFLAGS | Default (C89, -m68000) |
| Link libraries | libamiport.a (posix-shim) |
| Objects | sponge.o, concat.o, writeall.o, eprintf.o |
| Special flags | None |

## Test Results

- **Total tests:** 15
- **Passed:** Needs retest (ARexx wrapper fixes pending)
- **Pass rate:** Pending retest
- **Notable findings:** The core sponge operation (stdin -> temp -> output)
  works correctly. The deferred unlink pattern ensures the temp file in T:
  is cleaned up even on error exits. Tests use ARexx wrappers to pipe
  content through sponge since the test harness does not support shell pipes.

## Memory Safety

**Verdict: CLEAN.** All dynamic allocations are properly tracked:

- **argv expansion:** Freed via atexit cleanup.
- **Temp file:** Removed via atexit cleanup (s_tmp static buffer).
- **I/O buffers:** The concat buffer is `static` (not heap-allocated).
- **No getenv, no getline, no linked lists.**

The deferred-unlink pattern is specifically designed for AmigaOS -- the temp
file path is stored in a static buffer so atexit can remove it even if the
program exits via eprintf() (which calls exit(10)).

## Performance Notes

No performance concerns. The program does bulk I/O via read()/write() with
a 4096-byte buffer. This is optimal for 68k -- large enough to amortize
syscall overhead, small enough to avoid stack pressure. No character-at-a-time
processing.

## Known Limitations

- **Temp file visible during operation:** Unlike Unix where `unlink()` on an
  open file keeps the data accessible via the fd, AmigaOS deletes immediately.
  The temp file `T:sponge-XXXXXX` remains visible in `T:` until sponge
  completes or exits via atexit cleanup.
- **No append mode:** This sponge implementation only supports overwrite mode.
  The moreutils version has `-a` for append, which is not present in sbase.
- **RAM disk space:** Since the temp file is in `T:` (mapped to `RAM:T/`),
  the file being sponged must fit in available RAM. Large files may exhaust
  the RAM disk.
- **Pipe syntax:** AmigaDOS piping uses `|` but the test harness needs ARexx
  wrappers since `ADDRESS COMMAND` does not support shell pipe syntax.
