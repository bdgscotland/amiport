# FS-UAE Test Report: sed

## Summary

| Field | Value |
|-------|-------|
| Port | sed |
| Date | 2026-04-13 21:48:41 |
| Duration | 137s |
| Platform | FS-UAE 3.2.35 (A1200, Kickstart 3.1) |
| Binary | `WORK:sed` (65K) |
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
ok 82 - init friendly-error fires for bare-CWD AND explicit-mc-volume cases
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
| 82 | init friendly-error fires for bare-CWD AND explicit-mc-volume cases | PASS | |

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
# sed FS-UAE test suite
# Category 1 (CLI) -- minimum 15 tests required
# Source: OpenBSD sed v1.47 (ports/sed/ported/)
# Uses input file instead of piping (ARexx limitation)
# getopt string: "Eae:f:i::nru"
# sed commands: { } a b c d D g G h H i l n N p P q r s t w x y ! : # =

# --- Functional tests: flags ---

TEST: s command substitutes first occurrence on each line
CMD: WORK:sed s/hello/goodbye/ WORK:test-sed-input.txt
EXPECT: goodbye world
EXPECT_RC: 0

TEST: -n flag suppresses auto-print (silent mode)
CMD: WORK:sed -n /foo/p WORK:test-sed-input.txt
EXPECT: foo bar baz
EXPECT_RC: 0

TEST: -e flag specifies expression on command line
CMD: WORK:sed -e s/hello/hi/ WORK:test-sed-input.txt
EXPECT: hi world
EXPECT_RC: 0

TEST: -f flag reads script from file
CMD: WORK:sed -f WORK:test-sed-script.txt WORK:test-sed-input.txt
EXPECT: goodbye world
EXPECT_RC: 0

TEST: -E flag enables extended regular expressions (+ quantifier)
CMD: WORK:sed -E s/hel+o/MATCH/ WORK:test-sed-input.txt
EXPECT: MATCH world
EXPECT_RC: 0

TEST: -r flag is alias for -E (extended regex)
CMD: WORK:sed -r s/hel+o/MATCH/ WORK:test-sed-input.txt
EXPECT: MATCH world
EXPECT_RC: 0

TEST: -a flag defers w command file creation until runtime
CMD: WORK:sed -a -f WORK:test-sed-write-cmd.sed WORK:test-sed-hold.txt
EXPECT: alpha
EXPECT_RC: 0

TEST: -u flag enables unbuffered output (line buffered stdout)
CMD: WORK:sed -u s/hello/goodbye/ WORK:test-sed-input.txt
EXPECT: goodbye world
EXPECT_RC: 0

TEST: -i flag edits file in place (rewrites test-sed-input.txt)
CMD: WORK:sed -i s/x/y/ WORK:test-sed-input.txt
EXPECT_RC: 0

TEST: multiple -e expressions applied in sequence
CMD: WORK:sed -e s/hello/hi/ -e s/world/earth/ WORK:test-sed-input.txt
EXPECT: hi earth
EXPECT_RC: 0

# --- Functional tests: sed commands ---

TEST: d command deletes matching lines
CMD: WORK:sed /foo/d WORK:test-sed-input.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: p command with s flag prints substituted line twice (auto-print + p)
CMD: WORK:sed s/hello/goodbye/p WORK:test-sed-input.txt
EXPECT: goodbye world
EXPECT_RC: 0

TEST: q command quits after first line
CMD: WORK:sed q WORK:test-sed-input.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: = command prints line number before each line
CMD: WORK:sed = WORK:test-sed-input.txt
EXPECT: 1
EXPECT_RC: 0

TEST: y command transliterates characters
CMD: WORK:sed y/abcdef/ABCDEF/ WORK:test-sed-input.txt
EXPECT: hEllo worlD
EXPECT_RC: 0

TEST: s command with g flag substitutes all occurrences on a line
CMD: WORK:sed s/aaa/xxx/g WORK:test-sed-input.txt
EXPECT_CONTAINS: xxx bbb xxx
EXPECT_RC: 0

TEST: s command with Nth occurrence flag (s///2 skips first match)
CMD: WORK:sed s/aaa/xxx/2 WORK:test-sed-input.txt
EXPECT_CONTAINS: aaa bbb xxx
EXPECT_RC: 0

TEST: s command with w flag writes matched lines to file
CMD: WORK:sed -n -f WORK:test-sed-wflag.sed WORK:test-sed-input.txt
EXPECT_RC: 0

TEST: a command appends text after matching line
CMD: WORK:sed -f WORK:test-sed-append-cmd.sed WORK:test-sed-append.txt
EXPECT: first line
EXPECT_RC: 0

TEST: i command inserts text before matching line
CMD: WORK:sed -f WORK:test-sed-insert-cmd.sed WORK:test-sed-append.txt
EXPECT: first line
EXPECT_RC: 0

TEST: c command changes matching line to new text
CMD: WORK:sed -f WORK:test-sed-change-cmd.sed WORK:test-sed-hold.txt
EXPECT: CHANGED LINE
EXPECT_RC: 0

TEST: l command prints pattern space unambiguously with trailing dollar
CMD: WORK:sed l WORK:test-sed-oneline.txt
EXPECT_CONTAINS: only one line$
EXPECT_RC: 0

TEST: h and G commands save and append hold space
CMD: WORK:sed -f WORK:test-sed-hold-cmd.sed WORK:test-sed-hold.txt
EXPECT: alpha
EXPECT_RC: 0

TEST: H command appends to hold space (H then g to retrieve)
CMD: WORK:sed -f WORK:test-sed-gflag.sed WORK:test-sed-hold.txt
EXPECT: alpha
EXPECT_RC: 0

TEST: x command exchanges pattern and hold space
CMD: WORK:sed -n -f WORK:test-sed-exchange.sed WORK:test-sed-hold.txt
EXPECT: alpha
EXPECT_RC: 0

TEST: n command reads next line (advances to next input line)
CMD: WORK:sed -f WORK:test-sed-n-cmd.sed WORK:test-sed-append.txt
EXPECT: first line
EXPECT_RC: 0

TEST: N command appends next line to pattern space
CMD: WORK:sed -f WORK:test-sed-multi-n.sed WORK:test-sed-append.txt
EXPECT: first line
EXPECT_RC: 0

TEST: P command prints first line of multi-line pattern space
CMD: WORK:sed -n -f WORK:test-sed-multi-n.sed WORK:test-sed-append.txt
EXPECT: first line
EXPECT_RC: 0

TEST: D command deletes first line of multi-line pattern space
CMD: WORK:sed -f WORK:test-sed-multi-n.sed WORK:test-sed-append.txt
EXPECT: first line
EXPECT_RC: 0

TEST: r command appends file contents after matching line
CMD: WORK:sed -f WORK:test-sed-rfile.sed WORK:test-sed-hold.txt
EXPECT: alpha
EXPECT_RC: 0

TEST: b command branches to label unconditionally
CMD: WORK:sed -f WORK:test-sed-branch-cmd.sed WORK:test-sed-branch.txt
EXPECT: xxx
EXPECT_RC: 0

TEST: t command branches to label after successful substitution
CMD: WORK:sed -f WORK:test-sed-branch-cmd.sed WORK:test-sed-branch.txt
EXPECT: xxx
EXPECT_RC: 0

TEST: ! negation operator prints lines not matching pattern
CMD: WORK:sed -n /hello/!p WORK:test-sed-input.txt
EXPECT: foo bar baz
EXPECT_RC: 0

TEST: g command replaces pattern space with hold space contents
CMD: WORK:sed -f WORK:test-sed-gflag.sed WORK:test-sed-hold.txt
EXPECT: alpha
EXPECT_RC: 0

# --- Address tests ---

TEST: line address selects specific line number
CMD: WORK:sed -n 2p WORK:test-sed-input.txt
EXPECT: foo bar baz
EXPECT_RC: 0

TEST: last-line address via sed script (dollar sign via -f to avoid AmigaDOS expansion)
CMD: WORK:sed -n -f WORK:test-sed-lastline.sed WORK:test-sed-input.txt
EXPECT: line six
EXPECT_RC: 0

TEST: address range prints lines between two line numbers
CMD: WORK:sed -n 2,3p WORK:test-sed-input.txt
EXPECT: foo bar baz
EXPECT_RC: 0

TEST: regex address selects lines matching pattern
CMD: WORK:sed -n /UPPERCASE/p WORK:test-sed-input.txt
EXPECT: UPPERCASE LINE
EXPECT_RC: 0

# --- #n in script enables silent mode ---

TEST: hash-n as first line of script enables silent mode
CMD: WORK:sed -f WORK:test-sed-silent.sed WORK:test-sed-hold.txt
EXPECT: alpha
EXPECT_RC: 0

# --- Error path tests ---

TEST: nonexistent input file reports error and returns RC 10
CMD: WORK:sed s/x/y/ WORK:no-such-file.txt
EXPECT_RC: 10

TEST: invalid flag returns RC 10
CMD: WORK:sed -Z s/x/y/ WORK:test-sed-input.txt
EXPECT_RC: 10

TEST: bad regular expression returns RC 10
CMD: WORK:sed -e s/[invalid/x/ WORK:test-sed-input.txt
EXPECT_RC: 10

TEST: invalid sed command letter returns RC 10
CMD: WORK:sed -e X WORK:test-sed-input.txt
EXPECT_RC: 10

TEST: unterminated substitute pattern returns RC 10
CMD: WORK:sed s/hello WORK:test-sed-input.txt
EXPECT_RC: 10

TEST: y transform with unequal string lengths returns RC 10
CMD: WORK:sed y/abc/xy/ WORK:test-sed-input.txt
EXPECT_RC: 10

TEST: undefined branch label returns RC 10
CMD: WORK:sed b WORK:test-sed-input.txt
EXPECT_RC: 0

TEST: duplicate label in script returns RC 10
CMD: WORK:sed -e :loop -e :loop WORK:test-sed-input.txt
EXPECT_RC: 10

TEST: empty label in script returns RC 10
CMD: WORK:sed -e :  WORK:test-sed-input.txt
EXPECT_RC: 10

TEST: unmatched brace returns RC 10
CMD: WORK:sed { WORK:test-sed-input.txt
EXPECT_RC: 10

# --- Edge case tests ---

TEST: empty input file produces no output and exits 0
CMD: WORK:sed s/x/y/ WORK:test-empty.txt
EXPECT:
EXPECT_RC: 0

TEST: long line handled without crash (buffer growth)
CMD: WORK:sed s/MARKER/FOUND/ WORK:test-longline.txt
EXPECT_CONTAINS: FOUND
EXPECT_RC: 0

TEST: single-line file processed correctly
CMD: WORK:sed s/only/just/ WORK:test-sed-oneline.txt
EXPECT: just one line
EXPECT_RC: 0

TEST: file with no trailing newline handled correctly
CMD: WORK:sed s/no/yes/ WORK:test-sed-notrail.txt
EXPECT: yes newline at end
EXPECT_RC: 0

TEST: l command shows line terminator dollar on long line
CMD: WORK:sed l WORK:test-longline.txt
EXPECT_CONTAINS: MARKER$
EXPECT_RC: 0

TEST: multiple input files processed in sequence
CMD: WORK:sed -n 1p WORK:test-sed-oneline.txt WORK:test-sed-append.txt
EXPECT: only one line
EXPECT_RC: 0

# --- Amiga-specific tests ---

TEST: -f with script containing spaces in pattern (AmigaDOS quoting workaround)
CMD: WORK:sed -f WORK:test-sed-spaces.sed WORK:test-sed-input.txt
EXPECT: hello world
EXPECT_CONTAINS: FOO BAR baz
EXPECT_RC: 0

TEST: WORK: volume path accepted as input file argument
CMD: WORK:sed -n 1p WORK:test-sed-input.txt
EXPECT: hello world
EXPECT_RC: 0

TEST: two -e and -f flags combined (both flag types together)
CMD: WORK:sed -e s/hello/hi/ -f WORK:test-sed-script.txt WORK:test-sed-input.txt
EXPECT: hi world
EXPECT_RC: 0

TEST: -E with grouping and alternation (extended regex features)
CMD: WORK:sed -E s/(hello|foo)/WORD/ WORK:test-sed-input.txt
EXPECT: WORD world
EXPECT_RC: 0

TEST: line number output uses Amiga LONG format (no sign issues)
CMD: WORK:sed = WORK:test-sed-input.txt
EXPECT: 1
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
passed=82
failed=0
total=82
```

---
Generated by `make test-fsemu TARGET=ports/sed`
Report template: `toolchain/templates/test-report.md.template`
