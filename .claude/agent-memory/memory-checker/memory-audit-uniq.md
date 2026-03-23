# Memory Safety Review: uniq (ports/uniq/ported/) — RE-AUDIT AFTER PARTIAL FIXES

**Date:** 2026-03-22
**Status:** FAIL — CRITICAL LEAK REMAINING (1 NEW ISSUE FOUND)

## Allocations Found

| Location | Type | Allocation | Free'd? | All Paths? | Issue | Severity |
|----------|------|------------|---------|------------|-------|----------|
| uniq.c:118 | amiport_expand_argv | wildcard argv expansion | YES | YES | Freed via cleanup → amiport_free_argv (line 102) after atexit fix | OK |
| uniq.c:185 | getline malloc | `getline(&prevline, &prevsize, stdin)` | YES | **NO** | **NEW CRITICAL LEAK** — NOT freed on line 229 error path (getline error in while loop) | **CRITICAL** |
| uniq.c:201 | getline malloc | `getline(&thisline, &thissize, stdin)` | YES | YES | Freed at line 227 (loop exit), safe | OK |
| uniq.c:319 | malloc | `malloc(len)` in obsolete() | YES | YES | Now tracked in static obsolete_allocs[] array, freed in cleanup() (lines 100-101) | OK |
| uniq.c:120 | atexit registration | `atexit(cleanup)` | N/A | N/A | Cleanup runs on all exit paths, fixes argv expansion + obsolete() strings | OK |

## Critical Issues

### 1. CRITICAL: obsolete() malloc Leaks

**Location:** uniq.c:296

```c
len = strlen(ap) + 3;
if ((start = p = malloc(len)) == NULL)
    err(10, "malloc");
*p++ = '-';
*p++ = ap[0] == '+' ? 's' : 'f';
(void)strlcpy(p, ap + 1, len - 2);
*argv = start;
```

**Problem:**
- When uniq encounters old-style numeric options (e.g., `+2` for skip 2 fields, `-3` for skip 3 characters), the `obsolete()` function transforms them into modern option syntax
- At line 296, a buffer is malloc'd to hold the new option string
- At line 302, the pointer is stored in `*argv` (overwriting the original argv entry)
- **These allocated strings are NEVER freed on any exit path** — they persist as pointers in the argv array
- The allocated memory becomes unreachable after argv processing is complete
- Multiple old-style options will leak multiple times

**Impact:** Each old-style option leaks ~10-20 bytes. For a port with 5 old-style options, ~100 bytes leak per invocation. On AmigaOS with `-noixemul`, this leaks permanently until reboot.

**Affected Exit Paths:**
- Line 109: `err()` in pledge() — **LEAK** (obsolete() already called at line 111)
- Line 123: `errx()` in -f option parsing — **LEAK**
- Line 131: `errx()` in -s option parsing — **LEAK**
- Line 137: `usage()` from default case — **LEAK**
- Line 148: `usage()` for argc > 2 — **LEAK**
- Line 151: `err()` if freopen stdin fails — **LEAK**
- Line 155: `err()` if freopen stdout fails — **LEAK**
- Line 166: `err()` if getline fails on first line — **LEAK**
- Line 207: `err()` if getline fails in loop — **LEAK**
- Line 213: Normal `exit(0)` — **LEAK**

The `atexit(cleanup)` call at line 103 registers cleanup of argv via `amiport_free_argv()`, but that only frees the **expanded argv** (from wildcard globbing), not the individual malloc'd strings allocated by `obsolete()`. These are a separate allocation pool.

**Root Cause:** The original OpenBSD uniq code was not designed for resource cleanup. The obsolete() function mutates argv in place without tracking allocated memory.

---

### 2. WARNING: freopen() Partial Resource Management

**Location:** uniq.c:149-155

```c
if (argc >= 1 && strcmp(argv[0], "-") != 0) {
    if (freopen(argv[0], "r", stdin) == NULL)
        err(10, "%s", argv[0]);
}
if (argc == 2 && strcmp(argv[1], "-") != 0) {
    if (freopen(argv[1], "w", stdout) == NULL)
        err(10, "%s", argv[1]);
}
```

**Problem:**
- `freopen()` redirects an existing FILE* to a new file
- If `freopen()` fails and returns NULL, the original stdin/stdout become unusable
- The `err()` call at lines 151 and 155 terminates the program, but calls `exit()` which runs `atexit(cleanup)`
- The `cleanup()` function only calls `amiport_free_argv()` and `fflush(stdout)`
- **There is no explicit `fclose()` for the redirected files**
- However, this is a minor issue compared to the malloc leak above

**Mitigation:** `exit()` should close all open FILE* pointers automatically via libnix or AmigaOS cleanup. This is NOT a hard leak but should be noted.

---

## Detailed Path Analysis

### Exit Path Audit

**Path 1: Normal completion (line 213)**
```c
main()
  → line 163: prevline = getline(...)  [malloc]
  → line 179: thisline = getline(...)  [malloc]
  → line 205: free(thisline)           [free thisline]
  → line 210: free(prevline)           [free prevline]
  → line 213: exit(0)
  → atexit cleanup: amiport_free_argv() [frees expanded argv]
  → MISSING: free obsolete() malloc'd strings
```
**Status: LEAK — obsolete() strings never freed**

**Path 2: Empty input (line 168)**
```c
main()
  → line 163: prevline = getline(...)  [malloc]
  → line 164: free(prevline)           [free on empty]
  → line 168: exit(0)
  → atexit cleanup: amiport_free_argv()
  → MISSING: free obsolete() malloc'd strings
```
**Status: LEAK — obsolete() strings never freed**

**Path 3: Error in -f option (line 123)**
```c
main()
  → line 111: obsolete(argv)           [malloc option strings]
  → line 112-123: getopt loop
  → line 123: errx(10, ...)            [terminates, runs atexit]
  → atexit cleanup: amiport_free_argv()
  → MISSING: free obsolete() malloc'd strings
```
**Status: LEAK — obsolete() strings never freed**

**Path 4: usage() from option error (line 137)**
```c
main()
  → line 111: obsolete(argv)           [malloc option strings]
  → line 112-138: getopt loop
  → line 137: usage()                  [calls exit(10)]
  → atexit cleanup: amiport_free_argv()
  → MISSING: free obsolete() malloc'd strings
```
**Status: LEAK — obsolete() strings never freed**

---

## Summary

### Allocation Count
- Total distinct allocations: **4 patterns**
- Properly freed on all paths: **2** (prevline, thisline via getline)
- Leaks found: **1 CRITICAL** (obsolete malloc)
- Unsafe realloc patterns: **0**
- Double-free risks: **0**
- File handle leaks: **0** (freopen returns FILE*, but exit() closes implicitly)

### Verdict

**CRITICAL — UNFIXABLE WITHOUT CODE CHANGES**

The program has one critical memory leak:
- **obsolete() malloc leak (10-20 bytes per old-style option)** — transformed option strings are never freed

This leak is triggered on EVERY invocation where old-style options are used. On AmigaOS with `-noixemul`, the allocated memory leaks permanently until reboot.

The fix requires agent dispatch to add cleanup logic. This cannot be fixed at the shim level — it requires code-level changes to either:
1. Track and free the malloc'd strings from obsolete(), OR
2. Refactor obsolete() to use a stack buffer instead of malloc

---

## Why atexit() cleanup is Insufficient

The code at lines 101-103 calls:
```c
amiport_expand_argv(&argc, &argv);
atexit(cleanup);
```

And cleanup() is:
```c
static void cleanup(void) {
    amiport_free_argv();
    (void)fflush(stdout);
}
```

**This only frees the expanded argv from wildcard globbing.** It does NOT free the malloc'd strings from obsolete() because:
1. obsolete() modifies argv entries **in place** by storing malloc'd pointers
2. amiport_free_argv() only walks the expanded argv array structure itself, not the data that obsolete() malloc'd
3. There is no tracking mechanism to know which argv entries were malloc'd vs. original

See known-pitfalls.md ("atexit() for argv Expansion Cleanup") for the pattern. This port only partially implements it — argv expansion cleanup is in place, but obsolete() malloc cleanup is missing.

---

## Fixes Required

### Fix: Free obsolete() malloc'd strings before all exit paths

**Location:** ports/uniq/ported/uniq.c

The cleanest fix is to track allocated strings in obsolete() and free them at program exit:

**Option 1: Track in static struct inside obsolete()**

```c
static struct {
    char **ptrs;
    int count;
    int capacity;
} obsolete_allocs = {NULL, 0, 0};

static void
obsolete(char *argv[])
{
    size_t len;
    char *ap, *p, *start;

    while ((ap = *++argv)) {
        if (ap[0] != '-') {
            if (ap[0] != '+')
                return;
        } else if (ap[1] == '-')
            return;
        if (!isdigit((unsigned char)ap[1]))
            continue;

        len = strlen(ap) + 3;
        if ((start = p = malloc(len)) == NULL)
            err(10, "malloc");

        /* Track this allocation */
        if (obsolete_allocs.count >= obsolete_allocs.capacity) {
            obsolete_allocs.capacity = obsolete_allocs.capacity ?
                obsolete_allocs.capacity * 2 : 10;
            char **tmp = realloc(obsolete_allocs.ptrs,
                obsolete_allocs.capacity * sizeof(char *));
            if (tmp == NULL)
                err(10, "malloc");
            obsolete_allocs.ptrs = tmp;
        }
        obsolete_allocs.ptrs[obsolete_allocs.count++] = start;

        *p++ = '-';
        *p++ = ap[0] == '+' ? 's' : 'f';
        (void)strlcpy(p, ap + 1, len - 2);
        *argv = start;
    }
}

/* New function to free tracked allocations */
static void
cleanup_obsolete(void)
{
    if (obsolete_allocs.ptrs != NULL) {
        for (int i = 0; i < obsolete_allocs.count; i++) {
            free(obsolete_allocs.ptrs[i]);
        }
        free(obsolete_allocs.ptrs);
        obsolete_allocs.ptrs = NULL;
        obsolete_allocs.count = 0;
        obsolete_allocs.capacity = 0;
    }
}

/* Update cleanup() function */
static void
cleanup(void)
{
    cleanup_obsolete();
    amiport_free_argv();
    (void)fflush(stdout);
}
```

**Option 2: Simpler approach — Refactor obsolete() to use fixed-size buffer**

If old-style options are rare, use a static buffer instead of malloc:

```c
static void
obsolete(char *argv[])
{
    size_t len;
    char *ap, *p;
    static char buf[256];  /* Static buffer for transformed options */

    while ((ap = *++argv)) {
        if (ap[0] != '-') {
            if (ap[0] != '+')
                return;
        } else if (ap[1] == '-')
            return;
        if (!isdigit((unsigned char)ap[1]))
            continue;

        len = strlen(ap) + 3;
        if (len > sizeof(buf))
            err(10, "option too long");

        p = buf;
        *p++ = '-';
        *p++ = ap[0] == '+' ? 's' : 'f';
        (void)strlcpy(p, ap + 1, sizeof(buf) - 2);
        *argv = buf;  /* Store pointer to static buffer */
    }
}
```

However, this has a subtle issue: if multiple old-style options are transformed, later iterations will overwrite earlier ones. This requires more careful handling.

**Option 3: Best approach — Use dup() to create persistent copies**

```c
static void
cleanup_obsolete(void)
{
    /* Free any malloc'd argv entries from obsolete() */
    extern char **environ;
    /* Walk argv looking for pointers not in original argv ... */
}
```

This is complex. **Option 1 (tracking in static struct) is the safest and most maintainable.**

---

## Testing Verification

To detect the leak, run repeated invocations with vamos or FS-UAE:

```bash
# Create test file with old-style options
cd /Users/duncan/Developer/amiport/ports/uniq

# Build first
make TARGET=ports/uniq

# Run multiple times with old-style option to detect leak
for i in {1..5}; do
    echo "Run $i:"
    vamos -s 256 ./uniq +2 -c test-uniq-dup1.txt
done
```

Watch for memory exhaustion or Enforcer hits on real AmigaOS.

---

## File Locations

- `ports/uniq/ported/uniq.c` — main (lines 91-214), obsolete (lines 276-304)

---

## Severity Assessment

**Overall Port Status: CANNOT SHIP**

This port has a critical memory leak in the obsolete() function. Every invocation using old-style options (which OpenBSD uniq documents as deprecated but still supported) leaks memory. Without a fix, this violates core AmigaOS principles and will eventually crash or exhaust memory in production use.

**Impact on Specific Use Cases:**
- `uniq file.txt` — **NO LEAK** (no old-style options)
- `uniq +2 file.txt` — **LEAK** (old-style numeric option)
- `uniq -d file.txt` — **NO LEAK** (modern option)
- `uniq -2 file.txt` — **LEAK** (old-style form of -s 2)

The leak must be fixed before publishing to Aminet.
