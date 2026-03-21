# Port: grep

## Overview

| Field | Value |
|-------|-------|
| Program | grep |
| Version | 1.0 |
| Source | OpenBSD usr.bin/grep (v1.68, May 2025) |
| Category | 1 — CLI tool |
| License | BSD 2-Clause |
| Original Author | James Howard, Dag-Erling Smorgrav |
| Port Date | 2026-03-20 |

## Description

grep searches files (or standard input) for lines matching a pattern. Supports basic regular expressions (BRE), extended regular expressions (-E), fixed string matching (-F), recursive directory search (-R), context lines (-A/-B/-C), and many other options. This is the OpenBSD implementation, which is clean, compact, and BSD-licensed.

## Prior Art on Aminet

GNU grep 2.5 exists on Aminet (2006, m68k) but is 21 years behind upstream, likely requires Geek Gadgets runtime, and has no source integration with this project. Older GNU grep 2.0 (1998) explicitly requires ixemul.library. This port provides a modern, standalone, noixemul binary.

## Portability Analysis

Verdict: **COMPLEX** — Multi-file (7 source files), regex-heavy, has mmap/zlib dependencies that must be removed, fts(3) directory traversal needs replacement.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<regex.h>` (regcomp/regexec) | Tier 2 | Use amiport-emu/regex.h (Tier 2 regex emulation) |
| `<fts.h>` (fts_open/fts_read) | Tier 2 | Replaced with opendir/readdir-based walker in compat.h |
| `<sys/mman.h>` (mmap) | Tier 3 | Excluded — restructured file.c to use FILE_STDIO only |
| `<zlib.h>` (gzip support) | Tier 3 | Excluded — defined NOZ to disable gzip support |
| `fdopen()` | Tier 2 | Replaced with fopen() in grep_open(), stdin in grep_fdopen() |
| `fstat()`/`S_ISDIR()` | Tier 2 | Replaced with opendir() check for directory detection |
| `getopt_long()` | Tier 1 | Inline implementation in compat.h |
| `getline()` | Tier 1 | Inline implementation in compat.h (same as sed port) |
| `<sys/queue.h>` SLIST macros | Tier 1 | Inline SLIST_* macros in compat.h |
| `__progname` | Tier 1 | Set from argv[0] with basename extraction |
| `pledge()`/`unveil()` | Stub | Macro stub: `#define pledge(p, e) (0)` |
| `strtonum()` | Tier 1 | amiport/err.h (shimmed) |
| `reallocarray()` | Tier 1 | amiport/string.h (shimmed) |
| `warnc()` | Tier 1 | amiport/err.h (shimmed) |
| `err()`/`errx()`/`warn()` | Tier 1 | amiport/err.h (shimmed) |
| `REG_STARTEND` | Tier 2 | Defined as 0; substring matching handled manually in procline() |
| `REG_NOSPEC` | Tier 1 | Defined as flag; fgrep uses fastcomp/fgrepcomp (not regex) |
| `long long` / `%lld` | Tier 1 | Cast to `long`/`%ld` for libnix printf compatibility |
| Exit codes | Tier 1 | 0→0(OK), 1→5(WARN/no match), 2→10(ERROR) |
| `isatty()` | Tier 1 | amiport_isatty() from posix-shim |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| compat.h | - | (new) | All compat defs | Centralized compatibility header |
| grep.h | 36 | `#include <zlib.h>` | removed | No gzip support |
| grep.h | 105-108 | mmf_t/mmopen/mmclose/mmfgetln | removed | No mmap support |
| grep.h | 128 | gzbin_file/mmbin_file | removed | No gzip/mmap |
| grep.c | 47 | `eflags = REG_STARTEND` | `eflags = REG_STARTEND` (=0) | REG_STARTEND is no-op; handled in procline |
| grep.c | 108 | `extern char *__progname` | Set from argv[0] | OpenBSD-specific; extract basename |
| grep.c | 124 | `exit(2)` | `exit(AMIGA_RETURN_ERROR)` | Amiga exit codes |
| grep.c | 256 | `pledge("stdio rpath", NULL)` | no-op macro | Stub in compat.h |
| grep.c | all | `err(2,...)` / `errx(2,...)` | `err(10,...)` / `errx(10,...)` | Amiga RETURN_ERROR |
| grep.c | 529 | `exit(c ? ... : 1)` | exit(0/5/10) | 0=OK, 5=WARN(no match), 10=ERROR |
| util.c | 35 | `#include <fts.h>` | from compat.h | fts replacement using opendir/readdir |
| util.c | 177 | `printf("%llu", c)` | `printf("%lu", (long)c)` | libnix printf compatibility |
| util.c | 673 | `printf("%lld", line_no)` | `printf("%ld", (long)line_no)` | libnix printf compatibility |
| util.c | 189+ | `regexec(r, l->dat, ...)` | `regexec(r, l->dat+offset, ...)` | Manual REG_STARTEND emulation |
| file.c | all | open()/fdopen()/fstat() | fopen()/opendir() | fdopen/fstat not in libnix |
| file.c | - | mmap path | removed | FILE_STDIO only |
| file.c | - | gzip path | removed | NOZ defined |
| binary.c | 64-84 | gzbin_file() | removed | No gzip support |
| binary.c | 87-93 | mmbin_file() | removed | No mmap support |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`, `amiport_warn()`, `amiport_warnx()`, `amiport_warnc()`
- `amiport_strtonum()`
- `amiport_reallocarray()`
- `amiport_opendir()`, `amiport_readdir()`, `amiport_closedir()`
- `amiport_isatty()`
- `amiport_emu_regcomp()`, `amiport_emu_regexec()`, `amiport_emu_regfree()`, `amiport_emu_regerror()`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport-emu -lamiport` (Tier 2 regex + Tier 1 posix-shim) |
| Binary size | ~53KB |

## Test Results

Tested via vamos (Category 1 CLI tool).

| Test | Command | Input | Expected | Result |
|------|---------|-------|----------|--------|
| Basic match | `echo "hello world" \| grep hello` | stdin | `hello world` | PASS |
| Inverted | `grep -v banana` | stdin | apple, cherry | PASS |
| Case-insensitive | `grep -i hello` | stdin | Hello, hello, HELLO | PASS |
| Count | `grep -c aaa` | stdin | 3 | PASS |
| Line numbers | `grep -n foo` | stdin | 1:foo, 3:foo | PASS |
| Extended regex | `grep -E "ca[rt]"` | stdin | cat, car | PASS |
| Fixed string | `grep -F "a.b"` | stdin | a.b | PASS |
| No-match exit | `grep XYZZY stdin` | stdin | exit 5 (RETURN_WARN) | PASS |

Also tested interactively in FS-UAE (Amiga 1200): basic match, -i, -n -v, -c, -E with alternation — all correct.

## Known Limitations

1. **No gzip/zlib support** — `-Z` flag and zgrep/zegrep/zfgrep modes are disabled. No zlib available for AmigaOS 3.x without extra porting work.
2. **No mmap** — Files are read via stdio only. On large files, this may be slightly slower than mmap on systems that support it, but is functionally identical.
3. **Recursive search depth** — `-R` recursive search is limited to 32 levels of directory nesting (AMIPORT_FTS_MAX_DEPTH). Sufficient for any practical use.
4. **Word boundary with regex** — `-w` (word match) works perfectly for fixed strings and simple patterns via the fast grep path. For complex regex patterns that fall through to the regex engine, word boundaries use `[[:<:]]`/`[[:>:]]` which our Tier 2 regex emulation may not support. In such cases, word matching may not work correctly.
5. **Line numbers** — Displayed as 32-bit `long` (max ~2 billion). Files with more lines than this are unlikely on AmigaOS.
6. **POSIX character classes** — `[:alpha:]`, `[:digit:]` etc. in regex patterns are not supported by our Tier 2 regex emulation. Use character ranges `[a-zA-Z]`, `[0-9]` instead.

## Review

Reviewed with `/review-amiga`. Score: **READY**. No critical issues.

| Dimension | Score |
|-----------|-------|
| Stack safety | OK (32KB cookie, no deep recursion) |
| Memory handling | OK (libnix cleanup at exit) |
| Path handling | OK (Amiga volume: syntax handled) |
| Library cleanup | OK (no direct AmigaOS lib opens) |
| Conventions | OK (version string, exit codes, stack cookie) |

Performance optimized by `perf-optimizer` agent:
- `amiport_getline`: fgetc loop → fgets block read (2-4x I/O speedup)
- `grep_cmp`: split into 3 specialized paths (memcmp for -F, reduced branching for 68k)
