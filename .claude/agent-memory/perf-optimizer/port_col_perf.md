---
name: port_col_perf
description: Performance findings for ports/col 1.20 — PUTC macro per-character output, getchar() main loop, flush_line output pattern reviewed 2026-03-26
type: project
---

# col 1.20 — Performance Findings

Reviewed 2026-03-26. Source: ports/col/ported/col.c (single file).

## Hotpath

main() loop at col.c:180 — reads every byte of stdin via getchar().
flush_line() at col.c:405 — writes every output byte via PUTC(putchar()).

## Issues

### HIGH — col.c:180 — getchar() per-byte input loop
Main loop calls getchar() once per input byte. On 7MHz 68000 each getchar() is a libnix
function call through the stdio buffer machinery — ~10-15 cycles overhead beyond the
actual buffer check. For a 100KB document this is 100,000+ function call dispatches.
Replacement: read blocks with fread() into a static buffer then iterate over the buffer.
However: col is architecturally structured around character-at-a-time state machine
(backspace, ESC sequences, CR, SI/SO, VT all need per-character processing with state).
A getchar()-to-fread() refactor would require restructuring the escape sequence handling
(col.c:197-215 does getchar() inside the ESC case to read the follow byte).
The fix IS possible but invasive. Estimate: 2-3x throughput improvement on large inputs.

### HIGH — col.c:108-110, throughout flush_line() — PUTC macro calls putchar() per byte
The PUTC macro expands to putchar() with EOF check. flush_line() calls PUTC tens to
hundreds of times per line: for space padding (col.c:475-476), tab emission (col.c:472),
character output (col.c:492, 495). flush_blanks() calls PUTC in a loop (col.c:385-386).
Each putchar() is a libnix function call. Collecting output into a static buffer and
doing a single fwrite() per flush_line() call would eliminate hundreds of function calls
per line. Estimate: 2-4x speedup in output-heavy scenarios.

### MEDIUM — col.c:413-415 — static locals in flush_line sorted/count shared state
`static CHAR *sorted` and `static int *count` grow via xreallocarray() as needed.
This is actually a good pattern — avoids repeated malloc for sort buffers.
The `static size_t i` is unusual (size_t as static loop var) but functionally correct.
No action needed.

### MEDIUM — col.c:466-470 — tab compression uses modulo (%)
`ntabs = ((last_col % 8) + nspace) / 8` — the `% 8` is a modulo by non-power-of-2...
wait, 8 IS a power of 2. `last_col % 8` -> `last_col & 7`. Small saving per PUTC('\t')
emission. Fix: `ntabs = ((last_col & 7) + nspace) >> 3`. Also `nspace -= (ntabs * 8) - (last_col & 7)`.
On 68000: % costs DIVU (44 cycles), >> costs 1-8 cycles. Worth fixing.

### LOW — col.c:304 — xreallocarray doubles capacity from 45 initially
Growth factor of 2x from initial 45 is fine. Minor: initial size of 45 means first
allocation covers ~45 chars, second covers 90, etc. Not a hot path.

## Summary
- Primary bottleneck: character-at-a-time I/O (getchar + putchar per byte)
- The PUTC-to-buffer refactor is HIGH impact but requires careful restructuring
- The getchar refactor is HIGH impact but architecturally invasive
- The % -> & fix for tab compression is easy and MEDIUM impact
- Overall: I/O bound, two HIGH findings
