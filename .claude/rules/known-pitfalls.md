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

## exit() Hangs on AmigaOS — Use _exit()

libnix's `exit()` calls atexit handlers that can hang on real AmigaOS hardware (and FS-UAE), particularly when stdio handles are connected to a console. The program produces correct output but never returns to the shell prompt. **Fix:** Use `_exit(rval)` instead of `exit(rval)` in `main()`. Flush stdout explicitly first:
```c
fflush(stdout);
_exit(rval);
```
`_exit()` bypasses atexit handlers and goes straight to process termination. This is safe for ported programs because we explicitly free resources before calling it. Do NOT use `_exit()` in error paths where cleanup hasn't happened — use it only at the final exit point in `main()` after all cleanup is done.

## Exit Path Cleanup

AmigaOS has no automatic process memory cleanup with `-noixemul`. Every `exit()` call must free all allocated memory. When porting programs with global allocations (pattern arrays, compiled regex, line buffers), add a cleanup function called before every `exit()`. See grep port for the `cleanup_patterns()` pattern.
