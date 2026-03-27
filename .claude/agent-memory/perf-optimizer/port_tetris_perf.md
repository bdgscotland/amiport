---
name: port_tetris_perf
description: Performance findings for ports/tetris 1.35 — tputs per-char callback, tgoto snprintf per frame, CTOD multiply, fflush per frame reviewed 2026-03-26
type: project
---

# Tetris 1.35 Performance Findings

**Reviewed:** 2026-03-26
**Primary bottleneck:** Terminal output (tputs callback overhead + tgoto snprintf per cell move)
**Nature:** CPU-bound on 7MHz 68000; real-time game, every frame matters

## Hot Paths

1. `scr_update()` in screen.c — called every game tick (every fallrate period)
2. `tputs()` in lib/console-shim/src/term.c — called per escape sequence character
3. `tgoto()` in lib/console-shim/src/term.c — called per cursor move (snprintf per call)
4. `CTOD()` macro in tetris.h — multiply per display column translation

## HIGH Impact Findings

- `tputs()` callback loop: calls putc_func() (which calls putchar()) one character at a time for every escape sequence. Each moveto() = 1 tgoto() call + 1 tputs() call = ~9 putchar() calls for "\033[r;cH". Per-changed cell overhead: ~30-40 JSR calls. With ~20 changed cells per frame: ~600-800 function calls per frame BEFORE actual output.
  Fix: replace tputs() character-at-a-time loop with fputs(str, stdout) since pad characters are unused on Amiga.
- `tgoto()` calls snprintf() every single cursor move: snprintf("\033[%d;%dH", row+1, col+1). On 7MHz 68000, snprintf with format string parsing costs ~200-400 cycles. With ~20 moveto() calls per frame this is ~4000-8000 cycles per frame just in format parsing.
  Fix: replace tgoto() with a direct sprintf into a static 16-byte buffer, or better, avoid tgoto entirely and write the escape sequence directly in scr_update() via a fast itoa helper.
- `CTOD(i)` macro uses multiply: `(i) * 2 + (((Cols - 2 * B_COLS) >> 1) - 1)`. The `(Cols - 2*B_COLS)` term is constant per screen but recomputed 12 times per row. Fix: cache CTOD offset at scr_set() time.

## MEDIUM Impact Findings

- `fflush(stdout)` at end of every scr_update() call: necessary but expensive (~200 cycles for libnix stdio flush). Cannot be eliminated. Low priority.
- `printf("Score: %d", score)` in scr_update(): only called when score changes, acceptable.
- `scr_msg()` clears with putchar per character when CEstr is NULL: not hot path.

## LOW Impact / Already Good

- `fits_in()`: 4 array indexing operations, no function call overhead, clean 68k code.
- `place()`: 4 array writes, excellent.
- Board is 276 bytes (23*12) — fits in 68030 data cache entirely.
- `elide()`: called once per piece placement, not per frame.
- WaitForChar() timing: 20ms granularity (50Hz) is adequate for all game levels.
- `amiport_check_break()` in main loop: minor overhead, essential.

## Correctness Note

`tputs()` pad character path is dead on AmigaOS (PC=0, no hardware padding needed). The callback is `put()` which just calls `putchar()` — a single-function indirection with no real purpose on this platform. The entire tputs() mechanism can be short-circuited to fputs(str, stdout) for tetris's usage pattern.
