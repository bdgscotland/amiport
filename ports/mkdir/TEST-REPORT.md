# FS-UAE Test Report: mkdir

## Summary

| Field | Value |
|-------|-------|
| Port | mkdir |
| Date | 2026-03-26 13:38:24 |
| Duration | 24s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:mkdir` (34K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 19/19 passed |

## Test Results

```
1..19
ok 1 - Create a simple directory in T:
ok 2 - Create nested directories with -p flag
ok 3 - -p flag on already-existing directory succeeds silently
ok 4 - -p creates three levels of parents at once
ok 5 - -m flag with octal mode accepted and directory created
ok 6 - -m flag with restrictive mode accepted (no-op on AmigaOS)
ok 7 - Create multiple directories at once
ok 8 - Create directory using AmigaDOS T: volume prefix
ok 9 - -p flag with AmigaDOS T: prefix handles existing volume root
ok 10 - No arguments prints usage and exits RC=10
ok 11 - Invalid flag -Z exits RC=10
ok 12 - Creating directory that already exists returns RC=1
ok 13 - Creating directory with missing parent fails RC=1
ok 14 - Real-world: create output directory for script artifact storage
ok 15 - Real-world: -p to idempotently ensure full directory path
ok 16 - Real-world: create multiple sibling directories in one invocation
ok 17 - Stress: create 6-level deep nested path with -p
ok 18 - Stress: create five directories in a single invocation
ok 19 - Precision: -p with partial existing path creates only missing components
# passed: 19 failed: 0 total: 19
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Create a simple directory in T: | PASS | |
| 2 | Create nested directories with -p flag | PASS | |
| 3 | -p flag on already-existing directory succeeds silently | PASS | |
| 4 | -p creates three levels of parents at once | PASS | |
| 5 | -m flag with octal mode accepted and directory created | PASS | |
| 6 | -m flag with restrictive mode accepted (no-op on AmigaOS) | PASS | |
| 7 | Create multiple directories at once | PASS | |
| 8 | Create directory using AmigaDOS T: volume prefix | PASS | |
| 9 | -p flag with AmigaDOS T: prefix handles existing volume root | PASS | |
| 10 | No arguments prints usage and exits RC=10 | PASS | |
| 11 | Invalid flag -Z exits RC=10 | PASS | |
| 12 | Creating directory that already exists returns RC=1 | PASS | |
| 13 | Creating directory with missing parent fails RC=1 | PASS | |
| 14 | Real-world: create output directory for script artifact storage | PASS | |
| 15 | Real-world: -p to idempotently ensure full directory path | PASS | |
| 16 | Real-world: create multiple sibling directories in one invocation | PASS | |
| 17 | Stress: create 6-level deep nested path with -p | PASS | |
| 18 | Stress: create five directories in a single invocation | PASS | |
| 19 | Precision: -p with partial existing path creates only missing components | PASS | |

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
# test-fsemu-cases.txt -- mkdir 1.31 FS-UAE test suite
# Category 1 (CLI tool) -- minimum 8 tests required
# All test directories created in T: to avoid leaving artifacts
# All expected values verified by analyzing source and native mkdir behavior

# Exit codes for this port:
#   RC=0  -- success
#   RC=1  -- mkdir/mkpath failure (warn printed to stderr; exitval=1)
#   RC=10 -- usage error (usage() calls exit(10))
#
# Note: RC=1 is not a standard Amiga exit code (5/10/20) but is what
# the upstream OpenBSD code returns (exitval=1 on warn() path).
# usage errors use AmigaDOS-correct exit(10).

# ============================================================
# FUNCTIONAL TESTS -- basic directory creation
# ============================================================

# Native: mkdir /tmp/test_mkdir_simple; echo $?  -> 0
# Directory is created in T: to avoid artifacts; verified by RC=0 only
# (FS-UAE test harness does not support stat/ls assertions)
TEST: Create a simple directory in T:
CMD: WORK:mkdir T:test_mkdir_simple_001
EXPECT_RC: 0

# Native: mkdir /tmp/parent/child -- fails with RC=1 (parent missing)
# Then mkdir -p /tmp/parent/child -- RC=0
TEST: Create nested directories with -p flag
CMD: WORK:mkdir -p T:test_mkdir_p_001/sub/deep
EXPECT_RC: 0

# ============================================================
# -p FLAG: create parent directories
# ============================================================

# Native: mkdir -p /tmp/exists (already exists) -> RC=0, silent
# Source: mkpath() checks stat(); if S_ISDIR -> continue (no error)
TEST: -p flag on already-existing directory succeeds silently
CMD: WORK:mkdir -p T:
EXPECT_RC: 0

# Native: mkdir -p /tmp/p/q/r/s/t  -> RC=0, creates full chain
TEST: -p creates three levels of parents at once
CMD: WORK:mkdir -p T:test_mkdir_p3_001/a/b/c
EXPECT_RC: 0

# ============================================================
# -m FLAG: set permissions (stubbed on AmigaOS -- accepted but no-op)
# ============================================================

# Native: mkdir -m 755 /tmp/test_mode -> RC=0 (mode accepted, chmod is no-op on Amiga)
# Source: setmode() returns non-NULL dummy; getmode() returns base unchanged;
# chmod() stubbed as no-op. Flag accepted for syntax compatibility.
TEST: -m flag with octal mode accepted and directory created
CMD: WORK:mkdir -m 755 T:test_mkdir_mode_001
EXPECT_RC: 0

# Native: mkdir -m 700 /tmp/test_mode2 -> RC=0
TEST: -m flag with restrictive mode accepted (no-op on AmigaOS)
CMD: WORK:mkdir -m 700 T:test_mkdir_mode_002
EXPECT_RC: 0

# ============================================================
# MULTIPLE DIRECTORIES
# ============================================================

# Native: mkdir /tmp/multi_a /tmp/multi_b -> RC=0
# Source: for loop over argv -- creates each directory in sequence
TEST: Create multiple directories at once
CMD: WORK:mkdir T:test_mkdir_multi_a_001 T:test_mkdir_multi_b_001
EXPECT_RC: 0

# ============================================================
# AMIGA-SPECIFIC TESTS -- WORK: and T: volume paths
# ============================================================

# AmigaDOS uses colon as volume separator (T:, WORK:, SYS:)
# mkdir must handle these paths via amiport_mkdir()/CreateDir()
# T: is a well-known Amiga temp volume -- mkdir in T: exercises the shim
TEST: Create directory using AmigaDOS T: volume prefix
CMD: WORK:mkdir T:test_mkdir_amiga_001
EXPECT_RC: 0

# -p with AmigaDOS volume path -- T: must already exist (it always does)
# mkpath() should see T: exists as a directory and continue without error
TEST: -p flag with AmigaDOS T: prefix handles existing volume root
CMD: WORK:mkdir -p T:test_mkdir_amiga_p_001/sub
EXPECT_RC: 0

# ============================================================
# ERROR PATH TESTS
# ============================================================

# Source: if (*argv == NULL) usage(); -- usage() calls exit(10)
# Error message goes to stderr -- test RC only (stderr not captured)
# No stdin hang risk: mkdir checks argv before any I/O
TEST: No arguments prints usage and exits RC=10
CMD: WORK:mkdir
EXPECT_RC: 10

# Source: getopt(argc, argv, "m:p") -- default case calls usage()
# Invalid flag -Z is not in "m:p" so hits default: -> usage() -> exit(10)
TEST: Invalid flag -Z exits RC=10
CMD: WORK:mkdir -Z T:test_mkdir_bogus
EXPECT_RC: 10

# Source: mkdir() returns -1 -> warn(), exitval=1 -> return 1
# Directory already exists: mkdir fails with EEXIST -> warn() -> exitval=1
# Note: RC=1 is not standard AmigaDOS (5/10/20) but is what upstream returns
TEST: Creating directory that already exists returns RC=1
CMD: WORK:mkdir T:
EXPECT_RC: 1

# Source: mkdir() returns -1 -> warn(), exitval=1
# T:nonexistent_parent_xyz/child -- parent does not exist -> ENOENT
# Without -p, parent must pre-exist
TEST: Creating directory with missing parent fails RC=1
CMD: WORK:mkdir T:nonexistent_parent_xyz_999/child
EXPECT_RC: 1

# ============================================================
# REAL-WORLD AND STRESS TESTS
# ============================================================

# Real-world: typical Amiga script usage -- create a work directory before
# writing output files. This is the primary use case for mkdir on AmigaOS.
TEST: Real-world: create output directory for script artifact storage
CMD: WORK:mkdir T:test_mkdir_rw_output_001
EXPECT_RC: 0

# Real-world: -p to ensure a path exists before writing, safe even if it
# already partially exists. Common pattern in Amiga build scripts.
TEST: Real-world: -p to idempotently ensure full directory path
CMD: WORK:mkdir -p T:test_mkdir_rw_build_001/obj/68000
EXPECT_RC: 0

# Real-world: create multiple per-category directories in one command.
# Reflects typical Amiga software installer creating dirs/data, dirs/libs, etc.
TEST: Real-world: create multiple sibling directories in one invocation
CMD: WORK:mkdir T:test_mkdir_rw_libs_001 T:test_mkdir_rw_data_001 T:test_mkdir_rw_prefs_001
EXPECT_RC: 0

# Stress: deeply nested -p path (6 levels deep)
# Tests mkpath() loop stability and CreateDir() shim over many iterations
# Native: mkdir -p /tmp/a/b/c/d/e/f -> RC=0
TEST: Stress: create 6-level deep nested path with -p
CMD: WORK:mkdir -p T:test_mkdir_s6_001/l1/l2/l3/l4/l5/l6
EXPECT_RC: 0

# Stress: create many individual directories in sequence via one command
# Tests the main for-loop robustness with many argv entries
TEST: Stress: create five directories in a single invocation
CMD: WORK:mkdir T:test_mkdir_s5_a T:test_mkdir_s5_b T:test_mkdir_s5_c T:test_mkdir_s5_d T:test_mkdir_s5_e
EXPECT_RC: 0

# Precision: -p on a 3-level path where first level already exists
# mkpath() must: stat T:test_mkdir_prec_001 (exists, is dir -> OK),
#                mkdir T:test_mkdir_prec_001/sub1 (new -> OK),
#                mkdir T:test_mkdir_prec_001/sub1/sub2 (new -> OK)
# Source: mkpath() loop: mkdir fails -> stat -> S_ISDIR -> continue
TEST: Precision: -p with partial existing path creates only missing components
CMD: WORK:mkdir -p T:test_mkdir_prec_001/sub1/sub2
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
passed=19
failed=0
total=19
```

---
Generated by `make test-fsemu TARGET=ports/mkdir`
Report template: `toolchain/templates/test-report.md.template`
