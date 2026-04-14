# Port: patch

## Overview

| Field | Value |
|-------|-------|
| Program | patch |
| Version | 1.78 |
| Source | OpenBSD usr.bin/patch v1.78 (ISC + BSD) |
| Category | 1 — CLI |
| License | ISC + BSD (OpenBSD contributors) |
| Original Author | Larry Wall, OpenBSD contributors |
| Port Date | 2026-03-23 |

## Description

patch takes a patch file containing a difference listing produced by diff and applies those differences to one or more original files, producing patched versions. Supports unified, context, normal, and ed-style diffs. This is the first 68k AmigaOS port of patch — no standalone patch utility previously existed on Aminet.

## Prior Art on Aminet

- No Unix-style patch utility found on Aminet (Stage 0 research, 2026-03-22)
- `gpatch.lha` (2002) is a binary patcher tool (GCompare/GCompatch), NOT a diff-applying utility

## Portability Analysis

Verdict: MODERATE — 12 Tier 1 issues, 1 Tier 2 (mmap with graceful fallback), 0 Tier 3.

| Issue | Tier | Resolution |
|-------|------|------------|
| stat/fstat/lseek/open/read/write/close/unlink/rename/mkdir/chdir/chmod | 1 | amiport shim wrappers |
| pledge/unveil | 1 | macro stubs returning 0 |
| signal(SIGHUP) | 1 | compiles but ignored (only SIGINT works) |
| getline | 1 | amiport_getline via stdio_ext.h |
| fgetln | 1 | replaced with fgets + static buffer |
| stdbool.h | 1 | manual bool/true/false defs |
| paths.h | 1 | AmigaOS path equivalents (T:, *, NIL:) |
| sys/queue.h | 1 | vendored BSD LIST macros |
| d_namlen | 1 | replaced with strlen(d_name) |
| __dead attribute | 1 | __attribute__((noreturn)) |
| getopt.h | 1 | amiport/getopt.h (libnix getopt_long is broken, crash-patterns #16) |
| %lld format | 1 | cast to long, use %ld |
| dirname() | 1 | removed dead unveil block (dirname corrupts input, crash-patterns #17) |
| mmap/munmap/madvise | 2 | amiport-emu mmap (plan_b fallback exists) |

## Transformations Applied

Multi-file port (7 .c files, 6 .h files + 2 vendored headers — common.h, ed.h,
inp.h, pch.h, util.h, backupfile.h plus vendored queue.h and amiga_paths.h).
Every transformation is annotated with an `/* amiport: ... */` comment in the
source. Summary:

| Site | File / Line | Original | Transformed | Reason |
|------|-------------|----------|-------------|--------|
| Stack cookie + $VER | patch.c:64-67 | (none) | `__stack = 65536`, verstag | Recursive parse, deep call chains |
| `__progname` | patch.c:71 | weak symbol from libc | `char *__progname = "patch";` | crash-patterns "weak-symbol stripped" |
| pledge/unveil | patch.c:74-75 | OpenBSD syscalls | `#define ... (0)` macros | Not on AmigaOS; calls become no-ops |
| `__dead` attribute | patch.c:78 | OpenBSD `__dead` | `__attribute__((noreturn))` | bebbo-gcc convention |
| `<getopt.h>` | patch.c:36 | system getopt | `<amiport/getopt.h>` | libnix `getopt_long` broken (crash-patterns #17 / known-pitfalls "libnix getopt_long Is Broken") |
| `<paths.h>` | patch.c:42, util.c:34 | system paths.h | vendored `amiga_paths.h` | `_PATH_TMP` -> `T:`, `_PATH_TTY` -> `*`, `_PATH_DEVNULL` -> `NIL:` |
| Temp file setup | patch.c:206-244 | `_PATH_TMP/patchXXXXXX` | `T:patch[oirp]XXXXXX` with RAM: fallback | Volume-name semantics: no `/` after `:`. Falls through tmpdir candidates if `T:` not assigned. |
| `dirname()` removal | patch.c:292-301 | `unveil(dirname(filearg[0]),...)` | block removed (only the dirname call) | unveil is a no-op on AmigaOS so the whole block is dead; libnix `dirname()` corrupts input (crash-patterns #18 / known-pitfalls "dirname() Corrupts Its Input") |
| usage exit code | patch.c (multiple `my_exit(10)` sites) | `exit(2)` | `exit(10)` | RETURN_ERROR |
| `<sys/mman.h>` | inp.c:29-32 | mmap/munmap/madvise | `<amiport-emu/mmap.h>` | AllocMem+Read emulation, MAP_PRIVATE read-only, no lazy paging |
| `madvise()` | inp.c:57 | mmap hint | `((void)0)` macro | Not available on AmigaOS |
| `EXDEV` fallback | inp.c:60-62, util.c:54 | from errno.h | `#define EXDEV 18` | Some libnix errno.h omissions |
| `SIZE_MAX` fallback | inp.c:44-47, pch.c:36 | from limits.h | `((size_t)-1)` | libnix limits.h omits SIZE_MAX |
| `fgetln()` | inp.c:317-329 | BSD `fgetln(fp, &len)` | static `fgetln_buf[MAXHUNKSIZE+2]` + `fgets()` | Not in libnix |
| `<sys/queue.h>` | ed.c:19, vendored | system LIST macros | local `queue.h` (LIST_HEAD/LIST_INIT/LIST_INSERT_HEAD/etc.) | Not on AmigaOS |
| `<stdbool.h>` | common.h:32 | C99 bool | manual `typedef int bool; #define true 1` | C89 fallback |
| `mode_t`/sig_t | common.h:39, util.c:37 | various typedefs | use libnix's `__mode_t` / amiport `sig_t` | libnix has `mode_t` but not `sig_t` |
| `fstat(fileno(pfp))` | pch.c:128-132 | fd-based fstat | `amiport_stat(filename, &fs)` | crash-patterns #12: fstat crosses libnix/amiport fd namespaces |
| `%lld`/`(long long)` | pch.c:456 | C99 `long long` | `%ld` + `(long)` cast | `off_t` is 32-bit `long` on 68k |
| `d_namlen` | backupfile.c:113 | BSD dirent extension | `strlen(dp->d_name)` | dirent has no d_namlen on AmigaOS |
| `concat()` rename | backupfile.c:44, 165 | `concat(...)` | `backup_concat(...)` | libnix exports `concat()` from string.h |
| Backupfile mkdir | backupfile.c:20 | `<sys/stat.h>` mkdir | via `<amiport/dirent.h>` mkdir macro | Header consolidation |
| `<unistd.h>` | every .c | system unistd | `<amiport/unistd.h>` | shim wrappers |
| open/read/write/close | inp.c, util.c, ed.c | POSIX fd calls | `amiport_open/read/write/close` | Tier 1 shims |
| `unlink/rename/mkdir/chdir/chmod` | util.c, patch.c, mkpath.c | POSIX | `amiport_*` | Tier 1 shims |
| `lseek` | inp.c:436 | POSIX | `amiport_lseek` | Tier 1 shim |
| `signal(SIGHUP)` | util.c:326-343 | POSIX | `amiport_signal` (no-op for SIGHUP) | Only SIGINT (Ctrl-C) is delivered on AmigaOS |
| `getline()` | pch.c:45, patch.c:47 | BSD getline | `amiport_getline` via `<amiport/stdio_ext.h>` | Tier 1 shim |
| `mkstemp()` | patch.c:47 | POSIX | `amiport_mkstemp` | Tier 1 shim |
| `strtonum`, `warn`, `warnc`, `errc` | patch.c:49, util.c | OpenBSD err.h | via `<amiport/err.h>` | OpenBSD compat |
| `recallocarray`, `reallocarray` | patch.c:53, inp.c:53 | OpenBSD libc | `amiport_*` | OpenBSD compat |
| `strlcpy`/`strlcat` | patch.c:53, util.c:46 | BSD libc | via `<amiport/string.h>` | BSD compat |
| `putc()` loop -> `fwrite()` | patch.c (dump_line) | per-char output | bulk fwrite | perf-optimizer: 2-3x speedup on output |
| Tab stop bug | pch.c:1196 | `% 7` (upstream bug) | `% 8` for standard 8-col tabs | upstream defect, fixed in port |
| ttyfd file-scope | util.c:63 | local in `ask()` | static `ask_ttyfd` | so `my_cleanup()` can close it on exit paths |
| `my_cleanup` temp free | util.c:442-447 | (no cleanup) | `free(TMP[OIRP]NAME)` after unlink | -noixemul has no process cleanup |
| `_exit()` -> `exit()` | util.c:473 | `_exit(10)` from sigexit | `exit(10)` | crash-patterns #9: `_exit` debunked, `exit()` is fine and runs atexit cleanup |

## Shim Functions Exercised

POSIX shim (`-lamiport`):
- File I/O: `amiport_open`, `amiport_close`, `amiport_read`, `amiport_write`,
  `amiport_lseek` (inp.c, util.c, ed.c)
- Filesystem metadata: `amiport_stat` (filename-based; never `fstat(fileno())`
  because of fd-namespace crossing -- see crash-patterns #12)
- Filesystem ops: `amiport_unlink`, `amiport_rename`, `amiport_mkdir`,
  `amiport_chdir`, `amiport_chmod`
- Signal: `amiport_signal` (SIGHUP no-op, SIGINT delivers Ctrl-C only)
- Stdio extensions: `amiport_getline`, `amiport_mkstemp` (via
  `<amiport/stdio_ext.h>`)
- getopt: `amiport_getopt_long` (via `<amiport/getopt.h>` -- libnix
  `getopt_long` is broken, see crash-patterns #17)
- BSD compat in `<amiport/string.h>`: `strlcpy`, `strlcat`, `recallocarray`,
  `reallocarray`
- BSD err.h compat in `<amiport/err.h>`: `strtonum`, `warn`, `warnc`, `errc`,
  `err`
- Directory ops: `opendir`/`readdir`/`closedir` (used by `backupfile.c` for
  the numbered-backup search loop)

POSIX emulation (`-lamiport-emu`):
- `amiport_emu_mmap`, `amiport_emu_munmap` (used in inp.c plan_a path -- reads
  entire input file into AllocMem upfront, MAP_PRIVATE read-only only, no
  lazy paging). plan_b fgets-based fallback exists when allocation fails.
- `madvise()` is a no-op macro -- not provided by amiport-emu.

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ (`-m68000` inherited from `common.mk`) |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -std=gnu99 -I../../lib/posix-emu/include` |
| Libraries | `-L../../lib/posix-emu -lamiport-emu -lamiport` |
| Stack cookie | `__stack = 65536L` (recursive parse + memory-heavy) |
| VAMOS_STACK | 256 KB |
| Binary size | 90,148 bytes (88 KB) |
| Sources (7 .c) | patch.c, pch.c, inp.c, util.c, ed.c, backupfile.c, mkpath.c |
| Headers (5 + 2 vendored) | common.h, ed.h, inp.h, pch.h, util.h, backupfile.h + vendored queue.h, amiga_paths.h |

## Test Results

FS-UAE testing: **42/42 passed** (100%).

Test breakdown (`test-fsemu-cases.txt`):

| Category | Count | Coverage |
|----------|-------|----------|
| Functional (Cat 1) | 17 | Every accepted flag has at least one functional test: `-u`, `-c`, `-n`, `-e`, `-R`, `-C`, `-s`, `-i`, `-o`, `-p1`/`-p0`, `-f`, `-t`, `-l`, `-N`, `-F`, `-z`, `-b`, `-r`, `-E`, `--posix`, `-D` |
| Error path (Cat 2) | 8 | Invalid flag (-Z), nonexistent input/patch files, malformed/empty patch, too many args, bad fuzz/strip count |
| Exit code (Cat 3) | 3 | RC=0 success, RC=20 RETURN_FAIL on invalid patch, RC=10 RETURN_ERROR on bad args |
| Edge case (Cat 4) | 7 | Empty target, verbose stdout, p0 exact match, auto-detect of unified/context/normal formats |
| Amiga-specific (Cat 5) | 7 | WORK: volume paths, T: temp output, stdout `-o -` redirect, p1 path stripping with Amiga paths |

Total: **42** TEST blocks. 0 ITEST (Category 1 CLI port -- non-interactive
batch tool. The `-t` batch-mode test deliberately exercises the no-prompt code
path without needing an interactive console.)

Test inputs in port directory: `test-patch-base.txt` (5-line baseline file),
`test-patch-{unified,context,normal,ed}.txt` (the four diff format variants),
`test-patch-patched.txt` (post-patch content for `-R` reverse test),
`test-patch-{p1,whitespace,bad,empty,reverse}.txt` (specialised inputs for
the corresponding flag tests). All inputs live inside `ports/patch/` per
test-hygiene rules.

## Platform Compatibility Notes

- **`fstat(fileno(pfp))` -> `amiport_stat(filename, ...)`** (pch.c:128-132).
  patch was reading the patch file size via `fstat(fileno(pfp))`. `pfp` is a
  libnix `FILE*` so `fileno()` returns a libnix fd. Calling `amiport_fstat()`
  on a libnix fd would return EBADF -- the fd-namespace crossing problem
  (crash-patterns #12). Fix: drop `fstat`/`fileno` and use filename-based
  `amiport_stat()` directly.
- **AmigaOS exclusive write lock for ed-format diffs.** When applying an `-e`
  ed-style diff, patch was attempting to open the output file twice: once via
  `init_output()` and again from inside `write_lines()`. AmigaDOS
  `MODE_NEWFILE` acquires an exclusive lock, so the second `fopen("w")`
  failed with `ERROR_OBJECT_IN_USE`. Fix: close the first handle in
  `init_output()` before re-opening in the ed write path. See known-pitfalls
  "AmigaOS Exclusive Write Lock".
- **`SIZE_MAX` not in libnix limits.h.** Both `inp.c` and `pch.c` define a
  fallback `#define SIZE_MAX ((size_t)-1)`. This is a recurring libnix gap.
- **`EXDEV` not always defined.** Both `inp.c` and `util.c` define a fallback
  `#define EXDEV 18`. patch uses this on the rename failure path.
- **No 68k alignment issues** (crash-patterns #15): patch uses no custom
  allocators that compute alignment from `offsetof()`.
- **No struct-by-value return issues** (crash-patterns #16): all structs are
  passed/returned via pointer.
- **Tab stop bug** (pch.c:1196): upstream had `indent += 8 - (indent % 7)` --
  off-by-one for standard 8-column tabs. Fixed in port. This is an upstream
  defect, not an Amiga-specific issue.

## Memory Safety

Memory-checker (Stage 6b) audit during the original port found and fixed 2
leaks:

1. **Temp file name strings** (util.c:442-447). `TMPOUTNAME`, `TMPINNAME`,
   `TMPREJNAME`, `TMPPATNAME` are `asprintf()`-allocated in `patch.c` during
   temp file setup. AmigaOS with `-noixemul` has no process-exit memory
   reclaim. Fix: `my_cleanup()` now `free()`s each name pointer after the
   corresponding `amiport_unlink()` and sets it to NULL.
2. **TTY fd leak** (util.c:63, 449-453). `ask()` was opening a local tty fd
   that was never closed on FATAL/exit paths. Fix: hoisted to file-scope
   `static int ask_ttyfd = -1;` and closed in `my_cleanup()` before
   `exit()`.

`my_cleanup()` is called from BOTH `my_exit()` (normal exit path) AND
`my_sigexit()` (signal handler path). Both paths use plain `exit(10)` --
NOT `_exit(10)` -- because crash-patterns #9 debunked the "exit hangs" theory
and `exit()` correctly runs atexit handlers.

The mmap emulation (`amiport_emu_mmap` / `amiport_emu_munmap`) is paired
correctly in `re_input()` (inp.c:96-103) and `reallocate_lines()`
(inp.c:138-141). plan_a allocates the entire input file into a single
AllocMem block; failure to grow the line index calls `amiport_emu_munmap()`
to release that block before failing over to plan_b.

## Performance Notes

Two perf-optimizer wins applied during the original port (Stage 6c):

1. **`putc()` loop -> `fwrite()` in `dump_line()`** (patch.c). The original
   code wrote each character to the output file via `putc()` -- one libnix
   stdio call per byte. Replaced with a single `fwrite(line, 1, len, ofp)`
   call. Measured 2-3x speedup on output-heavy patches. Same pattern as the
   libnix `fgetc()` -> `fgets()` win (see known-pitfalls "fgetc() Is 3-5x
   Slower").
2. **Upstream tab-stop bug** (pch.c:1196). `% 7` -> `% 8`. Not a perf win in
   the throughput sense -- it's a correctness fix -- but it was caught by
   the perf review pass that walked the column-counting loops.

`-O2` is safe for patch -- the binary builds cleanly at `-O2` with no
struct-return corruption. patch's structs are all passed by pointer.

## Known Limitations

- Path handling assumes Unix-style paths in diff headers (the standard case). AmigaOS-native paths (DH0:foo/bar) in diff output may not strip correctly with `-p`.
- mmap in plan_a uses amiport-emu which reads entire file into memory. Files larger than available RAM fall through to plan_b (fgets-based) gracefully.
- SIGHUP handler is a no-op — only SIGINT (Ctrl-C) works for interruption.
- Ed-format diffs (`-e`) required closing ofp before write_lines to avoid AmigaOS exclusive lock conflict (ERROR_OBJECT_IN_USE). Fixed.
- pledge/unveil are stubbed as `(0)`. Any patch behaviour that depended on
  filesystem sandboxing on OpenBSD is silently absent on AmigaOS. Not a
  practical issue -- patch's sandboxing is defence in depth, not a
  correctness requirement.
- `--posix` mode compiles and accepts the flag but the strict POSIX path
  has not been exhaustively tested against POSIX 2017 patch semantics. The
  test (Cat 1) only verifies it does not error out.

## Review

- **Memory checker:** 2 leaks found and fixed (TMP name strings + ttyfd in my_cleanup)
- **Perf optimizer:** putc->fwrite in dump_line (2-3x output speedup), tab stop bug fix (% 7 -> % 8)
- **Crash patterns discovered:** #16 (libnix getopt_long broken), #17 (dirname corrupts input)
- **Score: READY** -- 42/42 FS-UAE tests passing, memory leaks fixed, performance optimized

## Revision Candidates

Items flagged for a future revision once shim/tooling improves. **Do not act
on these in this PORT.md edit pass** -- they are notes for a future
`/extend-shim` or pipeline cleanup pass.

1. **Bundled `EXDEV` fallback** (inp.c:60-62, util.c:54). Two source files
   independently define `EXDEV = 18`. amiport's `<amiport/errno.h>` could
   provide it once and these fallbacks could be removed.
2. **Bundled `SIZE_MAX` fallback** (inp.c:44-47, pch.c:36). Same story --
   `<amiport/limits.h>` could supply this once. Two-site duplication is a
   smell.
3. **`fstat()`-on-libnix-fd refactor** (pch.c:128-132). The current fix
   re-stats by filename, which races against any concurrent rename
   (impossible in patch's single-threaded flow but a minor smell). If
   amiport ever provides an `amiport_fstat()` that handles libnix fds
   correctly, this could revert to the upstream `fstat(fileno(pfp))` form.
4. **`<amiport-emu/popen.h>` not used.** patch does not use popen/system at
   runtime -- it has no shell-out. The Makefile pulls in `-lamiport-emu`
   only for `amiport_emu_mmap` / `amiport_emu_munmap`. If amiport ever
   splits the emu library into per-feature archives, patch could link only
   the mmap shim and shave a few KB off the final binary.
5. **`madvise()` no-op macro** (inp.c:57). Could be moved to `<amiport-emu/mmap.h>`
   so every consumer doesn't need to redefine it. Low priority.
6. **`fgetln()` static buffer** (inp.c:317-329). The current fix uses a
   static `MAXHUNKSIZE+2` byte buffer. amiport could provide a real
   `fgetln()` shim using `getline()` underneath. Until then, the static
   buffer is bounded and the worst case (very long patch hunks) is the
   same as upstream.
7. **`__progname` strong symbol** (patch.c:71). Per the updated known-pitfalls
   section "`__progname` -- Do NOT Define in Ported Source", since
   2026-03-25 `argv_expand.o` in `libamiport.a` defines `__progname` as a
   strong symbol. patch DOES NOT call `amiport_expand_argv()`, so it has
   to keep its local definition. If patch ever adopts `amiport_expand_argv`
   for argv globbing, the local `__progname` line should be removed to
   avoid the multiple-definition link error.
8. **Test gap: ENOENT input on plan_a.** The test suite checks nonexistent
   input file (RC=10) but does not specifically distinguish plan_a vs
   plan_b failure modes. A test that forces plan_b (via low memory or
   very large input) would document the fall-through path.
9. **Test gap: cross-device rename.** util.c handles `EXDEV` from
   `amiport_rename()` by copying instead of renaming. There is no test
   that exercises this path. AmigaOS volumes could trigger this when
   patching across `WORK:` and `RAM:`.
10. **Versioning string mismatch.** util.c:428 prints "Patch version
    2.0-12u8-OpenBSD" via `version()`, but the `$VER` cookie is "patch 1.78"
    and the Makefile `VERSION = 1.78`. The 2.0-12u8 string is an upstream
    artifact from the OpenBSD source. Cosmetic; consider aligning if a
    future revision touches util.c.
11. **`-lamiport-emu` link order.** The Makefile lists `-lamiport-emu` before
    `-lamiport` (via `LDFLAGS`). Confirm this order is required for symbol
    resolution; if not, switching could simplify common.mk inheritance.
