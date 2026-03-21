# Port: sed

## Overview

| Field | Value |
|-------|-------|
| Program | sed |
| Version | 1.0 |
| Source | OpenBSD src (usr.bin/sed, v1.47 2024/07/17) |
| Category | 1 — CLI tool |
| License | BSD 3-Clause |
| Original Author | Diomidis Spinellis / University of California, Berkeley |
| Port Date | 2026-03-20 |

## Description

sed is the POSIX stream editor — a non-interactive text transformation tool that reads input line by line, applies editing commands (substitution, deletion, insertion, transliteration), and writes the result to stdout. This is the OpenBSD implementation, which includes extended regex support (-E), in-place editing (-i), and unbuffered output (-u).

## Prior Art on Aminet

No standalone sed port exists on Aminet for AmigaOS 3.x. Some Amiga Unix-compatibility packages (e.g., Geek Gadgets) include sed, but those are bundled distributions rather than individual ports. This port provides a modern, standalone sed binary.

## Portability Analysis

Verdict: **MODERATE** — mostly Tier 1 shim transforms with Tier 2 regex emulation.

| Issue | Tier | Resolution |
|-------|------|------------|
| `<regex.h>` (regcomp, regexec, etc.) | Tier 2 | `amiport-emu/regex.h` — emulated POSIX regex |
| REG_STARTEND (non-standard extension) | Tier 2 | Emulated via substring copy + offset adjustment |
| `pledge()` / `unveil()` | Tier 1 | Stubbed as no-op macros |
| `err()` / `errx()` / `warn()` / `errc()` | Tier 1 | `amiport/err.h` shim |
| `strlcpy()` / `strlcat()` / `reallocarray()` | Tier 1 | `amiport/string.h` shim |
| `strtonum()` | Tier 1 | `amiport/err.h` shim |
| `getopt()` / `optarg` / `optind` | Tier 1 | `amiport/getopt.h` shim |
| `mkstemp()` | Tier 1 | `amiport/stdio_ext.h` shim |
| `getline()` (GNU extension) | Custom | Implemented in `compat.h` via fgetc loop |
| `fdopen()` | Custom | Stubbed (returns NULL) — limits -i mode |
| `dirname()` | Custom | Implemented in `compat.h` with Amiga path support |
| `__dead` (OpenBSD attribute) | Custom | `#define __dead __attribute__((noreturn))` |
| `fchown()` / `fchmod()` | Custom | No-op macros (no UNIX permissions on AmigaOS) |
| `TIOCGWINSZ` / `struct winsize` | Custom | Removed; terminal width from COLUMNS env or 80 default |
| `<sys/ioctl.h>` / `<sys/uio.h>` / `<libgen.h>` | N/A | Removed — not available on AmigaOS |
| `TEXT` enum member | Custom | Renamed to `SED_TEXT` — clashes with AmigaOS NDK `typedef` |
| `_POSIX2_LINE_MAX` / `DEFFILEMODE` / `ALLPERMS` | Custom | Defined in `compat.h` |
| `exit(1)` → `exit(10)` | Convention | All exit codes converted to Amiga RETURN_ERROR |

## Transformations Applied

| File | Line(s) | Original | Transformed | Comment |
|------|----------|----------|-------------|---------|
| compat.h | all | N/A | New file | Centralizes all BSD/OpenBSD compat shims |
| defs.h | 38 | `#include <regex.h>` etc. | `#include "compat.h"` | All shims via compat header |
| defs.h | 100 | `TEXT,` | `SED_TEXT,` | Avoids AmigaOS NDK TEXT typedef collision |
| main.c | 36-51 | `<sys/ioctl.h>`, `<libgen.h>` | Removed | Not available on AmigaOS |
| main.c | 57-60 | N/A | verstag + __stack | Amiga version string and stack cookie |
| main.c | 114-115 | `struct winsize`, `pledge_*` | Removed | No ioctl/pledge on AmigaOS |
| main.c | 157 | `exit(1)` | `exit(10)` | Amiga RETURN_ERROR |
| main.c | 158-172 | `TIOCGWINSZ` + `strtonum` | `getenv("COLUMNS")` + default 80 | Terminal width detection |
| main.c | 407 | `"%s/sedXXXXXXXXXX", dirname(dirbuf)` | `"T:sedXXXXXXXXXX"` | AmigaOS temp directory |
| main.c | all | `err(1,...)`, `errx(1,...)` | `err(10,...)`, `errx(10,...)` | Amiga exit codes |
| compile.c | 89,91,98,245 | `TEXT` | `SED_TEXT` | NDK collision fix |
| compile.c | all | `err(1,...)` | `err(10,...)` | Amiga exit codes |
| process.c | 38 | `<sys/uio.h>` | Removed | Not available on AmigaOS |
| process.c | 61,275 | `static inline int` | `static int` | C89 compatibility |
| process.c | 527-573 | `regexec` with REG_STARTEND | Substring copy + offset adjust | REG_STARTEND emulation |
| misc.c | 58,69,82,126 | `err(1, ...)` | `err(10, ...)` | Amiga exit codes |
| misc.c | 67-77 | `xreallocarray` | Zero-size guard | reallocarray(NULL,0,size) returns NULL legitimately |

## Shim Functions Exercised

- `amiport_err()`, `amiport_errx()`, `amiport_warn()`, `amiport_warnx()` — error reporting
- `amiport_errc()` — OpenBSD error with explicit code (custom in compat.h)
- `amiport_strtonum()` — safe string-to-number conversion
- `amiport_strlcpy()`, `amiport_strlcat()` — bounded string operations
- `amiport_reallocarray()` — overflow-checked array reallocation
- `amiport_getopt()`, `amiport_optarg`, `amiport_optind` — argument parsing
- `amiport_mkstemp()` — secure temp file creation
- `amiport_emu_regcomp()`, `amiport_emu_regexec()`, `amiport_emu_regfree()`, `amiport_emu_regerror()` — Tier 2 regex

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim), `-lamiport-emu` (regex) |
| Binary size | 58K |

## Test Results

Tested via vamos. All 7 tests passed.

| Test | Command | Input | Expected | Result |
|------|---------|-------|----------|--------|
| Basic substitution | `sed 's/hello/goodbye/'` | hello world | goodbye world | PASS |
| Print mode | `sed -n 'p'` | line1 | line1 | PASS |
| Delete line | `sed '2d'` | line1\nline2\nline3 | line1\nline3 | PASS |
| Transform | `sed 'y/abc/ABC/'` | aaa\nbbb\nccc | AAA\nBBB\nCCC | PASS |
| Multiple -e | `sed -e 's/hello/hi/' -e 's/world/earth/'` | hello world | hi earth | PASS |
| Address range | `sed '2,3d'` | line1..line4 | line1\nline4 | PASS |
| Extended regex | `sed -E 's/[0-9]+/NUM/'` | abc123def | abcNUMdef | PASS |

## Known Limitations

- **In-place editing (`-i`) not functional**: `fdopen()` is not available in libnix; stubbed to return NULL. The `-i` flag will fail with a warning. Workaround: use shell redirection (`sed 's/old/new/' file > file.new`).
- **Regex limitations**: Tier 2 regex emulation does not support POSIX character classes (`[:alpha:]`, `[:digit:]`), locale-dependent collation, or `REG_NEWLINE`. Maximum 9 capture groups, 512-byte pattern limit.
- **No Ctrl-C break handling**: Long-running sed operations cannot be interrupted gracefully. The program must be killed externally.
- **Terminal width**: Uses `COLUMNS` environment variable or defaults to 80. No dynamic terminal detection (no `ioctl`/`TIOCGWINSZ`).

## Performance Optimization

Reviewed by perf-optimizer agent. Key optimizations:
- **Static regex buffer**: `regexec_e()` reuses a static buffer for REG_STARTEND substring copies instead of malloc/free per match. 2-5x speedup for substitution-heavy scripts.
- **Realloc pointer safety**: `amiport_getline()` preserves the original buffer pointer on realloc failure to prevent memory leaks on OOM.
- **Version string retention**: `__attribute__((used))` ensures the `$VER:` string survives optimization.

## Review

Reviewed with `/review-amiga`. Score: **READY**.
- Stack safety: OK (64KB cookie)
- Memory handling: WARN (hot-path malloc fixed by perf-optimizer)
- Path handling: OK (T: for temp, no Unix paths)
- Conventions: OK (exit codes, version string, stack cookie)
- No critical issues.

## Memory Safety (Stage 6b)

Audited by `memory-checker` agent (2026-03-21). Verdict: **CLEAN**.

No critical memory safety issues. All allocations either properly freed or acceptable as process-lifetime allocations (regex_t patterns, transtab, appends array). No double-frees, no use-after-free, no buffer overflows. File handle lifecycle correct (cfclose called before exit). Realloc patterns use intermediate pointers where needed.
