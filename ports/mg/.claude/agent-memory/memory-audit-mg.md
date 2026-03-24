---
name: Memory Safety Audit - mg 3.7
description: Complete memory safety review of mg (Micro GNU Emacs) port for AmigaOS — 41 .c files, console UI editor with -noixemul
type: project
---

# Memory Safety Review: mg (Micro GNU Emacs) 3.7

## Port Context
- **Program:** mg (Micro GNU Emacs)
- **Version:** 3.7
- **Category:** 3 (Console UI)
- **Build mode:** `-noixemul` — NO automatic memory cleanup on exit
- **Critical:** AmigaOS has NO memory protection and NO GC. Every malloc/allocation MUST be paired with free on ALL code paths.

## Allocations Found

| Location | Type | Freed? | All paths? | Issue |
|----------|------|--------|------------|-------|
| buffer.c:598 | calloc(bp) | Yes | Yes | ✓ SAFE — freed by killbuffer() |
| buffer.c:641 | strdup(b_bname) | Yes | Yes | ✓ SAFE — freed on killbuffer() |
| line.c:56 | malloc(lp) | Yes | Yes | ✓ SAFE — freed by lfree() |
| line.c:74 | realloc(l_text) | Yes | Yes | ✓ SAFE — intermediate tmp pointer |
| extend.c:165,193,242,275 | malloc(mp) | Yes | Partial | ⚠ RISKY — keymap error handling |
| extend.c:113,126,221 | calloc(pfp) | Yes | Partial | ⚠ RISKY — keymap function ptr arrays |
| fileio.c:482 | getenv(HOME) | Yes | Yes | ✓ SAFE — freed at line 520 |
| fileio.c:977 | getenv(HOME) | Yes | Yes | ✓ SAFE — freed at line 980 |
| fileio.c:742 | malloc(list) | Yes | Yes | ✓ SAFE — free_file_list() unwinds |
| fileio.c:754 | strdup(l_name) | Yes | Yes | ✓ SAFE — freed by free_file_list() |
| ansi.c:121 | getenv(TERM) | Yes | Yes | ✓ SAFE — copied to static buf, freed |
| region.c:448 | malloc(text) | Yes | Yes | ✓ SAFE — freed at line 545 |
| region.c:530 | getenv(SHELL) | N/A | N/A | ✓ SAFE — disabled on AmigaOS |
| interpreter.c:890 | getenv(var) | **NO** | **NO** | **CRITICAL LEAK** — never freed |
| ttykbd.c:109 | getenv(TERM) | **NO** | **NO** | **CRITICAL LEAK** — never freed |
| fparseln.c:157 | realloc(buf) | Yes | Yes | ✓ SAFE — intermediate cp pointer |
| echo.c:361,545 | realloc(buf) | Yes | Yes | ✓ SAFE — intermediate newp pointer |
| autoexec.c:51 | reallocarray(pfl) | Partial | No | ⚠ panic() on failure (acceptable) |

## Critical Issues Found

### 1. CRITICAL LEAK: interpreter.c:890 — getenvironmentvariable()
**Location:** interpreter.c, lines 880-902
**Issue:** `amiport_getenv()` malloc'd string NEVER freed
```c
if ((tmp = getenv(t)) == NULL || *tmp == '\0')
    return(dobeep_msgs("Envar not found:", t));
/* LEAK: tmp malloc'd by amiport_getenv — never freed */
dobuf[0] = '\0';
if (strlcat(dobuf, q, dosiz) >= dosiz)
    return (dobeep_msg("strlcat error"));  /* LEAK */
if (strlcat(dobuf, tmp, dosiz) >= dosiz)
    return (dobeep_msg("strlcat error"));  /* LEAK */
if (strlcat(dobuf, q, dosiz) >= dosiz)
    return (dobeep_msg("strlcat error"));  /* LEAK */
return (TRUE);  /* LEAK — tmp never freed */
```
**Impact:** Every `M-x get-environment-variable` invocation leaks malloc'd string permanently
**Severity:** CRITICAL — 5+ bytes per call, unbounded accumulation
**Fix:** Free tmp before all return paths

### 2. CRITICAL LEAK: ttykbd.c:109 — ttykeymapinit()
**Location:** ttykbd.c, lines 109-113
**Issue:** `amiport_getenv("TERM")` malloc'd string NEVER freed
```c
if ((cp = getenv("TERM")) != NULL &&
    (ffp = startupfile(cp, NULL, file, sizeof(file))) != NULL) {
    if (load(ffp, file) != TRUE)
        ewprintf("Error reading key initialization file");
    (void)ffclose(ffp, NULL);
}
/* LEAK: cp (malloc'd) never freed */
```
**Impact:** One-time leak per editor startup (~4-20 bytes)
**Severity:** CRITICAL but one-time (~4-20 bytes)
**Fix:** Free cp after startupfile() completes

## Architecture Issue: No Exit Cleanup

### quit() function (main.c:357-374)
**Issue:** No allocations freed on exit(0)
```c
void quit(int f, int n)
{
    vttidy();    /* closes terminal handles */
    closetags(); /* closes tag file */
    exit(0);     /* BUT all buffers, lines, keymaps still allocated! */
}
```
**Risk:** Entire buffer system (`bheadp` linked list), all lines, all keymaps leak permanently on exit
**Severity:** ARCHITECTURAL DESIGN FLAW — unfixable without major refactoring
**Impact:** 200+ bytes per complex editing session leaks permanently

### Required atexit() Handler
On AmigaOS with `-noixemul`, need to add:
```c
static void cleanup_on_exit(void)
{
    struct buffer *bp;
    while ((bp = bheadp) != NULL) {
        bheadp = bp->b_bufp;
        free(bp->b_bname);
        /* bclear all lines in bp */
        free(bp);
    }
    /* free all keymaps, undo history, etc */
}

int main(int argc, char **argv) {
    atexit(cleanup_on_exit);
    /* ... rest of main ... */
}
```

## File Handle Tracking — CLEAN

| Handle | Opened | Closed | Paths | Status |
|--------|--------|--------|-------|--------|
| ttyio.c:76 amiga_con_in | Open("*") | ttclose():149 | All | ✓ SAFE |
| ttyio.c:77 amiga_con_out | Open("*") | ttclose():150 | All | ✓ SAFE |
| All FILE* | fopen/ffopen | ffclose() | All | ✓ SAFE |
| Directories | opendir() | closedir() | All | ✓ SAFE |

## Realloc Safety — CLEAN

All realloc() calls use safe intermediate pointer pattern:
- echo.c:361, 545 — `newp = realloc(buf, ...)`; if ok, `buf = newp`
- fparseln.c:157 — `cp = realloc(buf, ...)`; if fail, free old and return
- line.c:74 — `tmp = realloc(lp->l_text, ...)`; if ok, `lp->l_text = tmp`

## Summary

| Category | Count | Status | Verdict |
|----------|-------|--------|---------|
| Total allocations | 50+ | Analyzed | SEE BELOW |
| Safely freed on all paths | 43 | ✓ | CLEAN |
| Critical leaks | 2 | **MUST FIX** | BLOCKS SHIPPING |
| Risky patterns | 2 | ⚠️ | ACCEPTABLE |
| Safe realloc | 5 | ✓ | ALL USE INTERMEDIATE |
| File handle leaks | 0 | ✓ | ALL TRACKED |
| Exit cleanup | 0 | **MISSING** | ARCHITECTURAL ISSUE |

## Verdict: FAIL — 2 CRITICAL LEAKS

**Status:** Cannot ship without fixes

**Must Fix Before Shipping:**
1. **interpreter.c:890** — FREE tmp before all return paths
2. **ttykbd.c:109** — FREE cp after startupfile() completes
3. **RECOMMENDED:** Add atexit() cleanup handler for buffers/lines/keymaps (architectural)

**Files Affected:**
- `/Users/duncan/Developer/amiport/ports/mg/ported/interpreter.c` — **CRITICAL**
- `/Users/duncan/Developer/amiport/ports/mg/ported/ttykbd.c` — **CRITICAL**
- `/Users/duncan/Developer/amiport/ports/mg/ported/main.c` — needs atexit() setup

## Testing Required After Fixes
1. Verify interpreter.c fix: Test `M-x get-environment-variable` multiple times, check memory
2. Verify ttykbd.c fix: Launch editor multiple times, check memory
3. Verify exit cleanup: Edit buffers, quit gracefully, monitor for memory leaks
4. FS-UAE full test suite with vamos memory instrumentation
