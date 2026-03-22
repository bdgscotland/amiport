---
name: c89-reference
description: C89/ANSI C language constraints for AmigaOS porting. Use when writing or reviewing C code for this project. Loads the list of C99+ features that are NOT available, libnix function availability, and common mistakes agents make when writing C89 code.
allowed-tools: Read, Grep, Glob
---

# C89 Reference for AmigaOS Porting

This project targets **ANSI C89 (ISO C90)** compiled with `m68k-amigaos-gcc -ansi` and linked with libnix (`-noixemul`). This skill lists what you CANNOT use and what to use instead.

## What C99/C11 Features Are NOT Available

### Language Features — DO NOT USE

| Feature | C99+ | C89 Alternative |
|---------|------|-----------------|
| `//` single-line comments | C99 | `/* comment */` |
| Mixed declarations and statements | C99 | Declare all variables at block top |
| Variable-length arrays (VLAs) | C99 | `malloc()` or static arrays |
| `for (int i = 0; ...)` | C99 | `int i; for (i = 0; ...)` |
| Designated initializers `{.field = val}` | C99 | Initialize fields by position or assignment |
| Compound literals `(struct foo){1, 2}` | C99 | Declare a named variable |
| `_Bool` / `bool` type | C99 | `int` (0/1) |
| `inline` keyword | C99 | `static` function in header (GCC extension with `__inline__`) |
| `restrict` keyword | C99 | Omit (or use `__restrict__` GCC extension) |
| `long long` type | C99 | `long` (32-bit on 68k). For 64-bit, use two `ULONG` fields |
| `stdint.h` (`int32_t`, `uint8_t`, etc.) | C99 | Use Amiga types: `LONG`, `ULONG`, `UBYTE`, `UWORD` |
| `stdbool.h` | C99 | `#define TRUE 1` / `#define FALSE 0` |
| `snprintf` returning needed size | C99 | libnix `vsnprintf(NULL, 0, ...)` crashes — use probe buffer |
| Variadic macros `#define M(...)` | C99 | Use variadic functions instead |
| `__func__` predefined identifier | C99 | Use string literal `"function_name"` |
| Flexible array members `struct { int n; char data[]; }` | C99 | `char data[1]` + overallocation |
| `_Pragma()` operator | C99 | `#pragma` directive |

### Printf/Scanf Format Specifiers — DO NOT USE

| Format | C99+ | C89 Alternative |
|--------|------|-----------------|
| `%zu` (size_t) | C99 | `%lu` with `(unsigned long)` cast |
| `%zd` (ssize_t) | C99 | `%ld` with `(long)` cast |
| `%jd` (intmax_t) | C99 | Not available — use `%ld` |
| `%td` (ptrdiff_t) | C99 | `%ld` with `(long)` cast |
| `%lld` (long long) | C99 | Not available on 68k (no 64-bit int) |
| `%hh` (char) | C99 | `%d` with `(int)` cast |

### Headers — NOT AVAILABLE in libnix

| Header | C99+ | Alternative |
|--------|------|-------------|
| `<stdint.h>` | C99 | `<exec/types.h>` for Amiga types |
| `<stdbool.h>` | C99 | Define your own `TRUE`/`FALSE` |
| `<inttypes.h>` | C99 | Use `%ld`/`%lu` with casts |
| `<complex.h>` | C99 | Not applicable on 68k |
| `<fenv.h>` | C99 | Not applicable |
| `<tgmath.h>` | C99 | Use explicit math functions |

## libnix Function Availability

For the complete list of what libnix provides and what needs shimming, see:

```
Read file_path="docs/references/newlib-availability.md"
```

Key missing functions that commonly appear in ported code:
- `getopt_long()` — use `amiport_getopt()` (short options only)
- `reallocarray()` — use overflow check + `realloc()`
- `strlcpy()` / `strlcat()` — inline implementation in port headers
- `asprintf()` / `vasprintf()` — use probe buffer + `vsnprintf()` + `malloc()`
- `strtonum()` — available in `amiport/err.h`
- `pledge()` / `unveil()` — stub as `#define pledge(p, e) (0)`
- `err()` / `errx()` / `warn()` / `warnx()` — use `amiport/err.h`
- `glob()` / `globfree()` — use `amiport/glob.h`
- `fnmatch()` — use `amiport/fnmatch.h`
- `scandir()` / `alphasort()` — use `amiport/scandir.h`

## Amiga-Specific Type Rules

| Use This | Not This | Why |
|----------|----------|-----|
| `LONG` | `int32_t` | Amiga convention, exec/types.h |
| `ULONG` | `uint32_t` | Amiga convention |
| `UBYTE` | `uint8_t` | Amiga convention |
| `UWORD` | `uint16_t` | Amiga convention |
| `BPTR` | `void *` | AmigaDOS pointer (address >> 2) |
| `STRPTR` | `char *` | Amiga string pointer |
| `APTR` | `void *` | Generic Amiga pointer |
| `%ld` / `%lu` | `%d` / `%u` | LONG is `long`, not `int` |

## Common Agent Mistakes

These are patterns that agents frequently generate that won't compile on C89:

1. **`for (int i = 0; ...)`** — Declare `i` before the loop
2. **`// comments`** — Use `/* */` only
3. **`%zu` in printf** — Use `%lu` with cast
4. **Declarations after statements** — Move all declarations to top of block
5. **`snprintf(NULL, 0, ...)`** — Crashes on libnix, use 1024-byte probe buffer
6. **`long long`** — Not available on 68k, use `ULONG` pairs for 64-bit
7. **`bool` type** — Use `int`
8. **`#include <stdint.h>`** — Use `<exec/types.h>`
9. **Designated initializers** — Initialize struct fields individually
10. **`static inline`** — Use `static` only (or `__inline__` GCC extension)

## Quick Validation

When reviewing ported code, grep for these C99+ patterns:

```bash
# In ported source files:
grep -rn '//' ported/*.c               # C++ comments
grep -rn 'for\s*(\s*int' ported/*.c    # for-loop declarations
grep -rn '%zu\|%zd\|%jd' ported/*.c   # C99 format specifiers
grep -rn 'bool\|true\|false' ported/*.c # C99 bool type
grep -rn '_Bool\|restrict\|inline' ported/*.c # C99 keywords
grep -rn 'long long' ported/*.c         # 64-bit int (not on 68k)
grep -rn 'stdint\.h\|stdbool\.h' ported/*.c # C99 headers
```

## References

- C89/ANSI C Reference Card: https://users.ece.utexas.edu/~adnan/c-refcard.pdf
- libnix source: https://github.com/bebbo/libnix
- clib2 (more complete, alternative): https://github.com/adtools/clib2
- Project libnix availability table: `docs/references/newlib-availability.md`
- bebbo-gcc toolchain: https://github.com/bebbo/amiga-gcc
