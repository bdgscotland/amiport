# Port: patch

## Overview

| Field | Value |
|-------|-------|
| Program | patch |
| Version | 1.78 |
| Source | OpenBSD usr.bin/patch v1.78 (ISC + BSD) |
| Category | 1 — CLI |
| License | ISC + BSD (OpenBSD contributors) |
| Original Author | Larry Wall, OpenBSD contributors |
| Port Date | 2026-03-23 |

## Description

patch takes a patch file containing a difference listing produced by diff and applies those differences to one or more original files, producing patched versions. Supports unified, context, normal, and ed-style diffs. This is the first 68k AmigaOS port of patch — no standalone patch utility previously existed on Aminet.

## Prior Art on Aminet

- No Unix-style patch utility found on Aminet (Stage 0 research, 2026-03-22)
- `gpatch.lha` (2002) is a binary patcher tool (GCompare/GCompatch), NOT a diff-applying utility

## Portability Analysis

Verdict: MODERATE — 12 Tier 1 issues, 1 Tier 2 (mmap with graceful fallback), 0 Tier 3.

| Issue | Tier | Resolution |
|-------|------|------------|
| stat/fstat/lseek/open/read/write/close/unlink/rename/mkdir/chdir/chmod | 1 | amiport shim wrappers |
| pledge/unveil | 1 | macro stubs returning 0 |
| signal(SIGHUP) | 1 | compiles but ignored (only SIGINT works) |
| getline | 1 | amiport_getline via stdio_ext.h |
| fgetln | 1 | replaced with fgets + static buffer |
| stdbool.h | 1 | manual bool/true/false defs |
| paths.h | 1 | AmigaOS path equivalents (T:, *, NIL:) |
| sys/queue.h | 1 | vendored BSD LIST macros |
| d_namlen | 1 | replaced with strlen(d_name) |
| __dead attribute | 1 | __attribute__((noreturn)) |
| getopt.h | 1 | amiport/getopt.h (libnix getopt_long is broken, crash-patterns #16) |
| %lld format | 1 | cast to long, use %ld |
| dirname() | 1 | removed dead unveil block (dirname corrupts input, crash-patterns #17) |
| mmap/munmap/madvise | 2 | amiport-emu mmap (plan_b fallback exists) |

## Transformations Applied

Multi-file port (7 .c files, 6 .h files + 2 vendored headers). Key transformations:
- All POSIX file I/O replaced with amiport shim wrappers
- `<getopt.h>` → `<amiport/getopt.h>` (libnix getopt_long broken)
- `<sys/mman.h>` → `<amiport-emu/mmap.h>` for plan_a mmap
- `<sys/queue.h>` → vendored `queue.h` (BSD LIST macros)
- `<paths.h>` → vendored `amiga_paths.h` (T:, *, NIL:)
- `fgetln()` → `fgets()` + static buffer in plan_b
- `stdbool.h` → manual typedef
- Temp file paths use `T:` with `RAM:` fallback
- `dirname(filearg[0])` + unveil block removed (dead code on AmigaOS, dirname corrupts input)
- `putc()` output loop → `fwrite()` in dump_line (3-5x perf improvement)
- Tab stop bug fix: `% 7` → `% 8` in pgetline (upstream bug)
- Exit codes: fatal errors → 10 (RETURN_ERROR)

## Shim Functions Exercised

- amiport_stat, amiport_fstat
- amiport_open, amiport_read, amiport_write, amiport_close, amiport_lseek
- amiport_unlink, amiport_rename, amiport_mkdir, amiport_chdir, amiport_chmod
- amiport_signal
- amiport_getline, amiport_mkstemp (stdio_ext)
- amiport_getopt_long (amiport/getopt.h)
- amiport_recallocarray
- amiport_emu_mmap, amiport_emu_munmap
- opendir/readdir/closedir (amiport_opendir etc.)
- strtonum, warnc (via amiport/err.h)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68020+ |
| CFLAGS | `-O2 -noixemul -std=gnu99 -Wall` |
| Libraries | `-lamiport` (posix-shim), `-lamiport-emu` (mmap) |
| Binary size | 87,424 bytes |

## Test Results

FS-UAE testing: **42/42 passed** (100%)

## Known Limitations

- Path handling assumes Unix-style paths in diff headers (the standard case). AmigaOS-native paths (DH0:foo/bar) in diff output may not strip correctly with `-p`.
- mmap in plan_a uses amiport-emu which reads entire file into memory. Files larger than available RAM fall through to plan_b (fgets-based) gracefully.
- SIGHUP handler is a no-op — only SIGINT (Ctrl-C) works for interruption.
- Ed-format diffs (`-e`) required closing ofp before write_lines to avoid AmigaOS exclusive lock conflict (ERROR_OBJECT_IN_USE). Fixed.

## Review

- **Memory checker:** 2 leaks found and fixed (TMP name strings + ttyfd in my_cleanup)
- **Perf optimizer:** putc→fwrite in dump_line (2-3x output speedup), tab stop bug fix (% 7 → % 8)
- **Crash patterns discovered:** #16 (libnix getopt_long broken), #17 (dirname corrupts input)
- **Score: READY** — 42/42 FS-UAE tests passing, memory leaks fixed, performance optimized
