# Port: cmp

## Overview

| Field | Value |
|-------|-------|
| Program | cmp |
| Version | 1.19 (port revision: 1) |
| Source | OpenBSD cmp v1.19 (BSD 3-Clause) |
| Category | 1 -- CLI |
| License | BSD 3-Clause |
| Original Author | UC Berkeley |
| Port Date | 2026-04-11 |

## Description

Compare two files byte by byte. Reports first difference location (byte and line number) or confirms files are identical. Supports silent mode (-s), verbose mode (-l), and byte skip offsets.

## Prior Art on Aminet

Only GUI-based Cmp-AW (1995) and simple Compare 1.0 (1997) exist -- neither is POSIX-compliant. No standalone command-line cmp with -s/-l options available. This port provides standard Unix cmp behavior.

## Portability Analysis

Verdict: **MODERATE** -- four files (`cmp.c`, `regular.c`, `special.c`,
`misc.c`), heavy use of `mmap()` for the regular-file fast path, plus an
`fdopen()` pattern in `c_special()` that crosses the amiport / libnix fd
namespaces. The `fdopen()` problem (crash-patterns #12) was resolved by
changing the function signature to take paths instead of fds and using
`fopen()` internally.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<sys/stat.h>` / `fstat()` / `S_ISREG` | 1 | `<amiport/sys/stat.h>` |
| `<err.h>` / `err()` / `warn()` | 1 | `<amiport/err.h>` (bare `<err.h>` missing in libnix) |
| `<fcntl.h>` / `O_RDONLY` / `open()` | 1 | `<amiport/unistd.h>` (provides O_RDONLY + open shim) |
| `<stdlib.h>` / `exit()` | 1 | `<amiport/stdlib.h>` (activates exit -> amiport_exit macro) |
| `getopt()` / `optind` | 1 | `<amiport/getopt.h>` (libnix getopt_long broken, crash-patterns #17) |
| AmigaDOS argv wildcards | 1 | `<amiport/glob.h>` -- `amiport_expand_argv()` |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `<sys/mman.h>` / `mmap()` / `munmap()` | 2 | `<amiport-emu/mmap.h>` -- read-only `MAP_PRIVATE` only, AllocMem-backed |
| `madvise()` | Stub | `#define madvise(a, l, f) ((void)0)` -- no equivalent |
| `<stdint.h>` / `SIZE_MAX` | 1 | local `#define SIZE_MAX ((unsigned long)-1)` -- not in libnix C89 headers |
| `strtoll()` / `LLONG_MAX` | 1 | replaced with `strtol()` / `LONG_MAX` (off_t is 32-bit on AmigaOS) |
| `vwarn()` / `vwarnx()` (BSD va_list variants) | 1 | implemented locally in `misc.c` (not in `<amiport/err.h>`) |
| `fdopen()` on POSIX fd in `c_special()` | 3 | redesigned: signature takes paths, uses `fopen()` (crash-patterns #12) |
| Exit codes (1/2 -> 10) | 1 | `DIFF_EXIT` and `ERR_EXIT` both `10` (RETURN_ERROR) |

## Transformations Applied

| File | Change | Comment |
|------|--------|---------|
| extern.h | `DIFF_EXIT 1` -> `DIFF_EXIT 10` | exit(1) invisible to AmigaDOS scripts |
| extern.h | `ERR_EXIT 2` -> `ERR_EXIT 10` | RETURN_ERROR convention |
| extern.h | `c_regular` signature gained `path1`/`path2` | so mmap_failed fallback can call c_special with paths |
| extern.h | `c_special` signature changed to take paths (not fds) | avoids fdopen() on amiport fd (crash-patterns #12) |
| cmp.c | added `verstag` `$VER: cmp 1.19 (11.04.2026)` | AmigaOS Version cmd identification |
| cmp.c | added `long __stack = 16384;` | default 4KB stack too small |
| cmp.c | `<sys/stat.h>` -> `<amiport/sys/stat.h>` | fstat -> amiport_fstat |
| cmp.c | `<err.h>` -> `<amiport/err.h>` | bare <err.h> missing in libnix |
| cmp.c | `<fcntl.h>` -> `<amiport/unistd.h>` | provides O_RDONLY + open shim |
| cmp.c | `<stdlib.h>` -> `<amiport/stdlib.h>` | exit macro activation |
| cmp.c | added `<amiport/glob.h>` | argv wildcard expansion |
| cmp.c | added `<amiport/getopt.h>` | libnix getopt_long broken |
| cmp.c | `pledge()` stubbed as macro | OpenBSD sandbox absent |
| cmp.c | tracks `path1`/`path2` separately | NULL signals stdin to c_special |
| cmp.c | `LLONG_MAX` -> `LONG_MAX`, `strtoll` -> `strtol` | off_t is long (32-bit) on AmigaOS |
| cmp.c | added `cleanup()` registered via `atexit()` | frees argv on err()/exit() paths |
| cmp.c | closes amiport fds before falling back to c_special | c_special reopens via fopen (different namespace) |
| cmp.c | removed `__attribute__((noreturn))` from usage decl | C89 mode |
| regular.c | `<sys/mman.h>` -> `<amiport-emu/mmap.h>` | mmap emulated via AllocMem+Read |
| regular.c | `mmap`/`munmap`/`MAP_FAILED`/`PROT_READ`/`MAP_PRIVATE` -> AMIPORT_EMU_* | constant + macro remapping |
| regular.c | `madvise()` stubbed (`#define madvise(a, l, f) ((void)0)`) | no equivalent on AmigaOS |
| regular.c | `MADV_SEQUENTIAL` defined to silence reference inside stub | inert with stubbed madvise |
| regular.c | local `#define SIZE_MAX ((unsigned long)-1)` | not in libnix C89 headers |
| regular.c | `c_regular()` signature gained `path1`/`path2` | enables fall-through to c_special on mmap failure |
| regular.c | `printf("%6lld", byte)` -> `printf("%6ld", (long)byte)` | off_t is 32-bit long on AmigaOS |
| regular.c | mmap_failed: closes amiport fds before c_special() | crash-patterns #12 (fd namespace) |
| special.c | signature `(int fd1, ..., int fd2, ...)` -> `(const char *path1, ..., const char *path2, ...)` | avoids fdopen on amiport fd |
| special.c | uses `fopen()` for named files, `stdin` for path == NULL | unified libnix stdio |
| special.c | `printf("%6lld", byte)` -> `printf("%6ld", (long)byte)` | off_t is 32-bit |
| special.c | tracks `close1`/`close2` flags | only fclose files we opened |
| misc.c | added local `vwarn()` / `vwarnx()` | not provided by `<amiport/err.h>` |
| misc.c | `<err.h>` -> `<amiport/err.h>` | |
| misc.c | `<stdlib.h>` -> `<amiport/stdlib.h>` | |
| misc.c | `printf("char %lld, line %lld")` -> `printf("char %ld, line %ld", (long)..., (long)...)` | off_t is 32-bit |
| Makefile | added `-I../../lib/posix-emu/include` to CFLAGS | mmap emulation header |
| Makefile | LDFLAGS uses `-lamiport-emu -lamiport` (emu first) | mmap emu uses shim internals |

## Shim Functions Exercised

- `amiport_open()` (via `open()` macro)
- `amiport_close()` (via `close()` macro and explicit `amiport_close()`)
- `amiport_fstat()` (via `fstat()` macro)
- `amiport_getopt()` (via `getopt()` macro) plus `optind`
- `amiport_err()` / `amiport_errx()` / `amiport_warn()` / `amiport_warnx()` (via macros)
- `amiport_expand_argv()` / `amiport_free_argv()`
- `amiport_exit()` (via `exit()` macro)
- `amiport_emu_mmap()` / `amiport_emu_munmap()` (Tier 2 emulation, via macros)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -I../../lib/posix-shim/include -I../../lib/posix-emu/include` |
| LDFLAGS | `-L../../lib/posix-emu -lamiport-emu -L../../lib/posix-shim -lamiport` |
| Stack cookie | `long __stack = 16384;` |
| Binary size | 39908 bytes (40 KB) |
| Source files | 4 (`cmp.c` 207 lines, `regular.c` 154 lines, `special.c` 120 lines, `misc.c` 110 lines) |

## Test Results

Tested via FS-UAE on A1200 (Kickstart 3.1, 68020) using the ARexx test harness.
Test source: `test-fsemu-cases.txt` (30 TEST blocks). See `TEST-REPORT.md` for the full TAP output.

| Category | Test count | Notes |
|----------|------------|-------|
| Functional (per-flag) | 6 | identical, differ, `-s` identical/differ, `-l` differ, `-l` identical |
| Error path | 5 | `-l` + `-s` together, missing file1, missing file2, single arg, unknown flag |
| Exit code | 2 | two empty files (RC=0), empty vs non-empty (RC=10) |
| Edge case | 6 | shorter file2, shorter file1, longer file2, binary file diff, skip past EOF, double-stdin |
| Amiga-specific | 2 | WORK: volume paths both args, diffmsg uses passed filenames |
| Real-world / stress | 9 | identity check, skip past difference, multi-diff, stdin via `-`, scripting use, `-l` octal binary, large file (1024+ char lines), mid-line skip, format precision |

**Result: 30/30 PASS** (TAP output in `TEST-REPORT.md`).

The single-stdin `-` redirect test uses `<WORK:test-cmp-same.txt` to feed the
input through AmigaDOS shell stdin redirection.

## Memory Safety

**Verdict: CLEAN.**

- `amiport_expand_argv()` storage -- freed in `cleanup()` registered via
  `atexit()` after argv expansion. All exit paths (`fatal()`/`fatalx()` ->
  `exit(ERR_EXIT)`, `eofmsg()`/`diffmsg()` -> `exit(DIFF_EXIT)`, `usage()`
  -> `exit(ERR_EXIT)`, normal `return 0`) flow through `atexit` cleanup.
- `cleanup()` also calls `fflush(stdout)` so buffered diffmsg output reaches
  the shell before exit.
- `mmap()` / `munmap()` are paired in `c_regular()`: the second mmap failure
  unmaps `p1` before falling through to `mmap_failed:`. The mmap_failed path
  closes the amiport fds before calling `c_special()` (which reopens via
  `fopen()` -- avoids the crash-patterns #12 fd namespace conflict).
- `c_special()` tracks `close1`/`close2` flags and only `fclose()`s files it
  opened (never `fclose(stdin)` -- known-pitfall avoided).
- No allocations in `misc.c`, `extern.h`. No growth buffers.

## Performance Notes

- **`-O2` is safe** for all four files: no struct-by-value returns >8 bytes,
  no large local arrays. The build inherits `-O2` from `common.mk`.
- The mmap fast path (`c_regular()`) reads the entire file into memory in
  one `Read()` call via `amiport_emu_mmap()`. For files smaller than free
  Chip+Fast RAM this is much faster than the `getc()`-per-byte loop in
  `c_special()`. On mmap failure (large file or special file), the fallback
  to `c_special()` is automatic.
- `getc()` per-byte in `c_special()` is the slowest path; this only fires
  for stdin or when mmap allocation fails. No optimisation applied because
  the regular-file path is the common case.

## Platform Compatibility Notes

- `off_t` is `long` (32-bit) on AmigaOS, not `long long`. All `%lld` format
  strings were replaced with `%ld` and explicit `(long)` casts. This means
  `cmp` cannot handle files larger than ~2 GB -- not a practical limit on
  Classic Amiga (filesystems max out at 4 GB volumes anyway).
- `SIZE_MAX` is not defined in libnix C89 `<stdint.h>`; defined locally as
  `((unsigned long)-1)`.
- `LLONG_MAX` is not defined; `get_skip()` uses `LONG_MAX` instead.
- The `mmap()` -> `c_special()` fallback uses paths (not fds) so the file
  handles cross from amiport's internal fd table into libnix `FILE*` cleanly.

## Known Limitations

- **Cannot handle files larger than ~2 GB** because `off_t` is 32-bit signed
  long on AmigaOS. Skip offsets are also limited to `LONG_MAX`.
- **mmap is read-only `MAP_PRIVATE` only** -- the entire file is read into
  memory upfront via `AllocMem()` + `Read()`. Files larger than free
  contiguous memory will fall back to `c_special()` (slower per-byte path).
- **No `madvise()`** -- stubbed as a no-op. Sequential access hints are
  ignored on AmigaOS.
- **stderr error messages are not captured by the test harness** -- error
  path tests verify exit code only (RC=10 for all errors).
- **`pledge()` is a no-op** -- AmigaOS has no sandboxing.

## Review

Reviewed during the original pipeline run. No critical issues; the
fdopen-on-amiport-fd issue (crash-patterns #12) was caught and resolved via
the path-based redesign of `c_special()` and `c_regular()` signatures.

