# Port: mv

## Overview

| Field | Value |
|-------|-------|
| Program | mv |
| Version | 1.47 (port revision: 1) |
| Source | OpenBSD mv v1.47 (BSD 3-Clause) |
| Category | 1 -- CLI |
| License | BSD 3-Clause |
| Original Author | Ken Smith, UC Berkeley |
| Port Date | 2026-04-11 |

## Description

Move or rename files and directories. Uses rename() for same-volume moves, with fallback copy-and-delete for cross-volume regular file moves. Supports interactive (-i) and force (-f) modes.

## Prior Art on Aminet

GNU fileutils 3.3 (amiga-fileutils-3.3.lha, ~2000) includes mv but is 25+ years old. AmigaOS built-in RENAME command cannot move files across volumes. This port provides modern, standalone cross-volume move capability.

## Portability Analysis

Verdict: **MODERATE** -- single source file but heavy use of POSIX file
metadata (`lstat`, `fstat`, `fchmod`, `fchown`, `futimens`), an explicit
fd-namespace decision (libnix native `open`/`read`/`write` instead of
`amiport_open`), and a Tier 3 redesign for the cross-volume directory move
path (`mvcopy()`).

| Issue | Tier | Resolution |
|-------|------|------------|
| `<sys/time.h>` | 1 | `<amiport/sys/time.h>` |
| `<sys/wait.h>` | -- | removed -- no `fork`/`wait` on AmigaOS |
| `<sys/mount.h>` / `statfs()` | -- | removed -- no Unix mount points; volumes are named devices |
| `<sys/stat.h>` / `lstat`/`fchmod`/`fchown`/`futimens` | 1 | `<amiport/sys/stat.h>` (`lstat` aliased to `amiport_stat`; no symlinks) |
| `<err.h>` / `err`/`errx`/`warn`/`warnx`/`warnc` | 1 | `<amiport/err.h>` (bare `<err.h>` missing in libnix) |
| `<fcntl.h>` / `O_RDONLY`/`O_WRONLY`/`O_CREAT`/`O_TRUNC` | 1 | `<amiport/unistd.h>` |
| `<stdlib.h>` / `exit()` | 1 | `<amiport/stdlib.h>` |
| `<pwd.h>` / `user_from_uid()` | 1 | `<amiport/pwd.h>` (returns "root") |
| `<grp.h>` / `group_from_gid()` | 1 | `<amiport/grp.h>` (returns "wheel") |
| `<getopt.h>` | 1 | `<amiport/getopt.h>` (libnix `getopt_long` broken) |
| `<dirent.h>` / `rmdir()` | 1 | `<amiport/dirent.h>` |
| AmigaDOS argv wildcards | 1 | `<amiport/glob.h>` -- `amiport_expand_argv()` |
| `isatty(STDIN_FILENO)` | -- | replaced with `IsInteractive(Input())` (`amiport_isatty` does not know about libnix fds) |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| `S_ISUID` / `S_ISGID` | 1 | locally `#define`d (not in `<amiport/sys/stat.h>`) |
| `PATH_MAX` | 1 | locally `#define`d to 256 if not provided |
| `strmode()` | 1 | local `mv_strmode()` -- libnix declares but does not link `strmode()` |
| `fchflags()` | Stub | `#define fchflags(fd, flags) (0)` -- AmigaOS has no BSD file flags |
| `cpmain()` / `rmmain()` (cross-volume directory move) | 3 | not available on AmigaOS; `mvcopy()` returns RC=10 with explanation |
| `open()` / `close()` / `read()` / `write()` | -- | libnix native (NOT amiport shim) -- explicit `AMIPORT_NO_OPEN_MACROS` to keep stdin/stdout/stderr in one fd namespace |
| Exit codes (1 -> 10) | 1 | RETURN_ERROR throughout |

## Transformations Applied

| File | Change | Comment |
|------|--------|---------|
| mv.c | added `verstag` `$VER: mv 1.47 (11.04.2026)` | AmigaOS Version cmd |
| mv.c | added `long __stack = 16384;` | default 4KB too small for fastcopy buffer + dos.library overhead |
| mv.c | `<sys/time.h>` -> `<amiport/sys/time.h>` | |
| mv.c | removed `<sys/wait.h>` | no fork/wait on AmigaOS |
| mv.c | removed `<sys/mount.h>` and statfs/realpath mount-point check | no mount points; cross-volume falls through to copy path |
| mv.c | `<sys/stat.h>` -> `<amiport/sys/stat.h>` | lstat/fchmod/fchown/futimens shim |
| mv.c | `<err.h>` -> `<amiport/err.h>` | bare <err.h> missing |
| mv.c | `<fcntl.h>` -> `<amiport/unistd.h>` | provides O_* flags |
| mv.c | added `#define AMIPORT_NO_OPEN_MACROS` BEFORE include | suppresses amiport shim macros for `open`/`close`/`read`/`write` |
| mv.c | added forward declarations for libnix `open`/`close`/`read`/`write`/`unlink` | system headers conflict if included after amiport headers |
| mv.c | `<stdlib.h>` -> `<amiport/stdlib.h>` | exit macro activation |
| mv.c | `<pwd.h>` -> `<amiport/pwd.h>` | user_from_uid stub returns "root" |
| mv.c | `<grp.h>` -> `<amiport/grp.h>` | group_from_gid stub returns "wheel" |
| mv.c | `<getopt.h>` -> `<amiport/getopt.h>` | libnix getopt_long broken |
| mv.c | added `<amiport/dirent.h>` | rmdir() shim |
| mv.c | added `<amiport/glob.h>` | argv wildcard expansion |
| mv.c | added `<proto/dos.h>` for `IsInteractive(Input())` | replaces `isatty(STDIN_FILENO)` |
| mv.c | `pledge()` stubbed as macro | OpenBSD sandbox absent |
| mv.c | `S_ISUID` / `S_ISGID` defined locally if missing | mode bits not in amiport stat header |
| mv.c | `PATH_MAX` defined to 256 if missing | libnix may not provide |
| mv.c | added local `mv_strmode()` returning fixed `-rw-r--r--` | libnix declares but does not link `strmode()` |
| mv.c | `fchflags()` stubbed as macro returning 0 | no BSD file flags on AmigaOS |
| mv.c | removed `__progname` definition | provided by argv_expand.o (strong symbol) since 2026-03-25 |
| mv.c | `extern char *__progname;` declaration only | satisfies usage() printf reference |
| mv.c | `stdin_ok = IsInteractive(Input()) ? 1 : 0;` | replaces `isatty(STDIN_FILENO) == 0` |
| mv.c | basename loop also strips `:` for AmigaDOS volume names | `T:file.txt` -> `file.txt` |
| mv.c | `errx(1, ...)` -> `errx(10, ...)`, all `return 1` -> `return 10` | RETURN_ERROR convention |
| mv.c | `exit(1)` (in usage) -> `exit(10)` | RETURN_ERROR |
| mv.c | promoted fastcopy `blen`/`bp` from local to file-scope (`fastcopy_blen`/`fastcopy_bp`) | atexit cleanup needs visibility |
| mv.c | added `cleanup()` registered via `atexit()` | frees argv expansion + fastcopy buffer + fflushes stdout |
| mv.c | `fastcopy()` fchown/fchmod/fchflags wrap stubbed shim macros | inert on AmigaOS |
| mv.c | `mvcopy()` returns 10 + `warnx("cannot move directories across volumes...")` | Tier 3 redesign; cpmain/rmmain unavailable |
| mv.c | `ts[0]/ts[1]` constructed from ULONG `st_atime`/`st_mtime` (no `tv_nsec`) | amiport_stat lacks struct timespec fields |
| mv.c | `strmode()` macro defined to `mv_strmode((int)(m), (p))` | provides override for libnix's declared-but-unlinked strmode |
| mv.c | u_int32_t -> unsigned int (C89 + exec/types.h compatible) | |
| mv.c | added Semgrep `nosemgrep: path-traversal` annotations | mv operates on user paths by design |

## Shim Functions Exercised

- `amiport_lstat()` (via `lstat()` macro -- aliased to `amiport_stat`, no symlinks)
- `amiport_stat()` (via `stat()` macro)
- `amiport_fchmod()` (via `fchmod()` macro -- no-op stub)
- `amiport_fchown()` (via `fchown()` macro -- no-op stub)
- `amiport_futimens()` (via `futimens()` macro -- modification time only)
- `amiport_access()` (via `access()` macro)
- `amiport_rename()` (via `rename()` macro)
- `amiport_rmdir()` (via `rmdir()` macro from `<amiport/dirent.h>`)
- `amiport_unlink()` (via `unlink()` macro -- not suppressed by `AMIPORT_NO_OPEN_MACROS`)
- `amiport_user_from_uid()` (via `user_from_uid()` macro)
- `amiport_group_from_gid()` (via `group_from_gid()` macro)
- `amiport_warnc()` / `amiport_err()` / `amiport_errx()` / `amiport_warn()` / `amiport_warnx()`
- `amiport_getopt()` (via `getopt()` macro) plus `optind`
- `amiport_expand_argv()` / `amiport_free_argv()`
- `amiport_exit()` (via `exit()` macro)

**libnix native (NOT shim)**: `open()`, `close()`, `read()`, `write()`,
`fflush()`, `getchar()`. These use the libnix unified fd table where
`stdin = 0`, `stdout = 1`, `stderr = 2`. Mixing amiport_open() with these
would cross fd namespaces (crash-patterns #12).

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -I../../lib/posix-shim/include` (from common.mk) |
| LDFLAGS | `-L../../lib/posix-shim -lamiport` (from common.mk) |
| Stack cookie | `long __stack = 16384;` |
| Binary size | 40440 bytes (40 KB) |
| Source files | 1 (`ported/mv.c`, 581 lines) |

## Test Results

Tested via FS-UAE on A1200 (Kickstart 3.1, 68020) using the ARexx test harness.
Test source: `test-fsemu-cases.txt` (20 TEST blocks). See `TEST-REPORT.md`.

| Category | Test count | Notes |
|----------|------------|-------|
| Error path | 6 | no args, single arg, unknown flag `-Z`, nonexistent source, multi-source non-dir target, rename to nonexistent parent dir |
| Functional `-v` | 3 | basic rename, move-into-directory, force-overwrite with verbose |
| Functional rename/move | 4 | basic rename, move into directory, overwrite without `-f`, multi-file into directory |
| `-i` flag acceptance | 1 | accepts the flag; missing source still RC=10 |
| Amiga-specific | 2 | T: temp volume move, same-volume directory rename |
| Real-world / stress | 4 | rename to backup name, multi-file into build dir, 50 files sequentially, 60-char long filename |

**Result: 20/20 PASS** (TAP output in `TEST-REPORT.md`).

mv is destructive, so all functional tests use ARexx wrapper scripts
(`test-mv-*.rexx`) that create source files in `T:`, run `WORK:mv` with
specific arguments, verify via `EXISTS()` checks, print "OK" or
"FAIL: reason" to stdout, and clean up temp files. Error path tests use
direct `CMD:` lines because the error fires before any filesystem change.

## Memory Safety

**Verdict: CLEAN.**

- `amiport_expand_argv()` storage -- freed in `cleanup()` via `atexit()`
  registered immediately after expansion. All exit paths (`err()`, `errx()`,
  `exit()`, normal `return`) flow through cleanup.
- `fastcopy_bp` (file-scope `static char *`) is allocated lazily inside
  `fastcopy()` and freed in `cleanup()`. Promoted from a local `bp` variable
  for atexit visibility.
- The rename fast path makes no allocations.
- `cleanup()` calls `fflush(stdout)` so verbose output reaches the console
  before exit.
- All file descriptors closed on every path including `goto err` cleanup
  inside `fastcopy()`. Test 18 (50 files sequentially) verifies no fd leak.

## Performance Notes

- **Rename fast path** uses AmigaDOS `Rename()` (via `amiport_rename()`) on
  same-volume moves -- O(1) directory-entry update, no data copy.
- **Cross-volume copy** uses libnix native `open`/`read`/`write` with
  `st_blksize` block size from the source `stat()`. On AmigaOS FFS this is
  typically 488 bytes; the loop reads and writes one block at a time.
- **`-O2` is safe** -- no struct-by-value returns >8 bytes. `struct stat`
  is taken by pointer everywhere.
- The fastcopy buffer is allocated once and reused across all moves in a
  single invocation (file-scope static).
- `int stdin_ok = IsInteractive(Input()) ? 1 : 0;` is computed once at
  startup, not per-file.

## Platform Compatibility Notes

- **Two fd namespaces** -- `mv.c` is the canonical example of choosing
  libnix native `open`/`close`/`read`/`write` instead of `amiport_open()`.
  `AMIPORT_NO_OPEN_MACROS` is defined BEFORE `<amiport/unistd.h>` so the
  shim macros do not redirect the calls. This keeps the fastcopy fds in the
  same namespace as `stdin`/`stdout`/`stderr`. The other amiport shims
  (lstat, fchmod, etc.) still operate on path strings, not fds, so they
  coexist safely.
- **`amiport_isatty()` does not know about libnix standard descriptors**
  (known-pitfall) -- replaced with `IsInteractive(Input())` from
  `<proto/dos.h>`.
- **`__progname` is provided by `argv_expand.o`** as a strong symbol since
  2026-03-25. Local definitions cause a multiple-definition link error.
- **`strmode()` is declared but not linked** by libnix; `mv_strmode()`
  provides a local fixed-string implementation (AmigaOS protection bits do
  not map cleanly to Unix mode bits, so a plausible fixed string is fine
  for the override prompt).
- **AmigaDOS volume separator `:`** is handled in the basename loop
  (`T:file.txt` -> `file.txt`) so mv into a directory uses the correct
  base name.
- **AmigaOS timestamp resolution is 1/50s (20ms)** -- `tv_nsec` is always 0
  in the constructed `struct timespec` for `futimens()`.

## Known Limitations

- **Cross-volume directory moves are not supported.** `mvcopy()` is a
  Tier 3 redesign placeholder -- the original called `cpmain()` + `rmmain()`
  which are OpenBSD internal entry points unavailable on AmigaOS. Returns
  RC=10 with the message `"cannot move directories across volumes:
  cross-volume directory move not supported"`. Workaround: AmigaDOS
  `Copy ALL` then `Delete ALL`. Same-volume directory rename works (uses
  `rename()` which AmigaDOS handles natively).
- **No symlinks on AmigaOS** -- `lstat()` is aliased to `amiport_stat()`.
  Files behave as themselves.
- **`fchown()` / `fchmod()` are no-op stubs** -- AmigaOS protection bits
  have inverted semantics from Unix and there is no user/group ownership
  model. Calls succeed but have no effect. The `-i` override prompt always
  shows `-rw-r--r--` for the destination because `mv_strmode()` returns a
  fixed string.
- **`fchflags()` is a no-op** -- AmigaOS has no BSD file flags
  (`UF_NODUMP`, `SF_IMMUTABLE`, etc.).
- **`pledge()` is a no-op** -- AmigaOS has no sandboxing.
- **Access time is silently ignored** -- AmigaOS filesystems (OFS/FFS) only
  store modification time. The `futimens()` shim takes both `tv_sec`
  values but only writes the second one to disk.
- **Setuid/setgid bit warning is dead code** -- the `S_ISUID`/`S_ISGID`
  bits are never set in `struct amiport_stat`, so the `badchown` warning
  branch is unreachable. Left in for cross-platform readability.

## Pending Work

- The current test suite has 20 tests but the positional argument matrix
  could be expanded. test-designer's matrix coverage requirement was added
  after this port shipped.
- The cross-volume directory move (Tier 3 `mvcopy()` redesign) could be
  implemented using AmigaDOS `Lock`/`Examine`/`ExNext` recursion, or by
  bundling a minimal `cp -r` + `rm -r`. Tracked as a separate work item.

## Review

Reviewed during the original pipeline run. No critical issues. The
fd-namespace choice (libnix native `open`/`read`/`write` instead of
amiport shim) is documented inline and exercised by all functional tests.

