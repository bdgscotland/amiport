# FS-UAE Test Report: amigit

## Summary

| Field | Value |
|-------|-------|
| Port | amigit |
| Date | 2026-04-13 20:31:27 |
| Duration | 133s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:amigit` (1.0M) |
| Test method | ARexx harness → TAP output |
| Result | **PASS** — 82/82 passed |

## Test Results

```
1..82
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
ok 45 - init creates T:amigit-c3 fixture for Phase 3c
ok 46 - add --help prints usage and exits 0
ok 47 - add unknown flag exits RETURN_ERROR
ok 48 - add with no path argument exits RETURN_ERROR
ok 49 - add nonexistent file in c3 repo exits RETURN_ERROR
ok 50 - add hello.txt in c3 repo stages the file and exits 0
ok 51 - commit --help prints usage and exits 0
ok 52 - commit unknown flag exits RETURN_ERROR
ok 53 - commit without -m exits RETURN_ERROR (missing message)
ok 54 - commit outside repo exits RETURN_ERROR
ok 55 - commit with nothing staged exits RETURN_ERROR (empty repo, no index)
ok 56 - commit -m after add creates first commit and exits 0
ok 57 - second commit with no new staged changes exits RETURN_ERROR
ok 58 - branch --help prints usage and exits 0
ok 59 - branch unknown flag exits RETURN_ERROR
ok 60 - branch -d without name argument exits RETURN_ERROR
ok 61 - branch outside repo exits RETURN_ERROR
ok 62 - branch foo creates new branch at HEAD and exits 0
ok 63 - branch -l after branch foo shows foo in list
ok 64 - checkout --help prints usage and exits 0
ok 65 - checkout unknown flag exits RETURN_ERROR
ok 66 - checkout nonexistent ref exits RETURN_ERROR
ok 67 - checkout foo switches to foo branch and exits 0
ok 68 - branch -d foo while on foo exits RETURN_ERROR (cannot delete HEAD)
ok 69 - checkout master switches back to master and exits 0
ok 70 - branch -d foo after checkout master exits 0
ok 71 - branch -d foo again exits RETURN_ERROR (branch no longer exists)
ok 72 - tag --help prints usage and exits 0
ok 73 - tag unknown flag exits RETURN_ERROR
ok 74 - tag v0.1 in repo with unborn HEAD exits RETURN_ERROR
ok 75 - tag v0.1 creates lightweight tag at HEAD and exits 0
ok 76 - tag -l after tag v0.1 shows v0.1
ok 77 - tag v0.1 again exits RETURN_ERROR (duplicate tag name)
ok 78 - add second file world.txt stages it and exits 0
ok 79 - diff --cached after staging world.txt shows new file additions
ok 80 - commit second commit with parent exits 0 and shows message
ok 81 - log after two commits shows most recent commit on first line
ok 82 - init from multi-char-volume CWD emits helpful error RC=10
# passed: 82 failed: 0 total: 82
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
| 45 | init creates T:amigit-c3 fixture for Phase 3c | PASS | |
| 46 | add --help prints usage and exits 0 | PASS | |
| 47 | add unknown flag exits RETURN_ERROR | PASS | |
| 48 | add with no path argument exits RETURN_ERROR | PASS | |
| 49 | add nonexistent file in c3 repo exits RETURN_ERROR | PASS | |
| 50 | add hello.txt in c3 repo stages the file and exits 0 | PASS | |
| 51 | commit --help prints usage and exits 0 | PASS | |
| 52 | commit unknown flag exits RETURN_ERROR | PASS | |
| 53 | commit without -m exits RETURN_ERROR (missing message) | PASS | |
| 54 | commit outside repo exits RETURN_ERROR | PASS | |
| 55 | commit with nothing staged exits RETURN_ERROR (empty repo, no index) | PASS | |
| 56 | commit -m after add creates first commit and exits 0 | PASS | |
| 57 | second commit with no new staged changes exits RETURN_ERROR | PASS | |
| 58 | branch --help prints usage and exits 0 | PASS | |
| 59 | branch unknown flag exits RETURN_ERROR | PASS | |
| 60 | branch -d without name argument exits RETURN_ERROR | PASS | |
| 61 | branch outside repo exits RETURN_ERROR | PASS | |
| 62 | branch foo creates new branch at HEAD and exits 0 | PASS | |
| 63 | branch -l after branch foo shows foo in list | PASS | |
| 64 | checkout --help prints usage and exits 0 | PASS | |
| 65 | checkout unknown flag exits RETURN_ERROR | PASS | |
| 66 | checkout nonexistent ref exits RETURN_ERROR | PASS | |
| 67 | checkout foo switches to foo branch and exits 0 | PASS | |
| 68 | branch -d foo while on foo exits RETURN_ERROR (cannot delete HEAD) | PASS | |
| 69 | checkout master switches back to master and exits 0 | PASS | |
| 70 | branch -d foo after checkout master exits 0 | PASS | |
| 71 | branch -d foo again exits RETURN_ERROR (branch no longer exists) | PASS | |
| 72 | tag --help prints usage and exits 0 | PASS | |
| 73 | tag unknown flag exits RETURN_ERROR | PASS | |
| 74 | tag v0.1 in repo with unborn HEAD exits RETURN_ERROR | PASS | |
| 75 | tag v0.1 creates lightweight tag at HEAD and exits 0 | PASS | |
| 76 | tag -l after tag v0.1 shows v0.1 | PASS | |
| 77 | tag v0.1 again exits RETURN_ERROR (duplicate tag name) | PASS | |
| 78 | add second file world.txt stages it and exits 0 | PASS | |
| 79 | diff --cached after staging world.txt shows new file additions | PASS | |
| 80 | commit second commit with parent exits 0 and shows message | PASS | |
| 81 | log after two commits shows most recent commit on first line | PASS | |
| 82 | init from multi-char-volume CWD emits helpful error RC=10 | PASS | |

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

# ======================================================================
# Phase 3c -- add, commit, checkout, branch, tag
# ======================================================================
#
# Fixture: T:amigit-c3/ is created by the first test below and used by
# all subsequent Phase 3c in-repo tests.  It starts empty; after the
# add + commit tests it has one commit on master with hello.txt staged
# and committed.
#
# Wrapper scripts used:
#   test-amigit-inrepo.rexx       -- CD into repo, run amigit, capture RC+stdout
#   test-amigit-inrepo-setup.rexx -- CD into repo, Echo a file, run amigit
#     Usage: rx WORK:test-amigit-inrepo-setup.rexx <repo> <filename> <subcmd> [args]
#     Creates <filename> with content "Hello, Amiga!" then runs amigit.
#
# Test ordering matters for the stateful tests (marked with [ordered]):
#   init c3 -> add hello.txt -> commit "first commit"
#   -> branch foo -> branch -l -> checkout foo
#   -> branch -d foo (error: on HEAD) -> checkout master -> branch -d foo
#   -> tag v0.1 -> tag -l -> tag v0.1 again (error: duplicate)
#
# Exit codes:
#   0  success
#   10 any error (not a repo, bad args, missing file, nothing to commit, etc.)

# ======================================================================
# Phase 3c fixture: create the c3 repo
# ======================================================================

# [ordered-1] This creates the T:amigit-c3 fixture used by all Phase 3c
# in-repo tests.  The path format is consistent with Phase 3b: plain
# AmigaDOS volume syntax.  The inrepo wrappers apply the X:/foo rewrite.
TEST: init creates T:amigit-c3 fixture for Phase 3c
CMD: WORK:amigit init T:amigit-c3
EXPECT: Initialized empty git repository in T:amigit-c3
EXPECT_RC: 0

# ======================================================================
# add command -- flag parsing (no repo access required)
# ======================================================================

TEST: add --help prints usage and exits 0
CMD: WORK:amigit add --help
EXPECT_CONTAINS: amigit add
EXPECT_RC: 0

TEST: add unknown flag exits RETURN_ERROR
CMD: WORK:amigit add --force
EXPECT_RC: 10

TEST: add with no path argument exits RETURN_ERROR
CMD: WORK:amigit add
EXPECT_RC: 10

# ======================================================================
# add command -- error paths in c3 repo (wrapper required)
# ======================================================================

# Attempting to add a file that does not exist in the working tree.
# libgit2 git_index_add_bypath returns GIT_ENOTFOUND -> RC 10.
TEST: add nonexistent file in c3 repo exits RETURN_ERROR
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 add nosuchfile.txt
EXPECT_RC: 10

# ======================================================================
# add command -- happy path [ordered-2]
# ======================================================================

# The setup wrapper creates hello.txt in the working tree before running
# amigit add.  add produces no stdout on success.
TEST: add hello.txt in c3 repo stages the file and exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo-setup.rexx T:amigit-c3 hello.txt add hello.txt
EXPECT_RC: 0

# ======================================================================
# commit command -- flag parsing (no repo access required)
# ======================================================================

TEST: commit --help prints usage and exits 0
CMD: WORK:amigit commit --help
EXPECT_CONTAINS: amigit commit
EXPECT_RC: 0

TEST: commit unknown flag exits RETURN_ERROR
CMD: WORK:amigit commit --amend
EXPECT_RC: 10

TEST: commit without -m exits RETURN_ERROR (missing message)
CMD: WORK:amigit commit
EXPECT_RC: 10

TEST: commit outside repo exits RETURN_ERROR
CMD: WORK:amigit commit -m test
EXPECT_RC: 10

# ======================================================================
# commit command -- error paths in c3 repo (wrapper required)
# ======================================================================

# Fresh c3 repo: hello.txt was staged in the add test above.  Re-using
# the same c3 repo but testing with a different fixture that has nothing
# staged.  Use the Phase 3b empty fixture T:amigit-test for this.
TEST: commit with nothing staged exits RETURN_ERROR (empty repo, no index)
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-test commit -m first
EXPECT_RC: 10

# ======================================================================
# commit command -- happy path [ordered-3]
# ======================================================================

# hello.txt was staged in [ordered-2].  This commit creates the first
# commit on master in T:amigit-c3.  Output is "[<7-sha>] initial".
# The SHA is non-deterministic so we match on the message substring only.
# IMPORTANT: the commit message must be a single word with no spaces --
# AmigaDOS splits arguments on whitespace, so "first commit" would
# become two argv entries and the second would be rejected as unexpected.
TEST: commit -m after add creates first commit and exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 commit -m initial
EXPECT_CONTAINS: initial
EXPECT_RC: 0

# After the first commit, committing again with nothing new staged must
# fail with "nothing to commit".
TEST: second commit with no new staged changes exits RETURN_ERROR
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 commit -m again
EXPECT_RC: 10

# ======================================================================
# branch command -- flag parsing (no repo access required)
# ======================================================================

TEST: branch --help prints usage and exits 0
CMD: WORK:amigit branch --help
EXPECT_CONTAINS: amigit branch
EXPECT_RC: 0

TEST: branch unknown flag exits RETURN_ERROR
CMD: WORK:amigit branch --remote
EXPECT_RC: 10

TEST: branch -d without name argument exits RETURN_ERROR
CMD: WORK:amigit branch -d
EXPECT_RC: 10

TEST: branch outside repo exits RETURN_ERROR
CMD: WORK:amigit branch
EXPECT_RC: 10

# ======================================================================
# branch command -- in c3 repo (wrapper required) [ordered-4]
# ======================================================================

# Create a new branch named foo.  amigit branch <name> produces no stdout.
TEST: branch foo creates new branch at HEAD and exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 branch foo
EXPECT_RC: 0

# After creating foo, -l should list two branches: "* master" and "  foo".
# Branch listing order is not guaranteed -- verify foo appears somewhere.
TEST: branch -l after branch foo shows foo in list
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 branch -l
EXPECT_CONTAINS: foo
EXPECT_RC: 0

# ======================================================================
# checkout command -- flag parsing (no repo access required)
# ======================================================================

TEST: checkout --help prints usage and exits 0
CMD: WORK:amigit checkout --help
EXPECT_CONTAINS: amigit checkout
EXPECT_RC: 0

TEST: checkout unknown flag exits RETURN_ERROR
CMD: WORK:amigit checkout --detach
EXPECT_RC: 10

# ======================================================================
# checkout command -- in c3 repo (wrapper required) [ordered-5]
# ======================================================================

# checkout a ref that does not exist.  git_revparse_single fails with
# GIT_ENOTFOUND -> RC 10.
TEST: checkout nonexistent ref exits RETURN_ERROR
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 checkout doesnotexist
EXPECT_RC: 10

# checkout foo (the branch we created in [ordered-4]).  Output must be
# "Switched to 'foo'" -- this is deterministic.
TEST: checkout foo switches to foo branch and exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 checkout foo
EXPECT: Switched to 'foo'
EXPECT_RC: 0

# ======================================================================
# branch -d -- delete-HEAD error and successful delete [ordered-6]
# ======================================================================

# HEAD is now on foo (from [ordered-5]).  Deleting foo must fail.
TEST: branch -d foo while on foo exits RETURN_ERROR (cannot delete HEAD)
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 branch -d foo
EXPECT_RC: 10

# Switch back to master so foo can be deleted.
TEST: checkout master switches back to master and exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 checkout master
EXPECT: Switched to 'master'
EXPECT_RC: 0

# Now delete foo successfully.  amigit branch -d produces no stdout.
TEST: branch -d foo after checkout master exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 branch -d foo
EXPECT_RC: 0

# Deleting the same branch again should fail (GIT_ENOTFOUND -> RC 10).
TEST: branch -d foo again exits RETURN_ERROR (branch no longer exists)
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 branch -d foo
EXPECT_RC: 10

# ======================================================================
# tag command -- flag parsing (no repo access required)
# ======================================================================

TEST: tag --help prints usage and exits 0
CMD: WORK:amigit tag --help
EXPECT_CONTAINS: amigit tag
EXPECT_RC: 0

TEST: tag unknown flag exits RETURN_ERROR
CMD: WORK:amigit tag --annotate
EXPECT_RC: 10

# ======================================================================
# tag command -- in c3 repo (wrapper required) [ordered-7]
# ======================================================================

# Empty Phase 3b fixture: unborn HEAD.  Creating a tag at HEAD should
# fail (git_revparse_single("HEAD") -> GIT_ENOTFOUND -> RC 10).
TEST: tag v0.1 in repo with unborn HEAD exits RETURN_ERROR
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-test tag v0.1
EXPECT_RC: 10

# Create lightweight tag v0.1 at HEAD.  No stdout on success.
TEST: tag v0.1 creates lightweight tag at HEAD and exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 tag v0.1
EXPECT_RC: 0

# tag -l should now list v0.1.
TEST: tag -l after tag v0.1 shows v0.1
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 tag -l
EXPECT: v0.1
EXPECT_RC: 0

# Creating the same tag again must fail (GIT_EEXISTS -> RC 10).
TEST: tag v0.1 again exits RETURN_ERROR (duplicate tag name)
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 tag v0.1
EXPECT_RC: 10

# ======================================================================
# Stress / real-world: second commit via add + commit workflow
# ======================================================================

# [ordered-8] Create a second file world.txt, stage it via add, then
# commit.  This exercises the "subsequent commit with parent" path in
# cmd_commit.c -- the initial commit path was covered in [ordered-3].
# The setup wrapper creates world.txt in the working tree then runs add.
TEST: add second file world.txt stages it and exits 0
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo-setup.rexx T:amigit-c3 world.txt add world.txt
EXPECT_RC: 0

# [ordered-8b] diff --cached after staging world.txt shows a non-empty
# patch with "+" lines.  This exercises the Phase 3b diff command in a
# real add-then-inspect workflow (Phase 3c integration scenario).
TEST: diff --cached after staging world.txt shows new file additions
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 diff --cached
EXPECT_CONTAINS: +
EXPECT_RC: 0

TEST: commit second commit with parent exits 0 and shows message
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 commit -m worldadd
EXPECT_CONTAINS: worldadd
EXPECT_RC: 0

# log after two commits should show the most recent commit first.
TEST: log after two commits shows most recent commit on first line
CMD: SYS:Rexxc/rx WORK:test-amigit-inrepo.rexx T:amigit-c3 log
EXPECT_CONTAINS: worldadd
EXPECT_RC: 0

# ======================================================================
# Positional argument matrix: init from a CWD (known limitation)
# ======================================================================
# When amigit is invoked as "amigit init" with no positional args from
# inside a working Shell, cmd_init resolves "." to the current dir via
# NameFromLock(pr_CurrentDir), which on AmigaOS returns paths like
# "Ram Disk:foo" or "WORK:playground" -- multi-character volume names.
# libgit2's git_fs_path_root() only recognizes SINGLE-character ASCII
# drive prefixes ("X:"), so it treats multi-char volume paths as
# relative. For read-side commands (status, log, diff, etc.) this is
# fine -- git_repository_open_ext() has a tolerant path handler. But
# for INIT, libgit2's mkdir_canonicalize walks dirname back to "./."
# and fails with "failed to make directory './.'".
#
# Until libgit2 gets a proper AmigaOS path root recognizer, amigit
# detects this case in cmd_init.c and emits a helpful error telling
# the user to use an explicit path. This test verifies the friendly
# error path fires (RC=10) rather than letting libgit2's cryptic
# './.' error through. Users should run "amigit init <path>" from a
# Shell, not "amigit init" with no args.
#
# This is the zero-positional-arg cell of the init positional matrix
# (section 1a of docs/test-coverage-standard.md). Its presence is
# mandatory even though the current behavior is a documented error.

TEST: init from multi-char-volume CWD emits helpful error RC=10
CMD: SYS:Rexxc/rx WORK:test-amigit-cwd-init.rexx
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
passed=82
failed=0
total=82
```

---
Generated by `make test-fsemu TARGET=ports/amigit`
Report template: `toolchain/templates/test-report.md.template`
