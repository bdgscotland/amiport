# Port: seq

## Overview

| Field | Value |
|-------|-------|
| Program | seq |
| Version | 1.8 (port revision: 1) |
| Source | OpenBSD seq v1.8 (ISC) |
| Category | 1 -- CLI |
| License | ISC |
| Original Author | Todd C. Miller (NetBSD original by Brian Ginsbach) |
| Port Date | 2026-04-11 |
| Binary Size | 54 KB |
| Source Files | 1 (seq.c) |

## Description

Print sequences of numbers. Supports custom separators, format strings, and
fixed-width zero-padded output. Useful for shell scripting loops and generating
numbered lists. Accepts integer or floating-point first/increment/last values
and Plan 9 / GNU style flags (-f format, -s separator, -w equal-width).

## Prior Art on Aminet

No standalone seq utility found on Aminet. The jot port (already in amiport)
provides similar but not identical functionality -- jot uses a different
argument order and lacks Plan 9 / GNU compatible -f / -s / -w semantics.

## Portability Analysis

Verdict: **PORTABLE** -- All dependencies are Tier 1 POSIX. The only
non-trivial elements are localeconv() (libnix returns sane defaults on
AmigaOS) and the floating-point math in generate_format() which routes
through libm soft-float.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<err.h>` | Tier 1 | Replaced with `<amiport/err.h>` (libnix has no err.h) |
| `<getopt.h>` | Tier 1 | Replaced with `<amiport/getopt.h>` (libnix getopt_long broken, crash-patterns #17) |
| `<stdlib.h>` | Tier 1 | `<amiport/stdlib.h>` activates `exit()` macro |
| `<unistd.h>` | Tier 1 | `<amiport/unistd.h>` |
| `asprintf()` | Tier 1 | `amiport_asprintf()` via `<amiport/stdio_ext.h>` |
| `getprogname()` | Tier 1 | macro via `<amiport/utsname.h>` |
| Wildcard expansion | Tier 1 | `amiport_expand_argv()` via `<amiport/glob.h>` |
| `pledge()` | Stub | `#define pledge(p, e) (0)` -- no kernel sandbox on AmigaOS |
| `localeconv()` | Tier 1 | libnix has it; AmigaOS returns "." or empty -- existing guard handles both |
| Exit codes | Tier 1 | `exit(1)`/`err(1,...)`/`errx(1,...)` -> `exit(10)`/`err(10,...)`/`errx(10,...)` |
| Soft-float math | Tier 1 | `floor()` from libm (linked via `-lm`) |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|---------|----------|-------------|---------|
| 38 | (none) | `static const char *verstag = "$VER: seq 1.8 (11.04.2026)";` | AmigaOS version string |
| 42 | (none) | `long __stack = 8192;` | Stack cookie -- snprintf/printf headroom |
| 46 | `<err.h>` | `<amiport/err.h>` | libnix has no err.h |
| 50 | `<getopt.h>` | `<amiport/getopt.h>` | libnix getopt_long is broken |
| 54 | `<locale.h>` | `<locale.h>` (kept) | localeconv() works on libnix |
| 57 | `<stdlib.h>` | `<amiport/stdlib.h>` | activates exit() macro |
| 60 | `<unistd.h>` | `<amiport/unistd.h>` | shim include |
| 62 | (added) | `<amiport/stdio_ext.h>` | asprintf -> amiport_asprintf |
| 64 | (added) | `<amiport/utsname.h>` | getprogname() macro |
| 66 | (added) | `<amiport/glob.h>` | amiport_expand_argv / amiport_free_argv |
| 78 | (added) | `#define pledge(p, e) (0)` | pledge() stub |
| 96-98 | (added) | `static char *asprintf_cur/last/prev = NULL;` | Track asprintf buffers for atexit cleanup |
| 113-125 | (added) | `static void cleanup(void)` | atexit handler frees asprintf buffers + argv |
| 150 | (added) | `amiport_expand_argv(&argc, &argv);` | Glob expansion -- AmigaDOS does not glob |
| 154 | (added) | `atexit(cleanup);` | Register cleanup for err()/errx() exit paths |
| 175-176 | `getopt_long(...)` | `amiport_getopt_long(...)` (via macro) | Use amiport getopt |
| 199-202 | `usage(1)` | `usage(10)` | RETURN_ERROR convention |
| 209-211 | `usage(1)` | `usage(10)` | RETURN_ERROR convention |
| 223 | `errx(1, "zero...")` | `errx(10, "zero...")` | Amiga exit code |
| 232 | `errx(1, ...)` | `errx(10, ...)` | "needs positive increment" |
| 236 | `errx(1, ...)` | `errx(10, ...)` | "needs negative decrement" |
| 241 | `errx(1, ...)` | `errx(10, ...)` | "invalid format string" |
| 276-280 | `asprintf(...)` | `asprintf(...)` (now macro -> `amiport_asprintf`) | Tracked in static globals |
| 280 | `err(1, "asprintf")` | `err(10, "asprintf")` | RETURN_ERROR |
| 290-295 | (added) | `free(...); ... = NULL;` after asprintf use | Prevent double-free in atexit (crash-patterns: atexit double-free pattern) |
| 421 | `err(2, "%s", num)` | `err(2, "%s", num)` (kept) | Kept as 2 -- distinguishable for ERANGE float overflow |
| 424 | `errx(2, ...)` | `errx(2, ...)` (kept) | Same -- 2 distinguishes float parse errors |

## Shim Functions Exercised

- `amiport_expand_argv()` / `amiport_free_argv()` -- argv wildcard expansion
- `amiport_getopt_long()` (via getopt_long macro)
- `amiport_optind`, `amiport_optarg` (via optind/optarg macros)
- `amiport_asprintf()` (via asprintf macro)
- `getprogname()` macro (via `<amiport/utsname.h>`)
- libnix native: `localeconv`, `strtod`, `floor`, `printf`, `fputs`, `snprintf`, `strchr`, `strstr`, `strncmp`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo, GCC 6.5.0b) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | inherited from common.mk (`-noixemul -m68000`) |
| Stack cookie | 8192 bytes |
| Link libraries | `-lamiport -lm` |
| Binary size | 54 KB (55012 bytes) |

## Test Results

- **Total tests:** 47
- **Suite:** test-fsemu-cases.txt
- **Categories covered:**
  - Functional / positional args (5)
  - Negative & descending sequences (3)
  - Floating point (3)
  - `-w` equal-width / zero-pad (5)
  - `-s` separator (2)
  - `-f` printf format string (5)
  - `-v` / `-h` (rejected as unknown -- not in OpenBSD seq 1.8) (2)
  - Combined flags (2)
  - Error paths -- no args, too many args, zero increment,
    wrong-direction increment, invalid format string, invalid
    floating point arg, unknown flag (9)
  - Edge cases -- single value, unreachable last, accumulation,
    large step (4)
  - Amiga-specific -- WORK: volume path, colon separator (2)
  - Real-world -- padded filenames, scientific notation (2)
  - Stress -- 500-element sequences (2)
  - Precision -- float endpoint rounding correction (1)

### Flag Matrix Coverage

| Flag | Tested | Notes |
|------|--------|-------|
| `-f <fmt>` | YES | %03g, prefix-%g, %e, %05.2f, %G all covered |
| `-s <str>` | YES | colon, space |
| `-w` | YES | 1-10 (2 digit), 1-100 (3 digit), boundary, double-w (space pad), descending |
| `-v` | YES | Verified rejected as unknown flag (RC=10) |
| `-h` | YES | Verified rejected as unknown flag (RC=10) |
| Combined `-w -s` | YES | |
| Combined `-f -s` | YES | |
| Positional 1 arg | YES | `seq 5` |
| Positional 2 arg | YES | `seq 1 5` |
| Positional 3 arg | YES | `seq 1 2 10` |

Note on `-v`/`-h`: the source defines case 'v' and case 'h' in the
switch statement but the getopt option string is `"+f:s:w"` -- so `-v`
and `-h` reach getopt as unknown options, not as the documented cases.
The tests therefore correctly assert RC=10 for these flags. The
`long_opts[]` table accepts `--version` and `--help` as long options
which would route to the documented cases, but the test suite exercises
the short-form path and the unknown-flag path is the one that matters
for shell scripts.

## Memory Safety

**Verdict: CLEAN.** All dynamic allocations are tracked and freed:
- Wildcard-expanded argv freed by `amiport_free_argv()` in atexit cleanup
- The three `asprintf()` rounding-correction buffers are stored in file-scope
  globals (`asprintf_cur`, `asprintf_last`, `asprintf_prev`) and freed in
  both the normal path and the atexit cleanup. Each free is followed by
  `= NULL` to prevent double-free in atexit (avoids the AN_MemCorrupt
  pattern documented in known-pitfalls).
- `generate_format()` returns either a static buffer (`buf[256]`) or the
  static `default_format[]` -- no heap allocation on this path.
- No `strdup()`, no growth buffers, no linked lists.

## Performance Notes

- Hot path is the `for (step = 1; ...)` printf loop. Each iteration calls
  `printf(fmt, cur)` with a single double argument. This routes through
  libnix `vfprintf()` and libm soft-float -- the bottleneck on 7 MHz
  68000 is the soft-float multiply in `cur = first + incr * step++`.
- For typical `seq 1 100` invocations the loop runs in well under a
  second. The 500-element stress test verifies no pathological growth.
- The asprintf rounding-correction code at the end runs exactly three
  times per invocation and is not a hot path.
- No fgetc-style hot loops -- this program produces output, it does not
  parse input.

## Platform Compatibility Notes

- No custom allocators -- crash-patterns #15 (offsetof alignment) does
  not apply.
- No struct-by-value returns -- crash-patterns #16 (bebbo-gcc -O2
  struct corruption) does not apply.
- libm linked for `floor()` -- routes through `mathieeedoubbas.library`,
  no known crash patterns hit at this scale.

## Known Limitations

- **Short-form `-v` and `-h` rejected:** The OpenBSD source compiles a
  case for both letters but the getopt short option string is
  `"+f:s:w"`, so the short forms reach getopt as unknown flags and
  return RC=10. The long-form `--version` and `--help` (defined in
  `long_opts[]`) work as expected. This is upstream OpenBSD behavior --
  not an amiport-specific issue.
- **e_atof error code is 2, not 10:** When strtod() fails on a non-numeric
  argument, the program exits with RC=2 (preserved from upstream) rather
  than the standard amiport RC=10. This is intentional -- it lets shell
  scripts distinguish "bad number" from "bad usage" if they care to. RC=2
  is below RETURN_WARN (5) so AmigaDOS scripts will treat it as success
  for `IF WARN`/`IF ERROR` checks; programs that care must check `$RC`
  explicitly.
- **Locale dependence:** The decimal point character is read from
  `localeconv()`. On AmigaOS without locale support this defaults to ".",
  which is the correct behavior for portability with shell scripts.

## Pitfalls Addressed

- crash-patterns #17 (libnix getopt_long broken) -- via `<amiport/getopt.h>`
- atexit double-free pattern -- asprintf buffers nulled after free in
  the normal path so atexit cleanup is idempotent
- AmigaDOS missing glob -- `amiport_expand_argv()` at startup
- exit code visibility -- all `exit(1)`/`err(1,...)`/`errx(1,...)` remapped
  to 10 except the e_atof float-parse paths (intentionally kept at 2)
- err()/errx() bypass cleanup -- `atexit(cleanup)` registered before
  any error-emitting call
