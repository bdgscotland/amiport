# Port: sort

## Overview

| Field | Value |
|-------|-------|
| Program | sort |
| Version | 1.0 |
| Source | Plan 9 from User Space (plan9port) |
| Category | 1 — CLI |
| License | Lucent Public License 1.02 |
| Original Author | Plan 9 (Lucent Technologies) |
| Port Date | 2026-03-22 |

Version note: Plan 9 does not version individual utilities. `1.0` denotes the first Amiga release of this port.

## Description

POSIX-compatible sort utility that sorts lines of text files. Supports field-based sorting (-k), custom delimiters (-t), numeric sort (-n/-g), reverse (-r), unique (-u), case folding (-f), dictionary order (-d), month sort (-M), check mode (-c), and output to file (-o). Includes external merge sort for files larger than available memory, with temp files spilled to T:.

## Prior Art on Aminet

Built-in AmigaDOS SORT is severely limited — only supports COLSTART, CASE, and NUMERIC. No field keys, no delimiters, no reverse, no unique, no merge. GNU coreutils source exists but no m68k binaries are distributed. No modern, full-featured POSIX sort is available for AmigaOS 3.x.

## Portability Analysis

Verdict: **MODERATE** — single-file port, no POSIX blockers, but the entire Plan 9 API layer (Biobuf I/O, notify/exits, fprint, Rune) replaced with standard C89.

| Issue | Tier | Resolution |
|-------|------|------------|
| Plan 9 headers (u.h, libc.h, bio.h) | 1 | Removed; replaced with standard C89 headers |
| Biobuf I/O (Binit, Brdline, Bgetc, Bwrite) | 3 | Replaced with FILE* stdio throughout |
| Plan 9 types (uchar, uint, ulong, Rune) | 1 | Local typedefs |
| Plan 9 functions (fprint, sprint, exits, notify) | 1 | fprintf/sprintf/exit/signal |
| UTF-8 codec (chartorune, runetochar) | 1 | Bundled minimal implementation |
| Plan 9 file I/O (open/create with OREAD/OWRITE) | 1 | Replaced with fopen() |
| Nline = 500000 default | Arch | Reduced to 10000 for Amiga memory constraints |
| Temp dir /var/tmp | 1 | Changed to T: |
| void main() | 1 | Changed to int main() |
| rsort4 stack arrays | Arch | Made static to avoid 3KB/frame stack overflow |

## Critical Bug Found: amiport_open() + fdopen() Incompatibility

**Root cause:** `amiport_open()` returns file descriptors from amiport's internal fd table, which is a **separate namespace** from libnix's fd table. Calling `fdopen()` (a libnix function) with an amiport fd creates a FILE* that silently fails to read/write from the actual file.

**Symptoms:** Sort exits RC=0 (success) but produces no stdout output when given file arguments. stdin mode works because libnix manages stdin directly.

**Fix:** Replace all `amiport_open()` + `fdopen()` patterns with `fopen()` which goes through libnix's own fd management.

**This is a systemic issue** that would affect any port combining `amiport_open()` with libnix stdio functions like `fdopen()`.

## Transformations Applied

| Original | Transformed | Comment |
|----------|-------------|---------|
| `<u.h>`, `<libc.h>`, `<bio.h>` | Standard C89 + amiport headers | Plan 9 → ANSI C |
| Plan 9 types | typedef uchar/uint/ulong/Rune + macros | Compatibility layer |
| chartorune/runetochar | Bundled minimal UTF-8 codec | ~60 lines |
| `void main()` | `int main()` | C89 compliance |
| `notify(notifyf)` | `signal(SIGINT, sighandler)` | Plan 9 → POSIX signals |
| `open(s, OREAD)` + `Binit` | `fopen(s, "r")` | libnix-compatible I/O |
| `create()` + `Binit` | `fopen(path, "w")` | libnix-compatible I/O |
| `Brdline` + `Blinelen` + `Bgetc` | `fgetc()` loop | Line reading rewrite |
| `Bwrite()` + `exits("write")` | `fwrite()` + `done("write")` | Output + error handling |
| `exits(xs)` | `exit(xs ? 10 : 0)` | Amiga return codes |
| `/var/tmp` + `sprint()` | `T:` + `snprintf()` | Temp dir + bounds-safe |
| `Nline = 500000` | `Nline = 10000` | Amiga memory constraint |
| `part[257]`, `clist[514]` in rsort4 | `static` | Stack overflow prevention |

## Shim Functions Exercised

- `amiport_fopen()` (via `<amiport/stdio.h>` fopen macro) — file path translation
- `amiport_signal()` / `amiport_check_break()` — Ctrl-C handling
- `amiport_getpid()` — temp file name uniqueness

## Build Configuration

| Setting | Value |
|---------|-------|
| Compiler | m68k-amigaos-gcc (bebbo) |
| Target | m68k-amigaos, 68000+ |
| CFLAGS | `-O2 -noixemul -m68000 -Wall` |
| Libraries | `-lamiport` (posix-shim) |
| Binary size | 43,908 bytes |

## Test Results

18/18 FS-UAE tests passed. Also verified via vamos with piped stdin.

| Test | Expected | Result |
|------|----------|--------|
| Alphabetical sort | Sorted lines | PASS |
| Reverse sort (-r) | Reverse order | PASS |
| Numeric sort (-n) | 1,2,10,30,200 | PASS |
| Float sort (-g) | Float order | PASS |
| Case fold (-f) | Case-insensitive | PASS |
| Unique (-u) | Deduped | PASS |
| Check sorted (-c) | RC=0 | PASS |
| Check unsorted (-c) | RC=10 | PASS |
| Month sort (-M) | Calendar order | PASS |
| Field/delim (-k -t) | Field sort | PASS |
| Skip blanks (-b) | Ignore indent | PASS |
| Output file (-o) | RC=0, file written | PASS |
| Bad option (-Z) | RC=10 | PASS |
| Missing file | RC=10 | PASS |
| Empty + normal file | Sorted | PASS |
| Multi-file -c | RC=10 | PASS |
| Reverse numeric (-r -n) | Descending | PASS |
| WORK: path | Sorted | PASS |

## Known Limitations

- **No locale support** — sort uses byte-level comparison (C locale). AmigaOS has no locale infrastructure.
- **No stable sort (-s)** — the radix sort is not stable. The `-s` flag is not implemented.
- **Nline = 10000** — can sort up to ~10,000 lines in memory before spilling to T: temp files. Increase with `-l N`.
- **UTF-8 limited** — bundled minimal codec handles 1-3 byte sequences. 4-byte sequences treated as Latin-1 bytes.

## Review

Code review and memory-checker audit completed. Issues found and fixed:

1. **rsort4 stack overflow** (CRITICAL) — `part[257]` + `clist[514]` = 3100 bytes/frame, recursive. Made `static` — safe on single-threaded AmigaOS.
2. **args.linep never freed** (CRITICAL) — added cleanup loop in `done()`.
3. **Last line in -c mode leaked** (HIGH) — freed `ol` on all exit paths.
4. **Unsafe realloc in newline/buildkey** (MEDIUM) — use intermediate pointer, free old on failure.
5. **amiport_open() + fdopen() incompatibility** (CRITICAL) — replaced with fopen() throughout.

Score: **READY** after all fixes applied.
