# Port: tail

## Overview

| Field | Value |
|-------|-------|
| Program | tail |
| Version | 1.24 |
| Source | OpenBSD usr.bin/tail (v1.24, 2022-12-04) |
| Category | 1 — CLI tool |
| License | BSD 3-Clause |
| Original Author | Edward Sze-Tyan Wang / UC Berkeley |
| Port Date | 2026-03-21 |

## Description

Display the last part of a file. Supports display by lines (`-n`), bytes (`-c`), or blocks (`-b`), with reverse output (`-r`) and file following (`-f`) modes. Can process multiple files with per-file headers.

## Prior Art on Aminet

- `tail.lha` (2001) — basic line-only implementation with AmigaDOS syntax, no `-f` support, no POSIX options. 25 years old, unmaintained.
- GNU coreutils port (adtools, 2017) — unclear m68k support, likely requires ixemul.
- Our port provides full POSIX tail with `-f` polling fallback, no ixemul dependency.

## Portability Analysis

**Verdict: MODERATE**

| Issue | Tier | Resolution |
|-------|------|------------|
| `pledge()` calls | 1 | Macro stub `#define pledge(p,e) (0)` |
| `strlcpy()` | 1 | `amiport_strlcpy()` via shim |
| `reallocarray()` | 1 | `amiport_reallocarray()` via shim |
| `recallocarray()` | 1 | `amiport_recallocarray()` via shim (new) |
| `fstat()`/`stat()` | 1 | `amiport_fstat()`/`amiport_stat()` via shim |
| `S_ISREG()` | 1 | Defined in stat shim |
| `fseeko()` | 1 | Maps to `fseek()` (32-bit off_t) |
| `lseek()` + ESPIPE | 1 | `amiport_lseek()` via shim |
| `write()`/STDOUT_FILENO | 1 | `amiport_write()` via shim |
| `err()`/`errx()`/`warn()` | 1 | `amiport_err()` etc. via shim |
| `getopt()` | 1 | `amiport_getopt()` via shim |
| `fpurge()` | 1 | No-op macro stub (new) |
| `kqueue()`/`kevent()` | 3 | Replaced with `Delay()`-based polling fallback |
| `exit(1)` codes | arch | Changed to `exit(10)` (RETURN_ERROR) |
| `strtoll()` | arch | Available in bebbo-gcc; truncates to 32-bit off_t safely |

## Transformations Applied

<!-- Filled by code-transformer agent -->

## Shim Functions Exercised

- `amiport_stat()`, `amiport_fstat()`
- `amiport_lseek()`
- `amiport_write()`
- `amiport_strlcpy()`
- `amiport_reallocarray()`, `amiport_recallocarray()`
- `amiport_err()`, `amiport_errx()`, `amiport_warn()`, `amiport_warnx()`
- `amiport_getopt()`
- `amiport_fpurge()` (no-op)

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68020+ |
| CFLAGS | `-O2 -noixemul -m68020 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | |

## Test Results

<!-- Tested via vamos (Category 1). -->

| Test | Command | Input | Expected | Result |
|------|---------|-------|----------|--------|
| | | | | |

## Known Limitations

- **`-f` follow mode uses polling** — 1-second `Delay()` loop instead of kqueue. No detection of file deletion, rename, or truncation events. File following works for append-only growth (the common use case).
- **`fpurge()` is a no-op** — on file truncation during `-f`, the stdio read buffer is not discarded. In practice this is harmless since truncation detection is not available in the polling fallback.
- **32-bit file offsets** — files >2GB are not representable. Acceptable for AmigaOS target.
- **`strtoll()` truncation** — offset values are silently truncated to 32-bit. Benign in practice.

## Review

<!-- /review-amiga score summary. -->
