# Memory Safety Review: head.c (OpenBSD 1.24)

## Summary

**Verdict: CRITICAL LEAKS FOUND**

The head.c port contains 2 critical memory leaks related to argv expansion cleanup. On AmigaOS with `-noixemul` (no automatic process cleanup, no garbage collection), these leaks are **permanent** until reboot.

**Leaked resource:** Expanded argv array from `amiport_expand_argv()` (64+ bytes + N × strlen(path) per file matched)

**Affected scenarios:** Any invocation with wildcard arguments followed by an error condition (pledge failure, argument parse error, stdout write error)

**Cannot ship to Aminet without fixes.**

---

## Allocations Found

| Location | Type | Free'd? | All paths? | Issue |
|----------|------|---------|------------|-------|
| head.c:68 | `amiport_expand_argv()` malloc | Yes (107) | **NO** — leaks on err/errx at 71, 78, 88, 98, 134, 144 | **CRITICAL LEAK** |
| head.c:163 | `exit(10)` in `usage()` | Yes (107) | **NO** — usage() early-exits before cleanup | **CRITICAL LEAK** |
| argv_expand.c:115 | `realloc(expanded_argv)` | N/A | N/A | **SAFE** — intermediate pointer |
| argv_expand.c:139 | `realloc(expanded_strings)` | N/A | N/A | **SAFE** — intermediate pointer |

---

## Issue Details

### CRITICAL ISSUE #1: Expanded argv Leaked on Error Paths

**Location:** Lines 71, 78, 88, 98, 134, 144 in head.c

**Root cause:** The `err()` and `errx()` macros (from amiport/err.h) call `exit()`, which is aliased to `amiport_exit()` via the stdlib shim. This calls `_exit()` directly, **bypassing all cleanup code after the error call**.

The cleanup code at lines 107-110 is only reached on the normal (non-error) exit path:
```c
/* SAFE path — reached only on normal completion: */
amiport_free_argv();
fflush(stdout);
_exit(status);

/* UNSAFE paths — err()/errx() calls exit() directly: */
if (pledge("stdio rpath", NULL) == -1)
    err(10, "pledge");  /* Line 71 — exits immediately, skips cleanup */

/* Argument parse errors: */
if (errstr != NULL)
    errx(10, "count is %s: %s", ...);  /* Lines 78, 88 — exits immediately */

/* I/O errors in head_file(): */
if (ferror(stdout))
    err(10, "stdout");  /* Lines 134, 144 — exits immediately */
```

**Leak size:**
- `expanded_strings` array: 64 bytes (initial allocation, can grow to 128+)
- `expanded_argv` array: 128 × sizeof(char*) = 512-1024 bytes (initial allocation)
- String copies for each matched file: N × strlen(path)

Example: `head *.txt` with 100 matched files and an I/O error = ~2-5 KB leaked permanently.

**Why this is critical on AmigaOS:**
- No automatic process memory cleanup on exit with `-noixemul`
- No garbage collection or OS-level memory reclamation
- Leaked memory persists until the user reboots the machine
- Even a single invocation per day = 2-5 KB/day × N invocations = unbounded memory loss

---

### CRITICAL ISSUE #2: Expanded argv Leaked in usage()

**Location:** Lines 160-164 in head.c

```c
static void
usage(void)
{
    fputs("usage: head [-count | -n count] [file ...]\n", stderr);
    exit(10);  /* exits immediately, skips cleanup at line 107 */
}
```

**Trigger:** Invalid option after wildcard expansion. Example: `head *.txt -x`

The expansion happens at line 68, but `usage()` is called from the `getopt` switch at line 91, which is before the cleanup code.

**Leak:** Same as Issue #1 — all expanded argv memory is lost.

---

### ISSUE #3: File Handle Safety (Not a leak, but worth noting)

**Location:** Lines 126-130 in head_file()

```c
if (path != NULL) {
    name = path;
    fp = fopen(name, "r");
    if (fp == NULL) {
        warn("%s", name);
        return 10;  /* returns to main loop, no cleanup here */
    }
    /* ... rest of function ... */
}
```

**Status:** NOT A LEAK (file descriptor doesn't leak). The function returns a status code, main() continues processing other files. No memory is lost by the early return itself.

However, this function IS part of the overall exit-path problem: if all files fail to open and the program later hits an error (pledge failure, stdout write error), the expanded argv is still leaked.

---

## Safe Code Patterns Found

### realloc() Safety

Lines 115-116 and 139-142 in argv_expand.c use the correct intermediate pointer pattern:

```c
/* Line 115-116: SAFE */
char **tmp = (char **)realloc(expanded_argv, needed * sizeof(char *));
if (tmp == NULL) {
    amiport_globfree(&g);
    break;
}
expanded_argv = tmp;

/* Line 139-142: SAFE */
char **tmp = (char **)realloc(expanded_strings, (size_t)new_alloc * sizeof(char *));
if (tmp == NULL)
    break;
expanded_strings = tmp;
```

On realloc failure, the original pointer is preserved in `expanded_argv` / `expanded_strings`, and the function can continue or exit safely.

---

## Fix Recommendations

### RECOMMENDED: Fix at Shim Level

Add an **atexit hook mechanism** to `amiport/err.h` so that ALL ports using argv expansion + err/errx automatically clean up:

**Step 1:** In amiport/err.h, declare a global cleanup function pointer:
```c
extern void (*amiport_atexit_cleanup)(void);
```

**Step 2:** Update amiport_err() and amiport_errx() to call it:
```c
static void
amiport_err(int eval, const char *fmt, ...)
{
    va_list ap;
    if (fmt != NULL) {
        va_start(ap, fmt);
        (void)vfprintf(stderr, fmt, ap);
        va_end(ap);
        (void)fprintf(stderr, ": ");
    }
    (void)fprintf(stderr, "%s\n", strerror(errno));

    /* NEW: Call registered cleanup before exit */
    if (amiport_atexit_cleanup != NULL)
        amiport_atexit_cleanup();

    exit(eval);
}
```

**Step 3:** In head.c main(), register the cleanup:
```c
extern void (*amiport_atexit_cleanup)(void);

int
main(int argc, char *argv[])
{
    amiport_expand_argv(&argc, &argv);

    /* Register cleanup so err/errx calls it before exit */
    amiport_atexit_cleanup = amiport_free_argv;

    /* ... rest of main ... */
}
```

**Benefits:**
- Fixes head.c and automatically applies to ALL future ports using this pattern
- No code duplication
- Cleaner than per-port wrappers
- Follows the principle of fixing at the shim level (feedback_fix_root_cause_in_shim.md)

---

### ALTERNATIVE: Per-Port Cleanup Wrapper

If shim-level changes are deferred, add a cleanup wrapper in head.c:

```c
static void
cleanup_before_exit(void)
{
    amiport_free_argv();
    fflush(stdout);
}

/* Then wrap each early-exit point: */
if (pledge("stdio rpath", NULL) == -1) {
    cleanup_before_exit();
    err(10, "pledge");
}

if (errstr != NULL) {
    cleanup_before_exit();
    errx(10, "count is %s: %s", errstr, argv[1] + 1);
}
```

**Drawbacks:**
- Requires changes at every err/errx call (boilerplate)
- Same pattern needs to be added to tail.c, grep.c, and other ports with argv expansion
- Doesn't scale well

---

## Impact Assessment

### On vamos (No Memory Constraints)
- Leak is invisible — vamos doesn't track memory after process exit
- Leak will still be detected by memory-checker analysis

### On Real AmigaOS or FS-UAE
- Permanent memory loss until reboot
- User runs `head *.txt` on 100 files with a parse error = 2-5 KB lost
- User runs this 100× per day (e.g., in a build script) = 200-500 KB lost per day
- Over a week: 1.4-3.5 MB lost
- Over a month: 6-15 MB lost (this matters on Amiga systems with 8-32 MB RAM)

### Aminet Acceptance
- Leaking memory is a critical flaw on the target platform
- Aminet curators expect mature, polished software
- Cannot ship without fixing this

---

## Test Cases Needed

To verify the fix works:

```bash
# Test 1: Normal case (no leak)
head *.txt
# Expected: No memory loss, normal output

# Test 2: Parse error with wildcard (Issue #1)
head *.txt -x
# Expected: Usage error printed, NO argv leak

# Test 3: Pledge error (if pledge() is not stubbed)
head *.txt -n 5
# Expected: If pledge fails, NO argv leak

# Test 4: I/O error with wildcard (Issue #1)
head *.txt > /nonexistent/path
# Expected: I/O error printed, NO argv leak
```

---

## Checklist for Fix Verification

- [ ] Shim-level atexit hook added to amiport/err.h
- [ ] head.c registers cleanup in main()
- [ ] All err/errx calls in head_file() and usage() now clean up
- [ ] Recompile head and test with wildcard + error scenarios
- [ ] Run memory-checker agent again to verify leaks are gone
- [ ] Update CLAUDE.md to document the atexit hook pattern
- [ ] Apply the same fix to other ports using argv expansion (tail.c, grep.c, etc.)

---

## Files Affected

- `/Users/duncan/Developer/amiport/ports/head/ported/head.c` — Register cleanup in main()
- `/Users/duncan/Developer/amiport/lib/posix-shim/include/amiport/err.h` — Add atexit hook mechanism
- `/Users/duncan/Developer/amiport/lib/posix-shim/src/err.c` — Implement atexit hook calls
- Other ports: tail.c, grep.c, etc. (will need same fix when using argv expansion)

---

## References

- AmigaOS -noixemul: No automatic process cleanup on exit
- amiport/glob.h: argv expansion functions (amiport_expand_argv, amiport_free_argv)
- amiport/err.h: err/errx implementations
- Known-pitfalls.md #2: Exit path cleanup requirement
- Feedback: feedback_fix_root_cause_in_shim.md — Fix bugs at shim level, apply to all ports
