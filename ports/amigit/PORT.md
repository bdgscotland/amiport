# Port: amigit

## Overview

| Field | Value |
|-------|-------|
| Program | amigit |
| Version | 0.1-6 |
| Source | amiport-native (hand-written on libgit2 1.8.5) |
| Category | 1 -- CLI tool |
| License | GPL-2.0 (libgit2) + MIT (amiport code) |
| Original Author | Duncan Bowring (amiport project) |
| Port Date | 2026-04-13 |
| Last Update | 2026-04-14 (revision 6) |
| Maturity | **Developer preview** -- not yet stable for daily use |

## Status

**0.1-6 (2026-04-14) -- DEVELOPER PREVIEW.** All 11 PDR-010a v1
commands compile and run, **87/87** FS-UAE functional tests
passing. This revision closes Track A: bundled `libgit2.a`,
`libz.a`, and `libamiport.a` are now built at `-m68020` alongside
the amigit binary itself, via the new dual-flavor library build
(`lib/{zlib,libgit2,posix-shim}/Makefile` gained `all-020` and
`dual` targets; amigit's Makefile links against the `-020.a`
archives). 0.1-5 had been cosmetically `-m68020` -- amigit's own
13 port TUs were 68020-O1 but all the hot work (libgit2 pack/diff/
xdiff, zlib inflate, POSIX shims) ran 68000 code inside a 68020
process. 0.1-6 is the first revision where every code path in
the final image actually uses the 68020 instruction set.

**0.1-5 (2026-04-14) -- DEVELOPER PREVIEW.** Added `commit -F
<file>` for multi-word commit messages, promoted amigit's own 13
port TUs to `-O1`, and shipped a new rule enforcing that new
feature tests must exercise the feature's actual purpose (not
shallow happy-path "does it not crash" tests). See "What changed
in 0.1-5" below.

**0.1-4 (2026-04-13) -- DEVELOPER PREVIEW.** All 11 PDR-010a v1
commands compile and run, **82/82** FS-UAE functional tests
passing. But the test corpus covers synthetic happy paths and
does not exercise the full real-world workflow. The first user
who tried `WORK:amigit init` on a fresh Shell (post 0.1 release)
hit a libgit2 limitation immediately. 0.1-2 added a partial
friendly error; 0.1-3 widened it; 0.1-4 reframes the public
copy to be honest about preview status.

amigit 0.1.x is a **proof-of-concept that proves libgit2 can
be embedded into a 68k binary and that the on-disk git format
works under libnix file I/O**. It is NOT yet a usable git
client for daily work. Three honest gaps:

1. **Init friction wall.** `amigit init` from a typical Shell
   prompt on a multi-character volume name (`WORK:`, `Ram Disk:`,
   `System 3.1:`) does not work -- libgit2's `git_fs_path_root`
   only recognizes single-character drive prefixes. Workaround
   is to AmigaDOS Assign a single-letter alias (`Assign R: WORK:`)
   then init against that. A new user hits this wall in minute
   one. Three real-fix experiments tried and rolled back -- see
   "What changed in 0.1-3" below for the engineering notes.
2. **No network = no real git.** The whole collaboration value
   of git is missing. `amigit clone https://...` is the
   watershed feature and is not yet implemented. It needs a
   custom libgit2 smart-HTTP transport backend on AmiSSL,
   roughly 6-10 weeks of focused work tracked as the 1.0
   milestone.
3. **The 82 tests don't cover the workflow that broke.** They
   cover specific commands in specific synthetic paths
   (explicit `T:amigit-test` argument, isolated flag parsing,
   etc.). They didn't catch the bare-init case until the user
   typed it. They didn't catch the explicit-WORK:-init case
   until the user typed THAT either. The test corpus needs
   to grow alongside real usage. The positional-argument
   matrix rule (test-coverage-standard.md section 1a, added
   in this session) is the deterministic enforcement going
   forward.

### What's good for in this preview

- Validating the libgit2 + amiport posix-shim stack on real
  AmigaOS hardware (Vampire V2/V4, A1200, A3000/4000)
- Walking the history of an existing `.git/` directory copied
  over from a Linux machine via floppy/network
- Experimenting with a journal-style local-only workflow
- Reporting bugs that help shape 0.2 / 0.3 / 1.0

### What it's NOT good for yet

- Daily git workflow on real projects
- Collaboration via GitHub / GitLab / Codeberg / etc
- Anything that requires merge, rebase, or stash

### Roadmap (effort estimates from end-of-session perspective)

| Release | Scope | Effort estimate |
|---------|-------|-----------------|
| 0.2 | Init works from any volume + cheap perf wins | 1-3 sessions |
| 0.3 | Bundled libgit2 + zlib rebuilt at -m68020 (real perf) | 1-2 sessions |
| 1.0 | HTTPS clone, fetch, log against public repos via libgit2 smart-HTTP backend on AmiSSL | 6-10 weeks of focused work |
| 1.1+ | HTTPS push, libssh2 SSH, merge / rebase / stash | 4-8 weeks beyond 1.0 |

The real risk is **scope creep** on the 1.0 networking work. We
should ship 0.2 / 0.3 as visible incremental wins so the project
doesn't sit at 0.1.x and look abandoned during the long quiet
networking stretch.

### What changed in 0.1-6

Closes HANDOFF.md Track A ("CPU bump: -m68000 -> -m68020"). Bundled
libraries are now built with the 68020 instruction set, matching the
amigit binary's long-standing `-m68020` target.

- **Dual-flavor library builds.** `lib/zlib/Makefile`,
  `lib/libgit2/Makefile`, and `lib/posix-shim/Makefile` each grew
  a parallel `-020` target set. `make -C lib/<name> all` still
  builds only the 000 archive (unchanged for every other port and
  the Stage 5 test suite). `make -C lib/<name> all-020` builds the
  020 archive. `make -C lib/<name> dual` builds both. The 020
  objects use a `.020.o` suffix in the existing `src/` tree, so no
  shadow directories and no recursive make. CFLAGS_020 is derived
  via `$(subst -m68000,-m68020,$(CFLAGS))` so any future flag
  addition to the base CFLAGS propagates automatically.

- **000 archives are bit-identical to their pre-Track-A hashes.**
  Verified via `shasum -a 256` before and after the refactor:
  `libz.a` = `e12a2d28...`, `libgit2.a` = `0e04eca6...`, `libamiport.a`
  = `b0ecfdfa...`. The 000 variants are still what every other
  amiport port and the `tests/libgit2/` Stage 5 suite link against;
  there is zero behaviour change for any consumer that wasn't
  modified to explicitly opt in to the 020 flavors.

- **amigit relinks against the 020 archives.** `ports/amigit/Makefile`
  switched LDFLAGS from `-lgit2 -lz -lamiport` to `-lgit2-020 -lz-020
  -lamiport-020`, and the $(TARGET) rule's library prerequisites
  switched from `libgit2.a`/`libz.a` to `libgit2-020.a`/`libz-020.a`.
  `$(filter-out -lamiport,$(LDFLAGS))` removes the 000 libamiport
  that common.mk injected by default. `SHIM_LIB` override points
  at `libamiport-020.a`.

- **Binary is 2,540 bytes smaller at 1,078,892 bytes** (vs 0.1-5's
  1,081,432 bytes). The shrink comes from denser 020 instruction
  encoding in the libraries; amigit's own TUs are unchanged from
  0.1-5 (still -O1 -m68020).

- **The "cosmetic -m68020 at -O0" pitfall.** 0.1-5 was the first
  amigit revision compiled with `-m68020` for its own 13 TUs, but
  all three libraries (libgit2.a, libz.a, libamiport.a) were still
  `-m68000` because the project rule "Libraries MUST Use -m68000"
  (known-pitfalls.md) prevents vamos test regression. That meant
  every call out of amigit's code into libgit2's pack walker, zlib's
  inflate, or the POSIX shim's file I/O dropped from 68020 code
  back to 68000 code. Net perf win was close to zero -- the
  libraries were the hot path, not the CLI dispatch layer. Track A's
  dual-flavor build resolves this: the 020 variants exist alongside
  the 000 ones, amigit opts in, vamos tests for other ports still
  work because they see the unchanged 000 archives.

- **87/87 FS-UAE tests re-run.** The full test suite was run twice:
  once on the 0.1-5-stringed binary linked against 020 libraries (to
  prove the relink produces a functional binary before the version
  bump), and once on the 0.1-6-stringed binary after the `AMIGIT_VERSION`
  + `$VER` updates (to prove the version-bump verification gate).
  Both green. vamos smoke test also passed twice deterministically.

### What changed in 0.1-5

Cheap-wins pass from HANDOFF.md (B-track): perf promotion of amigit
port TUs to `-O1` (the 13 cmd_*.c + amigit.c files; `lib/libgit2/`
and `lib/zlib/` stayed at `-O0` in 0.1-5 but are rebuilt at 020
in 0.1-6), new `commit -F <file>` flag for multi-word commit
messages, and a testing-discipline rule to prevent the shallow-
happy-path lie that was caught in this session.

- **`-O1` promotion for amigit's port TUs.** perf-optimizer
  (2026-04-13 audit, HANDOFF.md item 1) declared all 13 TUs safe
  for `-O1`: no struct-by-value returns > 8 bytes
  (crash-patterns #16), no recursion, no float division in
  amigit's own code. Binary went from 1,085,544 bytes (0.1-4) to
  1,081,432 bytes (0.1-5) -- 4 KB smaller. `libgit2.a` and
  `libz.a` stay at `-O0` (changing those would require a full
  library pipeline re-run per `.claude/rules/library-pipeline.md`).

- **`commit -F <file>`.** Solves the single-word-message limitation
  of `commit -m`. AmigaDOS shells split argv on whitespace, so
  `commit -m "fix the bug"` delivers argv entries `"fix"`, `"the"`,
  `"bug"` and the second/third get rejected as "unexpected
  argument". `-F` reads the message from a file -- any content,
  including spaces, newlines, and up to 65,536 bytes. New helpers:
  `read_message_file()` slurps the file into a malloc'd buffer;
  `amigit_commit_free_msg_buf()` is registered via `atexit()` so
  every exit path (including deep libgit2 errors) cleans up the
  buffer. `-m` and `-F` are mutually exclusive.

- **Testing discipline -- the shallow-happy-path lie.** The first
  pass of `-F` tests had a single-word happy-path case
  (`"fromfile"`) that proved nothing `-m fromfile` wouldn't already
  prove -- a shallow test lie for a feature that exists
  specifically to deliver multi-word messages. Caught by user in
  mid-session and fixed: the happy-path now uses `"fix the broken
  parser in cmd_commit"` and is paired with a follow-up
  `log -n 1` test verifying the full multi-word message survives
  through libgit2's commit object storage. New rule
  `.claude/rules/test-designer-for-new-features.md` mandates
  test-designer dispatch for new feature tests; the test-designer
  agent prompt now carries a "Shallow Happy-Path Smell -- REJECT
  These Tests" section to detect this pattern in diff-audit mode.

- **Multi-suite testing (DEFERRED).** User also called out that
  "git is a very complex beast" and amigit specifically needs
  multiple test suite files (unit/integration/e2e/scenario/stress)
  with git-specific assertion primitives, not a single monolithic
  `test-fsemu-cases.txt`. Captured as a project memory
  (`project_amigit_multi_suite_testing.md`) with a full proposal;
  deferred to its own session -- not a blocker for 0.1-5.

- **Binary:** 1,081,432 bytes (1.03 MB), `-m68020 -O1 -noixemul`.
- **Test suite:** 87/87 FS-UAE functional tests passing (85 prior
  + 3 new `-F` error paths + 1 new `-F` happy path + 1 new
  `-F`-through-log roundtrip test... wait, 82 prior + 4 new = 86
  + 1 more roundtrip = 87. Math checks.).
- **Dependencies unchanged:** libgit2 1.8.5, zlib 1.3.1, posix-shim.

### What changed in 0.1-4

Metadata-and-copy reframe only. No source code changes.

- `amigit.readme` rewritten to lead with "DEVELOPER PREVIEW",
  describe what works and what doesn't honestly, lead with the
  init friction wall as the first thing the user will hit, and
  publish the 0.2 / 0.3 / 1.0 roadmap with effort estimates.
- `Short:` field changed from "Local-only git client for
  AmigaOS 3.x" to "Local-only git CLI for AmigaOS 3.x (preview)"
  -- the (preview) tag flags the maturity at a glance.
- PORT.md (this file) reframed to lead with the preview
  status and the three honest gaps, with the roadmap table
  above.
- site/data/packages/amigit.json status field flipped from
  "stable" to "preview" so the website renders the preview tag
  next to the package name.
- Version strings bumped to 0.1-4 in Makefile, AMIGIT_VERSION
  macro, $VER tag, test-fsemu-cases.txt expectation. Binary
  rebuilt clean, 82/82 tests still green (no functional change).

The reframe was triggered by Duncan's question end-of-session
2026-04-13: "we're overselling the usefulness of it in its
current form, do you think we'll get it to where it needs to
go?" The honest answer was yes-but-the-narrative-is-wrong, and
the right move was to fix the narrative tonight rather than
sit on it.

### What changed in 0.1-3 (since 0.1-2)

### What changed in 0.1-3 (since 0.1-2)

User-visible: `amigit init WORK:foo` now produces the same friendly
"libgit2 limitation" error as bare `amigit init` from a CWD on a
multi-character volume name. Pre-0.1-3, only the bare-CWD case fired
the friendly error -- explicit-path init on a multi-char volume
(`amigit init WORK:playground` from a Shell at `WORK:`) fell through
to libgit2's cryptic `failed to make directory './.'`.

Engineering note (kept for future fix attempts): three approaches to
making init *actually work* on multi-char volumes were tried in this
session and rolled back:

1. **Patch libgit2's `dos_drive_prefix_length`** to scan for the
   first `:` on AmigaOS, plus an amigit-side rewrite that injects
   `/` after the colon. Result: regressed 20 in-repo tests because
   libnix doesn't treat `"Ram Disk:/foo"` the same as `"Ram Disk:foo"`
   for `p_stat`/`p_open` operations. The deeper fix needs both
   libgit2 path recognition AND libnix path normalization to agree.

2. **chdir-then-init dance** -- `amiport_chdir` into the target,
   then call `git_repository_init(repo, ".", is_bare)`. Result:
   same `./.` mkdir error, because libgit2 absolutizes `.` early
   via `realpath` and ends up with the same multi-char form. The
   chdir doesn't bypass libgit2's path-root limitation.

3. **Manual init bypass** -- create the `.git/` directory structure
   directly via libnix `mkdir`+`fopen` (HEAD, config, refs/, objects/,
   etc.), skipping `git_repository_init` entirely for the multi-char
   case. Result: caused unrelated test 29 (`amigit show -h`) to hang
   on FS-UAE. Root cause not diagnosed in-session -- possibly an
   `<amiport/dirent.h>` header-include side-effect or a binary-size
   pressure on the 4 MB test config. Reverted to friendly-error-only
   for 0.1-3 to avoid shipping a regression.

The friendly-error coverage in 0.1-3 is the correct compromise for
this revision. The real fix (some combination of libgit2 patching,
libnix path normalization, or a more careful manual-init bypass)
is deferred to a future revision once the test-29 hang is understood
and the libnix/libgit2 path-handling reconciliation is designed.

### What changed in 0.1-2 (since 0.1 first release)

- **Compiled with `-m68020`** (was `-m68000`). amigit now targets
  accelerated Amigas only (Vampire V2/V4, A1200 with 030+, A3000/4000,
  modern emulators). Plain 68000/A500 is no longer supported -- the
  realistic audience for a git client is accelerators where the
  binary is large and the workload is non-trivial. See memory note
  `project_amigit_68020_target.md` for the full rationale. vamos is
  invoked with `-C 68020`.
  **Honest performance disclosure:** at `-O0`, the perf-optimizer
  audit found this flag flip is essentially cosmetic for amigit's
  own translation units -- bebbo-gcc 6.5.0b's `-O0` emits straight
  sequential code with no scheduling, and amigit's TUs have no
  integer math worth `MULS.L` and no pointer alignment hazards.
  The hot work lives in `lib/libgit2/libgit2.a` and `lib/zlib/libz.a`
  which are still built at `-m68000` for cross-port compatibility.
  Real perf wins from CPU targeting are blocked behind a future
  library rebuild at `-m68020` plus updating `tests/libgit2/`
  to pass `-C 68020` to vamos. Deferred to amigit 0.1-3 (or to a
  dedicated "amigit accelerator build" of `lib/libgit2/`). The
  `-m68020` flag on amigit binaries today is *forward-looking* --
  it ensures that when libgit2 flips, all the link-time pieces
  match, and it cleanly documents the project's stated minimum
  CPU target. For now, treat the speedup as <5%.
- **Friendly error for `amigit init` from a multi-character volume CWD.**
  When the user runs `amigit init` with no positional arguments from
  inside a Shell whose CWD is on a volume like `WORK:`, `Ram Disk:`,
  or `System 3.1:`, libgit2's `git_fs_path_root()` does not recognize
  the path as rooted (it only handles single-character ASCII drive
  letters), so `mkdir_canonicalize` walks dirname back to `./.` and
  fails with the cryptic `failed to make directory './.'`. amigit
  now detects this case in `cmd_init.c` and emits a multi-line error
  pointing the user at the explicit-path workaround:
  `amigit init <path>`. All other commands (status, log, add, commit,
  branch, checkout, tag) work normally from any CWD because they go
  through `git_repository_open_ext()` which has a tolerant path
  handler. The deeper fix (extending libgit2's path root recognizer
  to handle AmigaOS multi-char volume names) is deferred -- I tried
  it and it regressed 20 in-repo tests because libnix does not treat
  `"Ram Disk:/foo"` the same way as `"Ram Disk:foo"` for stat/read.
- **`cmd_log` hot path uses `fputs`+`fputc`** instead of `printf("%s %s\n",...)`.
  Skips libnix's format-parser overhead which adds up when walking
  long histories.
- **`amigit_is_help_flag()` consolidated** into a single shared helper
  (declared in `amigit.h`, defined in `amigit.c`). Removed 10 duplicate
  `static int is_help_flag(...)` definitions across `cmd_*.c` files.
  Mechanical refactor; behavior identical.
- **New regression test (test 82)** asserts that `amigit init` from
  a multi-char volume CWD exits RC=10 with the friendly error path.
  This is the zero-positional-arg cell of the `init` positional-
  argument matrix per the new test-coverage standard section 1a.
  Before 0.1-2, no test exercised the user CWD case at all -- this
  is exactly the gap that made the original bug invisible to 81/81.
- **Makefile fix:** `VERSION` and `REVISION` must be set BEFORE
  `include ../common.mk` -- common.mk evaluates `DISPLAY_VERSION`
  via `ifeq` at include time. Previously revision-bumping a port
  silently produced LHA filenames with the wrong version. Other
  ports doing post-include REVISION overrides have the same trap.

### v1 (0.1) status, retained

memory-checker CLEAN, perf-optimizer CLEAN (audited 2026-04-13).

Phase breakdown (per PDR-010a "Phased build"):

- **Phase 3a** -- scaffold + `version` + `init`. Proved libgit2.a links
  into a user binary, `git_libgit2_init` runs outside the unit-test
  harness, and the Makefile + force-include pattern from
  `tests/libgit2/` ports cleanly to `ports/amigit/`. Commit `e4d0466`.
- **Phase 3b** -- read-side commands: `status`, `log`, `show`, `diff`.
  44/44 FS-UAE tests. Discovered and worked around two libgit2
  AmigaOS path-handling bugs (see "Path handling" below). Added
  `amigit_resolve_repo_path()` as the single choke point for path
  normalization, and patched `amiport_realpath` in the shim to
  handle POSIX `"."`. Commit `d2ca06f`.
- **Phase 3c (this release)** -- write-side commands: `add`, `commit`,
  `checkout`, `branch`, `tag`. Completes v1 CLI surface. Discovered
  and worked around a critical FS-UAE soft-float crash in libgit2's
  patch-generation path (see "The __divsf3 crash" below).

## Upstream source

**None.** amigit is an amiport-native CLI written from scratch on top
of `lib/libgit2/libgit2.a`. Real git is structurally infeasible on
68k AmigaOS 3.x because its command dispatch is built on
`fork()`/`execvp()` with 53 call sites in `run-command.c` alone --
see `docs/pdr/010-amigit-on-libgit2.md` for the full analysis and
the reason libgit2 is the chosen path.

The `original/` directory exists to satisfy the port-directory
hygiene rule, but contains only a README explaining the absence.
Every source file in `ported/` is hand-written.

## Commands (v1 surface)

All 10 v1 commands per PDR-010a. Each has `--help` / `-h`, and every
flag is exercised in the FS-UAE test suite.

| Command | Purpose | libgit2 API |
|---|---|---|
| `amigit version` | Print amigit + libgit2 versions | `git_libgit2_version` |
| `amigit init [--bare] [path]` | Create an empty repo | `git_repository_init` |
| `amigit status [-s]` | Porcelain v1 status | `git_status_foreach_ext` |
| `amigit log [-n N] [--oneline]` | Walk HEAD history | `git_revwalk_*` + `git_commit_summary` |
| `amigit show <ref>` | Commit + diff | `git_revparse_single` + `git_diff_tree_to_tree` |
| `amigit diff [--cached]` | Unified diff | `git_diff_index_to_workdir` / `_tree_to_index` |
| `amigit add <path>...` | Stage files | `git_index_add_bypath` + `git_index_write` |
| `amigit commit -m <msg>` | Record commit | `git_commit_create_v` |
| `amigit checkout <ref>` | Switch HEAD | `git_checkout_tree` + `git_repository_set_head` |
| `amigit branch [-l\|-d] [name]` | List/create/delete | `git_branch_*` |
| `amigit tag [-l] [name]` | List/create tag | `git_tag_create_lightweight` |

`amigit version` prints:

```
amigit 0.1 (built 2026-04-13)
libgit2 1.8.5
amiport posix-shim available
```

## Architecture

```
ports/amigit/ported/
  amigit.h                   -- shared types, dispatch signature,
                                error-exit declarations, path resolver
                                prototype
  amigit.c                   -- main() + dispatcher + usage +
                                error mapping + libgit2 lifecycle +
                                amigit_resolve_repo_path()
  amigit_libgit2_stubs.c     -- link-time stubs (strnlen, difftime,
                                select, git_remote_*, git_clone__*,
                                git_failalloc_*, git_socket_stream__*,
                                __divsf3, __floatunsisf)
  cmd_version.c              -- `amigit version`
  cmd_init.c                 -- `amigit init [--bare] [path]`
  cmd_status.c               -- `amigit status [-s|--short]`
  cmd_log.c                  -- `amigit log [-n N] [--oneline]`
  cmd_show.c                 -- `amigit show <ref>`
  cmd_diff.c                 -- `amigit diff [--cached|--staged]`
  cmd_add.c                  -- `amigit add <path>...`
  cmd_commit.c               -- `amigit commit -m <msg>`
  cmd_checkout.c             -- `amigit checkout <ref>`
  cmd_branch.c               -- `amigit branch [-l|-d] [name]`
  cmd_tag.c                  -- `amigit tag [-l] [name]`
```

Each subcommand lives in its own translation unit. New commands are
added by:

1. Create `ported/cmd_<name>.c` with `int amigit_cmd_<name>(int argc, char **argv)`.
2. Add extern declaration to `ported/amigit.h`.
3. Add entry to `dispatch_table[]` in `ported/amigit.c`.
4. Add `ported/cmd_<name>.o` to `OBJECTS` in `Makefile`.

## Build

```
make -C ports/amigit
```

Depends on `lib/libgit2/libgit2.a`, `lib/zlib/libz.a`,
`lib/posix-shim/libamiport.a`. The Makefile declares them as
dependencies so missing libraries trigger a sub-make in the right
directory.

### Flags

- `-std=gnu99 -O0 -noixemul -m68000 -Wall` (overrides common.mk's -O2
  via `$(subst -O2,-O0,$(CFLAGS))` because libgit2.a is built -O0 and
  consuming binaries must match to avoid crash-patterns #16 struct
  return corruption)
- `-include ../../lib/libgit2/src/util/amigaos_compat.h` -- force-
  include required so the macro namespace (`pread`, `pwrite`,
  `realpath`, etc.) matches the built libgit2.a. Without this, libgit2
  symbols bind against mismatched type declarations at link time.
  Same pattern as `tests/libgit2/Makefile`.

### Link chain

```
-L../../lib/libgit2 -lgit2 \
-L../../lib/zlib -lz \
-L../../lib/posix-shim -lamiport \
-lm
```

Order matters:
- `-lgit2` before `-lz` -- libgit2 references zlib symbols.
- `-lamiport` from common.mk's LDFLAGS -- amiport shim is needed for
  `amiport_getopt_long`, future file ops, etc.
- `-lm` at the tail -- libgit2's khash uses `double` for load-factor
  math, pulling in `__muldf3`/`__adddf3` soft-float routines.
  See known-pitfalls "libgit2 khash requires -lm".

### Binary size

| Build | Bytes | Notes |
|---|---|---|
| Phase 3a (version + init) | 1,057,516 | 2 commands |
| Phase 3b (+ status/log/show/diff) | 1,071,256 | 6 commands |
| Phase 3c (+ add/commit/checkout/branch/tag) | 1,085,544 | 10 commands (v1 complete) |

The Phase 3c delta (~14 KB) is modest because libgit2's diff /
status / index / branch / tag / revwalk machinery was already
transitively pulled in by Phase 3b's `status` and `log` commands.
Each additional Phase 3c command TU adds only the argparse, the
thin libgit2 dispatch, and its per-command cleanup.

## Path handling

AmigaDOS paths and libgit2's POSIX-centric path logic have two
incompatibilities that required workarounds (both discovered in
Phase 3b, documented in `.claude/rules/known-pitfalls.md`):

1. **Volume-rooted paths are not recognized as rooted.**
   `git_fs_path_root("T:amigit-test")` returns -1 because
   `path[2]='a'` is not `/`. libgit2 then treats the path as
   relative, takes `dirname()` of a single-component path (which
   returns `"."`), and ends up running `p_mkdir("./.")`. Mkdir fails
   with "failed to make directory './.': No such file or directory".

   **Fix:** `amigit_resolve_repo_path()` rewrites `"X:foo"` to
   `"X:/foo"`. With the slash at offset 2, `git_fs_path_root` returns
   2 (rooted), and libgit2's mkdir walk terminates cleanly on the
   volume boundary.

2. **`Lock(".")` fails on AmigaOS.** libgit2's
   `git_fs_path_prettify()` calls `p_realpath(path, buf)` which maps
   to `amiport_realpath()`. The old implementation called
   `Lock(path, SHARED_LOCK)` -- AmigaOS does not accept `"."` as a
   valid path, so `Lock(".")` returns NULL and realpath fails.

   **Fix:** `lib/posix-shim/src/file_io.c` -- `amiport_realpath()`
   now special-cases `"."` / `""` / NULL and uses
   `pr_CurrentDir` + `NameFromLock()` instead of `Lock()`. This is
   a pure shim improvement that also benefits any future port using
   POSIX `realpath(".")` semantics.

3. **libnix/AmigaDOS divergence on `"T:foo"` vs `"T:/foo"`.**
   Important subtlety: these two forms are NOT equivalent under
   libnix. After `amigit init T:amigit-test` creates a repo, the
   directory is accessible to libgit2 via `"T:/amigit-test"` but
   AmigaDOS `CD T:amigit-test` returns "object not found". The
   FS-UAE test harness wrappers (`test-amigit-inrepo.rexx` and
   `test-amigit-inrepo-setup.rexx`) apply the same `"X:foo"` ->
   `"X:/foo"` rewrite before `CD`, keeping the storage-path
   convention consistent between amigit and the test harness.

`amigit_resolve_repo_path()` is the single choke point for path
normalization. Every command that hands a user-supplied path to
libgit2 open/init funnels it through this helper.

## The __divsf3 crash (Phase 3c critical fix)

**The most dangerous bug this release.** libgit2's `patch_generate.c`
line 261 computes:

```c
float progress = patch->diff ?
    ((float)patch->delta_index / patch->diff->deltas.length) : 1.0f;
```

This is the **only** single-precision float operation anywhere in
libgit2's compiled archive. And critically, the `progress` value is
computed **before** the `output->file_cb == NULL` check -- so it runs
even when the caller (e.g. `git_diff_to_buf`) has no progress
callback and throws the value away.

On AmigaOS with bebbo-gcc `-noixemul -m68000 -lm`, this pulls
`__divsf3` and `__floatunsisf` from libnix, both of which route
through ROM `mathieeesingbas.library`. On FS-UAE that ROM library
crashes with Guru Meditation `8000 000B` (ACPU_LineF) -- the same
crash documented in crash-patterns #2 for SDL_CreateRenderer's
`dpi_scale` float division. **Vamos does not exhibit the crash**
because vamos intercepts math-library calls through host math, so
the failure is entirely FS-UAE-specific and invisible to the vamos
smoke-test pipeline.

**Symptom:** `amigit diff --cached` on a repo with a real staged file
crashes with Guru `8000 000B` on FS-UAE A1200/68020, while all tests
that use `git_status_foreach_ext`, `git_diff_tree_to_index`,
`git_diff_index_to_workdir`, `git_revwalk`, or `git_commit_lookup`
pass cleanly. The sole differentiator: the crashing code path is
the only one that calls `git_patch_generate`'s internal callback
invoker.

**Diagnosis path (bisection):**
1. Initial 8-test isolation suite confirmed `diff --cached` was the
   unique failing command with the known add-commit-add-diff
   sequence.
2. Rebuilding `lib/libgit2/xdiff/*.c` at `-O0` did not fix the crash
   (eliminates codegen bug theory).
3. Rebuilding `lib/zlib/` fully at `-O0` did not fix the crash
   (eliminates zlib -O1 hotpath theory).
4. Switching `cmd_diff.c` and `cmd_show.c` from `git_diff_print` +
   per-line callback to `git_diff_to_buf` + single `fwrite` did not
   fix the crash (eliminates callback mechanism theory).
5. Adding intermediate tests (log on committed content, status on
   committed repo, status with staged changes) all passed -- proved
   the crash is NOT in object reading, blob inflate, tree walking,
   or zlib, but specifically in the diff-formatting code path.
6. `m68k-amigaos-nm lib/libgit2/src/libgit2/patch_generate.o` showed
   exactly two undefined soft-float symbols: `__divsf3` and
   `__floatunsisf`. These are the only ones.
7. amiga-kb crash-pattern #2 matched immediately on Guru 8000000B:
   libnix single-precision soft-float routes through ROM
   mathieeesingbas.library which crashes on FS-UAE.

**Fix:** override both symbols in `ported/amigit_libgit2_stubs.c`
with stubs that return `0.0f`. The linker resolves the stubs file
before libnix, so our definitions win. The progress value is
discarded by the NULL file_cb check immediately after, so
correctness is irrelevant -- only not-crashing matters.

```c
/* In ported/amigit_libgit2_stubs.c */
float __divsf3(float a, float b) { (void)a; (void)b; return 0.0f; }
float __floatunsisf(unsigned int x) { (void)x; return 0.0f; }
```

Verified via `nm` after override: symbols resolve to the amigit
binary's own addresses (not libnix's archive addresses). Full
81-test amigit FS-UAE suite goes from "crash on test 79
(`diff --cached`)" to 81/81 passing.

**Generalizes to any libgit2 consumer.** The only libgit2 TU that
references these symbols is `src/libgit2/patch_generate.o`, so a
consumer that never calls any `git_diff_print` / `git_diff_to_buf` /
`git_patch_*` API may not need the override. But any consumer that
does MUST provide it or crash. The pitfall is filed in amiga-kb and
also appended to `.claude/rules/known-pitfalls.md`.

## Testing

### Full FS-UAE test suite (81 tests)

`ports/amigit/test-fsemu-cases.txt` -- 81 TEST blocks covering all
11 commands (10 v1 commands + `version`). Run with:

```
make test-fsemu TARGET=ports/amigit
```

**Result: 81/81 passing on FS-UAE.** No weakened assertions.

Coverage breakdown:

| Command | Happy path | Flag parsing | Error paths | Total |
|---|---|---|---|---|
| `version` | 3 | 0 | 1 | 4 |
| top-level dispatch | 2 | 2 | 2 | 6 |
| `init` | 4 | 3 | 1 | 8 |
| `status` | 2 | 3 | 2 | 7 |
| `log` | 2 | 2 | 3 | 7 |
| `show` | 0 | 2 | 3 | 5 |
| `diff` | 4 | 2 | 2 | 8 |
| `add` | 1 | 2 | 2 | 5 |
| `commit` | 2 | 2 | 3 | 7 |
| `checkout` | 2 | 2 | 1 | 5 |
| `branch` | 4 | 3 | 2 | 9 |
| `tag` | 3 | 2 | 2 | 7 |
| Amiga-specific | 2 | 0 | 0 | 2 |
| Stress/real-world | 4 | 0 | 0 | 4 |

The Phase 3c "full workflow" scenario (tests 45-81) creates a
second repo fixture `T:amigit-c3`, stages hello.txt, commits,
creates branch foo, checks out foo, switches back to master,
deletes foo, creates tag v0.1, then stages world.txt, runs
`diff --cached`, commits again, and walks the log. This exercises
every v1 command end-to-end in a single session.

### Test harness wrappers

Two ARexx wrappers sit in `ports/amigit/`:

- `test-amigit-inrepo.rexx` -- CDs into a repo directory, runs
  `WORK:amigit <subcmd>`, captures stdout + RC. Used by every
  test that needs a CWD inside the repo.
- `test-amigit-inrepo-setup.rexx` -- same as inrepo.rexx plus a
  pre-step that creates a file via `Echo` before running amigit.
  Used by `add` / `commit` tests that need a file in the working
  tree before staging.

Both wrappers apply the `"X:foo"` -> `"X:/foo"` rewrite before CD
so AmigaDOS and libnix agree on the same directory.

Flag-parsing tests (`--help`, unknown flags, no-arg errors) do NOT
use the wrapper -- they short-circuit before the repo open call
and run directly via `WORK:amigit <subcmd> --help`. This halves
the FS-UAE process churn compared to routing everything through
the wrapper (which hung at ~24 tests due to resource exhaustion).

### Vamos regression

The libgit2 Stage 5 test suite (`tests/libgit2/test_libgit2`)
continues to pass 79/79 on vamos after the amigit changes. Both
`lib/libgit2/libgit2.a` and `lib/zlib/libz.a` are unchanged by
this release -- all Phase 3c fixes live in `ports/amigit/`.

## Memory audit (memory-checker CLEAN)

All 13 translation units were audited for libgit2 handle leaks and
double-frees. **Verdict: CLEAN.** Key findings:

- Every `git_*_free()` / `git_buf_dispose()` fires on both success
  and error paths, including the N-deep unwinding in `cmd_commit.c`
  (repo -> idx -> tree -> author -> committer -> head_ref -> parent).
- Pointer-stealing patterns (`commit = (git_commit *)obj; obj = NULL;`
  in `cmd_show.c`) are safe: the original handle is explicitly
  NULLed after the cast, so no double-free on error paths.
- `git_libgit2_init` / `git_libgit2_shutdown` refcount is balanced:
  one init in `main`, one shutdown via `atexit`. The atexit-failure
  fallback path manually shuts down to keep the refcount balanced.
- The soft-float stubs in `amigit_libgit2_stubs.c` take float args,
  return `0.0f`, do nothing -- no memory concerns.

## Performance review (perf-optimizer CLEAN)

All 13 TUs audited for 68k performance hazards. **Verdict: no
HIGH/CRITICAL findings.** Marginal optimizations noted as future
revision candidates (not applied in v1):

- `cmd_log.c:139` could use `fputs` + `putchar` instead of `printf`
  in the revwalk loop (~30-50% cycles/commit), but the overall path
  is I/O-bound so the gain is small.
- `cmd_show.c` `strlen(msg)` could be cached (saves ~400 cycles).
- `is_help_flag` is duplicated across 11 TUs (~500 bytes of binary).
  Consolidating to `amigit.h` as a non-static would save binary size
  and help the 68020 I-cache.
- **All 13 TUs are SAFE for `-O1` promotion.** No struct-by-value
  returns > 8 bytes (crash-patterns #16), no recursive functions,
  no float division in amigit's own code. The Makefile could switch
  from `-O0` to `-O1` across the port for a free marginal win,
  pending a fresh FS-UAE run to confirm no regressions. Deferred to
  a future revision to avoid re-triggering the fix/retest cycle.

## Known limitations (v1)

- **No network commands.** `clone`, `fetch`, `push`, `pull`,
  `remote` require bsdsocket + libssh2 / http-shim -- a Phase 4
  milestone with its own PDR.
- **No merge/rebase/cherry-pick/revert/stash.** libgit2 supports
  these but the test matrix grows quickly and the UX questions are
  nontrivial. Candidates for v1.1.
- **No interactive flows.** `rebase -i`, `add -i`, `commit --amend`
  with editor invocation are out of scope -- amiport has no portable
  $EDITOR story on AmigaOS, so all editing is non-interactive via
  `-m` flags.
- **No submodules, worktrees, reflog, bisect, blame, grep, archive,
  describe, notes, gc, fsck, prune, pack-objects.** v1.1+ candidates.
- **Single-word commit messages only.** AmigaDOS splits command-line
  arguments on whitespace and Execute scripts do not preserve quotes,
  so `amigit commit -m "first commit"` becomes two argv entries and
  the second ("commit") is rejected as "unexpected argument".
  Workaround: use single words (`first`, `firstcommit`, `first-commit`)
  or commit via a modern git elsewhere and copy the `.git` directory.
- **No color output, no pager integration.** Category 1 CLI per
  PDR-010a. Users can pipe through AmigaDOS `More` for paging.
- **amigit-side path normalization, not libgit2-side.** The
  `X:foo` -> `X:/foo` rewrite is applied in amigit itself rather
  than patched into libgit2. This keeps `lib/libgit2/` upstream
  source frozen. Any future libgit2-consumer port that uses
  volume-rooted paths must apply the same rewrite or reference
  `amigit_resolve_repo_path()`.
- **Empty initial commits rejected.** `amigit commit -m foo` in a
  brand-new repo with zero staged files fails with RC=10 and the
  message "nothing to commit". Upstream git rejects this too unless
  `--allow-empty` is passed; amigit v1 does not expose that flag.

## Shim dependencies

Beyond the standard `-lamiport` link, amigit pulls in:

- `amiport_getopt_long` (used by future command option parsing; not
  yet called but headers are pre-wired)
- File I/O via libnix native (libgit2 uses libnix for open/read/write)
- `pr_WindowPtr` direct manipulation via `proto/exec.h` + `proto/dos.h`
  (not a shim call, but uses AmigaOS types from dos/dosextens.h)

Link-time stubs in `ported/amigit_libgit2_stubs.c` cover the gaps in
libnix and the pruned libgit2 build:

| Symbol | Why stubbed |
|---|---|
| `strnlen` | POSIX.1-2008, missing from libnix |
| `difftime` | C89 standard, missing from libnix |
| `select` | `posix.c` p_poll() path; not called at runtime |
| `git_remote_*` | `remote.c` excluded from libgit2 build |
| `git_clone__submodule` | `clone.c` excluded; submodule.c references it |
| `git_failalloc_*` | `failalloc.c` not in `allocators/` dir |
| `git_socket_stream__connect_timeout/timeout` | excluded transports |
| `__divsf3` | FS-UAE ROM mathieeesingbas crash workaround |
| `__floatunsisf` | FS-UAE ROM mathieeesingbas crash workaround |

This file is a near-verbatim copy of the stub block in
`tests/libgit2/test_libgit2.c`, plus the Phase 3c soft-float
overrides. When libgit2's pruning configuration changes in Phase 4
(e.g. adding network transports), both files will need updating.

## References

- PDR-010 `docs/pdr/010-amigit-on-libgit2.md` -- why libgit2
- PDR-010a `docs/pdr/010a-amigit-cli-spec.md` -- v1 CLI surface
- Phase 2 audit reports:
  - `lib/libgit2/PATCHES.md`
  - `lib/libgit2/MEMORY-AUDIT.md`
  - `lib/libgit2/PERF-REPORT.md`
- Test pattern reference: `tests/libgit2/test_libgit2.c`,
  `tests/libgit2/PLAN.md`, `tests/libgit2/Makefile`
- Known pitfalls discovered in this port (added to
  `.claude/rules/known-pitfalls.md` and amiga-kb):
  - "libgit2 Treats AmigaDOS Volume Paths as Relative"
  - "amiport_realpath Must Handle POSIX '.'"
  - "libgit2 patch_generate Triggers FS-UAE mathieeesingbas Crash"
  - "cmd_commit Must Reject Empty Initial Commits Explicitly"
