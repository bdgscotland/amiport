---
name: c89-reference
description: C89/ANSI C language constraints for AmigaOS porting. Use when writing or reviewing C code for this project. Loads the list of C99+ features that are NOT available, libnix function availability, and common mistakes agents make when writing C89 code.
allowed-tools: Read, Grep, Glob
---

# C89 Reference for AmigaOS Porting

This project targets **ANSI C89 (ISO C90)** compiled with `m68k-amigaos-gcc -ansi` and linked with libnix (`-noixemul`). This skill lists what you CANNOT use, what you CAN use, and critical runtime behaviors.

## Compiler Identity

**m68k-amigaos-gcc** is based on **GCC 6.5.0b** (default branch `amiga6`). Newer branches exist (13.1, 13.2) but this project uses 6.5.0b.

**CPU targets:** 68000 through 68060, plus Vampire/Apollo 68080.

**Runtime selection:**
| Flag | Runtime | Target |
|------|---------|--------|
| `-noixemul` or `-mcrt=nix20` | libnix | Kickstart 2.0+ (our default) |
| `-mcrt=nix13` | libnix | Kickstart 1.3 |
| `-mcrt=clib2` | clib2 | Kickstart 2.04+ |
| `-mcrt=ixemul` | ixemul | Kickstart 2.0+ (requires ixemul.library) |
| *(default)* | newlib | Kickstart 2.0+ |

**This project uses `-noixemul` (libnix) exclusively.**

## GCC Extensions Available Under `-ansi`

The `-ansi` flag is equivalent to `-std=c89`. It disables bare GCC keywords but **double-underscore forms survive**. Without `-pedantic`, most GNU extensions are accepted.

### DISABLED by `-ansi` (will error)
| Keyword | Use Instead |
|---------|-------------|
| `asm` | `__asm__` |
| `typeof` | `__typeof__` |
| `inline` | `__inline__` or just `static` |

### STILL AVAILABLE under `-ansi` (safe to use)
| Extension | Notes |
|-----------|-------|
| `__asm__("...")` | Inline assembly (used in uaequit.c) |
| `__typeof__(expr)` | Type inference |
| `__inline__` | Inline hint |
| `__extension__` | Suppresses warnings for GNU extensions in headers |
| `__attribute__((used))` | Prevent linker removal (used for stack cookies, version strings) |
| `__attribute__((aligned(N)))` | Alignment control |
| `__attribute__((packed))` | Struct packing (caution: crashes 68000 on word access) |
| `__attribute__((section("...")))` | Section placement |
| `__attribute__((noreturn))` | No-return functions |
| `__attribute__((unused))` | Suppress unused warnings |
| `__builtin_expect(x, v)` | Branch prediction hints |
| Statement expressions `({ ... })` | Compound expressions returning a value |
| Zero-length arrays `char data[0]` | GCC extension (prefer `data[1]` for portability) |
| `__FUNCTION__` | Predefined function name string (GCC extension of `__func__`) |

**Rule of thumb:** If it starts with `__`, it probably works under `-ansi`. If it's a bare keyword without underscores, it probably doesn't.

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
| `inline` keyword | C99 | `static` function in header (or `__inline__`) |
| `restrict` keyword | C99 | Omit (or use `__restrict__`) |
| `long long` type | C99 | `long` (32-bit on 68k). For 64-bit, use two `ULONG` fields |
| `stdint.h` (`int32_t`, `uint8_t`, etc.) | C99 | Use Amiga types: `LONG`, `ULONG`, `UBYTE`, `UWORD` |
| `stdbool.h` | C99 | `#define TRUE 1` / `#define FALSE 0` |
| `snprintf` returning needed size | C99 | libnix `vsnprintf(NULL, 0, ...)` crashes — use probe buffer |
| Variadic macros `#define M(...)` | C99 | Use variadic functions instead |
| `__func__` predefined identifier | C99 | Use `__FUNCTION__` (GCC extension) or string literal |
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

**libnix printf quirk:** Floating-point format specifiers (`%f`, `%e`, `%g`) require linking with `-lm`. Without it, printf silently outputs nothing for floats.

### Headers — NOT AVAILABLE in libnix

| Header | C99+ | Alternative |
|--------|------|-------------|
| `<stdint.h>` | C99 | `<exec/types.h>` for Amiga types |
| `<stdbool.h>` | C99 | Define your own `TRUE`/`FALSE` |
| `<inttypes.h>` | C99 | Use `%ld`/`%lu` with casts |
| `<complex.h>` | C99 | Not applicable on 68k |
| `<fenv.h>` | C99 | Not applicable |
| `<tgmath.h>` | C99 | Use explicit math functions |

## libnix Runtime Behavior — CRITICAL

These are behaviors specific to libnix that differ from standard C runtimes. Agents MUST know these.

### Memory Allocation
- Uses task-local memory pools via `Allocate()`/`Deallocate()`
- Default pool block size: **16384 bytes**. Larger allocations round to 4096-byte multiples.
- Pool block size configurable via `unsigned long _MSTEP = <size>;`
- **No automatic process memory cleanup** with `-noixemul`. Every `malloc` without `free` leaks until reboot.

### Stack Handling — Three Methods

**Method 1: `__stack` variable (what we use)**
```c
long __stack = 65536;  /* Requires swapstack.o linked by libnix startup */
```
At startup, swapstack checks if the current stack is large enough and switches to a new one if not. **Caution:** There is a known issue (#38 on bebbo/libnix) where swapstack can damage memory structures. Despite this, it is the standard approach for libnix programs.

**Method 2: `$STACK` cookie (AmigaOS 4+ / modern shells)**
```c
static const char USED min_stack[] = "$STACK:65536";
/* USED = __attribute__((used)) — prevents linker removal */
```
The shell reads this cookie and allocates the requested stack before launching the program. More reliable than swapstack but requires a shell that supports it.

**Method 3: Compiler flags**
- `-mstackcheck` — inserts stack checks at function entry
- `-mstackextend` — attempts dynamic stack allocation when low
- **Cannot be used in hook/interrupt code** — may cause unexpected program exit

**This project uses Method 1 (`__stack` variable).** On vamos, `__stack` is NOT read at runtime — vamos uses its own stack size (default 8KiB). Pass `-s <size>` to vamos or set `VAMOS_STACK` in the Makefile.

### Signal Handling
- **Only SIGABRT and SIGINT are supported.** All other signals are silently ignored.
- SIGINT is **polled**, not asynchronous. `__chkabort()` is called at stdio function entries (fread, fwrite, fgets, etc.).
- To disable Ctrl-C checking: `signal(SIGINT, SIG_IGN)` or provide an empty `void __chkabort(void) {}`
- For ported programs, use `amiport_check_break()` in loops instead — it's more explicit.

### stdio Behavior
- **stdin/stdout:** Derived from dos.library `Input()`/`Output()`
- **stderr:** Tries process `pr_CES` field first, then falls back to `Open("*", MODE_NEWFILE)`, then stdout. This means stderr may go to a different console window on Workbench.
- **Buffering:** Some Amiga terminals don't line-buffer. Use `fflush(stdout)` after important output.
- **`__chkabort()` polling:** Every stdio call (fgets, fread, fprintf, etc.) checks for Ctrl-C. This means I/O-heavy programs can be interrupted, but CPU-bound loops without I/O cannot.

### Locale Behavior
- Default locale is loaded at startup from `locale.library` (not "C" locale!)
- This can cause unexpected behavior — `MB_CUR_MAX` may return >1 even without real locale support
- **Fix:** Call `setlocale(LC_ALL, "C")` early in main() to force the C locale
- Multibyte character functions only simulate "C" locale regardless

### Exit and Cleanup
- `atexit()` functions are called on `exit()` — this works correctly in C
- **C++ note:** Static destructors are called BEFORE `atexit()` handlers (reversed from standard)
- `setjmp`/`longjmp` do **NOT** restore FPU registers
- Initialization order: `__INIT_LIST__[]` functions run at startup, `__EXIT_LIST__[]` at cleanup

### Startup Control Variables
| Variable | Effect |
|----------|--------|
| `int __nocommandline = 1;` | Skip argc/argv parsing |
| `int __initlibraries = 0;` | Disable automatic library opening |
| `long __oslibversion = 39;` | Minimum Kickstart version (default: 37 = OS 2.04) |
| `char __stdiowin[] = "CON:...";` | Custom console window for Workbench launch |
| `unsigned long _MSTEP = N;` | Memory pool block size |

### Link Order — MATTERS

libnix requires correct link order. Objects/libraries must appear in this sequence:
1. Startup code (ncrt0.o/nbcrt0.o)
2. User object files
3. Command-line parser (libnixmain.a)
4. `-lm` (math library, if needed)
5. `-lnix` (libnix.a)
6. `-lamiga` (libamiga.a)
7. `-lstubs` (libstubs.a — auto library opening)

**The Makefile templates handle this correctly.** Don't rearrange link order manually.

## libnix Function Availability

For the complete list of what libnix provides and what needs shimming, see:

```
Read file_path="docs/references/libnix-reference.md"       # Complete function list (700+ functions from libc.a)
Read file_path="docs/references/newlib-availability.md"     # Curated availability table with amiport shim status
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

## clib2 vs libnix — When to Consider Switching

| Feature | libnix | clib2 |
|---------|--------|-------|
| C standard | C89 | C94 + C99 extensions |
| Size | Smaller | Larger |
| Kickstart | 1.3+ or 2.0+ | 2.04+ only |
| snprintf C99 return | No (crashes on NULL) | Yes |
| Thread safety | No | Optional (`__THREAD_SAFE`) |
| Link flag | `-noixemul` | `-mcrt=clib2` |
| Link order | `-lm -lnix -lamiga` | `-lm -lc` (order matters!) |
| Floating point printf | Requires `-lm` | Requires `-lm` |
| Daylight saving time | Unknown | Not supported |

**This project uses libnix.** clib2 is only relevant if a port requires C99 runtime features that libnix can't provide.

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

These are patterns that agents frequently generate that won't compile or behave wrong on C89/libnix:

### Compile-Time Errors
1. **`for (int i = 0; ...)`** — Declare `i` before the loop
2. **`// comments`** — Use `/* */` only
3. **`%zu` in printf** — Use `%lu` with cast
4. **Declarations after statements** — Move all declarations to top of block
5. **`long long`** — Not available on 68k, use `ULONG` pairs for 64-bit
6. **`bool` type** — Use `int`
7. **`#include <stdint.h>`** — Use `<exec/types.h>`
8. **Designated initializers** — Initialize struct fields individually
9. **`static inline`** — Use `static` only (or `static __inline__`)
10. **`inline` keyword** — Use `__inline__` or omit

### Runtime Errors (compiles but crashes/misbehaves)
11. **`snprintf(NULL, 0, ...)`** — Crashes on libnix. Use 1024-byte probe buffer.
12. **`MB_CUR_MAX > 1` check** — May be true on libnix (it's a function call, not a constant). Guard with `#ifndef __AMIGA__`.
13. **Forgetting `-lm` with printf `%f`** — Silently prints nothing for floats.
14. **Assuming C locale** — libnix loads system locale at startup. Call `setlocale(LC_ALL, "C")` early.
15. **Assuming SIGINT delivery** — Only polled at stdio calls. CPU-bound loops need `amiport_check_break()`.

### 68k Architecture Errors
16. **`offsetof()` alignment = 2, not 4/8** — On 68k, struct padding uses 2-byte alignment. Custom allocators using `offsetof()` to compute block alignment will pack blocks too tightly, corrupting metadata. Use `AMIPORT_ALIGN()` from `<amiport/compat.h>` (crash-patterns #15).
17. **Struct-by-value returns > 8 bytes at -O1/-O2** — bebbo-gcc (GCC 6.5.0b) corrupts struct returns larger than 8 bytes when optimization is enabled. Compile with `-O0` or refactor to return via pointer (crash-patterns #16).

## Quick Validation

When reviewing ported code, grep for these C99+ patterns:

```bash
# In ported source files:
grep -rn '//' ported/*.c               # C++ comments (exclude http:// URLs)
grep -rn 'for\s*(\s*int' ported/*.c    # for-loop declarations
grep -rn '%zu\|%zd\|%jd' ported/*.c   # C99 format specifiers
grep -rn 'bool\|true\|false' ported/*.c # C99 bool type
grep -rn '_Bool\|restrict\|inline' ported/*.c # C99 keywords (but __inline__ is OK)
grep -rn 'long long' ported/*.c         # 64-bit int (not on 68k)
grep -rn 'stdint\.h\|stdbool\.h' ported/*.c # C99 headers
```

## References

- bebbo-gcc toolchain: https://codeberg.org/bebbo/amiga-gcc (GCC 6.5.0b)
- libnix source: https://github.com/bebbo/libnix
- libnix documentation: https://github.com/adtools/libnix/blob/master/libnix.texi
- clib2 (more complete, alternative): https://github.com/adtools/clib2
- AmigaOS stack handling: https://wiki.amigaos.net/wiki/Controlling_Application_Stack
- GCC C dialect options: https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html
- Project libnix availability table: `docs/references/newlib-availability.md`
- Project libnix function list: `docs/references/libnix-reference.md`
