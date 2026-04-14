# Test-Designer For New Features — MANDATORY Dispatch

Paths: ports/**/test-fsemu-cases.txt, ports/**/test-fsemu-visual-cases.txt, ports/**/*.rexx

**When adding NEW test cases for a NEW feature (flag, option, subcommand,
behavior) in an EXISTING port, dispatch the `test-designer` agent to
audit the tests before committing.** Do not hand-write feature tests
and ship them without test-designer review.

## The Rule

Test-designer is not just a bootstrap tool used once at Stage 5 of
`/port-project`. It is the **deterministic enforcement** against
shallow tests, and must be re-dispatched whenever:

1. A new flag or option is added to an existing command
   (e.g. `commit -F`, `log --grep`, `diff --stat`)
2. A new subcommand is added to an existing multi-command port
   (e.g. amigit gains `stash`)
3. An existing command gains new behavior that changes its input or
   output surface
4. A new code path is exposed by a bug fix that previously blocked the
   path from being reachable

Dispatch `test-designer` in **diff-audit mode**: hand it the source
diff (new function, new argv branch) AND the proposed new test
blocks. Ask it to verify the tests exercise the feature's actual
PURPOSE, not just "does the binary exit without crashing".

## The happy-path lie

A "happy path" test is a lie when its input does not exercise the
REASON the feature exists. It proves the code doesn't crash, but so
would any random input. It does not prove the feature works.

Canonical example, amigit 0.1-5 (2026-04-14):

- **Feature:** `amigit commit -F <file>` — read commit message from
  a file instead of argv. Exists SPECIFICALLY because AmigaDOS
  splits `-m "hello world"` on whitespace and delivers `"hello"` +
  `"world"` as two argv entries, so multi-word messages via `-m`
  are impossible.
- **Shallow happy-path test (the lie):**
  ```
  TEST: commit -F from file
  CMD: ... commit-f.rexx T:amigit-c3 readme.txt fromfile
  EXPECT_CONTAINS: fromfile
  EXPECT_RC: 0
  ```
  The message is `"fromfile"` — a single word. This test proves
  nothing `-m fromfile` wouldn't already prove. It does NOT
  exercise the reason `-F` exists.
- **Real happy-path test:**
  ```
  TEST: commit -F delivers multi-word message (the whole point of -F)
  CMD: ... commit-f.rexx T:amigit-c3 readme.txt fix the broken parser in cmd_commit
  EXPECT_CONTAINS: fix the broken parser in cmd_commit
  EXPECT_RC: 0

  TEST: log after -F commit shows full multi-word message (-F roundtrip)
  CMD: ... log -n 1
  EXPECT_CONTAINS: fix the broken parser in cmd_commit
  EXPECT_RC: 0
  ```
  Multi-word input + roundtrip verification through git's storage
  proves the feature actually delivers the value it exists to
  deliver. A single-word message would satisfy the EXPECT but not
  the intent.

## Shallow-happy-path smell — how to detect

Before committing new feature tests, ask (or have test-designer ask):

1. **What problem does this feature solve that an existing feature
   already doesn't solve?** If the new feature is `-F` for files, the
   problem is multi-word messages via argv. The test must use a
   multi-word message.
2. **Can the test's happy-path input be delivered by the pre-feature
   tooling?** If yes, the test is shallow. If `commit -m fromfile`
   would produce the same result as `commit -F <file with "fromfile">`,
   the `-F` test is a lie.
3. **Is there a roundtrip?** For stateful features (git commits,
   database writes, config updates) a one-shot "command succeeded"
   test is insufficient. A subsequent read (e.g. `log` after `commit`,
   `cat` after `write`) must surface the exact value just written.
4. **Are error paths tested against distinct failure modes?** Not
   just "returns non-zero" — each failure branch in the source must
   have a test (missing arg, bad arg, resource not found, resource
   wrong type, etc.). This is the existing `feedback_no_happy_path_only`
   rule; the new rule above stacks on top of it.

## Relationship to existing rules

- `feedback_no_happy_path_only.md` (memory) — covers "don't only test
  the happy path, cover errors and edge cases too". That rule
  addresses breadth of coverage.
- **This rule** — covers "the happy path itself must exercise the
  feature's purpose". Addresses depth of the happy path specifically.
- `never-weaken-tests.md` — covers "don't change a failing test to
  pass by broadening the assertion". Addresses resisting the
  temptation to water down tests when they fail.

All three are complementary. A test suite can obey the other two
rules and still fail this one (every test passes, every error path
covered, none of the assertions weakened — but the happy-path input
is "fromfile" for a multi-word-message feature).

## Cross-cutting concern for stateful ports

For ports that manage non-trivial persistent state (git, databases,
config managers, VCS tools, shells), test-designer must go beyond
"one CMD line, one EXPECT_RC" and propose:

- **State-transition assertions** — before N state; run command;
  after must be N+1 state, verified via a follow-up read command
- **Roundtrip assertions** — written value is readable and matches
  exactly (not just "contains substring")
- **Cross-command scenarios** — sequences of 3+ commands with state
  assertions between each, matching how a real user exercises the
  tool
- **Fixture-based setup** — known-starting-state repositories /
  databases / configs, not just "empty T:scratch"

The amigit port is the current reference case for "git is a complex
beast, test it differently". As amigit evolves, its test suite will
grow new assertion primitives (`ASSERT_HEAD_SHA`, `ASSERT_BRANCH_EXISTS`,
`ASSERT_LOG_EXACT`) — test-designer should inherit these as the
recommended pattern for future git-adjacent ports.
