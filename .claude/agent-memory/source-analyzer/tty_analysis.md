---
name: tty_analysis
description: OpenBSD tty v1.14 portability analysis: EASY verdict, ttyname() stub breaks core logic, IsInteractive() fix, pledge/unveil stubs, paths.h remove, exit code fixes
type: project
---

## OpenBSD tty v1.14 — Portability Analysis

**Verdict:** EASY
**Category:** CLI tool (Category 1)
**Required shims:** posix-shim only (-lamiport)
**Test strategy:** vamos

### Issues

**Tier 1 (shim):**
- `getopt()` — amiport_getopt() via `<amiport/getopt.h>`
- `ttyname(STDIN_FILENO)` — amiport_ttyname() returns "CONSOLE:" always (see semantic gap)
- `err()` — amiport_err() via `<amiport/err.h>`

**Headers to fix:**
- `<unistd.h>` → `<amiport/unistd.h>` + `<amiport/getopt.h>`
- `<stdlib.h>` → `<amiport/stdlib.h>`
- `<err.h>` → `<amiport/err.h>`
- `<paths.h>` — REMOVE (only used with unveil stub; no Amiga equivalent)

**Stubs:**
- `pledge()` → `#define pledge(p, e) (0)`
- `unveil()` → `#define unveil(p, f) (0)` — also removes _PATH_DEVDB dependency

### Critical semantic gap: ttyname() stub

amiport_ttyname() ALWAYS returns "CONSOLE:", never NULL. But ttyname()'s return value is the entire program logic:
- `puts(t ? t : "not a tty")` — always prints "CONSOLE:", never "not a tty"
- `exit(t ? 0 : 1)` — always exits 0, never reports "not a tty"

**Fix:** Replace ttyname() call with `#ifdef __AMIGA__` block using `IsInteractive(Input())`:
```c
#ifdef __AMIGA__
{
    BPTR in = Input();
    t = (IsInteractive(in)) ? "CONSOLE:" : NULL;
}
#else
t = ttyname(STDIN_FILENO);
#endif
```
Requires `#include <proto/dos.h>`.

### Exit codes
- `exit(1)` (not a tty) → `exit(5)` (RETURN_WARN)
- `exit(2)` (usage) → `exit(10)` (RETURN_ERROR)

### Architecture
None. Pure C89, no 64-bit, no struct returns, no GNU extensions. Trivial stack.

**Why:** ttyname() stub analysis — returning constant "CONSOLE:" silently breaks programs where NULL-vs-non-NULL drives program logic. IsInteractive(Input()) is the correct Amiga idiom.
