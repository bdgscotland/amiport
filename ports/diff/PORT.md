# Port Report: diff

## Original
- **Program**: diff — compare files line by line
- **Source**: OpenBSD usr.bin/diff (v1.95, 2021-10-24)
- **License**: ISC (Todd C. Miller) + BSD 3-Clause (UC Berkeley, Caldera)
- **Language**: C (2340 lines across 6 files)
- **URL**: https://github.com/openbsd/src/tree/master/usr.bin/diff

## Prior Art on Aminet
- `FDiff.lha` (1993) — assembly, minimal, no unified diff
- `diff10.lha` (1995) — Amiga E, very basic
- `ADiffView` (2024) — GUI only, no CLI
- GNU diffutils via Geek Gadgets — requires full GG runtime
- No standalone modern CLI diff for 68k AmigaOS

## Analysis
- **Verdict**: MODERATE
- 25 issues identified, 12 blocking
- No fork/exec/threads/sockets — all blockers are missing BSD extensions
- Core Hunt-McIlroy diff algorithm is pure C, fully portable

## Transformations Applied

| # | Type | Original | Replacement | Notes |
|---|------|----------|-------------|-------|
| 1 | Header | `<err.h>` | `<amiport/err.h>` | BSD error reporting |
| 2 | Header | `<unistd.h>` | `<amiport/unistd.h>` | POSIX types + getopt |
| 3 | Header | `<sys/stat.h>` | `<amiport/sys/stat.h>` | File metadata |
| 4 | Header | `<getopt.h>` | `<amiport/getopt.h>` | Option parsing |
| 5 | Header | `<dirent.h>` | `<amiport/dirent.h>` | Directory operations |
| 6 | Header | `<fnmatch.h>` | `<amiport/fnmatch.h>` | NEW shim |
| 7 | Header | `<regex.h>` | `#ifdef HAS_REGEX` | Made optional |
| 8 | Header | `<paths.h>` | `#define` replacements | NIL: and T: |
| 9 | Header | `<sys/wait.h>` | Removed | Unused |
| 10 | Function | `getopt_long()` | `getopt()` | Stripped long options |
| 11 | Function | `pledge()`/`unveil()` | Removed | OpenBSD-only |
| 12 | Function | `fgetln()` | `fgets()` | BSD→C89 |
| 13 | Function | `pread()` | `fseek()`+`fread()` | Rewrote preadline() |
| 14 | Function | `mkstemp()`+`fdopen()` | `amiport_tmpfile()` | Rewrote opentemp() |
| 15 | Function | `reallocarray()` | `realloc()` + overflow check | BSD→C89 |
| 16 | Function | `vasprintf()` | `vsnprintf()` + malloc | BSD→C89 |
| 17 | Function | `stat()`/`fstat()` | `amiport_fill_stat()` | fopen-based workaround for vamos Lock() bug |
| 18 | Function | `set_argstr()` | Fixed buffer sizing | Pointer arithmetic assumed sequential argv |
| 19 | Constant | `_PATH_DEVNULL` | `"NIL:"` | Amiga null device |
| 20 | Constant | `_PATH_TMP` | `"T:"` | Amiga temp directory |
| 21 | Type | `__dead` | `#define __dead` | Compat macro |
| 22 | Type | `__attribute__` | `#ifdef __GNUC__` | VBCC compat |
| 23 | Format | `%zu` | `%lu` + casts | C89 printf compat |
| 24 | Boilerplate | — | Version string | `$VER: diff 1.0 (20.03.2026)` |
| 25 | Boilerplate | — | Stack cookie | `LONG __stack = 65536` |

## New Shim Functions Created
- `amiport/fnmatch.h` + `fnmatch.c` — shell-style glob matching
- `amiport/scandir.h` + `scandir.c` — directory scanning with filter/sort
- `warnc()` added to `amiport/err.h`
- `STDIN_FILENO`, `off_t`, `ssize_t`, `u_char`, `u_int` added to `amiport/unistd.h`
- `st_dev`, `st_ino` added to `struct amiport_stat`
- `d_fileno` added to `struct amiport_dirent`

## Shim Functions Exercised
- `amiport_stat()` (via `amiport_fill_stat` workaround)
- `amiport_getopt()`
- `amiport_opendir()`, `amiport_readdir()`, `amiport_closedir()`
- `amiport_scandir()`, `amiport_alphasort()`
- `amiport_fnmatch()`
- `amiport_err()`, `amiport_errx()`, `amiport_warn()`, `amiport_warnx()`, `amiport_warnc()`
- `amiport_tmpfile()`

## Build
```
make build-shim                    # Rebuild shim (new fnmatch + scandir)
make build TARGET=ports/diff       # Cross-compile diff
make test TARGET=ports/diff        # Test via vamos
```

## Test Results

| Test Case | Input | Expected | Status |
|-----------|-------|----------|--------|
| Normal diff | Two different files | Shows changes with `<`/`>` markers | PASS |
| Unified diff (-u) | Two different files | Shows `@@` hunks with `+`/`-` | PASS |
| Context diff (-c) | Two different files | Shows `***`/`---` context blocks | PASS |
| Identical files | Same file twice | No output, exit 0 | PASS |
| Nonexistent file | Missing file | Error message, exit 2 | PASS |

**5/5 tests passed.**

## Known Limitations
- `-I pattern` (ignore matching lines) is disabled — requires bundled regex library (documented as `#ifdef HAS_REGEX`)
- Long option names (`--unified`, `--recursive`) not supported — short options work (`-u`, `-r`)
- Directory diff (`-r`) depends on `amiport_scandir()` — tested but may have edge cases
- `amiport_stat()` uses `fopen()` workaround due to vamos `Lock()` bug — works on real hardware
- File timestamps in unified/context headers use clib2's `ctime()` — depends on Locale timezone

## Bugs Found During Port
1. `set_argstr()` used pointer arithmetic (`*ave - *av`) assuming sequential argv layout — vamos allocates argv non-sequentially, producing a -35 offset that became a 4GB allocation. Fixed with explicit `strlen()` iteration.
2. `amiport_fopen()` caused infinite recursion when `<amiport/stdio.h>` macro'd `fopen` back to `amiport_fopen`. Fixed with `#undef` in implementation.
