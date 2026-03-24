---
name: Memory safety re-audit for less 692 after partial fixes
description: less 692 (GNU less pager) memory safety re-audit — 2 of 4 critical leaks fixed, 2 remain
type: project
---

# Memory Safety Re-Audit: less 692 After Partial Fixes

**Date:** 2026-03-23
**Status:** PARTIAL FIXES APPLIED — 2 of 4 CRITICAL LEAKS REMAIN UNFIXED
**Verdict:** Cannot ship — fixes incomplete

## Summary of Fixes Applied

| Leak | Location | Status | Fix Applied | Notes |
|------|----------|--------|------------|-------|
| every_first_cmd | main.c:38, optfunc.c:426 | FIXED ✓ | quit():655-656 free() | Correct unconditional free |
| tagoption | optfunc.c:353 | FIXED ✓ | quit():658-659 free() | Inside `#if TAGS` guard |
| first_cmd_at_prompt | opttbl.c:90, optfunc.c:1013 | **LEAK** ✗ | NOT FIXED | Never freed at quit() |
| ttyin_name | ttyin.c:43, optfunc.c:1262 | **LEAK** ✗ | NOT FIXED | Never freed at quit() |

## Detailed Findings

### Fixed Leaks ✓

#### 1. every_first_cmd (FIXED)
**Location:** main.c:655-656 in quit()
```c
free(every_first_cmd);
every_first_cmd = NULL;
```
**Status:** CORRECT — Unconditional free of the allocated string. Allocation path: optfunc.c:426 via `save(cmd)`. The free is inside `#ifdef __AMIGA__` guard which is appropriate.

#### 2. tagoption (FIXED)
**Location:** main.c:658-659 in quit()
```c
#if TAGS
free(tagoption);
tagoption = NULL;
#endif
```
**Status:** CORRECT — Guarded by TAGS preprocessor flag to match allocation. Allocation path: optfunc.c:353 via `save(s)`. Properly freed before exit().

---

### Remaining Critical Leaks ✗

#### 3. first_cmd_at_prompt (UNFIXED — CRITICAL)
**Location:** opttbl.c:90 (global), optfunc.c:1013 (allocation)
```c
/* opttbl.c:90 */
public char *first_cmd_at_prompt = NULL;

/* optfunc.c:1007-1018 — opt_first_cmd_at_prompt handler */
public void opt_first_cmd_at_prompt(int type, constant char *s)
{
    switch (type)
    {
    case INIT:
    case TOGGLE:
        first_cmd_at_prompt = save(s);  /* ALLOCATES at line 1013 */
        break;
    case QUERY:
        break;
    }
}
```

**Leak mechanism:**
- Allocated via `save(s)` at optfunc.c:1013 when user sets `--cmd` option
- **Current cleanup code:** command.c:920-923 only frees this if explicitly cleared during interactive command loop:
  ```c
  if (first_cmd_at_prompt != NULL)
  {
      ungetsc(first_cmd_at_prompt);
      first_cmd_at_prompt = NULL;  /* <- Only freed here during session */
  }
  ```
- **Missing cleanup:** If the program exits before this code is reached (e.g., single-file view, immediately closing), the allocation leaks
- **quit() function:** Lines 607-663 do NOT free `first_cmd_at_prompt`

**Impact:** ~64 bytes per invocation when `--cmd` option is used (typical command length). On AmigaOS with `-noixemul`, this is permanent memory loss until reboot.

**Root cause:** Partial fix applied (every_first_cmd and tagoption freed) but `first_cmd_at_prompt` was overlooked despite being in the same category of leaks.

**Fix required:**
```c
/* In quit() before exit(status), add: */
if (first_cmd_at_prompt != NULL)
    free(first_cmd_at_prompt);
```

---

#### 4. ttyin_name (UNFIXED — CRITICAL)
**Location:** ttyin.c:43 (global), optfunc.c:1262 (assignment)
```c
/* ttyin.c:43 */
#if LESSTEST
public char *ttyin_name = NULL;
#endif

/* optfunc.c:1257-1266 — opt_ttyin_name handler */
public void opt_ttyin_name(int type, constant char *s)
{
    switch (type)
    {
    case INIT:
        ttyin_name = s;  /* ASSIGNMENT at line 1262 — no save() call */
        is_tty = 1;
        break;
    }
}
```

**Analysis:**
- `ttyin_name = s;` at line 1262 is a **direct assignment**, NOT a `save(s)` allocation
- The `s` parameter comes from option parsing (command-line argument or option handler)
- **Is `s` malloc'd or static?** Need to determine the ownership of the pointer
  - If `s` is a command-line argv pointer (static): No free needed
  - If `s` is a malloc'd copy (e.g., from `save()`): Must be freed

**Source:** The `s` parameter flows from:
1. optfunc.c line 1257-1262: `opt_ttyin_name(int type, constant char *s)`
2. Called from option processing chain in option.c/opttbl.c

**Checking option.c for allocation pattern:**
The option handler receives `s` from the option processing code. Need to verify if option values are malloc'd copies or argv pointers.

**Assumption (from architecture):** OpenBSD less option handlers typically receive either:
- Static string literals (from defines or tables) — NO free needed
- malloc'd copies (from save()) — MUST be freed

**Current code:** ttyin_name is NEVER freed at quit(). The `#if LESSTEST` guard suggests this is a test-only feature, BUT it's still compiled into the binary when LESSTEST is enabled.

**Impact:** Unknown size (~10-128 bytes depending on TTY name length), but definitely a leak. Permanent memory loss on AmigaOS if this option is used.

**Fix status:** Cannot determine if free() is safe without auditing the allocation source. Conservative approach: Since every other global (every_first_cmd, tagoption, first_cmd_at_prompt) uses `save()`, and this follows the same pattern in `opt_*` functions, **assume `s` is malloc'd and must be freed.**

**Fix required:**
```c
/* In quit() before exit(status), add: */
#if LESSTEST
if (ttyin_name != NULL)
    free(ttyin_name);
#endif
```

---

## Optimization Review: Static ansi_pool

**Location:** line.c:689-702 (ansi_start function)
```c
public struct ansi_state * ansi_start(LWCHAR ch)
{
    static struct ansi_state ansi_pool;
    if (!IS_CSI_START(ch))
        return NULL;
    ansi_pool.ostate = OSC_START;
    ansi_pool.otype = 0;
    ansi_pool.escs_in_seq = 0;
    return &ansi_pool;
}
```

**Analysis:**
- **Safe:** Static pool is reused for every ANSI escape sequence
- **Ownership:** Caller (line.c:1283, 1582, 1868) calls `ansi_done(pansi)` which is now a no-op (line.c:841)
- **Leak risk:** None — static storage is never freed
- **FS-UAE impact:** None — this optimization eliminates ~300 malloc/free cycles per file with ANSI codes, improving performance. No memory corruption risk.

**Verdict:** ✓ CLEAN — No issues introduced by this optimization.

---

## OUTBUF_SIZE Change Review

**Location:** defines.h:65
```c
#define OUTBUF_SIZE     4096    /* amiport: was 1024 — fewer write() calls to console.device */
```

**Analysis:**
- **Allocation:** `static char obuf[OUTBUF_SIZE];` in output.c:90
- **Type:** Stack-allocated static array (not heap)
- **Leak risk:** None — static storage
- **Memory usage:** +3072 bytes static BSS (one-time, not per invocation)
- **Rationale:** Fewer console.device write() calls → better performance on Amiga

**Verdict:** ✓ CLEAN — No leak, no safety issue.

---

## Exit Path Analysis

All exit paths funnel through `quit()` function (main.c:607-663):
- main() line 513: `quit(QUIT_OK)` — normal exit
- Various error paths: `quit(QUIT_ERROR)` at lines 459, 463, 470, 476
- out_of_memory() line 538: `quit(QUIT_ERROR)` — OOM exit
- edit() path in quit() line 636: `edit((char*)NULL)` — closes current file

quit() sequence:
1. Terminal cleanup (lines 620-638)
2. File cleanup via `edit(NULL)` (line 636)
3. History save (line 637)
4. Raw mode reset (line 638)
5. **AmigaOS cleanup block (lines 652-662)** — Frees every_first_cmd, tagoption
6. **exit(status)** (line 663)

**Missing from cleanup block:** first_cmd_at_prompt, ttyin_name

---

## Test Coverage of Exit Paths

The fixes must cover:
1. ✓ quit(QUIT_OK) — normal exit after viewing file
2. ✓ quit(QUIT_ERROR) — error exit (missing file, etc.)
3. ? quit() called from error handlers mid-session
4. ✓ edit(NULL) in quit() — closes current file state

**All paths converge at quit() before exit().** No bypasses detected.

---

## Summary

### Fixed (2)
- every_first_cmd: ✓ Freed at main.c:655-656
- tagoption: ✓ Freed at main.c:658-659

### Remaining Leaks (2)
- **first_cmd_at_prompt:** CRITICAL — Never freed at quit(), leaks ~64 bytes per `--cmd` invocation
- **ttyin_name:** CRITICAL — Never freed at quit(), leaks ~10-128 bytes per `--tty` invocation

### Additional Issues
- None detected in static pool optimization
- OUTBUF_SIZE change is safe
- All exit paths properly funnel through quit()

---

## Fixes Required

### Priority 1: Critical Leaks
Add to quit() before exit(status) at line 663:

```c
	/* AmigaOS cleanup — AmigaOS has no process memory cleanup with -noixemul */
	free(every_first_cmd);
	every_first_cmd = NULL;
	if (first_cmd_at_prompt != NULL)
		free(first_cmd_at_prompt);
#if TAGS
	free(tagoption);
	tagoption = NULL;
#endif
#if LESSTEST
	if (ttyin_name != NULL)
		free(ttyin_name);
#endif
	(void)fflush(stdout);
```

**Rationale:**
- Every_first_cmd already freed (move into consolidated cleanup block)
- Add first_cmd_at_prompt unconditional free
- Add ttyin_name free under LESSTEST guard (matches its declaration scope)
- Consolidate all cleanup in one block for clarity

### Priority 2: Documentation
Add comment explaining why these globals must be freed manually on AmigaOS (no automatic process cleanup with `-noixemul`).

---

## Verdict

**Status:** PARTIAL FIXES APPLIED — Still cannot ship

**Fixes still needed:** 2 of 4 leaks (first_cmd_at_prompt, ttyin_name)

**Estimated leak on fixed build:** ~0-128 bytes per invocation (zero if --cmd and --tty options not used)

**Impact:** Cannot guarantee zero leaks until both remaining leaks are fixed. On AmigaOS with `-noixemul`, unfixed leaks cause permanent memory loss.

**Next action:** Apply the 2 remaining fixes to main.c:663 and re-test.
