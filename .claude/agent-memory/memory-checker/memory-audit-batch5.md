---
name: Memory safety audit batch 5 (column, ln, expr, look, tty)
description: ports/column, ports/ln, ports/expr, ports/look, ports/tty memory safety reviews (2026-03-26)
type: reference
---

# Memory Safety Audit — Batch 5

**Date:** 2026-03-26
**Ports Audited:** column 1.27, ln 1.25, expr 1.28, look 1.27, tty 1.14
**Verdict:** 2 CLEAN (ln, look, tty), 2 FAIL (column, expr)

## Allocation Summary

| Port | Category | Issues | Verdict |
|------|----------|--------|---------|
| **column** | Category 2 | 5 CRITICAL: separator strdup, cols array, maxwidths array, getline buf, table entries | **CANNOT SHIP** |
| **ln** | Category 2 | 0 | **APPROVED** |
| **expr** | Category 1 | 6+ CRITICAL: eval*() leaks on errx() paths | **CANNOT SHIP** |
| **look** | Category 2 | 0 | **APPROVED** |
| **tty** | Category 1 | 0 | **APPROVED** |

---

## Port-by-Port Details

### column 1.27 — CRITICAL LEAKS FOUND

**File:** `/Users/duncan/Developer/amiport/ports/column/ported/column.c`

#### Leak 1: separator strdup (Line 140)

```c
case 's':
    if ((separator = strdup(optarg)) == NULL)
        err(10, NULL);
    break;
```

- **Type:** strdup malloc'd string
- **Allocation:** Line 140
- **Freed?** No
- **Path?** If `-s sep` option used, separator is malloc'd but never freed
- **Impact:** ~10-50 bytes per invocation

**Fix:** Add to cleanup() function:
```c
if (separator && separator != "\t ") free(separator);
```

#### Leak 2: cols array (Line 332)

```c
cols = ereallocarray(cols, maxcols, sizeof(*cols));
```

- **Type:** struct field array grown via realloc
- **Allocation:** Line 332 (and subsequent reallocations at same line)
- **Freed?** No
- **Path?** Never freed at exit
- **Impact:** 50-500 bytes total (1000 columns max at DEFCOLS=25)

**Root cause:** Static local variable in input() function. cleanup() has no visibility into how much was allocated. Requires global tracking.

#### Leak 3: maxwidths array (Line 334)

```c
maxwidths = ereallocarray(maxwidths, maxcols, sizeof(*maxwidths));
```

- **Type:** int array
- **Allocation:** Line 334
- **Freed?** No
- **Path?** Never freed
- **Impact:** 50-200 bytes (same as cols)

#### Leak 4: getline buf (Line 283)

```c
while ((llen = getline(&buf, &blen, fp)) > -1) {
```

- **Type:** malloc'd buffer from getline()
- **Allocation:** Line 283 (internally by libnix getline)
- **Freed?** No — buf is never free()'d after getline() returns
- **Path?** Never freed at exit
- **Impact:** 4096-65536 bytes (BUFSIZ or larger)
- **Severity:** CRITICAL — among the largest per-invocation

**Fix:** Track buf and free in cleanup():
```c
static char *getline_buf = NULL;

/* In input(): */
while ((llen = getline(&getline_buf, &blen, fp)) > -1) {
    buf = getline_buf;
    ...
}

/* In cleanup(): */
if (getline_buf) free(getline_buf);
```

#### Leak 5: table entries (Lines 367-371)

```c
table[entries] = ereallocarray(NULL, col + 1, sizeof(*(table[entries])));
table[entries][col].content = NULL;
while (col--)
    table[entries][col] = cols[col];
entries++;
```

- **Type:** Array of struct field* pointers, each pointing to malloc'd content
- **Allocation:** Line 367 (array header) + strdup calls at line 353 (content)
- **Freed?** No
- **Path?** Never freed
- **Impact:** 100-1000+ bytes per entry (huge for large files)

**Example leak scenario:**
- Input: 10,000 lines × 5 columns = 50,000 field entries
- Each entry has malloc'd content (line 353: `strdup(s)`)
- Total leak: ~500KB to 2MB

**Combined leak impact:** 600KB-2.5MB for large inputs. Permanent memory loss on AmigaOS with `-noixemul`.

---

### ln 1.25 — CLEAN

**File:** `/Users/duncan/Developer/amiport/ports/ln/ported/ln.c`

**Allocations:**
- argv expansion: ✓ freed via atexit(cleanup) at line 145
- __progname: ✓ freed via amiport_free_argv()
- path buffer: stack-local, N/A

**All paths covered.** Perfect atexit cleanup pattern.

**Verdict:** APPROVED FOR SHIPPING

---

### expr 1.28 — CRITICAL LEAKS ON ERROR PATHS

**File:** `/Users/duncan/Developer/amiport/ports/expr/ported/expr.c`

#### Overview

The expr program uses recursive descent parsing with make_int()/make_str() creating malloc'd struct val objects. The eval*() family of functions allocate intermediate values but fail to free them on error paths.

#### Leak 1: eval5() regex compilation (Line 304)

```c
if ((eval = regcomp(&rp, r->u.s, 0)) != 0) {
    regerror(eval, &rp, errbuf, sizeof(errbuf));
    errx(10, "%s", errbuf);  /* <-- exits without freeing l, r */
}
```

- **Allocations:** `l` (from eval6 at line 293), `r` (from eval6 at line 296)
- **Freed before exit?** No
- **Impact:** Two malloc'd struct val pointers lost per regex error
- **Severity:** CRITICAL — regex errors are user-triggerable

**Leak scenario:**
```bash
expr "abc" : "[invalid-regex"
```
- Line 296: r = eval6() allocates "u[invalid-regex"
- Line 293: l = eval6() allocates "abc"
- Line 304: regcomp() fails, l and r are never freed

#### Leak 2-6: Binary operators with error paths (Lines 358, 361, 372, 379, 420, 427)

All these follow the same pattern: allocate l and r, then errx() on error without cleanup.

**Line 358:**
```c
if (!to_integer(l, &errstr))
    errx(10, "number \"%s\" is %s", l->u.s, errstr);  /* l, r leak */
```

**Line 361:**
```c
if (!to_integer(r, &errstr))
    errx(10, "number \"%s\" is %s", r->u.s, errstr);  /* l, r leak */
```

**Lines 372, 379, 420, 427:** Similar pattern

#### Root Cause

The eval*() functions follow a recursive descent pattern where each function calls lower-precedence functions and processes operators. They allocate intermediate values but the error path via errx() exits immediately without cleanup.

**Example flow:**
1. eval0() calls eval1() → allocates `l` via eval1()
2. eval1() calls eval2() → allocates `r` via eval2()
3. eval2() processes operators, calls to_integer() which may fail
4. to_integer() calls strtonum() which may fail
5. Caller calls errx() which exits immediately
6. Neither eval1() nor eval0() gets a chance to free l or r

#### Leak Count

Regex compilation error: 2 values (l, r)
Numeric errors in eval4: 2 values per error (l, r) × 6 locations = 12 values max
Numeric errors in eval3: 2 values per error × 4 locations = 8 values max
Numeric errors in eval2: 2 values per error × 6 locations = 12 values max

**Total per execution:** 2-30+ malloc'd values depending on error position

**Fix required:** Add cleanup before every errx():

```c
/* In eval5(), line 304-308: */
if ((eval = regcomp(&rp, r->u.s, 0)) != 0) {
    free_value(l);  /* MISSING */
    free_value(r);  /* MISSING */
    regerror(eval, &rp, errbuf, sizeof(errbuf));
    errx(10, "%s", errbuf);
}

/* Repeat for all errx() in eval2-eval5 */
```

**Verdict:** CANNOT SHIP — requires systematic cleanup additions

---

### look 1.27 — CLEAN

**File:** `/Users/duncan/Developer/amiport/ports/look/ported/look.c`

**Allocations:**
- argv expansion: ✓ freed via atexit(cleanup) at line 107
- mmap buffer: ✓ munmap called at line 182 before exit
- Local arrays: stack-allocated, N/A

**All paths covered.** mmap/munmap properly paired. atexit cleanup correct.

**Verdict:** APPROVED FOR SHIPPING

---

### tty 1.14 — CLEAN

**File:** `/Users/duncan/Developer/amiport/ports/tty/ported/tty.c`

**Allocations:**
- argv expansion: ✓ freed via atexit(cleanup) at line 78
- Local variables (sflag, ch, t): stack-allocated, N/A

**No dynamic allocations beyond argv expansion.**

**Verdict:** APPROVED FOR SHIPPING

---

## Cleanup Pattern Analysis

### CLEAN Ports (ln, look, tty)

All three follow a consistent pattern:

```c
static void cleanup(void) {
    amiport_free_argv();
    (void)fflush(stdout);
}

int main(int argc, char *argv[]) {
    amiport_expand_argv(&argc, &argv);
    atexit(cleanup);  /* EARLY registration */

    /* ... rest of code ... */
}
```

**Key points:**
1. Expand argv immediately (line 105/143)
2. Register atexit(cleanup) immediately (line 107/145)
3. Cleanup function only calls amiport_free_argv() + fflush()
4. No other dynamic allocations that need tracking

### FAIL Ports (column, expr)

**column:** Allocations happen inside helper functions (input()) where cleanup() has no visibility. Static local variables (cols, maxwidths, maxentry) prevent tracking.

**expr:** Error paths via errx() bypass cleanup logic. eval*() functions allocate values but don't free them before errx().

---

## Notes on Unfixability

### column — Potentially unfixable without major refactoring

The input() function uses static growth pattern:
```c
static int maxentry = 0;
static int maxcols = 0;
static struct field *cols = NULL;
```

To fix, would require either:
1. Convert static to global and track in cleanup() — breaks function encapsulation
2. Return allocation metadata from input() — requires signature changes
3. Restructure entire input processing — significant effort

**Decision:** Can be fixed but requires redesign of input() function.

### expr — Fixable but systematic

Every eval*() function that calls errx() needs cleanup. ~8-10 locations need changes.

**Decision:** Fixable with systematic approach (add free_value calls before errx).

---

## Severity Classification

| Port | Severity | Fixability | Recommendation |
|------|----------|-----------|---|
| column | CRITICAL | MEDIUM (refactoring required) | Fix required before shipping. Likely ~100-200 LOC changes. |
| ln | CLEAN | N/A | Ship as-is. |
| expr | CRITICAL | HIGH (systematic additions) | Fix required before shipping. ~8 locations, ~1-2 lines each. |
| look | CLEAN | N/A | Ship as-is. |
| tty | CLEAN | N/A | Ship as-is. |

---

## Known Pitfalls Applied

- **Pitfall: Exit Path Cleanup** (from known-pitfalls.md) — AmigaOS has no automatic process memory cleanup with `-noixemul`. Both ports violate this: column doesn't free allocations made in input(), expr doesn't cleanup before err()/errx().
- **Pitfall: atexit() for argv Expansion Cleanup** — ln, look, tty all follow this correctly. column and expr also use it, but have OTHER allocations not covered.

