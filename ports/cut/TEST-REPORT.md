# FS-UAE Test Report: cut

## Summary

| Field | Value |
|-------|-------|
| Port | cut |
| Date | 2026-03-21 22:23:37 |
| Duration | 60s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:cut` (31K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 20/20 passed |

## Test Results

```
1..20
ok 1 - Extract second field with colon delimiter (-f -d)
ok 2 - Extract first three characters with -c range
ok 3 - Extract bytes 1-3 with -b flag
ok 4 - Extract multiple fields with comma list (-f1,3)
ok 5 - Extract field range (-f2-4) with colon delimiter
ok 6 - Suppress lines without delimiter (-s flag passes only delimited lines)
ok 7 - Lines without delimiter are passed through unchanged when -s not set
ok 8 - Extract field from tab-delimited input (default delimiter is tab)
ok 9 - Extract characters from position 5 to end of line (-c5-)
ok 10 - Extract bytes with open-ended trailing range (-b3-)
ok 11 - Empty file produces no output and exits cleanly
ok 12 - Long line beyond _POSIX2_LINE_MAX handled by getline dynamic buffer
ok 13 - Nonexistent file returns RETURN_ERROR
ok 14 - No mode flag given triggers usage error (missing -b/-c/-f)
ok 15 - Mutually exclusive flags -b and -f trigger usage error
ok 16 - -d without -f triggers usage error (sflag/dflag require fflag)
ok 17 - -s without -f triggers usage error
ok 18 - Position 0 in list triggers errx (positions are 1-based)
ok 19 - Amiga WORK: volume path resolves correctly for multi-field extraction
ok 20 - Multiple file arguments processed in sequence via Amiga paths
# passed: 20 failed: 0 total: 20
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Extract second field with colon delimiter (-f -d) | PASS | |
| 2 | Extract first three characters with -c range | PASS | |
| 3 | Extract bytes 1-3 with -b flag | PASS | |
| 4 | Extract multiple fields with comma list (-f1,3) | PASS | |
| 5 | Extract field range (-f2-4) with colon delimiter | PASS | |
| 6 | Suppress lines without delimiter (-s flag passes only delimited lines) | PASS | |
| 7 | Lines without delimiter are passed through unchanged when -s not set | PASS | |
| 8 | Extract field from tab-delimited input (default delimiter is tab) | PASS | |
| 9 | Extract characters from position 5 to end of line (-c5-) | PASS | |
| 10 | Extract bytes with open-ended trailing range (-b3-) | PASS | |
| 11 | Empty file produces no output and exits cleanly | PASS | |
| 12 | Long line beyond _POSIX2_LINE_MAX handled by getline dynamic buffer | PASS | |
| 13 | Nonexistent file returns RETURN_ERROR | PASS | |
| 14 | No mode flag given triggers usage error (missing -b/-c/-f) | PASS | |
| 15 | Mutually exclusive flags -b and -f trigger usage error | PASS | |
| 16 | -d without -f triggers usage error (sflag/dflag require fflag) | PASS | |
| 17 | -s without -f triggers usage error | PASS | |
| 18 | Position 0 in list triggers errx (positions are 1-based) | PASS | |
| 19 | Amiga WORK: volume path resolves correctly for multi-field extraction | PASS | |
| 20 | Multiple file arguments processed in sequence via Amiga paths | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE ARexx test suite for cut (OpenBSD 1.28)
# Category 1 (CLI tool) -- minimum 8 tests required
# Uses input files instead of piping (ARexx ADDRESS COMMAND limitation)

TEST: Extract second field with colon delimiter (-f -d)
CMD: WORK:cut -d: -f2 WORK:test-cut-input.txt
EXPECT: b
EXPECT_RC: 0

TEST: Extract first three characters with -c range
CMD: WORK:cut -c1-3 WORK:test-cut-fields.txt
EXPECT: ali
EXPECT_RC: 0

TEST: Extract bytes 1-3 with -b flag
CMD: WORK:cut -b1-3 WORK:test-cut-fields.txt
EXPECT: ali
EXPECT_RC: 0

TEST: Extract multiple fields with comma list (-f1,3)
CMD: WORK:cut -d: -f1,3 WORK:test-cut-fields.txt
EXPECT: alice:rabbit
EXPECT_RC: 0

TEST: Extract field range (-f2-4) with colon delimiter
CMD: WORK:cut -d: -f2-4 WORK:test-cut-fields.txt
EXPECT: wonderland:rabbit:hole
EXPECT_RC: 0

TEST: Suppress lines without delimiter (-s flag passes only delimited lines)
CMD: WORK:cut -d: -f1 -s WORK:test-cut-mixed.txt
EXPECT: has
EXPECT_RC: 0

TEST: Lines without delimiter are passed through unchanged when -s not set
CMD: WORK:cut -d: -f1 WORK:test-cut-mixed.txt
EXPECT: no-delimiter-here
EXPECT_RC: 0

TEST: Extract field from tab-delimited input (default delimiter is tab)
CMD: WORK:cut -f2 WORK:test-cut-tabs.txt
EXPECT: second
EXPECT_RC: 0

TEST: Extract characters from position 5 to end of line (-c5-)
CMD: WORK:cut -c5- WORK:test-cut-fields.txt
EXPECT: e:wonderland:rabbit:hole
EXPECT_RC: 0

TEST: Extract bytes with open-ended trailing range (-b3-)
CMD: WORK:cut -b3- WORK:test-cut-input.txt
EXPECT: b:c
EXPECT_RC: 0

TEST: Empty file produces no output and exits cleanly
CMD: WORK:cut -c1 WORK:test-cut-empty.txt
EXPECT:
EXPECT_RC: 0

TEST: Long line beyond _POSIX2_LINE_MAX handled by getline dynamic buffer
CMD: WORK:cut -c1201-1206 WORK:test-cut-longline.txt
EXPECT: MARKER
EXPECT_RC: 0

TEST: Nonexistent file returns RETURN_ERROR
CMD: WORK:cut -c1 WORK:nonexistent-file.txt
EXPECT_RC: 10

TEST: No mode flag given triggers usage error (missing -b/-c/-f)
CMD: WORK:cut WORK:test-cut-input.txt
EXPECT_RC: 10

TEST: Mutually exclusive flags -b and -f trigger usage error
CMD: WORK:cut -b1 -f1 WORK:test-cut-input.txt
EXPECT_RC: 10

TEST: -d without -f triggers usage error (sflag/dflag require fflag)
CMD: WORK:cut -d: WORK:test-cut-input.txt
EXPECT_RC: 10

TEST: -s without -f triggers usage error
CMD: WORK:cut -s WORK:test-cut-input.txt
EXPECT_RC: 10

TEST: Position 0 in list triggers errx (positions are 1-based)
CMD: WORK:cut -c0 WORK:test-cut-input.txt
EXPECT_RC: 10

TEST: Amiga WORK: volume path resolves correctly for multi-field extraction
CMD: WORK:cut -d: -f3 WORK:test-cut-fields.txt
EXPECT: rabbit
EXPECT_RC: 0

TEST: Multiple file arguments processed in sequence via Amiga paths
CMD: WORK:cut -d: -f1 WORK:test-cut-input.txt WORK:test-cut-fields.txt
EXPECT: a
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
Generated by `make test-fsemu TARGET=ports/cut`
Report template: `toolchain/templates/test-report.md.template`
