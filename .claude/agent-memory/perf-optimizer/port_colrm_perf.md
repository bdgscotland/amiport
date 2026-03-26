---
name: port_colrm_perf
description: Performance findings for ports/colrm 1.14 — getline+char-walk hotpath, putchar per character, reviewed 2026-03-26
type: project
---

# colrm 1.14 Performance Notes

**Why:** colrm is a stream filter; every byte of stdin goes through the inner loop.

## Hot Path
- `colrm.c:141-235` — getline() per line, then character-by-character walk via `p += len` with putchar()/fwrite() per character.

## Findings
- **MEDIUM** [colrm.c:148-185] — putchar() called once per output character inside a char-walk loop. For the common case (start==0, no range filter, no tabs/backspaces), every character costs a JSR + buffer check. No bulk fwrite opportunity without restructuring — the per-character logic (tabs, backspaces, column counting) makes bulk output hard to extract. The fwrite() at line 220 already handles the normal printable case correctly.
- **LOW** [colrm.c:145] — `for (p = line; *p != '\0'; p += len)` — len is always 1 on AmigaOS (ASCII-only path). The compiler may not eliminate the `p += 1` / `p += len` indirection. Minor — len=1 is a compile-time constant in the #else branch.
- **LOW** — No `amiport_check_break()` in the outer while loop. For large files, the user cannot interrupt. Not a performance issue but a usability one.

## Verdict
Mostly clean. The I/O pattern (getline + char walk) is the right approach for this kind of column filter. No CRITICAL or HIGH issues.

**How to apply:** No mandatory fixes needed before shipping.
