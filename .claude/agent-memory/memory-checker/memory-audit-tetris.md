---
name: memory-audit-tetris
description: ports/tetris 1.35 memory safety review (2026-03-26)
type: reference
---

# Memory Safety Review: ports/tetris 1.35

## Summary

**Status: CRITICAL LEAK FOUND — 1 issue**
**Verdict: Cannot ship without 1-line fix**

Almost all dynamic allocations are properly paired with deallocations. The atexit cleanup pattern is correctly implemented and covers all error paths. However, one error path in savescore() (fseek failure) bypasses the fclose() call, leaking a file handle.

**Issues:** 1 critical (rare, easily fixed)
**Leaks found:** 1 (file handle on fseek error)

## Allocations Found

| Location | Type | Allocated | Freed | All Paths? | Issue |
|----------|------|-----------|-------|------------|-------|
| tetris.c:222 | amiport_expand_argv() | argv expansion | atexit cleanup:125 | **YES** — atexit(cleanup) registered immediately | CLEAN |
| tetris.c:231 | getenv("HOME") | libnix static pointer | N/A — no free needed | **N/A** — static storage, not malloc'd | CLEAN |
| scores.c:256, 258 | getenv("LOGNAME"/"USER") | libnix static pointers | N/A — no free needed | **N/A** — static storage, not malloc'd | CLEAN |
| scores.c:261 | amiport_getlogin() | returns static buffer | N/A — no free needed | **N/A** — static buffer in amiport/pwd.h | CLEAN |
| scores.c:131,143 | fopen(scorepath, "r+"\|"w+") | FILE* handle | fclose(sf):168,234 | **YES** — closed on all paths | CLEAN |
| screen.c:194 | tgetstr() — termcap strings | points into combuf static buffer | N/A — no free needed | **N/A** — static buffer, not malloc'd | CLEAN |
| (All others) | (No malloc/calloc/realloc) | Static data, no dynamic allocation | N/A | N/A | CLEAN |

## Detailed Findings

### 1. argv Expansion (tetris.c:222-224) — SAFE

```c
amiport_expand_argv(&argc, &argv);      /* line 222 */
atexit(cleanup);                        /* line 224 */
```

**Pattern: Correct atexit cleanup**
The expanded argv is immediately registered for cleanup via atexit. The cleanup function runs on:
- Normal exit (line 473: `return 0`)
- err()/errx() calls (lines 239, 262, 290, 316) — these terminate immediately but atexit handlers are called
- onintr() signal handler (line 487: `exit(0)` not `_exit(0)`)
- usage() early exit (line 496: `exit(10)`)

All paths covered. **NO LEAK.**

### 2. getenv("HOME") (tetris.c:231) — SAFE

```c
home = getenv("HOME");
```

**Key fact:** This uses libnix's native `getenv()`, NOT `amiport_getenv()`. The tetris.c file does NOT include `<amiport/stdlib.h>` macro which would redirect getenv. libnix getenv() returns a pointer to static internal storage. The caller must NOT free this pointer.

Comment at line 228-229 correctly documents this:
```c
/* amiport: getenv("HOME") uses libnix getenv() -- static pointer, no free */
```

**NO LEAK.**

### 3. File I/O in scores.c — SAFE

#### getscores() function (scores.c:110-169)

**Pattern: Open + error handling + guaranteed close**

```c
sf = fopen(scorepath, mstr);                /* line 131 or 143 */
if (sf == NULL) {
    if (fpp == NULL) {
        nscores = 0;
        return;                            /* early return — no sf to close */
    }
    sf = fopen(scorepath, "w+");           /* line 143 — retry with create */
    if (sf == NULL) {
        err(10, "cannot open %s for %s", scorepath, human);  /* terminates */
    }
}
/* ... use sf ... */
if (fpp)
    *fpp = sf;                             /* line 166 — return handle to caller */
else
    (void)fclose(sf);                      /* line 168 — close if read-only */
```

**Analysis:**
- Line 131: Opens in "r+" or "r" mode
- Line 136: Early return if read-only and file not found (no sf, no leak)
- Line 143: Opens in "w+" if file missing and read-write needed (create file)
- Line 146: err() call — terminates, no leak
- Line 168: Closes if caller didn't take ownership (fpp == NULL)
- Caller takesownership via `*fpp = sf` only if needed for writing

**NO LEAK.**

#### savescore() function (scores.c:172-235)

```c
getscores(&sf);                            /* line 180 — returns open FILE* */
/* ... modify scores array ... */
if (change) {
    if (fseek(sf, 0L, SEEK_SET) == -1) {
        err(10, "fseek");                  /* terminates — sf unclosed! */
    }
    if (fwrite(scores, sizeof(*sp), nscores, sf) != (size_t)nscores ||
        fflush(sf) == EOF) {
        warnx("error writing scorefile: %s\n\t-- %s",
            strerror(errno),
            "high scores may be damaged");  /* warning only — continues */
    }
}
(void)fclose(sf);                          /* line 234 — always closes */
```

**ISSUE FOUND: fseek error path**
Line 226: If `fseek()` fails, `err(10, "fseek")` is called which terminates immediately. The FILE* `sf` is never closed. On AmigaOS with `-noixemul`, the file handle is lost permanently.

**Severity: CRITICAL**
The fseek failure is rare (filesystem error, not a programming bug), but when it occurs, the score file handle leaks. This is a **rare but real leak**.

**Fix required:** Add fclose before err():
```c
if (fseek(sf, 0L, SEEK_SET) == -1) {
    fclose(sf);
    err(10, "fseek");
}
```

**Total leak impact:** One file handle per fseek failure (very rare). Permanent until reboot on AmigaOS.

### 4. Termcap Strings (screen.c:194) — SAFE

```c
static char combuf[1024], tbuf[1024];      /* line 125 — static buffers */
/* ... in scr_init() ... */
fill = combuf;                             /* line 189 */
for (p = tcstrings; p->tcaddr; p++)
    *p->tcaddr = tgetstr(p->tcname, &fill);  /* line 194 */
```

**Pattern: Static buffer, no malloc**
The console-shim's tgetstr() implementation fills a static buffer (combuf, 1KB) and updates a pointer into it. The returned strings are pointers INTO the static buffer, not malloc'd. No free needed.

**NO LEAK.**

### 5. Local/Static Variables — SAFE

All other allocations are static:
- `board[B_SIZE]` — static array (tetris.c:102)
- `scores[NUMSPOTS]` — static array (scores.c:90)
- `curscreen[B_SIZE]` — static array (screen.c:62)
- `combuf[1024]`, `tbuf[1024]` — static buffers (screen.c:125)
- Local variables on stack (smallscr, ws, newtt, oldtt, u, etc.)

All static data is freed automatically at program exit.

**NO LEAK.**

## Exit Path Verification

### All exit paths from main():

| Path | Line | Status |
|------|------|--------|
| Normal exit (game quit) | 473 | atexit(cleanup) runs ✓ |
| err(10) — level validation | 290 | atexit(cleanup) runs ✓ |
| errx(10) — duplicate keys | 316 | atexit(cleanup) runs ✓ |
| errx(10) — usage() | 496 | atexit(cleanup) runs ✓ |
| err(10) — getenv fail (rare) | 239 | atexit(cleanup) runs ✓ |
| errc(10) — path too long | 262 | atexit(cleanup) runs ✓ |
| showscores() early return | 298 | atexit(cleanup) runs ✓ |
| Ctrl-C via onintr() | 487 | exit(0) → atexit(cleanup) runs ✓ |
| Ctrl-C via amiport_check_break() | 357 | return 0 → atexit(cleanup) runs ✓ |

### Score file cleanup paths:

| Function | Exit | File closed? |
|----------|------|--------------|
| getscores() read-only, no file | 136 | N/A — never opened |
| getscores() read-only, file OK | 168 | Yes — fclose(sf) |
| getscores() read-write, success | 166 | No — caller takes ownership (correct) |
| getscores() open fails (err) | 146 | N/A — never opened |
| savescore() normal | 234 | Yes — fclose(sf) always |
| savescore() fseek error | 226 | **NO** — LEAK (see Section 3) |
| savescore() fwrite error | 230 | Yes — fclose(sf) still called |
| showscores() read-only | 168 (via getscores) | Yes — fclose(sf) |

## Critical Issue Summary

### Issue 1: Score file handle leak on fseek() error

**File:** scores.c, line 226
**Type:** File descriptor leak (rare)
**Severity:** CRITICAL (unfixable without code change)
**Impact:** One file handle (permanent until reboot) when fseek() fails
**Probability:** Very rare (filesystem error, not common gameplay)
**Fix:** Add `fclose(sf)` before `err(10, "fseek")`

## Warnings

None. All other patterns are safe.

## Final Verdict

**Status: CRITICAL LEAK FOUND — 1 issue must be fixed**

The fseek() error path in savescore() leaks a file handle. This is a rare but real leak that will permanently consume a file handle on AmigaOS when fseek() fails.

**Severity breakdown:**
- **Probability of occurrence:** Extremely rare (only on filesystem errors, file deletion, or corruption)
- **Impact if it occurs:** Permanent file handle leak (~16 bytes per leak) until reboot
- **AmigaOS constraint:** No automatic process cleanup with `-noixemul`, so manual fclose() is mandatory

**The fix:** Add one line before the err() call:
```c
if (fseek(sf, 0L, SEEK_SET) == -1) {
    fclose(sf);  /* REQUIRED: close before terminating */
    err(10, "fseek");
}
```

**Recommendation:** Apply the fix (1 line) before shipping. This is a defensive measure against rare but real file I/O errors. The fix costs nothing (single statement), and AmigaOS has no automatic cleanup.

## Confidence

**Very High** — All allocations traced to source (only libnix stdlib + static + atexit cleanup patterns). No hidden allocations in macros or included headers.
