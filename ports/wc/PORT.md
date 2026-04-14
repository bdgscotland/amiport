# Port: wc

## Overview

| Field | Value |
|-------|-------|
| Program | wc |
| Version | 1.32 |
| Source | OpenBSD wc v1.32 (BSD 3-Clause) |
| Category | 1 -- CLI tool |
| License | BSD 3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-22 |

## Description

wc counts the number of lines, words, and characters (bytes) in files or standard input. Supports flags -l (lines), -w (words), -c (bytes), -h (human-readable sizes). The -m (multibyte characters) flag is accepted but equivalent to -c on AmigaOS (no multibyte locale support).

## Prior Art on Aminet

**WordCount v37.10** (by Torsten Poulin, 2021) exists on Aminet as `util/cli/WordCount`. It's a standalone 68k binary with full wc functionality (-l, -w, -c modes). BSD 2-Clause licensed, no ixemul dependency.

This port provides the actual OpenBSD wc v1.32 as part of a cohesive OpenBSD toolset (alongside grep, sed, cut, head, tail, etc.) and exercises additional shim functions (fstat, read, getopt, argv expansion).

## Portability Analysis

Verdict: **EASY** -- All Tier 1. Single-file CLI tool with standard file I/O.

| Issue | Tier | Resolution |
|-------|------|------------|
| `open()`, `read()`, `close()`, `fstat()` | 1 | posix-shim wrappers |
| `getopt()` | 1 | posix-shim |
| `err()`/`warn()` | 1 | posix-shim |
| `getline()` | 1 | posix-shim |
| `pledge()` | 1 | Stub macro |
| `setlocale()` | 1 | Stub (always "C") |
| `<util.h>` / `fmt_scaled()` | 1 | Inline implementation |
| `<wchar.h>` / `<wctype.h>` / multibyte path | 1 | Guard with `#ifdef __AMIGA__` -- unreachable (MB_CUR_MAX==1) |
| Exit codes (1 -> 10) | 1 | Amiga exit code convention |

## Transformations Applied

| File | Change | Comment |
|------|--------|---------|
| wc.c | `<sys/stat.h>` -> `<amiport/sys/stat.h>` | activates fstat -> amiport_fstat macro, struct stat -> struct amiport_stat |
| wc.c | `<stdlib.h>` -> `<amiport/stdlib.h>` | activates exit() -> amiport_exit() macro |
| wc.c | `<err.h>` -> `<amiport/err.h>` | bare <err.h> missing in libnix; provides err/warn |
| wc.c | `<unistd.h>` -> `<amiport/unistd.h>` | shim open/read/close + O_RDONLY + STDIN_FILENO |
| wc.c | removed `<fcntl.h>` | O_RDONLY now comes from amiport/unistd.h |
| wc.c | removed `<locale.h>` / `setlocale()` call | no locale support; MB_CUR_MAX always 1 |
| wc.c | removed `<util.h>` | inlined fmt_scaled() -- OpenBSD libutil unavailable |
| wc.c | removed `<wchar.h>` / `<wctype.h>` | multibyte path is unreachable; guarded `#ifndef __AMIGA__` |
| wc.c | added `<amiport/glob.h>` | amiport_expand_argv() / amiport_free_argv() |
| wc.c | added `<amiport/getopt.h>` | libnix `getopt_long` is broken (crash-patterns #17) |
| wc.c | added `<stdint.h>` | int64_t for line/word/char counters |
| wc.c | added `verstag` `$VER: wc 1.32 (22.03.2026)` | AmigaOS Version cmd identification |
| wc.c | added `long __stack = 16384;` | default 4KB stack too small for libnix stdio |
| wc.c | `pledge()` / `unveil()` -> stub macros returning 0 | OpenBSD sandbox APIs absent on AmigaOS |
| wc.c | added 256-byte `ws_table[]` whitespace lookup | replaces `isspace()` jsr in hot loop -- 3-5x speedup on 68000 |
| wc.c | added `__progname` set from `argv[0]` (handles `:` and `/`) | OpenBSD provides this; manual basename for AmigaDOS volume separator |
| wc.c | added `atexit(cleanup)` registration after `amiport_expand_argv` | frees argv on err()/errx()/exit() paths |
| wc.c | `cleanup()` calls `amiport_free_argv()` + `fflush(stdout)` | required because no process cleanup with `-noixemul` |
| wc.c | inlined `fmt_scaled()` (FMT_SCALED_STRSIZE=7) using `snprintf` | OpenBSD libutil not available |
| wc.c | guarded multibyte branch with `#ifndef __AMIGA__` | prevents wchar_t / iswspace compile errors |
| wc.c | removed `bufsz < _MAXBSIZE` realloc redundancy | avoids redundant realloc on each cnt() call |
| wc.c | guarded `close(fd)` against `STDIN_FILENO` | never close stdin (known-pitfalls: fclose(stdin)) |
| wc.c | guarded `fclose(stream)` against `stdin` | same reason |
| wc.c | `gotsp` widened from `short` to `int` | 32-bit is natural register width on 68k |
| wc.c | all `err(1, ...)` -> `err(10, ...)` and `rval = 1` -> `rval = 10` | RETURN_ERROR for AmigaDOS scripts |
| wc.c | usage path returns `10` not `1` | RETURN_ERROR convention |

## Shim Functions Exercised

- `amiport_open()` (via `open()` macro from `<amiport/unistd.h>`)
- `amiport_read()` (via `read()` macro)
- `amiport_close()` (via `close()` macro, guarded against STDIN_FILENO)
- `amiport_fstat()` (via `fstat()` macro from `<amiport/sys/stat.h>`)
- `amiport_getopt()` (via `getopt()` macro from `<amiport/getopt.h>`) plus `optind`
- `amiport_err()` / `amiport_warn()` (via macros from `<amiport/err.h>`)
- `amiport_expand_argv()` / `amiport_free_argv()`
- `amiport_exit()` (via `exit()` macro from `<amiport/stdlib.h>`)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -I../../lib/posix-shim/include` (from common.mk) |
| LDFLAGS | `-L../../lib/posix-shim -lamiport -lm` (`-lm` added for fmt_scaled() soft-float helpers) |
| Stack cookie | `long __stack = 16384;` |
| Binary size | 42524 bytes (42 KB stripped) |
| Source files | 1 (`ported/wc.c`, 428 lines) |

## Test Results

Tested via FS-UAE on A1200 (Kickstart 3.1) using the ARexx test harness.
Test source: `test-fsemu-cases.txt` (16 TEST blocks).

| Category | Test count | Notes |
|----------|------------|-------|
| Functional (per-flag) | 8 | default, `-l`, `-w`, `-c`, `-m`, `-h`, multi-file total, `-l` multiline |
| Error path | 2 | nonexistent file -> RC=10, invalid flag -> RC=10 |
| Edge case | 4 | empty file, no-trailing-newline, single-char file, long line (1107 bytes) |
| Amiga-specific | 2 | WORK: volume path, combined flags `-lw` |

All tests use exact `EXPECT:` assertions for column-aligned output;
`-h` and multi-file `total` use `EXPECT_CONTAINS:` because the output format
is dynamic. No tests have been weakened.

## Memory Safety

**Verdict: CLEAN.** Allocations:

- `realloc(buf, _MAXBSIZE)` (64 KB) inside `cnt()` -- the `static char *buf` /
  `static size_t bufsz` pattern is pre-allocated once across all files and
  cleaned up at process exit (no explicit free; lives until program exit).
  No leak across multiple input files because the realloc is gated on
  `bufsz < _MAXBSIZE`.
- `amiport_expand_argv()` storage -- freed in `cleanup()` registered via
  `atexit()`, so all exit paths (`err()`, `errx()`, `exit()`, normal `return`)
  free argv.
- `fflush(stdout)` in `cleanup()` ensures buffered counts reach the console
  before exit.

No `getenv()` usage, no growth buffers requiring tracking, no per-file
allocations.

## Performance Notes

- **`isspace()` -> `ws_table[]` lookup table**: the hot byte-scan loop in
  `cnt()` calls `isspace()` once per byte. On 68000, a JSR to libnix
  `isspace` costs ~20-40 cycles per call; the 256-byte lookup table is
  4-8 cycles. Estimated 3-5x speedup for the `-w` (doword) path on large
  files. The table is initialised once at startup via `init_ws_table()`.
- **`int gotsp` instead of `short gotsp`**: 32-bit values are the natural
  register width on 68k; using `short` would force `MOVE.W` + sign extension
  on every load.
- **`-O2` is safe here**: no struct-by-value returns, no large local arrays,
  so crash-patterns #16 does not apply. The build inherits `-O2` from
  `common.mk`.
- **`bufsz < _MAXBSIZE` early return**: avoids redundant realloc on the
  second and subsequent `cnt()` calls when wc is invoked with multiple files.

## Platform Compatibility Notes

- `MB_CUR_MAX` is `__locale_mb_cur_max()` -- a runtime function call, not a
  compile-time constant (crash-patterns #11). The multibyte branch in
  `cnt()` would otherwise be entered on AmigaOS even though wchar/locale is
  not supported. The branch is guarded with `#ifndef __AMIGA__` so the
  multibyte code is never compiled.
- The `#ifndef __AMIGA__` guard also covers the `fdopen(fd, "r")` call
  inside the multibyte branch -- this would have crossed the amiport fd
  namespace into libnix stdio (crash-patterns #12) if it had been reachable.
- `__progname` is computed from `argv[0]` with both `/` and `:` checked --
  AmigaDOS uses `:` after the volume name (`WORK:wc`).

## Known Limitations

- `-m` flag accepted but equivalent to `-c` (no multibyte locale on AmigaOS 3.x).
- `-h` flag uses a simplified `fmt_scaled()` implementation (inlined, not
  OpenBSD libutil); pulls in `-lm` for soft-float helpers (`__divdf3`, etc.).
- `fstat()` on directories returns `st_size=0` (AmigaOS FFS/SFS behaviour);
  `wc -c <directory>` reports 0 bytes.
- `pledge()` / `unveil()` are stubs that always return 0; the program runs
  without sandboxing on AmigaOS (which has no memory protection anyway).
- `setlocale()` is not called -- no locale support.
- Stdin is read via `read(STDIN_FILENO, ...)` only; the multibyte stdin
  path with `getline()` / `fdopen()` is compiled out.

## Pending Work

- The current test suite has 16 tests but does not yet include a positional
  argument matrix (zero/one/two/many file arguments crossed with each flag).
  The arg-matrix coverage that test-designer now enforces was added after
  this port shipped. Re-running test-designer would expand coverage.
- No FS-UAE TEST-REPORT.md is committed for this port; the cmp/mv ports
  carry one but wc does not.

## Review

Reviewed during the original pipeline run. No critical issues. The port
predates the `/review-amiga` score system.
