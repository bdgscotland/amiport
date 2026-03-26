# Port: getopt

## Overview

| Field | Value |
|-------|-------|
| Program | getopt |
| Version | 1.10 |
| Source | OpenBSD usr.bin/getopt (v1.10, October 2015) |
| Category | 1 -- CLI tool |
| License | Public Domain |
| Original Author | Henry Spencer |
| Port Date | 2026-03-26 |

## Description

getopt is a command-line option parser for shell scripts. It takes an option specification string (in the same format as the C `getopt()` function) and a set of arguments, then outputs the parsed options and non-option arguments in a canonical form suitable for shell `eval` and `set` processing. Options are emitted with leading spaces, followed by `--` to separate them from non-option arguments. This enables shell scripts to handle option parsing consistently without reimplementing it in shell logic.

## Prior Art on Aminet

No existing getopt port found on Aminet for AmigaOS 3.x.

## Portability Analysis

Verdict: **SIMPLE** -- Single-file port with standard POSIX dependencies. The program's core logic is a straightforward `getopt()` loop.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<stdlib.h>` | Tier 1 | Replaced with `<amiport/stdlib.h>` (exit macro) |
| `<unistd.h>` | Tier 1 | Replaced with `<amiport/unistd.h>` |
| `<err.h>` (err) | Tier 1 | Replaced with `<amiport/err.h>` (shimmed) |
| `getopt()` / `optind` / `optarg` | Tier 1 | Used `<amiport/getopt.h>` (libnix getopt_long is broken) |
| `pledge()` | Stub | Macro stub: `#define pledge(p, e) (0)` |
| `unveil()` | Stub | Macro stub: `#define unveil(p, f) (0)` |
| Exit codes | Tier 1 | `err(1,...)`/`status = 1` changed to `err(10,...)`/`status = 10` (RETURN_ERROR) |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| getopt.c | 10 | (none) | `static const char *verstag = "$VER: getopt 1.10 (26.03.2026)";` | Amiga version string |
| getopt.c | 13 | (none) | `long __stack = 8192;` | Stack cookie |
| getopt.c | 17 | (none) | `int __nowild = 1;` | Suppress wildcard expansion -- argv[1] is an option spec, not a filename |
| getopt.c | 20-21 | (none) | `#define pledge(p, e) (0)` / `#define unveil(p, f) (0)` | Stubs |
| getopt.c | 24 | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | Exit macro |
| getopt.c | 25 | `#include <unistd.h>` | `#include <amiport/unistd.h>` | POSIX shim |
| getopt.c | 26 | (none) | `#include <amiport/getopt.h>` | libnix getopt_long broken |
| getopt.c | 27 | (none) | `#include <amiport/err.h>` | Shimmed err |
| getopt.c | 28 | (none) | `#include <amiport/glob.h>` | argv expansion and __progname |
| getopt.c | 30-35 | (none) | `cleanup()` with `amiport_free_argv()` and `fflush(stdout)` | atexit cleanup |
| getopt.c | 46-48 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Wildcard expansion and cleanup registration |
| getopt.c | 51 | `err(1, "pledge")` | `err(10, "pledge")` | RETURN_ERROR |
| getopt.c | 57 | `status = 1;` | `status = 10;` | RETURN_ERROR for unknown option |

## Shim Functions Exercised

- `amiport_err()` (via `<amiport/err.h>`)
- `amiport_exit()` (via `<amiport/stdlib.h>` macro)
- `amiport_expand_argv()`, `amiport_free_argv()` (via `<amiport/glob.h>`)
- `getopt()`, `optind`, `optarg` (via `<amiport/getopt.h>`)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (Tier 1 posix-shim) |
| Stack size | 8192 bytes |
| Binary size | ~29 KB |

## Test Results

40 tests, all passing. Tested via FS-UAE (Category 1 CLI tool).

| Category | Count | Description |
|----------|-------|-------------|
| Functional | 12 | Basic flags, colon options with arguments, combined flags, end-of-options --, non-option args, interleaved |
| Option argument edge cases | 4 | Numeric argument, numeric option characters, repeated option, options after non-option args |
| Error paths | 3 | Unknown option, missing required argument, empty optstring |
| Suppress-error mode | 2 | Leading colon suppresses stderr, leading colon with missing arg |
| Exit code verification | 3 | RC=0 success, RC=0 no args, RC=10 bad option |
| Edge cases | 6 | Empty optstring no args, bare --, colon-only optstring, lone hyphen, combined -xyz, unused options |
| Amiga-specific | 3 | WORK: volume path in option argument, T: temp path, __nowild verification |
| Real-world / stress | 7 | Verbose+help, tar-style flags, 10 distinct options, 10 repeated options, 20 non-option args, output format precision, arg reordering |

The test suite validates the exact output format (leading space before each token), end-of-options `--` placement, and correct handling of the leading-colon suppress-error mode in the optstring.

## Memory Safety

Verdict: **CLEAN** -- No direct heap allocation in the program logic. The only allocations come from `amiport_expand_argv()`, which is cleaned up via `amiport_free_argv()` in the atexit handler. `fflush(stdout)` in the cleanup function ensures all printf output is flushed on err() exit paths. No getline or dynamic buffer patterns.

## Performance Notes

The program performs a single pass through argv with the standard `getopt()` loop, then a second pass to emit non-option arguments. Both loops are O(argc) with no allocation. Performance is dominated by printf formatting, which is negligible for typical shell script argument counts. No optimization opportunities identified.

## Known Limitations

1. **No long option support** -- This is `getopt(1)`, not `getopt_long(1)`. It only handles single-character options. For long options (`--verbose`, `--output=file`), shell scripts would need a different approach.
2. **POSIX getopt semantics** -- Options appearing after the first non-option argument are treated as non-option arguments (POSIX behavior). GNU getopt permutes arguments to allow options anywhere. This matches the OpenBSD original.
3. **Wildcard suppression** -- `__nowild = 1` prevents the Amiga shell from expanding wildcard characters in the optstring (argv[1]). An optstring like `"ab*"` would otherwise be expanded as a filename glob.
