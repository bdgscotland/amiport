# POSIX-to-AmigaOS Function Mapping

This is the master reference for mapping POSIX/Linux system calls to their AmigaOS equivalents
or amiport shim wrappers. Used by the `analyze-source` and `transform-source` skills.

## Severity Levels

- **trivial**: Direct mapping exists, mechanical replacement
- **needs-shim**: Requires amiport shim wrapper (behavior differs)
- **blocking**: No Amiga equivalent — requires redesign or stubbing

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
| `fcntl()` | — | — | blocking | No equivalent; stub with errno ENOSYS |
| `dup()`/`dup2()` | — | — | blocking | No file descriptor table; stub or redesign |
| `pipe()` | — | — | blocking | No pipes in classic AmigaOS |
| `mmap()` | — | — | blocking | No memory-mapped files |

## Standard I/O (stdio.h)

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `fopen()` | Provided by clib2/newlib | — | trivial | C runtime handles this |
| `fclose()` | Provided by clib2/newlib | — | trivial | |
| `fread()`/`fwrite()` | Provided by clib2/newlib | — | trivial | |
| `fprintf()`/`printf()` | Provided by clib2/newlib | — | trivial | |
| `fgets()` | Provided by clib2/newlib | — | trivial | |
| `popen()` | — | — | blocking | No pipes; requires SystemTags() workaround |
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
| `fork()` | — | — | blocking | No equivalent. Amiga uses CreateNewProc() but semantics differ completely |
| `exec*()` | `SystemTags()` | — | blocking | Can run a command but not replace current process |
| `wait()`/`waitpid()` | — | — | blocking | No child process concept |
| `getpid()` | `FindTask(NULL)` | `amiport_getpid()` | needs-shim | Returns task pointer, not integer PID |
| `exit()` | `exit()` via clib2 | — | trivial | C runtime handles this |
| `system()` | `SystemTags()` | — | needs-shim | Different return value semantics |
| `getenv()`/`setenv()` | `GetVar()`/`SetVar()` (dos 36+) | `amiport_getenv()` | needs-shim | Amiga uses local/global vars |

## Signals

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `signal()` | `SetSignal()` (exec.library) | `amiport_signal()` | needs-shim | Only SIGINT maps (to SIGBREAKF_CTRL_C) |
| `raise()` | `Signal()` (exec.library) | `amiport_raise()` | needs-shim | Limited to Amiga signal flags |
| `kill()` | — | — | blocking | No inter-process signaling |
| `sigaction()` | — | — | blocking | No signal handlers; stub |
| `alarm()` | `timer.device` | — | blocking | Requires timer device setup; stub for simple cases |

## Memory

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `malloc()`/`free()` | `AllocVec()`/`FreeVec()` or clib2 | — | trivial | C runtime provides these |
| `calloc()`/`realloc()` | Provided by clib2 | — | trivial | |
| `mmap()` | — | — | blocking | No memory-mapped files or shared memory |
| `mprotect()` | — | — | blocking | No memory protection on classic Amiga |

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
| `socket()` | bsdsocket.library | — | blocking | Separate library, not shimmed in MVP |
| `connect()`/`bind()` | bsdsocket.library | — | blocking | |
| `select()` | `WaitSelect()` | — | blocking | bsdsocket.library provides this |

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
