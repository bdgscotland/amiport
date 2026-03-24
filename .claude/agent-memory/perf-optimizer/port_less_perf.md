---
name: port_less_perf
description: Performance findings for ports/less 692 — follow-up review after first-round optimizations applied, ch.c/output.c/screen.c deep scan, new findings
type: project
---

# Performance findings: ports/less 692

Reviewed 2026-03-23. Follow-up pass after first-round optimizations were applied.

## Applied optimizations verified correct

1. **ansi_start() static pool** (line.c:689-701) — ecalloc replaced with single static struct.
   Zero malloc/free per ESC character. Correct: ansi_done() is always called before next ansi_start().

2. **is_ansi_end/middle lookup tables** (line.c:651-669, init at 176-185) — 256-byte tables built
   from LESSANSIENDCHARS/LESSANSIMIDCHARS strings during init_line(). O(1) array index replaces
   strchr(). Tables are rebuilt if env vars change (they are set once at startup).

3. **curr_last_ansi modulo** (line.c:1235-1237) — conditional increment replaces % NUM_LAST_ANSIS.
   NOTE: there is still a `% NUM_LAST_ANSIS` at line.c:916 in the resend_last path — this is a
   cold path (attribute restoration on hilite exit), not the display hot path. Correct to leave it.

4. **OUTBUF_SIZE = 4096** (defines.h:65) — was 1024. Confirmed. Reduces write() calls by ~4x for
   a typical 80-column screen (320 chars per screen worth of flush calls → ~80).

## Architecture notes (ch.c I/O path)

ch.c uses an 8192-byte block cache (LBUFSIZE) with a hash table (BUFHASH_SIZE=1024 slots) and
LRU eviction. The hot path (ch_get) has a fast-path check for head buffer hit first — this is
already well-optimized. No actionable improvements found in ch.c.

The file I/O goes through iread() → read() syscall at the LBUFSIZE granularity. AmigaOS
read() on a file is efficient at 8192 bytes. Nothing to change here.

## putchr hot path — NEW finding: double branch check per character

putchr() (output.c:457-509) is called for EVERY rendered character. On a 24-line × 80-col
screen = 1920 calls per screenful. Each call executes:
1. `if (!term_init_ever && outfd == 1)` — two variable loads + branch
2. `clear_bot_if_needed()` — function call + `if (!need_clr) return` check
3. `if (ob >= &obuf[sizeof(obuf)-1])` — pointer compare for flush

With OUTBUF_SIZE=4096, the flush trigger fires ~once per 5 screenfuls on an 80-col terminal.
So overhead is dominated by the init and clear_bot checks, not the flush.

**Optimization opportunity (MEDIUM):** The `term_init_ever` check can be converted from a
global variable branch to a function pointer swap — after term_init() runs, swap putchr to a
version without the check. However, this is a significant refactor (putchr is passed to tputs
as a callback). Simpler: since term_init() is called before the display loop in all normal
paths, the branch is effectively always-taken (term_init_ever==TRUE) during rendering. The
68020+ branch predictor will correctly predict this. Impact: LOW on 68020+, marginal on 68000.

## lgetenv — no leak concern

lgetenv() in decode.c:916 calls stdlib getenv() (not amiport_getenv). Returns pointer to
static env storage. No malloc — no leak. The many lgetenv calls in screen.c/filename.c are
all at startup or on user action, not in rendering loops.

## tab_spaces() modulo — NEW finding

tab_spaces() (line.c:1037-1052) uses `% tabdefault` on line 1043. tabdefault=8 (power of 2).
This is hit every time a TAB character is encountered in the file. At 7MHz 68000, DIVU =
120-158 cycles for this non-power-of-2 safe divide. Since tabdefault is runtime-configurable
via -x option, cannot unconditionally replace with `& 7`. Can guard:

```c
if (ntabstops < 2 || to_tab >= tabstops[ntabstops-1])
    to_tab = (tabdefault & (tabdefault-1)) == 0
        ? tabdefault - ((to_tab - tabstops[ntabstops-1]) & (tabdefault-1))
        : tabdefault - ((to_tab - tabstops[ntabstops-1]) % tabdefault);
```

But this adds a branch + check on the hot path. Better: track a `tabdefault_is_pow2` bool at
option-set time. Impact: LOW (only fires on TAB characters; most files have few tabs).

## at_switch() overhead — already minimal

at_switch() (screen.c:3127-3136) is called for every character via put_line(). It does:
1. apply_at_specials() — 2-3 bitwise ops
2. compare new vs old attrmode (bitmask compare)
3. Only calls at_exit()/at_enter() on attribute CHANGE — not on every character

This is already well-designed. The at_exit/at_enter calls that emit terminal sequences only
fire when attribute actually changes. No improvement needed.

## Summary of new actionable findings

- **LOW** [ARITH] line.c:1043 — tab_spaces() `% tabdefault` — replace with power-of-2 guard when
  tabdefault==8 (default). Only fires on TAB characters. Not worth applying unless profiler shows
  tab-heavy files as a use case.
- **INFO** output.c:467 — putchr `term_init_ever` check — benign on 68020+ (branch predicted).
  Not worth changing.

## What NOT to change

- ch.c block buffer and hash — already excellent
- at_switch() — attribute-change gate is already correct
- lgetenv calls — stdlib getenv, no AllocMem overhead
- % NUM_LAST_ANSIS at line.c:916 — cold path only

## Overall assessment

First-round optimizations were correctly applied. The port is in good shape for 68k.
Primary bottleneck for interactive use is console.device write() throughput (I/O bound),
not CPU. The 4096-byte output buffer and 8192-byte read buffer are the right levers.
No new CRITICAL or HIGH findings.
