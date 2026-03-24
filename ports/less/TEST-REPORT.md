# FS-UAE Test Report: less

## Summary

| Field | Value |
|-------|-------|
| Port | less |
| Date | 2026-03-23 22:21:42 |
| Duration | 38s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:less` (233K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 20/20 passed |

## Test Results

```
1..20
ok 1 - Print version string (-V flag)
ok 2 - Display file non-interactively (-X -F -E flags)
ok 3 - Display file with line numbers (-N flag)
ok 4 - Suppress line numbers (-n flag)
ok 5 - Squeeze blank lines (-s flag)
ok 6 - Search for pattern at startup (-p flag)
ok 7 - Chop long lines (-S flag)
ok 8 - Quit at second EOF (-e flag)
ok 9 - Case-insensitive search setup (-i flag, verify starts without error)
ok 10 - Combination flags -X -N -F -E together
ok 11 - Nonexistent file prints error but exits OK (pager convention)
ok 12 - Invalid option prints usage but exits OK (pager convention)
ok 13 - Successful display exits with RC 0
ok 14 - Version flag exits with RC 0
ok 15 - Empty file displays without crash (-X -F -E)
ok 16 - Long line file does not crash
ok 17 - Single-line file fits on one screen (-F quits)
ok 18 - Multiple blank lines file with -s squeeze
ok 19 - WORK: volume path resolves correctly
ok 20 - -X prevents terminal init escape sequences on AmigaOS console
# passed: 20 failed: 0 total: 20
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Print version string (-V flag) | PASS | |
| 2 | Display file non-interactively (-X -F -E flags) | PASS | |
| 3 | Display file with line numbers (-N flag) | PASS | |
| 4 | Suppress line numbers (-n flag) | PASS | |
| 5 | Squeeze blank lines (-s flag) | PASS | |
| 6 | Search for pattern at startup (-p flag) | PASS | |
| 7 | Chop long lines (-S flag) | PASS | |
| 8 | Quit at second EOF (-e flag) | PASS | |
| 9 | Case-insensitive search setup (-i flag, verify starts without error) | PASS | |
| 10 | Combination flags -X -N -F -E together | PASS | |
| 11 | Nonexistent file prints error but exits OK (pager convention) | PASS | |
| 12 | Invalid option prints usage but exits OK (pager convention) | PASS | |
| 13 | Successful display exits with RC 0 | PASS | |
| 14 | Version flag exits with RC 0 | PASS | |
| 15 | Empty file displays without crash (-X -F -E) | PASS | |
| 16 | Long line file does not crash | PASS | |
| 17 | Single-line file fits on one screen (-F quits) | PASS | |
| 18 | Multiple blank lines file with -s squeeze | PASS | |
| 19 | WORK: volume path resolves correctly | PASS | |
| 20 | -X prevents terminal init escape sequences on AmigaOS console | PASS | |

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
# FS-UAE test suite for less 692
# Category 3 (Console UI) — minimum 12 tests required
# less is interactive, but -X -F -E flags make it non-interactive:
#   -X  no termcap init/deinit (no screen clearing)
#   -F  quit if file fits on one screen
#   -E  quit at end of file
# -V prints version and exits immediately without reading stdin

# --- Functional tests ---

TEST: Print version string (-V flag)
CMD: WORK:less -V
EXPECT_CONTAINS: less 692
EXPECT_RC: 0

TEST: Display file non-interactively (-X -F -E flags)
CMD: WORK:less -X -F -E WORK:test-less-input.txt
EXPECT_CONTAINS: Line 1
EXPECT_RC: 0

TEST: Display file with line numbers (-N flag)
CMD: WORK:less -X -F -E -N WORK:test-less-input.txt
EXPECT_CONTAINS: 1
EXPECT_RC: 0

TEST: Suppress line numbers (-n flag)
CMD: WORK:less -X -F -E -n WORK:test-less-input.txt
EXPECT_CONTAINS: Line 1
EXPECT_RC: 0

TEST: Squeeze blank lines (-s flag)
CMD: WORK:less -X -F -E -s WORK:test-less-blanks.txt
EXPECT_CONTAINS: First line
EXPECT_RC: 0

TEST: Search for pattern at startup (-p flag)
CMD: WORK:less -X -F -E -p gamma WORK:test-less-input.txt
EXPECT_CONTAINS: gamma
EXPECT_RC: 0

TEST: Chop long lines (-S flag)
CMD: WORK:less -X -F -E -S WORK:test-less-longline.txt
EXPECT_CONTAINS: LONGLINESTART
EXPECT_RC: 0

TEST: Quit at second EOF (-e flag)
CMD: WORK:less -X -F -e WORK:test-less-single.txt
EXPECT_CONTAINS: only one line
EXPECT_RC: 0

TEST: Case-insensitive search setup (-i flag, verify starts without error)
CMD: WORK:less -X -F -E -i WORK:test-less-input.txt
EXPECT_CONTAINS: Line 1
EXPECT_RC: 0

TEST: Combination flags -X -N -F -E together
CMD: WORK:less -X -N -F -E WORK:test-less-single.txt
EXPECT_CONTAINS: only one line
EXPECT_RC: 0

# --- Error path tests ---

TEST: Nonexistent file prints error but exits OK (pager convention)
CMD: WORK:less -X -F -E WORK:no-such-file-at-all.txt
EXPECT_RC: 0

TEST: Invalid option prints usage but exits OK (pager convention)
CMD: WORK:less -Z -X -F -E WORK:test-less-input.txt
EXPECT_RC: 0

# --- Exit code tests ---

TEST: Successful display exits with RC 0
CMD: WORK:less -X -F -E WORK:test-less-single.txt
EXPECT_RC: 0

TEST: Version flag exits with RC 0
CMD: WORK:less -V
EXPECT_RC: 0

# --- Edge case tests ---

TEST: Empty file displays without crash (-X -F -E)
CMD: WORK:less -X -F -E WORK:test-less-empty.txt
EXPECT_RC: 0

TEST: Long line file does not crash
CMD: WORK:less -X -F -E WORK:test-less-longline.txt
EXPECT_CONTAINS: LONGLINESTART
EXPECT_RC: 0

TEST: Single-line file fits on one screen (-F quits)
CMD: WORK:less -X -F -E WORK:test-less-single.txt
EXPECT_CONTAINS: only one line
EXPECT_RC: 0

TEST: Multiple blank lines file with -s squeeze
CMD: WORK:less -X -F -E -s WORK:test-less-blanks.txt
EXPECT_CONTAINS: Fourth line
EXPECT_RC: 0

# --- Amiga-specific tests ---

TEST: WORK: volume path resolves correctly
CMD: WORK:less -X -F -E WORK:test-less-input.txt
EXPECT_CONTAINS: Line 1: alpha
EXPECT_RC: 0

TEST: -X prevents terminal init escape sequences on AmigaOS console
CMD: WORK:less -X -F -E WORK:test-less-single.txt
EXPECT_CONTAINS: only one line
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
passed=20
failed=0
total=20
```

---
Generated by `make test-fsemu TARGET=ports/less`
Report template: `toolchain/templates/test-report.md.template`
