---
name: memory-audit-expand
description: ports/expand memory safety review (2026-03-25) — OpenBSD expand 1.15 ported to AmigaOS
type: reference
---

# Memory Safety Audit: expand (OpenBSD 1.15)

**Port:** `/Users/duncan/Developer/amiport/ports/expand/ported/expand.c`
**Date:** 2026-03-25
**Auditor:** memory-checker agent
**Verdict:** CLEAN — No memory leaks or unsafe patterns found

## Summary

- **Total allocations:** 1 (`amiport_expand_argv()`)
- **Properly freed on all paths:** 1/1 (100%)
- **Leaks found:** 0
- **Unsafe realloc:** 0
- **Double-free risks:** 0
- **File handle leaks:** 0
- **Static allocations:** 1 (tabstops array, safe)
- **Status:** ✅ APPROVED FOR SHIPPING

## Allocation Inventory

| Location | Type | Free'd? | All paths? | Issue |
|----------|------|---------|------------|-------|
| Line 78 | amiport_expand_argv() | Yes (atexit) | Yes | CLEAN — cleanup via atexit(cleanup) at line 80 |
| Lines 57-58 | static int tabstops[100] | N/A | N/A | CLEAN — static allocation, no free needed |
| Line 108 | freopen(argv[0], "r", stdin) | N/A | Yes | CLEAN — file handle management safe |

---

## Detailed Analysis

### 1. argv Expansion (Line 78)

```c
amiport_expand_argv(&argc, &argv);
atexit(cleanup);
```

**Status:** ✅ SAFE

**Allocations:**
- `amiport_expand_argv()` allocates expanded argv via glob expansion
  - Dynamically expanded filenames are malloc'd
  - Tracking array in amiport infrastructure

**Cleanup path:**
- Registered via `atexit(cleanup)` at line 80
- `cleanup()` function defined at lines 64-69:
  ```c
  static void
  cleanup(void)
  {
      amiport_free_argv();
      (void)fflush(stdout);
  }
  ```

**Cleanup guarantees:**
- `amiport_free_argv()` from `lib/posix-shim/src/argv_expand.c` handles ALL allocations
- Called on ALL exit paths:
  - `usage()` at line 199: calls `exit(10)` → triggers atexit cleanup
  - `getstops()` at line 178: calls `errx(10, ...)` → err() calls exit() → triggers atexit
  - `getstops()` at line 183: calls `errx(10, ...)` → triggers atexit
  - `main()` at line 109: calls `err(10, ...)` → triggers atexit
  - `main()` at line 163: calls `exit(0)` → triggers atexit

**No early returns in main() that bypass atexit.** All code paths reach either `usage()` (line 199), `getstops()` (line 178/183), explicit `err()` at line 109, or final `exit(0)` at line 163.

**Verdict:** ✅ SAFE — all allocations freed on all exit paths via atexit cleanup

---

### 2. Static Tab Stops Array (Lines 57-58)

```c
int	nstops;
int	tabstops[100];
```

**Status:** ✅ SAFE

**Analysis:**
- Static allocation — no malloc/free needed
- `tabstops[100]` is initialized to zero by default (static scope)
- `nstops` tracks the count (0..100)
- No growth, no realloc, fixed-size array
- Maximum capacity is 100 tab stops — enforced at line 182:
  ```c
  if (nstops >= sizeof(tabstops) / sizeof(tabstops[0]))
      errx(10, "Too many tab stops");
  ```

**Verdict:** ✅ SAFE — no dynamic memory involved

---

### 3. File Handle Management (Line 108)

```c
if (freopen(argv[0], "r", stdin) == NULL)
    err(10, "%s", argv[0]);
```

**Status:** ✅ SAFE

**Analysis:**
- `freopen()` reassigns stdin to a file
- No manual file handle allocation or deallocation
- Each iteration of the `do...while` loop (lines 106-162) reassigns stdin to the next file
- **stdin is NEVER explicitly closed** — this is correct (closing stdin would break the console handler)
- On program exit (line 163 `exit(0)`), AmigaOS automatically closes all open file handles

**File handle lifecycle:**
1. Loop iteration 1: `freopen(argv[0], "r", stdin)` — stdin → file1
2. Read from stdin (getchar loop, lines 113-161)
3. Loop back to line 106, check `argc > 0`
4. Loop iteration 2: `freopen(argv[1], "r", stdin)` — closes file1, stdin → file2
5. Repeat until argc == 0
6. Line 163: `exit(0)` — AmigaOS cleanup closes remaining handle
7. `cleanup()` via atexit executes
8. Process terminates

**Error path:**
- Line 109: `freopen()` fails → `err(10, "%s")` → exit(10) → atexit cleanup
- No dangling stdin assignment on error — the failed open() leaves stdin pointing to the previous file (or console) unchanged

**Verdict:** ✅ SAFE — file handles managed correctly without explicit close

---

### 4. Exit Code Safety (Lines 84, 109, 178, 183, 198)

```c
err(10, "pledge");          /* line 84 */
err(10, "%s", argv[0]);     /* line 109 */
errx(10, "Bad tab stop spec");      /* line 178 */
errx(10, "Too many tab stops");     /* line 183 */
exit(10);                   /* line 198 */
```

**Status:** ✅ SAFE

**Verification:**
- ALL `err()` and `errx()` calls use `exit(10)` (RETURN_ERROR), not `exit(1)`
- From `<amiport/err.h>` — err() macro calls `amiport_exit(code)` which is properly defined
- `amiport_exit()` is transparent to atexit cleanup — it calls libnix exit() which runs atexit handlers

**Final exit:**
- Line 163: `exit(0)` (success) — triggers atexit cleanup

**Verdict:** ✅ SAFE — all exit codes Amiga-correct, atexit always executes

---

### 5. No Dynamic Memory in Processing Loop (Lines 106-162)

```c
do {
    if (argc > 0) {
        if (freopen(argv[0], "r", stdin) == NULL)
            err(10, "%s", argv[0]);
        argc--, argv++;
    }
    column = 0;
    while ((c = getchar()) != EOF) {
        switch (c) {
        case '\t':
            /* ... putchar() calls only ... */
        /* ... other cases ... */
        }
    }
} while (argc > 0);
```

**Status:** ✅ SAFE

**Analysis:**
- All variables in the loop are stack-local (`c`, `column`, `n`)
- The `do...while` loop never allocates memory
- getchar()/putchar() use libnix stdin/stdout — no amiport-specific allocation
- Column tracking is a single int, no dynamic growth
- Tab stop calculations use the static `tabstops[100]` array

**Verdict:** ✅ SAFE — no memory allocations in inner loop

---

### 6. Function Scope Analysis

**getstops() function (lines 166-191):**
- Stack-local variables only (`i`)
- Parses command-line numeric string
- Writes to static `tabstops[]` array
- Calls `errx()` on error (triggers atexit cleanup + exit)
- **No allocations, no resource leaks**

**usage() function (lines 193-199):**
- Prints to stderr via fprintf()
- Calls `exit(10)` → triggers atexit cleanup
- **No allocations**

**main() function (lines 71-164):**
- Stack-local: `c`, `column`, `n` (lines 74-75)
- One dynamic allocation via `amiport_expand_argv()` at line 78
- Registered cleanup via `atexit(cleanup)` at line 80
- All exit paths (lines 84, 109, 163) → exit() → atexit cleanup
- **All allocations paired with cleanup**

**cleanup() function (lines 64-69):**
- Calls `amiport_free_argv()` to free expanded argv
- Calls `fflush(stdout)` to ensure output is written before exit
- **No loops, no conditionals, always runs on exit**

**Verdict:** ✅ SAFE — all functions have clear allocation ownership

---

## Crash Pattern Checks

**Pattern #5 (vsnprintf NULL destination):** Not applicable — expand doesn't use vsnprintf or asprintf. fprintf() is used for usage/error output.

**Pattern #7 (stack overflow from large locals):** Not applicable — max stack usage is ~50 bytes (c, column, n are ints, no arrays in locals).

**Pattern #9 (FS-UAE test hang):** Not applicable — no ARexx issues, program is single-threaded, no infinite loops that require Ctrl-C.

**Pattern #18 (dirname corruption):** Not applicable — expand doesn't use dirname().

**Pattern #19 (exclusive write lock):** Not applicable — expand reads files, doesn't write.

**Pattern #22 (amiport_getenv malloc'd strings):** Not applicable — expand doesn't use getenv().

---

## Pointer Ownership Verification (Section 7 — Critical)

**expand has exactly ONE dynamic allocation: amiport_expand_argv()**

**Ownership analysis:**
- **Allocated by:** `amiport_expand_argv()` at line 78
- **Owned by:** main() stack frame
- **Shared pointers?** NO
  - `argv` is only accessed in the main do...while loop (lines 106-162)
  - `argv` is never passed to other functions (getstops gets *cp pointer to static strings, usage gets no args)
  - Expanded argv entries are only used as filenames to freopen()
  - No data structures hold pointers to expanded argv entries
- **Uninitialized array entries?** NO
  - `amiport_expand_argv()` initializes ALL entries it creates
  - Glob expansion fills contiguous array with pointers to expanded filenames
- **Double-free risk?** NO
  - `amiport_free_argv()` called exactly once (atexit cleanup)
  - `did_expand` flag in argv_expand.c prevents double-free on multiple calls

**Severity:** SAFE — single-owner, fully initialized, no sharing

---

## Test Coverage for Memory Safety

The current test suite in `test-fsemu-cases.txt` covers:
- Simple tab expansion (default 8-column stops)
- Custom tab stops via `-t` flag
- Multiple files
- Empty files
- Error cases (bad tab stop spec, too many tab stops)
- Backspace handling
- No exit code on error paths → all trigger atexit cleanup

**Verdict:** ✅ Test coverage exercises all code paths that could leak memory

---

## Conclusion

**expand 1.15 port is CLEAN and safe to ship.**

| Criterion | Result |
|-----------|--------|
| All allocations freed? | ✅ YES |
| All exit paths covered? | ✅ YES |
| Unsafe realloc patterns? | ✅ NO |
| Double-free risks? | ✅ NO |
| File handle leaks? | ✅ NO |
| Pointer ownership clear? | ✅ YES |
| Test coverage adequate? | ✅ YES |

**No fixes required. Approved for publication to Aminet.**
