---
name: memory-audit-bc
description: ports/bc 1.07.1 memory safety review — GNU bc ported to AmigaOS -noixemul
type: reference
---

# Memory Safety Audit: GNU bc 1.07.1

**Port:** `/Users/duncan/Developer/amiport/ports/bc/ported/`
**Date:** 2026-03-23
**Auditor:** memory-checker agent
**Verdict:** CRITICAL LEAKS FOUND — Multiple allocation errors prevent shipping

## Summary

- **Total allocations:** 10+ identified
- **Properly freed on all paths:** 3/10 (30%)
- **Leaks found:** 4 CRITICAL
- **Unsafe realloc patterns:** 0
- **Double-free risks:** 0
- **File handle leaks:** 0
- **Status:** CANNOT SHIP — Unfixable without code changes

## Allocation Inventory

| Location | Type | Allocation | Free'd? | All Paths? | Issue | Severity |
|----------|------|------------|---------|------------|-------|----------|
| main.c:193 | strdup() | `strdup("BC_ENV_ARGS")` | **NO** | **NO** | Never freed on any exit path | **CRITICAL** |
| main.c:305-310 | strdup() (x6) | Math lib function names "e", "l", "s", "a", "c", "j" | **NO** | **NO** | 6 leaked strdup() calls in open_new_file(), never freed | **CRITICAL** |
| main.c:189,217,220 | amiport_getenv() (x3) | Malloc'd env var strings | **NO** | **NO** | 3 malloc'd env strings never freed | **CRITICAL** |
| util.c:158 | bc_malloc() | file_node malloc in parse_args() | YES | YES | Freed in open_new_file() at line 328 | OK |
| storage.c:47 | strdup() | "(main)" function name | **NO** | **NO** | Allocated but never freed on exit | **CRITICAL** |
| util.c:73-142 | static allocation | arglist1/arglist2 strings | YES | YES | Freed/reused in call_str/arg_str | OK |
| util.c:234 | bc_malloc() | genstr buffer | YES | YES | Freed in set_genstr_size() | OK |
| storage.c:89-90,136-137,173-174 | bc_malloc() | function/var/array storage arrays | **UNKNOWN** | **UNKNOWN** | Allocated but no cleanup on exit — not freed | **CRITICAL** |
| load.c:68-71 | bc_malloc() | f_body reallocation | YES | YES | Freed when functions[].f_body is reallocated | OK |
| number.c:72,79 | malloc() | bc_num and bc_num->n_ptr | **UNKNOWN** | **UNKNOWN** | Reference-counted, freed via bc_free_num() — but _zero_, _one_, _two_ never freed | **CRITICAL** |

## Critical Issues

---

### 1. CRITICAL: strdup("BC_ENV_ARGS") at main.c:193

**Location:** main.c:189-210

**Status:** LEAK — Never freed on any exit path

```c
189:  env_value = amiport_getenv ("BC_ENV_ARGS");
190:  if (env_value != NULL)
191:    {
192:      env_argc = 1;
193:      env_argv[0] = strdup("BC_ENV_ARGS");  ← LEAK HERE
194:      while (*env_value != 0) {
195:        /* ... modify env_value and build env_argv ... */
...
210:      parse_args (env_argc, env_argv);
211:    }
```

**Analysis:**

- Line 193: `strdup("BC_ENV_ARGS")` allocates a 12-byte string and stores it in `env_argv[0]`
- The string is passed to `parse_args()` at line 210
- Inside `parse_args()`, the env_argv array is **not freed** — it's a local stack array used only during argument parsing
- No cleanup on any exit path:
  - Line 244: `bc_exit(10)` — **env_argv[0] leaked**
  - Line 117 (usage): `bc_exit(0)` — **env_argv[0] leaked**
  - Line 138 (show version): `bc_exit(0)` — **env_argv[0] leaked**
  - Line 275: `bc_exit(0)` at normal completion — **env_argv[0] leaked**

**Impact:** 12 bytes per invocation with BC_ENV_ARGS set. Permanent memory loss until reboot on AmigaOS.

**Root Cause:** `amiport_getenv()` returns dynamically allocated strings (see process.c:122), but bc does not track or free environment variable allocations.

---

### 2. CRITICAL: Math Library Function Name strdups at main.c:305-310

**Location:** main.c:297-316 in open_new_file()

**Status:** LEAK — 6 malloc'd strings never freed

```c
297:  if (use_math && first_file)
298:    {
299:      CONST char **mstr;
300:      /* Load the code from a precompiled version of the math libarary. */
301:
302:      /* These MUST be in the order of first mention of each function. */
305:      (void) lookup (strdup("e"), FUNCT);  ← LEAK
306:      (void) lookup (strdup("l"), FUNCT);  ← LEAK
307:      (void) lookup (strdup("s"), FUNCT);  ← LEAK
308:      (void) lookup (strdup("a"), FUNCT);  ← LEAK
309:      (void) lookup (strdup("c"), FUNCT);  ← LEAK
310:      (void) lookup (strdup("j"), FUNCT);  ← LEAK
311:      mstr = libmath;
312:      while (*mstr) {
313:           load_code (*mstr);
314:           mstr++;
315:      }
316:    }
```

**Analysis:**

Each `strdup()` call allocates a 2-byte string ("e", "l", "s", "a", "c", "j").

Inside `lookup()` at util.c:536-626:
```c
550:      id->id = strcopyof (name);  /* DOUBLE ALLOCATION: strdup result is copied again */
```

The `lookup()` function:
1. Takes the `strdup()` result as parameter `name`
2. Calls `strcopyof(name)` which does **another malloc and strcpy**
3. Stores the result in `id->id`
4. Lines 565, 584, 605: Only frees `name` if it's a duplicate lookup (already registered)
5. **If first registration: `name` (the strdup result) is NEVER freed**

**All 6 function names leak because:**
- They are first-time registrations (FUNCT case in lookup)
- Line 595: `f_names[id->f_name] = name;` — stores the leaked pointer directly
- No free() ever called on these strdups
- They remain allocated until bc exits

**Impact:** 6 × 2 bytes = 12 bytes per bc invocation with `-l` flag. But more significantly, this shows a pattern of unsafe strdup usage.

**Note:** The duplicate strings allocated by `strcopyof()` at line 550 are also problematic but remain in the f_names/v_names/a_names arrays which are never freed at exit either.

---

### 3. CRITICAL: amiport_getenv() Malloc'd Strings Never Freed

**Location:** main.c:189, 217, 220

**Status:** LEAK — 3 environment variable strings never freed

```c
189:  env_value = amiport_getenv ("BC_ENV_ARGS");  ← MALLOC
190:  if (env_value != NULL) { ... parse_args(env_argc, env_argv); }
211:  /* env_value NEVER freed */

217:  if (amiport_getenv ("POSIXLY_CORRECT") != NULL)  ← MALLOC (not stored)
     /* Result pointer lost — memory leak + leaks result value check */

220:  env_value = amiport_getenv ("BC_LINE_LENGTH");  ← MALLOC
221:  if (env_value != NULL) { line_size = atoi (env_value); }
228:  /* env_value NEVER freed */
```

**Analysis:**

`amiport_getenv()` (process.c:110-129) **returns dynamically allocated strings**:
```c
122:    result = (char *)malloc(len + 1);
123:    if (!result) {
124:        errno = ENOMEM;
125:        return NULL;
126:    }
127:    memcpy(result, tmp, len + 1);
128:    return result;
```

Three calls in main():

1. **Line 189:** `env_value = amiport_getenv ("BC_ENV_ARGS");`
   - Result stored in local `env_value`
   - Used at lines 194-204 to build env_argc/env_argv
   - **Never freed before bc_exit() on any path**
   - Leak size: strlen(BC_ENV_ARGS env var) bytes

2. **Line 217:** `if (amiport_getenv ("POSIXLY_CORRECT") != NULL)`
   - Result allocated, pointer **never stored**, address used only for NULL check
   - **Pointer lost immediately — double leak: (1) malloc never freed, (2) unused allocation**
   - Leak size: strlen(POSIXLY_CORRECT env var) bytes

3. **Line 220:** `env_value = amiport_getenv ("BC_LINE_LENGTH");`
   - Result stored in local `env_value`
   - Used at line 223 in `atoi(env_value)`
   - **Never freed before bc_exit() on any path**
   - Leak size: strlen(BC_LINE_LENGTH env var) bytes

**Impact:**
- 3 environment variable strings allocated per invocation
- Each variable can be up to 256 bytes (GetVar buffer)
- Permanent memory loss until reboot on AmigaOS

**Root Cause:** bc was ported assuming POSIX `getenv()` which returns pointer to static storage. But `amiport_getenv()` returns malloc'd strings (necessary because AmigaOS requires caller-provided buffers). This ABI mismatch was not caught during porting.

---

### 4. CRITICAL: Storage Arrays Never Freed at Exit

**Location:** storage.c:45-55, 73-119, 122-156, 159-194

**Status:** LEAK — Function/variable/array storage arrays allocated but never freed

```c
/* In init_storage() — called from main.c:231 */
45:  f_count = 0;
46:  more_functions ();      ← Allocates functions array
47:  f_names[0] = strdup("(main)");  ← Allocates "(main)" string

50:  v_count = 0;
51:  more_variables ();      ← Allocates variables array

54:  a_count = 0;
55:  more_arrays ();         ← Allocates arrays array
```

**From more_functions() (lines 73-119):**
```c
89:  functions = bc_malloc (f_count*sizeof (bc_function));
90:  f_names = bc_malloc (f_count*sizeof (char *));
```

**From more_variables() (lines 122-156):**
```c
136:  variables = bc_malloc (v_count*sizeof(bc_var *));
137:  v_names = bc_malloc (v_count*sizeof(char *));
```

**From more_arrays() (lines 159-194):**
```c
173:  arrays = bc_malloc (a_count*sizeof(bc_var_array *));
174:  a_names = bc_malloc (a_count*sizeof(char *));
```

Plus line 47: `f_names[0] = strdup("(main)");`

**Analysis:**

- These allocations are made in `init_storage()` at startup
- The arrays grow dynamically via `more_*()` functions which reallocate and free old pointers
- **But the final allocation is NEVER freed at program exit**
- bc_exit() at line 830 in util.c (and all other exit paths) do NOT free these global arrays
- No `atexit()` cleanup handler to deallocate

**Impact:**
- Initial allocation: ~3 × (32 × sizeof(void*)) ≈ 3 × 256 bytes = 768 bytes
- For large bc programs with many functions/variables/arrays: kilobytes per invocation
- Permanent memory loss until reboot on AmigaOS

**Why This Matters on AmigaOS:**
Unlike Unix where process exit automatically frees all memory, AmigaOS with `-noixemul` does NOT clean up task memory. Every allocation must be manually freed before exit.

---

### 5. CRITICAL: Global Constants _zero_, _one_, _two_ Never Freed

**Location:** number.c:54-57, 109-114

**Status:** LEAK — Reference-counted bc_num objects allocated at startup but never freed

```c
/* Global constants */
55:bc_num _zero_;
56:bc_num _one_;
57:bc_num _two_;

/* In bc_init_numbers() called from init_storage() */
109:  _zero_ = bc_new_num (1,0);
110:  _one_  = bc_new_num (1,0);
111:  _one_->n_value[0] = 1;
112:  _two_  = bc_new_num (1,0);
113:  _two_->n_value[0] = 2;
```

**Analysis:**

Each `bc_new_num()` call allocates:
1. `struct bc_struct` (the bc_num header) — malloc
2. `bc_num->n_ptr` data buffer — malloc

From number.c:64-84:
```c
68:    if (_bc_Free_list != NULL) {
69:      temp = _bc_Free_list;
70:      _bc_Free_list = temp->n_next;
71:    } else {
72:      temp = (bc_num) malloc (sizeof(bc_struct));
73:      if (temp == NULL) bc_out_of_memory ();
74:    }
...
79:  temp->n_ptr = (char *) malloc (length+scale);
80:  if (temp->n_ptr == NULL) bc_out_of_memory();
```

The reference-counting system:
- `bc_copy_num()`: increments `n_refs`
- `bc_free_num()`: decrements `n_refs`, only frees when `n_refs == 0`
- Freed nodes go to `_bc_Free_list` for reuse

**The leak:**
- _zero_, _one_, _two_ start with `n_refs = 1`
- They are copied many times in the program (`n_refs` increases)
- When bc exits, these globals are NEVER freed
- All copies still have `n_refs > 0`, so `bc_free_num()` never deallocates
- The nodes remain in the free list, their data buffers remain allocated

**Impact:**
- _zero_: 1 (bc_struct) + 1 (data) = ~40 bytes
- _one_: 1 (bc_struct) + 1 (data) = ~40 bytes
- _two_: 1 (bc_struct) + 1 (data) = ~40 bytes
- Total: ~120 bytes permanent per invocation

Plus all the bc_struct nodes allocated during execution that remain in `_bc_Free_list` at exit.

---

## Exit Path Coverage

### Main Exit Paths

1. **Line 117 (usage after -h):** `bc_exit(0)`
   - ALL 4 critical leaks active
   - **Leaked:** env_argv[0], BC_ENV_ARGS string, POSIXLY_CORRECT, BC_LINE_LENGTH, math lib strdups, storage arrays, constants

2. **Line 138 (usage after -v):** `bc_exit(0)`
   - ALL 4 critical leaks active
   - **Leaked:** Same as above

3. **Line 147 (usage after bad option):** `bc_exit(10)`
   - ALL 4 critical leaks active
   - **Leaked:** Same as above

4. **Line 244 (open_new_file fails):** `bc_exit(10)`
   - ALL 4 critical leaks active
   - **Leaked:** Same as above (includes 6 math lib strdups)

5. **Line 275 (normal exit after yyparse):** `bc_exit(0)`
   - ALL 4 critical leaks active
   - **Leaked:** Same as above

6. **Line 316 (load_code "program too big"):** `bc_exit(1)`
   - ALL 4 critical leaks active
   - **Leaked:** Same as above

No path frees the critical allocations.

---

## Summary of Leaks by Severity

### CRITICAL (4 issues, unfixable without significant changes)

1. **strdup("BC_ENV_ARGS")** — 12 bytes per invocation
2. **6 × strdup() math function names** — 12 bytes per math invocation
3. **3 × amiport_getenv() results** — up to 768 bytes per invocation
4. **Global storage arrays** — 768+ bytes per invocation
5. **Global constants _zero_, _one_, _two_** — 120+ bytes per invocation

**Total permanent memory loss per invocation:** ~1.6 KB + env var size

### ON AMIGAOS THIS IS CATASTROPHIC

- Each bc invocation loses ~1.6 KB permanently
- Running bc 100 times exhausts 160 KB of RAM
- Running bc 1000 times exhausts 1.6 MB of RAM
- On Amiga with 512 KB to 2 MB RAM, this quickly becomes unusable
- **No automatic cleanup — requires reboot to reclaim**

---

## Why These Leaks Exist

1. **bc written for Unix** where process exit auto-frees
2. **amiport_getenv() ABI mismatch** — differs from POSIX getenv()
   - POSIX: returns pointer to static storage (caller must not free)
   - amiport: returns malloc'd string (caller MUST free)
   - This was not documented in porting notes, causing the leak pattern
3. **Global allocations never freed** — common in Unix but fatal on AmigaOS
4. **No atexit() cleanup** — AmigaOS requires explicit cleanup

---

## Fixes Required

### Fix 1: Track and free amiport_getenv() Results

```c
/* In main.c */
static char *env_strings[3];
static int env_string_count = 0;

void cleanup(void) {
    int i;
    for (i = 0; i < env_string_count; i++)
        free(env_strings[i]);
    (void)fflush(stdout);
}

int main(int argc, char **argv) {
    char *env_value;
    env_value = amiport_getenv("BC_ENV_ARGS");
    if (env_value != NULL) {
        env_strings[env_string_count++] = env_value;  /* TRACK */
        /* ... use env_value ... */
    }
    /* Similar for POSIXLY_CORRECT and BC_LINE_LENGTH */
    atexit(cleanup);
    /* ... rest of main ... */
}
```

### Fix 2: Free strdup("BC_ENV_ARGS")

```c
/* In parse_args after use */
if (env_value != NULL) {
    /* ... parse_args ... */
    free(env_argv[0]);  /* ADD THIS */
}
```

### Fix 3: Use strcopyof() Instead of strdup() for Math Lib Names

OR track strdups in an array like the grep/tail/uniq ports.

### Fix 4: Add Global Cleanup

```c
void cleanup_globals(void) {
    /* Free storage arrays */
    if (functions) free(functions);
    if (f_names) {
        if (f_names[0]) free(f_names[0]);  /* "(main)" */
        free(f_names);
    }
    if (variables) free(variables);
    if (v_names) free(v_names);
    if (arrays) free(arrays);
    if (a_names) free(a_names);

    /* Free constants */
    if (_zero_) bc_free_num(&_zero_);
    if (_one_) bc_free_num(&_one_);
    if (_two_) bc_free_num(&_two_);
}
```

Then call from atexit cleanup.

---

## Verdict

**CANNOT SHIP without fixes.** This port has 4 separate critical memory leak categories affecting every invocation. On AmigaOS with `-noixemul`, this makes bc unusable for any non-trivial usage (memory exhaustion after dozens of runs).

**Estimated Fix Effort:** Medium — requires:
1. Add atexit cleanup pattern (like grep/tail)
2. Track 3 getenv results
3. Free math lib strdups
4. Free global storage arrays

All fixes are localized to main.c and util.c's bc_exit() function. No changes needed to bc core logic.

