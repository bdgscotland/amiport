# amigit 0.1 (Phase 3b: read-side commands)

## Status

**Phase 3b: read-side commands complete.** PDR-010 Phase 3 is split
into three sub-phases (see `docs/pdr/010a-amigit-cli-spec.md` "Phased
build"):

- **Phase 3a (shipped):** scaffold + `version` + `init`.
  Proved libgit2.a links into a user binary, `git_libgit2_init` runs
  outside the unit-test harness, and the Makefile + force-include
  pattern from `tests/libgit2/` ports cleanly to `ports/amigit/`.
- **Phase 3b (this commit):** read-side commands -- `status`, `log`,
  `show`, `diff`. Full 44-test FS-UAE test suite green. Discovered
  and worked around two libgit2 AmigaOS path-handling bugs (see
  "Path handling" below). Added `amigit_resolve_repo_path()` helper
  as the single choke point for path normalization, and patched
  `amiport_realpath` in the shim to handle POSIX `"."`.
- **Phase 3c (final):** write-side commands -- `add`, `commit`,
  `checkout`, `branch`, `tag`. Completes v1 CLI surface. Produces
  PORT.md final writeup, amigit.readme, LHA package, PORTS.md entry,
  mandatory memory-checker + perf-optimizer runs.

## Upstream source

**None.** amigit is an amiport-native CLI written from scratch on top
of `lib/libgit2/libgit2.a`. Real git is structurally infeasible on
68k AmigaOS 3.x because its command dispatch is built on
`fork()`/`execvp()` with 53 call sites in `run-command.c` alone -- see
PDR-010 for the full analysis and the reason libgit2 is the chosen
path. The `original/` directory exists in the port-directory-hygiene
rule but is empty for amigit. Every source file in `ported/` is
hand-written.

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
                                git_failalloc_*, git_socket_stream__*)
  cmd_version.c              -- `amigit version`
  cmd_init.c                 -- `amigit init [--bare] [path]`
  cmd_status.c               -- `amigit status [-s|--short]`
  cmd_log.c                  -- `amigit log [-n N] [--oneline]`
  cmd_show.c                 -- `amigit show <ref>`
  cmd_diff.c                 -- `amigit diff [--cached|--staged]`
```

Each subcommand lives in its own translation unit. New commands are
added by:

1. Create `ported/cmd_<name>.c` with `int amigit_cmd_<name>(int argc, char **argv)`.
2. Add `extern` declaration to `ported/amigit.h`.
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
- `-Wno-unused-parameter -Wno-unused-function` -- libgit2 headers and
  the stub file generate these; they're not actionable.

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

Phase 3b: 1,071,256 bytes (1.02 MB). Compare:
- Phase 3a: 1,057,516 bytes (2 commands)
- `tests/libgit2/test_libgit2`: 1,105,944 bytes (1.05 MB, all APIs)

The Phase 3b delta (~13.4 KB for 4 new commands) is modest because
`status`, `log`, `show`, and `diff` pull in revwalk, diff, and status
machinery that libgit2's init code already transitively referenced
via its internal wiring. Each additional command TU adds only the
argparse + the thin libgit2 dispatch.

## Path handling

AmigaOS AmigaDOS paths and libgit2's POSIX-centric path logic have
two incompatibilities that required workarounds:

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
   FS-UAE test harness wrapper `test-amigit-inrepo.rexx` applies
   the same `"X:foo"` -> `"X:/foo"` rewrite before `CD`, keeping
   the storage path convention consistent between amigit and the
   test harness.

`amigit_resolve_repo_path()` is the single choke point for path
normalization. Every command that hands a user-supplied path to
libgit2 open/init funnels it through this helper:

```c
char resolved[256];
if (amigit_resolve_repo_path(in_path, resolved, sizeof(resolved))
        != RETURN_OK) {
    /* handle error */
}
rc = git_repository_open_ext(&repo, resolved, ..., NULL);
```

`amigit init` uses the helper on the user-supplied path. The other
read-side commands (`status`, `log`, `show`, `diff`) pass `"."` and
rely on the helper's CWD-resolution path.

## Testing

### Phase 3b FS-UAE test suite

`ports/amigit/test-fsemu-cases.txt` -- 44 TEST blocks covering all 6
commands shipped so far. Run with:

```
make test-fsemu TARGET=ports/amigit
```

Coverage breakdown:

| Command | Happy path | Flag parsing | Error paths | Total |
|---|---|---|---|---|
| `version` | 3 | 0 | 1 | 4 |
| top-level dispatch | 2 | 2 | 2 | 6 |
| `init` | 4 | 3 | 1 | 8 |
| `status` | 2 | 3 | 2 | 7 |
| `log` | 1 | 2 | 3 | 6 |
| `show` | 0 | 2 | 3 | 5 |
| `diff` | 3 | 2 | 2 | 7 |
| Amiga-specific | 2 | 0 | 0 | 2 |
| Stress/real-world | 4 | 0 | 0 | 4 |

**Result: 44/44 passing on FS-UAE.** No weakened EXPECT assertions.

Phase 3b has no `add`/`commit` so "happy path" for `log`, `show`, and
`diff` is limited to empty-repo behavior (log exits 0 with no output
on unborn HEAD; diff exits 0 with no deltas; show HEAD exits 10 on
unborn HEAD). Commit-content happy paths are deferred to Phase 3c.

### Test harness wrapper

For commands that open the repo at `"."` (status/log/show/diff), the
test harness cannot CD between TEST blocks. `test-amigit-inrepo.rexx`
wraps each such test: it builds an AmigaDOS Execute script that `CD`s
into the target repo and then runs `WORK:amigit <subcmd>`, reading
the captured output back into ARexx via `OPEN`/`READLN` so the
test-runner sees the expected stdout. The wrapper applies the same
`"X:foo"` -> `"X:/foo"` rewrite as cmd_init.c so AmigaDOS's CD finds
the same directory libnix created.

Tests that only parse flags (`--help`, unknown flags, no-arg errors)
do NOT need the wrapper -- they short-circuit before the repo open
call and run directly via `WORK:amigit <subcmd> --help`. This halves
the FS-UAE process churn compared to routing everything through the
wrapper (which hung at ~24 tests due to resource exhaustion).

### `amigit version` canonical output

```
amigit 0.1 (built 2026-04-13)
libgit2 1.8.5
amiport posix-shim available
```

Confirms libgit2 statically linked and the version reporting path
(`git_libgit2_version`) works from a user binary context.

## Known limitations (Phase 3b)

- **6 of 10 commands implemented.** `version`, `init`, `status`,
  `log`, `show`, `diff`. Phase 3c adds `add`, `commit`, `checkout`,
  `branch`, `tag`.
- **No commit-content happy-path tests for log/show/diff.** Requires
  `add`+`commit` which land in Phase 3c. The Phase 3b test suite
  exercises flag parsing, error paths, and empty-repo behavior for
  these commands, but cannot yet verify "log shows commit SHAs" or
  "show HEAD prints diff content".
- **Path normalization is amigit-side, not libgit2-side.** The
  `X:foo` -> `X:/foo` rewrite is applied by `amigit_resolve_repo_path`
  in amigit itself rather than patched into libgit2. This keeps
  `lib/libgit2/` upstream source frozen. Any future port that
  consumes `libgit2.a` with volume-rooted paths must apply the same
  rewrite or reference this helper.
- **No memory-checker / perf-optimizer yet.** ~600 lines of code in
  Phase 3b still under the "defer to 3c" rule -- the final v1 will
  run both agents before release.
- **No Aminet readme, no LHA package.** Deferred to Phase 3c final.
- **No catalog or site entries.** Deferred to Phase 3c final.

## Shim dependencies

Beyond the standard `-lamiport` link, amigit pulls in:
- `amiport_getopt_long` (used by future command option parsing; not
  yet called in Phase 3a but headers are pre-wired)
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
| `git_remote_*` | `remote.c` excluded from libgit2 build; branch/submodule/repository objects reference the symbols |
| `git_clone__submodule` | `clone.c` excluded; submodule.c references it |
| `git_failalloc_*` | `failalloc.c` not in `allocators/` dir; alloc.o table has function pointers |
| `git_socket_stream__connect_timeout`, `git_socket_stream__timeout` | extern ints in `settings.c`, defined in excluded transports |

This file is a near-verbatim copy of the stub block in
`tests/libgit2/test_libgit2.c`. When libgit2's pruning configuration
changes in Phase 4 (e.g. adding network transports), both files
will need updating.

## References

- PDR-010 `docs/pdr/010-amigit-on-libgit2.md` -- why libgit2
- PDR-010a `docs/pdr/010a-amigit-cli-spec.md` -- v1 CLI surface
- Phase 2 audit reports:
  - `lib/libgit2/PATCHES.md`
  - `lib/libgit2/MEMORY-AUDIT.md`
  - `lib/libgit2/PERF-REPORT.md`
- Test pattern reference: `tests/libgit2/test_libgit2.c`,
  `tests/libgit2/PLAN.md`, `tests/libgit2/Makefile`
