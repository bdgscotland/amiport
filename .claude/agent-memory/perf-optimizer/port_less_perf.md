---
name: port_less_perf
description: Performance findings for ports/less 692 — hotpath summary and critical recommendations
type: project
---

# Performance findings: ports/less 692

Reviewed 2026-03-23.

## Summary

less 692 is a 37-file Console UI (Category 3) port. The architecture is well-designed for 68k — ch.c uses an 8KB block buffer (LBUFSIZE=8192) for file I/O, avoiding fgetc-per-character reads from disk. The output path is also buffered via obuf[OUTBUF_SIZE=1024].

## Hot paths identified

1. **put_line() → putchr()** — output.c:52. Every character of every displayed line calls putchr() individually. putchr() does a terminal init check and buffer-full check on EVERY call. On 80-column display, that's 80+ calls per line, each with 2 branch checks.
2. **is_ansi_end() / is_ansi_middle()** — line.c:636,646. Called on every non-ASCII character during line rendering. Each call invokes strchr() over a ~20-character string. In normal text rendering without ANSI, this is dead overhead for every character.
3. **ansi_start() malloc** — line.c:672. Calls ecalloc(1, sizeof(struct ansi_state)) for EVERY ESC character seen. On a file with ANSI color codes, this is a malloc+free per escape sequence. On AmigaOS with -noixemul, every AllocMem/FreeMem is ~50 cycles minimum.
4. **store_char() in line.c** — the per-character pipeline: is_hilited_attr() RB-tree lookup, pwidth() computation, add_linebuf() — all executed for every character. This is unavoidably complex, but the strchr() hot spots inside it are actionable.
5. **putstr() in output.c:522** — calls putchr() in a character loop; every terminal sequence (tputs output for bold, underline, clear-eol) goes through this.

## Critical/HIGH findings

1. **[ALLOC] line.c:678** — ansi_start() does ecalloc on every ESC byte. A static ansi_state pool of 4 entries would eliminate malloc/free on the hot path. ANSI sequences don't nest deeply; 3 is the NUM_LAST_ANSIS maximum.

2. **[STRING] line.c:640,652** — is_ansi_end() and is_ansi_middle() call strchr() on every character that passes the IS_CSI_START gate. Default end_ansi_chars = "m" (1 char), mid_ansi_chars = "0123456789:;[?!\"'#%()*+ " (~22 chars). Since these strings are compile-time constant in the common case, a 256-byte lookup table (set during init_line()) would replace strchr() with a single array index — 1 cycle vs ~20.

3. **[OUTBUF] defines.h:65** — OUTBUF_SIZE=1024. This is already reasonably sized, but flush() is called every 1023 bytes, which means at minimum 1 write() syscall per screen line on a wide terminal. Increasing to 4096 would halve write() calls on typical terminal widths (AmigaOS write() through console.device is not cheap).

## MEDIUM findings

4. **[STACK] lesskey_parse.c:669,727** — char line[1024] twice in parse_lesskey() and parse_file(). These are startup-only, not hot path. With __stack=65536 there's headroom, but worth noting.

5. **[STACK] screen.c:3501** — char buf[2048] in a non-MSDOS path inside WIN32textout(). This is dead code on AmigaOS (MSDOS_COMPILER=0 means this branch is never reached), but if the preprocessor path is ever entered it would be dangerous.

6. **[MODULO] line.c:1215** — curr_last_ansi = (curr_last_ansi + 1) % NUM_LAST_ANSIS, where NUM_LAST_ANSIS=3. Non-power-of-2 modulo on 68000 = 120-158 cycle divide. With NUM_LAST_ANSIS=3 it could be replaced with: if (++curr_last_ansi >= NUM_LAST_ANSIS) curr_last_ansi = 0;

7. **[MODULO] line.c:1023** — tab_spaces() uses % tabdefault in default tab calculation. tabdefault=8 (power of 2), so this is already amenable to & 7, but only if tabdefault stays 8. Could guard: if tabdefault is power of 2, use bitwise AND.

## LOW findings

8. **[CALL] output.c:522** — putstr() is a character loop over putchr(). For short terminal sequences (bold, underline etc.), write() directly on the obuf append path would save the per-char branch overhead. Minor on cold paths.

9. **[LINEBUF_SIZE] defines.h:64** — LINEBUF_SIZE=1024 seems adequate for Amiga 80-column terminals. Not a problem.

## What NOT to change

- ch.c block buffering (LBUFSIZE=8192) — already correct
- The RB-tree hilite structure in search.c — amortized O(1), correct for this use
- The dual-buffer (linebuf.buf + linebuf.attr) parallel arrays — this is a necessary design for per-char attribute tracking
