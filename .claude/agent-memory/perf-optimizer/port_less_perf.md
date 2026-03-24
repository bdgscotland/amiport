---
name: port_less_perf
description: Performance findings for ports/less 692 — all first-round fixes verified, ch.c/output.c/screen.c deep scan, no new HIGH findings reviewed 2026-03-23
type: project
---

# Performance findings: ports/less 692

Reviewed 2026-03-23. Final shipping gate audit — all 37 .c files scanned.

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

## putchr hot path — double branch check per character

putchr() (output.c:457-509) is called for EVERY rendered character. On a 24-line × 80-col
screen = 1920 calls per screenful. Each call executes:
1. `if (!term_init_ever && outfd == 1)` — two variable loads + branch
2. `clear_bot_if_needed()` — function call + `if (!need_clr) return` check
3. `if (ob >= &obuf[sizeof(obuf)-1])` — pointer compare for flush

With OUTBUF_SIZE=4096, the flush trigger fires ~once per 5 screenfuls on an 80-col terminal.
So overhead is dominated by the init and clear_bot checks, not the flush.

The `term_init_ever` check: since term_init() is called before the display loop in all normal
paths, the branch is effectively always-taken (term_init_ever==TRUE) during rendering. The
68020+ branch predictor will correctly predict this. Impact: LOW on 68020+, marginal on 68000.
Not worth changing.

## lgetenv — no leak concern

lgetenv() in decode.c:916 calls stdlib getenv() (not amiport_getenv). Returns pointer to
static env storage. No malloc — no leak. The many lgetenv calls in screen.c/filename.c are
all at startup or on user action, not in rendering loops.

## tab_spaces() modulo

tab_spaces() (line.c:1043) uses `% tabdefault`. tabdefault=8 (power of 2) by default.
This fires on TAB characters in the file. At 7MHz 68000, DIVU = 120-158 cycles.
Since tabdefault is runtime-configurable via -x option, cannot unconditionally replace with `& 7`.
Impact: LOW (only fires on TAB characters; most files have few tabs). Not worth applying.

## at_switch() overhead — already minimal

at_switch() (screen.c:3127-3136) is called for every character via put_line(). It does:
1. apply_at_specials() — 2-3 bitwise ops
2. compare new vs old attrmode (bitmask compare)
3. Only calls at_exit()/at_enter() on attribute CHANGE — not on every character

Already well-designed. No improvement needed.

## Final shipping gate audit — all 37 files

### fgetc usage
- tags.c:573 — fgetc in line-drain loop (when fgets reads truncated line). Cold path:
  only fires when ctags lines exceed 1024 bytes. Not actionable.
- No other fgetc/getchar in any file.

### strchr in hot paths
- regexp.c:731,754 — strchr in regex match scan. ANYOF/ANYBUT node scanning.
  Character class strings are typically 1-10 chars. Only in search mode, not display.
  Not actionable.
- regexp.c:861,866,1027,1033 — same ANYOF/ANYBUT character class matching.
- All other strchr calls (lesskey_parse.c, charset.c, filename.c, optfunc.c, decode.c) are
  in cold paths (startup, option parsing, user commands).
- line.c ANSI handling: already replaced with O(1) lookup tables (optimization #2 above).

### multiply/divide in rendering loops
- output.c:548 — `% radix` in TYPE_TO_A_FUNC (number→string). Called from iprint_int/
  iprint_linenum, only during prompt rendering (once per keypress). Not hot.
- screen.c:967,969 — `* 10` in terminal size detection. Startup only, once.
- screen.c:1057 — `/ 2` in wscroll calculation. Called on SIGWINCH only.
- No division or multiply in the character rendering loop (line.c/output.c putchr path).

### large local arrays not marked static
- lesskey_parse.c:149 — `char buf[1024]` in parse_error(). Error handler, startup only.
  Call depth ~6-7 frames, no dos.library calls in path. Stack budget 65536 — safe.
- lesskey_parse.c:669 — `char line[1024]` in parse_lesskey(). Startup only. Safe.
- lesskey_parse.c:727 — `char line[1024]` in parse_lesskey_content() while loop.
  Stack frame allocated once, reused per iteration. Startup only. Safe.
- screen.c:3575 — `char buf[2048]` in WIN32textout(). Dead code: MSDOS_COMPILER=0 in
  defines.h, entire function conditionally compiled out. Not present in Amiga binary.
- tags.c:487 — `static char buf[1024]` — already marked static (previous fix).

## Summary of new actionable findings from shipping gate audit

NONE. All remaining issues are LOW or INFO:

- **LOW** [ARITH] line.c:1043 — tab_spaces() `% tabdefault` — not worth applying without profiler data.
- **INFO** output.c:467 — putchr `term_init_ever` check — benign on 68020+ (branch predicted).
- **INFO** regexp.c ANYOF/ANYBUT — short character class strings, search-mode only.

## What NOT to change

- ch.c block buffer and hash — already excellent
- at_switch() — attribute-change gate is already correct
- lgetenv calls — stdlib getenv, no AllocMem overhead
- % NUM_LAST_ANSIS at line.c:916 — cold path only
- lesskey_parse.c large arrays — startup only, no dos.library call chain, stack safe

## Overall assessment

All four first-round optimizations are correctly applied and verified. The shipping gate
audit found no new CRITICAL or HIGH issues in any of the 37 .c files.

Primary bottleneck for interactive use is console.device write() throughput (I/O bound),
not CPU. The 4096-byte output buffer and 8192-byte read buffer are the right levers.

**VERDICT: SHIP**
