# mini-find — Simplified Find Utility (amiport example)

A simplified `find` utility that recursively walks a directory tree, exercising
directory traversal, stat, and getopt through the posix-shim.

## Files

- `original/mini-find.c` — Original POSIX source
- `ported/mini-find.c` — Amiga-compatible source (transformed by amiport)
- `Makefile` — Cross-compilation build file

## Analysis Verdict: EASY

All POSIX calls used in mini-find have direct equivalents in the amiport
posix-shim library. No functionality needs to be stubbed or removed.

## Transformation Table

| Original (POSIX)           | Ported (amiport)                     | Notes                          |
|----------------------------|--------------------------------------|--------------------------------|
| `#include <dirent.h>`      | `#include <amiport/dirent.h>`        | Directory operations shim      |
| `#include <sys/stat.h>`    | `#include <amiport/sys/stat.h>`      | Stat shim                      |
| `#include <getopt.h>`      | `#include <amiport/getopt.h>`        | Getopt shim                    |
| `DIR`                      | `AMIPORT_DIR`                        | Opaque directory handle         |
| `struct dirent`            | `struct amiport_dirent`              | Directory entry struct          |
| `struct stat`              | `struct amiport_stat`                | File status struct              |
| `opendir()`                | `amiport_opendir()`                  | Uses AmigaDOS Lock()            |
| `readdir()`                | `amiport_readdir()`                  | Uses AmigaDOS ExNext()          |
| `closedir()`               | `amiport_closedir()`                 | Releases lock                   |
| `stat()`                   | `amiport_stat()`                     | Uses Lock()/Examine()           |
| `getopt()`                 | `amiport_getopt()`                   | Portable getopt implementation  |
| `optarg`                   | `amiport_optarg`                     | Getopt argument pointer         |
| `optind`                   | `amiport_optind`                     | Getopt index                    |
| `S_ISDIR()`                | `AMIPORT_S_ISDIR()`                  | Directory type check macro      |
| `S_ISREG()`                | `AMIPORT_S_ISREG()`                  | Regular file type check macro   |
| _(none)_                   | `#include <exec/types.h>`            | Added for LONG type             |
| _(none)_                   | `static const char *verstag = ...`   | Amiga version string            |
| _(none)_                   | `LONG __stack = 65536`               | Stack cookie for recursion      |

## Shim Functions Exercised

- `amiport_opendir()` — Open directory for enumeration
- `amiport_readdir()` — Read next directory entry
- `amiport_closedir()` — Close directory handle
- `amiport_stat()` — Get file type and size information
- `amiport_getopt()` — Parse command-line options

## Build

```bash
# From the project root:
make build TARGET=examples/mini-find

# Or from this directory:
make
```

## Test

```bash
make test
# Requires vamos (pip install amitools)
```

The test target creates a temporary directory tree and runs mini-find with
various option combinations: list all, files only, directories only, name
pattern matching, and error handling for nonexistent directories.

## Known Limitations

- **No symlink support.** Classic AmigaOS FFS does not support symbolic links.
  The original POSIX version would follow symlinks; this is a non-issue on
  AmigaOS since they do not exist in the target filesystem.

- **Simplified pattern matching.** Only `*` prefix and suffix globbing is
  supported (e.g., `*.c`, `test*`). Wildcards in the middle of a pattern
  (e.g., `foo*bar`) are not matched. Full AmigaDOS pattern matching could be
  added via `MatchPattern()` if needed.

- **Deep directory trees may need larger stack.** The default stack cookie is
  set to 64KB, which should handle most directory structures. Extremely deep
  trees (100+ levels) may require increasing `__stack`.

- **Path separator.** The ported version uses `/` as the path separator in
  output, which works on AmigaOS but differs from the native `:` convention
  for volume-relative paths.
