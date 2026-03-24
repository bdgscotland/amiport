Paths: ports/**/*.c, ports/**/*.h, lib/**/*.c, lib/**/*.h, examples/**/*.c, examples/**/*.h

# Known Pitfalls — Hard-Won Lessons

These are real bugs encountered in real ports. Check for them every time.

## fopen() Macro Collision

`amiport/stdio.h` defines `#define fopen(p, m) amiport_fopen(p, m)`. Inside `file_io.c` (which implements `amiport_fopen`), this causes infinite recursion. **Fix:** `#undef fopen` before the implementation. Same for `fclose`.

## vamos argv Pointer Arithmetic

On vamos, argv entries are NOT contiguous — each is independently allocated. Code that does `argv[argc-1] - argv[0]` will crash. **Fix:** Use explicit `strlen()` iteration.

## pledge/unveil Stubs

OpenBSD code almost always calls `pledge()` and `unveil()`. Stub as macros:
```c
#define pledge(p, e) (0)
#define unveil(p, f) (0)
```
Never shim as functions.

## vsnprintf(NULL, 0, ...) Crash

libnix's `vsnprintf` does NOT support `NULL` as the destination buffer (C99 extension). Calling `vsnprintf(NULL, 0, fmt, ap)` to measure buffer size crashes on 68000 — libnix writes to address zero. **Fix:** Use a probe buffer:
```c
char probe[1024];
int len = vsnprintf(probe, sizeof(probe), fmt, ap);
```
This commonly appears when replacing `vasprintf()`/`asprintf()`. Use `amiport_asprintf()` instead. See crash-patterns.md #5.

## Long-Running Loops Need Ctrl-C Check

Any `while (1)` or `for (;;)` loop that runs indefinitely (e.g., `tail -f` polling, event loops, recursive directory walks) MUST call `amiport_check_break()` from `<amiport/signal.h>`. Without this, the user cannot interrupt the program — there is no OS-level SIGINT delivery on AmigaOS. **Fix:**
```c
#include <amiport/signal.h>
/* ... inside the loop: */
if (amiport_check_break()) {
    (void)fflush(stdout);
    return;
}
```

## ~~exit() Hangs on AmigaOS~~ DEBUNKED

This was a misdiagnosis. Testing with minimal programs on FS-UAE + Workbench 3.1 confirmed that `exit(0)` returns immediately — no hang. The actual cause of all FS-UAE test timeouts was ARexx syntax errors in the test harness (UTF-8 characters and `\=` operator). See crash-patterns.md #9 for the full post-mortem.

The `amiport_exit()` shim exists but is unnecessary. It remains in place to avoid churn.

## ARexx Files Must Be Pure ASCII

ARexx (1987) does not understand UTF-8. Any non-ASCII byte in a `.rexx` file — including inside comments — causes "Error 8: Unrecognized token" and crashes the script. Also, use `~=` for not-equal, not `\=` which may not be recognized by all ARexx interpreters. **This caused weeks of misdiagnosed "exit hang" bugs.**

## AmigaDOS Double-Redirect with Run

`Run >file1 cmd >file2` applies BOTH `>` redirections to the `Run` command at the top level. The backgrounded command gets NO redirect and floods the console. **Fix:** Write the command + redirect into a temp Execute script, then `Run >clinumfile Execute scriptfile`. This isolates the command's `>` from Run's `>`. See the test-designer agent and write-arexx skill for the full pattern.

## Large Local Buffers Cause Guru on Real AmigaOS

Local arrays > 512 bytes are dangerous. On real AmigaOS, dos.library/filesystem calls add 2-4KB of hidden stack depth that vamos doesn't simulate. A program with `char buf[4096]` and `__stack=16384` passes all vamos tests but crashes with Guru Meditation ACPU_LineF (0x8000000B) on FS-UAE/real hardware. **Fix:** Use `static` for large buffers in non-recursive functions, or increase `__stack` with an 8KB safety margin. See crash-patterns.md #10.

## atexit() for argv Expansion Cleanup

When using `amiport_expand_argv()`, the expanded argv is never freed if `err()`/`errx()`/`exit()` are called before `amiport_free_argv()`. These functions terminate immediately, skipping cleanup code. **Fix:** Register cleanup via `atexit()` right after expanding argv:
```c
amiport_expand_argv(&argc, &argv);
atexit(cleanup);  /* cleanup() calls amiport_free_argv() + fflush(stdout) */
```
This ensures cleanup runs on ALL exit paths, including err/errx. Without this, every error exit leaks argv memory permanently on AmigaOS with `-noixemul`.

## Never fclose(stdin) on AmigaOS

Calling `fclose(stdin)` closes the shell's input file handle. On Workbench, this crashes the console handler and can take down the entire desktop. Many C programs unconditionally `fclose(fp)` after use — when `fp == stdin`, this is fatal. **Fix:** Guard with `if (fp != stdin) fclose(fp);`. The source-analyzer and code-transformer agents should flag any `fclose()` that could receive stdin.

## MB_CUR_MAX Is a Runtime Function Call (crash-patterns #11)

`MB_CUR_MAX` in bebbo-gcc libnix expands to `__locale_mb_cur_max()` — a **runtime function call**, not a compile-time constant. It can return >1 even without locale support. Code that checks `if (MB_CUR_MAX > 1)` to enable multibyte paths will enter that path on AmigaOS, even though there's no wchar/locale support.

This is dangerous when the multibyte code path is conditionally compiled out with `#ifndef __AMIGA__` — setting a flag based on `MB_CUR_MAX` but removing the code that uses that flag means the non-multibyte path is also skipped, producing wrong output (typically all zeros).

**Fix:** Guard the `MB_CUR_MAX` check itself:
```c
#ifndef __AMIGA__
if (MB_CUR_MAX > 1)
    multibyte = 1;
#endif
```

**General rule:** For any POSIX macro used in conditionals (`MB_CUR_MAX`, `PATH_MAX`, `BUFSIZ`, etc.), check the actual expansion in bebbo-gcc headers — it may be a function call, not a constant. Don't assume compile-time values.

## amiport_open() + fdopen() = Silent Failure

`amiport_open()` returns fds from amiport's internal fd table. `fdopen()` is a libnix function expecting libnix fds. **These are different namespaces.** Passing an amiport fd to `fdopen()` creates a FILE* that silently fails to read/write. No crash, no error — just empty output.

**Fix:** Use `fopen()` whenever you need a `FILE*`. Only use `amiport_open()` with `amiport_read()`/`amiport_write()`/`amiport_close()` — never cross into libnix stdio. See crash-patterns.md #12.

## fgetc() Is 3-5x Slower Than fgets() on 68000

Each `fgetc()` call costs a JSR + stack frame + buffer check through libnix. For a 40-character line that's 40 function calls. Replace character-at-a-time loops with `fgets()` into a static buffer + `memcpy()`. On 7MHz 68000 with chip RAM contention, this eliminates ~1-2 million cycles per 1000-line file.

## Recursive Functions With Large Local Arrays

Large local arrays (>512 bytes) in recursive functions will blow the stack on real AmigaOS, even with `__stack = 65536`. Real AmigaOS adds 2-4KB of hidden stack depth from dos.library dispatch that vamos doesn't simulate. **Fix:** Make large arrays `static` when the function is single-threaded and not reentrant (which is always the case on AmigaOS). See crash-patterns.md #10.

## Exit Path Cleanup

AmigaOS has no automatic process memory cleanup with `-noixemul`. Every `exit()` call must free all allocated memory. When porting programs with global allocations (pattern arrays, compiled regex, line buffers), add a cleanup function called before every `exit()`. See grep port for the `cleanup_patterns()` pattern.

## vamos Default Stack Is 8 KiB — __stack Cookie Not Read at Runtime

The libnix startup code in `-noixemul` binaries does NOT check the `LONG __stack` variable or reallocate the stack. It uses whatever stack the OS (or vamos) provides. vamos defaults to 8 KiB (`amitools/vamos/cfg/proc.py: "stack": 8`). Programs that use AmigaDOS calls (Lock/Examine/GetDeviceProc) consume extra hidden stack beyond what C locals suggest. **Fix:** Add `VAMOS_STACK = 256` to the port Makefile and pass `-s $(VAMOS_STACK)` to all vamos invocations. On real AmigaOS hardware, the shell reads the `__stack` cookie correctly. See crash-patterns.md #7.

## Directory Filename Comparison Must Be Case-Insensitive

AmigaOS filesystems (OFS, FFS, SFS) are case-insensitive. Ported `diff -r`, `find`, `ls` and other directory-walking tools that use `strcmp()` to match filenames across directories will fail to match `Makefile` with `makefile`. **Fix:** Replace `strcmp()` with `strcasecmp()` for any filename/path comparison in directory operations. libnix provides `strcasecmp()`.

## readdir() d_type Is Always DT_UNKNOWN on AmigaOS

The `d_type` field in `struct dirent` is never populated by AmigaOS filesystems (OFS, FFS, SFS). It is always `DT_UNKNOWN` (0). Code that checks `dp->d_type == DT_DIR` to detect directories will silently treat all directories as regular files, breaking recursive operations like `grep -R`, `find`, and `diff -r`. **Fix:** Replace `d_type` checks with an `opendir()` probe — try `opendir(path)` on each entry; if it succeeds, the entry is a directory. This was discovered in the grep 1.68 port where `-R` silently skipped subdirectories.

## __progname Weak Symbol Lost During Linking

The `__progname` variable is declared as a weak symbol in `argv_expand.o` (inside `libamiport.a`). Even though the object is linked (because `amiport_expand_argv()` is called), the weak data symbol `__progname` may not survive in the final binary — bebbo-gcc's linker can strip unreferenced weak data symbols from archive members. The result: `__progname` resolves to address 0 (or garbage), and any `fprintf(stderr, "%s", __progname)` in `usage()` crashes with an invalid memory access. On FS-UAE this manifests as garbled output flooding the console before a Guru Meditation. **Fix:** Define `__progname` directly in the ported source: `char *__progname = "progname";`. The `amiport_expand_argv()` function writes to it via the extern declaration, so it will be updated from argv[0] at runtime. Discovered in the uniq 1.33 port.

## Braceless Multi-Statement if/else Blocks

When adding multi-statement blocks to if/else branches (e.g., `fflush(stdout); exit(RC);`), forgetting braces is a common and subtle bug. In C, only the first statement belongs to the branch — the second executes unconditionally. The program appears to work but always exits with the wrong code. **Fix:** Always use braces when a branch has more than one statement. This was discovered in the grep 1.68 port where the exit-code logic always returned 0 regardless of match/error status.

Note: the `_exit()` pattern referenced in older code was based on a debunked exit hang theory (crash-patterns #9). Use `exit()` with `atexit()` cleanup instead.

## libnix getopt_long Is Broken — Use amiport/getopt.h

The system `getopt_long()` from `<getopt.h>` exists as a symbol in libnix but returns `'?'` for ALL options, not just unknown ones. Every ported program using `getopt_long` will exit via usage() with RC=10 on the first option. This can masquerade as "works for error tests" since those expect RC=10.

**Fix:** Replace `#include <getopt.h>` with `#include <amiport/getopt.h>`. The amiport implementation aliases `getopt_long` → `amiport_getopt_long`, so no other source changes are needed. See crash-patterns.md #17.

**Detection:** If all functional tests return RC=10 and debug output shows `ch=63` ('?') for valid flags, this is the cause.

## dirname() Corrupts Its Input Buffer

`dirname(path)` is permitted by POSIX to modify the input string in-place. OpenBSD's `dirname` uses a static buffer and leaves the input alone — but libnix's `dirname` from `<libgen.h>` DOES modify the input. Any code that calls `dirname(filearg[0])` and then uses `filearg[0]` as a filename will find `filearg[0]` corrupted or empty.

**Fix (when dirname result not needed):** Remove the `dirname()` call if the result is only passed to a no-op (e.g., `unveil()` is always a no-op on AmigaOS).

**Fix (when dirname result IS needed):** Pass a `strdup()` copy to `dirname`, use the result, then free the copy AFTER you're done with the result (the result points into the copy's buffer).

See crash-patterns.md #18.

## AmigaOS Exclusive Write Lock — No Double-Open for Writing

AmigaDOS `MODE_NEWFILE` (used by `fopen("w")`) acquires an exclusive lock. If a file is already open for writing by the same program, a second `fopen(path, "w")` fails with `ERROR_OBJECT_IN_USE` (reported as "Text file busy" by strerror, which is misleading). On Unix, multiple handles can write to the same file simultaneously.

**Fix:** Close the first handle before opening the second. Check for patterns where `init_output(path)` opens a file globally, then a subroutine like `write_lines(path)` tries to open the same file.

See crash-patterns.md #19.

## obsolete() Argv Rewrite Leaks Memory

Many OpenBSD utilities have an `obsolete()` function that rewrites old-style numeric options (`+2` → `-s2`, `-3` → `-f3`) by malloc'ing new strings and assigning them to `*argv`. These strings are never freed because the pointers are stored in argv (which is not tracked for cleanup). On AmigaOS with `-noixemul`, this is a permanent leak per invocation. Affected ports: grep, tail, uniq, and any OpenBSD tool with `obsolete()`.

**Fix:** Track allocations in a static array and free them in the atexit cleanup function:
```c
#define MAX_OBSOLETE_ALLOCS 4
static char *obsolete_allocs[MAX_OBSOLETE_ALLOCS];
static int obsolete_alloc_count;

/* In obsolete(), after malloc: */
if (obsolete_alloc_count < MAX_OBSOLETE_ALLOCS)
    obsolete_allocs[obsolete_alloc_count++] = start;

/* In cleanup(), before amiport_free_argv(): */
for (i = 0; i < obsolete_alloc_count; i++)
    free(obsolete_allocs[i]);
```

## 68k Alignment Is 2, Not 4 or 8

`offsetof(struct { char x; union { int; double; } u; }, u)` returns **2** on 68k (word alignment). Code that uses this for custom allocator alignment (arenas, slab stacks, memory pools) will pack blocks at 2-byte boundaries. Storing `int`/`long`/pointer metadata between blocks corrupts data when indexed. **Fix:** Use `AMIPORT_ALIGN(offsetof(...))` from `<amiport/compat.h>` to force minimum 4-byte alignment. See crash-patterns #15.

## amiport_getenv() Returns malloc'd Strings — Caller MUST Free

POSIX `getenv()` returns a pointer to static internal storage — the caller must NOT free it. `amiport_getenv()` returns a `malloc()`'d copy (necessary because AmigaOS `GetVar()` requires a caller-provided buffer). Every `getenv()` → `amiport_getenv()` transformation silently introduces a memory leak unless the caller frees the result.

**Patterns that leak:**
```c
/* Pattern 1: result stored but never freed */
env_value = amiport_getenv("FOO");
if (env_value != NULL) { use(env_value); }
/* LEAK: env_value never freed */

/* Pattern 2: result used only for NULL check — pointer lost */
if (amiport_getenv("BAR") != NULL) { ... }
/* LEAK: malloc'd string discarded immediately */
```

**Fix:** Track results and free them, or free immediately after use:
```c
/* For NULL-check only: */
char *tmp = amiport_getenv("BAR");
if (tmp != NULL) { flag = TRUE; free(tmp); }

/* For value needed later: track in static array, free in atexit cleanup */
env_allocs[env_alloc_count++] = amiport_getenv("FOO");
```

Discovered in bc 1.07.1 port — 3 getenv calls leaked ~768 bytes per invocation.

## random() Missing from libnix — Use rand()

The libnix-reference.md lists `random()` as available, but it is **absent from the actual libnix archive** in current bebbo-gcc. Linking fails with `undefined reference to _random`. **Fix:** `#define random() rand()`. Both return `int` from `<stdlib.h>` and are equivalent for non-cryptographic use. Discovered in bc 1.07.1 port (execute.c used `random()` for the bc `random` built-in).

## Bundled getopt Clashes with libnix Symbols

When a port bundles its own GNU getopt (the correct approach — libnix's `getopt_long` is broken, see above), the `getopt`, `getopt_long`, `optarg`, `optind`, `opterr`, and `optopt` symbols clash with libnix's copies at link time. **Fix:** Add `-Wl,--allow-multiple-definition` to LDFLAGS. The bundled version (linked as object files) wins over libnix's archive version, which is the correct behavior. Discovered in bc 1.07.1 port.

## atexit Cleanup Must Not Free Uninitialized Array Entries

When programs use `more_*()` growth functions (realloc pattern: allocate larger array, copy old entries, leave new entries uninitialized), the `atexit` cleanup MUST NOT iterate and free individual entries. The new entries contain garbage pointers, and `free(garbage)` causes Guru `8100 0005` (AN_MemCorrupt — non-recoverable memory list corruption). **Fix:** Only free the array headers (`free(array_ptr)`), not individual entries like `free(array_ptr[i])`. Accept the ~200 byte leak of individual name strings as a tradeoff vs crashing. Discovered in bc 1.07.1 port — aggressive cleanup of `f_names[i]`/`v_names[i]`/`a_names[i]` entries caused AN_MemCorrupt because `more_functions()` leaves uninitialized slots after reallocation.

## bebbo-gcc -O1/-O2 Corrupts Large Struct Returns

GCC 6.5.0b for 68k generates incorrect code for functions returning structs > 8 bytes by value at `-O1` or `-O2`. The first byte of the struct reads as 0 in the caller despite being correct inside the function. **Fix:** Compile with `-O0`. Add `CFLAGS := $(subst -O2,-O0,$(CFLAGS))` to the port Makefile. No source-level workaround exists. See crash-patterns #16.

## Libraries MUST Use -m68000 (Not -m68020)

vamos only emulates a 68000 CPU. Libraries compiled with `-m68020` produce object files containing 68020 instructions. When a test binary links against such a library, the test crashes on vamos with `ALERT: code=00068020` — even if the test code itself is pure C89 with no 68020-specific features. The 68020 instructions come from the library's `.o` files inside the `.a` archive.

**Fix:** ALL libraries (`lib/posix-shim/`, `lib/console-shim/`, `lib/posix-emu/`, `lib/bsdsocket-shim/`) and ALL test Makefiles (`tests/*/Makefile`) MUST use `-m68000`. Individual port Makefiles inherit `-m68000` from `common.mk`. Discovered when console-shim was compiled with `-m68020` and test_termcap crashed on vamos.

## Static Archive Globals Pull Unwanted Dependencies

When a static archive (`.a`) contains objects with heavy OS dependencies (e.g., `initscr.o` opens `intuition.library`), programs that only need lightweight functions from the archive still pull in those heavy objects if they reference shared global variables defined in those objects. **Fix:** Put shared globals (`COLS`, `LINES`, `stdscr`, etc.) in a separate dependency-free `.c` file (e.g., `globals.c`) with no AmigaOS `#include <proto/*.h>` headers. This prevents the linker from pulling heavyweight objects into lightweight consumers. Discovered in console-shim — `test_termcap` only needed `term.o` but `COLS`/`LINES` in `initscr.o` forced `intuition.library` linkage.
