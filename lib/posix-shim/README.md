# posix-shim — Tier 1 POSIX Compatibility Library for AmigaOS

`libamiport.a` provides direct POSIX-to-AmigaOS function wrappers. Each function maps
one-to-one to an AmigaOS equivalent with matching semantics for common use cases.

## Quick Start

```c
#include <amiport/amiport.h>   /* all Tier 1 functions */
```

Link with: `-L../../lib/posix-shim -lamiport`

## Architecture

```
  POSIX program          amiport shim              AmigaOS
  ─────────────          ────────────              ───────
  open(path, flags)  →   amiport_open()        →   Open(path, mode)
  read(fd, buf, n)   →   amiport_read()        →   Read(fh, buf, n)
  stat(path, &sb)    →   amiport_stat()        →   Lock() + Examine()
  getopt(argc, argv) →   amiport_getopt()      →   Pure C implementation
```

The shim maintains an internal file descriptor table (`fd_table`) that maps POSIX
integer fds to AmigaOS `BPTR` file handles.

## Implemented Functions (39)

### File I/O (`file_io.c`)
| POSIX | Shim | AmigaDOS |
|-------|------|----------|
| `open()` | `amiport_open()` | `Open()` with flag mapping |
| `close()` | `amiport_close()` | `Close()` — protects fd 0/1/2 |
| `read()` | `amiport_read()` | `Read()` |
| `write()` | `amiport_write()` | `Write()` |
| `lseek()` | `amiport_lseek()` | `Seek()` — handles old-position return |
| `dup()` | `amiport_dup()` | fd_table sharing + scan-on-close |
| `dup2()` | `amiport_dup2()` | fd_table sharing + scan-on-close |
| `unlink()` | `amiport_unlink()` | `DeleteFile()` |
| `rename()` | `amiport_rename()` | `Rename()` |
| `access()` | `amiport_access()` | `Lock()` test |
| `isatty()` | `amiport_isatty()` | `IsInteractive()` |
| `pread()` | `amiport_pread()` | `Seek()+Read()+Seek()` (non-atomic) |
| `pwrite()` | `amiport_pwrite()` | `Seek()+Write()+Seek()` (non-atomic) |
| `tmpfile()` | `amiport_tmpfile()` | `Open("T:...")` |
| `mkstemp()` | `amiport_mkstemp()` | Unique name via task address + counter |
| `chmod()` | `amiport_chmod()` | No-op stub — Amiga protection bits have inverted semantics |
| `realpath()` | `amiport_realpath()` | `Lock()+NameFromLock()` — NULL resolved arg malloc's buffer |

### Environment (`process.c`)
| POSIX | Shim | AmigaDOS |
|-------|------|----------|
| `getenv()` | `amiport_getenv()` | `GetVar()` — returns malloc'd string |
| `setenv()` | `amiport_setenv()` | `SetVar()` + `GVF_GLOBAL_ONLY` — writes to ENV: |
| `unsetenv()` | `amiport_unsetenv()` | `DeleteVar()` + `GVF_GLOBAL_ONLY` — succeeds if not set |

### Directory Operations (`dir_ops.c`)
| POSIX | Shim | AmigaDOS |
|-------|------|----------|
| `opendir()` | `amiport_opendir()` | `Lock()+Examine()` |
| `readdir()` | `amiport_readdir()` | `ExNext()` |
| `closedir()` | `amiport_closedir()` | `UnLock()` |
| `mkdir()` | `amiport_mkdir()` | `CreateDir()` — mode bits ignored |
| `rmdir()` | `amiport_rmdir()` | `DeleteFile()` — same call as unlink on AmigaOS |
| `getcwd()` | `amiport_getcwd()` | `GetCurrentDirName()` |
| `chdir()` | `amiport_chdir()` | `CurrentDir()+Lock()` |

### File Status (`stat.c`)
| POSIX | Shim | AmigaDOS |
|-------|------|----------|
| `stat()` | `amiport_stat()` | `Lock()+Examine()` — DateStamp→Unix timestamp |
| `fstat()` | `amiport_fstat()` | `ExamineFH()` — requires AmigaOS 2.0+ |
| `lstat()` | `amiport_lstat()` | Alias to `amiport_stat()` — classic FFS has no symlinks |

### String Safety (`string_bsd.c`)
| Function | Shim | Notes |
|----------|------|-------|
| `strlcpy()` | `amiport_strlcpy()` | BSD safe string copy |
| `strlcat()` | `amiport_strlcat()` | BSD safe string concatenation |
| `reallocarray()` | `amiport_reallocarray()` | Overflow-checked realloc |
| `strtok_r()` | `amiport_strtok_r()` | Thread-safe strtok |
| `strsep()` | `amiport_strsep()` | BSD string tokenizer |
| `strndup()` | `amiport_strndup()` | Bounded string duplication |

### stdio Extensions (`stdio_ext.c`)
| Function | Shim | Notes |
|----------|------|-------|
| `asprintf()` | `amiport_asprintf()` | Dynamic formatted string |
| `vasprintf()` | `amiport_vasprintf()` | va_list variant |
| `getline()` | `amiport_getline()` | GNU extension, fgets-based |
| `getdelim()` | `amiport_getdelim()` | Read until delimiter |

### Pattern Matching (`fnmatch.c`, `scandir.c`)
| Function | Shim | Notes |
|----------|------|-------|
| `fnmatch()` | `amiport_fnmatch()` | Shell-style glob matching |
| `scandir()` | `amiport_scandir()` | With filter and sort callbacks |
| `alphasort()` | `amiport_alphasort()` | strcmp wrapper for scandir |

### Process / Misc (`process.c`, `signal_emu.c`, `time_funcs.c`)
| Function | Shim | Notes |
|----------|------|-------|
| `sleep()` | `amiport_sleep()` | `Delay()` — 1/50s granularity |
| `usleep()` | `amiport_usleep()` | `Delay()` — rounds up |
| `getenv()` | `amiport_getenv()` | `GetVar()` — returns malloc'd string |
| `getpid()` | `amiport_getpid()` | `FindTask(NULL)` as int |
| `time()` | `amiport_time()` | `DateStamp()` + epoch offset |
| `gettimeofday()` | `amiport_gettimeofday()` | Approximate microseconds |
| `signal(SIGINT)` | `amiport_signal()` | `SetSignal()` / Ctrl-C |
| `raise(SIGINT)` | `amiport_raise()` | `Signal()` |
| `getopt()` | `amiport_getopt()` | Pure C implementation |

### Error Reporting (`err.h` — header-only)
| Function | Notes |
|----------|-------|
| `err()`, `errx()`, `warn()`, `warnx()`, `warnc()` | BSD error reporting |
| `strtonum()` | Safe string-to-number with range check |
| `setprogname()`, `getprogname()` | Program name tracking |

## Headers

| Header | Purpose |
|--------|---------|
| `amiport/amiport.h` | Umbrella — includes all Tier 1 headers |
| `amiport/unistd.h` | File I/O, process, misc |
| `amiport/dirent.h` | Directory operations |
| `amiport/stdio.h` | fopen/fclose macro wrappers |
| `amiport/stdio_ext.h` | asprintf, getline, mkstemp, pread/pwrite |
| `amiport/string.h` | strlcpy, strlcat, reallocarray, strsep, strndup |
| `amiport/err.h` | BSD error reporting + strtonum |
| `amiport/signal.h` | SIGINT mapping |
| `amiport/getopt.h` | getopt() |
| `amiport/fnmatch.h` | Shell glob matching |
| `amiport/scandir.h` | scandir + alphasort |
| `amiport/errno_map.h` | AmigaDOS→POSIX errno mapping |
| `amiport/sys/stat.h` | stat/fstat + struct stat |
| `amiport/sys/time.h` | gettimeofday |
| `amiport/internal.h` | Private — fd_table access for Tier 2 |

## Building

```bash
make build-shim    # from project root
# or
make               # from lib/posix-shim/
```

Produces `libamiport.a`. Requires bebbo-gcc cross-compiler (via Docker).

## Testing

```bash
make test-shim     # from project root — runs all tests via vamos
```

Test files in `tests/shim/` (11 test files covering all modules).

## Tier Model

This library is **Tier 1** — direct mapping with matching semantics. For approximate
emulation (Tier 2), see `lib/posix-emu/`. For functions requiring structural redesign
(Tier 3), see `docs/posix-tiers.md`.
