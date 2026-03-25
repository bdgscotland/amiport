---
name: comm_analysis
description: OpenBSD comm v1.11 portability analysis — EASY verdict, single-file CLI tool
type: project
---

comm v1.11 portability analysis (2026-03-24).

**Verdict: EASY**
**Category: 1 — CLI tool**
**Port strategy: vamos testing**

Key findings:
- Single 180-line file, no C99 features (pure C89 throughout)
- No Tier 3 blockers
- `pledge()` — stub as `#define pledge(p,e) (0)` (Tier 1, trivial)
- `err(1, ...)` / `err(1, ...)` in usage() — change exit codes to 10 (RETURN_ERROR); use `<amiport/err.h>`
- `exit(1)` in usage() — change to `exit(10)`
- `setlocale(LC_ALL, "")` — Tier 1 (libnix loads system locale; call is safe but note MB_CUR_MAX warning)
- `strcoll()` — available in libnix (string.h), no shim needed
- `LINE_MAX` from `<limits.h>` — may not be defined in bebbo-gcc libnix; need fallback `#ifndef LINE_MAX #define LINE_MAX 2048 #endif`
- `fclose(stdout)` on line 149 — harmless on AmigaOS for stdout (not stdin), but flag for review
- `MAXLINELEN = LINE_MAX + 1` as local array sizes: line1/line2 on stack. If LINE_MAX=2048, arrays are 2049 bytes each = 4098 bytes of stack locals. With typical call chain stack usage this is safe with __stack=32768.

**Why:** Straightforward text-processing utility with no process model, no sockets, no curses.
**How to apply:** Use as template for other simple BSD text tools.
