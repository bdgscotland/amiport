---
name: expr_analysis
description: OpenBSD expr v1.28 portability analysis: EASY verdict, int64_t via gnu99, regex Tier 2, pledge stub, __dead macro, exit code fixes, no fork/mmap
type: project
---

OpenBSD expr v1.28 portability analysis.

**Verdict: EASY** — Single file (519 LOC), Category 1 CLI.

## Key Findings

- `<stdint.h>` not in libnix — enable `-std=gnu99` to get GCC's internal stdint.h; avoids rewriting all `int64_t` usages
- `<regex.h>` — Tier 2 via `<amiport-emu/regex.h>`; expr uses BRE (flag=0) anchored match, fits within emu limits
- `pledge()` — stub as `#define pledge(p,e) (0)`
- `__dead` macro — add `#ifndef __dead` / `#define __dead __attribute__((noreturn))` / `#endif`
- All `err(3,...)`, `errx(2,...)`, `errx(3,...)` — change exit codes to 10 (RETURN_ERROR)
- `return is_zero_or_null(vp)` — LEAVE as 0/1 (POSIX semantics, 1 < 5=WARN threshold, correct)
- `asprintf` — libnix native; no shim needed (gnu99 covers %lld in format)
- `strtonum` — via `<amiport/err.h>` macro (uses strtoll internally, available in libnix)
- `strcoll` — libnix provides (confirmed in libnix-reference string.h list)
- No fork, mmap, file I/O beyond argv/stdout, no threads, no sockets

## Headers

| Header | Action |
|--------|--------|
| `<stdint.h>` | Remove explicit include; GCC provides under -std=gnu99 |
| `<stdlib.h>` | `<amiport/stdlib.h>` |
| `<unistd.h>` | `<amiport/unistd.h>` |
| `<regex.h>` | `<amiport-emu/regex.h>` |
| `<err.h>` | `<amiport/err.h>` |

## Build

- `-std=gnu99` (mandatory for stdint.h and %lld)
- `-I../../lib/posix-shim/include -I../../lib/posix-emu/include`
- `-lamiport-emu -lamiport`
- `long __stack = 16384;`

**Why:** The fact that asprintf is natively available in libnix was verified — prior analyses that flagged it as needing amiport wrapper were overly cautious for single-file gnu99 ports.
