# BSD-isms Reference

Functions and idioms commonly found in OpenBSD/FreeBSD/NetBSD source code that need special handling during Amiga porting. The analyzer and transformer skills should flag these automatically.

## Status Legend

- **Shimmed**: Available in amiport shim/emu libraries
- **Stub**: Should be stubbed (no-op or removed)
- **Inline**: Needs inline replacement during transformation
- **Missing**: Not yet available — needs `/extend-shim` or manual handling

---

## Error Reporting (`<err.h>`)

| Function | Signature | Status | Shim Location | Notes |
|----------|-----------|--------|---------------|-------|
| `err()` | `void err(int eval, const char *fmt, ...)` | Shimmed | `amiport/err.h` | Print msg + errno + exit |
| `errx()` | `void errx(int eval, const char *fmt, ...)` | Shimmed | `amiport/err.h` | Print msg + exit (no errno) |
| `warn()` | `void warn(const char *fmt, ...)` | Shimmed | `amiport/err.h` | Print msg + errno to stderr |
| `warnx()` | `void warnx(const char *fmt, ...)` | Shimmed | `amiport/err.h` | Print msg to stderr |
| `warnc()` | `void warnc(int code, const char *fmt, ...)` | Shimmed | `amiport/err.h` | Print msg + explicit error code |
| `errc()` | `void errc(int eval, int code, const char *fmt, ...)` | Missing | — | Like err() but with explicit code; add to err.h |
| `vwarn()` | `void vwarn(const char *fmt, va_list)` | Missing | — | va_list variant of warn() |
| `vwarnx()` | `void vwarnx(const char *fmt, va_list)` | Missing | — | va_list variant of warnx() |
| `verr()` | `void verr(int eval, const char *fmt, va_list)` | Missing | — | va_list variant of err() |
| `verrx()` | `void verrx(int eval, const char *fmt, va_list)` | Missing | — | va_list variant of errx() |

## String Safety (`<string.h>` / `<bsd/string.h>`)

| Function | Signature | Status | Shim Location | Notes |
|----------|-----------|--------|---------------|-------|
| `strlcpy()` | `size_t strlcpy(char *dst, const char *src, size_t size)` | Shimmed | `amiport/string.h` | Size-bounded strcpy |
| `strlcat()` | `size_t strlcat(char *dst, const char *src, size_t size)` | Shimmed | `amiport/string.h` | Size-bounded strcat |
| `explicit_bzero()` | `void explicit_bzero(void *s, size_t n)` | Missing | — | Secure memset (never optimized away); use `memset()` as fallback |
| `timingsafe_memcmp()` | `int timingsafe_memcmp(const void *, const void *, size_t)` | Missing | — | Constant-time comparison; use `memcmp()` as fallback |

## Number Conversion

| Function | Signature | Status | Shim Location | Notes |
|----------|-----------|--------|---------------|-------|
| `strtonum()` | `long long strtonum(const char *, long long min, long long max, const char **)` | Shimmed | `amiport/err.h` | Safe string-to-number with range check |
| `strtoq()` | `long long strtoq(const char *, char **, int)` | Inline | — | Replace with `strtol()` (Amiga is 32-bit anyway) |

## Memory Allocation

| Function | Signature | Status | Shim Location | Notes |
|----------|-----------|--------|---------------|-------|
| `reallocarray()` | `void *reallocarray(void *ptr, size_t nmemb, size_t size)` | Shimmed | `amiport/string.h` | Overflow-checked realloc |
| `recallocarray()` | `void *recallocarray(void *ptr, size_t old_n, size_t new_n, size_t size)` | Missing | — | Like reallocarray but zeros freed memory; rarely needed |
| `freezero()` | `void freezero(void *ptr, size_t size)` | Missing | — | Free + zero; use `memset()+free()` as fallback |

## Dynamic String Formatting

| Function | Signature | Status | Shim Location | Notes |
|----------|-----------|--------|---------------|-------|
| `asprintf()` | `int asprintf(char **strp, const char *fmt, ...)` | Shimmed | `amiport/stdio_ext.h` | Dynamic formatted string |
| `vasprintf()` | `int vasprintf(char **strp, const char *fmt, va_list ap)` | Shimmed | `amiport/stdio_ext.h` | va_list variant |

## Temporary Files

| Function | Signature | Status | Shim Location | Notes |
|----------|-----------|--------|---------------|-------|
| `mkstemp()` | `int mkstemp(char *template)` | Shimmed | `amiport/stdio_ext.h` | Creates unique temp file |
| `mkstemps()` | `int mkstemps(char *template, int suffixlen)` | Missing | — | mkstemp with suffix |
| `mkdtemp()` | `char *mkdtemp(char *template)` | Missing | — | Create unique temp directory |

## Positional I/O

| Function | Signature | Status | Shim Location | Notes |
|----------|-----------|--------|---------------|-------|
| `pread()` | `ssize_t pread(int fd, void *buf, size_t count, off_t offset)` | Shimmed | `amiport/stdio_ext.h` | Read at offset (non-atomic on Amiga) |
| `pwrite()` | `ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)` | Shimmed | `amiport/stdio_ext.h` | Write at offset (non-atomic on Amiga) |

## Security / Sandboxing (always stub)

| Function | Signature | Status | Notes |
|----------|-----------|--------|-------|
| `pledge()` | `int pledge(const char *promises, const char *execpromises)` | Stub | OpenBSD-only; always return 0 |
| `unveil()` | `int unveil(const char *path, const char *permissions)` | Stub | OpenBSD-only; always return 0 |

**Transformation rule:** Remove `#include <unistd.h>` reference to pledge/unveil. Add stubs:
```c
/* amiport: pledge/unveil not available on AmigaOS — stubbed */
#define pledge(p, e) (0)
#define unveil(p, f) (0)
```

## Line Input (`<stdio.h>`)

| Function | Signature | Status | Notes |
|----------|-----------|--------|-------|
| `fgetln()` | `char *fgetln(FILE *stream, size_t *len)` | Inline | BSD-only; replace with `fgets()` into stack buffer |
| `getline()` | `ssize_t getline(char **lineptr, size_t *n, FILE *stream)` | Missing | GNU extension; implement with `fgets()` + `realloc()` loop |

**Transformation rule for fgetln:**
```c
/* Before: */
line = fgetln(fp, &len);
/* After: */
/* amiport: replaced fgetln() with fgets() — line is NUL-terminated */
static char _line_buf[8192];
line = fgets(_line_buf, sizeof(_line_buf), fp);
if (line) len = strlen(line);
```

## Regular Expressions

| Function | Signature | Status | Shim Location | Notes |
|----------|-----------|--------|---------------|-------|
| `regcomp()` | `int regcomp(regex_t *preg, const char *regex, int cflags)` | Shimmed (Tier 2) | `amiport-emu/regex.h` | BRE and ERE supported |
| `regexec()` | `int regexec(const regex_t *preg, const char *string, ...)` | Shimmed (Tier 2) | `amiport-emu/regex.h` | Backtracking NFA |
| `regfree()` | `void regfree(regex_t *preg)` | Shimmed (Tier 2) | `amiport-emu/regex.h` | No-op (stack allocated) |
| `regerror()` | `size_t regerror(int errcode, ...)` | Shimmed (Tier 2) | `amiport-emu/regex.h` | Error messages |

## Time Formatting

| Function | Signature | Status | Notes |
|----------|-----------|--------|-------|
| `strptime()` | `char *strptime(const char *s, const char *format, struct tm *tm)` | Missing | Parse time string; needed by cal — inline or add shim |
| `strftime()` | `size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm)` | Available | Provided by newlib/clib2 |

## Miscellaneous

| Function | Signature | Status | Notes |
|----------|-----------|--------|-------|
| `setprogname()` | `void setprogname(const char *progname)` | Stub | Store in global; used by err() family |
| `getprogname()` | `const char *getprogname(void)` | Stub | Return stored progname or "unknown" |
| `arc4random()` | `uint32_t arc4random(void)` | Missing | Random number; use Amiga timer seed + LCG |
| `arc4random_uniform()` | `uint32_t arc4random_uniform(uint32_t upper_bound)` | Missing | Uniform random |
| `fts_open()` / `fts_read()` | file tree traversal | Missing | Complex; use recursive opendir/readdir |
| `vis()` / `unvis()` | encode/decode non-printable chars | Missing | Rarely needed; stub or port |

---

## Analyzer Integration

When the `source-analyzer` agent encounters any function from this table, it should:

1. Check the "Status" column
2. If **Shimmed**: classify as Tier 1 `needs-shim` or Tier 2 `needs-emu`
3. If **Stub**: classify as `trivial` and note it should be stubbed
4. If **Inline**: classify as `needs-shim` and include the transformation rule
5. If **Missing**: classify as `needs-shim` and recommend running `/extend-shim <function>`

## Transformer Integration

The `code-transformer` agent should:

1. Replace shimmed functions with their `amiport_*` equivalents
2. Add stub macros for pledge/unveil
3. Apply inline transformations for fgetln→fgets etc.
4. Flag missing functions with `/* amiport: TODO — needs /extend-shim */` comments
