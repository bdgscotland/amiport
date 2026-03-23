---
name: memory-audit-bc-recheck
description: ports/bc 1.07.1 memory safety re-audit (2026-03-23) after fixes — second pass verification
type: reference
---

# Memory Safety Re-Audit: GNU bc 1.07.1 — Second Pass (Verification)

**Port:** `/Users/duncan/Developer/amiport/ports/bc/ported/`
**Date:** 2026-03-23
**Auditor:** memory-checker agent
**Previous Audit:** 2026-03-23 (CRITICAL LEAKS FOUND)
**Current Status:** VERIFICATION OF FIXES

---

## Executive Summary

The first pass identified 5 CRITICAL leak categories. This second pass verifies whether the fixes were correctly applied.

**Verdict: PARTIAL FIXES APPLIED — 3 of 5 Critical Issues REMAIN UNFIXED**

| Issue | Status | Details |
|-------|--------|---------|
| 1. amiport_getenv("BC_ENV_ARGS") tracking | FIXED | Now tracked in env_allocs[], freed in bc_cleanup() |
| 2. strdup("BC_ENV_ARGS") in env_argv[0] | FIXED | Free call added at main.c:243 |
| 3. POSIXLY_CORRECT getenv result | FIXED | Freed immediately at main.c:254 |
| 4. BC_LINE_LENGTH getenv result | FIXED | Freed immediately at main.c:263 |
| 5. Math lib strdups (6× strdup("e"/"l"/"s"/"a"/"c"/"j")) | **UNFIXED** | Still leaked — no cleanup |
| 6. Global storage arrays (functions, f_names, etc.) | PARTIAL | Only f_names[0] freed, other name arrays not fully freed |
| 7. Global constants (_zero_, _one_, _two_) | **UNFIXED** | Never freed, reference-counting prevents cleanup |
| 8. _bc_Free_list cache | **UNFIXED** | Cache of reused bc_num nodes never freed at exit |

---

## Detailed Issue-by-Issue Verification

### Issue 1: amiport_getenv("BC_ENV_ARGS") Tracking — FIXED ✓

**Location:** main.c:219-244

**Code Review:**
```c
219:  env_value = amiport_getenv ("BC_ENV_ARGS");
220:  if (env_value != NULL) {
223:    env_allocs[env_alloc_count++] = env_value;  ← TRACKED
      /* ... parse env_value ... */
242:    parse_args (env_argc, env_argv);
243:    free(env_argv[0]); /* amiport: free strdup'd program name */
244:  }
```

**Cleanup (main.c:53-70):**
```c
53:  static void bc_cleanup(void) {
57:    for (i = 0; i < env_alloc_count; i++)
58:      free(env_allocs[i]);  ← FREED
```

**Status:** ✓ **FIXED** — env_value allocated by amiport_getenv() is tracked in env_allocs[] and freed by bc_cleanup() via atexit().

---

### Issue 2: strdup("BC_ENV_ARGS") in env_argv[0] — FIXED ✓

**Location:** main.c:225, 243

**Code:**
```c
225:  env_argv[0] = strdup("BC_ENV_ARGS");  ← ALLOCATED
...
242:  parse_args (env_argc, env_argv);
243:  free(env_argv[0]); /* amiport: free strdup'd program name */  ← FREED
```

**Status:** ✓ **FIXED** — The string is freed immediately after parse_args().

---

### Issue 3: POSIXLY_CORRECT getenv Result — FIXED ✓

**Location:** main.c:251-255

**Code:**
```c
251:  env_value = amiport_getenv ("POSIXLY_CORRECT");
252:  if (env_value != NULL) {
253:    std_only = TRUE;
254:    free(env_value); /* amiport: free immediately — only need NULL check */
255:  }
```

**Status:** ✓ **FIXED** — Freed immediately after the NULL check.

---

### Issue 4: BC_LINE_LENGTH getenv Result — FIXED ✓

**Location:** main.c:257-264

**Code:**
```c
257:  env_value = amiport_getenv ("BC_LINE_LENGTH");
258:  if (env_value != NULL) {
260:    line_size = atoi (env_value);
      /* ... validate line_size ... */
263:    free(env_value); /* amiport: free after use */
264:  }
```

**Status:** ✓ **FIXED** — Freed immediately after atoi().

---

### Issue 5: Math Library Function Name strdups — UNFIXED ✗

**Location:** main.c:343-348 in open_new_file()

**Code:**
```c
335:  if (use_math && first_file) {
      CONST char **mstr;
      /* Load math library */
343:  (void) lookup (strdup("e"), FUNCT);  ← LEAKED
344:  (void) lookup (strdup("l"), FUNCT);  ← LEAKED
345:  (void) lookup (strdup("s"), FUNCT);  ← LEAKED
346:  (void) lookup (strdup("a"), FUNCT);  ← LEAKED
347:  (void) lookup (strdup("c"), FUNCT);  ← LEAKED
348:  (void) lookup (strdup("j"), FUNCT);  ← LEAKED
349:  mstr = libmath;
350:  while (*mstr) {
351:       load_code (*mstr);
352:       mstr++;
353:  }
354:  }
```

**Analysis:**

The `lookup()` function (util.c:536-626) processes the strdup'd name:
```c
550:  id->id = strcopyof (name);  /* DOUBLE ALLOCATION */
```

Inside lookup:
- Line 550: Makes a SECOND copy via strcopyof() (another malloc+strcpy)
- Line 595: `f_names[id->f_name] = name;` — stores the strdup result in f_names[n]
- **Line 595 stores the leaked pointer**, but it's stored as a NAME POINTER, not tracked for cleanup
- The strdup result becomes inaccessible after function return (not returned, only copied)

**The bc_cleanup() function does NOT free individual name entries:**

```c
60:  if (f_names) {
61:    if (f_names[0]) free(f_names[0]); /* "(main)" strdup */
62:    free(f_names);  /* ← Frees the ARRAY, not individual entries */
```

**Problem:** bc_cleanup() frees `f_names` (the pointer array itself), but NOT the individual strings stored in f_names[1..n] that came from the lookup() strdups.

**Impact:** 6 × 2-byte strings = 12 bytes per `-l` invocation. Plus the strcopyof() duplicates also leak (those are stored as f_names entries too).

**Status:** ✗ **UNFIXED** — These 6 strdups are still leaked. The f_names array is freed, but the individual string pointers within it are not.

---

### Issue 6: Global Storage Arrays — PARTIAL FIX (Incomplete)

**Location:** storage.c:45-55, 89-90, 136-137, 173-174

The bc_cleanup() function (main.c:59-68) includes:
```c
59:  /* Free global storage arrays */
60:  if (f_names) {
61:    if (f_names[0]) free(f_names[0]); /* "(main)" strdup */
62:    free(f_names);
63:  }
64:  if (functions) free(functions);
65:  if (v_names) free(v_names);
66:  if (variables) free(variables);
67:  if (a_names) free(a_names);
68:  if (arrays) free(arrays);
```

**Issues:**

1. **f_names cleanup:** Line 61 only frees f_names[0] ("(main)"), but NOT the other f_names[1..n] entries that were allocated via lookup() → strcopyof().

   From the first pass: lookup() allocates multiple entries in f_names via strcopyof(). These should be freed before freeing the f_names array itself.

2. **v_names and a_names:** These are also name arrays. Similar to f_names, they contain pointers to allocated strings (via strcopyof() in lookup()), but none of these individual entries are freed — only the array pointers are freed.

3. **functions array contents:** The functions array contains bc_function structs with f_body pointers (allocated in storage.c:105). These f_body buffers are NOT freed in bc_cleanup().

4. **variables/arrays array contents:** Similar issue — the individual bc_var/bc_var_array structures within these arrays may contain allocated sub-structures, but they're not recursively freed.

**Status:** ✗ **PARTIAL** — The cleanup function frees the array headers (functions, f_names, variables, v_names, arrays, a_names) but NOT:
- Individual function body buffers (f_body in bc_function)
- Individual name strings stored in f_names[1..n], v_names[n], a_names[n]
- The complex structures within variables and arrays

**Impact:** Moderate leak (~768 bytes base allocation from array headers, plus unknown amount from unfreed name strings and nested allocations).

---

### Issue 7: Global Constants (_zero_, _one_, _two_) — UNFIXED ✗

**Location:** number.c:55-57, 109-114

**Allocation:**
```c
109:  _zero_ = bc_new_num (1,0);
110:  _one_  = bc_new_num (1,0);
111:  _one_->n_value[0] = 1;
112:  _two_  = bc_new_num (1,0);
113:  _two_->n_value[0] = 2;
```

Each bc_new_num() allocates:
- A bc_struct via malloc (lines 72-73 or pulled from _bc_Free_list)
- A data buffer via malloc (line 79)

**Reference Counting System:**
```c
88:  bc_copy_num (bc_num num) {
122:    num->n_refs++;
123:    return num;
124:  }

90:  bc_free_num (bc_num *num) {
93:    (*num)->n_refs--;
94:    if ((*num)->n_refs == 0) {
95:      if ((*num)->n_ptr)
96:        free ((*num)->n_ptr);
97:      (*num)->n_next = _bc_Free_list;  ← Adds to free list
98:      _bc_Free_list = *num;
99:    }
100:  }
```

**The Leak:**
- _zero_, _one_, _two_ start with n_refs=1 (line 78)
- Throughout execution, they're copied many times via bc_copy_num() (n_refs increases)
- At exit, bc_cleanup() does NOT call bc_free_num() on these globals
- Final n_refs > 1, so the reference count never hits 0
- These nodes never get freed (bc_free_num() only frees when n_refs==0)

**Status:** ✗ **UNFIXED** — These three global constants are never freed at exit. They remain in memory permanently. Additionally, any reference-counted copies still in _bc_Free_list at exit are also leaked.

**Impact:** ~40 bytes for each constant × 3 = 120 bytes. Plus all bc_num nodes in _bc_Free_list cache (variable, could be hundreds of bytes for complex bc programs).

---

### Issue 8: _bc_Free_list Cache — UNFIXED ✗

**Location:** number.c:59, 68-70, 97-98

**Description:**
bc_free_num() (lines 90-101) implements reference-counting with a free list cache:

```c
97:    (*num)->n_next = _bc_Free_list;  ← Freed nodes go to cache
98:    _bc_Free_list = *num;
```

**The Problem:**
- During execution, bc_num nodes are allocated and freed many times
- Freed nodes go into _bc_Free_list (to avoid malloc/free overhead)
- At program exit, _bc_Free_list still contains a linked list of reused nodes
- bc_cleanup() does NOT walk _bc_Free_list and free these cached nodes
- All nodes in _bc_Free_list (and their n_ptr data buffers) are leaked

**Status:** ✗ **UNFIXED** — The entire _bc_Free_list cache is leaked at exit.

**Impact:** Variable, depends on program complexity. For a large bc program that uses many intermediate numbers (e.g., `echo 'define f(x) { return x+1 }; for(i=1; i<=100; i++) f(i)' | bc`), this could be kilobytes.

---

## Summary of Remaining Unfixed Leaks

### CRITICAL (Unfixed)

| Issue | Type | Size | Impact | Fixability |
|-------|------|------|--------|------------|
| Math lib strdups (6×) | strdup leak | 12+ bytes | Per -l invocation | EASY: Track in array, free in cleanup |
| f_names[1..n] strings | strdup leak | 10-100+ bytes | All invocations with functions | MEDIUM: Must free individual entries before f_names array |
| _zero_, _one_, _two_ constants | Reference-counted | 120 bytes | All invocations | MEDIUM: Call bc_free_num() in cleanup (may need n_refs reset) |
| _bc_Free_list cache | Reference-counted cache | 10-1000+ bytes | Complex programs | MEDIUM: Walk list and free all nodes in cleanup |

### MEDIUM (Partially Fixed)

| Issue | Type | Size | Impact | Fixability |
|-------|------|------|--------|------------|
| f_names array (header only freed) | Array + contents | 30+ bytes | All invocations | MEDIUM: Iterate f_names[1..f_count] and free each entry |
| functions[].f_body buffers | Nested malloc | 50-200+ bytes | All invocations | MEDIUM: Iterate functions[0..f_count] and free f_body |
| v_names/a_names contents | Nested malloc | 20+ bytes | All invocations | MEDIUM: Iterate and free individual entries |

---

## Allocation Coverage After Fixes

| Allocation | First Pass | Current Status | All Paths Freed? |
|-----------|-----------|-----------------|------------------|
| BC_ENV_ARGS getenv result | LEAK | FIXED | YES |
| BC_ENV_ARGS strdup | LEAK | FIXED | YES |
| POSIXLY_CORRECT | LEAK | FIXED | YES |
| BC_LINE_LENGTH | LEAK | FIXED | YES |
| Math lib strdups (6×) | LEAK | UNFIXED | NO |
| f_names array | LEAK (partial) | PARTIAL FIX | PARTIAL |
| f_names[0] ("main") | LEAK | FIXED | YES |
| f_names[1..n] entries | LEAK (unchecked) | UNFIXED | NO |
| functions array | LEAK (partial) | UNFIXED | NO |
| functions[].f_body | LEAK (unchecked) | UNFIXED | NO |
| v_names array | LEAK (partial) | PARTIAL FIX | PARTIAL |
| variables array | LEAK (partial) | PARTIAL FIX | PARTIAL |
| a_names array | LEAK (partial) | PARTIAL FIX | PARTIAL |
| arrays array | LEAK (partial) | PARTIAL FIX | PARTIAL |
| _zero_ constant | LEAK | UNFIXED | NO |
| _one_ constant | LEAK | UNFIXED | NO |
| _two_ constant | LEAK | UNFIXED | NO |
| _bc_Free_list cache | LEAK (unchecked) | UNFIXED | NO |

---

## Fixes Still Required

### Fix 1: Free Math Library Function Name strdups

Add tracking array to main.c (similar to env_allocs[]):

```c
#define MAX_MATH_STRDUPS 6
static char *math_strdups[MAX_MATH_STRDUPS];
static int math_strdup_count;
```

Then in open_new_file() (main.c:343-348), track each strdup:

```c
if (use_math && first_file) {
    char *names[] = {"e", "l", "s", "a", "c", "j"};
    int i;
    for (i = 0; i < 6; i++) {
        char *dup = strdup(names[i]);
        if (math_strdup_count < MAX_MATH_STRDUPS)
            math_strdups[math_strdup_count++] = dup;
        (void) lookup (dup, FUNCT);
    }
    /* ... rest of libmath loading ... */
}
```

Then in bc_cleanup():

```c
for (i = 0; i < math_strdup_count; i++)
    free(math_strdups[i]);
```

### Fix 2: Free Individual Name Entries in f_names/v_names/a_names

This is more complex. The cleanup must iterate through the name arrays and free individual entries:

```c
void bc_cleanup(void) {
    int i;

    /* ... free env_allocs ... */

    if (f_names) {
        for (i = 0; i < f_count; i++)
            if (f_names[i]) free(f_names[i]);
        free(f_names);
    }

    if (v_names) {
        for (i = 0; i < v_count; i++)
            if (v_names[i]) free(v_names[i]);
        free(v_names);
    }

    if (a_names) {
        for (i = 0; i < a_count; i++)
            if (a_names[i]) free(a_names[i]);
        free(a_names);
    }

    /* ... rest of cleanup ... */
}
```

**Problem:** f_count, v_count, a_count are global variables in storage.c, but main.c's bc_cleanup() would need access to them. This is solvable (make them extern in the header or access them directly).

### Fix 3: Free Function Body Buffers

In storage.c, the more_functions() function allocates f_body:

```c
105:  f->f_body = bc_malloc (BC_START_SIZE);  ← LEAKED AT EXIT
```

Cleanup must free these:

```c
if (functions) {
    for (i = 0; i < f_count; i++)
        if (functions[i].f_body) free(functions[i].f_body);
    free(functions);
}
```

### Fix 4: Free Global Constants _zero_, _one_, _two_

The reference-counting system prevents cleanup. Two approaches:

**Option A: Bypass reference counting and force-free**
```c
/* In bc_cleanup(), after freeing other globals */
if (_zero_ != NULL) {
    if (_zero_->n_ptr) free(_zero_->n_ptr);
    free(_zero_);
}
if (_one_ != NULL) {
    if (_one_->n_ptr) free(_one_->n_ptr);
    free(_one_);
}
if (_two_ != NULL) {
    if (_two_->n_ptr) free(_two_->n_ptr);
    free(_two_);
}
```

**Option B: Reset reference counts to 1 and call bc_free_num()**
```c
if (_zero_ != NULL) {
    _zero_->n_refs = 1;
    bc_free_num(&_zero_);
}
/* Similar for _one_, _two_ */
```

Option A is safer (doesn't rely on bc_free_num() internals).

### Fix 5: Free _bc_Free_list Cache

Add this to bc_cleanup():

```c
bc_num node;
while (_bc_Free_list != NULL) {
    node = _bc_Free_list;
    _bc_Free_list = node->n_next;
    if (node->n_ptr) free(node->n_ptr);
    free(node);
}
```

**Problem:** _bc_Free_list is static in number.c, not accessible from main.c. Would need to add a cleanup function in number.c or export _bc_Free_list.

---

## Verdict

**PARTIAL PROGRESS — Still Cannot Ship**

**Fixes Applied:** 4 (getenv tracking, env_argv[0] free, POSIXLY_CORRECT, BC_LINE_LENGTH)

**Fixes Still Needed:** 5 (math strdups, name array entries, function bodies, global constants, free list cache)

**Severity of Remaining Leaks:** CRITICAL
- Math lib strdups: 12+ bytes/invocation (minor but easy to fix)
- Name array entries: 10-100+ bytes/invocation
- Function bodies: 50-200+ bytes/invocation
- Global constants: 120 bytes + free list cache (variable, could be KB)
- Total estimated leak: 150+ bytes per invocation, up to 1+ KB for complex programs

**Recommendation:** Complete the remaining 5 fixes before shipping. The fixes are localized to main.c (bc_cleanup function) and number.c (new cleanup for free list). None require changes to bc core logic.

**Estimated Effort:** 2-3 hours to apply and test all fixes.

---

## Testing Recommendation

After applying remaining fixes, run:
```
make clean && make build TARGET=ports/bc
make test TARGET=ports/bc
valgrind --leak-check=full ports/bc/bc -l <<< 'define f(x) { return x+1 }; for(i=1;i<=100;i++) f(i)'
```

to verify all allocations are freed.

