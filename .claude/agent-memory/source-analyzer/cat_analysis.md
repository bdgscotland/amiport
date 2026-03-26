---
name: cat_analysis
description: OpenBSD cat v1.34 portability analysis: EASY verdict, st_blksize missing from amiport_stat, fileno() works via amiport/stdio.h, pledge stub, long long counter, exit code fixes
type: project
---

# cat v1.34 Portability Analysis

**Verdict:** EASY
**Category:** 1 — CLI tool

## Key Findings

1. **st_blksize MISSING from amiport_stat struct** (CRITICAL): raw_cat() calls fstat() then uses sbuf.st_blksize (line 222). The amiport_stat struct has no st_blksize field — this is a compile error. Fix: fall back to BUFSIZ constant directly when st_blksize is unavailable; guard with #ifndef __AMIGA__.

2. **fileno(stdout) for raw I/O path**: cat.c calls fileno(stdout) at line 218. libnix has native fileno() in stdio.h. The amiport/stdio.h macro overrides it with amiport_fileno() which only works for files opened via amiport_fopen(). stdout is NOT in the amiport_files table, so amiport_fileno(stdout) returns -1. This breaks the entire raw_cat() path. Fix: bypass the amiport macro for stdout/stdin/stderr — use raw libnix fileno() or hardcode STDOUT_FILENO=1.

3. **long long counter** (line 148): `unsigned long long line` — C99 type. With -std=gnu99 this is fine. If C89 mode required, use `unsigned long`.

4. **%llu format** (line 164): Paired with long long. OK with -std=gnu99 + libnix.

5. **pledge**: Stub as macro. Standard pattern.

6. **getprogname()**: Available via amiport/utsname.h macro.

7. **exit codes**: return 1 on usage error → should be return 10 (RETURN_ERROR). err(1,...) → err(10,...).

8. **fclose(stdout)**: AmigaOS stdout is dos.library Output() handle — do NOT close it. Guard: if (fclose(stdout)) is called without guarding. This will close the shell's output handle. Fix: remove or guard.

## Required Headers
- sys/stat.h → amiport/sys/stat.h
- unistd.h → amiport/unistd.h
- err.h → amiport/err.h
- fcntl.h → amiport/unistd.h
- pledge → stub macro

## Shim Functions
- open() → amiport_open() (Tier 1)
- close() → amiport_close() (Tier 1)
- read() → amiport_read() (Tier 1)
- write() → amiport_write() (Tier 1)
- fstat() → amiport_fstat() (Tier 1) BUT st_blksize not in amiport_stat
- fileno() → libnix has it natively; amiport/stdio.h macro breaks stdout

## Blockers for Compilation
1. sbuf.st_blksize — field does not exist in amiport_stat struct
2. fileno(stdout) via amiport macro returns -1 for stdout
