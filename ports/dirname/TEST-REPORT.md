# FS-UAE Test Report: dirname

## Summary

| Field | Value |
|-------|-------|
| Port | dirname |
| Date | 2026-03-25 23:09:41 |
| Duration | 26s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:dirname` (33K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 24/24 passed |

## Test Results

```
1..24
ok 1 - Strip filename from absolute path
ok 2 - Strip trailing component with trailing slash
ok 3 - Single-level absolute path returns root
ok 4 - Strip filename from relative path
ok 5 - Strip filename from multi-level relative path
ok 6 - Strip filename from multi-level absolute path
ok 7 - Deep five-level absolute path
ok 8 - Root path returns root
ok 9 - Bare filename with no directory returns dot
ok 10 - Single dot returns dot
ok 11 - Double dot returns dot
ok 12 - Absolute path with trailing slash strips trailing component
ok 13 - Relative path with trailing slash returns dot
ok 14 - Double slash in path component
ok 15 - AmigaDOS WORK: volume path strips filename
ok 16 - AmigaDOS WORK: multi-level path strips last component
ok 17 - No arguments prints usage and exits with RC 10
ok 18 - Too many arguments prints usage and exits with RC 10
ok 19 - Invalid flag prints usage and exits with RC 10
ok 20 - Real-world config file path
ok 21 - Real-world library path
ok 22 - Twenty-component deep path
ok 23 - Long filename component stripped correctly
ok 24 - Single-character root directory exact output
# passed: 24 failed: 0 total: 24
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Strip filename from absolute path | PASS | |
| 2 | Strip trailing component with trailing slash | PASS | |
| 3 | Single-level absolute path returns root | PASS | |
| 4 | Strip filename from relative path | PASS | |
| 5 | Strip filename from multi-level relative path | PASS | |
| 6 | Strip filename from multi-level absolute path | PASS | |
| 7 | Deep five-level absolute path | PASS | |
| 8 | Root path returns root | PASS | |
| 9 | Bare filename with no directory returns dot | PASS | |
| 10 | Single dot returns dot | PASS | |
| 11 | Double dot returns dot | PASS | |
| 12 | Absolute path with trailing slash strips trailing component | PASS | |
| 13 | Relative path with trailing slash returns dot | PASS | |
| 14 | Double slash in path component | PASS | |
| 15 | AmigaDOS WORK: volume path strips filename | PASS | |
| 16 | AmigaDOS WORK: multi-level path strips last component | PASS | |
| 17 | No arguments prints usage and exits with RC 10 | PASS | |
| 18 | Too many arguments prints usage and exits with RC 10 | PASS | |
| 19 | Invalid flag prints usage and exits with RC 10 | PASS | |
| 20 | Real-world config file path | PASS | |
| 21 | Real-world library path | PASS | |
| 22 | Twenty-component deep path | PASS | |
| 23 | Long filename component stripped correctly | PASS | |
| 24 | Single-character root directory exact output | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE test suite for dirname 1.17
# Category 1 (CLI tool) -- minimum 15 tests required
# dirname strips the last component from a filename.
# Usage: dirname pathname
# RC=0 on success, RC=10 on usage error (no flags, any flag -> usage)
#
# All EXPECT: values derived by running native macOS dirname.
# No test input files needed -- dirname takes arguments only.

# --- Functional: basic path stripping ---

# Native: dirname /usr/bin/ls
TEST: Strip filename from absolute path
CMD: WORK:dirname /usr/bin/ls
EXPECT: /usr/bin
EXPECT_RC: 0

# Native: dirname /usr/bin/
TEST: Strip trailing component with trailing slash
CMD: WORK:dirname /usr/bin/
EXPECT: /usr
EXPECT_RC: 0

# Native: dirname /usr
TEST: Single-level absolute path returns root
CMD: WORK:dirname /usr
EXPECT: /
EXPECT_RC: 0

# Native: dirname foo/bar
TEST: Strip filename from relative path
CMD: WORK:dirname foo/bar
EXPECT: foo
EXPECT_RC: 0

# Native: dirname foo/bar/baz
TEST: Strip filename from multi-level relative path
CMD: WORK:dirname foo/bar/baz
EXPECT: foo/bar
EXPECT_RC: 0

# Native: dirname /foo/bar/baz
TEST: Strip filename from multi-level absolute path
CMD: WORK:dirname /foo/bar/baz
EXPECT: /foo/bar
EXPECT_RC: 0

# Native: dirname /a/b/c/d/e
TEST: Deep five-level absolute path
CMD: WORK:dirname /a/b/c/d/e
EXPECT: /a/b/c/d
EXPECT_RC: 0

# --- Functional: edge cases per POSIX spec ---

# Native: dirname /
TEST: Root path returns root
CMD: WORK:dirname /
EXPECT: /
EXPECT_RC: 0

# Native: dirname foo
TEST: Bare filename with no directory returns dot
CMD: WORK:dirname foo
EXPECT: .
EXPECT_RC: 0

# Native: dirname .
TEST: Single dot returns dot
CMD: WORK:dirname .
EXPECT: .
EXPECT_RC: 0

# Native: dirname ..
TEST: Double dot returns dot
CMD: WORK:dirname ..
EXPECT: .
EXPECT_RC: 0

# Native: dirname /usr/bin/ls/
TEST: Absolute path with trailing slash strips trailing component
CMD: WORK:dirname /usr/bin/ls/
EXPECT: /usr/bin
EXPECT_RC: 0

# Native: dirname foo/
TEST: Relative path with trailing slash returns dot
CMD: WORK:dirname foo/
EXPECT: .
EXPECT_RC: 0

# Native: dirname /usr//bin
TEST: Double slash in path component
CMD: WORK:dirname /usr//bin
EXPECT: /usr
EXPECT_RC: 0

# --- Amiga-specific: WORK: volume path handling ---

# Native: dirname WORK:foo/bar
TEST: AmigaDOS WORK: volume path strips filename
CMD: WORK:dirname WORK:foo/bar
EXPECT: WORK:foo
EXPECT_RC: 0

# Native: dirname WORK:foo/bar/baz
TEST: AmigaDOS WORK: multi-level path strips last component
CMD: WORK:dirname WORK:foo/bar/baz
EXPECT: WORK:foo/bar
EXPECT_RC: 0

# --- Error path tests ---
# Note: error messages go to stderr and are NOT captured by the harness.
# Use EXPECT_RC: 10 only.

TEST: No arguments prints usage and exits with RC 10
CMD: WORK:dirname
EXPECT_RC: 10

TEST: Too many arguments prints usage and exits with RC 10
CMD: WORK:dirname /usr/bin /etc
EXPECT_RC: 10

TEST: Invalid flag prints usage and exits with RC 10
CMD: WORK:dirname -h
EXPECT_RC: 10

# --- Real-world and stress tests ---

# Real-world: extracting the directory of a deeply nested config file
# Native: dirname /home/user/.config/app/settings.conf
TEST: Real-world config file path
CMD: WORK:dirname /home/user/.config/app/settings.conf
EXPECT: /home/user/.config/app
EXPECT_RC: 0

# Real-world: Makefile-style variable extraction (common in shell scripts)
# Native: dirname /usr/local/lib/libfoo.so
TEST: Real-world library path
CMD: WORK:dirname /usr/local/lib/libfoo.so
EXPECT: /usr/local/lib
EXPECT_RC: 0

# Stress: maximum path depth -- many components
# Native: dirname /a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/r/s/t
TEST: Twenty-component deep path
CMD: WORK:dirname /a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/r/s/t
EXPECT: /a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/r/s
EXPECT_RC: 0

# Stress: long filename component
# Native: dirname /usr/bin/averylongprogramnamethatexceedsthirtycharacters
TEST: Long filename component stripped correctly
CMD: WORK:dirname /usr/bin/averylongprogramnamethatexceedsthirtycharacters
EXPECT: /usr/bin
EXPECT_RC: 0

# Precision: verify exact output character-by-character -- no trailing spaces
# Native: dirname /x produces exactly "/"
TEST: Single-character root directory exact output
CMD: WORK:dirname /x
EXPECT: /
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
passed=24
failed=0
total=24
```

---
Generated by `make test-fsemu TARGET=ports/dirname`
Report template: `toolchain/templates/test-report.md.template`
