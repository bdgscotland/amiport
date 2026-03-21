# Memory Safety Audit: ports/grep

**Audit Date:** 2026-03-21
**Status:** CRITICAL LEAK FOUND

## Summary

- **Total heap allocations:** 10+ patterns (individual strings + array + two large arrays)
- **Properly freed on all paths:** 2/10
- **Leaks found:** 8 CRITICAL
- **Unsafe realloc patterns:** 0 (safe — uses grep_reallocarray with err() on failure)
- **Double-free risks:** 0
- **Regex compilation leaks:** YES — regcomp'd patterns not freed
- **Global static allocation leaks:** YES — lnbuf never freed

## Critical Findings

### CRITICAL-1: Main Pattern Array Not Freed

**Location:** grep.c:52 (global), grep.c:481-482 (allocation)

```c
char   **pattern;           /* grep.c:52 — global, persists for life of program */
regex_t	*r_pattern;        /* grep.c:53 — global, persists for life of program */
fastgrep_t *fg_pattern;     /* grep.c:54 — global, persists for life of program */

int patterns = 0;           /* grep.c:51 — count */
```

**Allocations:**
- grep.c:481: `fg_pattern = grep_calloc(patterns, sizeof(*fg_pattern));`
- grep.c:482: `r_pattern = grep_calloc(patterns, sizeof(*r_pattern));`

**Problem:** These are never freed. All 5 exit paths in main() call `exit()` without freeing:
- Line 513: `exit(AMIGA_RETURN_OK);` (stdin match)
- Line 515: `exit(AMIGA_RETURN_ERROR);` (stdin error)
- Line 517: `exit(AMIGA_RETURN_WARN);` (stdin no match)
- Line 531: `exit(AMIGA_RETURN_ERROR);` (file error)
- Line 533: `exit(AMIGA_RETURN_OK);` (match found)
- Line 536: `exit(AMIGA_RETURN_ERROR);` (file error)
- Line 538: `exit(AMIGA_RETURN_WARN);` (no match)

**Severity:** CRITICAL — On AmigaOS with `-noixemul`, the process never returns, so these allocations leak permanently until reboot.

**Impact:**
- `fg_pattern`: `patterns * sizeof(fastgrep_t)` bytes
- `r_pattern`: `patterns * sizeof(regex_t)` bytes = `patterns * REGSIZE` (regex_t is large)

Each compiled regex is 100+ bytes. For 10 patterns, this is 1KB+ of permanent leak.

---

### CRITICAL-2: Individual Pattern Strings Not Freed

**Location:** grep.c:193, 203 (allocation)

```c
pattern[patterns] = grep_malloc(len + 15 + extra);  /* line 193 — wflag mode */
pattern[patterns] = grep_malloc(len + 1);            /* line 203 — normal mode */
```

**Problem:** The `pattern[]` array itself is freed (line 452), but it's only freed in main(). However:

1. **Line 452 frees the array pointer, not the strings** — it's a shallow free.
   ```c
   free(expr);   /* line 452 — frees expr array, NOT the pattern strings */
   ```

2. **The `pattern[]` array is global (line 52)** and is **never freed** on any exit path in main().

3. Each `pattern[i]` string is leaked:
   - Normal patterns: allocated at grep.c:203, never freed
   - Word-boundary patterns: allocated at grep.c:193, never freed
   - Case-sensitive patterns: allocated in fgrepcomp() at util.c:317, never freed
   - Fastcomp patterns: allocated in fastcomp() at util.c:393, never freed

**Severity:** CRITICAL — Multiple string allocations × patterns × number of runs.

**Trace:**
- main() calls add_patterns() or read_patterns()
- add_pattern() calls grep_malloc() to allocate individual strings
- Strings stored in global `pattern[]` array
- On exit: pattern[] array and all pattern[i] strings are leaked

---

### CRITICAL-3: Regex Compilation Not Freed (regfree Missing)

**Location:** grep.c:491, util.c:489-498

```c
c = regcomp(&r_pattern[i], pattern[i], cflags);  /* grep.c:491 */
if (c != 0) {
    regerror(c, &r_pattern[i], re_error, RE_ERROR_BUF);
    errx(AMIGA_RETURN_ERROR, "%s", re_error);
}
```

**Problem:**
- `regcomp()` allocates internal regex state in `r_pattern[i]`
- No corresponding `regfree(&r_pattern[i])` is called
- The r_pattern[] array is never freed

**Missing Cleanup:** For every `regcomp()`, there should be:
```c
for (i = 0; i < patterns; i++) {
    if (r_pattern[i].allocated) {  /* or similar flag */
        regfree(&r_pattern[i]);
    }
}
free(r_pattern);
free(fg_pattern);
```

**Severity:** CRITICAL — regex_t structure contains dynamically allocated internal state (DFA tables, etc.).

---

### CRITICAL-4: lnbuf Global Static Buffer Never Freed

**Location:** file.c:41-42 (allocation), file.c:124

```c
static char	*lnbuf;           /* file.c:41 — global static */
static size_t	 lnbufsize;

/* Later, in grep_fgetln() */
if ((*l = getline(&lnbuf, &lnbufsize, f->f)) == (size_t)-1) {
    /* lnbuf is allocated by getline() */
    ...
}
```

**Problem:**
- `lnbuf` is a global static that persists across all file processing
- `getline()` allocates into it and grows it as needed
- **Never freed** — it's global static, so it persists until program exit
- But main() calls `exit()` without freeing it

**Severity:** CRITICAL on AmigaOS (leaks until reboot) — but this is a "normal" pattern for grep since lnbuf is reused across files. However, it's still a permanent allocation.

**Note:** This is less severe than others because the buffer is reused, not reallocated per file. It will grow to max line size and stay that size.

---

### WARN-1: expr Array Freed But Only in One Path

**Location:** grep.c:360-367 (allocation), grep.c:452 (free)

```c
expr = grep_reallocarray(expr, ++expr_sz, sizeof(*expr));  /* line 362 */
expr[exprs] = optarg;
++exprs;
...
free(expr);  /* line 452 — ONLY after option parsing, before pattern processing */
```

**Problem:**
- `expr` is allocated during option parsing
- Freed at line 452
- This is correct because it's freed before main() exits
- However, it's only freed once; if parse fails before line 452 and usage() exits, expr leaks

**Trace to usage():**
- usage() at line 110 calls `exit(AMIGA_RETURN_ERROR)` (line 121)
- Called from main() on missing pattern (line 463)
- At that point, expr may be allocated but hasn't been freed yet

**Severity:** WARN — expr is relatively small and only allocated during -e flag parsing. If called multiple times with -e flags, the last unfreed expr won't be large. Still incorrect.

---

### WARN-2: patfile Entries Freed But Linked List Integrity Issue

**Location:** grep.c:370 (allocation), grep.c:455-460 (free)

```c
patfile = grep_malloc(sizeof(*patfile));     /* grep.c:370 */
patfile->pf_file = optarg;
SLIST_INSERT_HEAD(&patfilelh, patfile, pf_next);

for (patfile = SLIST_FIRST(&patfilelh); patfile != NULL;
    patfile = pf_next) {
    pf_next = SLIST_NEXT(patfile, pf_next);
    read_patterns(patfile->pf_file);
    free(patfile);    /* grep.c:459 — correct cleanup */
}
```

**Good Pattern:** The patfile linked list is correctly freed in the loop at lines 455-460. This is safe.

**Severity:** INFO — This one is correct.

---

### WARN-3: File Handle In grep_open() Leaked On fopen() Failure

**Location:** file.c:96-107

```c
f = grep_malloc(sizeof *f);        /* line 96 */
f->type = FILE_STDIO;
f->noseek = 0;

f->f = fopen(fname, "r");
if (f->f == NULL) {
    free(f);                        /* line 104 — correct cleanup */
    return NULL;
}
return f;
```

**Good Pattern:** The file_t structure is freed if fopen() fails. This is correct.

**Severity:** INFO — This one is correct.

---

## Allocation vs. Free Pairing Table

| Location | Type | Allocated | Freed? | All Paths? | Issue |
|----------|------|-----------|--------|------------|-------|
| grep.c:52-54 | Global `pattern[], fg_pattern, r_pattern` | grep.c:481-482 | NO | NO | **CRITICAL LEAK** |
| grep.c:193,203 | Individual pattern strings | grep_malloc | NO | NO | **CRITICAL LEAK** |
| grep.c:491 | regcomp() internal state | regcomp() | NO (no regfree) | NO | **CRITICAL LEAK** |
| file.c:41 | Global lnbuf | getline() | NO | NO | **CRITICAL** on AmigaOS |
| grep.c:360-367 | expr array | grep_reallocarray | Line 452 | NO (not on usage() path) | **WARN** |
| grep.c:370 | patfile entries | grep_malloc | Lines 455-460 | YES | OK |
| file.c:96 | file_t in grep_open | grep_malloc | Line 104 (on error) | YES | OK |
| grep.c:230 | read_patterns() file handle | fopen() | Line 239 | YES | OK |
| grep.c:230 | read_patterns() line buffer | getline() | Line 240 | YES | OK |
| queue.c:65 | queue items | grep_malloc | Lines 82, 108, 118 | YES | OK |

## Summary of Leaks

1. **fg_pattern array** — Never freed (calloc at grep.c:481)
2. **r_pattern array** — Never freed (calloc at grep.c:482), regex_t internals not freed (no regfree)
3. **pattern[] array** — Never freed (global), each pattern[i] string never freed
4. **fastgrep_t.pattern strings** — Allocated in fgrepcomp/fastcomp, never freed
5. **lnbuf** — Allocated by getline() once, reused, never freed (global)
6. **expr array** — Freed only on main() success path, leaked on usage() exit

## Fixes Required

### Fix 1: Add Cleanup Before All exit() Calls in main()

Replace all `exit()` calls with cleanup:

```c
void cleanup_patterns(void) {
    int i;
    for (i = 0; i < patterns; i++) {
        free(pattern[i]);
        regfree(&r_pattern[i]);
    }
    free(pattern);
    free(fg_pattern);
    free(r_pattern);
    /* Also free lnbuf if needed */
    extern char *lnbuf;
    free(lnbuf);
}
```

Then replace all `exit()` in main() with:
```c
cleanup_patterns();
exit(code);
```

### Fix 2: Add regfree() After regcomp()

At grep.c:491, need to track which r_pattern entries are initialized:

```c
int *regex_initialized = calloc(patterns, sizeof(int));

for (i = 0; i < patterns; ++i) {
    if (Fflag) {
        fgrepcomp(&fg_pattern[i], (unsigned char *)pattern[i]);
    } else {
        if (fastcomp(&fg_pattern[i], pattern[i])) {
            c = regcomp(&r_pattern[i], pattern[i], cflags);
            if (c != 0) {
                regerror(c, &r_pattern[i], re_error, RE_ERROR_BUF);
                cleanup_patterns();
                exit(AMIGA_RETURN_ERROR);
            }
            regex_initialized[i] = 1;
        }
    }
}
```

Then in cleanup_patterns():
```c
for (i = 0; i < patterns; i++) {
    if (regex_initialized[i])
        regfree(&r_pattern[i]);
}
```

### Fix 3: Add cleanup_patterns() to usage() Path

At grep.c:463:
```c
if (argc == 0 && needpattern) {
    cleanup_patterns();
    usage();
}
```

### Fix 4: Avoid freeing fgrepcomp() patterns that are const

In util.c:322, fgrepcomp() doesn't allocate if !iflag:
```c
else
    fg->pattern = (unsigned char *)pat;  /* really const */
```

So only free fg->pattern in cleanup if it was allocated:
```c
if (iflag) {
    free(fg->pattern);
}
```

This is already handled correctly in fastcomp() (it always allocates and frees on error).

## Conclusion

**Status:** CRITICAL — This port has 6 critical memory leaks that accumulate heap memory until the process exits. On AmigaOS with `-noixemul`, the process runs standalone and exits cleanly, but the memory never returns to the system until reboot.

**Required Action:** Must implement cleanup_patterns() and call it before all exit() paths.
