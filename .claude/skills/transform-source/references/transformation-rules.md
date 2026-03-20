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
#include <sys/time.h>
// After:
#include <amiport/sys/time.h>
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
// EXIT_SUCCESS (0) and EXIT_FAILURE (1) are fine — clib2 handles them
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

## Blocking Patterns — What to Do

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
