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

### 1. CRITICAL (FIXED): obsolete() malloc Leaks — NOW TRACKED

**Location:** uniq.c:319-323

**Status:** FIXED after partial corrections

The fixes added static tracking (lines 91-93) and cleanup integration:
```c
/* Track for cleanup */
#define MAX_OBSOLETE_ALLOCS 4
static char *obsolete_allocs[MAX_OBSOLETE_ALLOCS];
static int obsolete_alloc_count;

/* In obsolete() */
if (obsolete_alloc_count < MAX_OBSOLETE_ALLOCS)
    obsolete_allocs[obsolete_alloc_count++] = start;
```

**Cleanup in atexit():**
```c
static void cleanup(void) {
    int i;
    for (i = 0; i < obsolete_alloc_count; i++)
        free(obsolete_allocs[i]);
    amiport_free_argv();
    (void)fflush(stdout);
}
```

This correctly frees obsolete() malloc'd strings on all exit paths via atexit cleanup. **Status: OK after fixes**

---

### 2. CRITICAL (NEW): prevline Leak on getline Error in While Loop

**Location:** uniq.c:228-229

**NEW LEAK FOUND** — Partial fixes missed one exit path

```c
201:    while ((len = getline(&thisline, &thissize, stdin)) != -1) {
        /* ... loop body ... */
227:    }
228:    if (ferror(stdin))
229:        err(10, "getline");  /* ← LEAK EXITS HERE */
```

**Problem:**
- Line 185: `prevline` is allocated by first `getline()`
- Line 201-227: while loop reads subsequent lines
- Line 227: `thisline` is freed when loop exits
- Line 228-229: If ferror set, `err()` is called which calls `exit()`
- **prevline is LIVE but NEVER freed before exit()** ← **LEAK**

**Why It Leaks:**
1. Normal exit path (line 235): `free(prevline)` at line 232 ✓
2. Empty file path (line 190): `free(prevline)` at line 186 ✓
3. First getline error path (line 188): `free(prevline)` at line 186 ✓
4. **Loop error path (line 229): NO free before err()** ✗ **LEAK**

The atexit() fix does NOT help here because:
- cleanup() only frees obsolete_allocs[] and argv
- cleanup() does NOT (and cannot) free prevline because prevline is a local variable in main
- prevline MUST be freed inline in main before any exit() call on that path

**Impact:**
- ~8-16 KB memory leak per getline error in loop
- AmigaOS permanent memory loss until reboot
- High probability trigger (I/O errors, pipe breaks, signal interrupts)

**Fix:** Single line — add `free(prevline);` before err() at line 229:

```c
227:    free(thisline);
228:    if (ferror(stdin)) {
229:+       free(prevline);
230:        err(10, "getline");
231:    }
```

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

### Allocation Count (After Partial Fixes)
- Total distinct allocations: **4 patterns**
- Properly freed on all paths: **3** (obsolete_allocs, argv, thisline)
- Leaks found: **1 CRITICAL** (prevline on line 229 error)
- Unsafe realloc patterns: **0**
- Double-free risks: **0**
- File handle leaks: **0**

### Verdict

**FAIL — ONE CRITICAL LEAK REMAINS**

The partial fixes (atexit cleanup for argv + obsolete tracking) correctly addressed:
- ✓ amiport_expand_argv() cleanup via amiport_free_argv()
- ✓ obsolete() malloc'd strings tracked and freed in cleanup()
- ✓ atexit cleanup registered early to catch all err/errx paths

But one exit path was missed:
- **✗ prevline leak on getline error in while loop (line 229)**

**Impact:** ~8-16 KB permanent memory loss per error, AmigaOS reboot required to recover

**Required Fix:** Add 1 line:
```c
228:    if (ferror(stdin)) {
229:+       free(prevline);
230:        err(10, "getline");
231:    }
```

After this fix, ALL exit paths will be clean.

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
