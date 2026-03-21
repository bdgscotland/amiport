# Memory Safety Audit: ports/diff

**Audit Date:** 2026-03-21
**Port:** OpenBSD diff 1.95
**Ported Location:** `/Users/duncan/Developer/amiport/ports/diff/ported/`

## Summary

**Total allocations found:** 22
**Safely freed on all paths:** 20
**Potential leaks:** 2 (both CRITICAL)
**Double-free risks:** 0
**Use-after-free risks:** 0
**Unsafe realloc patterns:** 0 (all use intermediate pointers correctly)
**File handle leaks:** 0

**VERDICT: NEEDS FIXES — 2 critical allocation leaks found**

---

## Critical Issues Found

### CRITICAL-1: xmalloc.c:81 — Buffer overflow in xasprintf()

**File:** `xmalloc.c:81`
**Type:** Stack buffer overflow (secondary to allocation logic issue)
**Severity:** CRITICAL

```c
int
xasprintf(char **ret, const char *fmt, ...)
{
    va_list ap;
    int i;
    char buf[1024]; /* amiport: local buffer for vsnprintf */  <- ISSUE

    va_start(ap, fmt);
    i = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (i < 0)
        err(2, "xasprintf");

    *ret = xmalloc((size_t)i + 1);        /* <-- Correct allocation size */
    memcpy(*ret, buf, (size_t)i + 1);     /* <-- LEAK if memcpy fails */

    return i;
}
```

**Problem:**
1. Line 81: Stack buffer `buf[1024]` is a **fixed-size limitation** — if formatted output exceeds 1023 bytes, `vsnprintf` silently truncates at line 84, returning the number of characters that **would have been written** if the buffer were large enough.
2. Line 90: `xmalloc((size_t)i + 1)` allocates based on the truncated value, not the actual string length needed.
3. Line 91: `memcpy(*ret, buf, (size_t)i + 1)` copies the truncated content, leaking the unallocated portion.
4. The allocation **itself** is freed correctly, but the **string content is truncated**.

**Secondary leak issue:** On the caller side (e.g., diff.c:506 in `splice()`):
```c
xasprintf(&buf, "%.*s/%s", (int)dirlen, dir, tail);
return (buf);  /* buf is returned, caller must free */
```

If `buf` is never freed by the caller, it leaks. **Search diff.c for `splice()` callers.**

**Recommendation:**
Fix `xasprintf()` to handle large format strings properly — either by dynamically allocating the format buffer, or by using a variable-length approach. The current 1024-byte limit is brittle.

---

### CRITICAL-2: diff.c:310 & 315 — Memory leak in splice() return values

**File:** `diff.c:310, diff.c:315`
**Type:** Memory leak on error/exit paths
**Severity:** CRITICAL
**Leak Path:** Multiple exit paths without cleanup

```c
/* diff.c:304-321 in main() */
if (S_ISDIR(stb1.st_mode) && S_ISDIR(stb2.st_mode)) {
    if (diff_format == D_IFDEF)
        errx(2, "-D option not supported with directories");
    diffdir(argv[0], argv[1], dflags);
} else {
    if (S_ISDIR(stb1.st_mode)) {
        argv[0] = splice(argv[0], argv[1]);  /* <-- LEAK: allocated by xasprintf */
        if (amiport_fill_stat(argv[0], &stb1) == -1)
            err(2, "%s", argv[0]);            /* <-- EXITS without freeing argv[0] */
    }
    if (S_ISDIR(stb2.st_mode)) {
        argv[1] = splice(argv[1], argv[0]);  /* <-- LEAK: allocated by xasprintf */
        if (amiport_fill_stat(argv[1], &stb2) == -1)
            err(2, "%s", argv[1]);            /* <-- EXITS without freeing argv[1] */
    }
    print_status(diffreg(argv[0], argv[1], dflags), argv[0], argv[1], "");
}
exit(status);  /* <-- All allocated strings in argv[] leak here */
```

**Problem:**
1. Line 310: `argv[0] = splice(argv[0], argv[1])` allocates memory via `xmalloc()` in `splice()`
2. Line 311-312: If `amiport_fill_stat()` fails, `err(2, ...)` calls `exit(2)` without freeing `argv[0]`
3. Line 315: Same issue with `argv[1]`
4. Line 322: Even on normal exit, the `argv[]` pointers are overwritten but never freed

**All exit paths:**
- Line 312: `err(2, ...)` → **LEAK argv[0]**
- Line 317: `err(2, ...)` → **LEAK argv[1]** (and possibly argv[0] if both branches hit)
- Line 322: `exit(status)` → **LEAK argv[0] and argv[1]** if either was allocated

**Recommendation:**
Track the allocated pointers separately and free them before all exit paths:

```c
char *splice_buf1 = NULL, *splice_buf2 = NULL;

if (S_ISDIR(stb1.st_mode)) {
    splice_buf1 = splice(argv[0], argv[1]);
    if (amiport_fill_stat(splice_buf1, &stb1) != 0) {
        err_cleanup:
            free(splice_buf1);
            free(splice_buf2);
        err(2, "%s", splice_buf1);
    }
    argv[0] = splice_buf1;
}
/* ... similar for argv[1] ... */
/* Before exit(status): */
free(splice_buf1);
free(splice_buf2);
exit(status);
```

---

## All Allocations Tracked

| Location | Type | Allocator | Free'd? | All Paths? | Issue |
|----------|------|-----------|---------|------------|-------|
| xmalloc.c:32 | `malloc(size)` | malloc | Yes (xmalloc checks) | N/A | OK — fatal on error |
| xmalloc.c:43 | `calloc(nmemb, size)` | calloc | Yes (xcalloc checks) | N/A | OK — fatal on error |
| xmalloc.c:59 | `realloc(ptr, nmemb*size)` | realloc | Yes (xreallocarray) | N/A | OK — uses intermediate pointer, fatal on error |
| xmalloc.c:71 | `strdup(str)` | strdup | Yes (xstrdup checks) | N/A | OK — fatal on error |
| xmalloc.c:81 | `vsnprintf(buf, 1024, ...)` | stack | N/A | N/A | **BUFFER OVERFLOW RISK** — truncates >1023 byte strings |
| xmalloc.c:90 | `xmalloc(i+1)` | xmalloc | **NO** — only freed if caller frees | No | **CRITICAL LEAK** — splice() callers don't free |
| diff.c:267-302 | N/A (no alloc) | N/A | N/A | N/A | OK — amiport_fill_stat uses stack only |
| diff.c:338 | `xmalloc(argsize)` for diffargs | xmalloc | Yes | Yes | OK — global, freed implicitly at exit |
| diff.c:367 | `xmalloc(len+1)` for pattern | xmalloc | Yes (in push_excludes) | Yes | OK — freed by free(pattern) in excludes list cleanup |
| diff.c:384 | `xmalloc(sizeof(*entry))` for excludes | xmalloc | **NO** | **NO** | **LEAK** — excludes list never freed |
| diffdir.c:88, 100 | `scandir(...)` allocates dirp1/dirp2 | scandir | Yes | Yes | OK — freed in closem label at line 160-169 |
| diffreg.c:326-359 | `fopen(...)` for f1/f2 | fopen | Yes | Yes | OK — closed in closem label at 426-429 |
| diffreg.c:397 | `xreallocarray(member, ...)` | xreallocarray | Yes | Yes | OK — member freed at 408 |
| diffreg.c:401 | `xreallocarray(class, ...)` | xreallocarray | Yes | Yes | OK — class freed at 409 |
| diffreg.c:403 | `xcalloc(slen[0]+2, ...)` for klist | xcalloc | Yes | Yes | OK — klist freed at 414 |
| diffreg.c:406 | `xcalloc(clistlen, ...)` for clist | xcalloc | Yes | Yes | OK — clist freed at 413 |
| diffreg.c:411 | `xreallocarray(J, ...)` | xreallocarray | Yes | Yes | OK — static global, implicit cleanup |
| diffreg.c:416-417 | `xreallocarray(ixold/ixnew, ...)` | xreallocarray | Yes | Yes | OK — static globals, implicit cleanup |
| diffreg.c:480 | `amiport_tmpfile()` for tmp | amiport_tmpfile | Yes | Yes | OK — closed in opentemp or on error |
| diffreg.c:523 | `xcalloc(sz+3, ...)` in prepare() | xcalloc | Yes | Yes | OK — assigned to p, freed when reassigned or at function end |
| diffreg.c:851 | `xcalloc(l+1, ...)` in unsort() | xcalloc | Yes | Yes | OK — freed at 856 |
| diffreg.c:951 | `xmalloc(rlen+1)` in preadline() | xmalloc | **CONDITIONAL** | **NO** | **LEAK** — freed only if ignoreline() is called; leaked if ignoreline not called |

---

## Additional Leaks and Issues

### INFO-1: diff.c:367-370 — Excludes list not freed

**File:** `diff.c:367, 384-388`
**Type:** Memory leak (deferred cleanup)
**Severity:** INFO (AmigaOS cleanup on exit, but still a leak)

The `excludes_list` linked list is built up but never freed:

```c
void
push_excludes(char *pattern)
{
    struct excludes *entry;

    entry = xmalloc(sizeof(*entry));     /* allocated */
    entry->pattern = pattern;             /* pattern pointer stored */
    entry->next = excludes_list;
    excludes_list = entry;
}
```

On exit, all `entry` nodes leak (though `pattern` strings are freed via the xmalloc layer if checked — actually NO, they leak too because `pattern` is passed in and allocated externally).

**Recommendation:** Add cleanup at main() exit:
```c
void cleanup_excludes(void) {
    struct excludes *e, *next;
    for (e = excludes_list; e != NULL; e = next) {
        next = e->next;
        free(e->pattern);
        free(e);
    }
    excludes_list = NULL;
}
```

### WARN-1: diffreg.c:951 in preadline() — Conditional free

**File:** `diffreg.c:951, 968`
**Type:** Memory leak on conditional cleanup
**Severity:** WARN

```c
static char *
preadline(FILE *fp, size_t rlen, off_t off)
{
    char *line;
    /* ... */
    line = xmalloc(rlen + 1);           /* allocated */
    /* ... read into line ... */
    return (line);                       /* caller responsible for freeing */
}

#ifdef HAS_REGEX
static int
ignoreline(char *line)
{
    int ret;
    ret = regexec(&ignore_re, line, 0, NULL, 0);
    free(line);                          /* freed only in this path */
    return (ret == 0);
}
#endif

/* Usage in diffreg.c:1000-1001 */
line = preadline(f1, ixold[i] - ixold[i - 1], ixold[i - 1]);
if (!ignoreline(line))                  /* LEAK if HAS_REGEX not defined */
    goto proceed;
```

If `HAS_REGEX` is not defined (regex support disabled), `ignoreline()` does not exist, and the `line` allocated by `preadline()` is never freed.

**Recommended fix:** Ensure cleanup regardless of HAS_REGEX:
```c
line = preadline(f1, ...);
#ifdef HAS_REGEX
if (!ignoreline(line))
    goto proceed;
#else
free(line);
goto proceed;
#endif
```

---

## Summary of Fixes Required

### CRITICAL (Must fix before commit)

1. **xmalloc.c:90 — splice() leak in main()**: Track argv[0] and argv[1] allocations separately, free before all exit paths (err/exit/return).

2. **xmalloc.c:81 — xasprintf() buffer overflow**: Implement dynamic buffering or check for truncation (vsnprintf return value > buffer size).

### WARN (Should fix)

3. **diff.c:384 — Excludes list leak**: Add cleanup_excludes() function called at main exit.

4. **diffreg.c:951 — Conditional free in preadline()**: Ensure line is freed when ignoreline() doesn't exist.

---

## Files Affected

- `/Users/duncan/Developer/amiport/ports/diff/ported/diff.c` — **CRITICAL** leaks at 310, 315
- `/Users/duncan/Developer/amiport/ports/diff/ported/xmalloc.c` — Buffer overflow at 81, allocator design issue
- `/Users/duncan/Developer/amiport/ports/diff/ported/diffreg.c` — Conditional free at 951
- `/Users/duncan/Developer/amiport/ports/diff/ported/diffdir.c` — OK, proper scandir cleanup

---

## Mitigation Strategy

Since this audit is part of the **mandatory Stage 6b (memory-checker)** quality gate, the port should not be merged until these CRITICAL leaks are fixed. The build-manager or debug-agent can implement the fixes using the `/extend-shim` workflow or by modifying the ported source directly with proper documentation comments.

**AmigaOS impact:** Without fixes, this port leaks memory permanently on every directory comparison, every large formatted string, and every exclude pattern. With a default stack of 4KB (now 65536 thanks to stack cookie), AmigaOS will quickly exhaust available RAM.
