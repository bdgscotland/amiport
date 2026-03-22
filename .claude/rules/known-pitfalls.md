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

## MB_CUR_MAX Is a Runtime Function Call

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

## Exit Path Cleanup

AmigaOS has no automatic process memory cleanup with `-noixemul`. Every `exit()` call must free all allocated memory. When porting programs with global allocations (pattern arrays, compiled regex, line buffers), add a cleanup function called before every `exit()`. See grep port for the `cleanup_patterns()` pattern.
