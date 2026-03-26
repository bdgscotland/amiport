# FS-UAE Test Report: basename

## Summary

| Field | Value |
|-------|-------|
| Port | basename |
| Date | 2026-03-25 23:06:46 |
| Duration | 31s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:basename` (33K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 32/32 passed |

## Test Results

```
1..32
ok 1 - Strip directory prefix from absolute path
ok 2 - No directory component -- return filename unchanged
ok 3 - Trailing slash stripped before basename extraction
ok 4 - Root slash returns slash
ok 5 - Dot argument returns dot
ok 6 - Double-dot argument returns double-dot
ok 7 - Remove matching suffix from plain filename
ok 8 - Remove matching suffix from path filename
ok 9 - Non-matching suffix leaves filename unchanged
ok 10 - Suffix equal to entire string is NOT removed (strict less-than rule)
ok 11 - Suffix longer than string is not removed
ok 12 - Remove suffix that is shorter than full string
ok 13 - Remove only final extension leaving intermediate extension
ok 14 - Hidden dot-file returned unchanged when no suffix given
ok 15 - Empty string argument prints empty line
ok 16 - Double slash treated as root
ok 17 - Leading double slash strips to last component
ok 18 - Strip single dot as suffix
ok 19 - Amiga WORK: path with slash -- return final component
ok 20 - Amiga WORK: path with suffix removal
ok 21 - Amiga WORK: path with no slash -- libnix strips colon prefix
ok 22 - No arguments prints usage and exits RC=10
ok 23 - Three arguments prints usage and exits RC=10
ok 24 - Invalid flag -z exits RC=10
ok 25 - Invalid flag -a exits RC=10 (port does not support -a unlike macOS)
ok 26 - Real-world: strip .c suffix from kernel source path
ok 27 - Real-world: extract manpage name from deep path
ok 28 - Real-world: strip .sh from deeply nested script path
ok 29 - Stress: deeply nested 26-level path -- return leaf component
ok 30 - Stress: filename with many extensions -- strip only rightmost
ok 31 - Precision: suffix match is case-sensitive -- wrong case not stripped
ok 32 - Precision: multiple trailing slashes stripped before extraction
# passed: 32 failed: 0 total: 32
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Strip directory prefix from absolute path | PASS | |
| 2 | No directory component -- return filename unchanged | PASS | |
| 3 | Trailing slash stripped before basename extraction | PASS | |
| 4 | Root slash returns slash | PASS | |
| 5 | Dot argument returns dot | PASS | |
| 6 | Double-dot argument returns double-dot | PASS | |
| 7 | Remove matching suffix from plain filename | PASS | |
| 8 | Remove matching suffix from path filename | PASS | |
| 9 | Non-matching suffix leaves filename unchanged | PASS | |
| 10 | Suffix equal to entire string is NOT removed (strict less-than rule) | PASS | |
| 11 | Suffix longer than string is not removed | PASS | |
| 12 | Remove suffix that is shorter than full string | PASS | |
| 13 | Remove only final extension leaving intermediate extension | PASS | |
| 14 | Hidden dot-file returned unchanged when no suffix given | PASS | |
| 15 | Empty string argument prints empty line | PASS | |
| 16 | Double slash treated as root | PASS | |
| 17 | Leading double slash strips to last component | PASS | |
| 18 | Strip single dot as suffix | PASS | |
| 19 | Amiga WORK: path with slash -- return final component | PASS | |
| 20 | Amiga WORK: path with suffix removal | PASS | |
| 21 | Amiga WORK: path with no slash -- libnix strips colon prefix | PASS | |
| 22 | No arguments prints usage and exits RC=10 | PASS | |
| 23 | Three arguments prints usage and exits RC=10 | PASS | |
| 24 | Invalid flag -z exits RC=10 | PASS | |
| 25 | Invalid flag -a exits RC=10 (port does not support -a unlike macOS) | PASS | |
| 26 | Real-world: strip .c suffix from kernel source path | PASS | |
| 27 | Real-world: extract manpage name from deep path | PASS | |
| 28 | Real-world: strip .sh from deeply nested script path | PASS | |
| 29 | Stress: deeply nested 26-level path -- return leaf component | PASS | |
| 30 | Stress: filename with many extensions -- strip only rightmost | PASS | |
| 31 | Precision: suffix match is case-sensitive -- wrong case not stripped | PASS | |
| 32 | Precision: multiple trailing slashes stripped before extraction | PASS | |

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
# test-fsemu-cases.txt -- basename 1.14 FS-UAE test suite
# Category 1 (CLI tool) -- minimum 8 tests required
# All expected values verified by running native basename on macOS

# ============================================================
# FUNCTIONAL TESTS -- basic path stripping
# ============================================================

# Native: basename /usr/bin/sort
TEST: Strip directory prefix from absolute path
CMD: WORK:basename /usr/bin/sort
EXPECT: sort
EXPECT_RC: 0

# Native: basename sort
TEST: No directory component -- return filename unchanged
CMD: WORK:basename sort
EXPECT: sort
EXPECT_RC: 0

# Native: basename /usr/bin/
TEST: Trailing slash stripped before basename extraction
CMD: WORK:basename /usr/bin/
EXPECT: bin
EXPECT_RC: 0

# Native: basename /
TEST: Root slash returns slash
CMD: WORK:basename /
EXPECT: /
EXPECT_RC: 0

# Native: basename .
TEST: Dot argument returns dot
CMD: WORK:basename .
EXPECT: .
EXPECT_RC: 0

# Native: basename ..
TEST: Double-dot argument returns double-dot
CMD: WORK:basename ..
EXPECT: ..
EXPECT_RC: 0

# ============================================================
# SUFFIX REMOVAL TESTS
# ============================================================

# Native: basename sort.c .c
TEST: Remove matching suffix from plain filename
CMD: WORK:basename sort.c .c
EXPECT: sort
EXPECT_RC: 0

# Native: basename /usr/bin/sort.exe .exe
TEST: Remove matching suffix from path filename
CMD: WORK:basename /usr/bin/sort.exe .exe
EXPECT: sort
EXPECT_RC: 0

# Native: basename sort .c
TEST: Non-matching suffix leaves filename unchanged
CMD: WORK:basename sort .c
EXPECT: sort
EXPECT_RC: 0

# Native: basename hello hello
TEST: Suffix equal to entire string is NOT removed (strict less-than rule)
CMD: WORK:basename hello hello
EXPECT: hello
EXPECT_RC: 0

# Native: basename hi hello
TEST: Suffix longer than string is not removed
CMD: WORK:basename hi hello
EXPECT: hi
EXPECT_RC: 0

# Native: basename hello ello
TEST: Remove suffix that is shorter than full string
CMD: WORK:basename hello ello
EXPECT: h
EXPECT_RC: 0

# Native: basename /path/file.tar.gz .gz
TEST: Remove only final extension leaving intermediate extension
CMD: WORK:basename /path/file.tar.gz .gz
EXPECT: file.tar
EXPECT_RC: 0

# Native: basename /path/.hidden
TEST: Hidden dot-file returned unchanged when no suffix given
CMD: WORK:basename /path/.hidden
EXPECT: .hidden
EXPECT_RC: 0

# ============================================================
# EDGE CASES
# ============================================================

# Native: basename '' (empty string argument)
# Source: if (**argv == '\0') { puts(""); return 0; }
TEST: Empty string argument prints empty line
CMD: WORK:basename ""
EXPECT:
EXPECT_RC: 0

# Native: basename //
TEST: Double slash treated as root
CMD: WORK:basename //
EXPECT: /
EXPECT_RC: 0

# Native: basename //usr/bin
TEST: Leading double slash strips to last component
CMD: WORK:basename //usr/bin
EXPECT: bin
EXPECT_RC: 0

# Native: basename /path/file. .
TEST: Strip single dot as suffix
CMD: WORK:basename /path/file. .
EXPECT: file
EXPECT_RC: 0

# ============================================================
# AMIGA-SPECIFIC TESTS -- WORK: volume path handling
# ============================================================

# AmigaOS uses colon as volume separator: WORK:dir/file
# basename only treats slash as directory separator, so WORK: prefix is kept
# as part of the "directory" -- slash after it behaves normally

# Native: basename WORK:dir/filename
TEST: Amiga WORK: path with slash -- return final component
CMD: WORK:basename WORK:dir/filename
EXPECT: filename
EXPECT_RC: 0

# Native: basename WORK:dir/filename.c .c
TEST: Amiga WORK: path with suffix removal
CMD: WORK:basename WORK:dir/filename.c .c
EXPECT: filename
EXPECT_RC: 0

# On AmigaOS, libnix basename() treats colon as path separator (correct for AmigaDOS).
# WORK:filename strips the "WORK:" volume prefix, returning just "filename".
# This differs from native POSIX basename (which only recognizes /).
TEST: Amiga WORK: path with no slash -- libnix strips colon prefix
CMD: WORK:basename WORK:filename
EXPECT: filename
EXPECT_RC: 0

# ============================================================
# ERROR PATH TESTS
# ============================================================

# Source: if (argc != 1 && argc != 2) usage(); -- usage() calls exit(10)
# Error output goes to stderr -- not captured by test harness
# Test exit code only

TEST: No arguments prints usage and exits RC=10
CMD: WORK:basename
EXPECT_RC: 10

TEST: Three arguments prints usage and exits RC=10
CMD: WORK:basename a b c
EXPECT_RC: 10

# Source: getopt(argc, argv, "") -- empty string, any flag hits default: usage()
TEST: Invalid flag -z exits RC=10
CMD: WORK:basename -z hello
EXPECT_RC: 10

TEST: Invalid flag -a exits RC=10 (port does not support -a unlike macOS)
CMD: WORK:basename -a hello
EXPECT_RC: 10

# ============================================================
# REAL-WORLD AND STRESS TESTS
# ============================================================

# Real-world: typical script usage -- stripping .c to get object name
# Native: basename /usr/src/sys/kern/vfs_lookup.c .c
TEST: Real-world: strip .c suffix from kernel source path
CMD: WORK:basename /usr/src/sys/kern/vfs_lookup.c .c
EXPECT: vfs_lookup
EXPECT_RC: 0

# Real-world: extracting manpage name without section suffix
# Native: basename /usr/local/share/man/man1/basename.1 .1
TEST: Real-world: extract manpage name from deep path
CMD: WORK:basename /usr/local/share/man/man1/basename.1 .1
EXPECT: basename
EXPECT_RC: 0

# Real-world: shell script stripping .sh extension
# Native: basename /some/very/long/path/to/a/file/with/many/dirs/script.sh .sh
TEST: Real-world: strip .sh from deeply nested script path
CMD: WORK:basename /some/very/long/path/to/a/file/with/many/dirs/script.sh .sh
EXPECT: script
EXPECT_RC: 0

# Stress: path with many directory components (deep nesting)
# Native: basename /a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/r/s/t/u/v/w/x/y/z/leaf
TEST: Stress: deeply nested 26-level path -- return leaf component
CMD: WORK:basename /a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/r/s/t/u/v/w/x/y/z/leaf
EXPECT: leaf
EXPECT_RC: 0

# Stress: filename with many dots -- only strip exact suffix match
# Native: basename /path/a.b.c.d.e.f .f
TEST: Stress: filename with many extensions -- strip only rightmost
CMD: WORK:basename /path/a.b.c.d.e.f .f
EXPECT: a.b.c.d.e
EXPECT_RC: 0

# Precision: verify suffix stripping is exact byte match, not prefix/partial
# Native: basename result.exe .EXE  (case mismatch -- no strip)
TEST: Precision: suffix match is case-sensitive -- wrong case not stripped
CMD: WORK:basename result.exe .EXE
EXPECT: result.exe
EXPECT_RC: 0

# Precision: verify trailing slash removal before basename extraction
# Native: basename /usr/local///
TEST: Precision: multiple trailing slashes stripped before extraction
CMD: WORK:basename /usr/local///
EXPECT: local
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
passed=32
failed=0
total=32
```

---
Generated by `make test-fsemu TARGET=ports/basename`
Report template: `toolchain/templates/test-report.md.template`
