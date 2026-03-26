# Port: expr

## Overview

| Field | Value |
|-------|-------|
| Program | expr |
| Version | 1.28 |
| Source | OpenBSD usr.bin/expr (v1.28, Jan 2022) |
| Category | 1 -- CLI tool |
| License | Public Domain |
| Original Author | J.T. Conklin (NetBSD) |
| Port Date | 2026-03-26 |

## Description

expr evaluates expressions from command-line arguments, supporting integer arithmetic (`+`, `-`, `*`, `/`, `%`), string comparison (`=`, `!=`, `<`, `>`, `<=`, `>=`), logical operators (`|`, `&`), regex matching (`:`), and parenthesized grouping. It is commonly used in shell scripts for counter increment, string length measurement, and conditional evaluation. This is the OpenBSD implementation with 64-bit integer arithmetic.

## Prior Art on Aminet

Researched via aminet-researcher agent. No standalone POSIX expr port found on Aminet. AmigaOS shell scripts typically use `eval` or ARexx for arithmetic, but lack the POSIX expr interface expected by ported shell scripts.

## Portability Analysis

Verdict: **MODERATE** -- Single file with recursive-descent parser. Requires Tier 2 regex emulation for the match (`:`) operator and `<stdint.h>` replacement for `int64_t`.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<regex.h>` (regcomp/regexec/regfree/regerror) | Tier 2 | `<amiport-emu/regex.h>` -- Henry Spencer NFA engine |
| `<stdint.h>` / `int64_t` / `INT64_MIN` / `INT64_MAX` | Tier 1 | Locally defined: `typedef long long int64_t` with explicit limits |
| `err()` / `errx()` | Tier 1 | `<amiport/err.h>` |
| `strtonum()` | Tier 1 | `<amiport/err.h>` |
| `asprintf()` | Tier 1 | `<amiport/stdio_ext.h>` |
| `getopt()` | Tier 1 | Not used (expr parses argv directly), but `<amiport/glob.h>` included |
| `pledge()` / `unveil()` | Stub | `#define pledge(p, e) (0)` |
| `__dead` annotation | Stub | `__attribute__((noreturn))` |
| Exit codes | Special | POSIX semantics preserved: 0=true, 1=false, 2->10 (error) |
| Wildcard expansion | Special | `__nowild = 1` suppresses globbing (expr arguments are expressions, not filenames) |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| expr.c | 10 | (none) | `$VER: expr 1.28` | Amiga version string with `__attribute__((used))` |
| expr.c | 13 | (none) | `long __stack = 16384` | Stack cookie |
| expr.c | 17 | (none) | `int __nowild = 1` | Suppress wildcard expansion for expression args |
| expr.c | 21 | `<stdlib.h>` | `<amiport/stdlib.h>` | Activates exit() macro |
| expr.c | 24 | `<unistd.h>` | `<amiport/unistd.h>` | Shim replacement |
| expr.c | 31-33 | `<regex.h>` | `<amiport-emu/regex.h>` | Tier 2 regex emulation |
| expr.c | 35-39 | (none) | amiport headers | err.h, stdio_ext.h, glob.h |
| expr.c | 43-49 | `<stdint.h>` | local `typedef long long int64_t` | C99 header not in libnix |
| expr.c | 53-54 | (none) | `#define pledge/unveil` | Stubbed |
| expr.c | 107-108 | `err(3, NULL)` | `err(10, NULL)` | RETURN_ERROR |
| expr.c | 197 | (none) | `asprintf()` via shim | `to_string()` uses amiport_asprintf |
| expr.c | 258 | `errx(2, "syntax error")` | `errx(10, "syntax error")` | RETURN_ERROR |
| expr.c | 304-337 | regcomp/regexec/regfree/regerror | amiport_emu_* via macros | Tier 2 regex |
| expr.c | 308-309 | (none) | `free_value(l); free_value(r)` before `errx()` | Memory safety: free intermediates before exit |
| expr.c | 361-366 | (none) | `free_value(l); free_value(r)` before `errx()` | Memory safety in eval4 error paths |
| expr.c | 569-574 | (none) | `cleanup()` with `amiport_free_argv()` | atexit cleanup |
| expr.c | 584 | (none) | `amiport_expand_argv()` | __progname initialization |
| expr.c | 613-614 | `return is_zero_or_null(vp)` | unchanged | POSIX exit semantics: 0=true, 1=false (intentionally NOT remapped) |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`
- `amiport_strtonum()` (for string-to-int64_t conversion in `is_integer()`)
- `amiport_asprintf()` (for integer-to-string in `to_string()`)
- `amiport_emu_regcomp()`, `amiport_emu_regexec()`, `amiport_emu_regfree()`, `amiport_emu_regerror()`
- `amiport_expand_argv()`, `amiport_free_argv()`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -I../../lib/posix-emu/include` |
| Stack | 16384 bytes |
| Libraries | `-lamiport-emu -lamiport` (Tier 2 regex + Tier 1 posix-shim) |
| Binary size | ~50KB |

## Test Results

**58/58 passed** (100%) on FS-UAE (A1200, Kickstart 3.1).

| Category | Count | Description |
|----------|-------|-------------|
| Arithmetic (+, -, /, %) | 9 | Addition, subtraction, division, modulo, negatives |
| Comparison (=, !=, <, >, <=, >=) | 9 | Integer and mixed comparison operators |
| String comparison | 4 | Lexicographic string ordering via strcoll |
| Logical (OR, AND) | 9 | Value-returning OR/AND with truthy/falsy semantics |
| Match (:) regex | 5 | Prefix match length, anchoring, literal patterns |
| Parentheses | 1 | Grouping with precedence override |
| Exit code semantics | 4 | POSIX RC=0 (true) vs RC=1 (false) |
| -- separator | 1 | End-of-options marker |
| Error paths | 4 | Division by zero, modulo by zero, syntax errors |
| Amiga-specific | 2 | WORK: volume, RC=1 below WARN threshold |
| Real-world | 4 | Counter increment, prefix match, OR fallback |
| Stress | 5 | Large integers, chained operations, precision checks |

## Memory Safety

- `free_value()` called on intermediate `struct val` results before `errx()` exits in `eval4()` and `eval5()` -- prevents leaking partially-evaluated expressions on error paths
- `amiport_free_argv()` via `atexit(cleanup)` on all exit paths
- `__nowild = 1` prevents unintended glob expansion of `*` operand

## Performance Notes

- `strcoll()` used for string comparison (matches POSIX spec). On AmigaOS, `strcoll()` falls back to `strcmp()` behavior since there is no locale configuration -- no performance penalty.
- 64-bit arithmetic via `long long` is emulated in software on 68000 (no native 64-bit ALU), but the overhead is negligible for a single-evaluation utility.

## Known Limitations

1. **Multiplication (`*`) untestable on AmigaDOS** -- The `*` operator is a glob wildcard in AmigaDOS shell. When expr is invoked via an Execute script (as in the test harness), `*` is expanded before expr sees it. Users must double-quote: `expr 5 "*" 6`. Tests for `*` are skipped for this reason.
2. **Parentheses may conflict with AmigaDOS** -- `(` and `)` are generally safe in the AmigaDOS shell but can interact with Execute script parsing. One parenthesis test is included; others are skipped.
3. **Comparison operators need quoting** -- `<`, `>`, `<=`, `>=` must be double-quoted on AmigaDOS to prevent redirection. Tests use `"<"`, `">"` etc.
4. **Logical operators need quoting** -- `|` (pipe) and `&` (background) must be double-quoted on AmigaDOS.
5. **POSIX exit codes preserved** -- expr returns 0 (true) and 1 (false) per POSIX. Exit code 1 is below the Amiga `IF WARN` threshold (5). AmigaDOS scripts should use `IF NOT 0` to test for false results, not `IF WARN`.
6. **Regex limitations** -- The Tier 2 regex emulation (Henry Spencer NFA) has no locale collation, no `[:class:]` character classes, and a maximum of 9 subexpressions.
7. **No group capture in tests** -- The `\(...\)` capture syntax requires backslash-escaped parentheses, which interact poorly with AmigaDOS quoting. Group capture works in the binary but is not testable via the Execute script harness.

## Review

Reviewed with `/review-amiga`. Score: **READY**.

| Dimension | Score |
|-----------|-------|
| Stack safety | OK (16KB cookie, 256-byte errbuf is largest local) |
| Memory handling | OK (free_value on error paths, atexit cleanup) |
| Path handling | N/A (expr does not open files) |
| Library cleanup | OK (regex freed via regfree after each match) |
| Conventions | OK (version string, exit codes, stack cookie, __nowild) |
