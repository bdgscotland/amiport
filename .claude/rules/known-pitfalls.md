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

## Braceless Multi-Statement if/else With _exit()

When adding Amiga exit code blocks (`fflush(stdout); _exit(RC);`), forgetting braces around two-statement if/else branches is a common and subtle bug. In C, only the first statement belongs to the branch — `_exit()` executes unconditionally. The program appears to work but always exits with the wrong code. **Fix:** Always use braces when a branch has more than one statement. This was discovered in the grep 1.68 port where the exit-code logic always returned 0 regardless of match/error status.
