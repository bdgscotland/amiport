# Source Transformation Rules

Rules for transforming POSIX C source to Amiga-compatible code. Applied by the `transform-source` skill.

## Transformation Order

Apply transformations in this sequence:

1. **Header replacements** — Swap POSIX includes for amiport shim headers
2. **Type replacements** — Replace POSIX-specific types
3. **Function call replacements** — Swap POSIX functions for shim wrappers
4. **Macro/constant replacements** — Replace POSIX constants with Amiga equivalents
5. **Conditional compilation** — Add `#ifdef __AMIGA__` blocks where needed
6. **Amiga boilerplate** — Add version string, stack cookie

## Rule Format

Each rule specifies: pattern to match, replacement, and when to apply.

---

## 1. Header Replacements

```c
/* RULE: Replace POSIX headers with amiport shim headers */

// Before:
#include <unistd.h>
// After:
#include <amiport/unistd.h>

// Before:
#include <dirent.h>
// After:
#include <amiport/dirent.h>

// Before:
#include <sys/stat.h>
// After:
#include <amiport/sys/stat.h>

// Before:
#include <getopt.h>
// After:
#include <amiport/getopt.h>

// Before:
#include <signal.h>
// After:
#include <amiport/signal.h>

// Before:
#include <stdlib.h>
// After:
#include <amiport/stdlib.h>
// Note: This activates the exit() → amiport_exit() macro which prevents
// the libnix exit() hang (crash-patterns #9). Every ported program MUST
// include this header.

// Before:
#include <sys/time.h>
// After:
#include <amiport/sys/time.h>
```

// Console UI headers (Category 3 — link with -lamiport-console):
// Before:
#include <curses.h>
// After:
#include <amiport-console/curses.h>

// Before:
#include <ncurses.h>
// After:
#include <amiport-console/curses.h>

// Before:
#include <term.h>
// After:
#include <amiport-console/term.h>

// Network headers (Category 4 — link with -lamiport-net):
// Before:
#include <sys/socket.h>
// After:
#include <amiport-net/socket.h>

// Before:
#include <netinet/in.h>
// After:
#include <amiport-net/netinet/in.h>

// Before:
#include <netdb.h>
// After:
#include <amiport-net/netdb.h>

// Before:
#include <arpa/inet.h>
// After:
#include <amiport-net/arpa/inet.h>
```

Headers to **remove entirely** (with a comment):
```c
// Before:
#include <pthread.h>
// After:
/* amiport: removed <pthread.h> — no pthreads on AmigaOS */

// Before:
#include <sys/mman.h>
// After:
/* amiport: removed <sys/mman.h> — no mmap on AmigaOS */

// Before:
#include <dlfcn.h>
// After:
/* amiport: removed <dlfcn.h> — no dynamic loading on classic AmigaOS */

// Before:
#include <locale.h>
// After:
/* amiport: removed <locale.h> — setlocale() stub is in <amiport/unistd.h> */

// Before:
#include <util.h>
// After:
/* amiport: removed <util.h> — OpenBSD-specific; inline fmt_scaled() if needed */
// See Section 9 "fmt_scaled()" rule for inline implementation

// Before:
#include <wchar.h>
// After:
/* amiport: removed <wchar.h> — no wchar support on AmigaOS 3.x */
// Guard multibyte code paths with #ifndef __AMIGA__

// Before:
#include <wctype.h>
// After:
/* amiport: removed <wctype.h> — no wctype support on AmigaOS 3.x */
```

Headers that **need no change** (provided by clib2/newlib):
```c
#include <stdio.h>      /* OK — provided by C runtime */
#include <stdlib.h>     /* OK */
#include <string.h>     /* OK */
#include <ctype.h>      /* OK */
#include <math.h>       /* OK */
#include <errno.h>      /* OK — but errno values may differ */
#include <limits.h>     /* OK */
#include <stdarg.h>     /* OK */
#include <assert.h>     /* OK */
```

## 2. Type Replacements

```c
/* RULE: Replace POSIX types with portable equivalents */

// pid_t — replace with LONG or remove
// Before:
pid_t pid = fork();
// After:
/* amiport: fork() not available — see blocking issues */

// ssize_t — replace with LONG
// Before:
ssize_t n = read(fd, buf, count);
// After:
LONG n = amiport_read(fd, buf, count);

// mode_t — replace with ULONG
// Before:
mode_t mode = 0644;
// After:
ULONG mode = 0644; /* amiport: mode bits not used on AmigaOS */

// off_t — replace with LONG (classic) or use amiport typedef
// Before:
off_t pos = lseek(fd, 0, SEEK_END);
// After:
LONG pos = amiport_lseek(fd, 0, SEEK_END);
```

## 3. Function Call Replacements

### File I/O
```c
/* RULE: Replace POSIX file I/O with amiport shim calls */

// Before:
int fd = open("file.txt", O_RDONLY);
// After:
int fd = amiport_open("file.txt", O_RDONLY); /* amiport: replaced open() */

// Before:
ssize_t n = read(fd, buffer, sizeof(buffer));
// After:
LONG n = amiport_read(fd, buffer, sizeof(buffer)); /* amiport: replaced read() */

// Before:
close(fd);
// After:
amiport_close(fd); /* amiport: replaced close() */
```

### Directory Operations
```c
/* RULE: Replace POSIX directory ops with amiport shim calls */

// Before:
DIR *dir = opendir("/path");
struct dirent *entry;
while ((entry = readdir(dir)) != NULL) {
    printf("%s\n", entry->d_name);
}
closedir(dir);
// After:
AMIPORT_DIR *dir = amiport_opendir("/path"); /* amiport: replaced opendir() */
struct amiport_dirent *entry;
while ((entry = amiport_readdir(dir)) != NULL) { /* amiport: replaced readdir() */
    printf("%s\n", entry->d_name);
}
amiport_closedir(dir); /* amiport: replaced closedir() */
```

### Process/System
```c
/* RULE: Replace process management calls */

// Before:
char *home = getenv("HOME");
// After:
char *home = amiport_getenv("HOME"); /* amiport: replaced getenv() — uses GetVar() */

// Before:
sleep(5);
// After:
amiport_sleep(5); /* amiport: replaced sleep() — uses Delay() */
```

## 4. Constant Replacements

```c
/* RULE: Replace POSIX constants */

// Path separators — Amiga uses / like Unix but volume: prefix differs
// Generally no change needed for / separators
// But watch for hardcoded /tmp, /dev/null etc:

// Before:
FILE *f = fopen("/dev/null", "w");
// After:
FILE *f = fopen("NIL:", "w"); /* amiport: /dev/null → NIL: */

// Before:
tmpnam("/tmp/myfile");
// After:
tmpnam("T:myfile"); /* amiport: /tmp → T: assign */

// Exit codes — Amiga uses RETURN_OK (0), RETURN_WARN (5), RETURN_ERROR (10), RETURN_FAIL (20)
// EXIT_SUCCESS (0) maps to RETURN_OK — correct.
// EXIT_FAILURE (1) does NOT map to any Amiga convention — Amiga scripts use
// IF WARN (>=5), IF ERROR (>=10), IF FAIL (>=20). Exit code 1 is invisible.
// RULE: Replace exit(1) with exit(10) for errors, exit(20) for fatal errors.
// Or use the Amiga constants directly:
// Before:
exit(1);
// After:
exit(10); /* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */

// Before:
exit(EXIT_FAILURE);
// After:
exit(10); /* amiport: RETURN_ERROR */
```

## 5. Conditional Compilation

Use `#ifdef __AMIGA__` when code should remain cross-platform:

```c
/* RULE: Wrap platform-specific code in #ifdef blocks */

#ifdef __AMIGA__
#include <amiport/unistd.h>
#include <proto/exec.h>
#include <proto/dos.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
```

Only use `#ifdef` when the original source might still be compiled on other platforms.
For dedicated Amiga ports, direct replacement is preferred.

## 6. Amiga Boilerplate

Add to every ported program:

```c
/* amiport: Amiga version string */
static const char *verstag = "$VER: progname 1.0 (19.03.2026)";

/* amiport: Stack size cookie — increase if program uses deep recursion */
/* Default 8KB is often too small for ported software */
LONG __stack = 32768;
```

## 7. BSD-ism Replacements

### Header additions for BSD functions
```c
/* RULE: Add amiport shim headers for BSD functions */

// For strlcpy/strlcat/reallocarray:
#include <amiport/string.h>

// For asprintf/vasprintf/mkstemp/pread/pwrite:
#include <amiport/stdio_ext.h>

// For err/errx/warn/warnx/strtonum:
#include <amiport/err.h>

// For fnmatch:
#include <amiport/fnmatch.h>

// For scandir/alphasort:
#include <amiport/scandir.h>

// For regex (Tier 2):
#include <amiport-emu/regex.h>
```

### BSD string functions
```c
/* RULE: Replace BSD string functions with amiport shim wrappers */

// Before:
strlcpy(dst, src, sizeof(dst));
// After:
amiport_strlcpy(dst, src, sizeof(dst)); /* amiport: replaced strlcpy() */

// Before:
strlcat(dst, src, sizeof(dst));
// After:
amiport_strlcat(dst, src, sizeof(dst)); /* amiport: replaced strlcat() */
```

Note: With AMIPORT_NO_STRING_MACROS not defined, the convenience macros handle this
automatically — just include `<amiport/string.h>` and use the original function names.

### BSD/GNU memory and string formatting
```c
/* RULE: Replace BSD/GNU memory and formatting functions */

// Before:
p = reallocarray(p, n, sizeof(*p));
// After:
p = amiport_reallocarray(p, n, sizeof(*p)); /* amiport: replaced reallocarray() */

// Before:
p = recallocarray(p, oldcount, newcount, sizeof(*p));
// After:
p = amiport_recallocarray(p, oldcount, newcount, sizeof(*p)); /* amiport: replaced recallocarray() */

// Before:
asprintf(&str, "hello %s", name);
// After:
amiport_asprintf(&str, "hello %s", name); /* amiport: replaced asprintf() */

/* CRITICAL: Never use vsnprintf(NULL, 0, ...) to measure buffer size.
 * libnix does NOT support NULL destination — crashes on 68000.
 * Use a probe buffer instead:
 *   char probe[1024];
 *   int len = vsnprintf(probe, sizeof(probe), fmt, ap);
 * See crash-patterns.md #5. */

// Before:
fd = mkstemp(template);
// After:
fd = amiport_mkstemp(template); /* amiport: replaced mkstemp() */

// Before:
n = pread(fd, buf, count, offset);
// After:
n = amiport_pread(fd, buf, count, offset); /* amiport: replaced pread() — non-atomic seek+read */
```

### BSD security stubs
```c
/* RULE: Stub OpenBSD security functions */

// Before:
#include <unistd.h>
if (pledge("stdio rpath", NULL) == -1)
    err(1, "pledge");
if (unveil("/path", "r") == -1)
    err(1, "unveil");
// After:
/* amiport: pledge/unveil not available on AmigaOS — stubbed */
#define pledge(p, e) (0)
#define unveil(p, f) (0)
```

### BSD fgetln → fgets
```c
/* RULE: Replace fgetln() with fgets() */

// Before:
char *line;
size_t len;
line = fgetln(fp, &len);
// After:
/* amiport: replaced fgetln() with fgets() — line is NUL-terminated */
static char _line_buf[8192];
char *line;
size_t len;
line = fgets(_line_buf, sizeof(_line_buf), fp);
if (line) len = strlen(line);
```

## 8. Tier 2 Emulation Replacements

For functions classified as `needs-emu` (Tier 2), use `amiport_emu_*` wrappers from `lib/posix-emu/`.
Always add a `/* amiport-emu: ... */` comment documenting the behavioural difference.

### select() / poll()
```c
/* RULE: Replace select() with amiport_emu_select() — Tier 2 emulation */

/* Before: */
#include <sys/select.h>
fd_set readfds;
FD_ZERO(&readfds);
FD_SET(fd, &readfds);
select(fd + 1, &readfds, NULL, NULL, &timeout);

/* After: */
#include <amiport-emu/select.h>
/* amiport-emu: select() emulated via WaitForChar() polling — 20ms granularity, no socket support, exceptfds ignored */
amiport_emu_fd_set readfds;
AMIPORT_EMU_FD_ZERO(&readfds);
AMIPORT_EMU_FD_SET(fd, &readfds);
amiport_emu_select(fd + 1, &readfds, NULL, NULL, &timeout);
```

### mmap() (read-only)
```c
/* RULE: Replace read-only mmap() with amiport_emu_mmap() — Tier 2 emulation */

/* Before: */
#include <sys/mman.h>
void *p = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
/* ... use p ... */
munmap(p, size);

/* After: */
#include <amiport-emu/mmap.h>
/* amiport-emu: mmap() emulated via AllocMem+Read — entire file loaded upfront, no lazy paging */
void *p = amiport_emu_mmap(NULL, size, AMIPORT_EMU_PROT_READ, AMIPORT_EMU_MAP_PRIVATE, fd, 0);
/* ... use p ... */
amiport_emu_munmap(p, size);
```

### pipe()
```c
/* RULE: Replace pipe() with amiport_emu_pipe() — Tier 2 emulation */

/* Before: */
int pipefd[2];
pipe(pipefd);

/* After: */
#include <amiport-emu/pipe.h>
/* amiport-emu: pipe() emulated via PIPE: device — named pipe, different buffering, no SIGPIPE */
int pipefd[2];
amiport_emu_pipe(pipefd);
```

### alarm()
```c
/* RULE: Replace alarm() with amiport_emu_alarm() — Tier 2 emulation */

/* Before: */
alarm(30);

/* After: */
#include <amiport-emu/alarm.h>
/* amiport-emu: alarm() emulated via timer.device — cooperative, not async. Call amiport_emu_check_alarm() in main loop */
amiport_emu_alarm_init(); /* once at startup */
amiport_emu_alarm(30);
/* In main loop: amiport_emu_check_alarm(); */
/* At cleanup: amiport_emu_alarm_cleanup(); */
```

### regex (POSIX)
```c
/* RULE: Replace POSIX regex with amiport_emu_regex — Tier 2 emulation */

/* Before: */
#include <regex.h>
regex_t re;
regmatch_t matches[2];
regcomp(&re, "pattern", REG_EXTENDED);
if (regexec(&re, string, 2, matches, 0) == 0) { /* matched */ }
regfree(&re);

/* After: */
#include <amiport-emu/regex.h>
/* amiport-emu: regex emulated — no locale collation, no [:class:], max 9 groups, backtracking NFA */
amiport_emu_regex_t re;
amiport_emu_regmatch_t matches[2];
amiport_emu_regcomp(&re, "pattern", REG_EXTENDED);
if (amiport_emu_regexec(&re, string, 2, matches, 0) == 0) { /* matched */ }
amiport_emu_regfree(&re);
```

Note: With AMIPORT_NO_REGEX_MACROS not defined, convenience macros allow using
the original POSIX names. Just include `<amiport-emu/regex.h>`.

## Tier 3 — Redesign Patterns (Do NOT Auto-Apply)

For functions classified as `needs-redesign` (Tier 3), do NOT stub silently.
Mark the location for human review and reference `redesign-patterns.md`:

```c
/* amiport-redesign: NEEDS HUMAN REVIEW
 * fork()+exec()+waitpid() pattern detected — subprocess-and-wait
 * See .claude/skills/transform-source/references/redesign-patterns.md
 * Options: SystemTags() (blocking) or CreateNewProcTags() (async) */
```

See `redesign-patterns.md` in this same directory for all available patterns.

## 9. Exit Code Fixup — CRITICAL

Every POSIX exit code must be remapped to Amiga conventions. Apply these systematically
across ALL source files after all other transformations are complete.

**IMPORTANT:** The final `exit()` call in `main()` MUST use `_exit()` instead of `exit()`.
libnix's `exit()` atexit handlers hang on real AmigaOS when stdio is connected to a
console. Always do explicit cleanup + `fflush(stdout)` + `_exit(rval)`.

```c
/* RULE: Replace POSIX exit codes with Amiga equivalents */

/* Final exit in main() — use _exit() to avoid libnix atexit hang */
// Before:
exit(rval);
// After:
fflush(stdout);          /* amiport: explicit flush — libnix exit() hangs on AmigaOS console */
_exit(rval);             /* amiport: _exit() bypasses atexit handlers that hang on AmigaOS */

/* Other exit() calls (error paths, usage) — keep as exit() for cleanup */
// Before:
exit(1);
// After:
exit(10); /* amiport: RETURN_ERROR — visible to Amiga IF ERROR scripts */

// Before:
exit(EXIT_FAILURE);
// After:
exit(10); /* amiport: RETURN_ERROR */

// Before:
exit(2);
// After:
exit(20); /* amiport: RETURN_FAIL — visible to Amiga IF FAIL scripts */

/* err()/errx() calls — exit code is the first argument */
// Before:
err(1, "cannot open %s", filename);
// After:
err(10, "cannot open %s", filename); /* amiport: RETURN_ERROR */

// Before:
errx(1, "invalid argument");
// After:
errx(10, "invalid argument"); /* amiport: RETURN_ERROR */

// Before:
err(2, "fatal: %s", msg);
// After:
err(20, "fatal: %s", msg); /* amiport: RETURN_FAIL */

// Before:
errx(2, "fatal error");
// After:
errx(20, "fatal error"); /* amiport: RETURN_FAIL */

/* warn()/warnx() — no exit code, no change needed */

/* return from main() */
// Before:
return 1;
// After:
return 10; /* amiport: RETURN_ERROR */

// Before:
return 2;
// After:
return 20; /* amiport: RETURN_FAIL */
```

### Exit Code Mapping Table

| POSIX | Amiga | Constant | Amiga Shell Test |
|-------|-------|----------|-----------------|
| 0 | 0 | RETURN_OK | (always passes) |
| 1 | 10 | RETURN_ERROR | `IF ERROR` |
| 2 | 20 | RETURN_FAIL | `IF FAIL` |
| EXIT_SUCCESS | 0 | RETURN_OK | (always passes) |
| EXIT_FAILURE | 10 | RETURN_ERROR | `IF ERROR` |

**Special case:** Some programs use exit(1) for "no match found" (e.g., grep returns 1
when no lines match). In these cases, use exit(5) (RETURN_WARN) so Amiga scripts can
distinguish "no match" from "error": `IF WARN` catches exit(5), `IF ERROR` catches exit(10).

## Legacy: Blocking Patterns — What to Do

When you encounter these, **do not silently remove them**. Stub with a clear message:

```c
/* RULE: Stub blocking patterns */

// fork/exec — stub the function, print warning
#ifdef __AMIGA__
/* amiport: fork() is not available on AmigaOS.
 * This functionality requires redesign for the Amiga's
 * CreateNewProc() model. */
#define fork() (-1)
#endif

// pthreads — stub with single-threaded equivalents
#ifdef __AMIGA__
/* amiport: pthreads not available on AmigaOS.
 * Mutex operations are no-ops (single-threaded). */
#define pthread_mutex_lock(m)   (0)
#define pthread_mutex_unlock(m) (0)
#define pthread_mutex_init(m,a) (0)
#endif

// mmap — stub to return MAP_FAILED
#ifdef __AMIGA__
#define mmap(...)  ((void *)-1)
#define munmap(...)  (0)
#endif
```

## 10. Wildcard/Glob Handling

### argv wildcard expansion
Every ported program SHOULD call `amiport_expand_argv()` at the top of `main()` and
`amiport_free_argv()` before `_exit()`. This expands wildcard arguments (*.c, #?.c)
since AmigaOS shells do not glob-expand like Unix.

```c
/* RULE: Add argv wildcard expansion to main() */

#include <amiport/glob.h>

int main(int argc, char *argv[])
{
    /* amiport: expand wildcard args — Amiga shell doesn't glob */
    amiport_expand_argv(&argc, &argv);

    /* ... original main body ... */

    /* amiport: free expanded argv before exit */
    amiport_free_argv();
    fflush(stdout);
    _exit(rval);
}
```

### __nowild opt-out for pattern-argument programs
Programs that accept regex or pattern arguments (grep -e PATTERN, sed SCRIPT,
find -name PATTERN, awk PROGRAM) MUST define `__nowild` to prevent expansion of
those arguments:

```c
/* RULE: Suppress argv expansion for programs taking pattern args */
/* amiport: suppress wildcard expansion — program takes pattern arguments */
int __nowild = 1;
```

Programs that need `__nowild`: grep, sed, awk, find, expr, test, and any program
where a non-option argument is a pattern/regex rather than a filename.

### glob()/globfree() replacement
```c
/* RULE: Replace POSIX glob with amiport shim wrapper */

// Before:
#include <glob.h>
glob_t g;
glob("pattern", 0, NULL, &g);
globfree(&g);

// After:
#include <amiport/glob.h>
/* amiport: replaced glob() */
amiport_glob_t g;
amiport_glob("pattern", 0, NULL, &g);
amiport_globfree(&g);
```

Note: With AMIPORT_NO_GLOB_MACROS not defined, convenience macros allow using
the original POSIX names. Just include `<amiport/glob.h>`.

## 11. Long-Running Loops — Ctrl-C Break Check

Any loop that runs indefinitely or for an extended period MUST include a Ctrl-C
break check using `amiport_check_break()` from `<amiport/signal.h>`. Without this,
the user cannot interrupt the program except by closing the shell window.

Common patterns that need break checks:
- `tail -f` follow loops (polling with `Delay()`)
- `grep -r` recursive directory walks
- Event loops in interactive programs
- Any `while (1)` or `for (;;)` that doesn't have a natural termination

```c
/* RULE: Add Ctrl-C break check to long-running loops */

#include <amiport/signal.h>

// Before (infinite polling loop with no break):
while (1) {
    Delay(50);
    do_work();
}

// After:
while (1) {
    Delay(50);
    /* amiport: check for Ctrl-C break signal */
    if (amiport_check_break()) {
        (void)fflush(stdout);
        return;
    }
    do_work();
}
```

## 9. Crash-Pattern Prevention Rules

Rules derived from `docs/references/crash-patterns.md`. Apply these AFTER all other
transformations to prevent known AmigaOS-specific bugs.

### exit() → _exit() at end of main() (crash-patterns #9)

libnix's `exit()` calls atexit handlers that deadlock when closing console-connected
stdio streams. `_exit()` bypasses atexit and goes straight to AmigaDOS `Exit()`.

**IMPORTANT:** Only change `exit()` at the final return point of `main()`. Do NOT change
`exit()` in error paths, `err()`/`errx()` calls, or helper functions — those still need
atexit cleanup for proper resource release.

```c
/* RULE: Replace exit() with _exit() at end of main() */

// Before (at end of main()):
exit(0);
// After:
fflush(stdout); /* amiport: flush before _exit — no atexit handlers */
_exit(0); /* amiport: _exit to avoid libnix exit() hang (crash-patterns #9) */

// Before (with variable):
exit(rval);
// After:
fflush(stdout);
_exit(rval); /* amiport: _exit to avoid libnix exit() hang (crash-patterns #9) */
```

### Missing __stack cookie (crash-patterns #7)

Amiga default stack is 4KB. vamos defaults to 8KB and ignores `__stack`. Most ported
programs need 32KB+. Recursive programs (find, diff) need 64KB+.

```c
/* RULE: Add __stack cookie if missing */

// Add at file scope near top of main source file:
long __stack = 32768; /* amiport: stack cookie — Amiga default 4KB is too small */

// For recursive programs (grep -r, find, diff):
long __stack = 65536; /* amiport: stack cookie — extra for recursion */
```

### __progname initialization (OpenBSD programs)

OpenBSD programs use `extern char *__progname` (auto-set by libc). bebbo-gcc libnix does NOT provide this. Define it and initialize from argv[0]:

```c
/* RULE: Initialize __progname for OpenBSD ports */

// Before (at file scope):
extern char *__progname;

// After (at file scope):
char *__progname; /* amiport: defined here — libnix does not provide it */

// Add at top of main(), before getopt:
__progname = strrchr(argv[0], '/');
if (__progname == NULL)
    __progname = strrchr(argv[0], ':'); /* amiport: Amiga volume separator */
if (__progname != NULL)
    __progname++;
else
    __progname = argv[0];
```

### <util.h> / fmt_scaled() (OpenBSD human-readable formatting)

OpenBSD's `<util.h>` provides `fmt_scaled()` and `FMT_SCALED_STRSIZE` for human-readable byte counts (-h flags). Not available on AmigaOS. Inline a minimal implementation:

```c
/* RULE: Replace <util.h> with inline fmt_scaled() */

// Remove:
#include <util.h>

// Add (at file scope):
/* amiport: fmt_scaled() from OpenBSD util.h — inlined */
#define FMT_SCALED_STRSIZE 7
static int
fmt_scaled(long long number, char *result)
{
    const char units[] = " KMGTPE";
    int i = 0;
    double v = (double)number;
    while (v >= 1024.0 && i < 6) { v /= 1024.0; i++; }
    if (i == 0)
        snprintf(result, FMT_SCALED_STRSIZE, "%6lld", number);
    else
        snprintf(result, FMT_SCALED_STRSIZE, "%5.1f%c", v, units[i]);
    return 0;
}
// Note: Uses double arithmetic — link with -lm for soft-float helpers
```

### MB_CUR_MAX guard (locale/multibyte paths)

`MB_CUR_MAX` expands to `__locale_mb_cur_max()` in libnix — a runtime function that may return >1. If the multibyte code path is compiled out (`#ifndef __AMIGA__`), guard the `MB_CUR_MAX` check too:

```c
/* RULE: Guard MB_CUR_MAX checks when multibyte path is compiled out */

// Before:
if (MB_CUR_MAX > 1)
    multibyte = 1;

// After:
#ifndef __AMIGA__
if (MB_CUR_MAX > 1)
    multibyte = 1;
#endif
/* amiport: MB_CUR_MAX is __locale_mb_cur_max() on libnix — may return >1
 * even without locale support. Must guard to prevent entering a compiled-out
 * code path. See known-pitfalls.md. */
```

### vsnprintf(NULL, 0, ...) probe buffer (crash-patterns #5)

C99 allows `vsnprintf(NULL, 0, fmt, ap)` to measure buffer size. libnix does NOT
support this — it writes to address zero and crashes.

```c
/* RULE: Replace vsnprintf(NULL, 0, ...) with probe buffer */

// Before:
int len = vsnprintf(NULL, 0, fmt, ap);

// After:
char probe[1024];
int len = vsnprintf(probe, sizeof(probe), fmt, ap);
/* amiport: probe buffer — libnix vsnprintf crashes on NULL (crash-patterns #5) */
```
