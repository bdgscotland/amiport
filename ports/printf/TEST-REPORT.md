# FS-UAE Test Report: printf

## Summary

| Field | Value |
|-------|-------|
| Port | printf |
| Date | 2026-03-26 13:37:25 |
| Duration | 45s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:printf` (51K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 51/51 passed |

## Test Results

```
1..51
ok 1 - Basic string literal with newline escape
ok 2 - Tab escape sequence in format string
ok 3 - Decimal integer format %d
ok 4 - Signed integer format %i with negative value
ok 5 - Octal format %o
ok 6 - Unsigned decimal format %u
ok 7 - Lowercase hex format %x
ok 8 - Uppercase hex format %X
ok 9 - Fixed-point float format %f
ok 10 - Scientific notation format %e
ok 11 - General float format %g uses shorter representation
ok 12 - General float %g switches to scientific for very small values
ok 13 - Scientific notation %e with large value
ok 14 - String format %s
ok 15 - Character format %c
ok 16 - Right-aligned decimal with field width 10
ok 17 - Zero-padded hex with field width 8
ok 18 - Zero-padded decimal with field width 5
ok 19 - String precision limits output to 5 chars
ok 20 - String precision truncates to 3 chars
ok 21 - Float precision 6 digits (pi approximation)
ok 22 - Float with width 20 and precision 8 (pi, right-aligned)
ok 23 - Alternate form flag # prefixes octal with 0
ok 24 - Alternate form flag # prefixes hex with 0x
ok 25 - Plus flag forces sign for positive integers
ok 26 - Multiple arguments with mixed format specifiers
ok 27 - Two string arguments
ok 28 - Format string reused for each extra argument
ok 29 - Format reuse produces two lines from four arguments
ok 30 - %b format processes backslash-n as newline
ok 31 - %b format processes backslash-t as tab
ok 32 - %b \c escape halts output immediately
ok 33 - %b octal escape sequences produce ASCII characters
ok 34 - Hex string argument auto-converts to integer
ok 35 - Octal string argument auto-converts to integer
ok 36 - Quoted character argument gives ASCII value of first char
ok 37 - Double percent produces a literal percent sign
ok 38 - Double-dash separator is accepted and ignored
ok 39 - No arguments produces usage error exit code
ok 40 - Invalid format directive exits with error
ok 41 - Empty string argument for %s produces blank line
ok 42 - Zero integer value
ok 43 - Fewer arguments than conversions uses default zero value
ok 44 - Amiga WORK volume path literal passed as string argument
ok 45 - Real-world multi-field formatted record
ok 46 - Real-world key-value pair generation via format reuse
ok 47 - Format reuse stress -- five integer arguments produce five lines
ok 48 - Stress -- multiple padded hex values
ok 49 - Precision float pi reference value (arithmetic regression check)
ok 50 - Large integer as float in scientific notation
ok 51 - Uppercase %G switches to scientific notation for large value
# passed: 51 failed: 0 total: 51
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Basic string literal with newline escape | PASS | |
| 2 | Tab escape sequence in format string | PASS | |
| 3 | Decimal integer format %d | PASS | |
| 4 | Signed integer format %i with negative value | PASS | |
| 5 | Octal format %o | PASS | |
| 6 | Unsigned decimal format %u | PASS | |
| 7 | Lowercase hex format %x | PASS | |
| 8 | Uppercase hex format %X | PASS | |
| 9 | Fixed-point float format %f | PASS | |
| 10 | Scientific notation format %e | PASS | |
| 11 | General float format %g uses shorter representation | PASS | |
| 12 | General float %g switches to scientific for very small values | PASS | |
| 13 | Scientific notation %e with large value | PASS | |
| 14 | String format %s | PASS | |
| 15 | Character format %c | PASS | |
| 16 | Right-aligned decimal with field width 10 | PASS | |
| 17 | Zero-padded hex with field width 8 | PASS | |
| 18 | Zero-padded decimal with field width 5 | PASS | |
| 19 | String precision limits output to 5 chars | PASS | |
| 20 | String precision truncates to 3 chars | PASS | |
| 21 | Float precision 6 digits (pi approximation) | PASS | |
| 22 | Float with width 20 and precision 8 (pi, right-aligned) | PASS | |
| 23 | Alternate form flag # prefixes octal with 0 | PASS | |
| 24 | Alternate form flag # prefixes hex with 0x | PASS | |
| 25 | Plus flag forces sign for positive integers | PASS | |
| 26 | Multiple arguments with mixed format specifiers | PASS | |
| 27 | Two string arguments | PASS | |
| 28 | Format string reused for each extra argument | PASS | |
| 29 | Format reuse produces two lines from four arguments | PASS | |
| 30 | %b format processes backslash-n as newline | PASS | |
| 31 | %b format processes backslash-t as tab | PASS | |
| 32 | %b \c escape halts output immediately | PASS | |
| 33 | %b octal escape sequences produce ASCII characters | PASS | |
| 34 | Hex string argument auto-converts to integer | PASS | |
| 35 | Octal string argument auto-converts to integer | PASS | |
| 36 | Quoted character argument gives ASCII value of first char | PASS | |
| 37 | Double percent produces a literal percent sign | PASS | |
| 38 | Double-dash separator is accepted and ignored | PASS | |
| 39 | No arguments produces usage error exit code | PASS | |
| 40 | Invalid format directive exits with error | PASS | |
| 41 | Empty string argument for %s produces blank line | PASS | |
| 42 | Zero integer value | PASS | |
| 43 | Fewer arguments than conversions uses default zero value | PASS | |
| 44 | Amiga WORK volume path literal passed as string argument | PASS | |
| 45 | Real-world multi-field formatted record | PASS | |
| 46 | Real-world key-value pair generation via format reuse | PASS | |
| 47 | Format reuse stress -- five integer arguments produce five lines | PASS | |
| 48 | Stress -- multiple padded hex values | PASS | |
| 49 | Precision float pi reference value (arithmetic regression check) | PASS | |
| 50 | Large integer as float in scientific notation | PASS | |
| 51 | Uppercase %G switches to scientific notation for large value | PASS | |

## Environment

| Component | Version/Path |
|-----------|-------------|
| FS-UAE | 3.2.35 |
| Kickstart | 3.1 (40.68) |
| Amiga model | A1200 (68020) |
| Compiler | m68k-amigaos-gcc (bebbo) |
| POSIX shim | libamiport.a |
| Regex emu | libamiport-emu.a |
| Test harness | ARexx (test-runner.rexx) |

## Test Cases

Each test runs the command inside AmigaOS, captures stdout to a file,
and compares against the expected output string.

```
# FS-UAE test suite for printf 1.28 (OpenBSD port)
# Category 1 -- CLI tool
# Minimum: 8 tests; this suite has 20+
#
# NOTE: printf takes a FORMAT string as its first argument.
# AmigaDOS double-quotes are used where the format contains spaces or
# special characters. No $-variables in CMD lines (AmigaDOS expands them).
# All error messages go to stderr -- error path tests use EXPECT_RC: only.

# --- Functional: basic string output ---

# Native: /usr/bin/printf "hello\n"
TEST: Basic string literal with newline escape
CMD: WORK:printf "hello\n"
EXPECT: hello
EXPECT_RC: 0

# Native: /usr/bin/printf "hello\tworld\n"
TEST: Tab escape sequence in format string
CMD: WORK:printf "hello\tworld\n"
EXPECT: hello	world
EXPECT_RC: 0

# --- Functional: integer format specifiers ---

# Native: /usr/bin/printf "%d\n" 42
TEST: Decimal integer format %d
CMD: WORK:printf "%d\n" 42
EXPECT: 42
EXPECT_RC: 0

# Native: /usr/bin/printf "%i\n" -7
TEST: Signed integer format %i with negative value
CMD: WORK:printf "%i\n" -7
EXPECT: -7
EXPECT_RC: 0

# Native: /usr/bin/printf "%o\n" 8
TEST: Octal format %o
CMD: WORK:printf "%o\n" 8
EXPECT: 10
EXPECT_RC: 0

# Native: /usr/bin/printf "%u\n" 255
TEST: Unsigned decimal format %u
CMD: WORK:printf "%u\n" 255
EXPECT: 255
EXPECT_RC: 0

# Native: /usr/bin/printf "%x\n" 255
TEST: Lowercase hex format %x
CMD: WORK:printf "%x\n" 255
EXPECT: ff
EXPECT_RC: 0

# Native: /usr/bin/printf "%X\n" 255
TEST: Uppercase hex format %X
CMD: WORK:printf "%X\n" 255
EXPECT: FF
EXPECT_RC: 0

# --- Functional: float format specifiers ---

# Native: /usr/bin/printf "%f\n" 3.14
TEST: Fixed-point float format %f
CMD: WORK:printf "%f\n" 3.14
EXPECT: 3.140000
EXPECT_RC: 0

# Native: /usr/bin/printf "%e\n" 3.14
TEST: Scientific notation format %e
CMD: WORK:printf "%e\n" 3.14
EXPECT: 3.140000e+00
EXPECT_RC: 0

# Native: /usr/bin/printf "%g\n" 3.14
TEST: General float format %g uses shorter representation
CMD: WORK:printf "%g\n" 3.14
EXPECT: 3.14
EXPECT_RC: 0

# Native: /usr/bin/printf "%g\n" 0.00001
TEST: General float %g switches to scientific for very small values
CMD: WORK:printf "%g\n" 0.00001
EXPECT: 1e-05
EXPECT_RC: 0

# Native: /usr/bin/printf "%e\n" 123456789.0
TEST: Scientific notation %e with large value
CMD: WORK:printf "%e\n" 123456789.0
EXPECT: 1.234568e+08
EXPECT_RC: 0

# --- Functional: string and character format specifiers ---

# Native: /usr/bin/printf "%s\n" hello
TEST: String format %s
CMD: WORK:printf "%s\n" hello
EXPECT: hello
EXPECT_RC: 0

# Native: /usr/bin/printf "%c\n" A
TEST: Character format %c
CMD: WORK:printf "%c\n" A
EXPECT: A
EXPECT_RC: 0

# --- Functional: width and precision ---

# Native: /usr/bin/printf "%10d\n" 42
TEST: Right-aligned decimal with field width 10
CMD: WORK:printf "%10d\n" 42
EXPECT:         42
EXPECT_RC: 0

# Native: /usr/bin/printf "%08x\n" 255
TEST: Zero-padded hex with field width 8
CMD: WORK:printf "%08x\n" 255
EXPECT: 000000ff
EXPECT_RC: 0

# Native: /usr/bin/printf "%05d\n" 42
TEST: Zero-padded decimal with field width 5
CMD: WORK:printf "%05d\n" 42
EXPECT: 00042
EXPECT_RC: 0

# Native: /usr/bin/printf "%.5s\n" hello
TEST: String precision limits output to 5 chars
CMD: WORK:printf "%.5s\n" hello
EXPECT: hello
EXPECT_RC: 0

# Native: /usr/bin/printf "%.3s\n" hello
TEST: String precision truncates to 3 chars
CMD: WORK:printf "%.3s\n" hello
EXPECT: hel
EXPECT_RC: 0

# Native: /usr/bin/printf "%.6f\n" 3.14159265358979
TEST: Float precision 6 digits (pi approximation)
CMD: WORK:printf "%.6f\n" 3.14159265358979
EXPECT: 3.141593
EXPECT_RC: 0

# Native: /usr/bin/printf "%20.8f\n" 3.14159265358979
TEST: Float with width 20 and precision 8 (pi, right-aligned)
CMD: WORK:printf "%20.8f\n" 3.14159265358979
EXPECT:           3.14159265
EXPECT_RC: 0

# Native: /usr/bin/printf "%.*f\n" 3 3.14159
# amiport: dynamic precision/width tests removed -- AmigaDOS expands * as wildcard
# before printf receives the format string. These features work in the code but
# cannot be tested via the FS-UAE harness CMD line.

# --- Functional: flag characters ---

# Native: /usr/bin/printf "%#o\n" 8
TEST: Alternate form flag # prefixes octal with 0
CMD: WORK:printf "%#o\n" 8
EXPECT: 010
EXPECT_RC: 0

# Native: /usr/bin/printf "%#x\n" 255
TEST: Alternate form flag # prefixes hex with 0x
CMD: WORK:printf "%#x\n" 255
EXPECT: 0xff
EXPECT_RC: 0

# Native: /usr/bin/printf "%+d\n" 42
TEST: Plus flag forces sign for positive integers
CMD: WORK:printf "%+d\n" 42
EXPECT: +42
EXPECT_RC: 0

# --- Functional: multiple arguments ---

# Native: /usr/bin/printf "%d %s %f\n" 42 hello 2.718
TEST: Multiple arguments with mixed format specifiers
CMD: WORK:printf "%d %s %f\n" 42 hello 2.718
EXPECT: 42 hello 2.718000
EXPECT_RC: 0

# Native: /usr/bin/printf "%s %s\n" hello world
TEST: Two string arguments
CMD: WORK:printf "%s %s\n" hello world
EXPECT: hello world
EXPECT_RC: 0

# --- Functional: format reuse with extra arguments ---

# Native: /usr/bin/printf "%d\n" 1 2 3
TEST: Format string reused for each extra argument
CMD: WORK:printf "%d\n" 1 2 3
EXPECT: 1
EXPECT_LINE: 2,2
EXPECT_LINE: 3,3
EXPECT_RC: 0

# Native: /usr/bin/printf "%s%s\n" foo bar baz qux
TEST: Format reuse produces two lines from four arguments
CMD: WORK:printf "%s%s\n" foo bar baz qux
EXPECT: foobar
EXPECT_LINE: 2,bazqux
EXPECT_RC: 0

# --- Functional: %b format (echo-style escape processing) ---

# Native: /usr/bin/printf "%b\n" "hello\nworld"
TEST: %b format processes backslash-n as newline
CMD: WORK:printf "%b\n" "hello\nworld"
EXPECT: hello
EXPECT_LINE: 2,world
EXPECT_RC: 0

# Native: /usr/bin/printf "%b\n" "tab\there"
TEST: %b format processes backslash-t as tab
CMD: WORK:printf "%b\n" "tab\there"
EXPECT: tab	here
EXPECT_RC: 0

# Native: /usr/bin/printf "%b\n" "line1\cstop"
TEST: %b \c escape halts output immediately
CMD: WORK:printf "%b\n" "line1\cstop"
EXPECT: line1
EXPECT_RC: 0

# Native: /usr/bin/printf "%b\n" "\0101\0102\0103"
TEST: %b octal escape sequences produce ASCII characters
CMD: WORK:printf "%b\n" "\0101\0102\0103"
EXPECT: ABC
EXPECT_RC: 0

# --- Functional: numeric conversions ---

# Native: /usr/bin/printf "%d\n" 0x1a  -> 26
TEST: Hex string argument auto-converts to integer
CMD: WORK:printf "%d\n" 0x1a
EXPECT: 26
EXPECT_RC: 0

# Native: /usr/bin/printf "%d\n" 010  -> 8
TEST: Octal string argument auto-converts to integer
CMD: WORK:printf "%d\n" 010
EXPECT: 8
EXPECT_RC: 0

# Native: /usr/bin/printf "%d\n" "'A"  -> 65
TEST: Quoted character argument gives ASCII value of first char
CMD: WORK:printf "%d\n" "'A"
EXPECT: 65
EXPECT_RC: 0

# --- Functional: escape sequences in format string ---

# Native: /usr/bin/printf "%%\n"
TEST: Double percent produces a literal percent sign
CMD: WORK:printf "%%\n"
EXPECT: %
EXPECT_RC: 0

# --- Functional: -- separator ---

# Native: /usr/bin/printf -- "%d\n" 42
TEST: Double-dash separator is accepted and ignored
CMD: WORK:printf -- "%d\n" 42
EXPECT: 42
EXPECT_RC: 0

# --- Error path tests ---

# Usage: printf format [argument ...]
# No arguments: exits RC=10 (usage printed to stderr, not captured)
# source: argc < 2 triggers usage() -> exit(10), no stdin read
TEST: No arguments produces usage error exit code
CMD: WORK:printf
EXPECT_RC: 1

# Invalid format directive
# Native: /usr/bin/printf "%z\n" 2>&1 -> "invalid directive", RC=1 (OpenBSD RC=10)
TEST: Invalid format directive exits with error
CMD: WORK:printf "%z\n"
EXPECT_RC: 10

# --- Edge cases ---

# Native: /usr/bin/printf "%s\n" ""
TEST: Empty string argument for %s produces blank line
CMD: WORK:printf "%s\n" ""
EXPECT:
EXPECT_RC: 0

# Native: /usr/bin/printf "%d\n" 0
TEST: Zero integer value
CMD: WORK:printf "%d\n" 0
EXPECT: 0
EXPECT_RC: 0

# Native: /usr/bin/printf "%d\n" (no arg, default 0)
# When there are fewer args than format conversions, default 0/"" is used
TEST: Fewer arguments than conversions uses default zero value
CMD: WORK:printf "%d %d\n" 5
EXPECT: 5 0
EXPECT_RC: 0

# --- Amiga-specific: volume paths work as format input ---
# This just exercises the program with a format argument using Amiga path conventions
# (no direct path formatting in printf, but verify WORK: paths in args work normally)

# Native: /usr/bin/printf "%s\n" WORK:test-oneline.txt  -> prints path literally
TEST: Amiga WORK volume path literal passed as string argument
CMD: WORK:printf "%s\n" WORK:test-oneline.txt
EXPECT: WORK:test-oneline.txt
EXPECT_RC: 0

# --- Real-world usage: multi-field formatted output ---

# Native: /usr/bin/printf "Name: %s  Age: %d  Score: %.2f\n" Alice 30 98.765
TEST: Real-world multi-field formatted record
CMD: WORK:printf "Name: %s  Age: %d  Score: %.2f\n" Alice 30 98.765
EXPECT: Name: Alice  Age: 30  Score: 98.76
EXPECT_RC: 0

# Native: /usr/bin/printf "%s=%d\n" ONE 1 TWO 2 THREE 3
TEST: Real-world key-value pair generation via format reuse
CMD: WORK:printf "%s=%d\n" ONE 1 TWO 2 THREE 3
EXPECT: ONE=1
EXPECT_LINE: 2,TWO=2
EXPECT_LINE: 3,THREE=3
EXPECT_RC: 0

# --- Stress tests ---

# Native: /usr/bin/printf "%d\n" 1 2 3 4 5  -> 5 lines
TEST: Format reuse stress -- five integer arguments produce five lines
CMD: WORK:printf "%d\n" 1 2 3 4 5
EXPECT: 1
EXPECT_LINE: 5,5
EXPECT_RC: 0

# Native: /usr/bin/printf "%08x\n" 0 255 65535 2147483647
TEST: Stress -- multiple padded hex values
CMD: WORK:printf "%08x\n" 0 255 65535 2147483647
EXPECT: 00000000
EXPECT_LINE: 2,000000ff
EXPECT_LINE: 3,0000ffff
EXPECT_LINE: 4,7fffffff
EXPECT_RC: 0

# Native: /usr/bin/printf "%20.8f\n" 3.14159265358979
# Verifies float precision isn't clipped by libnix -- reference pi value
TEST: Precision float pi reference value (arithmetic regression check)
CMD: WORK:printf "%20.8f\n" 3.14159265358979
EXPECT:           3.14159265
EXPECT_RC: 0

# Native: /usr/bin/printf "%e\n" 123456789.0
TEST: Large integer as float in scientific notation
CMD: WORK:printf "%e\n" 123456789.0
EXPECT: 1.234568e+08
EXPECT_RC: 0

# Native: /usr/bin/printf "%G\n" 1234567.0
TEST: Uppercase %G switches to scientific notation for large value
CMD: WORK:printf "%G\n" 1234567.0
EXPECT: 1.23457E+06
EXPECT_RC: 0
```

## Emulator Log

```
(log not captured in this run)
```

## Sentinel File

Written by the ARexx harness when all tests complete:

```
TESTS_COMPLETE
passed=51
failed=0
total=51
```

---
Generated by `make test-fsemu TARGET=ports/printf`
Report template: `toolchain/templates/test-report.md.template`
