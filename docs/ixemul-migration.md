# Migrating from ixemul to amiport

This guide is for Amiga developers who have code that depends on `ixemul.library`
and want to switch to `-noixemul` builds using the amiport POSIX shim.

## Why migrate?

| | ixemul.library | amiport (-noixemul) |
|---|---|---|
| **Runtime dependency** | Requires ixemul.library installed | Self-contained — no shared library needed |
| **Binary size** | Smaller binary (shared lib) | Larger binary (statically linked) |
| **POSIX coverage** | Most comprehensive | ~40 functions (growing) |
| **Maintenance** | Unmaintained since ~2005 | Actively developed |
| **Compatibility** | Can conflict with other libs | No conflicts — pure static linking |
| **68000 support** | Yes | Yes (targets 68000 by default) |
| **Process model** | Emulates fork/exec | Does not emulate fork/exec (Tier 3 redesign) |

**Choose amiport when:** you want self-contained binaries, don't need fork/exec, and
prefer a lightweight dependency. Most CLI tool ports fit this profile.

**Stay with ixemul when:** your code heavily uses fork/exec/pthreads and you can't
redesign those patterns.

## Migration steps

### 1. Switch compiler flags

```makefile
# Before (ixemul)
CFLAGS = -O2
LDFLAGS =

# After (amiport)
CFLAGS = -O2 -noixemul -I/path/to/lib/posix-shim/include
LDFLAGS = -L/path/to/lib/posix-shim -lamiport -noixemul
```

### 2. Replace headers

| ixemul header | amiport header |
|---|---|
| `<unistd.h>` (for open/close/read/write) | `<amiport/unistd.h>` |
| `<sys/stat.h>` | `<amiport/sys/stat.h>` |
| `<dirent.h>` | `<amiport/dirent.h>` |
| `<regex.h>` | `<amiport-emu/regex.h>` (Tier 2) |
| `<sys/mman.h>` | `<amiport-emu/mmap.h>` (Tier 2, read-only) |

Or use the umbrella header: `#include <amiport/amiport.h>`

### 3. Replace function calls

Standard C functions (`printf`, `malloc`, `strcmp`, etc.) work identically —
libnix provides them. Only POSIX/Unix-specific functions need replacement:

| ixemul function | amiport replacement |
|---|---|
| `open(path, flags)` | `amiport_open(path, flags)` |
| `close(fd)` | `amiport_close(fd)` |
| `read(fd, buf, n)` | `amiport_read(fd, buf, n)` |
| `write(fd, buf, n)` | `amiport_write(fd, buf, n)` |
| `stat(path, &sb)` | `amiport_stat(path, &sb)` |
| `opendir(path)` | `amiport_opendir(path)` |
| `readdir(dp)` | `amiport_readdir(dp)` |
| `closedir(dp)` | `amiport_closedir(dp)` |
| `getopt(argc, argv, opts)` | `amiport_getopt(argc, argv, opts)` |
| `sleep(n)` | `amiport_sleep(n)` |
| `getenv(name)` | `amiport_getenv(name)` |
| `strlcpy(dst, src, n)` | `amiport_strlcpy(dst, src, n)` |
| `regcomp(preg, pat, flags)` | `amiport_emu_regcomp(preg, pat, flags)` |

The `amiport/stdio.h` header provides `#define fopen(...) amiport_fopen(...)` macros
that make the replacement transparent for some functions.

### 4. Handle unsupported functions

Functions that ixemul emulates but amiport does not:

| Function | ixemul | amiport | Action |
|---|---|---|---|
| `fork()` | Emulated (slow) | Not available | Use `SystemTags()` for subprocess-and-wait |
| `exec*()` | Emulated | Not available | Use `SystemTags()` or `CreateNewProcTags()` |
| `waitpid()` | Emulated | Not available | `SystemTags()` returns exit code directly |
| `pipe()` | Emulated | Tier 2 (`PIPE:` device) | Works for simple cases |
| `mmap(SHARED)` | Emulated | Not available | Use explicit `Read()`/`Write()` |
| `pthreads` | Emulated | Not available | Remove threading or use Exec tasks |
| `signal()` | Full emulation | SIGINT only | Only Ctrl-C handling supported |

See `docs/posix-tiers.md` for Tier 3 redesign patterns with code examples.

### 5. Add Amiga conventions

```c
/* Version string — required for Amiga programs */
static const char *verstag = "$VER: myprog 1.0 (21.03.2026)";

/* Stack size — Amiga default is 4KB, most ports need more */
long __stack = 32768;

/* Exit codes — use Amiga values, not POSIX */
/* exit(0)  = RETURN_OK   (fine)       */
/* exit(5)  = RETURN_WARN              */
/* exit(10) = RETURN_ERROR (not exit(1)) */
/* exit(20) = RETURN_FAIL              */
```

## What you get

The amiport shim currently provides 39 Tier 1 functions (direct mapping) and 6 Tier 2
modules (approximate emulation). Run `scripts/shim-coverage.sh` for the current
coverage report.

Full function list: see `lib/posix-shim/README.md` and `lib/posix-emu/README.md`.

## Resources

- `docs/posix-tiers.md` — Complete tier classification for every POSIX function
- `docs/references/newlib-availability.md` — What libnix provides natively
- `docs/references/bsd-isms.md` — BSD-specific function handling
- `lib/posix-shim/README.md` — Tier 1 function inventory
- `lib/posix-emu/README.md` — Tier 2 emulation modules
