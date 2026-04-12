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
#include <err.h>
// After:
#include <amiport/err.h>
// Note: bare <err.h> does NOT exist in bebbo-gcc libnix. Always replace.
// Also provides strtonum(), errc(), warnc() macros.

// Before:
#include <signal.h>
// After:
#include <amiport/signal.h>

// Before (Category 3 console ports):
#include <term.h>
// After:
#include <amiport-console/term.h>
// Note: Provides termcap API (tgetent, tgetstr, tgetnum, tgetflag, tgoto, tputs)
// and classic termcap globals (PC, BC, UP). Link with -lamiport-console.

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

// Before:
#include <pwd.h>
// After:
#include <amiport/pwd.h>

// Before:
#include <grp.h>
// After:
#include <amiport/grp.h>

// Before:
#include <sys/utsname.h>
// After:
#include <amiport/utsname.h>
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
/* RULE: Replace POSIX file I/O with amiport shim calls.
 *
 * CRITICAL: amiport_open() returns fds from amiport's internal fd table.
 * These are NOT libnix fds. NEVER pass an amiport fd to fdopen(), fprintf(),
 * or any libnix stdio function — the FILE* will silently fail to read/write.
 *
 * When you need a FILE*: use fopen() (which goes through libnix).
 * When you need raw fd I/O: use amiport_open/read/write/close consistently.
 * See crash-patterns.md #12.
 */

// PATTERN A: Need FILE* (most common — stdio, fprintf, fgets, etc.)
// Before:
int fd = open("file.txt", O_RDONLY);
FILE *fp = fdopen(fd, "r");
// After:
FILE *fp = fopen("file.txt", "r"); /* amiport: fopen, not amiport_open+fdopen */

// PATTERN B: Need raw fd I/O (read/write/lseek on binary data)
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
// CRITICAL: amiport_getenv() returns malloc'd strings (unlike POSIX getenv which
// returns static storage). The caller MUST free the result or track it for atexit
// cleanup. For NULL-check-only usage: store in temp, check, free immediately.
// See known-pitfalls.md "amiport_getenv() Returns malloc'd Strings".

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
/* amiport: Amiga version string — use upstream version from Makefile VERSION.
   If REVISION > 1, append -REVISION (e.g., "1.68-2"). See common.mk DISPLAY_VERSION. */
static const char *verstag = "$VER: progname 1.68 (25.03.2026)";

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

**NOTE:** The libnix exit() hang (crash-patterns #9) was **DEBUNKED** — it was caused by
ARexx syntax errors in the test harness, not by exit() itself. Use regular `exit()`
everywhere. Do NOT use `_exit()` — it bypasses atexit handlers, defeating cleanup
patterns like `amiport_free_argv()` and `fflush(stdout)` that are critical on AmigaOS
(no automatic process memory cleanup with -noixemul).

```c
/* RULE: Replace POSIX exit codes with Amiga equivalents */
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
`amiport_free_argv()` via atexit(). This expands wildcard arguments (*.c, #?.c)
since AmigaOS shells do not glob-expand like Unix.

```c
/* RULE: Add argv wildcard expansion to main() */

#include <amiport/glob.h>

static void cleanup(void) {
    amiport_free_argv();
    (void)fflush(stdout);
}

int main(int argc, char *argv[])
{
    /* amiport: expand wildcard args — Amiga shell doesn't glob */
    amiport_expand_argv(&argc, &argv);
    /* amiport: register cleanup for all exit paths including err()/errx() */
    atexit(cleanup);

    /* ... original main body ... */

    exit(rval); /* atexit cleanup handles amiport_free_argv + fflush */
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

### ~~exit() → _exit() at end of main()~~ DEBUNKED (crash-patterns #9)

The libnix exit() hang was **DEBUNKED** — it was caused by ARexx syntax errors in the
FS-UAE test harness (UTF-8 characters and `\=` operator), not by exit() itself. Testing
with minimal programs on FS-UAE + Workbench 3.1 confirmed that `exit(0)` returns
immediately. See known-pitfalls.md "exit() Hangs on AmigaOS — DEBUNKED".

**Do NOT use `_exit()` in ported code.** `_exit()` bypasses atexit handlers, which
defeats cleanup patterns (`amiport_free_argv()`, memory freeing) that are critical on
AmigaOS where there is no automatic process memory cleanup with `-noixemul`.

Use `exit()` normally. Register cleanup via `atexit()` at the top of main().

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

### __progname (OpenBSD programs)

OpenBSD programs use `extern char *__progname` (auto-set by libc). The amiport shim now provides this automatically — `amiport_expand_argv()` initializes `__progname` from `argv[0]` (stripping path/volume prefix). It is declared in `<amiport/glob.h>` with weak linkage.

```c
/* RULE: __progname is provided by the shim — no per-port boilerplate needed */

// Before (at file scope):
extern char *__progname;

// After: KEEP AS-IS — the shim provides __progname via <amiport/glob.h>.
// The extern declaration in the source is satisfied by the shim's weak definition.
// No need to define it or initialize it manually.

// If <amiport/glob.h> is not included (rare — most ports use expand_argv),
// add: #include <amiport/glob.h>
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

---

## Category 3 (Console UI) Transforms

These apply only to programs using terminal capabilities (less, nano, vim, htop).

### H-TERMCAP: termcap header replacement

**When:** Source includes `<termcap.h>`, `<ncurses/termcap.h>`, or `<curses.h>` for termcap functions
**Pattern:** `#include <termcap.h>`
**Replace:** `#include <amiport-console/term.h>`
**Comment:** `/* amiport: termcap via console-shim (ADR-009) */`

### H-TERMIOS: termios header replacement

**When:** Source includes `<termios.h>` for raw/cooked terminal mode
**Pattern:** `#include <termios.h>`
**Replace:** `#include <amiport/termios.h>`
**Comment:** `/* amiport: termios shim — maps tcsetattr to SetMode() */`

### H-CURSES: ncurses header replacement

**When:** Source includes `<curses.h>` or `<ncurses.h>` for full ncurses API
**Pattern:** `#include <curses.h>` or `#include <ncurses.h>`
**Replace:** `#include <amiport-console/curses.h>`
**Comment:** `/* amiport: ncurses via console-shim (ADR-009) */`

### F-DEVTTY: /dev/tty replacement

**When:** Source opens `/dev/tty` for keyboard input (bypassing stdin redirection)
**Pattern:** `open("/dev/tty", ...)` or string constant `"/dev/tty"`
**Replace:** `open("*", ...)` or `"*"` — AmigaDOS star device = current console
**Comment:** `/* amiport: /dev/tty → "*" (AmigaDOS star device) */`
**Guard:** `#ifdef __AMIGA__` / `#else` / `#endif` for cross-platform

### F-SIGWINCH: Window resize signal stub

**When:** Source installs a SIGWINCH handler for terminal resize events
**Pattern:** `signal(SIGWINCH, handler)` or `sigaction(SIGWINCH, ...)`
**Action:** Disabled via defines.h (`SIGWINCH` not defined on AmigaOS). If not `#ifdef`-guarded in source, add `#ifndef __AMIGA__` guard.
**Note:** Console-shim's `scrsize()` queries window size at startup via CSI Window Status Request (ADCD). No runtime resize detection — AmigaOS shell windows are typically fixed-size.

### C-TI-TE: Terminal init/deinit sequences

**When:** Console-shim returns terminal init (`ti`) and deinit (`te`) capability strings
**Critical:** Amiga console.device does NOT support alternate screen buffers (`ESC[?47h`/`ESC[?47l`). Use clear screen (`ESC[2J ESC[H`) instead. This is already set in console-shim's tgetstr() — documented here for awareness.

### C-CURSOR-HIDE: Cursor visibility

**When:** Program uses `civis`/`cnorm` termcap capabilities to hide/show cursor
**Note:** Amiga console.device does not support DEC private mode `ESC[?25l`/`ESC[?25h`. These are silently ignored. Cursor remains visible during paging. Known limitation — document in PORT.md.

---

## Numeric Format Transforms

### FMT-G-PRECISION: snprintf %g precision above 15 (crash-patterns #20)

**When:** Source uses `snprintf(buf, size, "%.Ng", val)` where N > 15
**Pattern:** `%.30g`, `%.20g`, `%.17g`, etc.
**Replace:** `%.15g` -- maximum meaningful precision for IEEE 754 double
**Comment:** `/* amiport: %.Ng -> %.15g -- libnix shows FP noise above 15 digits (crash-patterns #20) */`
**Why:** libnix does not strip trailing zeros beyond 15 digits, so `%.30g` of `1.0` produces `1.0000000000` instead of `1`. The math is correct but the formatting is wrong.
**Detection:** `grep -rn '%\.\(1[5-9]\|[2-9][0-9]\)g' ported/*.c`

---

## Memory Safety Transforms (MANDATORY)

### MEM-STATIC-BUF: Promote static local buffers to file scope for atexit cleanup

**When:** ANY function allocates memory via `static char *buf`, `getline(&buf, ...)`, `malloc()` into a static local, or `strdup()` into a local/global that isn't tracked
**Pattern:** `static char *buf = NULL;` inside a helper function (e.g., fold(), input(), slurp())
**Replace:** Move to file scope: `static char *funcname_buf = NULL;` at top of file. Inside the function, add `#define buf funcname_buf`. Add `free(funcname_buf); funcname_buf = NULL;` to `cleanup()`.
**Comment:** `/* amiport: buf promoted to file scope for atexit cleanup -- AmigaOS has no GC */`
**Why:** AmigaOS with -noixemul has NO automatic process memory cleanup. atexit(cleanup) is the ONLY cleanup mechanism. Static locals inside helper functions are invisible to cleanup(). Every dynamic allocation must be reachable from cleanup().

### MEM-OBSOLETE-TRACK: Track obsolete() argv rewrite mallocs

**When:** An `obsolete()` function mallocs strings and stores them in argv
**Pattern:** `*p = t;` where `t = malloc(...)` inside obsolete()
**Replace:** Add global tracking array: `static char *obsolete_allocs[MAX]; static int obsolete_alloc_count;`. After malloc: `if (count < MAX) obsolete_allocs[count++] = t;`. In cleanup: `for (i = 0; i < count; i++) free(obsolete_allocs[i]);`
**Comment:** `/* amiport: track for atexit cleanup -- AmigaOS has no GC */`
**Why:** obsolete() rewrites argv entries with malloc'd strings that are never freed. Affects: grep, tail, uniq, join, and any OpenBSD tool with obsolete().

### MEM-ERRX-CLEANUP: Free intermediate values before errx() in recursive-descent parsers

**When:** eval*() functions allocate intermediate values (struct val, etc.) and call errx() on error paths without freeing
**Pattern:** `l = eval*(); r = eval*(); if (error) errx(10, ...);`
**Replace:** `l = eval*(); r = eval*(); if (error) { free_value(l); free_value(r); errx(10, ...); }`
**Comment:** `/* amiport: free intermediates before exit -- AmigaOS has no GC */`
**Why:** errx() exits immediately, bypassing any cleanup except atexit(). Intermediate allocations in the call chain are permanently leaked.

### MEM-STRCOLL-STRCMP: Replace strcoll() with strcmp() on AmigaOS

**When:** Source uses `strcoll()` for string comparison
**Pattern:** `strcoll(a, b)`
**Replace:** `strcmp(a, b)`
**Comment:** `/* amiport: strcoll -> strcmp -- no functional locale on AmigaOS 3.x */`
**Why:** strcoll() invokes locale infrastructure even in C locale, adding ~30% overhead per comparison. AmigaOS has no functional locale support, so strcoll() == strcmp() semantically but costs more.

### INIT-SUPPRESS-REQUESTERS: Suppress AmigaDOS volume requesters (Category 3+)

**When:** Category 3+ port (editors, shells, interpreters) that probes config/runtime paths at startup
**Pattern:** Program init function (e.g., `main()` or OS-specific init)
**Add:**
```c
/* amiport: suppress "Please insert volume" system requesters.
 * CLI tools should handle missing paths gracefully, not pop up dialogs. */
{
    struct Process *me = (struct Process *)FindTask(NULL);
    me->pr_WindowPtr = (APTR)-1L;
}
```
**Comment:** `/* amiport: suppress system requesters -- see crash-patterns #22 */`
**Why:** Any Lock()/Open() on a bare name triggers AmigaDOS volume requesters. Programs that search for config files (vim->$VIM, python->$PYTHONPATH, shell->.profile) hit this on every non-existent path. The requester freezes the FS-UAE test harness. Global suppression via pr_WindowPtr = -1 is the ADCD-documented approach.

### Ungated DEBUG_MALLOC Cleanup Code

**Pattern:** `#if defined DEBUG_MALLOC || defined TESTING` (or similar guards) around cleanup/free functions.
**Search:** `grep -n 'DEBUG_MALLOC\|#ifdef TESTING' ported/src/*.c` in any function named `cleanup`, `free_*`, `destroy_*`.
**Action:** Remove the `#ifdef` guard and its matching `#endif`. Make cleanup unconditional.
**Also:** Add `atexit(cleanup)` early in main() after `amiport_expand_argv()`.
**Comment:** `/* amiport: removed DEBUG_MALLOC/TESTING guard -- AmigaOS has no process memory cleanup with -noixemul */`
**Why:** GNU programs assume the OS reclaims process memory on exit. AmigaOS with -noixemul does not. Production builds with these guards leak ALL dynamic allocations permanently. Discovered in wget 1.20.3 where cleanup() had 70+ xfree() calls gated behind DEBUG_MALLOC.

## 10. Complete Shim Function Reference

All `amiport_*` function and type mappings provided by the posix-shim headers.
The code-transformer applies these as `#define` macro substitutions when the
corresponding `<amiport/*.h>` header is included.

### `<amiport/unistd.h>` -- File I/O, Process, Filesystem

```c
/* File I/O (see also Section 3 File I/O for usage patterns) */
open(path, flags)        -> amiport_open(path, flags)
close(fd)                -> amiport_close(fd)
read(fd, buf, n)         -> amiport_read(fd, buf, n)
write(fd, buf, n)        -> amiport_write(fd, buf, n)
lseek(fd, offset, whence) -> amiport_lseek(fd, offset, whence)

/* File descriptor operations (no macro -- call directly) */
dup(oldfd)               -> amiport_dup(oldfd)
dup2(oldfd, newfd)       -> amiport_dup2(oldfd, newfd)

/* Filesystem operations */
unlink(path)             -> amiport_unlink(path)           /* no macro -- call directly */
rename(old, new)         -> amiport_rename(old, new)       /* no macro -- call directly */
access(path, mode)       -> amiport_access(path, mode)
chmod(path, mode)        -> amiport_chmod(path, mode)
realpath(path, resolved) -> amiport_realpath(path, resolved)
symlink(target, linkpath) -> amiport_symlink(target, linkpath)  /* stub -- returns -1 */
readlink(path, buf, n)   -> amiport_readlink(path, buf, n)      /* stub -- returns -1 */
ftruncate(fd, len)       -> amiport_ftruncate(fd, len)
fchmod(fd, mode)         -> amiport_fchmod(fd, mode)       /* stub -- returns 0 */
fchown(fd, owner, group) -> amiport_fchown(fd, owner, group)  /* stub -- returns 0 */
lchown(path, owner, group) -> amiport_lchown(path, owner, group)  /* stub -- returns 0 */
utimensat(dirfd, path, times, flags) -> amiport_utimensat(dirfd, path, times, flags)
futimens(fd, times)      -> amiport_futimens(fd, times)

/* Process operations (no macro -- call directly) */
getcwd(buf, size)        -> amiport_getcwd(buf, size)
chdir(path)              -> amiport_chdir(path)
getpid()                 -> amiport_getpid()
isatty(fd)               -> amiport_isatty(fd)
sleep(seconds)           -> amiport_sleep(seconds)

/* Environment variables */
setenv(name, value, overwrite) -> amiport_setenv(name, value, overwrite)
unsetenv(name)           -> amiport_unsetenv(name)

/* Locale stubs */
setlocale(category, locale) -> amiport_setlocale(category, locale)
localeconv()             -> amiport_localeconv()

/* Time */
timegm(tm)               -> amiport_timegm(tm)

/* Terminal */
ioctl(fd, request, arg)  -> amiport_ioctl(fd, request, arg)  /* no macro -- call directly */
strtok_r(str, delim, saveptr) -> amiport_strtok_r(str, delim, saveptr)  /* no macro -- call directly */
tmpfile()                -> amiport_tmpfile()                 /* no macro -- call directly */

/* Types */
struct winsize           -> struct amiport_winsize   /* via #define winsize amiport_winsize */
struct lconv             -> struct amiport_lconv     /* via #define lconv amiport_lconv */
struct timespec          -> struct amiport_timespec  /* defined in header */
```

### `<amiport/stdlib.h>` -- Exit, Environment

```c
exit(status)             -> amiport_exit(status)
getenv(name)             -> amiport_getenv(name)    /* CRITICAL: returns malloc'd string, caller must free */
```

### `<amiport/stdio.h>` -- File Stream Operations

```c
fopen(path, mode)        -> amiport_fopen(path, mode)
fdopen(fd, mode)         -> amiport_fdopen(fd, mode)
fclose(fp)               -> amiport_fclose(fp)
fileno(fp)               -> amiport_fileno(fp)
```

### `<amiport/stdio_ext.h>` -- Extended stdio (BSD/GNU)

```c
asprintf(strp, fmt, ...) -> amiport_asprintf(strp, fmt, ...)
vasprintf(strp, fmt, ap) -> amiport_vasprintf(strp, fmt, ap)
mkstemp(template)        -> amiport_mkstemp(template)
pread(fd, buf, count, offset) -> amiport_pread(fd, buf, count, offset)
pwrite(fd, buf, count, offset) -> amiport_pwrite(fd, buf, count, offset)
getdelim(lineptr, n, delim, stream) -> amiport_getdelim(lineptr, n, delim, stream)
getline(lineptr, n, stream) -> amiport_getline(lineptr, n, stream)
fpurge(fp)               -> amiport_fpurge(fp)      /* stub -- no-op, returns 0 */
```

### `<amiport/string.h>` -- String Functions (BSD)

```c
strlcpy(dst, src, size)  -> amiport_strlcpy(dst, src, size)
strlcat(dst, src, size)  -> amiport_strlcat(dst, src, size)
reallocarray(ptr, n, size) -> amiport_reallocarray(ptr, n, size)
recallocarray(ptr, old, new, size) -> amiport_recallocarray(ptr, old, new, size)
strcasecmp(s1, s2)       -> amiport_strcasecmp(s1, s2)
strncasecmp(s1, s2, n)   -> amiport_strncasecmp(s1, s2, n)
strcasestr(haystack, needle) -> amiport_strcasestr(haystack, needle)
explicit_bzero(s, n)     -> amiport_explicit_bzero(s, n)
```

### `<amiport/err.h>` -- Error Reporting (BSD)

```c
err(eval, fmt, ...)      -> amiport_err(eval, fmt, ...)
errx(eval, fmt, ...)     -> amiport_errx(eval, fmt, ...)
errc(eval, code, fmt, ...) -> amiport_errc(eval, code, fmt, ...)
warn(fmt, ...)           -> amiport_warn(fmt, ...)
warnc(code, fmt, ...)    -> amiport_warnc(code, fmt, ...)
warnx(fmt, ...)          -> amiport_warnx(fmt, ...)
strtonum(str, min, max, errstr) -> amiport_strtonum(str, min, max, errstr)
```

### `<amiport/signal.h>` -- Signal Handling

```c
signal(signum, handler)  -> amiport_signal(signum, handler)
raise(signum)            -> amiport_raise(signum)
sigaction(sig, act, oact) -> amiport_sigaction(sig, act, oact)
sigemptyset(set)         -> amiport_sigemptyset(set)
sigaddset(set, signo)    -> amiport_sigaddset(set, signo)
sigprocmask(how, set, oset) -> amiport_sigprocmask(how, set, oset)
nanosleep(req, rem)      -> amiport_nanosleep(req, rem)

/* Types */
sighandler_t             -> amiport_sighandler_t
sigset_t                 -> amiport_sigset_t
struct sigaction         -> struct amiport_sigaction
```

### `<amiport/dirent.h>` -- Directory Operations

```c
opendir(path)            -> amiport_opendir(path)
readdir(dir)             -> amiport_readdir(dir)
closedir(dir)            -> amiport_closedir(dir)
mkdir(path, mode)        -> amiport_mkdir(path, mode)
rmdir(path)              -> amiport_rmdir(path)

/* Types */
struct dirent            -> struct amiport_dirent
```

### `<amiport/sys/stat.h>` -- File Status

```c
stat(path, buf)          -> amiport_stat(path, buf)
lstat(path, buf)         -> amiport_lstat(path, buf)   /* alias to amiport_stat -- no symlinks */
fstat(fd, buf)           -> amiport_fstat(fd, buf)
```

### `<amiport/sys/time.h>` -- Time Functions

```c
gettimeofday(tv, tz)     -> amiport_gettimeofday(tv)   /* no macro -- call directly */
time(tloc)               -> amiport_time(tloc)          /* no macro -- call directly */
usleep(usec)             -> amiport_usleep(usec)        /* no macro -- call directly */
strptime(s, fmt, tm)     -> amiport_strptime(s, fmt, tm)

/* Types */
struct timeval           -> struct amiport_timeval
struct tm                -> struct amiport_tm   /* used by amiport_strptime */
```

### `<amiport/pwd.h>` -- User Database

```c
getpwuid(uid)            -> amiport_getpwuid(uid)
getpwnam(name)           -> amiport_getpwnam(name)
getuid()                 -> amiport_getuid()
geteuid()                -> amiport_geteuid()
setuid(uid)              -> amiport_setuid(uid)
getlogin()               -> amiport_getlogin()
user_from_uid(uid, noname) -> amiport_user_from_uid(uid, noname)

/* Types */
struct passwd            -> struct amiport_passwd
```

### `<amiport/grp.h>` -- Group Database

```c
getgrgid(gid)            -> amiport_getgrgid(gid)
getgrnam(name)           -> amiport_getgrnam(name)
getgid()                 -> amiport_getgid()
getegid()                -> amiport_getegid()
setgid(gid)              -> amiport_setgid(gid)
getgroups(size, list)    -> amiport_getgroups(size, list)
getgrouplist(user, basegid, groups, ngroups) -> amiport_getgrouplist(user, basegid, groups, ngroups)
group_from_gid(gid, noname) -> amiport_group_from_gid(gid, noname)
ttyname(fd)              -> amiport_ttyname(fd)

/* Types */
struct group             -> struct amiport_group
```

### `<amiport/utsname.h>` -- System Information

```c
uname(buf)               -> amiport_uname(buf)
gethostname(name, len)   -> amiport_gethostname(name, len)
sysconf(name)            -> amiport_sysconf(name)
getrusage(who, usage)    -> amiport_getrusage(who, usage)
setproctitle(fmt, ...)   -> amiport_setproctitle(fmt, ...)  /* no macro -- call directly, no-op */

/* Types */
struct utsname           -> struct amiport_utsname
struct rusage            -> struct amiport_rusage
```

### `<amiport/getopt.h>` -- Option Parsing

```c
getopt(argc, argv, optstring) -> amiport_getopt(argc, argv, optstring)
getopt_long(argc, argv, optstring, longopts, longindex) -> amiport_getopt_long(...)
/* Globals: optarg, optind, opterr, optopt all remapped via macros */
```

### `<amiport/glob.h>` -- Pathname Pattern Matching

```c
glob(pattern, flags, errfunc, pglob) -> amiport_glob(pattern, flags, errfunc, pglob)
globfree(pglob)          -> amiport_globfree(pglob)
```

### `<amiport/fnmatch.h>` -- Filename Matching

```c
fnmatch(pattern, string, flags) -> amiport_fnmatch(pattern, string, flags)
```

### `<amiport/scandir.h>` -- Directory Scanning

```c
scandir(dirname, namelist, filter, compar) -> amiport_scandir(dirname, namelist, filter, compar)
alphasort(a, b)          -> amiport_alphasort(a, b)
```

### `<amiport/fts.h>` -- File Hierarchy Traversal

```c
fts_open(argv, options, compar) -> amiport_fts_open(argv, options, compar)
fts_read(ftsp)           -> amiport_fts_read(ftsp)
fts_close(ftsp)          -> amiport_fts_close(ftsp)
fts_set(ftsp, f, instr)  -> amiport_fts_set(ftsp, f, instr)
fts_children(ftsp, options) -> amiport_fts_children(ftsp, options)

/* Types (internal struct names: _amiport_fts, _amiport_ftsent) */
FTS                      -> AMIPORT_FTS      /* typedef of struct _amiport_fts */
FTSENT                   -> AMIPORT_FTSENT   /* typedef of struct _amiport_ftsent */
```

### `<amiport/profile.h>` -- Performance Profiling

```c
/* Profiling API (no POSIX equivalent -- amiport-specific) */
amiport_profile_init()       /* initialize timer.device for ReadEClock */
amiport_profile_summary()    /* print profiling results to stdout */
amiport_profile_record(name, ticks)  /* record a timing measurement */
amiport_profile_eclock()     /* raw ReadEClock timestamp */
/* Use AMIPORT_PROFILE_BEGIN(name) / AMIPORT_PROFILE_END(name) macros for scoped timing */
```
