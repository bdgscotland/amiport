---
name: memory-audit-basename
description: ports/basename memory safety review (2026-03-25) — OpenBSD basename 1.14 ported to AmigaOS
type: reference
---

# Memory Safety Audit: basename (OpenBSD 1.14)

**Port:** `/Users/duncan/Developer/amiport/ports/basename/ported/basename.c`
**Date:** 2026-03-25
**Auditor:** memory-checker agent
**Verdict:** CLEAN — No memory leaks found

## Summary

- **Total allocations:** 1 (amiport_expand_argv)
- **Properly freed on all paths:** 1/1 (100%)
- **Leaks found:** 0
- **Unsafe realloc:** 0
- **Double-free risks:** 0
- **File handle leaks:** 0
- **Status:** APPROVED FOR SHIPPING

## Allocation Inventory

| Location | Type | Allocation | Free'd? | All Paths? | Issue |
|----------|------|------------|---------|------------|-------|
| Line 75 | amiport_expand_argv() | Expanded argv array + string copies | Yes (atexit) | Yes | CLEAN — atexit(cleanup) at line 77 |
| Line 57 | pledge() stub | `#define pledge(p, e) (0)` | N/A | N/A | CLEAN — macro returns 0 immediately, no allocation |

## Detailed Analysis

### 1. argv Expansion (Lines 75-77)

```c
amiport_expand_argv(&argc, &argv);
atexit(cleanup);
```

**Status:** ✓ SAFE

- **Allocation:** `amiport_expand_argv()` allocates:
  - `expanded_argv` — new argv array with expanded filenames
  - `expanded_strings` — tracking array of malloc'd string copies
  - Individual string copies via `malloc(strlen(g.gl_pathv[j]) + 1)`

- **Cleanup function** (lines 61-66):
  ```c
  static void cleanup(void)
  {
      amiport_free_argv();
      (void)fflush(stdout);
  }
  ```

- **Cleanup registration** (line 77):
  - `atexit(cleanup)` called immediately after expansion
  - Ensures cleanup runs on ALL exit paths

- **All exit paths covered:**
  1. **Line 80:** `err(10, "pledge")` — err() calls exit() → atexit cleanup runs
  2. **Line 86:** `usage()` at line 127 calls `exit(10)` → atexit cleanup runs
  3. **Line 97:** `return 0;` (empty string case) → atexit cleanup runs
  4. **Line 101:** `err(10, "%s", *argv)` — err() calls exit() → atexit cleanup runs
  5. **Line 121:** `return 0;` (normal exit) → atexit cleanup runs

- **No path skips atexit.** Even `err()`/`errx()` which terminate immediately still run atexit handlers before exiting per C89 standard.

**Verdict:** SAFE — All allocated memory freed via atexit on every exit path.

### 2. pledge() Stub (Line 57)

```c
#define pledge(p, e) (0)
```

**Status:** ✓ SAFE

- **Operation:** Macro that expands to `0` (integer constant)
- **No allocation:** This is purely syntactic substitution
- **Usage at line 79:** `if (pledge("stdio", NULL) == -1)` evaluates to `if (0 == -1)` which is false
- **Consequence:** The err() call on line 80 never executes
- **No memory impact**

**Verdict:** SAFE — No allocation or memory interaction.

## Exit Path Trace

### main() Return Paths

All paths from main() to exit verified:

```
main() entry
  ↓
Line 75: amiport_expand_argv(&argc, &argv)
  ↓
Line 77: atexit(cleanup)  ← Cleanup registered
  ↓
Line 79-80: pledge check (always passes, stub returns 0)
  ↓
Line 83-88: getopt loop (no allocations)
  ↓
Line 92-93: argc check → usage() exit(10) → atexit cleanup runs ✓
  ↓
Line 95-98: empty string check → return 0 → atexit cleanup runs ✓
  ↓
Line 99-101: basename() call → on failure err(10, ...) → atexit cleanup runs ✓
  ↓
Line 108-119: suffix stripping (no allocations, string manipulation only)
  ↓
Line 120: puts(p)
  ↓
Line 121: return 0 → atexit cleanup runs ✓
```

**Verdict:** All paths properly instrumented with atexit cleanup.

### usage() Exit Path (Lines 126-131)

```c
static void usage(void)
{
    (void)fprintf(stderr, "usage: %s string [suffix]\n", __progname);
    exit(10);
}
```

**Status:** ✓ SAFE

- Called from main at lines 86 and 93 when argument count wrong
- Prints error message using `__progname` (defined in shim library)
- Calls `exit(10)` → atexit cleanup runs before process termination
- No allocations in usage() itself

**Verdict:** SAFE — exit() ensures atexit cleanup runs.

## Data Flow Verification

### __progname Initialization

- **Declaration:** Line 53-54 comment indicates `__progname` is defined in libamiport.a (argv_expand.o) as a strong symbol
- **Initialization:** `amiport_expand_argv()` initializes __progname from argv[0] before checking __nowild (argv_expand.c:85-90)
- **Usage:** Line 129 in usage() — `fprintf(stderr, "usage: %s ...", __progname)`
- **Ownership:** __progname is owned by the shim library; the port just references it
- **Safety:** No allocation in the port; shim manages it

**Verdict:** SAFE — __progname properly initialized by shim.

### Local Variables

```c
int ch;          /* getopt() return value — no allocation */
char *p;         /* Points to result of basename(*argv) — no copy */
```

Both are stack-allocated and require no cleanup.

### basename() Return Value

- **Line 99:** `p = basename(*argv);`
- **Behavior:** basename() from `<libgen.h>` returns pointer into input buffer or to a static result buffer
- **Modification:** Lines 108-119 modify p[off] to null-terminate a suffix-stripped version
- **Output:** Line 120 calls `puts(p)` with the modified pointer
- **No allocation:** The input string comes from argv (already managed by amiport_expand_argv)
- **No new allocation:** This is pointer manipulation only

**Verdict:** SAFE — No new memory allocated; reusing argv strings managed by atexit cleanup.

## Potential Pitfall: dirname() Usage

The audit rules flag `dirname()` as a known pitfall (crashes its input buffer on AmigaOS libnix). **Basename does NOT call dirname()**, so this pitfall does not apply.

- `grep -n "dirname" basename.c` returns no matches ✓
- The program calls `basename()` (from libgen.h) only, not `dirname()`

**Verdict:** SAFE — dirname pitfall not applicable.

## Potential Pitfall: fclose(stdin)

The audit rules flag closing stdin as unsafe. **Basename does NOT close any file handles**, so this pitfall does not apply.

- No `fopen()` or `fclose()` calls ✓
- No `open()` or `close()` calls ✓
- No file I/O beyond reading argv ✓

**Verdict:** SAFE — No file handle pitfall applies.

## Potential Pitfall: getenv() Usage

The audit rules flag `getenv()` calls as potentially leaking if not freed properly. **Basename does NOT call getenv()**, so this pitfall does not apply.

- `grep -n "getenv" basename.c` returns no matches ✓

**Verdict:** SAFE — No getenv pitfall applies.

## String Safety

### Suffix Removal Logic (Lines 108-119)

```c
if (*++argv) {
    size_t suffixlen, stringlen, off;

    suffixlen = strlen(*argv);
    stringlen = strlen(p);

    if (suffixlen < stringlen) {
        off = stringlen - suffixlen;
        if (!strcmp(p + off, *argv))
            p[off] = '\0';
    }
}
```

**Status:** ✓ SAFE

- **Input:** `p` is result of `basename(*argv)`, `*argv` is suffix from argc/argv
- **Validation:** `suffixlen < stringlen` check prevents off-by-one
- **Bounds:** `p + off` is within p's allocated space (p points into basename's result)
- **Modification:** Sets `p[off] = '\0'` to truncate — safe string manipulation
- **No allocation:** Modifying existing string in-place

**Verdict:** SAFE — Bounds checked, no allocation.

## Conclusion

**CLEAN — No memory leaks detected.**

### Key Observations

1. **Single allocation point:** Only amiport_expand_argv() allocates memory
2. **Proper atexit registration:** cleanup() registered immediately after expansion
3. **No manual malloc/free:** Program contains no malloc(), calloc(), realloc(), or strdup() calls
4. **No file handles:** Program does not open or close files
5. **No getenv() calls:** Program does not use environment variables
6. **Simple control flow:** Direct argv processing, no recursive functions or complex error paths
7. **Pledge stub harmless:** No allocation, just condition checking

### Minor Notes

- **__progname:** Managed by shim library (argv_expand.o), not the port
- **basename():** Returns pointer into caller's buffer (no new allocation)
- **Dead code potential:** The `if (*++argv)` suffix logic could be dead code if argc == 1, but this is checked at line 92-93, so argc is always 1 or 2 at this point.

## Recommendation

**APPROVED FOR SHIPPING** — basename port is memory-safe on AmigaOS 3.x with `-noixemul`.

The atexit() callback ensures argv cleanup runs on all exit paths. No dynamic allocations within the port logic itself — all memory management delegated to libamiport.a via amiport_expand_argv() and atexit(cleanup).

The program is exceptionally simple and well-audited. No fixes required.
