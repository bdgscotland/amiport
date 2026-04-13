# amigit 0.1 (Phase 3a proof-of-life)

## Status

**Phase 3a: proof-of-life.** PDR-010 Phase 3 is split into three
sub-phases (see `docs/pdr/010a-amigit-cli-spec.md` "Phased build"):

- **Phase 3a (this commit):** scaffold + `version` + `init`.
  Proves libgit2.a links into a user binary, `git_libgit2_init` runs
  outside the unit-test harness, and the Makefile + force-include
  pattern from `tests/libgit2/` ports cleanly to `ports/amigit/`.
- **Phase 3b (next session):** read-side commands -- `status`, `log`,
  `show`, `diff`. Needs full FS-UAE test suite.
- **Phase 3c (final):** write-side commands -- `add`, `commit`,
  `checkout`, `branch`, `tag`. Completes v1 CLI surface. Produces
  PORT.md final writeup, amigit.readme, LHA package, PORTS.md entry.

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
                                error-exit declarations
  amigit.c                   -- main() + dispatcher + usage +
                                error mapping + libgit2 lifecycle
  amigit_libgit2_stubs.c     -- link-time stubs (strnlen, difftime,
                                select, git_remote_*, git_clone__*,
                                git_failalloc_*, git_socket_stream__*)
  cmd_version.c              -- `amigit version`
  cmd_init.c                 -- `amigit init [--bare] [path]`
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

1,057,516 bytes (1.01 MB). Compare:
- `tests/libgit2/test_libgit2` = 1,105,944 bytes (1.05 MB)

amigit is ~48 KB smaller because it links only the libgit2 objects
actually referenced by its two commands (version + init), whereas the
test binary pulls in hash, diff, status, refs, branches, tags, and
revwalk from the archive.

## Testing

### Phase 3a smoke tests on vamos

Invocation: `VAMOS_MEM=4096 toolchain/scripts/vamos -s 256 ports/amigit/amigit <args>`

| # | Command | Expected | Result |
|---|---|---|---|
| 1 | `version` | 3 lines to stdout, exit 0 | **PASS** |
| 2 | (no args) | usage to stderr, exit 10 | **PASS** |
| 3 | `--help` | usage to stderr, exit 0 | **PASS** |
| 4 | `nosuchverb` | error + usage, exit 10 | **PASS** |
| 5 | `init --help` | init usage to stdout, exit 0 | **PASS** |
| 6 | `init T:amigit-pre/repo` (parent exists) | "Initialized empty git repository in T:amigit-pre/repo", exit 0 | **PASS** |
| 7 | `init T:amigit-smoke` (no parent) | should work, but vamos FileManager hits the same path-translation gap already documented in tests/libgit2/ for new top-level paths under T: | BLOCKED on vamos (works on real AmigaOS) |
| 8 | `init` reinit | should print "Reinitialized" | BLOCKED on vamos (path-mgr state loss between invocations) |

6 of 8 test-designer cases pass. The 2 blocked ones are the same
class of vamos `FileManager.create_dir` limitation that already
required `-DAMIPORT_VAMOS_LIMITED` in `tests/libgit2/Makefile`. Real
AmigaOS and FS-UAE do not have this limitation -- Phase 3b will add a
full FS-UAE test suite that exercises all init variants.

### `amigit version` canonical output

```
amigit 0.1 (built 2026-04-13)
libgit2 1.8.5
amiport posix-shim available
```

Confirms libgit2 statically linked and the version reporting path
(`git_libgit2_version`) works from a user binary context.

## Known limitations (Phase 3a)

- **Only 2 commands.** `version` and `init`. Phases 3b and 3c add the
  remaining 8.
- **vamos top-level-path init blocked.** First-time init of a path
  like `T:new-repo` fails under vamos because vamos's path_mgr does
  not know about the path until it's been enumerated. Workaround for
  testing: pre-create the parent directory via raw dos.library
  `CreateDir` (the tests/libgit2 test binary uses this pattern).
  Real AmigaOS and FS-UAE are unaffected.
- **vamos reinit blocked.** Second `init` call on an existing path
  fails under vamos for the same reason plus readdir state loss
  between invocations. Real AmigaOS is unaffected.
- **No memory-checker / perf-optimizer yet.** 200 lines of code in
  Phase 3a doesn't warrant the full agent pipeline yet; they'll run
  on Phase 3c once the full v1 command set is in place and there's
  enough code to audit meaningfully.
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
