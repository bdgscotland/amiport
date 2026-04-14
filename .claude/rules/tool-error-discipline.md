# Tool Error Discipline — Never Move On Without Fixing

**When any tool call returns an error, your VERY NEXT action must be
to resolve the error before doing anything else.** Do not acknowledge
the error in prose and continue with other work. Do not assume a
retry will succeed later. Do not hope the error was spurious.

## The Rule

Tool errors come in several shapes, and each has a required next
action:

| Error shape | Required next action |
|-------------|---------------------|
| `Edit` / `Write` returns "File has not been read yet" | `Read` the file, then retry the same `Edit` immediately. Do NOT move to other edits first. |
| `Edit` returns "old_string not found" | `Read` the file, locate the actual text, retry with corrected `old_string`. |
| `Bash` returns non-zero exit code | Inspect the stderr, diagnose the root cause, re-run or fix. Never paper over with a different command. |
| `Write` refuses because file already exists | `Read` the file first, then decide: is your intent to overwrite (use `Write`) or patch (use `Edit`)? |
| Any agent dispatch returns an error or ambiguous result | Re-dispatch with clarified prompt, or drop to direct tools. Never assume silent success. |

## The failure mode this rule prevents

A session drifts when you acknowledge an error in your response text
("edit failed, moving on") and then keep going with other tool
calls. The skipped edit is silently lost. Later actions build on
the assumption that the skipped edit was applied, and the
mismatch only surfaces during verification — potentially 30-90
minutes later, after multiple rebuilds, test runs, and agent
dispatches that all operated on the wrong state.

## Canonical incident

2026-04-14, amigit 0.1-5 ship:

1. Dispatched session began bumping version 0.1-4 → 0.1-5 across
   `ports/amigit/Makefile` (edit succeeded), `ports/amigit/ported/amigit.h`
   (edit returned "File has not been read yet" error), and
   `ports/amigit/ported/amigit.c` (same error).
2. I noted the errors in prose but did not retry them. Moved on to
   edit `cmd_version.c` (succeeded — I had read that file earlier).
3. Dispatched build-manager to rebuild. Build-manager reported
   clean build at 1,081,432 bytes. I accepted the report as
   confirmation of the version bump.
4. Dispatched FS-UAE verification. Came back 86/87 — one test
   failing. Spent several minutes chasing the failure.
5. Root cause: `strings ports/amigit/amigit | grep 0.1` showed the
   binary still embedded `0.1-4` in both AMIGIT_VERSION and the
   $VER tag. The two silent edit errors from step 1 meant the
   source files still said 0.1-4, the "successful" build compiled
   stale source, and the FS-UAE version test (which expected
   0.1-5) failed exactly as it should have.
6. Wasted wall-clock: ~90 minutes across multiple rebuilds, FS-UAE
   runs, and agent dispatches. Real fix: re-read amigit.h, re-edit,
   re-read amigit.c, re-edit, rebuild. Takes 2 minutes.

## How to apply

**Before any new tool call after an error**, ask: "Did the last
tool call do what I asked, or did it report an error I didn't
resolve?" If the answer is the latter, the next tool call must
be the fix — nothing else.

**Trust but verify** (from CLAUDE.md) applies doubly to
tool-error-recovery: your own prose about what "happened" is not
evidence that it happened. Check the file, the binary, the
working tree — the authoritative source of truth is the artifact,
not the conversation summary.

## Cross-reference

- `.claude/rules/never-weaken-tests.md` — resist the temptation
  to broaden a failing test assertion; similar discipline
- `CLAUDE.md` "Trust but verify" section — the umbrella principle
- Build-manager agent's version-bump verification section —
  added concurrently with this rule (2026-04-14) so that version
  bumps specifically always trigger `strings | grep` verification
  before reporting success
