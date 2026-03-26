# Port: cat

## Overview

| Field | Value |
|-------|-------|
| Program | cat |
| Version | 1.34 (port revision: 1) |
| Source | OpenBSD (GitHub mirror) |
| Category | 1 -- CLI |
| License | BSD-3-Clause |
| Original Author | University of California, Berkeley |
| Port Date | 2026-03-26 |

## Description

cat concatenates files and prints their contents to standard output. It supports line numbering (-n, -b), end-of-line markers (-e), tab visualization (-t), non-printing character display (-v), blank line squeezing (-s), and unbuffered output (-u). The raw path uses amiport_open/read/write for efficient binary-safe I/O, while the cooked path uses stdio for character-level processing. This is the OpenBSD implementation.

## Prior Art on Aminet

No existing standalone 68k port of cat found on Aminet. AmigaDOS provides a built-in `Type` command, but it lacks line numbering, non-printing character visualization, and the other formatting flags that make cat useful in pipelines and scripts.

## Portability Analysis

Verdict: **MODERATE** -- Single-file, but uses both stdio and low-level file I/O (open/read/write), requires fstat/st_blksize removal, and needs careful handling of stdout/stdin close semantics on AmigaOS.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err/warn) | Tier 1 | Replaced with `<amiport/err.h>` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `<sys/stat.h>` (fstat, st_blksize) | Tier 1 | Removed -- BUFSIZ used directly |
| `open()` / `read()` / `write()` / `close()` | Tier 1 | Mapped to amiport_open/read/write/close via `<amiport/unistd.h>` macros |
| `fclose(stdout)` | Pitfall | Replaced with `fflush(stdout)` -- fclose(stdout) crashes AmigaOS |
| `fclose(fp)` for stdin | Pitfall | Guarded with `if (fp != stdin) fclose(fp)` |
| `unsigned long long` (line counter) | Tier 1 | Downgraded to `unsigned long` (C89, 32-bit) |
| `%llu` format | Tier 1 | Changed to `%lu` |
| `ssize_t` | Tier 1 | Replaced with `LONG` |
| `getopt()` | Tier 1 | Via `<amiport/getopt.h>` |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `getprogname()` | Tier 1 | Via `<amiport/utsname.h>` macro |
| `__progname` | Tier 1 | Provided by `<amiport/glob.h>` |
| Wildcard expansion | Tier 1 | `amiport_expand_argv()` via `<amiport/glob.h>` |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)` (RETURN_ERROR) |
| `fileno(stdout)` | Pitfall | Not used -- STDOUT_FILENO used directly for amiport fd table |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| cat.c | 43 | `#include <sys/stat.h>` | removed | fstat/st_blksize not used |
| cat.c | 47 | `#include <err.h>` | `#include <amiport/err.h>` | amiport shim |
| cat.c | 53 | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | exit() macro |
| cat.c | 56 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | open/read/write/close |
| cat.c | 58 | (none) | `#include <amiport/getopt.h>` | getopt shim |
| cat.c | 60 | (none) | `#include <amiport/glob.h>` | Wildcard expansion |
| cat.c | 62 | (none) | `#include <amiport/utsname.h>` | getprogname() |
| cat.c | 65 | pledge call | `#define pledge(p, e) (0)` | Stub |
| cat.c | 77-82 | (none) | `cleanup()` function | atexit handler |
| cat.c | 90-92 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Amiga shell glob |
| cat.c | 96 | `err(1, ...)` | `err(10, ...)` | RETURN_ERROR |
| cat.c | 124 | `return 1` | `return 10` | RETURN_ERROR |
| cat.c | 143 | `fclose(stdout)` check | `fflush(stdout)` check | Never fclose(stdout) on AmigaOS |
| cat.c | 166 | `fclose(fp)` | `if (fp != stdin) fclose(fp)` | Never fclose(stdin) |
| cat.c | 190 | `unsigned long long line` | `unsigned long line` | C89, 32-bit |
| cat.c | 207 | `%6llu` | `%6lu` | libnix printf compatibility |
| cat.c | 255-256 | `ssize_t nr, nw, off` | `LONG nr, nw, off` | C89 on 68k |
| cat.c | 267-268 | fstat(fileno(stdout)) for st_blksize | `bsize = BUFSIZ` | No st_blksize on AmigaOS |

## Shim Functions Exercised

- `amiport_err()`, `amiport_warn()`
- `amiport_open()`, `amiport_read()`, `amiport_write()`, `amiport_close()` (via macros)
- `amiport_expand_argv()`, `amiport_free_argv()`
- `amiport_exit()` (via exit() macro)
- `getprogname()` (via amiport macro)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | libamiport.a (posix-shim) |
| Stack size | 8192 bytes |
| Binary size | ~34 KB |

## Test Results

Tested via FS-UAE (Category 1 CLI tool). 30 tests, 100% pass rate.

| Category | Count | Status |
|----------|-------|--------|
| Basic file output | 1 | PASS |
| Individual flags (-n, -b, -e, -s, -t, -v, -u) | 7 | PASS |
| Combined flags (-te, -ne, -ns, -bs, -b) | 5 | PASS |
| Error paths (unknown flag, missing file) | 2 | PASS |
| Exit codes | 1 | PASS |
| Edge cases (empty file, single blank, long line) | 5 | PASS |
| Amiga-specific (WORK: paths, concatenation) | 2 | PASS |
| Real-world usage | 3 | PASS |
| Stress tests (long lines, binary data) | 3 | PASS |
| Precision tests | 1 | PASS |

## Memory Safety

No agent-memory from memory-checker found. Manual analysis: single heap allocation for raw_cat buffer (`malloc(bsize)`) stored in static variable -- allocated once, reused for all files, never freed (acceptable -- process-lifetime allocation). `amiport_expand_argv()` freed via atexit handler. No leaks on error paths.

## Performance Notes

The raw path (no flags) uses amiport_read/write with BUFSIZ-sized chunks -- efficient for bulk I/O. The cooked path (flags active) uses character-at-a-time getc() which is inherently slower but necessary for line numbering and character classification. The `getc()` loop is a potential perf-optimizer target (fgets+memcpy pattern from known-pitfalls), but correctness requires character-level processing for -v and -e flag interaction.

## Known Limitations

1. **Line counter wraps at ~4 billion** -- `unsigned long` (32-bit) replaces `unsigned long long`. No practical impact on AmigaOS file sizes.
2. **`-` for stdin not fully tested** -- The code handles `-` as a filename alias for stdin, but the FS-UAE test harness cannot pipe stdin to the program.
3. **Raw I/O uses STDOUT_FILENO directly** -- The raw_cat path writes to amiport fd 1 (mapped to AmigaDOS Output()). This bypasses libnix stdio buffering, which is correct for raw binary passthrough but means `-u` has no effect in raw mode (it was already unbuffered).
