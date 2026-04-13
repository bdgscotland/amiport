# amigit Phase 3 Session Handoff

**For:** next session resuming PDR-010 Phase 3c after `/clear`.
**As of:** 2026-04-13, commit `d2ca06f` on `origin/main`.

Read this file first in a fresh session, then the files it points to.
Do not re-derive the Phase 2 or Phase 3a/b story -- both are shipped.

## TL;DR state

- **Phase 2 (lib/libgit2):** COMPLETE. 79/79 tests green on vamos via
  `-DAMIPORT_VAMOS_LIMITED`. 1.34 MB `libgit2.a` with 9-file `-O1`
  hotpath promotion. Memory audit CLEAN with one HIGH leak documented.
- **Phase 3a (amigit version + init):** COMPLETE at commit `e4d0466`.
- **Phase 3b (amigit status/log/show/diff):** COMPLETE at commit
  `d2ca06f`. 44/44 FS-UAE tests green. Two libgit2 AmigaOS path bugs
  worked around -- `X:foo` -> `X:/foo` rewrite in cmd_init + helper,
  and `amiport_realpath("."` fix in `lib/posix-shim/src/file_io.c`.
  Binary is 1,071,256 bytes.
- **Phase 3c (this session):** add `add`, `commit`, `checkout`,
  `branch`, `tag`. Completes v1 CLI surface. Produces PORT.md final
  writeup, `amigit.readme`, LHA package, PORTS.md + README.md
  entries, catalog + site updates. Mandatory memory-checker and
  perf-optimizer runs.

## CRITICAL: local vamos monkey-patches (NOT in git)

Without these, `tests/libgit2/test_libgit2` and any filesystem-heavy
amigit test crash on vamos. Re-apply after `pip upgrade amitools` or
a fresh clone-install.

**File:** `~/.pyenv/versions/3.14.4/lib/python3.14/site-packages/amitools/vamos/lib/DosLibrary.py`

**Patch 1 -- raise DOS lock table from 1024 to 65536:**
Find line ~99:
```python
self.lock_mgr = LockManager(ctx.path_mgr, self.dos_list, ctx.alloc, ctx.mem)
```
Change to:
```python
self.lock_mgr = LockManager(ctx.path_mgr, self.dos_list, ctx.alloc, ctx.mem, max_locks=65536)
```

**Patch 2 -- SetFileDate must not raise FileNotFoundError:**
Find the `SetFileDate` method (~line 539-558). Wrap the `os.utime`
call in `try/except (FileNotFoundError, OSError)` that sets
`ERROR_OBJECT_NOT_FOUND` and returns DOSFALSE. See Phase 3a/b
handoff history in git for the exact diff.

**After both patches:**
```bash
rm ~/.pyenv/versions/3.14.4/lib/python3.14/site-packages/amitools/vamos/lib/__pycache__/DosLibrary.cpython-314.pyc
```

Verify with `make -C tests/libgit2 run` -- should print 79/79 in ~2s.

## Files to read first in a fresh session

1. `docs/pdr/010a-amigit-cli-spec.md` -- authoritative v1 CLI spec.
2. `ports/amigit/PORT.md` -- current state. "Path handling" section
   documents the X:foo -> X:/foo rewrite + realpath fix in detail.
3. `ports/amigit/ported/amigit.h` -- `amigit_resolve_repo_path()`
   prototype + `amigit_cmd_fn` signature. New commands mirror this.
4. `ports/amigit/ported/amigit.c` -- `main()` + dispatcher +
   `amigit_resolve_repo_path()` helper. **Do not duplicate these in
   new command files.**
5. `ports/amigit/ported/cmd_init.c` -- canonical template for any
   command that takes a user-supplied path. Shows the
   `amigit_resolve_repo_path` + probe + init pattern.
6. `ports/amigit/ported/cmd_status.c` / `cmd_log.c` / `cmd_show.c` /
   `cmd_diff.c` -- canonical template for commands that open the
   CWD repo. Shows the `amigit_resolve_repo_path(".", ...)` +
   `git_repository_open_ext` pattern.
7. `ports/amigit/Makefile` -- per-file compilation. Adding a new
   command = one line in `OBJECTS`.
8. `tests/libgit2/test_libgit2.c` -- richest reference for how to
   call every libgit2 API that Phase 3c needs (index add, commit
   create, checkout tree, branch create, tag create). Sections 7
   (tree/index), 8 (commit), 9 (references), 10 (branches/tags).
9. `lib/libgit2/include/git2/*.h` -- authoritative function
   signatures.
10. `.claude/rules/known-pitfalls.md` -- recent additions cover
    Phase 3b discoveries. Search for "libgit2", "X:foo", "realpath".
11. `ports/amigit/test-fsemu-cases.txt` -- 44-test Phase 3b suite.
    Phase 3c adds ~20-30 more tests for write-side commands.
12. `ports/amigit/test-amigit-inrepo.rexx` -- ARexx CWD wrapper for
    repo-relative tests. Applies the X:foo -> X:/foo rewrite.

## Phase 3c task list

Execute in order. Each command is an incremental commit.

### 1. Spec review + alignment (5 min)

Read `docs/pdr/010a-amigit-cli-spec.md` for `add`, `commit`,
`checkout`, `branch`, `tag` sections. Confirm no spec drift. If the
user wants changes, amend the spec first.

### 2. `cmd_add.c`

- `amigit add <path>...`
- `git_repository_index` + `git_index_add_bypath` for each path.
- `git_index_write` at the end.
- Repo opened via `amigit_resolve_repo_path(".", ...)` pattern.
- `--help` / `-h` prints usage, exits 0.

Reference: `tests/libgit2/test_libgit2.c` Section 7 (tree/index).

### 3. `cmd_commit.c`

- `amigit commit -m <msg>`
- `git_repository_index` + `git_index_write_tree` (if no staged
  changes, exit 10 with "nothing to commit").
- `git_signature_now` for author and committer. Source from
  environment variables `GIT_AUTHOR_NAME` / `GIT_AUTHOR_EMAIL` if
  set, else a sane default ("amigit user" / "amigit@localhost").
- `git_commit_create_v` with the staged tree.
- First commit has zero parents; subsequent commits have HEAD
  parent via `git_commit_lookup(&parent, repo, git_reference_target(head_ref))`.

Reference: `tests/libgit2/test_libgit2.c` `commit_create_initial`,
`commit_create_with_parent`.

### 4. `cmd_checkout.c`

- `amigit checkout <ref>`
- `git_revparse_single` -> resolve ref to object -> commit.
- `git_checkout_tree` with `GIT_CHECKOUT_SAFE`.
- `git_repository_set_head` to point HEAD at the new ref (handle
  both branch refs and detached-HEAD by OID).

Reference: `tests/libgit2/test_libgit2.c` branch/reference tests.

### 5. `cmd_branch.c`

- `amigit branch` (list) -- `git_branch_iterator_new` +
  `git_branch_next`, print each local branch name.
- `amigit branch <name>` (create) -- `git_branch_create` from HEAD.
- `amigit branch -d <name>` (delete) -- `git_branch_lookup` +
  `git_branch_delete`. Refuse to delete the current branch.
- `amigit branch -l` (list explicit, same as no args).

Reference: `tests/libgit2/test_libgit2.c` Section 10.

### 6. `cmd_tag.c`

- `amigit tag` (list) -- `git_tag_list` or iteration pattern.
- `amigit tag <name>` (create lightweight tag) -- `git_tag_create_lightweight`
  pointing at HEAD's commit.
- `amigit tag -l` (list, same as no args).

Reference: `tests/libgit2/test_libgit2.c` Section 10 (tag_create_lightweight).

### 7. Test harness

Add ~20-30 TEST blocks to `ports/amigit/test-fsemu-cases.txt`:
- `add` happy path (add a file, verify status shows it staged).
- `commit -m` happy path (commit, verify log shows the SHA).
- `commit` with nothing staged -> RC 10.
- `log` after real commits -> SHA visible in output.
- `show HEAD` after commit -> header + diff content.
- `diff` before and after staging.
- `branch` list after commit -> shows master/main.
- `branch foo` create + list shows foo.
- `branch -d foo` delete.
- `checkout foo` + `log` shows same history.
- `tag v0.1` + `tag -l` shows v0.1.
- Error paths for every command.

Dispatch `test-designer` agent for the design, but do it in port
mode (it'll produce the test-fsemu-cases.txt structure). The test
harness wrapper pattern is already in place.

### 8. Run FS-UAE tests

```
make test-fsemu TARGET=ports/amigit
```

Gate on all tests passing. Phase 3b used `EXPECT_RC` only for most
wrapper tests to avoid flakiness; Phase 3c should use `EXPECT` /
`EXPECT_CONTAINS` for happy-path commits so the actual output is
verified (see the version tests for the exact-match pattern).

### 9. Mandatory agent reviews

Per `.claude/rules/never-weaken-tests.md` + the Phase 3 gate in
PDR-010a:

- `memory-checker` agent -- dispatch with "amigit is a libgit2 CLI
  port, audit all TUs in `ports/amigit/ported/`. Focus on git_*_free
  call balance, cleanup on error paths, and libgit2 init/shutdown
  refcount." Apply CRITICAL and HIGH findings immediately.
- `perf-optimizer` agent -- dispatch with "68k review of amigit.
  All TUs are already at -O0 (required for libgit2.a binary
  consistency per crash-patterns #16). Focus on amigit-side code
  paths: argv parsing, string handling, callback loops. Recommend
  per-file -O1 promotion only if the function has no struct-by-value
  returns >8 bytes."

### 10. Final PORT.md, readme, LHA, catalog

- `ports/amigit/PORT.md` -- update Status to "v1 complete". Fill in
  the portability table, transformation table, memory audit
  results, perf review results.
- `ports/amigit/amigit.readme` -- Aminet-format readme. See
  `ports/templates/readme.template`. Short description <= 40 chars.
- `ports/amigit/amigit-0.1.lha` + `ports/amigit/amigit-0.1.readme`
  via `make package TARGET=ports/amigit`.
- `PORTS.md` -- add amigit row alphabetically in the CLI section.
- `README.md` -- add amigit row to the ports table.
- `data/catalog.json` -- add amigit entry. Remember
  `.claude/rules/catalog-sync.md` -- copy to
  `site/data/catalog.json` immediately after.
- `site/data/packages/amigit.json` -- per-port package JSON with
  `test_count`, `test_pass`, `porting_notes`, etc.

### 11. Commit as Phase 3c (v1)

Single commit with all 5 new commands + test suite + final PORT.md
+ Aminet artifacts + catalog/site entries. This is the v1 release
commit.

## Phase 3c gotchas

### Path handling is already solved

The `amigit_resolve_repo_path()` helper in `amigit.c` handles every
path case Phase 3c needs. DO NOT reinvent path normalization in
the new command files. Follow the existing cmd_status/log/show/diff
pattern for opening the CWD repo and cmd_init pattern for user-
supplied paths.

### libgit2 cleanup order is load-bearing

`git_index_free` + `git_tree_free` + `git_commit_free` + `git_signature_free`
+ `git_repository_free` (in roughly that order) on both success and
error paths. Missing a free leaks on AmigaOS permanently (no process
memory cleanup with `-noixemul`).

### `git_commit_create_v` appends \n to the message

Documented in known-pitfalls.md. Test expectations must include the
trailing newline for `show HEAD` message comparisons.

### `git_signature_now` requires a valid clock

Uses `time(NULL)` internally. libnix's time() on AmigaOS is fine
under FS-UAE / real hardware; vamos may need a fixed timestamp.
If Phase 3c tests run on vamos, prefer `git_signature_new` with a
fixed time for deterministic assertions.

### Never call exit() from command functions

Same rule as Phase 3a/b. Return the code; let main() propagate.

### Cleanup on commit-with-nothing-staged

`git_index_write_tree` on an empty index returns an error. Handle
this explicitly with a "nothing to commit" message rather than
letting it propagate through `amigit_error_exit`.

### No direct edits to lib/libgit2/ source

Upstream remains frozen. If Phase 3c discovers another upstream
path-handling bug, document it in known-pitfalls and work around
it in amigit itself (like the X:foo -> X:/foo rewrite).

## Phase 3c out-of-scope (explicit)

- Network commands (`clone`, `fetch`, `push`, `pull`, `remote`) --
  Phase 4, needs its own PDR.
- Merge/rebase/stash -- v1.1.
- Commit --amend -- no editor story on AmigaOS.
- Interactive rebase / add -- no editor story.
- Color output -- Category 1 CLI per PDR-010a.
- Pager integration -- users can run `amigit log | More` in the
  shell.

## Fresh session starting prompt

Copy this block verbatim into a fresh session after `/clear`:

```
Resume PDR-010 Phase 3c for amiport. Start by reading
ports/amigit/HANDOFF.md -- it has the full state, the local vamos
monkey-patches you need to verify, the files to read first, and the
Phase 3c task list. Current head is d2ca06f on origin/main; Phase
3a (version + init) and Phase 3b (status/log/show/diff) are both
shipped with 44/44 FS-UAE tests green.

After reading the handoff, verify the local vamos monkey-patches
are still applied by running `make -C tests/libgit2 run` -- should
see 79/79 pass in ~2s. Then verify Phase 3b still passes with
`make test-fsemu TARGET=ports/amigit` -- should see 44/44. If
either regresses, re-apply patches per HANDOFF.md or revert.

Then execute Phase 3c task list from HANDOFF.md: implement cmd_add,
cmd_commit, cmd_checkout, cmd_branch, cmd_tag following the
cmd_status.c template. After each command compiles, dispatch
build-manager to rebuild. After all 5 commands are in place,
dispatch test-designer in port mode for the Phase 3c extensions
to test-fsemu-cases.txt, then run `make test-fsemu TARGET=ports/amigit`
on FS-UAE. Gate on all tests passing.

After all tests pass, dispatch memory-checker and perf-optimizer
agents for the full amigit codebase. Apply CRITICAL/HIGH findings.
Then produce the final PORT.md writeup, amigit.readme,
amigit-0.1.lha package, PORTS.md + README.md entries, and
catalog.json + site/data/packages/amigit.json entries. Commit as
v1 release.
```

## Useful one-liners

```bash
# Phase 2 regression check (should print 79/79)
make -C tests/libgit2 run

# Phase 3b regression check (should print 44/44)
make test-fsemu TARGET=ports/amigit

# Smoke-test amigit version command
VAMOS_MEM=4096 toolchain/scripts/vamos -s 256 ports/amigit/amigit version

# Build amigit after adding a new command
make -C ports/amigit clean && make -C ports/amigit

# Check what libgit2 APIs exist
grep '^GIT_EXTERN' lib/libgit2/include/git2/*.h | less

# Verify docs haven't drifted
make check-docs
```

## Phase 3a/3b session artifacts

Commits pushed so far:

| Commit | Description |
|---|---|
| `0ca6a44` | test(lib): libgit2 Stage 5 tests 79/79 green (vamos-limited) |
| `9519f3b` | docs(lib): libgit2 Stage 6 memory audit CLEAN with 1 HIGH documented |
| `94fada7` | perf(lib): libgit2 Stage 7 hotpath promotion + Stage 8 re-verify |
| `d5f80d1` | docs(lib): libgit2 Stage 9 -- doc updates for Phase 2 completion |
| `e4d0466` | feat(port): amigit 0.1 Phase 3a proof-of-life (version + init) |
| `490839c` | docs(port): amigit Phase 3 session handoff brief |
| `d2ca06f` | feat(port): amigit 0.1 Phase 3b read-side commands (status/log/show/diff) |

Knowledge base additions (shared amiga-kb via `amiga_add_pitfall`):
- Phase 2: 6 pitfalls (vamos lock cap, strnlen/difftime gaps, khash
  -lm requirement, force-include replication, libgit2 commit message
  newline, libgit2 init refcount, vamos readdir gap).
- Phase 3b: 2 pitfalls (libgit2 volume path handling `X:foo` rewrite,
  `amiport_realpath` POSIX `.` handling).

Local environment changes NOT in git (must be re-applied):
- 2 amitools monkey-patches (see "CRITICAL" section above).
