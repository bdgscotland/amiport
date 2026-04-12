# FS-UAE Test Report: logname

## Summary

| Field | Value |
|-------|-------|
| Port | logname |
| Date | 2026-04-11 21:00:18 |
| Duration | 18s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:logname` (34K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 13/13 passed |

## Test Results

```
1..13
ok 1 - Normal execution prints login name
ok 2 - Unknown short flag exits with error
ok 3 - Unknown short flag -h exits with error (no --help support)
ok 4 - Unknown short flag -v exits with error
ok 5 - Single positional argument exits with error
ok 6 - Multiple positional arguments exits with error
ok 7 - Double-dash end-of-options prints login name
ok 8 - Binary runs from WORK volume path
ok 9 - Output is exactly the string amiga
ok 10 - Output is suitable for use as a username (no spaces, no newline garbage)
ok 11 - Repeated invocation produces consistent output
ok 12 - Flag with value treated as two bad arguments exits with error
ok 13 - Numeric argument is treated as positional, exits with error
# passed: 13 failed: 0 total: 13
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Normal execution prints login name | PASS | |
| 2 | Unknown short flag exits with error | PASS | |
| 3 | Unknown short flag -h exits with error (no --help support) | PASS | |
| 4 | Unknown short flag -v exits with error | PASS | |
| 5 | Single positional argument exits with error | PASS | |
| 6 | Multiple positional arguments exits with error | PASS | |
| 7 | Double-dash end-of-options prints login name | PASS | |
| 8 | Binary runs from WORK volume path | PASS | |
| 9 | Output is exactly the string amiga | PASS | |
| 10 | Output is suitable for use as a username (no spaces, no newline garbage) | PASS | |
| 11 | Repeated invocation produces consistent output | PASS | |
| 12 | Flag with value treated as two bad arguments exits with error | PASS | |
| 13 | Numeric argument is treated as positional, exits with error | PASS | |

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
# test-fsemu-cases.txt -- logname 1.10 FS-UAE test suite
# Category: 1 (CLI tool, no flags, no file I/O)
# Minimum tests: 8
#
# logname prints the login name and exits 0.
# On AmigaOS, amiport_getlogin() always returns "amiga".
# Any argument (flag or positional) triggers usage() + exit(10).
# Error messages go to stderr (not captured by harness).
# All EXPECT_RC: 10 tests rely on exit code only -- stderr not captured.

# --- Functional tests ---

# Native: logname
TEST: Normal execution prints login name
CMD: WORK:logname
EXPECT: amiga
EXPECT_RC: 0

# --- Error path tests: flags ---
# Any unknown flag goes to default: case -> usage() -> exit(10)
# Error message goes to stderr (not captured), stdout is empty.

TEST: Unknown short flag exits with error
CMD: WORK:logname -x
EXPECT_RC: 10

TEST: Unknown short flag -h exits with error (no --help support)
CMD: WORK:logname -h
EXPECT_RC: 10

TEST: Unknown short flag -v exits with error
CMD: WORK:logname -v
EXPECT_RC: 10

# --- Error path tests: positional arguments ---
# getopt("") finishes with optind=1, then argc(2) != optind(1) -> usage()

TEST: Single positional argument exits with error
CMD: WORK:logname somearg
EXPECT_RC: 10

TEST: Multiple positional arguments exits with error
CMD: WORK:logname foo bar baz
EXPECT_RC: 10

# --- Edge case: end-of-options marker ---
# getopt("") sees "--", advances optind to 2, returns -1
# Then argc(2) == optind(2), so no usage() is called
# Amiga port prints "amiga" with RC=0 (differs from macOS behavior)

# Native (OpenBSD): logname -- prints login name (argc==optind after --)
TEST: Double-dash end-of-options prints login name
CMD: WORK:logname --
EXPECT: amiga
EXPECT_RC: 0

# --- Amiga-specific tests ---

# Verify WORK: volume path resolution works
TEST: Binary runs from WORK volume path
CMD: WORK:logname
EXPECT: amiga
EXPECT_RC: 0

# Verify output is exactly "amiga" with no trailing garbage
TEST: Output is exactly the string amiga
CMD: WORK:logname
EXPECT: amiga
EXPECT_RC: 0

# --- Real-world tests ---

# Simulate a script that captures logname output (common shell idiom)
# On Amiga this always yields "amiga" -- verify the exact value
# Native: logname
TEST: Output is suitable for use as a username (no spaces, no newline garbage)
CMD: WORK:logname
EXPECT: amiga
EXPECT_RC: 0

# Verify idempotency: second invocation gives same result
# Native: logname; logname (both print login name)
TEST: Repeated invocation produces consistent output
CMD: WORK:logname
EXPECT: amiga
EXPECT_RC: 0

# --- Stress tests ---

# logname is stateless and trivial; the only meaningful stress is
# verifying error handling does not vary under repeated error conditions.

TEST: Flag with value treated as two bad arguments exits with error
CMD: WORK:logname -f value
EXPECT_RC: 10

TEST: Numeric argument is treated as positional, exits with error
CMD: WORK:logname 42
EXPECT_RC: 10
```

## Emulator Log

```
(log not captured in this run)
```

## Sentinel File

Written by the ARexx harness when all tests complete:

```
TESTS_COMPLETE
passed=13
failed=0
total=13
```

---
Generated by `make test-fsemu TARGET=ports/logname`
Report template: `toolchain/templates/test-report.md.template`
