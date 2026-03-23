---
name: Memory audit re-check — jq 1.7.1 port (post-fix)
description: ports/jq memory safety re-audit after fixes applied — identifies remaining leaks
type: project
---

# Memory Safety Re-Audit: jq 1.7.1 (Post-Fix)

**Status: CRITICAL LEAKS REMAINING — 2 unfixed paths**

**Verdict: CANNOT SHIP — fixes incomplete. Previously identified die() leaks were fixed, but 2 additional exit() paths remain unfixed.**

---

## Previous Fixes (Verified ✓ CORRECT)

### 1. die() Macro Replacement ✓

**Status: CORRECT** — All 9 die() call sites now use `goto out` instead of `exit()`:

- Line 425 (--jsonargs invalid JSON)
- Line 442 (-L without parameter)
- Line 520 (--indent without parameter)
- Line 526 (--indent out of range)
- Line 562 (--arg without name/value)
- Line 572 (--argjson without name/value)
- Line 578 (--argjson with invalid JSON)
- Line 595 (--slurpfile invalid)
- Line 651 (Unknown option)

**Macro definition at line 160-165:**
```c
#define die() do { \
  fprintf(stderr, "Use %s --help for help with command-line options,\n", progname); \
  fprintf(stderr, "or see the jq manpage, or online docs  at https://jqlang.github.io/jq\n"); \
  ret = JQ_ERROR_SYSTEM; \
  goto out; \
} while(0)
```

### 2. jv_mem_realloc() Fixed ✓

**Status: CORRECT** — Lines 180-190 in jv_alloc.c now use intermediate pointer:

```c
void* jv_mem_realloc(void* p, size_t sz) {
  /* amiport: use intermediate pointer to avoid losing original on realloc failure.
   * memory_exhausted() aborts, so the old pointer leaks permanently on AmigaOS. */
  void* tmp = realloc(p, sz);
  if (!tmp) {
    memory_exhausted();
    /* memory_exhausted calls abort(), but if it somehow returns: */
    return p;
  }
  return tmp;
}
```

### 3. strdup() Check Added (INCOMPLETE)

**Status: PARTIALLY CORRECT** — Line 381-385 in linker.c:

```c
lib_state->names[state_idx] = strdup(jv_string_value(lib_path));
if (!lib_state->names[state_idx]) {
  lib_state->names[state_idx] = ""; /* empty string fallback, prevents NULL deref */
}
```

**Issue:** The check uses an **empty string fallback** instead of proper error handling. This masks the OOM condition rather than returning an error. While it prevents a NULL pointer dereference, it corrupts the library path metadata silently.

**However**, line 370's strdup is **UNCHECKED** and unfixed:
```c
char *lib_origin = strdup(jv_string_value(lib_path));
```

This can fail and return NULL, which is then passed directly to `dirname()` at line 372.

### 4. lib_search_paths NOT freed in cleanup ✗ CRITICAL

**Status: LEAK REMAINS** — Line 414 allocates lib_search_paths as a jv_array:

```c
jv lib_search_paths = jv_null();
...
lib_search_paths = jv_array_append(lib_search_paths, ...);  /* lines 439, 444 */
```

But the cleanup section (lines 815-837) does NOT free it:

```c
out:
  badwrite = ferror(stdout);
  if (fclose(stdout)!=0 || badwrite) {
    fprintf(stderr,"jq: error: writing output failed: %s\n", strerror(errno));
    ret = JQ_ERROR_SYSTEM;
  }

  jv_free(ARGS);
  jv_free(program_arguments);
  jq_util_input_free(&input_state);
  jq_teardown(&jq);
  /* Missing: jv_free(lib_search_paths); */
```

**Leak size:** Each `-L` path adds ~20-100 bytes. On every error invocation (die() path), lib_search_paths leaks.

**Impact:** 9 die() paths now reach cleanup, but lib_search_paths is lost on all of them. Permanent leak of ~20-100 bytes per error invocation.

---

## NEW CRITICAL ISSUES FOUND

### Issue 1: exit(10) at Line 699 — Bypasses Cleanup

**Severity: CRITICAL — Early strdup failure**
**Location:** main.c:696-700

```c
char *origin = strdup(argv[0]);
if (origin == NULL) {
  fprintf(stderr, "jq: error: out of memory\n");
  exit(10); /* amiport: RETURN_ERROR — was exit(1) */
}
```

**Timeline:** This occurs AFTER allocations at lines 392-414:
- Line 392: ARGS (jv_array)
- Line 393: program_arguments (jv_object)
- Line 397: jq (jq_state)
- Line 407: input_state
- Line 414: lib_search_paths (jv_array)

If strdup() fails (rare but possible under memory pressure), ALL these allocations leak permanently.

**Impact:** On AmigaOS with `-noixemul`, ~100-200 bytes leak per malloc failure.

**Fix required:** Replace `exit(10)` with `goto out`.

---

### Issue 2: exit(20) at Line 720 — Bypasses Cleanup

**Severity: CRITICAL — Early strdup failure in FROM_FILE path**
**Location:** main.c:716-721

```c
if (options & FROM_FILE) {
  char *program_origin = strdup(program);
  if (program_origin == NULL) {
    perror("malloc");
    exit(20); /* amiport: RETURN_FAIL — was exit(2) */
  }
```

**Timeline:** Same as Issue 1 — occurs AFTER main allocations (lines 392-414 and all -L, -arg, etc. argument processing).

**Impact:** On malloc failure, leaks ARGS, program_arguments, lib_search_paths, jq, input_state — all accumulated allocations.

**Fix required:** Replace `exit(20)` with `goto out`.

---

### Issue 3: usage() Hard Exit at Line 714 — Bypasses Cleanup

**Severity: HIGH — Missing filter argument**
**Location:** main.c:714

```c
if (!program) usage(2, 1);
```

This calls `usage()` which unconditionally calls `exit()` at line 152:

```c
exit(ec);
```

**Timeline:** This occurs AFTER all main allocations (lines 392-414), so when user runs `jq` with no filter:
```
$ jq
(usage() exits with exit code 2, mapped to 20)
```

All allocations leak.

**Impact:** Every "--help" or missing-filter invocation leaks ~100-200 bytes. This is a common usage pattern.

**Fix required:** Replace `usage(2, 1)` with `ret = 2; goto out;` or modify usage() to not call exit() but return a code to caller.

---

## Summary of Remaining Leaks

| Issue | Location | Trigger | Leaks | Frequency | Severity |
|-------|----------|---------|-------|-----------|----------|
| 1. lib_search_paths not freed | main.c:414, cleanup 815 | Any die() path (9 paths) | jv_array (~20-100 bytes per -L) | On every error | **CRITICAL** |
| 2. exit(10) on strdup failure | main.c:699 | argv[0] strdup OOM | ARGS, program_arguments, lib_search_paths, jq, input_state | Rare (malloc failure) | **CRITICAL** |
| 3. exit(20) on strdup failure | main.c:720 | program_origin strdup OOM | ARGS, program_arguments, lib_search_paths, jq, input_state | Rare (malloc failure) | **CRITICAL** |
| 4. usage() hard exit | main.c:714 | User runs: `jq` (no args) or `jq --help` | ARGS, program_arguments, lib_search_paths, jq, input_state | **COMMON** | **CRITICAL** |
| 5. unchecked strdup at line 370 | linker.c:370 | Library import statement | NULL pointer passed to dirname() | Rare (malloc failure) | **HIGH** |

**Total allocations leaked:** ~150-400 bytes per error/help invocation. The usage() path (Issue 4) is especially problematic because it's a normal user interaction.

---

## Fixes Required

### Fix 1: Free lib_search_paths in cleanup

**File:** main.c, lines 815-837

Change:
```c
out:
  badwrite = ferror(stdout);
  if (fclose(stdout)!=0 || badwrite) {
    fprintf(stderr,"jq: error: writing output failed: %s\n", strerror(errno));
    ret = JQ_ERROR_SYSTEM;
  }

  jv_free(ARGS);
  jv_free(program_arguments);
  jq_util_input_free(&input_state);
  jq_teardown(&jq);
  /* ADD: */ jv_free(lib_search_paths);
```

### Fix 2: Replace exit(10) with goto out

**File:** main.c, lines 696-700

Change from:
```c
char *origin = strdup(argv[0]);
if (origin == NULL) {
  fprintf(stderr, "jq: error: out of memory\n");
  exit(10);
}
```

To:
```c
char *origin = strdup(argv[0]);
if (origin == NULL) {
  fprintf(stderr, "jq: error: out of memory\n");
  ret = JQ_ERROR_SYSTEM;
  goto out;
}
```

### Fix 3: Replace exit(20) with goto out

**File:** main.c, lines 716-721

Change from:
```c
if (options & FROM_FILE) {
  char *program_origin = strdup(program);
  if (program_origin == NULL) {
    perror("malloc");
    exit(20);
  }
```

To:
```c
if (options & FROM_FILE) {
  char *program_origin = strdup(program);
  if (program_origin == NULL) {
    perror("malloc");
    ret = JQ_ERROR_SYSTEM;
    goto out;
  }
```

### Fix 4: Replace usage() hard exit

**File:** main.c, line 714

Option A (simplest): Replace the hard exit with controlled flow

Change:
```c
if (!program) usage(2, 1);
```

To:
```c
if (!program) {
  usage(2, 1);  /* usage() will exit; this is early, before major allocations */
}
```

**OR** (better): Modify usage() to not exit, return a code:

```c
static void usage(int code, int keep_it_short) {
  FILE *f = stderr;
  ...
  (void)ret;  /* suppress unused warning */
  /* amiport: don't exit here; caller decides when to exit via goto out */
}
```

But this requires larger surgery. The simpler fix: acknowledge that this is an early exit before major allocations (lines 392-414 have NOT been reached yet when program==NULL at line 714 in early option parsing... wait, let me double-check).

**Wait — recheck flow:** Looking at line 714, it's INSIDE the main loop that starts at line 415. So it's AFTER allocations 392-414. So usage(2,1) DOES leak all those allocations.

**Correct fix:** Replace `usage(2, 1)` call with controlled flow:

```c
if (!program) {
  ret = JQ_ERROR_SYSTEM;
  goto out;
}
```

(The error message is printed by die() macro or error handlers elsewhere.)

### Fix 5: Check strdup at line 370

**File:** linker.c, line 370

Change from:
```c
char *lib_origin = strdup(jv_string_value(lib_path));
```

To:
```c
char *lib_origin = strdup(jv_string_value(lib_path));
if (!lib_origin) {
  fprintf(stderr, "jq: error: memory allocation failed for library path\n");
  return 1;  /* error code to caller */
}
```

---

## Verdict

**Status: CANNOT SHIP**

The previous die() fix was correct and eliminated 9 leak paths. However, **4 NEW critical paths** have been discovered that bypass cleanup:

1. lib_search_paths jv_array never freed (affects all die() paths)
2. exit(10) on strdup failure at line 699
3. exit(20) on strdup failure at line 720
4. usage() hard exit on missing filter (common user interaction)

These must be fixed before the port can be approved. The usage() path is especially critical because users frequently run `jq --help` or `jq` with no arguments, and each invocation will leak ~150+ bytes permanently.

---

## Post-Fix Verification Checklist

After applying all 5 fixes:

- [ ] Verify EVERY exit() call in main.c either:
  - Occurs before line 392 (before allocations), OR
  - Uses `goto out` to reach cleanup
- [ ] Verify cleanup at `out:` label frees:
  - [ ] ARGS
  - [ ] program_arguments
  - [ ] lib_search_paths
  - [ ] jq (via jq_teardown)
  - [ ] input_state (via jq_util_input_free)
- [ ] Run `jq` with no arguments — should return cleanly without leaks
- [ ] Run `jq --help` — should return cleanly without leaks
- [ ] Run `jq -L /nonexistent`.filter < /dev/null — should return cleanly without leaks
- [ ] Re-run memory-checker agent to verify zero leaks
