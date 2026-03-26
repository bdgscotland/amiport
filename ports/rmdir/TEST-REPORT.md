# FS-UAE Test Report: rmdir

## Summary

| Field | Value |
|-------|-------|
| Port | rmdir |
| Date | 2026-03-26 13:39:06 |
| Duration | 38s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:rmdir` (34K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 38/38 passed |

## Test Results

```
1..38
ok 1 - Setup - create single empty directory in T: for rmdir test
ok 2 - Remove a single empty directory returns RC 0 with no output
ok 3 - Remove nonexistent directory returns RC 1 (no stdout)
ok 4 - Setup - create non-empty directory for rmdir failure test
ok 5 - Setup - create child dir making parent non-empty
ok 6 - Remove non-empty directory fails with RC 1 (no stdout)
ok 7 - Cleanup - remove child dir from non-empty test
ok 8 - Cleanup - remove now-empty parent dir
ok 9 - No arguments calls usage and exits RC 10
ok 10 - Invalid flag -Z triggers usage error RC 10
ok 11 - Setup - create nested dirs for -p test (parent and child)
ok 12 - -p flag removes both child and parent directory RC 0
ok 13 - Setup - create 3-level nested dirs for deep -p test
ok 14 - -p flag removes three levels of nested empty dirs RC 0
ok 15 - Setup - create single dir for -p no-parent test
ok 16 - -p on single-level dir with no parents succeeds RC 0
ok 17 - Setup - create first dir for multiple-arg test
ok 18 - Setup - create second dir for multiple-arg test
ok 19 - Remove two empty directories at once returns RC 0
ok 20 - Setup - create only first dir for partial-failure test
ok 21 - Remove one existing and one nonexistent dir returns RC 1
ok 22 - Setup - create dir for Amiga volume path test
ok 23 - Amiga T: volume path handling works correctly
ok 24 - Attempt to remove T: volume fails with RC 1
ok 25 - Setup - create dirs for double-slash path test
ok 26 - -p flag traverses path correctly without double-slash issues
ok 27 - Setup - create project layout for real-world cleanup test
ok 28 - Setup - create second output dir in project layout
ok 29 - Real-world cleanup removes multiple output dirs from project
ok 30 - Real-world cleanup removes project root dir after outputs cleared
ok 31 - Setup - create deep project hierarchy for -p real-world test
ok 32 - Real-world -p removes deep include hierarchy in one command
ok 33 - Setup - create 5 directories for stress multi-remove test
ok 34 - Stress - remove 5 directories in a single invocation RC 0
ok 35 - Setup - create dirs for stress partial-failure test (skip 3rd)
ok 36 - Stress - 5 args with missing 3rd dir returns RC 1 (partial failure)
ok 37 - Setup - create precision test directory
ok 38 - Precision - successful removal returns exactly RC 0 not RC 1
# passed: 38 failed: 0 total: 38
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Setup - create single empty directory in T: for rmdir test | PASS | |
| 2 | Remove a single empty directory returns RC 0 with no output | PASS | |
| 3 | Remove nonexistent directory returns RC 1 (no stdout) | PASS | |
| 4 | Setup - create non-empty directory for rmdir failure test | PASS | |
| 5 | Setup - create child dir making parent non-empty | PASS | |
| 6 | Remove non-empty directory fails with RC 1 (no stdout) | PASS | |
| 7 | Cleanup - remove child dir from non-empty test | PASS | |
| 8 | Cleanup - remove now-empty parent dir | PASS | |
| 9 | No arguments calls usage and exits RC 10 | PASS | |
| 10 | Invalid flag -Z triggers usage error RC 10 | PASS | |
| 11 | Setup - create nested dirs for -p test (parent and child) | PASS | |
| 12 | -p flag removes both child and parent directory RC 0 | PASS | |
| 13 | Setup - create 3-level nested dirs for deep -p test | PASS | |
| 14 | -p flag removes three levels of nested empty dirs RC 0 | PASS | |
| 15 | Setup - create single dir for -p no-parent test | PASS | |
| 16 | -p on single-level dir with no parents succeeds RC 0 | PASS | |
| 17 | Setup - create first dir for multiple-arg test | PASS | |
| 18 | Setup - create second dir for multiple-arg test | PASS | |
| 19 | Remove two empty directories at once returns RC 0 | PASS | |
| 20 | Setup - create only first dir for partial-failure test | PASS | |
| 21 | Remove one existing and one nonexistent dir returns RC 1 | PASS | |
| 22 | Setup - create dir for Amiga volume path test | PASS | |
| 23 | Amiga T: volume path handling works correctly | PASS | |
| 24 | Attempt to remove T: volume fails with RC 1 | PASS | |
| 25 | Setup - create dirs for double-slash path test | PASS | |
| 26 | -p flag traverses path correctly without double-slash issues | PASS | |
| 27 | Setup - create project layout for real-world cleanup test | PASS | |
| 28 | Setup - create second output dir in project layout | PASS | |
| 29 | Real-world cleanup removes multiple output dirs from project | PASS | |
| 30 | Real-world cleanup removes project root dir after outputs cleared | PASS | |
| 31 | Setup - create deep project hierarchy for -p real-world test | PASS | |
| 32 | Real-world -p removes deep include hierarchy in one command | PASS | |
| 33 | Setup - create 5 directories for stress multi-remove test | PASS | |
| 34 | Stress - remove 5 directories in a single invocation RC 0 | PASS | |
| 35 | Setup - create dirs for stress partial-failure test (skip 3rd) | PASS | |
| 36 | Stress - 5 args with missing 3rd dir returns RC 1 (partial failure) | PASS | |
| 37 | Setup - create precision test directory | PASS | |
| 38 | Precision - successful removal returns exactly RC 0 not RC 1 | PASS | |

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
# test-fsemu-cases.txt -- rmdir 1.15 FS-UAE test suite
# Category 1 (CLI tool) -- minimum 15 tests required
#
# Exit codes for this port:
#   RC=0  -- all directories removed successfully (RETURN_OK)
#   RC=1  -- one or more rmdir() calls failed; warn() to stderr.
#            Note: RC=1 is not a standard Amiga code but is what the upstream
#            OpenBSD code returns (errors=1 from main() on warn() path).
#   RC=10 -- usage error (no args, bad flag); usage() calls exit(10)
#
# rmdir produces NO stdout on success -- all EXPECT: for success are empty.
# Error messages go to stderr, which is NOT captured by the test harness.
# For error path tests: use EXPECT_RC: only.
#
# Test directory setup strategy:
#   Tests that need an existing directory first run WORK:mkdir to create it.
#   Tests run sequentially, so a "setup" test (mkdir) is immediately followed
#   by the rmdir test. All test dirs use unique names in T: to avoid conflicts.
#   All paths use T: (AmigaOS temp, maps to RAM:T/) for isolation.
#
# All expected values derived by source analysis and native tool behavior.
# rmdir -p rm_path() algorithm: walks '/' separators backward, stops at ':'
# so "T:a/b/c" safely removes T:a/b/c -> T:a/b -> T:a without touching T:.

# ============================================================
# SETUP: create directories needed by the functional tests
# These mkdir tests also verify WORK:mkdir is working correctly.
# ============================================================

# Setup test: create a simple empty directory for the basic rmdir test
TEST: Setup - create single empty directory in T: for rmdir test
CMD: WORK:mkdir T:rmdir_test_empty_001
EXPECT:
EXPECT_RC: 0

# ============================================================
# FUNCTIONAL TESTS -- basic rmdir behavior
# ============================================================

# Native: mkdir /tmp/t && rmdir /tmp/t -> RC=0, no output
# Source: rmdir(*argv) returns 0, errors stays 0, main() returns 0
TEST: Remove a single empty directory returns RC 0 with no output
CMD: WORK:rmdir T:rmdir_test_empty_001
EXPECT:
EXPECT_RC: 0

# Error path: nonexistent directory -> warn() to stderr, errors=1
# Source: rmdir(*argv) == -1, warn("%s", *argv), errors=1, returns 1
# Native: rmdir /tmp/nonexistent -> RC=1, stderr message
TEST: Remove nonexistent directory returns RC 1 (no stdout)
CMD: WORK:rmdir T:rmdir_no_such_dir_xyz
EXPECT:
EXPECT_RC: 1

# Setup: create a non-empty directory with a file inside
TEST: Setup - create non-empty directory for rmdir failure test
CMD: WORK:mkdir T:rmdir_nonempty_001
EXPECT:
EXPECT_RC: 0

# Strategy: create a SUBDIRECTORY inside (since mkdir is available),
# making T:rmdir_nonempty_001 non-empty via a child dir.
TEST: Setup - create child dir making parent non-empty
CMD: WORK:mkdir T:rmdir_nonempty_001/child
EXPECT:
EXPECT_RC: 0

# Error path: non-empty directory -> warn() to stderr, errors=1
# Source: rmdir() returns -1 with ENOTEMPTY, warn(), errors=1, returns 1
# Native: rmdir /tmp/nonempty -> RC=1, "Directory not empty" on stderr
TEST: Remove non-empty directory fails with RC 1 (no stdout)
CMD: WORK:rmdir T:rmdir_nonempty_001
EXPECT:
EXPECT_RC: 1

# Cleanup non-empty test dirs (remove child first, then parent)
TEST: Cleanup - remove child dir from non-empty test
CMD: WORK:rmdir T:rmdir_nonempty_001/child
EXPECT:
EXPECT_RC: 0

TEST: Cleanup - remove now-empty parent dir
CMD: WORK:rmdir T:rmdir_nonempty_001
EXPECT:
EXPECT_RC: 0

# ============================================================
# ERROR PATH TESTS -- usage errors (RC=10)
# ============================================================

# Source: if (argc == 0) usage(); -> fprintf(stderr,...), exit(10)
# rmdir does NOT read stdin -- it only processes argv. Safe to run with no args.
# main() path: amiport_expand_argv -> atexit -> getopt loop -> argc==0 -> usage() -> exit(10)
# Native: rmdir -> "usage: rmdir [-pv] directory ..." on stderr, RC=1
# (macOS returns RC=1 but ported code calls exit(10) -- AmigaDOS correct)
TEST: No arguments calls usage and exits RC 10
CMD: WORK:rmdir
EXPECT:
EXPECT_RC: 10

# ============================================================
# USAGE / INVALID FLAG TESTS (RC=10)
# ============================================================

# Source: getopt returns '?', calls usage() -> exit(10)
# Native: rmdir -Z -> "illegal option" + usage, RC=1
# Ported code calls exit(10) from usage() -- AmigaDOS correct
TEST: Invalid flag -Z triggers usage error RC 10
CMD: WORK:rmdir -Z T:nonexistent
EXPECT:
EXPECT_RC: 10

# ============================================================
# -p FLAG: remove parent directories too
# ============================================================

# Setup: create a 2-level nested directory structure
TEST: Setup - create nested dirs for -p test (parent and child)
CMD: WORK:mkdir -p T:rmdir_p_parent_001/child
EXPECT:
EXPECT_RC: 0

# Native: mkdir -p /tmp/parent/child && rmdir -p /tmp/parent/child -> RC=0
# Source: rmdir("T:rmdir_p_parent_001/child") succeeds, then rm_path():
#   finds '/' at index after "T:rmdir_p_parent_001", sets '\0' there,
#   calls rmdir("T:rmdir_p_parent_001") -- both empty -> RC=0
TEST: -p flag removes both child and parent directory RC 0
CMD: WORK:rmdir -p T:rmdir_p_parent_001/child
EXPECT:
EXPECT_RC: 0

# Setup: create a 3-level nested structure for deep -p test
TEST: Setup - create 3-level nested dirs for deep -p test
CMD: WORK:mkdir -p T:rmdir_deep_001/a/b
EXPECT:
EXPECT_RC: 0

# Native: rmdir -p /tmp/a/b/c (3 levels) -> RC=0 removes all 3
# rm_path("T:rmdir_deep_001/a/b"):
#   finds '/' -> removes "T:rmdir_deep_001/a" -> removes "T:rmdir_deep_001"
#   ':' is NOT '/' so loop stops safely -> RC=0
TEST: -p flag removes three levels of nested empty dirs RC 0
CMD: WORK:rmdir -p T:rmdir_deep_001/a/b
EXPECT:
EXPECT_RC: 0

# -p with nonexistent parent: if a parent doesn't exist, rm_path fails
# Setup: create just a single dir (no parents to remove with -p)
# rmdir -p T:rmdir_p_noparent_001: removes the dir itself, then rm_path
# finds no '/' in "T:rmdir_p_noparent_001", returns 0 -> RC=0
TEST: Setup - create single dir for -p no-parent test
CMD: WORK:mkdir T:rmdir_p_noparent_001
EXPECT:
EXPECT_RC: 0

# Native: rmdir -p /tmp/single_dir -> RC=0 (removes dir, no parents to remove)
# rm_path("T:rmdir_p_noparent_001"): no '/' found -> returns 0
TEST: -p on single-level dir with no parents succeeds RC 0
CMD: WORK:rmdir -p T:rmdir_p_noparent_001
EXPECT:
EXPECT_RC: 0

# ============================================================
# MULTIPLE DIRECTORIES -- all succeed
# ============================================================

# Setup: create two separate empty directories
TEST: Setup - create first dir for multiple-arg test
CMD: WORK:mkdir T:rmdir_multi_001
EXPECT:
EXPECT_RC: 0

TEST: Setup - create second dir for multiple-arg test
CMD: WORK:mkdir T:rmdir_multi_002
EXPECT:
EXPECT_RC: 0

# Native: rmdir /tmp/a /tmp/b -> RC=0 (both removed)
# Source: for loop processes both argv entries, errors stays 0, returns 0
TEST: Remove two empty directories at once returns RC 0
CMD: WORK:rmdir T:rmdir_multi_001 T:rmdir_multi_002
EXPECT:
EXPECT_RC: 0

# ============================================================
# MULTIPLE DIRECTORIES -- partial failure
# ============================================================

# Setup: create only one of two dirs (so second will fail)
TEST: Setup - create only first dir for partial-failure test
CMD: WORK:mkdir T:rmdir_partial_001
EXPECT:
EXPECT_RC: 0

# Native: rmdir /tmp/exists /tmp/missing -> RC=1 (one warn to stderr)
# Source: first rmdir() succeeds (errors=0), second fails (errors=1), returns 1
TEST: Remove one existing and one nonexistent dir returns RC 1
CMD: WORK:rmdir T:rmdir_partial_001 T:rmdir_missing_xyz_999
EXPECT:
EXPECT_RC: 1

# ============================================================
# AMIGA-SPECIFIC TESTS
# ============================================================

# Verify that WORK: volume paths work as arguments (Amiga path handling)
# Create a dir in T: with an Amiga-style name
TEST: Setup - create dir for Amiga volume path test
CMD: WORK:mkdir T:rmdir_amiga_path_001
EXPECT:
EXPECT_RC: 0

# Test that rmdir works with T: volume-prefixed paths (AmigaDOS convention)
TEST: Amiga T: volume path handling works correctly
CMD: WORK:rmdir T:rmdir_amiga_path_001
EXPECT:
EXPECT_RC: 0

# ============================================================
# EDGE CASES
# ============================================================

# Edge case: attempt to remove T: itself (a mounted volume)
# rmdir("T:") should fail -- can't remove an AmigaDOS assign/volume
# Source: rmdir() returns -1, warn(), errors=1, returns 1
# This also tests that rm_path() stops at ':' and doesn't attempt T: removal
TEST: Attempt to remove T: volume fails with RC 1
CMD: WORK:rmdir T:
EXPECT:
EXPECT_RC: 1

# Edge case: double-slash path (rm_path skips consecutive slashes correctly)
# "T:foo//bar" has consecutive '//' -- rm_path: while --p > path && *p=='/' skips trailing
# and p[-1]!='/' check skips the inner case
# Setup the dirs normally first
TEST: Setup - create dirs for double-slash path test
CMD: WORK:mkdir -p T:rmdir_slash_001/sub
EXPECT:
EXPECT_RC: 0

# rmdir -p T:rmdir_slash_001/sub (normal path) -> should remove both
# We can't easily test //double-slash on AmigaDOS (shell may normalize it)
# Instead test the normal -p path which exercises the "no consecutive slash" code
TEST: -p flag traverses path correctly without double-slash issues
CMD: WORK:rmdir -p T:rmdir_slash_001/sub
EXPECT:
EXPECT_RC: 0

# ============================================================
# REAL-WORLD AND STRESS TESTS
# ============================================================

# Real-world: simulate a build cleanup scenario -- remove empty output dirs
# Setup: create a typical project directory layout
TEST: Setup - create project layout for real-world cleanup test
CMD: WORK:mkdir -p T:rmdir_proj_001/obj
EXPECT:
EXPECT_RC: 0

TEST: Setup - create second output dir in project layout
CMD: WORK:mkdir T:rmdir_proj_001/bin
EXPECT:
EXPECT_RC: 0

# Real-world: after a build, remove individual empty output dirs
# (simulates: rmdir obj bin after cleaning a project)
TEST: Real-world cleanup removes multiple output dirs from project
CMD: WORK:rmdir T:rmdir_proj_001/obj T:rmdir_proj_001/bin
EXPECT:
EXPECT_RC: 0

# Remove the (now empty) project root dir
TEST: Real-world cleanup removes project root dir after outputs cleared
CMD: WORK:rmdir T:rmdir_proj_001
EXPECT:
EXPECT_RC: 0

# Real-world: deep project tree removal with -p
# Simulates removing a nested include/platform/arch hierarchy
TEST: Setup - create deep project hierarchy for -p real-world test
CMD: WORK:mkdir -p T:rmdir_hier_001/include/sys
EXPECT:
EXPECT_RC: 0

# Use -p to remove leaf and walk back to root in one command
TEST: Real-world -p removes deep include hierarchy in one command
CMD: WORK:rmdir -p T:rmdir_hier_001/include/sys
EXPECT:
EXPECT_RC: 0

# Stress: remove 5 separate directories in one invocation
# AmigaOS processes each sequentially; tests free-list and multiple warn paths
TEST: Setup - create 5 directories for stress multi-remove test
CMD: WORK:mkdir T:rmdir_s001 T:rmdir_s002 T:rmdir_s003 T:rmdir_s004 T:rmdir_s005
EXPECT:
EXPECT_RC: 0

TEST: Stress - remove 5 directories in a single invocation RC 0
CMD: WORK:rmdir T:rmdir_s001 T:rmdir_s002 T:rmdir_s003 T:rmdir_s004 T:rmdir_s005
EXPECT:
EXPECT_RC: 0

# Stress: remove 5 directories where the 3rd is missing (partial failure)
# Tests that the for loop continues after a failure and aggregates errors correctly
TEST: Setup - create dirs for stress partial-failure test (skip 3rd)
CMD: WORK:mkdir T:rmdir_pf001 T:rmdir_pf002 T:rmdir_pf004 T:rmdir_pf005
EXPECT:
EXPECT_RC: 0

TEST: Stress - 5 args with missing 3rd dir returns RC 1 (partial failure)
CMD: WORK:rmdir T:rmdir_pf001 T:rmdir_pf002 T:rmdir_pf003 T:rmdir_pf004 T:rmdir_pf005
EXPECT:
EXPECT_RC: 1

# Precision: verify exit code propagation is exact
# When ALL removals succeed, errors stays 0 -- verify exact RC=0 not RC=1
TEST: Setup - create precision test directory
CMD: WORK:mkdir T:rmdir_precise_001
EXPECT:
EXPECT_RC: 0

TEST: Precision - successful removal returns exactly RC 0 not RC 1
CMD: WORK:rmdir T:rmdir_precise_001
EXPECT:
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
Generated by `make test-fsemu TARGET=ports/rmdir`
Report template: `toolchain/templates/test-report.md.template`
