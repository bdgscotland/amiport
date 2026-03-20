# POSIX-to-AmigaOS Function Mapping

This is the master reference for mapping POSIX/Linux system calls to their AmigaOS equivalents
or amiport shim wrappers. Used by the `analyze-source` and `transform-source` skills.

## Severity Levels (Tiered — see ADR-008)

### Tier 1 — Shim (`lib/posix-shim/`)
- **trivial**: Direct mapping exists, mechanical replacement
- **needs-shim**: Requires amiport shim wrapper (behaviour differs slightly)

### Tier 2 — Emulation (`lib/posix-emu/`)
- **needs-emu**: Approximate mapping via stateful emulation with documented behavioural differences

### Tier 3 — Redesign (no library)
- **needs-redesign**: No library can bridge this; requires structural rewrite using patterns from `redesign-patterns.md`

### Legacy mapping
- Previous `blocking` severity maps to either `needs-emu` (Tier 2) or `needs-redesign` (Tier 3) depending on whether emulation is feasible. See `docs/posix-tiers.md` for the full classification.

## File I/O

| POSIX | AmigaOS Equivalent | Shim Wrapper | Severity | Notes |
|-------|-------------------|--------------|----------|-------|
| `open()` | `Open()` (dos.library) | `amiport_open()` | needs-shim | Mode flags differ: O_RDONLY→MODE_OLDFILE, O_WRONLY\|O_CREAT→MODE_NEWFILE |
| `close()` | `Close()` (dos.library) | `amiport_close()` | trivial | Direct mapping, but uses BPTR not fd |
| `read()` | `Read()` (dos.library) | `amiport_read()` | needs-shim | Returns LONG not ssize_t, -1 on error same |
| `write()` | `Write()` (dos.library) | `amiport_write()` | needs-shim | Same as read() |
| `lseek()` | `Seek()` (dos.library) | `amiport_lseek()` | needs-shim | Seek() returns OLD position, whence values differ |
| `fstat()` | `ExamineFH()` (dos.library 36+) | `amiport_fstat()` | needs-shim | Must build stat struct from FileInfoBlock |
| `stat()` | `Lock()`+`Examine()` | `amiport_stat()` | needs-shim | Two-step: lock file, examine, unlock |
| `unlink()` | `DeleteFile()` (dos.library) | `amiport_unlink()` | trivial | Direct mapping |
| `rename()` | `Rename()` (dos.library) | `amiport_rename()` | trivial | Direct mapping |
| `access()` | `Lock()`+`UnLock()` | `amiport_access()` | needs-shim | Only checks existence, not mode bits |
| `fcntl()` | — | — | needs-redesign | No equivalent; stub with errno ENOSYS (Tier 3) |
| `dup()`/`dup2()` | — | — | needs-shim | Planned Tier 1 addition via fd_table ref-counting |
| `pipe()` | PIPE: device | `amiport_emu_pipe()` | needs-emu | Tier 2 via PIPE: handler (AmigaOS 2.0+) |
| `mmap()` (read-only) | `AllocMem()`+`Read()` | `amiport_emu_mmap()` | needs-emu | Tier 2 for MAP_PRIVATE+PROT_READ only |
| `mmap()` (shared/write) | — | — | needs-redesign | Tier 3 — requires explicit file I/O redesign |

## Standard I/O (stdio.h)

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `fopen()` | Provided by clib2/newlib | — | trivial | C runtime handles this |
| `fclose()` | Provided by clib2/newlib | — | trivial | |
| `fread()`/`fwrite()` | Provided by clib2/newlib | — | trivial | |
| `fprintf()`/`printf()` | Provided by clib2/newlib | — | trivial | |
| `fgets()` | Provided by clib2/newlib | — | trivial | |
| `popen()` | — | — | needs-redesign | Tier 3 — requires SystemTags() + PIPE: redesign |
| `tmpfile()` | — | `amiport_tmpfile()` | needs-shim | Use T: assign for temp files. Declared in `amiport/unistd.h` |

## Directory Operations

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `opendir()` | `Lock()`+`Examine()` | `amiport_opendir()` | needs-shim | Amiga uses Lock/Examine/ExNext pattern |
| `readdir()` | `ExNext()` (dos.library) | `amiport_readdir()` | needs-shim | Fills FileInfoBlock, must wrap in dirent |
| `closedir()` | `UnLock()` (dos.library) | `amiport_closedir()` | needs-shim | Release the Lock |
| `mkdir()` | `CreateDir()` (dos.library) | `amiport_mkdir()` | needs-shim | CreateDir returns Lock that must be UnLock'd |
| `rmdir()` | `DeleteFile()` (dos.library) | `amiport_rmdir()` | trivial | Same function as file deletion |
| `getcwd()` | `GetCurrentDirName()` (dos 36+) | `amiport_getcwd()` | needs-shim | Or NameFromLock on pr_CurrentDir |
| `chdir()` | `CurrentDir()` (dos.library) | `amiport_chdir()` | needs-shim | Sets process current dir Lock |

## Process Management

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `fork()` | — | — | needs-redesign | Tier 3 — see redesign patterns (subprocess-and-wait, async-subprocess, daemon-fork) |
| `exec*()` | `SystemTags()` | — | needs-redesign | Tier 3 — can run command but not replace current process |
| `wait()`/`waitpid()` | — | — | needs-redesign | Tier 3 — no child process concept |
| `getpid()` | `FindTask(NULL)` | `amiport_getpid()` | needs-shim | Returns task pointer, not integer PID |
| `exit()` | `exit()` via clib2 | — | trivial | C runtime handles this |
| `system()` | `SystemTags()` | — | needs-shim | Different return value semantics |
| `getenv()`/`setenv()` | `GetVar()`/`SetVar()` (dos 36+) | `amiport_getenv()` | needs-shim | Amiga uses local/global vars |

## Signals

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `signal()` | `SetSignal()` (exec.library) | `amiport_signal()` | needs-shim | Only SIGINT maps (to SIGBREAKF_CTRL_C) |
| `raise()` | `Signal()` (exec.library) | `amiport_raise()` | needs-shim | Limited to Amiga signal flags |
| `kill()` | — | — | needs-redesign | Tier 3 — no inter-process signaling |
| `sigaction()` | — | — | needs-redesign | Tier 3 — no signal handlers; stub |
| `alarm()` | `timer.device` | `amiport_emu_alarm()` | needs-emu | Tier 2 — cooperative checking, not async |

## Memory

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `malloc()`/`free()` | `AllocVec()`/`FreeVec()` or clib2 | — | trivial | C runtime provides these |
| `calloc()`/`realloc()` | Provided by clib2 | — | trivial | |
| `mmap()` (read-only) | `AllocMem()`+`Read()` | `amiport_emu_mmap()` | needs-emu | Tier 2 for MAP_PRIVATE+PROT_READ |
| `mmap()` (shared) | — | — | needs-redesign | Tier 3 — requires explicit file I/O redesign |
| `mprotect()` | — | — | needs-redesign | Tier 3 — no memory protection on classic Amiga |

## String / Utility

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `getopt()` | — | `amiport_getopt()` | needs-shim | Not in AmigaOS; bundled in posix-shim |
| `regex` (POSIX) | — | — | needs-shim | Must bundle a regex library |
| `strdup()` | Provided by clib2 | — | trivial | |
| `strtok_r()` | — | `amiport_strtok_r()` | needs-shim | Not always available; simple to implement |

## Time

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `time()` | `DateStamp()` (dos.library) | `amiport_time()` | needs-shim | DateStamp uses days/minutes/ticks |
| `gettimeofday()` | `timer.device` | `amiport_gettimeofday()` | needs-shim | Requires timer device |
| `sleep()` | `Delay()` (dos.library) | `amiport_sleep()` | needs-shim | Delay() uses ticks (1/50s PAL, 1/60s NTSC) |
| `usleep()` | `timer.device` | `amiport_usleep()` | needs-shim | Requires timer device for microsecond precision |

## Networking

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `socket()` | bsdsocket.library | — | needs-redesign | Tier 3 — see bsdsocket redesign pattern |
| `connect()`/`bind()` | bsdsocket.library | — | needs-redesign | Tier 3 |
| `select()` (on files) | `WaitForChar()` | `amiport_emu_select()` | needs-emu | Tier 2 for file I/O; sockets need bsdsocket.library |
| `select()` (on sockets) | `WaitSelect()` | — | needs-redesign | Tier 3 — bsdsocket.library provides this |

## Headers to Replace

When transforming source, these `#include` directives indicate POSIX dependencies:

```
#include <unistd.h>      → #include <amiport/unistd.h>
#include <dirent.h>       → #include <amiport/dirent.h>
#include <sys/stat.h>     → #include <amiport/sys/stat.h>
#include <sys/wait.h>     → remove (stub definitions in amiport/unistd.h)
#include <sys/time.h>     → #include <amiport/sys/time.h>
#include <signal.h>       → #include <amiport/signal.h>
#include <getopt.h>       → #include <amiport/getopt.h>
#include <fcntl.h>        → #include <amiport/unistd.h> (O_* flags defined there)
#include <dlfcn.h>        → remove (no dynamic loading on classic Amiga)
#include <pthread.h>      → remove (no pthreads; blocking)
#include <sys/mman.h>     → remove (no mmap; blocking)
```
