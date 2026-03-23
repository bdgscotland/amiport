---
name: port_patch_perf
description: Performance findings for ports/patch v1.78 — hotpath summary and recommendations reviewed 2026-03-23
type: project
---

# Performance Findings: ports/patch v1.78

**Reviewed:** 2026-03-23

## Hot Paths

1. **pch.c `another_hunk()` / `intuit_diff_type()`** — primary parse loop. Calls `pgetline()` → `getline()` per patch line. These are buffered stdio reads so the per-call overhead is already low. The dominant cost here is the `savestr()` allocation (one `strdup()` per hunk line).

2. **pch.c `pgetline()` — indentation skip** — uses `strlcpy(buf, s, bufsz)` for in-buffer shift when `p_indent > 0`. The strlcpy scans the entire remaining line for NUL. Low frequency (only when patches are indented) — LOW impact.

3. **pch.c `remove_special_line()`** — uses `fgetc(pfp)` in a one-shot look-ahead. Called once per patch line ending with `\` marker. Single fgetc followed by a do-while fgetc loop to consume rest of the `\` line. Very low frequency — acceptable.

4. **ed.c `write_lines()`** — character-at-a-time `putc(*p, ofp)` loop for SRC_INP lines. Iterates character-by-character over the line content, writing via putc. For large ed-script patches, this could be many thousands of putc calls.

5. **patch.c `dump_line()`** — same pattern as ed.c write_lines: `for (; *s != '\n'; s++) putc(*s, ofp)`. Called once per unchanged line copied to output. This is THE primary output path for normal unified/context diffs.

6. **util.c `copy_file()`** — read/write loop using `buf`/`bufsz`. `bufsz` starts at `INITLINELEN = 8192`. This is adequate for AmigaOS block boundaries (disk DMA is 512-byte blocks; 8192 is 16 blocks). Not a bottleneck.

7. **inp.c `plan_b` second pass** — `fgets(p, maxlen+1, ifp)` + `amiport_write()` loop. Reads file a second time into tifd temp file. Triggered only when file too large for plan_a (rare on 68k with limited RAM).

8. **backupfile.c `max_backup_version()`** — `readdir()` loop with `strlen(dp->d_name)` per entry. readdir() on AmigaOS already calls `ExNext()` per entry (slow). The `strlen()` inside the loop is minor by comparison.

## Key Findings

### MEDIUM: dump_line putc character-at-a-time (patch.c:1138, ed.c:251)
Both `dump_line()` and `write_lines()` use character-at-a-time `putc()` loops to write non-NUL-terminated strings up to `\n`. On 7MHz 68000 with chip RAM this is ~3x slower than using `fwrite()` with a computed length. The fix is to use `fwrite(s, 1, len, ofp)` where `len = strchr(s, '\n') - s + 1` (or a pointer walk). NOT `fputs` because the strings are not NUL-terminated.

### LOW: pgetline indent % 8 modulo (pch.c:1196)
Bug: `indent += 8 - (indent % 7)` should be `% 8` (matching intuit_diff_type at line 302). The `% 7` is incorrect. On 68k the `%` for integers compiles to `DIVS` (68000: 158 cycles). This is called only on indented patches and the loop is short, so real impact is negligible — but it IS a correctness bug.

### LOW: version_number multiply * 10 (backupfile.c:155)
`version = version * 10 + *p - '0'` — called only when parsing backup version numbers from filenames, not a hot path. On 68020 multiply is cheap (20-28 cycles).

### LOW: reallocate_lines * 3 / 2 multiply (inp.c:135)
`new_size = *lines_allocatedp * 3 / 2` — called only when input file has more lines than pre-allocated. Rare.

## Summary

- **Primary bottleneck:** I/O bound (disk reads via fgets/getline, temp file writes). CPU overhead is minimal for typical patch sizes on 68020.
- **Only actionable optimization:** Replace `putc()` loops in `dump_line()` and `write_lines()` with `fwrite()`. This affects every unchanged line written to output.
- **Correctness bug found:** `pgetline()` has `indent % 7` where `% 8` is clearly intended (intuit_diff_type uses `% 8`).
- **No fgetc hot paths found** in the primary parse loop — `pgetline()` uses `getline()` (buffered), not fgetc.
- **Buffer size for copy_file is adequate** — 8192 bytes = 16 x 512-byte Amiga disk blocks.
- **Stack is safe** — `__stack = 65536`, large arrays in inp.c plan_b are `static`.
