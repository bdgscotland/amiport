# Port: bc

## Overview

| Field | Value |
|-------|-------|
| Program | bc |
| Version | 1.07.1 |
| Source | GNU bc 1.07.1 (https://ftp.gnu.org/gnu/bc/) |
| Category | 1 — CLI tool |
| License | GPL-3.0 |
| Original Author | Philip A. Nelson |
| Port Date | 2026-03-23 |

## Description

GNU bc is an arbitrary precision calculator language. It supports interactive use and script execution with variables, functions, and a C-like syntax. The `-l` flag loads a standard math library with transcendental functions (sin, cos, arctan, ln, exp, bessel).

## Prior Art on Aminet

GNU bc 1.6 exists on Aminet (dev/gg/bc-1.6, uploaded 2002 by Diego Casorran). It requires ixemul.library. Our port upgrades to 1.07.1, removes the ixemul dependency (noixemul with posix-shim), and produces a standalone binary.

## Portability Analysis

Verdict: **EASY**

| Issue | Tier | Resolution |
|-------|------|------------|
| signal(SIGINT) | 1 | amiport_signal() — Ctrl-C polling |
| isatty() | 1 | amiport_isatty() — IsInteractive() |
| getenv() | 1 | amiport_getenv() — GetVar(), tracked for cleanup |
| write() in signal handler | 1 | Replace with fputs()+fflush() |
| read()/fileno() in YY_INPUT | 1 | Replace with fread()-based YY_INPUT |
| getopt_long() | — | Use bundled GNU getopt (pure C89) |
| readline/libedit | — | Disabled (not available on AmigaOS) |
| exit(1) → exit(10) | 1 | Amiga RETURN_ERROR for all error exits |
| exit(YY_EXIT_FAILURE) | 1 | bc_exit(20) — RETURN_FAIL for scanner fatal |
| fclose(stdin) | 1 | Guard: if (yyin != stdin) fclose(yyin) |
| random() | 1 | Mapped to rand() — libnix lacks random() |

## Transformations Applied

| File | Original | Transformed | Comment |
|------|----------|-------------|---------|
| main.c | `#include <signal.h>` | `#include <amiport/signal.h>` | Ctrl-C shim |
| main.c | `isatty(0) && isatty(1)` | `amiport_isatty(0) && amiport_isatty(1)` | Interactive detection |
| main.c | `getenv(...)` | `amiport_getenv(...)` + tracked/freed | 3 env vars |
| main.c | `write(1, ...)` | `fputs(..., stdout); fflush(stdout)` | Signal handler safety |
| main.c | `bc_exit(1)` (x3) | `bc_exit(10)` | Amiga RETURN_ERROR |
| main.c | `fclose(yyin)` | `if (yyin != stdin) fclose(yyin)` | Prevent stdin crash |
| main.c | — | `atexit(bc_cleanup)` + cleanup function | Free globals on exit |
| execute.c | `#include <signal.h>` | `#include <amiport/signal.h>` | Ctrl-C shim |
| execute.c | — | `amiport_check_break()` in execute loop | Ctrl-C polling |
| execute.c | — | `#define random() rand()` | libnix compat |
| scan.c | `read(fileno(yyin))` | `fread()` | YY_INPUT rewrite |
| scan.c | `isatty(fileno(file))` | `0` (non-interactive) | No fileno on AmigaOS |
| scan.c | `bc_exit(1)` (x2) | `bc_exit(10)` | Amiga RETURN_ERROR |
| scan.c | `exit(YY_EXIT_FAILURE)` | `bc_exit(20)` | RETURN_FAIL |
| proto.h | `#include <stdlib.h>` | `#include <amiport/stdlib.h>` | exit() macro |
| proto.h | `#include <unistd.h>` | `#include <amiport/unistd.h>` | isatty/getenv |
| load.c | `bc_exit(1)` | `bc_exit(10)` | Amiga RETURN_ERROR |
| util.c | `bc_exit(1)` (x5) | `bc_exit(10)` | Amiga RETURN_ERROR |

## Shim Functions Exercised

- amiport_signal()
- amiport_isatty()
- amiport_getenv()
- amiport_check_break()

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -std=gnu99 -Wall -Iported/` |
| LDFLAGS | `-lamiport -lm -Wl,--allow-multiple-definition` |
| Binary size | 107,708 bytes |
| Source files | 12 .c + 8 .h (bc core + bundled getopt + number library) |

## Test Results

31/31 FS-UAE tests pass (2026-03-23).

Covers all 8 flags (-c, -h, -i, -l, -q, -s, -w, -v), arithmetic operations, math library functions (e, l, s, a, j), user-defined functions, control flow (for, while, if/else), ibase/obase, error paths, and Amiga-specific path handling.

## Platform Compatibility Notes

- bc.c and scan.c are yacc/lex-generated code with GCC builtins — all supported by bebbo-gcc 6.5.0b
- libmath.h generated via bootstrap: native fbc compiles libmath.b to bytecode array
- Bundled GNU getopt used instead of libnix's broken getopt_long (crash-patterns #17)
- `--allow-multiple-definition` linker flag needed for bundled getopt vs libnix symbol clash

## Known Limitations

- No readline/libedit line editing in interactive mode
- BC_ENV_ARGS only works if set in AmigaOS ENV: variable
- `random()` mapped to `rand()` — adequate for bc's non-cryptographic use

## Performance Notes (perf-optimizer)

Identified but not yet applied:
- `% BASE` and `/ BASE` in number.c hot loops generate DIVU (76-140 cycles on 68000) — replaceable with shift/subtract for 3-5x arithmetic speedup
- fpush/fpop malloc per call — poolable for reduced allocation overhead
- MUL_BASE_DIGITS=80 may be too low for 68000 Karatsuba threshold

## Review

Reviewed with `/review-amiga`. Score: **READY** after fixing:
- 3 missed bc_exit(1) calls (load.c, scan.c x2)
- exit(YY_EXIT_FAILURE) in scanner
- fclose(stdin) risk in new_yy_file
- Missing amiport_check_break() in execute loop
- Memory leaks: getenv results, strdup leaks, global storage — all fixed with atexit cleanup
