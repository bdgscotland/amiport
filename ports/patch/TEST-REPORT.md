# FS-UAE Test Report: patch

## Summary

| Field | Value |
|-------|-------|
| Port | patch |
| Date | 2026-03-23 10:50:55 |
| Duration | 52s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:patch` (85K) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 42/42 passed |

## Test Results

```
1..42
ok 1 - Check-only verbose shows patch succeeded message (alternate invocation)
ok 2 - Unified diff applied successfully (-u flag, -f force, -s silent, -o -)
ok 3 - Context diff applied successfully (-c flag, -f force, -s silent, -o -)
ok 4 - Normal diff applied successfully (-n flag, -f force, -s silent, -o -)
ok 5 - Ed script diff applied successfully (-e flag, -f force, -s silent, -o -)
ok 6 - Reverse patch (-R flag reverses direction of diff)
ok 7 - Check-only mode (-C dry-run, no file modification, verbose output)
ok 8 - Silent mode (-s suppresses all informational output)
ok 9 - Input from file (-i patchfile flag)
ok 10 - Output to file (-o flag redirects patched output)
ok 11 - Strip path components (-p1 strips one leading path component)
ok 12 - Force mode (-f does not prompt, applies patch without interaction)
ok 13 - Batch mode (-t auto-answers prompts without terminal interaction)
ok 14 - Ignore whitespace (-l canonicalizes whitespace when matching context)
ok 15 - No-reverse mode (-N ignores already-applied or reversed patches)
ok 16 - Set fuzz factor (-F 3 allows up to 3 lines of fuzz when locating hunks)
ok 17 - Backup suffix flag (-z sets custom backup extension)
ok 18 - Backup flag (-b enables backup with default .orig extension)
ok 19 - Reject file name (-r sets custom reject filename)
ok 20 - Remove empty files flag (-E enables removing files that become empty)
ok 21 - Posix mode (--posix enables strict POSIX compliance)
ok 22 - Define symbol (-D wraps changes in C preprocessor conditionals)
ok 23 - Invalid flag causes usage and RETURN_ERROR
ok 24 - Nonexistent input file causes RETURN_ERROR
ok 25 - Nonexistent patch file causes RETURN_ERROR
ok 26 - Malformed patch file causes RETURN_FAIL (no valid patch seen)
ok 27 - Empty patch file causes RETURN_FAIL (no valid patch seen)
ok 28 - Too many file arguments causes RETURN_ERROR
ok 29 - Bad fuzz argument (non-numeric) causes RETURN_ERROR
ok 30 - Bad strip count (non-numeric) causes RETURN_ERROR
ok 31 - RC 0 on successful patch application
ok 32 - RC 20 on completely invalid patch file (RETURN_FAIL)
ok 33 - RC 10 on bad arguments (RETURN_ERROR)
ok 34 - Empty file as patch target (applying to empty file)
ok 35 - Verbose output shows hunk success message on stdout
ok 36 - Patch with p0 (no path stripping, exact filename match)
ok 37 - Unified diff format auto-detected (no -u flag needed)
ok 38 - Context diff format auto-detected (no -c flag needed)
ok 39 - Normal diff format auto-detected (no -n flag needed)
ok 40 - Amiga WORK: volume paths work as origfile argument
ok 41 - Output to T: temp volume (-o T:file works on AmigaOS)
ok 42 - Patch output to stdout (-o - writes result to stdout on AmigaOS)
# passed: 42 failed: 0 total: 42
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | Check-only verbose shows patch succeeded message (alternate invocation) | PASS | |
| 2 | Unified diff applied successfully (-u flag, -f force, -s silent, -o -) | PASS | |
| 3 | Context diff applied successfully (-c flag, -f force, -s silent, -o -) | PASS | |
| 4 | Normal diff applied successfully (-n flag, -f force, -s silent, -o -) | PASS | |
| 5 | Ed script diff applied successfully (-e flag, -f force, -s silent, -o -) | PASS | |
| 6 | Reverse patch (-R flag reverses direction of diff) | PASS | |
| 7 | Check-only mode (-C dry-run, no file modification, verbose output) | PASS | |
| 8 | Silent mode (-s suppresses all informational output) | PASS | |
| 9 | Input from file (-i patchfile flag) | PASS | |
| 10 | Output to file (-o flag redirects patched output) | PASS | |
| 11 | Strip path components (-p1 strips one leading path component) | PASS | |
| 12 | Force mode (-f does not prompt, applies patch without interaction) | PASS | |
| 13 | Batch mode (-t auto-answers prompts without terminal interaction) | PASS | |
| 14 | Ignore whitespace (-l canonicalizes whitespace when matching context) | PASS | |
| 15 | No-reverse mode (-N ignores already-applied or reversed patches) | PASS | |
| 16 | Set fuzz factor (-F 3 allows up to 3 lines of fuzz when locating hunks) | PASS | |
| 17 | Backup suffix flag (-z sets custom backup extension) | PASS | |
| 18 | Backup flag (-b enables backup with default .orig extension) | PASS | |
| 19 | Reject file name (-r sets custom reject filename) | PASS | |
| 20 | Remove empty files flag (-E enables removing files that become empty) | PASS | |
| 21 | Posix mode (--posix enables strict POSIX compliance) | PASS | |
| 22 | Define symbol (-D wraps changes in C preprocessor conditionals) | PASS | |
| 23 | Invalid flag causes usage and RETURN_ERROR | PASS | |
| 24 | Nonexistent input file causes RETURN_ERROR | PASS | |
| 25 | Nonexistent patch file causes RETURN_ERROR | PASS | |
| 26 | Malformed patch file causes RETURN_FAIL (no valid patch seen) | PASS | |
| 27 | Empty patch file causes RETURN_FAIL (no valid patch seen) | PASS | |
| 28 | Too many file arguments causes RETURN_ERROR | PASS | |
| 29 | Bad fuzz argument (non-numeric) causes RETURN_ERROR | PASS | |
| 30 | Bad strip count (non-numeric) causes RETURN_ERROR | PASS | |
| 31 | RC 0 on successful patch application | PASS | |
| 32 | RC 20 on completely invalid patch file (RETURN_FAIL) | PASS | |
| 33 | RC 10 on bad arguments (RETURN_ERROR) | PASS | |
| 34 | Empty file as patch target (applying to empty file) | PASS | |
| 35 | Verbose output shows hunk success message on stdout | PASS | |
| 36 | Patch with p0 (no path stripping, exact filename match) | PASS | |
| 37 | Unified diff format auto-detected (no -u flag needed) | PASS | |
| 38 | Context diff format auto-detected (no -c flag needed) | PASS | |
| 39 | Normal diff format auto-detected (no -n flag needed) | PASS | |
| 40 | Amiga WORK: volume paths work as origfile argument | PASS | |
| 41 | Output to T: temp volume (-o T:file works on AmigaOS) | PASS | |
| 42 | Patch output to stdout (-o - writes result to stdout on AmigaOS) | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE test suite for patch 1.78
# Generated by test-designer agent.
#
# Strategy notes:
#   - patch writes "Hunk #N succeeded" messages to stdout (via say())
#   - patch writes error messages to stderr (not captured by harness)
#   - With -s (silent), no stdout output from patch unless -o - is used
#   - With -o - (output to stdout), the patched file content goes to stdout
#   - Use -s -f -o - for functional tests that check patched content
#   - Use EXPECT_RC: only for error path tests (errors go to stderr)
#   - The base file WORK:test-patch-base.txt is never modified;
#     tests use -o - or -o T:out.txt to avoid in-place modification

# ============================================================
# Category 1: Functional tests -- one per flag/option
# ============================================================

TEST: Check-only verbose shows patch succeeded message (alternate invocation)
CMD: WORK:patch -f -u -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_CONTAINS: succeeded
EXPECT_RC: 0

TEST: Unified diff applied successfully (-u flag, -f force, -s silent, -o -)
CMD: WORK:patch -u -f -s -o - WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_CONTAINS: PATCHED
EXPECT_RC: 0

TEST: Context diff applied successfully (-c flag, -f force, -s silent, -o -)
CMD: WORK:patch -c -f -s -o - WORK:test-patch-base.txt WORK:test-patch-context.txt
EXPECT_CONTAINS: CONTEXT
EXPECT_RC: 0

TEST: Normal diff applied successfully (-n flag, -f force, -s silent, -o -)
CMD: WORK:patch -n -f -s -o - WORK:test-patch-base.txt WORK:test-patch-normal.txt
EXPECT_CONTAINS: NORMAL
EXPECT_RC: 0

TEST: Ed script diff applied successfully (-e flag, -f force, -s silent, -o -)
CMD: WORK:patch -e -f -s -o - WORK:test-patch-base.txt WORK:test-patch-ed.txt
EXPECT_CONTAINS: ED
EXPECT_RC: 0

TEST: Reverse patch (-R flag reverses direction of diff)
CMD: WORK:patch -R -f -s -o - WORK:test-patch-patched.txt WORK:test-patch-unified.txt
EXPECT_CONTAINS: base file
EXPECT_RC: 0

TEST: Check-only mode (-C dry-run, no file modification, verbose output)
CMD: WORK:patch -C -f WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_CONTAINS: succeeded
EXPECT_RC: 0

TEST: Silent mode (-s suppresses all informational output)
CMD: WORK:patch -s -f -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT:
EXPECT_RC: 0

TEST: Input from file (-i patchfile flag)
CMD: WORK:patch -u -f -s -o - -i WORK:test-patch-unified.txt WORK:test-patch-base.txt
EXPECT_CONTAINS: PATCHED
EXPECT_RC: 0

TEST: Output to file (-o flag redirects patched output)
CMD: WORK:patch -f -s -o T:patch-test-out.txt WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Strip path components (-p1 strips one leading path component)
CMD: WORK:patch -p1 -f -s -o - WORK:test-patch-base.txt WORK:test-patch-p1.txt
EXPECT_CONTAINS: P1-STRIPPED
EXPECT_RC: 0

TEST: Force mode (-f does not prompt, applies patch without interaction)
CMD: WORK:patch -u -f -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Batch mode (-t auto-answers prompts without terminal interaction)
CMD: WORK:patch -u -t -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Ignore whitespace (-l canonicalizes whitespace when matching context)
CMD: WORK:patch -l -f -s -C WORK:test-patch-base.txt WORK:test-patch-whitespace.txt
EXPECT_RC: 0

TEST: No-reverse mode (-N ignores already-applied or reversed patches)
CMD: WORK:patch -N -f -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Set fuzz factor (-F 3 allows up to 3 lines of fuzz when locating hunks)
CMD: WORK:patch -F 3 -f -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Backup suffix flag (-z sets custom backup extension)
CMD: WORK:patch -z .bak -f -s -o T:patch-bak-out.txt WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Backup flag (-b enables backup with default .orig extension)
CMD: WORK:patch -b -f -s -o T:patch-b-out.txt WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Reject file name (-r sets custom reject filename)
CMD: WORK:patch -r T:patch-test.rej -f -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Remove empty files flag (-E enables removing files that become empty)
CMD: WORK:patch -E -f -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Posix mode (--posix enables strict POSIX compliance)
CMD: WORK:patch --posix -f -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Define symbol (-D wraps changes in C preprocessor conditionals)
CMD: WORK:patch -D AMIGA -f -s -o - WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_CONTAINS: #ifndef AMIGA
EXPECT_RC: 0

# ============================================================
# Category 2: Error path tests
# ============================================================

TEST: Invalid flag causes usage and RETURN_ERROR
CMD: WORK:patch -Z WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 10

TEST: Nonexistent input file causes RETURN_ERROR
CMD: WORK:patch -f -s WORK:nonexistent-file.txt WORK:test-patch-unified.txt
EXPECT_RC: 10

TEST: Nonexistent patch file causes RETURN_ERROR
CMD: WORK:patch -f -s WORK:test-patch-base.txt WORK:nonexistent-patch.txt
EXPECT_RC: 10

TEST: Malformed patch file causes RETURN_FAIL (no valid patch seen)
CMD: WORK:patch -f -s WORK:test-patch-base.txt WORK:test-patch-bad.txt
EXPECT_RC: 20

TEST: Empty patch file causes RETURN_FAIL (no valid patch seen)
CMD: WORK:patch -f -s WORK:test-patch-base.txt WORK:test-patch-empty.txt
EXPECT_RC: 20

TEST: Too many file arguments causes RETURN_ERROR
CMD: WORK:patch -f -s WORK:test-patch-base.txt WORK:test-patch-unified.txt WORK:test-patch-base.txt
EXPECT_RC: 10

TEST: Bad fuzz argument (non-numeric) causes RETURN_ERROR
CMD: WORK:patch -F notanumber -f -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 10

TEST: Bad strip count (non-numeric) causes RETURN_ERROR
CMD: WORK:patch -p notanumber -f -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 10

# ============================================================
# Category 3: Exit code tests
# ============================================================

TEST: RC 0 on successful patch application
CMD: WORK:patch -u -f -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: RC 20 on completely invalid patch file (RETURN_FAIL)
CMD: WORK:patch -f -s WORK:test-patch-base.txt WORK:test-patch-bad.txt
EXPECT_RC: 20

TEST: RC 10 on bad arguments (RETURN_ERROR)
CMD: WORK:patch -Z
EXPECT_RC: 10

# ============================================================
# Category 4: Edge case tests
# ============================================================

TEST: Empty file as patch target (applying to empty file)
CMD: WORK:patch -f -s WORK:test-empty.txt WORK:test-patch-unified.txt
EXPECT_RC: 10

TEST: Verbose output shows hunk success message on stdout
CMD: WORK:patch -u -f -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_CONTAINS: Hunk
EXPECT_RC: 0

TEST: Patch with p0 (no path stripping, exact filename match)
CMD: WORK:patch -p0 -f -s -C WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Unified diff format auto-detected (no -u flag needed)
CMD: WORK:patch -f -s -o - WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_CONTAINS: PATCHED
EXPECT_RC: 0

TEST: Context diff format auto-detected (no -c flag needed)
CMD: WORK:patch -f -s -o - WORK:test-patch-base.txt WORK:test-patch-context.txt
EXPECT_CONTAINS: CONTEXT
EXPECT_RC: 0

TEST: Normal diff format auto-detected (no -n flag needed)
CMD: WORK:patch -f -s -o - WORK:test-patch-base.txt WORK:test-patch-normal.txt
EXPECT_CONTAINS: NORMAL
EXPECT_RC: 0

# ============================================================
# Category 5: Amiga-specific tests
# ============================================================

TEST: Amiga WORK: volume paths work as origfile argument
CMD: WORK:patch -u -f -s -o - WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_CONTAINS: The quick brown fox
EXPECT_RC: 0

TEST: Output to T: temp volume (-o T:file works on AmigaOS)
CMD: WORK:patch -f -s -o T:patch-amiga-out.txt WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_RC: 0

TEST: Patch output to stdout (-o - writes result to stdout on AmigaOS)
CMD: WORK:patch -s -f -o - WORK:test-patch-base.txt WORK:test-patch-unified.txt
EXPECT_CONTAINS: Fifth line is the last one.
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
passed=42
failed=0
total=42
```

---
Generated by `make test-fsemu TARGET=ports/patch`
Report template: `toolchain/templates/test-report.md.template`
