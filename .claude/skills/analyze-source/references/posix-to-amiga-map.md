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
| `regex` (POSIX) | — | `amiport_emu_regcomp()` etc. | needs-emu | Tier 2 regex emulation in `lib/posix-emu/` |
| `strdup()` | Provided by clib2 | — | trivial | |
| `strtok_r()` | — | `amiport_strtok_r()` | needs-shim | Not always available; simple to implement |
| `strlcpy()` | — | `amiport_strlcpy()` | needs-shim | BSD safe string copy |
| `strlcat()` | — | `amiport_strlcat()` | needs-shim | BSD safe string concatenation |
| `reallocarray()` | — | `amiport_reallocarray()` | needs-shim | OpenBSD overflow-checked realloc |
| `asprintf()` | — | `amiport_asprintf()` | needs-shim | Dynamic string formatting |
| `vasprintf()` | — | `amiport_vasprintf()` | needs-shim | va_list variant |
| `mkstemp()` | — | `amiport_mkstemp()` | needs-shim | Secure temp file via T: assign |
| `pread()` | — | `amiport_pread()` | needs-shim | Positional read (non-atomic) |
| `pwrite()` | — | `amiport_pwrite()` | needs-shim | Positional write (non-atomic) |
| `fnmatch()` | — | `amiport_fnmatch()` | needs-shim | Shell-style glob matching |
| `scandir()` | — | `amiport_scandir()` | needs-shim | Directory scanning with filter/sort |

## Time

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `time()` | `DateStamp()` (dos.library) | `amiport_time()` | needs-shim | DateStamp uses days/minutes/ticks |
| `gettimeofday()` | `timer.device` | `amiport_gettimeofday()` | needs-shim | Requires timer device |
| `sleep()` | `Delay()` (dos.library) | `amiport_sleep()` | needs-shim | Delay() uses ticks (1/50s PAL, 1/60s NTSC) |
| `usleep()` | `timer.device` | `amiport_usleep()` | needs-shim | Requires timer device for microsecond precision |

## Networking (Category 4 — see ADR-010)

| POSIX | AmigaOS | Shim | Severity | Notes |
|-------|---------|------|----------|-------|
| `socket()` | `socket()` via bsdsocket.library | `amiport_socket()` | needs-shim | Auto library lifecycle via bsdsocket-shim |
| `connect()` | `connect()` via bsdsocket.library | `amiport_connect()` | needs-shim | Identical API |
| `bind()` | `bind()` via bsdsocket.library | `amiport_bind()` | needs-shim | Identical API |
| `listen()` | `listen()` via bsdsocket.library | `amiport_listen()` | needs-shim | Identical API |
| `accept()` | `accept()` via bsdsocket.library | `amiport_accept()` | needs-shim | Identical API |
| `send()` | `send()` via bsdsocket.library | `amiport_send()` | needs-shim | Identical API |
| `recv()` | `recv()` via bsdsocket.library | `amiport_recv()` | needs-shim | Identical API |
| `sendto()` | `sendto()` via bsdsocket.library | `amiport_sendto()` | needs-shim | Identical API |
| `recvfrom()` | `recvfrom()` via bsdsocket.library | `amiport_recvfrom()` | needs-shim | Identical API |
| `close()` (socket) | `CloseSocket()` | `amiport_closesocket()` | needs-shim | **Must use CloseSocket, not close!** |
| `setsockopt()` | `setsockopt()` via bsdsocket.library | `amiport_setsockopt()` | needs-shim | Identical API |
| `getsockopt()` | `getsockopt()` via bsdsocket.library | `amiport_getsockopt()` | needs-shim | Identical API |
| `shutdown()` | `shutdown()` via bsdsocket.library | `amiport_shutdown()` | needs-shim | Identical API |
| `select()` (sockets) | `WaitSelect()` | `amiport_net_select()` | needs-shim | Extra signal mask param handled by shim |
| `select()` (files) | `WaitForChar()` | `amiport_emu_select()` | needs-emu | Tier 2 for file I/O only |
| `gethostbyname()` | `gethostbyname()` via bsdsocket.library | `amiport_gethostbyname()` | needs-shim | Identical API |
| `gethostbyaddr()` | `gethostbyaddr()` via bsdsocket.library | `amiport_gethostbyaddr()` | needs-shim | Identical API |
| `getaddrinfo()` | — | — | needs-redesign | Not in bsdsocket.library — use gethostbyname |
| `inet_addr()` | Pure C in shim | `amiport_inet_addr()` | needs-shim | No library needed |
| `inet_ntoa()` | Pure C in shim | `amiport_inet_ntoa()` | needs-shim | No library needed |
| `inet_aton()` | Pure C in shim | `amiport_inet_aton()` | needs-shim | No library needed |

## Console UI (Category 3 — see ADR-009)

| POSIX/ncurses | AmigaOS | Shim | Severity | Notes |
|---------------|---------|------|----------|-------|
| `initscr()` | Console setup + RAW: mode | via console-shim | needs-shim | Opens raw console, allocates stdscr |
| `endwin()` | Restore console | via console-shim | needs-shim | Restores cooked mode |
| `getch()` | `Read()` in RAW: mode | via console-shim | needs-shim | Decodes ANSI escape sequences for keys |
| `addch()`/`addstr()` | ANSI output | via console-shim | trivial | Direct ANSI escape mapping |
| `move()` | `ESC[y;xH` | via console-shim | trivial | Direct ANSI cursor positioning |
| `attron()`/`attroff()` | ANSI SGR sequences | via console-shim | trivial | Bold, underline, reverse, color |
| `start_color()`/`init_pair()` | ANSI color codes | via console-shim | trivial | 8 ANSI colors supported |
| `clear()`/`erase()` | `ESC[2J` | via console-shim | trivial | Direct ANSI clear |
| `refresh()` | Diff-based output | via console-shim | needs-shim | Compares buffer, emits changes |
| `newwin()`/`subwin()` | Buffer management | via console-shim | needs-shim | Window buffer allocation |
| `keypad()` | ANSI key decoding | via console-shim | needs-shim | Arrow/function key sequences |
| `curs_set()` | `ESC[?25h/l` | via console-shim | needs-shim | Partial support |
| `box()`/`wborder()` | ASCII characters | via console-shim | trivial | Uses +, -, | fallbacks |
| `mousemask()` | — | — | needs-redesign | No mouse support in console.device |
| `newterm()` | — | — | needs-redesign | No multiple terminal support |

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

# Console UI (Category 3 — link with -lamiport-console):
#include <curses.h>       → #include <amiport-console/curses.h>
#include <ncurses.h>      → #include <amiport-console/curses.h>
#include <term.h>         → #include <amiport-console/term.h>

# Networking (Category 4 — link with -lamiport-net):
#include <sys/socket.h>   → #include <amiport-net/socket.h>
#include <netinet/in.h>   → #include <amiport-net/netinet/in.h>
#include <netdb.h>        → #include <amiport-net/netdb.h>
#include <arpa/inet.h>    → #include <amiport-net/arpa/inet.h>
```
