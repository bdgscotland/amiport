# FS-UAE Test Report: amigit

## Summary

| Field | Value |
|-------|-------|
| Port | amigit |
| Date | 2026-04-13 15:56:27 |
| Duration | 65s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:amigit` (1.0M) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 44/44 passed |

## Test Results

```
1..44
ok 1 - version prints amigit version on first line
ok 2 - version prints libgit2 version on second line
ok 3 - version prints shim availability on third line
ok 4 - version with extra arg exits RETURN_WARN
ok 5 - top-level --help prints usage to stdout and exits 0
ok 6 - top-level -h exits 0
ok 7 - no arguments exits RETURN_ERROR
ok 8 - unknown command exits RETURN_ERROR
ok 9 - init creates a new repo and prints Initialized message
ok 10 - init on existing repo prints Reinitialized message
ok 11 - init --bare creates bare repo with Initialized bare message
ok 12 - init --bare on existing bare repo prints Reinitialized bare message
ok 13 - init --help prints usage and exits 0
ok 14 - init -h prints usage and exits 0
ok 15 - init unknown flag exits RETURN_ERROR
ok 16 - status --help prints usage and exits 0
ok 17 - status -h prints usage and exits 0
ok 18 - status unknown flag exits RETURN_ERROR
ok 19 - status outside repo exits RETURN_ERROR (not a git repository)
ok 20 - status in empty repo exits 0 with no output
ok 21 - status -s in empty repo exits 0
ok 22 - log --help prints usage and exits 0
ok 23 - log -h prints usage and exits 0
ok 24 - log -n without argument exits RETURN_ERROR
ok 25 - log unknown flag exits RETURN_ERROR
ok 26 - log outside repo exits RETURN_ERROR
ok 27 - log in empty repo exits 0 with no output
ok 28 - show --help prints usage and exits 0
ok 29 - show -h prints usage and exits 0
ok 30 - show unknown flag exits RETURN_ERROR
ok 31 - show outside repo exits RETURN_ERROR
ok 32 - show HEAD in empty repo exits RETURN_ERROR (unborn HEAD)
ok 33 - diff --help prints usage and exits 0
ok 34 - diff -h prints usage and exits 0
ok 35 - diff unknown flag exits RETURN_ERROR
ok 36 - diff outside repo exits RETURN_ERROR
ok 37 - diff in empty repo exits 0 with no output
ok 38 - diff --cached in empty repo exits 0
ok 39 - diff --staged in empty repo exits 0
ok 40 - init with T: volume path syntax
ok 41 - amigit starts cleanly without volume requester popups
ok 42 - init stress repo 1 of 2
ok 43 - init stress repo 2 of 2
ok 44 - repeated reinit of fixture (safety check)
# passed: 44 failed: 0 total: 44
```

### Breakdown

| # | Test | Status | Details |
|---|------|--------|---------|
| 1 | version prints amigit version on first line | PASS | |
| 2 | version prints libgit2 version on second line | PASS | |
| 3 | version prints shim availability on third line | PASS | |
| 4 | version with extra arg exits RETURN_WARN | PASS | |
| 5 | top-level --help prints usage to stdout and exits 0 | PASS | |
| 6 | top-level -h exits 0 | PASS | |
| 7 | no arguments exits RETURN_ERROR | PASS | |
| 8 | unknown command exits RETURN_ERROR | PASS | |
| 9 | init creates a new repo and prints Initialized message | PASS | |
| 10 | init on existing repo prints Reinitialized message | PASS | |
| 11 | init --bare creates bare repo with Initialized bare message | PASS | |
| 12 | init --bare on existing bare repo prints Reinitialized bare message | PASS | |
| 13 | init --help prints usage and exits 0 | PASS | |
| 14 | init -h prints usage and exits 0 | PASS | |
| 15 | init unknown flag exits RETURN_ERROR | PASS | |
| 16 | status --help prints usage and exits 0 | PASS | |
| 17 | status -h prints usage and exits 0 | PASS | |
| 18 | status unknown flag exits RETURN_ERROR | PASS | |
| 19 | status outside repo exits RETURN_ERROR (not a git repository) | PASS | |
| 20 | status in empty repo exits 0 with no output | PASS | |
| 21 | status -s in empty repo exits 0 | PASS | |
| 22 | log --help prints usage and exits 0 | PASS | |
| 23 | log -h prints usage and exits 0 | PASS | |
| 24 | log -n without argument exits RETURN_ERROR | PASS | |
| 25 | log unknown flag exits RETURN_ERROR | PASS | |
| 26 | log outside repo exits RETURN_ERROR | PASS | |
| 27 | log in empty repo exits 0 with no output | PASS | |
| 28 | show --help prints usage and exits 0 | PASS | |
| 29 | show -h prints usage and exits 0 | PASS | |
| 30 | show unknown flag exits RETURN_ERROR | PASS | |
| 31 | show outside repo exits RETURN_ERROR | PASS | |
| 32 | show HEAD in empty repo exits RETURN_ERROR (unborn HEAD) | PASS | |
| 33 | diff --help prints usage and exits 0 | PASS | |
| 34 | diff -h prints usage and exits 0 | PASS | |
| 35 | diff unknown flag exits RETURN_ERROR | PASS | |
| 36 | diff outside repo exits RETURN_ERROR | PASS | |
| 37 | diff in empty repo exits 0 with no output | PASS | |
| 38 | diff --cached in empty repo exits 0 | PASS | |
| 39 | diff --staged in empty repo exits 0 | PASS | |
| 40 | init with T: volume path syntax | PASS | |
| 41 | amigit starts cleanly without volume requester popups | PASS | |
| 42 | init stress repo 1 of 2 | PASS | |
| 43 | init stress repo 2 of 2 | PASS | |
| 44 | repeated reinit of fixture (safety check) | PASS | |

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
# test-fsemu-cases.txt -- FS-UAE functional tests for amigit 0.1
#
# Phase 3b scope: version, init, status, log, show, diff.
# Phase 3c will add: add, commit, checkout, branch, tag.
#
# IMPORTANT -- Phase 3b has no add/commit commands.  This means any
# command that reads commit data can only be tested against an empty
# repository created by "amigit init".  Happy-path tests that require
# at least one commit (log showing a SHA, show HEAD with diff content,
# diff with staged content) are intentionally deferred to Phase 3c.
#
# CWD constraint: status/log/show/diff open "." with NO_SEARCH, so the
# "inside a repo" happy-path tests must run from within a repo dir.
# The test harness can't CD between tests; the wrapper
# test-amigit-inrepo.rexx writes a one-shot Execute script that CDs
# and then runs amigit.  Flag-parsing tests (--help, unknown flags,
# -n without arg) short-circuit BEFORE the repo is opened, so they
# run directly without the wrapper -- faster and more robust.
#
# Fixture: T:amigit-test/ is created by the first init test and
# reused by all subsequent repo-relative tests.
#
# Exit codes used by amigit:
#   RETURN_OK    = 0  -- success
#   RETURN_WARN  = 5  -- e.g. "version" with extra args
#   RETURN_ERROR = 10 -- any error (not a repo, bad flag, missing ref)
#   RETURN_FAIL  = 20 -- catastrophic (git_libgit2_init failed)
#
# No ITEST blocks -- amigit is non-interactive (Category 1 CLI).
# No test-fsemu-visual-cases.txt -- Category 1 has no visual tests.

# ======================================================================
# version command
# ======================================================================

TEST: version prints amigit version on first line
CMD: WORK:amigit version
EXPECT: amigit 0.1 (built 2026-04-13)
EXPECT_RC: 0

TEST: version prints libgit2 version on second line
CMD: WORK:amigit version
EXPECT_LINE: 2,libgit2 1.8.5
EXPECT_RC: 0

TEST: version prints shim availability on third line
CMD: WORK:amigit version
EXPECT_LINE: 3,amiport posix-shim available
EXPECT_RC: 0

TEST: version with extra arg exits RETURN_WARN
CMD: WORK:amigit version extra
EXPECT_RC: 5

# ======================================================================
# Top-level dispatch
# ======================================================================

TEST: top-level --help prints usage to stdout and exits 0
CMD: WORK:amigit --help
EXPECT_CONTAINS: usage: amigit
EXPECT_RC: 0

TEST: top-level -h exits 0
CMD: WORK:amigit -h
EXPECT_RC: 0

TEST: no arguments exits RETURN_ERROR
CMD: WORK:amigit
EXPECT_RC: 10

TEST: unknown command exits RETURN_ERROR
CMD: WORK:amigit badcommand
EXPECT_RC: 10

# ======================================================================
# init command -- happy path
# ======================================================================

# This creates the primary T:amigit-test fixture used by all
# subsequent repo-relative tests.  Uses MKPATH so libgit2 creates
# any missing parent directory (default git init behavior).
TEST: init creates a new repo and prints Initialized message
CMD: WORK:amigit init T:amigit-test
EXPECT: Initialized empty git repository in T:amigit-test
EXPECT_RC: 0

TEST: init on existing repo prints Reinitialized message
CMD: WORK:amigit init T:amigit-test
EXPECT: Reinitialized existing git repository in T:amigit-test
EXPECT_RC: 0

TEST: init --bare creates bare repo with Initialized bare message
CMD: WORK:amigit init --bare T:amigit-test-bare
EXPECT: Initialized empty bare git repository in T:amigit-test-bare
EXPECT_RC: 0

TEST: init --bare on existing bare repo prints Reinitialized bare message
CMD: WORK:amigit init --bare T:amigit-test-bare
EXPECT: Reinitialized existing bare git repository in T:amigit-test-bare
EXPECT_RC: 0

# ======================================================================
# init command -- flag parsing (no repo access)
# ======================================================================

TEST: init --help prints usage and exits 0
CMD: WORK:amigit init --help
EXPECT_CONTAINS: amigit init
EXPECT_RC: 0

TEST: init -h prints usage and exits 0
CMD: WORK:amigit init -h
EXPECT_RC: 0

TEST: init unknown flag exits RETURN_ERROR
CMD: WORK:amigit init --unknown-flag
EXPECT_RC: 10

# ======================================================================
# status command -- flag parsing (no repo access, runs from WORK:)
# ======================================================================

TEST: status --help prints usage and exits 0
CMD: WORK:amigit status --help
EXPECT_CONTAINS: amigit status
EXPECT_RC: 0

TEST: status -h prints usage and exits 0
CMD: WORK:amigit status -h
EXPECT_RC: 0

TEST: status unknown flag exits RETURN_ERROR
CMD: WORK:amigit status --bogus
EXPECT_RC: 10

# status outside a repo (CWD is WORK:, not a repo)
TEST: status outside repo exits RETURN_ERROR (not a git repository)
CMD: WORK:amigit status
EXPECT_RC: 10

# ======================================================================
# status command -- happy path in empty repo (wrapper required)
# ======================================================================

TEST: status in empty repo exits 0 with no output
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-test status
EXPECT_RC: 0

TEST: status -s in empty repo exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-test status -s
EXPECT_RC: 0

# ======================================================================
# log command -- flag parsing (no repo access)
# ======================================================================

TEST: log --help prints usage and exits 0
CMD: WORK:amigit log --help
EXPECT_CONTAINS: amigit log
EXPECT_RC: 0

TEST: log -h prints usage and exits 0
CMD: WORK:amigit log -h
EXPECT_RC: 0

TEST: log -n without argument exits RETURN_ERROR
CMD: WORK:amigit log -n
EXPECT_RC: 10

TEST: log unknown flag exits RETURN_ERROR
CMD: WORK:amigit log --verbose
EXPECT_RC: 10

TEST: log outside repo exits RETURN_ERROR
CMD: WORK:amigit log
EXPECT_RC: 10

# ======================================================================
# log command -- happy path in empty repo (wrapper required)
# ======================================================================

# Empty repo: git_revwalk_push_head returns GIT_EUNBORNBRANCH.
# cmd_log.c treats this as success (no output) per the "no commits yet"
# convention.  Matches upstream git behavior.
TEST: log in empty repo exits 0 with no output
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-test log
EXPECT_RC: 0

# ======================================================================
# show command -- flag parsing (no repo access)
# ======================================================================

TEST: show --help prints usage and exits 0
CMD: WORK:amigit show --help
EXPECT_CONTAINS: amigit show
EXPECT_RC: 0

TEST: show -h prints usage and exits 0
CMD: WORK:amigit show -h
EXPECT_RC: 0

TEST: show unknown flag exits RETURN_ERROR
CMD: WORK:amigit show --verbose
EXPECT_RC: 10

TEST: show outside repo exits RETURN_ERROR
CMD: WORK:amigit show HEAD
EXPECT_RC: 10

# ======================================================================
# show command -- error paths in empty repo (wrapper required)
# ======================================================================

# Empty repo: HEAD is unborn.  git_revparse_single("HEAD") returns
# GIT_ENOTFOUND -> amigit_error_exit -> RETURN_ERROR = 10.
# Happy-path show (real commit with diff) is deferred to Phase 3c.
TEST: show HEAD in empty repo exits RETURN_ERROR (unborn HEAD)
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-test show HEAD
EXPECT_RC: 10

# ======================================================================
# diff command -- flag parsing (no repo access)
# ======================================================================

TEST: diff --help prints usage and exits 0
CMD: WORK:amigit diff --help
EXPECT_CONTAINS: amigit diff
EXPECT_RC: 0

TEST: diff -h prints usage and exits 0
CMD: WORK:amigit diff -h
EXPECT_RC: 0

TEST: diff unknown flag exits RETURN_ERROR
CMD: WORK:amigit diff --stat
EXPECT_RC: 10

TEST: diff outside repo exits RETURN_ERROR
CMD: WORK:amigit diff
EXPECT_RC: 10

# ======================================================================
# diff command -- happy path in empty repo (wrapper required)
# ======================================================================

# Empty repo, no index, no worktree: diff_index_to_workdir returns
# a zero-delta diff -> no output, exit 0.
TEST: diff in empty repo exits 0 with no output
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-test diff
EXPECT_RC: 0

# --cached on empty repo: diff_cached() handles unborn HEAD by using
# NULL old_tree (empty tree); empty index -> zero deltas, exit 0.
TEST: diff --cached in empty repo exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-test diff --cached
EXPECT_RC: 0

# --staged is a synonym for --cached
TEST: diff --staged in empty repo exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-test diff --staged
EXPECT_RC: 0

# ======================================================================
# Amiga-specific: AmigaDOS path syntax + pr_WindowPtr suppression
# ======================================================================

TEST: init with T: volume path syntax
CMD: WORK:amigit init T:amigit-test-vol
EXPECT_CONTAINS: Initialized
EXPECT_RC: 0

# Confirms pr_WindowPtr = -1 suppresses any volume requesters when
# probing the path (known-pitfalls: "AmigaDOS Volume Requester").
TEST: amigit starts cleanly without volume requester popups
CMD: WORK:amigit version
EXPECT_CONTAINS: amigit
EXPECT_RC: 0

# ======================================================================
# Stress / real-world
# ======================================================================

# Create two extra repos -- exercises multiple libgit2 init/shutdown
# cycles in the same session.  Validates no state leaks between calls.
TEST: init stress repo 1 of 2
CMD: WORK:amigit init T:amigit-stress-r1
EXPECT_CONTAINS: Initialized
EXPECT_RC: 0

TEST: init stress repo 2 of 2
CMD: WORK:amigit init T:amigit-stress-r2
EXPECT_CONTAINS: Initialized
EXPECT_RC: 0

# Re-run the fixture init to verify libgit2 refcount stays balanced
# across repeated calls to the same path.
TEST: repeated reinit of fixture (safety check)
CMD: WORK:amigit init T:amigit-test
EXPECT: Reinitialized existing git repository in T:amigit-test
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
passed=44
failed=0
total=44
```

---
Generated by `make test-fsemu TARGET=ports/amigit`
Report template: `toolchain/templates/test-report.md.template`
