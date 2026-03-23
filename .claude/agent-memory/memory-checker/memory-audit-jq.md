---
name: Memory audit — jq 1.7.1 port
description: ports/jq memory safety review — critical die() path leaks, realloc issue, strdup leak
type: project
---

# Memory Safety Review: jq 1.7.1

**Status: CRITICAL LEAKS FOUND**
**Verdict: Unfixable without code changes**

## Critical Issues

### Issue 1: die() Function Bypasses All Cleanup

**Severity: CRITICAL — Multiple paths**
**Frequency: On every argument error**

The `die()` function at line 156 of main.c:

```c
static void die() {
  fprintf(stderr, "Use %s --help for help with command-line options,\n", progname);
  fprintf(stderr, "or see the jq manpage, or online docs  at https://jqlang.github.io/jq\n");
  exit(20); /* amiport: RETURN_FAIL — was exit(2) */
}
```

This calls `exit(20)` without freeing any allocated state. It is called from 9 locations:

| Line | Trigger | Leaks |
|------|---------|-------|
| 420 | Invalid JSON passed to `--jsonargs` | ARGS (jv array), program_arguments (jv object), jq (jq_state), input_state, lib_search_paths (jv array) |
| 437 | `-L` option without path parameter | lib_search_paths (jv array), ARGS, program_arguments, jq, input_state |
| 515 | `--indent` option without parameter | ARGS, program_arguments, jq, input_state, lib_search_paths |
| 521 | `--indent` value out of range | ARGS, program_arguments, jq, input_state, lib_search_paths |
| 557 | `--arg` option without name/value | ARGS, program_arguments, jq, input_state, lib_search_paths |
| 567 | `--argjson` option without name/value | ARGS, program_arguments, jq, input_state, lib_search_paths |
| 573 | `--argjson` with invalid JSON | ARGS, program_arguments, jq, input_state, lib_search_paths |
| 590 | `--slurpfile` option invalid | ARGS, program_arguments, jq, input_state, lib_search_paths |
| 646 | No filter program specified | ARGS, program_arguments, jq, input_state, lib_search_paths |

**Total:** 9 die() calls, each leaks ~50-100 bytes minimum (the jv_array and jv_object allocations are malloc-backed).

### Issue 2: jv_mem_realloc() Unsafe Pattern

**Severity: HIGH — Pointer loss on failure**
**Location:** jv_alloc.c:180-186

```c
void* jv_mem_realloc(void* p, size_t sz) {
  p = realloc(p, sz);        /* <-- BAD: loses old pointer on failure */
  if (!p) {
    memory_exhausted();
  }
  return p;
}
```

If `realloc()` fails, the old pointer is lost and `memory_exhausted()` calls `abort()`. While the program terminates, this is poor error handling. The proper pattern:

```c
void* tmp = realloc(p, sz);
if (!tmp) memory_exhausted();  /* old p still valid */
p = tmp;
```

**Impact:** Used by linker.c, exec_stack.h, jv_parse.c. On malloc exhaustion (rare but possible with large JSON files or many library files), the pattern loses the pointer.

### Issue 3: strdup() Without Null Check

**Severity: CRITICAL — Unchecked allocation**
**Location:** linker.c:381

```c
lib_state->ct++;
lib_state->names = jv_mem_realloc(...);  /* wraps realloc(), calls abort() on failure */
lib_state->defs = jv_mem_realloc(...);   /* wraps realloc(), calls abort() on failure */
lib_state->names[state_idx] = strdup(jv_string_value(lib_path));  /* <-- NO NULL CHECK */
```

The `strdup()` at line 381 can fail and return NULL. If a library path string is large and memory is tight, `strdup()` failure goes undetected and the NULL pointer is stored in the array. Later dereferencing will crash.

**Fix required:** Check strdup() return value or use jv_mem_strdup().

### Issue 4: lib_search_paths jv Array Never Freed on Error

**Severity: CRITICAL — Path leak**
**Location:** main.c:409, 432, 434, 439

The `lib_search_paths` jv_array is initialized at line 409:
```c
jv lib_search_paths = jv_null();
```

Then populated via:
```c
lib_search_paths = jv_array_append(...);
```

But when `die()` is called at lines 420, 437, 515, 521, 557, 567, 573, 590, 646, this array is never freed. It's not in the `out:` cleanup section because cleanup only reaches there from non-die() paths.

**Size:** Each `-L` path adds a jv string allocation (20-100 bytes per path).

## Summary Table

| Allocation Type | Location | Count | Properly Freed? | Issue |
|---|---|---|---|---|
| ARGS (jv_array) | main.c:387 | 1 | **No** — lost on all die() calls | LEAK (9 paths) |
| program_arguments (jv_object) | main.c:388 | 1 | **No** — lost on all die() calls | LEAK (9 paths) |
| lib_search_paths (jv_array) | main.c:409 | 1 | **No** — lost on all die() calls | LEAK (9 paths) |
| jq (jq_state) | main.c:392 | 1 | **No** — lost on all die() calls | LEAK (9 paths) |
| input_state | main.c:402 | 1 | **No** — lost on all die() calls | LEAK (9 paths) |
| strdup() lib path | linker.c:381 | N (per lib) | **Yes** — freed via lib_state cleanup | **NO LEAK** (but unchecked) |
| exec_stack mem | exec_stack.h:70 | 1 | **Yes** — freed in stack_reset() | **NO LEAK** |
| jv values in ARGS | main.c:415,422 | N | **Yes** — freed via jv_free(ARGS) | **NO LEAK** (if exit path reached) |

## Root Cause

The `die()` function was designed for POSIX/Windows where `exit()` automatically cleans up process memory. **On AmigaOS with `-noixemul`, exit() does NOT clean up — all mallocs become permanent memory leaks until reboot.**

## Verdict

**Status: CRITICAL — Unfixable without code changes**

The jq 1.7.1 port has at least 9 code paths that leak allocations when argument parsing fails. On AmigaOS with `-noixemul`, each invocation that hits a die() path leaks ~50-150 bytes permanently.

**Cannot ship to Aminet in current state.**

## Fixes Required

1. **Create an atexit() cleanup handler** that frees ARGS, program_arguments, lib_search_paths, and calls jq_teardown(). Register it right after allocations at main.c:387-409.

2. **Replace die() with goto out** at all 9 call sites (lines 420, 437, 515, 521, 557, 567, 573, 590, 646) so cleanup is reached.

3. **Fix strdup() check** at linker.c:381:
   ```c
   const char *name_copy = jv_mem_strdup(jv_string_value(lib_path));
   if (name_copy == NULL) {  /* or wrap in jv_mem_strdup() */
       fprintf(stderr, "jq: memory allocation failed for library path\n");
       return 1;  /* error */
   }
   lib_state->names[state_idx] = name_copy;
   ```

4. **Fix jv_mem_realloc()** to use intermediate pointer (defensive):
   ```c
   void* jv_mem_realloc(void* p, size_t sz) {
     void* tmp = realloc(p, sz);
     if (!tmp) {
       memory_exhausted();  /* still has old p if needed */
     }
     return tmp;
   }
   ```

## Post-Fix Verification

After applying fixes:
- Verify all 9 die() paths now reach `out:` label
- Run `jq 2>&1 | grep error` with invalid arguments — should clean up
- Use `amiport_memory_check` or vamos with memory instrumentation to confirm no leaks
- Re-run memory-checker agent to confirm zero leaks
