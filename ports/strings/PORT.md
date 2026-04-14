# Port: strings

## Overview

| Field | Value |
|-------|-------|
| Program | strings |
| Version | 1.0 (port revision: 1) |
| Source | Custom implementation (public domain) |
| Category | 1 -- CLI |
| License | Public Domain |
| Original Author | amiport |
| Port Date | 2026-04-11 |
| Binary Size | 38 KB |
| Source Files | 1 (strings.c) |

## Description

Find printable strings in binary files. Scans files (or stdin) for sequences
of printable characters of a given minimum length (default 4). Supports
offset display in decimal, octal, or hex. A minimal, self-contained
implementation -- not based on GNU binutils strings.

## Prior Art on Aminet

Old implementations exist (jffabre Unix commands collection ~1995, Geek
Gadgets binutils ~1998) but both are 25+ years old with no recent
maintenance. This is a fresh, lightweight, standalone implementation
written from scratch for amiport, sized at 38 KB versus binutils' multi-
megabyte footprint.

## Portability Analysis

Verdict: **PORTABLE** -- A minimal C89 program with very low POSIX surface.
The only Amiga-specific concern is the libnix `isprint()` quirk
(documented below).

| Issue | Tier | Resolution |
|-------|------|------------|
| `<err.h>` | Tier 1 | Replaced with `<amiport/err.h>` |
| `<stdlib.h>` | Tier 1 | `<amiport/stdlib.h>` activates exit() macro |
| `<unistd.h>` | Tier 1 | `<amiport/unistd.h>` |
| `<getopt.h>` (implicit) | Tier 1 | `<amiport/getopt.h>` -- libnix getopt broken |
| Wildcard expansion | Tier 1 | `amiport_expand_argv()` via `<amiport/glob.h>` |
| Ctrl-C polling | Tier 1 | `amiport_check_break()` via `<amiport/signal.h>` |
| `isprint()` for binary scan | KNOWN PITFALL | Replaced with explicit ASCII range check |
| Exit codes | Tier 1 | `exit(1)` -> `exit(10)`, `errx(1,...)` -> `errx(10,...)` |
| `fclose(stdin)` risk | Known pitfall | Guarded `if (fp != stdin) fclose(fp);` |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|---------|----------|-------------|---------|
| 17 | `<stdlib.h>` | `<amiport/stdlib.h>` | exit() macro activation |
| 20 | `<unistd.h>` | `<amiport/unistd.h>` | shim include |
| 22 | (added) | `<amiport/getopt.h>` | libnix getopt_long broken (crash-patterns #17) |
| 24 | `<err.h>` | `<amiport/err.h>` | bare err.h missing from libnix |
| 26 | (added) | `<amiport/signal.h>` | amiport_check_break() Ctrl-C polling |
| 28 | (added) | `<amiport/glob.h>` | amiport_expand_argv |
| 31 | (added) | `static const char *verstag = "$VER: strings 1.0 (11.04.2026)";` | AmigaOS version string |
| 34 | (added) | `long __stack = 8192;` | Stack cookie |
| 43-49 | (added) | `static void cleanup(void)` | atexit handler frees argv + flushes stdout |
| 60 | (added) | `static unsigned char ibuf[8192];` | Block buffer for fread (perf-optimizer HIGH) |
| 63 | `while ((ch = fgetc(fp)) != EOF)` | `while ((nr = fread(ibuf, 1, sizeof(ibuf), fp)) > 0) { ... for (bi=0; bi<nr; bi++) }` | 3-5x speedup vs per-byte fgetc on 68000 |
| 65-68 | (added) | `if (amiport_check_break()) { fflush(stdout); return; }` | Ctrl-C check once per block |
| 71-73 | `if (isprint(ch) || ch == '\t')` | `if ((ch >= 0x20 && ch <= 0x7E) || ch == '\t')` | libnix isprint() includes 0x80-0xFF (known pitfall) |
| 115 | `exit(1)` | `exit(10)` | RETURN_ERROR -- exit(1) invisible to Amiga scripts |
| 126 | (added) | `amiport_expand_argv(&argc, &argv);` | AmigaDOS does not glob |
| 128 | (added) | `atexit(cleanup);` | Register cleanup for err/errx exit paths |
| 140 | `errx(1, ...)` | `errx(10, ...)` | RETURN_ERROR |
| 148 | `errx(1, ...)` | `errx(10, ...)` | RETURN_ERROR |
| 168 | `ret = 1` | `ret = 10` | RETURN_ERROR |
| 173-174 | `fclose(fp);` | `if (fp != stdin) fclose(fp);` | Never fclose(stdin) on AmigaOS (known pitfall) |

## Shim Functions Exercised

- `amiport_expand_argv()` / `amiport_free_argv()` -- argv wildcard expansion
- `amiport_getopt()` (via getopt macro)
- `amiport_check_break()` -- Ctrl-C polling
- libnix native: `fopen` / `fclose` / `fread` / `printf` / `putchar` / `fputs` / `fflush` / `atoi` / `strcmp`
- `<amiport/err.h>`: `warn()`, `errx()`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo, GCC 6.5.0b) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | inherited from common.mk (`-noixemul -m68000`) |
| Stack cookie | 8192 bytes |
| Link libraries | `-lamiport` |
| Binary size | 38 KB (38856 bytes) |

## Test Results

- **Total tests:** 38
- **Suite:** test-fsemu-cases.txt
- **Categories covered:**
  - Default operation -- text file, binary file, multi-line (3)
  - `-a` flag (no-op equivalence) (1)
  - `-n` minimum length -- 1, 3, 5, 6, 9, 10, 11, 12 boundaries (8)
  - `-t` offset format -- d, o, x (3)
  - Multiple file arguments (1)
  - stdin via `-` filename redirect (1)
  - Edge cases -- empty file, exact-min-length string, one-short
    string, long line buffer stress (5)
  - Amiga-specific -- WORK: volume paths, offset calculation (2)
  - Error paths -- nonexistent file, `-n 0`, `-n abc`, `-n -5`,
    `-t z`, unknown flag, mix valid+missing (7)
  - Real-world -- 22-string binary scan, long symbol filtering,
    decimal offsets, combined `-n` + `-t` (5)
  - Stress -- 324-byte binary full scan (2)

### Flag Matrix Coverage

| Flag | Tested | Notes |
|------|--------|-------|
| `-a` | YES | Verified equivalence with default |
| `-n <min>` | YES | 1, 3, 4, 5, 6, 9, 10, 11, 12 plus boundary cases |
| `-t d` | YES | Decimal offsets |
| `-t o` | YES | Octal offsets |
| `-t x` | YES | Hex offsets |
| `-n` + `-t` combined | YES | |
| Multiple files | YES | |
| `-` (stdin) | YES | Via AmigaDOS `<` redirect |

## Memory Safety

**Verdict: CLEAN.** The program has zero heap allocations of its own:
- `static char buf[8192]` and `static unsigned char ibuf[8192]` are
  file-scope static buffers -- safe on single-threaded AmigaOS, no
  cleanup needed.
- The wildcard-expanded argv from `amiport_expand_argv()` is freed in
  `cleanup()` registered via `atexit()` so it runs on normal exit AND
  on `err()`/`errx()` exit paths.
- `fopen()` results are tracked in a local `fp` pointer with explicit
  `fclose()` (guarded against stdin).
- No `getenv()` (which on libnix returns a static pointer anyway -- see
  known-pitfalls), no `strdup()`, no growth buffers.

## Performance Notes

- **Block-buffered fread (perf-optimizer HIGH):** The original
  per-byte `fgetc()` loop was replaced with `fread()` into an 8 KB
  block buffer plus an inner `for` over the bytes. On a 7 MHz 68000
  this gives a 3-5x speedup because each `fgetc()` call costs a JSR +
  stack frame + buffer-end check through libnix; the block-read amortizes
  that to one call per 8192 bytes. See known-pitfalls
  "fgetc() Is 3-5x Slower Than fgets() on 68000".
- **Ctrl-C polling once per block:** `amiport_check_break()` is called
  once per 8 KB read, not per byte. This is the right granularity --
  small enough to feel responsive, large enough to not dominate runtime.
- The per-byte ASCII range check (`(ch >= 0x20 && ch <= 0x7E) || ch == '\t'`)
  compiles to two compares and a conditional branch -- much faster than
  `isprint()`'s table lookup, and correct on libnix where `isprint()` is
  buggy (see "Known Limitations" / "Pitfalls Addressed").
- The longest run of work per byte is `putchar(ch)` which on AmigaOS
  goes through libnix stdio buffering -- still much faster than the
  original fgetc bottleneck.

## Platform Compatibility Notes

- No custom allocators -- crash-patterns #15 does not apply.
- No struct-by-value returns -- crash-patterns #16 does not apply.
- The static 8 KB buffers (`buf` and `ibuf`) are file-scope, not local,
  so they do not consume the per-function stack budget. Stack cookie at
  8192 is comfortable for this workload.

## Known Limitations

- **Architecture-aware offsets not supported:** `strings -t` on GNU
  binutils accepts `b`, `B`, `l`, `L` for byte-swapped output. This
  port supports `d`, `o`, `x` only (per its OpenBSD-style minimal
  design).
- **No `--target`/`--all-data`/object file format awareness:** This is
  a flat-file scanner, not an object/ELF-aware tool. It scans every
  byte of the input file regardless of section markers.
- **`-a` is a no-op:** The implementation always scans the whole file,
  so `-a` is accepted for compatibility but has no effect (matching
  the documented behavior in this port).

## Pitfalls Addressed

- **libnix isprint() includes 0x80-0xFF (KNOWN PITFALL):** libnix's
  `isprint()` returns true for bytes 0x80-0xFF (extended ASCII / high
  bytes). For binary scanning this would produce garbage characters
  mixed with real strings. The transformation replaces `isprint(ch)`
  with an explicit ASCII range check `(ch >= 0x20 && ch <= 0x7E) || ch
  == '\t'`. This pitfall is documented in
  `.claude/rules/known-pitfalls.md` under "libnix isprint() Treats
  0x80-0xFF as Printable" and was first discovered during this very
  port (2026-04-11).
- **Never fclose(stdin) on AmigaOS:** `fclose(stdin)` closes the shell's
  input handle and crashes the console handler. Guarded with `if (fp !=
  stdin) fclose(fp);`. (known-pitfalls "Never fclose(stdin) on AmigaOS")
- **fgetc() 3-5x slower than block reads on 68000:** Replaced with
  `fread()` block buffer pattern. (known-pitfalls "fgetc() Is 3-5x Slower
  Than fgets() on 68000")
- **libnix getopt_long broken (crash-patterns #17):** Replaced via
  `<amiport/getopt.h>` even though this program only uses short-form
  `getopt()`.
- **Long-running loops need Ctrl-C check:** The fread loop calls
  `amiport_check_break()` once per block. (known-pitfalls "Long-Running
  Loops Need Ctrl-C Check")
- **err()/errx() bypass cleanup:** `atexit(cleanup)` registered before
  any error-emitting call.
- **Exit code visibility:** All `exit(1)`/`errx(1,...)` remapped to 10.
