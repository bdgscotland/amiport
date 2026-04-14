# Port: which

## Overview

| Field | Value |
|-------|-------|
| Program | which |
| Version | 1.27 (port revision: 1) |
| Source | OpenBSD usr.bin/which v1.27 |
| Category | 1 -- CLI |
| License | ISC |
| Original Author | Todd C. Miller |
| Port Date | 2026-03-26 |
| Binary Size | 37 KB |
| Source Files | 1 (which.c) |

## Description

Locate a program file in the user's PATH. Given one or more program names,
which searches each PATH entry for an executable file matching that name and
prints the first match (or all matches with `-a`). Also serves as `whereis`
when invoked under that name (uses a fixed default path instead of `$PATH`).

## Prior Art on Aminet

No existing which port found on Aminet for AmigaOS 3.x. Standard AmigaOS
ships a `Which` command that scans the C: assign and reports a single match
in AmigaDOS path style, but it does not understand a Unix-style `PATH`
environment variable nor support multiple matches via `-a`. This port
provides POSIX-style PATH semantics adapted to AmigaDOS path conventions.

## Portability Analysis

Verdict: **PORTABLE with design adaptation** -- All POSIX dependencies are
Tier 1, but the port required a non-trivial design adaptation for the
AmigaDOS PATH separator. AmigaDOS uses `:` both as the volume name suffix
(`C:`, `WORK:`) AND would naively be the POSIX PATH separator -- the two
collide. The port resolves this by using `;` as the PATH separator on
AmigaOS and joining volume-suffixed entries directly (`C:` + `Sort` =
`C:Sort`) versus path-suffixed entries with `/` (`SYS:Utilities` + `prog`
= `SYS:Utilities/prog`).

| Issue | Tier | Resolution |
|-------|------|------------|
| `<sys/stat.h>` | Tier 1 | `<amiport/sys/stat.h>` -- amiport_stat |
| `<sys/sysctl.h>` | Removed | Not available, not needed |
| `<err.h>` | Tier 1 | `<amiport/err.h>` |
| `<paths.h>` | Removed | Replaced with local `_PATH_STDPATH` / `_PATH_DEFPATH` defines |
| `<stdlib.h>` | Tier 1 | `<amiport/stdlib.h>` exit() macro |
| `<unistd.h>` | Tier 1 | `<amiport/unistd.h>` -- access, getenv |
| `<getopt.h>` | Tier 1 | `<amiport/getopt.h>` (libnix getopt broken) |
| `<pwd.h>` / `<grp.h>` | Tier 1 | `<amiport/pwd.h>` / `<amiport/grp.h>` -- setuid/setgid stubs |
| `getenv("PATH")` | Tier 1 | `amiport_getenv()` -- returns malloc'd string (must free) |
| Wildcard expansion | Tier 1 | `amiport_expand_argv()` |
| `pledge()` / `unveil()` | Stub | `#define ... (0)` -- no kernel sandbox |
| `__dead` attribute | Removed | OpenBSD-specific compiler attribute, not needed |
| **PATH separator `:` collision** | **DESIGN** | Use `;` as separator on AmigaOS; treat `X:` directly |
| Exit codes | Tier 1 | POSIX 0/1/2 -> Amiga 0/5/20 (RETURN_OK / RETURN_WARN / RETURN_FAIL); usage = 10 |

## Transformations Applied

| Line(s) | Original | Transformed | Comment |
|---------|----------|-------------|---------|
| 20 | `<sys/stat.h>` | `<amiport/sys/stat.h>` | stat() macro |
| 21 | `<sys/sysctl.h>` | (removed) | Not available on AmigaOS |
| 24 | `<err.h>` | `<amiport/err.h>` | libnix has no err.h |
| 27 | `<paths.h>` | (removed) | Not available |
| 30 | `<stdlib.h>` | `<amiport/stdlib.h>` | exit() macro |
| 33 | `<unistd.h>` | `<amiport/unistd.h>` | access(), getenv |
| 35 | (added) | `<amiport/glob.h>` | argv expansion |
| 37 | (added) | `<amiport/getopt.h>` | libnix getopt broken |
| 39 | (added) | `<amiport/pwd.h>` | setuid/geteuid stubs |
| 41 | (added) | `<amiport/grp.h>` | setgid/getegid stubs |
| 44 | (added) | `static const char *verstag = "$VER: which 1.27 (26.03.2026)";` | AmigaOS version string |
| 47 | (added) | `LONG __stack = 16384;` | Stack cookie |
| 55-56 | (added) | `#define _PATH_STDPATH "C:"` / `#define _PATH_DEFPATH "C:"` | Local replacements for paths.h |
| 59-60 | (added) | `#define pledge(p, e) (0)` / `#define unveil(p, f) (0)` | Stubs |
| 68 | `__dead static void usage(void)` | `static void usage(void)` | Removed __dead |
| 82-87 | (added) | `static void cleanup(void)` | atexit handler |
| 98 | (added) | `amiport_expand_argv(&argc, &argv);` | AmigaDOS does not glob |
| 100 | (added) | `atexit(cleanup);` | Cleanup on err()/errx() paths |
| 122-132 | `path = getenv("PATH");` then literal use | `path_alloc = amiport_getenv("PATH"); ... path = path_alloc;` | Track malloc'd PATH for free() |
| 138-140 | `err(1, ...)` (setgid/setuid) | `err(10, ...)` | RETURN_ERROR |
| 142-143 | `err(1, "pledge")` | `err(20, "pledge")` | RETURN_FAIL (pledge is stubbed) |
| 150-151 | (added) | `if (path_alloc != NULL) free(path_alloc);` | Free PATH from amiport_getenv |
| 153-155 | `return notfound == 0 ? 0 : (notfound == argc ? 2 : 1);` | `return notfound == 0 ? 0 : (notfound == argc ? 20 : 5);` | POSIX 0/1/2 -> Amiga 0/5/20 |
| 163 | `struct stat sbuf;` | `struct amiport_stat sbuf;` | Use amiport struct directly |
| 167 | `if (strchr(prog, '/'))` | `if (strchr(prog, '/') || strchr(prog, ':'))` | Detect AmigaDOS volume paths as direct |
| 181 | `err(1, "strdup")` | `err(10, "strdup")` | RETURN_ERROR |
| 188 | `strsep(&pathcpy, ":")` | `strsep(&pathcpy, ";")` | **Key design fix**: AmigaDOS PATH separator is `;` |
| 189-190 | (added) | `if (*p == '\0') continue;` | Skip empty entries from `;;` |
| 192-202 | `snprintf(filename, ..., "%s/%s", p, prog)` | volume-aware join | `X:` -> `X:prog`, `dir` -> `dir/prog`, with `/` stripping |
| 232 | `exit(1)` | `exit(10)` | RETURN_ERROR for usage |

## Shim Functions Exercised

- `amiport_expand_argv()` / `amiport_free_argv()` -- argv wildcard expansion
- `amiport_getenv()` -- PATH lookup (returns malloc'd, freed by caller)
- `amiport_getopt()` (via getopt macro), `amiport_optind`
- `amiport_stat()` (via stat macro)
- `amiport_access()` (via access macro)
- `setuid` / `setgid` / `geteuid` / `getegid` stubs (no-op on single-user AmigaOS)
- libnix native: `strchr`, `strsep`, `strdup`, `snprintf`, `puts`, `fprintf`, `strcmp`, `strlen`, `free`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo, GCC 6.5.0b) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | inherited from common.mk (`-noixemul -m68000`) |
| Stack cookie | 16384 bytes |
| Link libraries | `-lamiport` |
| Binary size | 37 KB (37620 bytes) |

## Test Results

- **Total tests:** 23
- **Suite:** test-fsemu-cases.txt (uses ARexx wrapper scripts to set ENV:PATH)
- **Test wrapper scripts:**
  - `test-which-path.rexx` -- sets `ENV:PATH="C:;WORK:"`, runs which, cleans up
  - `test-which-nopath.rexx` -- ensures `ENV:PATH` unset, exercises default fallback to `C:`
  - `test-which-slash.rexx` -- exercises trailing slash stripping in PATH entries
- **Categories covered:**
  - Functional / per-flag (3) -- find via PATH, `-a` single-match, no-flag first-match
  - Error paths (4) -- no args, invalid flag, command not found, direct-path missing
  - Exit code matrix (1) -- RC=5 mixed found/not-found case
  - Edge cases (4) -- default PATH fallback, missing PATH, multiple-not-found, trailing slash
  - Amiga-specific (5) -- direct path with `:`, direct path with `/`, FFS case-insensitive,
    multiple commands via PATH, WORK: direct path
  - Real-world / stress (6) -- find ported tool, find multiple ported tools,
    `-a` with single C: match, five-command PATH lookup, five-command direct lookup,
    precision on `-a` output format

### Flag Matrix Coverage

| Flag | Tested | Notes |
|------|--------|-------|
| `-a` | YES | Single match (only one match exists), and exact output format |
| Bare command | YES | PATH search + direct path detection |
| Multiple commands | YES | `-a` and bare modes |
| Volume direct path (`C:Sort`) | YES | Bypasses PATH search |
| Slash direct path (`WORK:which`) | YES | Treated as direct (`:` triggers direct mode) |

### Exit Code Matrix Coverage

| RC | Meaning | Tested |
|----|---------|--------|
| 0 | All found | YES |
| 5 | Some found, some not (RETURN_WARN) | YES |
| 10 | Bad arguments / unknown flag (RETURN_ERROR) | YES |
| 20 | None found (RETURN_FAIL) | YES |

## Memory Safety

**Verdict: CLEAN.**
- `amiport_getenv("PATH")` returns a malloc'd string (unlike libnix
  native getenv which returns a static pointer). The port tracks this in
  `path_alloc` and calls `free(path_alloc)` before normal exit.
- Inside `findprog()`, `strdup(path)` is freed on every return path
  (single-match return, all-match completion, error returns).
- `amiport_free_argv()` runs in atexit cleanup, covering all
  `err()`/`errx()` and direct `exit()` paths.
- `filename[PATH_MAX]` is a stack-local buffer, sized via `PATH_MAX`
  (typically 1024) -- well within the 16384-byte stack cookie even with
  AmigaOS hidden depth.
- No growth buffers, no linked lists, no shared global allocations.

**One subtle correctness fix:** the original code uses `path = getenv("PATH")`
and then assigns `_PATH_DEFPATH` to the same `path` variable on fallback.
Because `amiport_getenv()` returns malloc'd memory, the port introduces a
separate `path_alloc` to track ownership: assignments to `path` are pointer
copies that don't affect ownership; the free runs only against
`path_alloc`. If `amiport_getenv` returned non-NULL but empty (the vamos
edge case -- see known-pitfalls "vamos GetVar() Returns 0 for Missing
Variables"), the port still frees it before assigning the literal default.

## Performance Notes

- Each PATH lookup is O(entries in PATH) `stat()` + `access()` calls.
  On AmigaOS each `stat()` is a `Lock()` + `Examine()` + `UnLock()`
  sequence (BPTR/BCPL traffic). For typical PATHs of 2-5 entries this
  is well under 100 ms even on a stock 7 MHz 68000.
- The `strdup(path)` on each `findprog()` call is a small per-program
  allocation (PATH is typically <100 bytes). Not a hot path.
- No fgetc-style hot loops -- this program does not read file contents,
  only file metadata.

## Platform Compatibility Notes

- **AmigaDOS PATH separator design (load-bearing):** The port uses `;` as
  the PATH separator instead of POSIX `:`. This is necessary because `:`
  is the volume name suffix on AmigaDOS, and `strsep(path, ":")` on a
  PATH entry like `C:` would split it into `C` (a relative directory
  reference, which is wrong) plus an empty string. The fix is documented
  in known-pitfalls "AmigaDOS PATH Separator Is Colon -- Conflicts with
  Volume Names" and was first discovered during this port (2026-04-11).
- **Volume-aware join:** When a PATH entry ends with `:` (volume name
  like `C:`), the port joins directly: `C:` + `Sort` = `C:Sort`. When
  it ends with anything else (a directory like `SYS:Utilities`), it
  joins with `/`: `SYS:Utilities` + `prog` = `SYS:Utilities/prog`.
- **AmigaDOS case-insensitive lookup:** OFS/FFS/SFS are
  case-insensitive, so `which Sort` and `which sort` find the same
  file. The port returns the path as constructed (PATH entry +
  requested name) -- matching the user's case, not the on-disk case.
- No custom allocators (crash-patterns #15 N/A) and no struct-by-value
  returns (crash-patterns #16 N/A).

## Known Limitations

- **No environment expansion of PATH entries:** PATH entries are used
  literally. Constructs like `$HOME/bin` are not expanded.
- **`whereis` mode is mode-only, not a separate binary:** The port does
  not install a `whereis` symlink/copy. To use whereis semantics, rename
  the binary to `whereis` -- the program switches behavior based on
  `__progname`. (This is the upstream OpenBSD design.)
- **PATH separator is `;`, not `:`:** Users coming from POSIX systems
  may expect `:` to separate PATH entries. On AmigaOS this would not
  work (see Platform Compatibility above). PATH must be set with `;`
  as separator: `SetEnv PATH "C:;SYS:Utilities;WORK:bin"`.
- **`pledge()`/`unveil()` are stubs:** No kernel sandbox on AmigaOS.
  These macros return success and the actual sandbox is absent. The
  upstream code calls them at startup but does not depend on the
  sandbox for correctness -- only for defense in depth.
- **`setuid`/`setgid`/`geteuid`/`getegid` are no-ops:** AmigaOS has no
  multi-user permission model. The shim returns 0 (success) for all
  four. The original `err()` calls remain in place for correctness but
  will never fire under the shim.

## Pitfalls Addressed

- **AmigaDOS PATH separator collision (KNOWN PITFALL, discovered in this
  port):** Using `;` instead of `:` for PATH parsing. Documented in
  `.claude/rules/known-pitfalls.md` "AmigaDOS PATH Separator Is Colon".
- **Volume-suffixed path joining:** `X:` joins directly with the program
  name, no extra `/`. Detected via `len > 0 && p[len-1] == ':'`.
- **amiport_getenv() returns malloc'd, must free (known-pitfalls):** The
  port tracks `path_alloc` separately from the `path` working pointer
  and frees on exit.
- **vamos GetVar() returns 0 for missing variables (known-pitfalls):**
  The port checks `path_alloc == NULL || *path_alloc == '\0'` and
  frees the empty result before falling back to `_PATH_DEFPATH`.
- **libnix getopt_long broken (crash-patterns #17):** Replaced via
  `<amiport/getopt.h>`.
- **err()/errx() bypass cleanup:** `atexit(cleanup)` registered before
  any error-emitting call.
- **Exit code visibility:** POSIX `0/1/2` remapped to Amiga `0/5/20`
  (RETURN_OK / RETURN_WARN / RETURN_FAIL). Usage error uses 10
  (RETURN_ERROR).
- **AmigaDOS does not glob:** `amiport_expand_argv()` runs at startup so
  callers can pass `which sort grep wc` with wildcards if desired.
