# FS-UAE Test Report: strings

## Summary

| Field | Value |
|-------|-------|
| Port | strings |
| Date | 2026-04-11 17:37:00 |
| Duration | 38s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:strings` (38K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 38/38 passed |

## Test Results

```
1..38
ok 1 - Default operation prints printable strings from text file
ok 2 - Default operation extracts strings from binary file
ok 3 - Default output of multi-line text file (9 strings)
ok 4 - -a flag produces same output as default (scans whole file)
ok 5 - -n 3 includes strings of 3+ chars
ok 6 - -n 11 matches string of exactly 11 chars (hello world)
ok 7 - -n 12 produces no output when no string is that long
ok 8 - -n 5 finds strings of 5 and 7 chars, skips 2-char strings
ok 9 - -n 6 skips 5-char Hello, finds 7-char Testing
ok 10 - -n 9 finds both HelloAmiga and WorldPort
ok 11 - -n 10 finds HelloAmiga (10 chars) but not WorldPort (9 chars)
ok 12 - -n 1 minimum valid value still produces correct output
ok 13 - -t d shows decimal offset of strings in binary file
ok 14 - -t o shows octal offset of strings in binary file
ok 15 - -t x shows hexadecimal offset of strings in binary file
ok 16 - Multiple file arguments processed in order
ok 17 - Dash argument reads from stdin (via AmigaDOS redirect)
ok 18 - Empty file produces no output with RC 0
ok 19 - String exactly at minimum length 4 is printed
ok 20 - String one char shorter than minimum produces no output
ok 21 - -n 4 on exact-length 4-char string prints it
ok 22 - -n 5 on 4-char string produces no output (one short of threshold)
ok 23 - Long line file processed without crash (stress buffer handling)
ok 24 - WORK: volume paths are handled correctly
ok 25 - Offset calculation via -t d is correct on AmigaOS fgetc
ok 26 - Nonexistent file produces RC 10 (error code)
ok 27 - -n 0 is rejected with RC 10 (must be >= 1)
ok 28 - -n with non-numeric argument produces RC 10
ok 29 - -n with negative value produces RC 10
ok 30 - -t with invalid format character produces RC 10
ok 31 - Unknown flag produces usage error with RC 10
ok 32 - Mix of valid and missing files returns RC 10 but processes valid files
ok 33 - Real-world scan of binary with 22 embedded strings
ok 34 - Real-world symbol extraction with -n 13 filters to long strings only
ok 35 - Real-world -n 20 finds only the 21-char symbol AmigaOS_string_target
ok 36 - Real-world -t d on stress binary shows function_00 at correct decimal offset
ok 37 - Stress combination of -n 13 and -t x shows hex offsets for long symbols
ok 38 - Stress scan of 324-byte binary file processes all bytes correctly
# passed: 38 failed: 0 total: 38
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Default operation prints printable strings from text file | PASS | |
| 2 | Default operation extracts strings from binary file | PASS | |
| 3 | Default output of multi-line text file (9 strings) | PASS | |
| 4 | -a flag produces same output as default (scans whole file) | PASS | |
| 5 | -n 3 includes strings of 3+ chars | PASS | |
| 6 | -n 11 matches string of exactly 11 chars (hello world) | PASS | |
| 7 | -n 12 produces no output when no string is that long | PASS | |
| 8 | -n 5 finds strings of 5 and 7 chars, skips 2-char strings | PASS | |
| 9 | -n 6 skips 5-char Hello, finds 7-char Testing | PASS | |
| 10 | -n 9 finds both HelloAmiga and WorldPort | PASS | |
| 11 | -n 10 finds HelloAmiga (10 chars) but not WorldPort (9 chars) | PASS | |
| 12 | -n 1 minimum valid value still produces correct output | PASS | |
| 13 | -t d shows decimal offset of strings in binary file | PASS | |
| 14 | -t o shows octal offset of strings in binary file | PASS | |
| 15 | -t x shows hexadecimal offset of strings in binary file | PASS | |
| 16 | Multiple file arguments processed in order | PASS | |
| 17 | Dash argument reads from stdin (via AmigaDOS redirect) | PASS | |
| 18 | Empty file produces no output with RC 0 | PASS | |
| 19 | String exactly at minimum length 4 is printed | PASS | |
| 20 | String one char shorter than minimum produces no output | PASS | |
| 21 | -n 4 on exact-length 4-char string prints it | PASS | |
| 22 | -n 5 on 4-char string produces no output (one short of threshold) | PASS | |
| 23 | Long line file processed without crash (stress buffer handling) | PASS | |
| 24 | WORK: volume paths are handled correctly | PASS | |
| 25 | Offset calculation via -t d is correct on AmigaOS fgetc | PASS | |
| 26 | Nonexistent file produces RC 10 (error code) | PASS | |
| 27 | -n 0 is rejected with RC 10 (must be >= 1) | PASS | |
| 28 | -n with non-numeric argument produces RC 10 | PASS | |
| 29 | -n with negative value produces RC 10 | PASS | |
| 30 | -t with invalid format character produces RC 10 | PASS | |
| 31 | Unknown flag produces usage error with RC 10 | PASS | |
| 32 | Mix of valid and missing files returns RC 10 but processes valid files | PASS | |
| 33 | Real-world scan of binary with 22 embedded strings | PASS | |
| 34 | Real-world symbol extraction with -n 13 filters to long strings only | PASS | |
| 35 | Real-world -n 20 finds only the 21-char symbol AmigaOS_string_target | PASS | |
| 36 | Real-world -t d on stress binary shows function_00 at correct decimal offset | PASS | |
| 37 | Stress combination of -n 13 and -t x shows hex offsets for long symbols | PASS | |
| 38 | Stress scan of 324-byte binary file processes all bytes correctly | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE test suite for strings 1.0
# strings: find printable strings in binary files
# Source flags: -a (whole file, no-op), -n number (min len), -t format (d/o/x)
# Category 1 (CLI utility)

# -----------------------------------------------------------------------
# FUNCTIONAL: Default operation (no flags)
# -----------------------------------------------------------------------

# Native: strings ports/common-test-data/test-oneline.txt | head -1
TEST: Default operation prints printable strings from text file
CMD: WORK:strings WORK:test-oneline.txt
EXPECT: hello world
EXPECT_RC: 0

# Native: strings ports/strings/test-strings-binary.dat | head -1
TEST: Default operation extracts strings from binary file
CMD: WORK:strings WORK:test-strings-binary.dat
EXPECT: HelloAmiga
EXPECT_LINE: 2,WorldPort
EXPECT_RC: 0

# Native: strings ports/common-test-data/test-multiline.txt
TEST: Default output of multi-line text file (9 strings)
CMD: WORK:strings WORK:test-multiline.txt
EXPECT: hello world
EXPECT_LINE: 2,foo bar baz
EXPECT_LINE: 9,last line
EXPECT_RC: 0

# -----------------------------------------------------------------------
# FUNCTIONAL: -a flag (scan whole file -- no-op in this implementation)
# -----------------------------------------------------------------------

# Native: strings -a ports/common-test-data/test-oneline.txt | head -1
TEST: -a flag produces same output as default (scans whole file)
CMD: WORK:strings -a WORK:test-oneline.txt
EXPECT: hello world
EXPECT_RC: 0

# -----------------------------------------------------------------------
# FUNCTIONAL: -n flag (minimum string length)
# -----------------------------------------------------------------------

# Native: strings -n 3 ports/common-test-data/test-oneline.txt | head -1
TEST: -n 3 includes strings of 3+ chars
CMD: WORK:strings -n 3 WORK:test-oneline.txt
EXPECT: hello world
EXPECT_RC: 0

# Native: strings -n 11 ports/common-test-data/test-oneline.txt | head -1
TEST: -n 11 matches string of exactly 11 chars (hello world)
CMD: WORK:strings -n 11 WORK:test-oneline.txt
EXPECT: hello world
EXPECT_RC: 0

# Native: strings -n 12 ports/common-test-data/test-oneline.txt (no output)
TEST: -n 12 produces no output when no string is that long
CMD: WORK:strings -n 12 WORK:test-oneline.txt
EXPECT_RC: 0

# Native: strings -n 5 ports/strings/test-strings-mixed.dat (Hello=5, Testing=7)
TEST: -n 5 finds strings of 5 and 7 chars, skips 2-char strings
CMD: WORK:strings -n 5 WORK:test-strings-mixed.dat
EXPECT: Hello
EXPECT_LINE: 2,Testing
EXPECT_RC: 0

# Native: strings -n 6 ports/strings/test-strings-mixed.dat (Hello=5 excluded, Testing=7 included)
TEST: -n 6 skips 5-char Hello, finds 7-char Testing
CMD: WORK:strings -n 6 WORK:test-strings-mixed.dat
EXPECT: Testing
EXPECT_RC: 0

# Native: strings -n 9 ports/strings/test-strings-binary.dat (both HelloAmiga=10, WorldPort=9)
TEST: -n 9 finds both HelloAmiga and WorldPort
CMD: WORK:strings -n 9 WORK:test-strings-binary.dat
EXPECT: HelloAmiga
EXPECT_LINE: 2,WorldPort
EXPECT_RC: 0

# Native: strings -n 10 ports/strings/test-strings-binary.dat (WorldPort=9 excluded)
TEST: -n 10 finds HelloAmiga (10 chars) but not WorldPort (9 chars)
CMD: WORK:strings -n 10 WORK:test-strings-binary.dat
EXPECT: HelloAmiga
EXPECT_RC: 0

# Native: strings -n 1 ports/common-test-data/test-oneline.txt
TEST: -n 1 minimum valid value still produces correct output
CMD: WORK:strings -n 1 WORK:test-oneline.txt
EXPECT: hello world
EXPECT_RC: 0

# -----------------------------------------------------------------------
# FUNCTIONAL: -t flag (offset format)
# -----------------------------------------------------------------------

# Native (port): printf("%7d ", 5) then "HelloAmiga" -> "      5 HelloAmiga"
# The port uses %7ld for decimal offsets
TEST: -t d shows decimal offset of strings in binary file
CMD: WORK:strings -t d WORK:test-strings-binary.dat
EXPECT:       5 HelloAmiga
EXPECT_LINE: 2,      18 WorldPort
EXPECT_RC: 0

# Native (port): printf("%7o ", 5) = "      5", offset 18 in octal = 22 -> "     22"
TEST: -t o shows octal offset of strings in binary file
CMD: WORK:strings -t o WORK:test-strings-binary.dat
EXPECT:       5 HelloAmiga
EXPECT_LINE: 2,      22 WorldPort
EXPECT_RC: 0

# Native (port): printf("%7x ", 5) = "      5", offset 18 in hex = 12 -> "     12"
TEST: -t x shows hexadecimal offset of strings in binary file
CMD: WORK:strings -t x WORK:test-strings-binary.dat
EXPECT:       5 HelloAmiga
EXPECT_LINE: 2,      12 WorldPort
EXPECT_RC: 0

# -----------------------------------------------------------------------
# FUNCTIONAL: Multiple files
# -----------------------------------------------------------------------

# Native: strings ports/strings/test-strings-binary.dat ports/common-test-data/test-oneline.txt
TEST: Multiple file arguments processed in order
CMD: WORK:strings WORK:test-strings-binary.dat WORK:test-oneline.txt
EXPECT: HelloAmiga
EXPECT_LINE: 2,WorldPort
EXPECT_LINE: 3,hello world
EXPECT_RC: 0

# -----------------------------------------------------------------------
# FUNCTIONAL: stdin via - filename
# -----------------------------------------------------------------------

# The port handles "-" as filename to read stdin.
# We use a pre-created input file piped via AmigaDOS < redirect.
TEST: Dash argument reads from stdin (via AmigaDOS redirect)
CMD: WORK:strings - <WORK:test-oneline.txt
EXPECT: hello world
EXPECT_RC: 0

# -----------------------------------------------------------------------
# EDGE CASES
# -----------------------------------------------------------------------

# Native: strings ports/common-test-data/test-empty.txt (no output)
TEST: Empty file produces no output with RC 0
CMD: WORK:strings WORK:test-empty.txt
EXPECT_RC: 0

# String exactly at minimum length (4 chars: "abcd")
TEST: String exactly at minimum length 4 is printed
CMD: WORK:strings WORK:test-strings-exact4.txt
EXPECT: abcd
EXPECT_RC: 0

# String one char shorter than minimum (3 chars: "abc") should not print
TEST: String one char shorter than minimum produces no output
CMD: WORK:strings WORK:test-strings-short3.txt
EXPECT_RC: 0

# -n 4 on exact4 should match
TEST: -n 4 on exact-length 4-char string prints it
CMD: WORK:strings -n 4 WORK:test-strings-exact4.txt
EXPECT: abcd
EXPECT_RC: 0

# -n 5 on exact4 (4-char string) should produce no output
TEST: -n 5 on 4-char string produces no output (one short of threshold)
CMD: WORK:strings -n 5 WORK:test-strings-exact4.txt
EXPECT_RC: 0

# Long line test - ARexx READLN limit ~500 bytes; only check start of line
TEST: Long line file processed without crash (stress buffer handling)
CMD: WORK:strings WORK:test-longline.txt
EXPECT_CONTAINS: AAAAAAAAAAAAAAAAAAA
EXPECT_RC: 0

# -----------------------------------------------------------------------
# AMIGA-SPECIFIC
# -----------------------------------------------------------------------

# Test using WORK: volume paths explicitly (AmigaOS volume path handling)
TEST: WORK: volume paths are handled correctly
CMD: WORK:strings WORK:test-strings-binary.dat
EXPECT: HelloAmiga
EXPECT_RC: 0

# Test -t d with WORK: path (verifies offset calculation with Amiga fgetc)
TEST: Offset calculation via -t d is correct on AmigaOS fgetc
CMD: WORK:strings -t d WORK:test-strings-binary.dat
EXPECT:       5 HelloAmiga
EXPECT_RC: 0

# -----------------------------------------------------------------------
# ERROR PATHS
# -----------------------------------------------------------------------

# Nonexistent file: warn() goes to stderr, ret=10 returned
TEST: Nonexistent file produces RC 10 (error code)
CMD: WORK:strings WORK:nonexistent-file.txt
EXPECT_RC: 10

# Invalid -n value 0: errx(10, "invalid minimum string length: 0")
TEST: -n 0 is rejected with RC 10 (must be >= 1)
CMD: WORK:strings -n 0 WORK:test-oneline.txt
EXPECT_RC: 10

# Invalid -n non-numeric: atoi("abc")=0 < 1, errx(10, ...)
TEST: -n with non-numeric argument produces RC 10
CMD: WORK:strings -n abc WORK:test-oneline.txt
EXPECT_RC: 10

# Invalid -n negative: atoi("-5")=-5 < 1, errx(10, ...)
TEST: -n with negative value produces RC 10
CMD: WORK:strings -n -5 WORK:test-oneline.txt
EXPECT_RC: 10

# Invalid -t format: errx(10, "invalid offset format: z")
TEST: -t with invalid format character produces RC 10
CMD: WORK:strings -t z WORK:test-oneline.txt
EXPECT_RC: 10

# Unknown flag: getopt returns '?' -> usage() -> exit(10)
TEST: Unknown flag produces usage error with RC 10
CMD: WORK:strings -Z WORK:test-oneline.txt
EXPECT_RC: 10

# Nonexistent file with valid file: processes valid file, returns RC 10 for error
TEST: Mix of valid and missing files returns RC 10 but processes valid files
CMD: WORK:strings WORK:nonexistent1.txt WORK:test-oneline.txt
EXPECT: hello world
EXPECT_RC: 10

# -----------------------------------------------------------------------
# REAL-WORLD TESTS
# -----------------------------------------------------------------------

# Real-world: scan a binary executable-like file with known strings
# stress.dat has 22 strings (function_00..function_19, AmigaOS_string_target, version_1_2_3)
TEST: Real-world scan of binary with 22 embedded strings
CMD: WORK:strings WORK:test-strings-stress.dat
EXPECT: function_00
EXPECT_LINE: 22,version_1_2_3
EXPECT_RC: 0

# Real-world: filter for long symbol names only (-n 13 gives AmigaOS_string_target + version_1_2_3)
TEST: Real-world symbol extraction with -n 13 filters to long strings only
CMD: WORK:strings -n 13 WORK:test-strings-stress.dat
EXPECT: AmigaOS_string_target
EXPECT_LINE: 2,version_1_2_3
EXPECT_RC: 0

# Real-world: find unique long symbol (-n 20 gives only AmigaOS_string_target at 21 chars)
TEST: Real-world -n 20 finds only the 21-char symbol AmigaOS_string_target
CMD: WORK:strings -n 20 WORK:test-strings-stress.dat
EXPECT: AmigaOS_string_target
EXPECT_RC: 0

# Real-world: scan with decimal offsets to locate all function_ symbols
# Native (port): printf("%7d ", 7) = "      7 function_00"
TEST: Real-world -t d on stress binary shows function_00 at correct decimal offset
CMD: WORK:strings -t d WORK:test-strings-stress.dat
EXPECT:       7 function_00
EXPECT_RC: 0

# Stress: combine -n with -t to verify both work together
# Native (port): printf("%7x ", 0x11f) = "    11f AmigaOS_string_target"
TEST: Stress combination of -n 13 and -t x shows hex offsets for long symbols
CMD: WORK:strings -n 13 -t x WORK:test-strings-stress.dat
EXPECT:     11f AmigaOS_string_target
EXPECT_LINE: 2,    136 version_1_2_3
EXPECT_RC: 0

# Stress: scan large binary file (stress.dat 324 bytes) without crash or truncation
TEST: Stress scan of 324-byte binary file processes all bytes correctly
CMD: WORK:strings WORK:test-strings-stress.dat
EXPECT_LINE: 21,AmigaOS_string_target
EXPECT_LINE: 22,version_1_2_3
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
passed=38
failed=0
total=38
```

---
Generated by `make test-fsemu TARGET=ports/strings`
Report template: `toolchain/templates/test-report.md.template`
