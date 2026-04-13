# amigit Phase 3 Session Handoff

**For:** next session resuming PDR-010 Phase 3b after `/clear`.
**As of:** 2026-04-13, commit `e4d0466` on `origin/main`.

Read this file first in a fresh session, then the files it points to.
Do not re-derive the Phase 2 story -- it is shipped and working.

## TL;DR state

- **Phase 2 (lib/libgit2):** COMPLETE. 79/79 tests green on vamos via
  `-DAMIPORT_VAMOS_LIMITED`. 1.34 MB `libgit2.a` with 9-file `-O1`
  hotpath promotion. Memory audit CLEAN with one HIGH leak documented
  in `lib/libgit2/PATCHES.md` (revwalk init error path). Published to
  origin at commits `0ca6a44`, `9519f3b`, `94fada7`, `d5f80d1`.
- **Phase 3a (ports/amigit/ proof-of-life):** COMPLETE at commit
  `e4d0466`. `amigit version` and `amigit init` run cleanly on vamos.
  Binary is 1,057,516 bytes. Proves libgit2.a consumes into a user
  binary with zero friction.
- **Phase 3b (what's next):** add `status`, `log`, `show`, `diff`.
  Read-only commands. Full FS-UAE test suite. Specified in
  `docs/pdr/010a-amigit-cli-spec.md`.
- **Phase 3c (after 3b):** add `add`, `commit`, `checkout`, `branch`,
  `tag`. Write-side commands. Final v1 release -- PORT.md writeup,
  `amigit.readme`, LHA package, PORTS.md + README.md entries, catalog
  + site updates. Full memory-checker and perf-optimizer runs.

## CRITICAL: local vamos monkey-patches (NOT in git)

Without these, the `tests/libgit2/test_libgit2` and any filesystem-heavy
amigit test **crash on vamos**. Re-apply after any `pip upgrade amitools`
or fresh clone-install of amitools.

**File:** `~/.pyenv/versions/3.14.4/lib/python3.14/site-packages/amitools/vamos/lib/DosLibrary.py`

**Patch 1 -- raise DOS lock table from 1024 to 65536:**

Find line ~99 in `DosLibrary.py`:
```python
        self.lock_mgr = LockManager(ctx.path_mgr, self.dos_list, ctx.alloc, ctx.mem)
```
Change to:
```python
        self.lock_mgr = LockManager(ctx.path_mgr, self.dos_list, ctx.alloc, ctx.mem, max_locks=65536)
```

**Patch 2 -- SetFileDate must not raise FileNotFoundError:**

Find the `SetFileDate` method (around line 539-558). The end of the
method has:
```python
        else:
            os.utime(sys_path, (seconds, seconds))
            return DOSTRUE
```
Change to:
```python
        else:
            try:
                os.utime(sys_path, (seconds, seconds))
                return DOSTRUE
            except (FileNotFoundError, OSError) as e:
                log_dos.info("SetFileDate os.utime failed: %s -> %s", sys_path, e)
                self.setioerr(ctx, ERROR_OBJECT_NOT_FOUND)
                return DOSFALSE
```

**After both patches:**
```bash
rm ~/.pyenv/versions/3.14.4/lib/python3.14/site-packages/amitools/vamos/lib/__pycache__/DosLibrary.cpython-314.pyc
```

Verify with `make -C tests/libgit2 run` -- should print 79/79 in ~2s.

## Files to read first in a fresh session

Read these in order. They give a cold agent everything needed to
execute Phase 3b without re-deriving decisions:

1. `docs/pdr/010a-amigit-cli-spec.md` -- the v1 CLI spec. Defines
   exactly which 10 commands ship, output format, error mapping,
   category, test strategy. **Authoritative.**
2. `ports/amigit/PORT.md` -- current state of the port, smoke test
   results, known limitations. Starts with "Phase 3a proof-of-life".
3. `ports/amigit/ported/amigit.h` -- shared types, `amigit_cmd_fn`
   signature. New commands mirror this pattern.
4. `ports/amigit/ported/amigit.c` -- `main()` + dispatcher +
   `amigit_error_exit()` + `amigit_usage()`. **Do not duplicate these
   in new command files** -- call them.
5. `ports/amigit/ported/cmd_init.c` -- the canonical template for a
   new subcommand. Copy-paste this as the starting point for each new
   `cmd_<name>.c`.
6. `ports/amigit/Makefile` -- per-file compilation pattern. Adding a
   new command means adding one line to `OBJECTS`.
7. `tests/libgit2/test_libgit2.c` -- the richest reference for how to
   call every libgit2 API that Phase 3b needs (revwalk, diff, status).
   Specifically sections 9, 10, 11, 12, 13, 14. Even though 6 tests
   are gated for vamos, the *code* shows the right call sequence.
8. `lib/libgit2/include/git2/*.h` -- public API headers, authoritative
   for function signatures.
9. `.claude/rules/known-pitfalls.md` -- recent additions (search for
   "libgit2", "vamos", "strnlen") document everything already learned.
10. `.claude/rules/library-pipeline.md` -- the rule set that applies
    to `lib/libgit2/`. Phase 3b is a **port**, so `/port-project` rules
    apply, NOT the library-pipeline -- but the pitfalls still bite.

## Phase 3b task list

Execute in order. Each command is an incremental commit.

### 1. Spec review + alignment (10 min)

Read `docs/pdr/010a-amigit-cli-spec.md` sections "v1 scope" and "Output
format" and "Error mapping". Confirm no spec drift has been requested
by the user. If the user wants changes, amend the spec first.

### 2. `cmd_status.c` (first command)

- Create `ports/amigit/ported/cmd_status.c` modeled on `cmd_init.c`.
- Call `git_repository_open_ext` (NO_SEARCH) to get the repo; error
  with `GIT_ENOTFOUND` -> "fatal: not a git repository" to stderr,
  exit 10 (RETURN_ERROR).
- Call `git_status_foreach_ext` with porcelain v1 output format:
  - `M  <file>` for index modified
  - ` M <file>` for worktree modified
  - `A  <file>` for index added
  - `D  <file>` for index deleted
  - `?? <file>` for untracked
  - Omit clean files from output (default porcelain behavior).
- Supports flag: `-s` / `--short` (no-op for v1 -- already porcelain).
- Supports `--help` / `-h` (prints usage + exit 0).
- Register in `dispatch_table[]` in `amigit.c`.
- Declare `amigit_cmd_status` in `amigit.h`.
- Add `ported/cmd_status.o` to `OBJECTS` in `Makefile`.
- Dispatch build-manager to compile.

Reference: `tests/libgit2/test_libgit2.c` `status_new_file_untracked`
and `amiga_d_type_unknown_in_walk` tests. Both are currently gated
under `AMIPORT_VAMOS_LIMITED` because of vamos readdir gaps -- that
only blocks vamos testing, not real execution.

### 3. `cmd_log.c`

- `amigit log [-n N] [--oneline]`.
- Use `git_revwalk_new` + `git_revwalk_push_head` + `git_revwalk_next`
  loop until `GIT_ITEROVER`.
- Default output: `<7-char SHA> <commit summary>` per line.
- `-n N`: limit output to first N commits.
- `--oneline`: synonym for default (it's already oneline in v1).
- Use `git_commit_lookup` + `git_commit_summary` for each revwalk
  OID. Free each commit after reading.
- `amigit_error_exit` on any libgit2 error.

Reference: `tests/libgit2/test_libgit2.c` `revwalk_push_head_and_walk`
and `stress_10_commits_revwalk`.

### 4. `cmd_show.c`

- `amigit show <ref>`.
- Use `git_revparse_single` to resolve ref -> git_object.
- If object is `GIT_OBJECT_COMMIT`, cast to git_commit, print
  author/date/message + diff against first parent.
- Use `git_commit_tree` + `git_commit_parent(0)` + `git_diff_tree_to_tree`
  for the diff.
- Use `git_diff_print` with `GIT_DIFF_FORMAT_PATCH` to dump unified
  diff to stdout via a line callback.
- Handle no-parent case (initial commit): diff against empty tree.

Reference: `tests/libgit2/test_libgit2.c` `revparse_single_head` for
the resolve path, `diff_tree_to_tree_initial` for diff pattern.

### 5. `cmd_diff.c`

- `amigit diff [--cached]`.
- Default: `git_diff_index_to_workdir` (index vs worktree).
- `--cached`: `git_diff_tree_to_index` with HEAD's tree.
- Print via `git_diff_print` + `GIT_DIFF_FORMAT_PATCH` line callback.
- If there are no deltas, exit 0 with no output (standard git).

Reference: `tests/libgit2/test_libgit2.c` `diff_numdeltas_after_add`
and `diff_empty_repo`.

### 6. Test harness (test-designer dispatch)

Dispatch `test-designer` agent in **port mode** (not library mode):

> "Design `ports/amigit/test-fsemu-cases.txt` for the 6 commands
> amigit currently has: version, init, status, log, show, diff.
> Category 1 CLI. Non-interactive tests only (no ITEST blocks).
> Fixtures built at runtime via amigit itself -- use `amigit init`
> in the first test to create T:amigit-test, then subsequent tests
> assume that fixture. Cover every flag per test-coverage-standard.
> Include error paths: non-repo status, missing ref show, bad
> argument for each command. This is FS-UAE only -- vamos cannot
> run the suite (see AMIPORT_VAMOS_LIMITED gate in tests/libgit2).
> Reference the libgit2 test binary's repo-creation pattern."

### 7. Run FS-UAE tests

```
make test-fsemu TARGET=ports/amigit
```

Needs the forked FS-UAE at `~/Developer/fs-uae/` per the project's
standard FS-UAE setup. Scripts handle the rest.

Gate on all tests passing. If anything fails, triage and fix.

### 8. Commit as Phase 3b

Single commit with all 4 new commands + test suite + updated PORT.md.
Do NOT yet add PORTS.md / README.md entries or LHA package -- those
are Phase 3c deliverables when the full v1 lands.

## Phase 3b gotchas (proven in Phase 3a)

**Makefile: `TARGET` must be set before `include ../common.mk`.**
common.mk defines `all: $(TARGET)` which expands at parse time. If
TARGET is empty when common.mk is parsed, `all:` becomes a no-op and
make reports "Nothing to be done." This was learned the hard way in
Phase 3a. Other multi-file ports (less, mg, lua) have the same latent
bug and only work because their binaries already exist.

**Force-include is mandatory.** `-include ../../lib/libgit2/src/util/amigaos_compat.h`
in CFLAGS. Without it, libgit2 symbols bind against mismatched type
declarations and the link-time mismatch is not caught by the compiler
(see `.claude/rules/known-pitfalls.md` entry "Library unit test
Makefiles must replicate force-include flags").

**`-lm` is required.** libgit2's khash uses `double` for load-factor
math, which pulls in `__muldf3`/`__adddf3` soft-float routines that
live in libm on bebbo-gcc. Already in amigit's Makefile; don't remove.

**`git_error_clear()` is in `git2/sys/errors.h`, not `git2/errors.h`.**
Not pulled in by `git2.h`. Add the explicit include when calling it.

**Never call `exit()` from a command function.** main() runs `atexit`
cleanup that shuts down libgit2. Return from the command function;
let main() return the code. cmd_init.c is the template.

**Use `err()` / exit codes 10 not 1.** `RETURN_ERROR = 10` per
`.claude/rules/amiga-coding.md`. Amiga scripts test `IF WARN` (5),
`IF ERROR` (10), `IF FAIL` (20). `amigit_error_exit()` already does
the right thing; call it, don't rewrite.

**Do not touch `lib/libgit2/` source.** Upstream is frozen. Any
surprise from libgit2's behavior goes in a PATCHES.md note (like
the revwalk leak already there) or a new known-pitfalls entry.

**vamos is not a reliable test harness for amigit.** Phase 3a's
smoke-test failures (init on new top-level T: paths, reinit) hit
vamos's `FileManager.create_dir` path-translation gap. Real AmigaOS
and FS-UAE are unaffected. Phase 3b MUST use FS-UAE for the real
test suite -- do not chase vamos compatibility.

**No memory-checker or perf-optimizer in Phase 3b.** Defer to
Phase 3c. 400 lines of code across 6 commands still isn't enough
to warrant the full audit, and FS-UAE testing is the load-bearing
validation at this stage.

**`pr_WindowPtr = -1` at main() startup.** Already done in
`amigit.c`. If you add an alternate entry point or a helper that
spawns a subprocess, you'll need to repeat this pattern there too.

## Fresh session starting prompt

Copy this block verbatim into a fresh session after `/clear`:

```
Resume PDR-010 Phase 3b for amiport. Start by reading
ports/amigit/HANDOFF.md -- it has the full state, the local vamos
monkey-patches you need to verify, the files to read first, and the
Phase 3b task list with exact commands and gotchas. Current head is
e4d0466 on origin/main, Phase 3a (amigit version + init) is shipped.

After reading the handoff, verify the local vamos monkey-patches are
still applied by running `make -C tests/libgit2 run` -- you should
see 79/79 pass in ~2s. If it crashes with "no more lock slots" or a
FileNotFoundError, re-apply the patches per HANDOFF.md before
proceeding.

Then execute Phase 3b task list from HANDOFF.md: implement
cmd_status, cmd_log, cmd_show, cmd_diff as separate .c files under
ports/amigit/ported/, following cmd_init.c as the template. After
each command compiles, dispatch build-manager to rebuild. After all
4 commands are in place, dispatch test-designer in port mode for
ports/amigit/test-fsemu-cases.txt, then run `make test-fsemu
TARGET=ports/amigit` on FS-UAE. Gate on all tests passing. Commit
Phase 3b as a single commit when green. Do NOT yet create PORTS.md
or README.md entries or LHA packages -- those are Phase 3c.
```

## Out-of-scope for Phase 3b (explicit)

- Network commands (`clone`, `fetch`, `push`, `pull`, `remote`) --
  Phase 4, needs its own PDR.
- Merge/rebase/stash -- v1.1.
- `memory-checker` and `perf-optimizer` agent runs -- defer to 3c.
- `PORTS.md` row / `README.md` ports table entry -- defer to 3c.
- `amigit.readme` and LHA package -- defer to 3c.
- Catalog + site entries -- defer to 3c.
- `/extend-shim` for strnlen/difftime (still in the stubs file) --
  optional cleanup for a future session.
- Interactive tests (ITEST blocks) -- amigit is non-interactive by
  design, no ITESTs needed.
- Visual tests (`test-fsemu-visual-cases.txt`) -- Category 1 CLI,
  no visual verification.

## Session artifact summary (what landed in git)

5 commits on `origin/main` this session:

| Commit | Description |
|---|---|
| `0ca6a44` | test(lib): libgit2 Stage 5 tests 79/79 green (vamos-limited) |
| `9519f3b` | docs(lib): libgit2 Stage 6 memory audit CLEAN with 1 HIGH documented |
| `94fada7` | perf(lib): libgit2 Stage 7 hotpath promotion + Stage 8 re-verify |
| `d5f80d1` | docs(lib): libgit2 Stage 9 -- doc updates for Phase 2 completion |
| `e4d0466` | feat(port): amigit 0.1 Phase 3a proof-of-life (version + init) |

Additional changes NOT in git (local environment only):
- 2 amitools monkey-patches (see "CRITICAL" section above)

Knowledge base additions (via `amiga_add_pitfall` MCP): 6 pitfalls
routed to shared kb covering vamos lock cap, strnlen/difftime gaps,
khash -lm requirement, force-include replication, libgit2 commit
message newline, libgit2 init refcount, vamos readdir gap.

## Useful one-liners

```bash
# Verify Phase 2 tests still pass (should print 79/79)
make -C tests/libgit2 run

# Smoke-test amigit version command (expected: 3 lines + exit 0)
VAMOS_MEM=4096 toolchain/scripts/vamos -s 256 ports/amigit/amigit version

# Smoke-test amigit init with pre-existing parent (vamos workaround)
mkdir -p ~/.vamos/volumes/ram/t/amigit-test
VAMOS_MEM=4096 toolchain/scripts/vamos -s 256 ports/amigit/amigit init T:amigit-test/repo

# Build amigit after adding a new command
make -C ports/amigit clean && make -C ports/amigit

# Run the FS-UAE test suite (once cases file exists)
make test-fsemu TARGET=ports/amigit

# Check what libgit2 APIs exist
grep '^GIT_EXTERN' lib/libgit2/include/git2/*.h | less

# Verify docs haven't drifted
make check-docs
make check-port-metadata
```
