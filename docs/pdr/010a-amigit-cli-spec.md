# PDR-010a: amigit v1 CLI Specification

## Status

Proposed -- amendment to PDR-010

## Date

2026-04-13

## Relationship to PDR-010

PDR-010 established the *why* (local-only git for AmigaOS 3.x) and the
*what* (wrap libgit2 in a purpose-built Amiga CLI, skip fork/exec). This
amendment establishes the *v1 surface* -- exactly which commands ship,
what they output, how arguments are parsed, how errors are mapped, how
tests are written. It locks in scope so implementation does not drift.

Phase 2 completed 2026-04-13 with `lib/libgit2/libgit2.a` (1.31 MB,
79/79 tests green on vamos, CLEAN memory audit, 9-file `-O1` hotpath
promotion). Phase 3 is the consumer: `ports/amigit/`.

## v1 scope -- the 10-command core

v1 ships exactly these commands. No more, no less. Scope creep is
explicitly out of bounds; new commands require a PDR-010b or later
amendment.

| Command | Purpose | libgit2 API |
|---|---|---|
| `amigit init [path]` | Create an empty repo | `git_repository_init` |
| `amigit status` | List staged / unstaged / untracked | `git_status_foreach_ext` |
| `amigit log [-n N] [--oneline]` | Walk commit history from HEAD | `git_revwalk_*`, `git_commit_*` |
| `amigit show <ref>` | Show a single commit + its diff | `git_revparse_single` + `git_diff_tree_to_tree` |
| `amigit diff [--cached]` | Diff workdir vs index or index vs HEAD | `git_diff_index_to_workdir` / `_tree_to_index` |
| `amigit add <path>...` | Stage a file | `git_index_add_bypath` + `git_index_write` |
| `amigit commit -m <msg>` | Commit staged changes | `git_commit_create_v` |
| `amigit checkout <ref>` | Move HEAD + update workdir | `git_checkout_tree` + `git_repository_set_head` |
| `amigit branch [-l\|-d] [name]` | List, create, delete branches | `git_branch_*` |
| `amigit tag [-l] [name]` | List or create lightweight tag | `git_tag_*` |

Also: `amigit version` (no arguments) -- prints amigit version, libgit2
version via `git_libgit2_version()`, and the build date. Does not count
against the 10; it is required as a smoke-test entry point.

### Explicitly excluded from v1

- **Network transports:** `clone`, `fetch`, `push`, `pull`, `remote`.
  Blocked by the libgit2 pruning decision in Phase 2 (no streams/,
  transports/, clone.c, fetch.c, remote.c body). Re-adding any of these
  requires bsdsocket-shim + libssh2 or http-shim + smart HTTP -- a
  Phase 4 milestone with its own PDR.
- **Merge-heavy operations:** `merge`, `rebase`, `cherry-pick`,
  `revert`, `stash`. libgit2 supports these but the test matrix grows
  quickly and the UX questions are nontrivial. Candidates for v1.1.
- **Submodules, worktrees, reflog, bisect, blame, grep, archive, describe,
  notes, gc, fsck, prune, pack-objects.** Each has design questions
  that a v1 cannot responsibly answer.
- **Interactive flows:** `rebase -i`, `add -i`, `commit --amend` with
  editor invocation. The amiport pipeline has no portable $EDITOR story
  on AmigaOS; all editing is non-interactive via `-m` flags.
- **Color output.** Category 1 CLI (see below). A future `--color`
  flag could promote to Category 3 if we build terminal auto-detect.
- **`.gitignore` semantic parsing beyond what libgit2 handles
  internally.** Users can add files explicitly.

### Category classification (ADR-011)

**Category 1 (CLI tool).** amigit writes to stdout/stderr, exits with
POSIX return codes, takes arguments, and reads no terminal input. No
ANSI escapes, no cursor control, no raw-mode keyboard. Tests run on
vamos + FS-UAE functional harness (`test-fsemu-cases.txt` only, no
`test-fsemu-visual-cases.txt`).

Rationale: Category 3 unlocks color and paging, but both require
terminal detection plus visual-test infrastructure. amigit v1 gains no
user value from either; `amigit log | more` achieves pagination via the
Amiga shell's built-in `More` command. Keep it simple.

## Argument parsing

- **Subcommand style:** `amigit <verb> [args]`. Single top-level dispatch.
- **Parser:** `amiport_getopt_long` from `lib/posix-shim/`. Each
  subcommand owns its own option table. `amigit` itself only takes the
  verb; `--` is not supported at the top level.
- **Long vs short options:** short-only for v1 (`-n 5`, not
  `--max-count=5`). Reduces parser complexity. Long options can be
  added in v1.1 without breaking v1 scripts.
- **`--help` / `-h`:** each subcommand prints a one-paragraph usage
  then exits with RETURN_OK. `amigit --help` prints the command list.
- **Version strings:** `amigit version` prints three lines:
  `amigit VERSION (built DATE)`, `libgit2 VERSION`, `amiport-shim available`.
- **AmigaOS path support:** arguments that look like repository paths
  may use either POSIX (`foo/bar`) or AmigaOS (`WORK:repo`, `T:test`)
  syntax. libgit2 handles both once it is given the string; we do not
  pre-translate.

## Output format

- **Plain ASCII only.** Per `.claude/rules/amiga-coding.md`. No UTF-8,
  no smart quotes, no em-dashes.
- **Porcelain by default.** `amigit status` uses the git porcelain v1
  short format (`M  file`, `?? file`, etc.) even without
  `--porcelain`. Stable across versions, easy to test with `EXPECT:`.
- **`amigit log` default is oneline.** `SHA1 subject` per line. The
  25-row Amiga shell cannot usefully display multi-line commit entries.
  Users who want full format can ask for it in v1.1; meanwhile
  `amigit show HEAD` gives the full entry for one commit.
- **`amigit diff` is unified format**, 3 lines of context, no color.
  Direct passthrough of `git_diff_print` with
  `GIT_DIFF_FORMAT_PATCH`.
- **Error output** goes to stderr with `err(10, ...)` -- NOT
  `err(1, ...)` -- so Amiga shell scripts `IF WARN` / `IF ERROR` /
  `IF FAIL` work correctly (amiga-coding rule: `RETURN_ERROR = 10`).
- **Success output** goes to stdout. amigit never writes progress or
  informational messages to stderr.

## Error mapping

libgit2 returns negative integers on failure. amigit v1 handles them
through one central function:

```c
/* amigit_error_exit(rc) reads git_error_last(), prints a human string
 * to stderr, and exits with RETURN_ERROR (10). Never returns. */
void amigit_error_exit(int libgit2_rc);
```

Mapping table (libgit2 → amigit behavior):

| libgit2 code | Meaning | amigit behavior |
|---|---|---|
| `0` | Success | return 0 |
| `GIT_ENOTFOUND (-3)` | Object/path not found | print message, exit 10 |
| `GIT_EEXISTS (-4)` | Already exists | print message, exit 10 |
| `GIT_EAMBIGUOUS (-5)` | Ambiguous ref prefix | print message, exit 10 |
| `GIT_EBUFS (-6)` | Buffer too small | print message, exit 10 |
| `GIT_EINVALIDSPEC (-15)` | Bad ref name | print message, exit 10 |
| `GIT_EUNBORNBRANCH (-9)` | HEAD points nowhere | print message, exit 10 |
| `GIT_EUNMERGED (-10)` | Merge in progress | print message, exit 10 |
| `GIT_ENONFASTFORWARD (-11)` | Not fast-forward | print message, exit 10 |
| any other negative | Unknown libgit2 error | print `git_error_last()->message`, exit 10 |
| positive | N/A for most APIs | treat as success |

Special cases:
- **`amigit status` on a non-repo:** `git_repository_open` returns
  `GIT_ENOTFOUND`; amigit prints `fatal: not a git repository` to
  stderr and exits 10. Matches real git.
- **`amigit init` on an existing repo:** libgit2 returns 0 and
  reinitializes. amigit prints `Reinitialized existing repository in
  <path>` to stdout and exits 0. Matches real git.
- **No arguments to `amigit`:** prints usage to stderr, exits 10.

## Memory discipline

- Every libgit2 pointer has a matching `git_*_free()`. Cleanup runs on
  both success and error paths.
- `git_libgit2_init()` at the top of main, `git_libgit2_shutdown()`
  registered via `atexit()`. Balanced refcount per the known-pitfalls
  libgit2 discipline.
- `pr_WindowPtr = -1` suppression in main to block volume requesters,
  matching `tests/libgit2/test_libgit2.c` and the vim port.
- No global `git_repository *` singleton. Each command opens its own
  and frees before returning. Commands that need the repo accept it
  as a parameter from the dispatcher.
- `atexit` cleanup frees anything allocated before the dispatch returns.

## Build dependencies

```
ports/amigit/Makefile LDFLAGS:
  -L../../lib/libgit2 -lgit2
  -L../../lib/zlib    -lz
  -L../../lib/posix-shim -lamiport
  -lm
```

CFLAGS: `-std=gnu99 -O0 -noixemul -m68000 -Wall
-include ../../lib/libgit2/src/util/amigaos_compat.h`. Force-include
is required for the same reason as `tests/libgit2/` -- the macro
namespace must match libgit2.a. Plus the same link-time stubs for
`strnlen`, `difftime`, `select`, `git_remote_*`, `git_clone__submodule`,
`git_failalloc_*`, `git_socket_stream__*` -- consolidate into a shared
`amigit_libgit2_stubs.c` so each command TU does not re-paste them.

Stack: `long __stack = 262144;` (same as test binary). vamos stack via
`VAMOS_STACK = 256` + `-s 256`, RAM via `VAMOS_MEM = 4096` + `-m 4096`.

Category 1 link flag follows the shim-usage rule: `-lamiport` plus
the library-specific additions above.

## Test strategy

`test-fsemu-cases.txt` (functional only, no visual tests). Fixtures
built at runtime in `T:amigit-test/` via `amigit init` + `amigit
commit` itself -- meta-test. Covers:

1. `amigit version` -- smoke test, exit 0, output contains "libgit2"
2. `amigit init T:amigit-test-init` -- RC 0, directory exists
3. `amigit init` on existing dir -- RC 0, "Reinitialized" in output
4. `amigit status` in empty repo -- RC 0, empty output
5. `amigit add` on missing file -- RC 10
6. `amigit commit -m` with nothing staged -- RC 10
7. Full happy path: `init` + `add hello.txt` + `commit -m "first"` +
   `log --oneline` -- RC 0 for each, log shows 1 commit
8. Second commit + `log -n 2` -- RC 0, log shows 2 commits
9. `branch test` + `branch -l` -- RC 0, lists `test` and master/main
10. `checkout test` + `log --oneline` -- RC 0, history preserved
11. `tag v0.1` + `tag -l` -- RC 0, lists `v0.1`
12. `show HEAD` -- RC 0, output contains commit SHA
13. `diff` against staged but uncommitted changes -- RC 0, output
    contains `+` and `-` lines
14. Error paths for every command: RC 10 with appropriate message

Every flag the program accepts has at least one TEST, per
`docs/test-coverage-standard.md`.

The 6 `tests/libgit2/` tests that vamos cannot run (readdir-dependent)
will run fine in amigit's FS-UAE tests because those go through the
shell + AmigaDOS filesystem, not vamos's RAM volume.

## Phased build

Phase 3 is broken into three sub-milestones to avoid a ~2000-line
single commit:

- **Phase 3a (this session):** scaffold + `version` + `init` only.
  Proves the link works, `git_libgit2_init` runs in a user binary,
  and the Makefile / test harness patterns port from `tests/libgit2/`.
  **Single commit, single vamos smoke test.**

- **Phase 3b:** add `status`, `log`, `show`, `diff`. These are
  read-only; if libgit2 can hash and traverse, they should all work.
  Commit with a full FS-UAE functional test suite for all 6 commands.

- **Phase 3c:** add `add`, `commit`, `checkout`, `branch`, `tag`.
  Write-side commands; test the full happy path (init -> add ->
  commit -> branch -> tag). Final commit includes PORT.md write-up,
  amigit.readme, LHA package, PORTS.md entry, README.md row.

Each phase runs through the standard port pipeline stages 4-7 (build,
test, memory-checker, perf-optimizer) before committing.

## Non-goals

- Bit-compatibility with upstream git porcelain for every edge case.
  Where upstream git has 20 years of accumulated behavior, amigit v1
  picks the sensible-default interpretation and documents it in
  PORT.md.
- SHA-256 repos. SHA-1 only (matches Phase 2 libgit2 build
  configuration).
- `.gitattributes` filters, line-ending conversion, smudge/clean.
- Anything involving a pager, editor, or terminal UI.

## Acceptance criteria

Phase 3 is done when:
- `ports/amigit/amigit` (binary) exists and is committed
- All 10 v1 commands implemented, argv-parsed, error-mapped
- `test-fsemu-cases.txt` covers every command + every flag + error
  paths, per test-coverage-standard
- `make test-fsemu TARGET=ports/amigit` passes on FS-UAE
- `make test TARGET=ports/amigit` smoke-passes on vamos (subject to
  the readdir limit -- some tests may be gated with a comment)
- memory-checker agent signs CLEAN
- perf-optimizer agent reports no HIGH findings
- PORT.md exists (>= 60 lines), lists commands + flags + limitations
- amigit.readme is Aminet-format
- LHA package builds
- Catalog + site entries created
- PORTS.md and README.md ports table updated

Until then, whichever sub-phase is mid-flight is documented in PORT.md
so a future session can pick up without losing state.
