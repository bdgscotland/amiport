# Port: test

## Overview

| Field | Value |
|-------|-------|
| Program | test |
| Version | 1.23 (port revision: 1) |
| Source | OpenBSD (GitHub mirror) |
| Category | 1 -- CLI |
| License | Public Domain |
| Original Author | Erik Baalbergen |
| Port Date | 2026-03-26 |

## Description

test evaluates conditional expressions for use in shell scripts. It supports string comparison (=, !=, <, >), integer comparison (-eq, -ne, -gt, -ge, -lt, -le), file tests (-f, -d, -e, -r, -w, -x, -s, -L, -S, -b, -c, -p, -u, -g, -k, -O, -G), file comparison (-nt, -ot, -ef), and boolean operators (-a, -o, !). Can also be invoked as `[` (bracket notation). This is the OpenBSD implementation with full POSIX.2 section 4.62.4 special case handling.

## Prior Art on Aminet

No existing standalone 68k port of test found on Aminet. AmigaDOS provides `If EXISTS` and `If WARN` but lacks the full POSIX conditional expression language needed by ported shell scripts.

## Portability Analysis

Verdict: **COMPLEX** -- Single-file, but requires extensive stat shim integration with synthesized POSIX fields (uid, gid), access() shim, timespeccmp replacement, and stubs for Unix-specific file types (symlinks, FIFOs, sockets, block/char devices) that do not exist on AmigaOS.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<sys/stat.h>` (stat, S_ISDIR, S_IFREG, etc.) | Tier 1 | Replaced with `<amiport/sys/stat.h>` |
| `<unistd.h>` (access, isatty) | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err/errx) | Tier 1 | Replaced with `<amiport/err.h>` |
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `stat()` / `lstat()` | Tier 1 | Mapped to `amiport_stat()` via macro; lstat aliased to stat |
| `access()` (R_OK, W_OK, X_OK, F_OK) | Tier 1 | Mapped to `amiport_access()` via macro |
| `struct stat` st_uid, st_gid | Stub | `struct full_stat` wrapper with synthesized uid=0, gid=0 |
| `struct stat` st_mtim (timespec) | Stub | `timespeccmp()` macro using scalar st_mtime |
| `geteuid()` / `getegid()` | Stub | `#define geteuid() (0)` / `#define getegid() (0)` |
| `S_IFCHR`, `S_IFBLK`, `S_IFIFO`, `S_IFSOCK` | Stub | Defined to POSIX values; amiport_stat never sets them |
| `S_ISUID`, `S_ISGID`, `S_ISVTX` | Stub | Defined to POSIX values; never set on AmigaOS |
| `S_IFLNK` | Stub | Defined; lstat->stat, -L/-h always false |
| `strlcpy()` | Tier 1 | Via `<amiport/string.h>` |
| `strtonum()` | Tier 1 | Via `<amiport/err.h>` |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `__dead` attribute | Tier 1 | `#define __dead __attribute__((noreturn))` |
| `__progname` | Tier 1 | Provided by `<amiport/glob.h>` |
| `__nowild` | Tier 1 | Set to 1 -- suppress wildcard expansion for expression args |
| Exit codes | Tier 1 | `exit(2)` / `errx(2,...)` -> `exit(20)` / `errx(20,...)` (RETURN_FAIL) |
| `mode_t` | Stub | Replaced with `unsigned long` |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| test.c | 15 | (none) | `$VER: test 1.23 (26.03.2026)` | Amiga version string |
| test.c | 18 | (none) | `long __stack = 8192` | Stack cookie |
| test.c | 21 | (none) | `int __nowild = 1` | Suppress wildcard expansion |
| test.c | 25 | `#include <sys/stat.h>` | `#include <amiport/sys/stat.h>` | stat shim |
| test.c | 27 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | access/isatty |
| test.c | 33 | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | exit() macro |
| test.c | 36 | `#include <err.h>` | `#include <amiport/err.h>` | err/errx shim |
| test.c | 38 | (none) | `#include <amiport/string.h>` | strlcpy, strtonum |
| test.c | 40 | (none) | `#include <amiport/glob.h>` | __progname |
| test.c | 43 | pledge call | `#define pledge(p, e) (0)` | Stub |
| test.c | 58-60 | geteuid/getegid | `#define geteuid() (0)` / `#define getegid() (0)` | Single-user OS |
| test.c | 68-103 | (none) | S_IFCHR/S_IFBLK/S_IFIFO/S_IFSOCK/S_ISUID/S_ISGID/S_ISVTX/S_IFLNK defines | File type stubs |
| test.c | 135-165 | `struct stat` | `struct full_stat` + `full_stat_get()` wrapper | Synthesized uid/gid |
| test.c | 173 | `timespeccmp(a, b, CMP)` | Scalar st_mtime comparison macro | No struct timespec |
| test.c | 316 | `err(2, "pledge")` | `err(20, "pledge")` | RETURN_FAIL |
| test.c | 320 | `errx(2, ...)` | `errx(20, ...)` | RETURN_FAIL |
| test.c | 368-370 | `errx(2, ...)` | `errx(20, ...)` | RETURN_FAIL |
| test.c | 490 | `errx(2, ...)` | `errx(20, ...)` | RETURN_FAIL |
| test.c | 580 | `mode_t i` | `unsigned long i` | No mode_t on AmigaOS |
| test.c | 740 | `errx(2, ...)` | `errx(20, ...)` | RETURN_FAIL |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`
- `amiport_stat()` (via stat() macro)
- `amiport_access()` (via access() macro)
- `amiport_strlcpy()`
- `amiport_strtonum()`
- `amiport_exit()` (via exit() macro)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | libamiport.a (posix-shim) |
| Stack size | 8192 bytes |
| Binary size | ~37 KB |

## Test Results

Tested via FS-UAE (Category 1 CLI tool). 97 tests, 100% pass rate.

| Category | Count | Status |
|----------|-------|--------|
| String operators (-z, -n, =, !=, <, >) | 7 | PASS |
| Integer operators (-eq, -ne, -gt, -ge, -lt, -le) | 12 | PASS |
| File existence and type (-e, -f, -d) | 9 | PASS |
| File permission tests (-r, -w, -x) | 6 | PASS |
| File size test (-s) | 4 | PASS |
| Special file types (-c, -b, -p, -S, -L/-h) | 10 | PASS |
| Special permission bits (-u, -g, -k) | 6 | PASS |
| File ownership (-O, -G) | 4 | PASS |
| File comparison (-nt, -ot, -ef) | 6 | PASS |
| Terminal test (-t) | 2 | PASS |
| Boolean operators (-a, -o, !, parentheses) | 8 | PASS |
| POSIX special cases (1-5 args) | 5 | PASS |
| Error paths (syntax errors) | 4 | PASS |
| Edge cases (empty args, whitespace) | 4 | PASS |
| Real-world usage | 5 | PASS |
| Stress tests | 5 | PASS |

## Memory Safety

No agent-memory from memory-checker found. Manual analysis: no dynamic heap allocation -- all data is on the stack or in static storage. The `full_stat` structs are stack-allocated locals. The program is inherently memory-safe since it only parses argv without allocating. No wildcard expansion (disabled via `__nowild = 1`).

## Performance Notes

Pure argv parsing with no I/O (except stat/access syscalls for file tests). The operator lookup in `t_lex()` is a linear scan of the 37-entry ops table -- could be hash-based but the table is tiny. The recursive descent parser (oexpr/aexpr/nexpr/primary) has call depth bounded by expression complexity, which is always shallow in practice. No performance concerns on 68k.

## Known Limitations

1. **File type tests always false** -- `-c` (char device), `-b` (block device), `-p` (FIFO), `-S` (socket), `-L`/`-h` (symlink) always return false because AmigaOS FFS/OFS/SFS has no equivalent file types.
2. **Permission bit tests always false** -- `-u` (setuid), `-g` (setgid), `-k` (sticky) always return false. AmigaOS does not have Unix permission bits.
3. **Ownership tests always true** -- `-O` (owned by euid) and `-G` (owned by egid) always return true for existing files because AmigaOS is single-user (all stubs return 0).
4. **-nt/-ot granularity is ~1 second** -- AmigaOS timestamps from DateStamp() have ~1/50s precision but amiport_stat rounds to seconds. Files modified within the same second compare as equal.
5. **isatty() for -t** -- Uses libnix isatty() which may not behave identically to POSIX for all fd values. The common `test -t 1` case works.
6. **Makefile target conflict** -- The binary name `test` conflicts with the standard `make test` phony target. Use `make run-tests` for the test runner.
