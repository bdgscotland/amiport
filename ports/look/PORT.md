# Port: look

## Overview

| Field | Value |
|-------|-------|
| Program | look |
| Version | 1.27 |
| Source | OpenBSD usr.bin/look (v1.27, Dec 2022) |
| Category | 1 -- CLI tool |
| License | BSD 3-Clause |
| Original Author | David Hitz (Auspex Systems), The Regents of the University of California |
| Port Date | 2026-03-26 |

## Description

look performs a binary search on a sorted file for lines beginning with a given string prefix. It supports dictionary mode (`-d`, ignoring non-alphanumeric characters), case-insensitive matching (`-f`), and a termination character (`-t`) to truncate the search string at a delimiter. On Unix, the default file is `/usr/share/dict/words`; on this AmigaOS port, it defaults to `WORK:words`.

## Prior Art on Aminet

Researched via aminet-researcher agent. No look utility found on Aminet. AmigaOS has no built-in equivalent for binary-search prefix matching on sorted files.

## Portability Analysis

Verdict: **MODERATE** -- Single file, but uses `mmap()` for file access and `<stdint.h>` for `SIZE_MAX`. The mmap dependency is the main porting challenge.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<sys/mman.h>` / `mmap()` / `munmap()` | Tier 2 | `<amiport-emu/mmap.h>` -- emulated via AllocMem()+Read() |
| `<sys/stat.h>` / `fstat()` | Tier 1 | `<amiport/sys/stat.h>` with `AMIPORT_NO_STAT_MACROS` to avoid `fcntl.h` conflict |
| `<stdint.h>` / `SIZE_MAX` | Tier 1 | Removed -- replaced with negative `st_size` sanity check |
| `open()` / `O_RDONLY` | Tier 1 | `amiport_open()` via macro in `<amiport/unistd.h>` |
| `err()` / `errc()` | Tier 1 | `<amiport/err.h>` |
| `getopt()` | Tier 1 | System `<unistd.h>` getopt (short options only) |
| `_PATH_WORDS` | Tier 1 | Remapped to `"WORK:words"` in `pathnames.h` |
| `pledge()` | Stub | `#define pledge(p, e) (0)` |
| Exit codes | Tier 1 | 0=found (OK), 1->5 (RETURN_WARN/not found), 2->20 (RETURN_FAIL/error) |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| look.c | 45 | (none) | `$VER: look 1.27` | Amiga version string |
| look.c | 48 | (none) | `long __stack = 16384` | Stack cookie |
| look.c | 51 | (none) | `#define pledge(p, e) (0)` | Stubbed |
| look.c | 54-55 | `<sys/mman.h>` | removed; `<amiport-emu/mmap.h>` added | mmap emulation |
| look.c | 57-58 | `<sys/stat.h>` | `AMIPORT_NO_STAT_MACROS` + `<amiport/sys/stat.h>` | Avoid fcntl.h conflict |
| look.c | 64 | `<stdint.h>` | removed | SIZE_MAX check replaced |
| look.c | 65-71 | system headers | amiport shim headers | stdlib.h, unistd.h, err.h, glob.h, mmap.h |
| look.c | 88-94 | (none) | `cleanup()` with `amiport_free_argv()` | atexit cleanup |
| look.c | 99 | `struct stat sb` | `struct amiport_stat sb` | Type replacement |
| look.c | 105-107 | (none) | `amiport_expand_argv()` + `atexit(cleanup)` | Wildcard expansion |
| look.c | 150-151 | `open()` + `fstat()` | `open()` macro + `amiport_fstat()` direct call | AMIPORT_NO_STAT_MACROS requires explicit call |
| look.c | 158-160 | `if (sb.st_size > SIZE_MAX)` | `if (sb.st_size < 0)` | SIZE_MAX unavailable; negative = overflow on 32-bit |
| look.c | 172-176 | `mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0)` | `amiport_emu_mmap(NULL, ..., AMIPORT_EMU_PROT_READ, AMIPORT_EMU_MAP_PRIVATE, fd, 0)` | Tier 2 emulation |
| look.c | 182 | `munmap()` | `amiport_emu_munmap()` | Paired cleanup |
| look.c | 191-194 | `exit(1)` | `exit(5)` | RETURN_WARN for "not found" |
| look.c | 324-326 | `err(2, "stdout")` | `err(10, "stdout")` | RETURN_ERROR |
| look.c | 367 | `exit(2)` | `exit(20)` | RETURN_FAIL for usage error |
| pathnames.h | 36 | `"/usr/share/dict/words"` | `"WORK:words"` | Amiga path |

## Shim Functions Exercised

- `amiport_open()`, `amiport_fstat()`
- `amiport_emu_mmap()`, `amiport_emu_munmap()`
- `amiport_err()`, `amiport_errc()`
- `amiport_expand_argv()`, `amiport_free_argv()`

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall -I../../lib/posix-emu/include` |
| Stack | 16384 bytes |
| Libraries | `-lamiport-emu -lamiport` (Tier 2 mmap emulation + Tier 1 posix-shim) |
| Binary size | ~38KB |

## Test Results

**36/36 passed** (100%) on FS-UAE (A1200, Kickstart 3.1).

| Category | Count | Description |
|----------|-------|-------------|
| Functional (-d, -f, -t, -df) | 7 | Dictionary mode, case-insensitive, termchar, combined |
| Multi-line output | 4 | Prefix matching returning multiple lines |
| Exit codes | 5 | RC=0 found, RC=5 not found, RC=20 error/usage |
| Error paths | 1 | Too many arguments |
| Edge cases | 7 | Empty file, single-line, first/last entry, colon delimiter, mixed case |
| Amiga-specific | 2 | WORK: volume paths, missing default words file |
| Real-world | 3 | Dictionary search, key:value lookup, punctuation normalization |
| Stress | 5 | Binary search on 192-entry file (start/middle/end/no-match/exact) |
| Precision | 1 | -t flag truncation at specific character |

## Memory Safety

- `amiport_emu_munmap()` frees the mmap'd buffer after use
- `amiport_free_argv()` via `atexit(cleanup)` on all exit paths
- No other dynamic allocations -- look operates entirely on the mmap'd buffer

## Performance Notes

- `print_from()` uses `putchar()` for character-by-character output. Replacing with `fwrite()` for entire lines would save ~N function calls per output line on 68000 (where N is average line length). Low priority since look typically outputs few lines.
- The binary search + linear search pattern is efficient for sorted files. The mmap emulation loads the entire file into memory upfront (no lazy paging), which is fine for typical dictionary/word files.

## Known Limitations

1. **mmap replaced with full file read** -- `amiport_emu_mmap()` reads the entire file into AllocMem'd memory. This means look cannot efficiently handle files larger than available RAM. For typical sorted text files (a few hundred KB), this is not a concern.
2. **No system words file** -- AmigaOS has no `/usr/share/dict/words`. When invoked without a file argument, look uses `WORK:words` as the default path. Users must supply their own word list.
3. **Empty file returns RC=20** -- On Unix, an empty file returns RC=1 (not found). On this port, `amiport_emu_mmap()` of a zero-length file returns MAP_FAILED (cannot AllocMem zero bytes), triggering the `err(20, ...)` path. This is a minor behavioral difference.
4. **No POSIX locale collation** -- Comparisons use C locale byte ordering. The `-f` flag performs ASCII `tolower()` only.

## Review

Reviewed with `/review-amiga`. Score: **READY**.

| Dimension | Score |
|-----------|-------|
| Stack safety | OK (16KB cookie, no large local buffers) |
| Memory handling | OK (mmap buffer freed via munmap, atexit cleanup) |
| Path handling | OK (pathnames.h remapped to WORK:words) |
| Library cleanup | OK (no direct AmigaOS lib opens beyond amiport shims) |
| Conventions | OK (version string, 3-tier exit codes, stack cookie) |
