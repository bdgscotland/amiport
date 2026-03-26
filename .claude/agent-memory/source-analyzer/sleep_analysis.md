---
name: sleep_analysis
description: OpenBSD sleep v1.29 portability analysis: EASY verdict, SIGALRM handler design, _exit in signal handler, timespecclear/timespecisset macros missing
type: project
---

## Sleep 1.29 Portability Analysis

**Verdict:** EASY — Category 1 CLI tool

**Source:** OpenBSD sleep.c v1.29 (125 lines, single file)

### Key findings

1. **nanosleep()** — Tier 1, `amiport_nanosleep()` already exists in `amiport/signal.h`. Direct replacement.

2. **signal(SIGALRM, alarmh)** — SIGALRM is not defined in `amiport/signal.h`. The shim defines SIGHUP(1), SIGINT(2), SIGQUIT(3), SIGTERM(15), SIGPIPE(13) but NOT SIGALRM(14). Need to add `#define SIGALRM 14` to the shim header or stub the signal() call. Since AmigaOS has no async signal delivery, the SIGALRM handler (`alarmh`) is dead code — `nanosleep()` cannot be interrupted by SIGALRM on AmigaOS. Signal registration can be stubbed as a no-op or mapped to the cooperative alarm emulation via `amiport_emu_alarm()`.

3. **timespecclear / timespecisset** — BSD convenience macros from `<sys/time.h>`. Not provided by libnix or amiport shims. Must be defined locally or included via a guard. Definitions: `#define timespecclear(t) ((t)->tv_sec = (t)->tv_nsec = 0)` and `#define timespecisset(t) ((t)->tv_sec || (t)->tv_nsec)`.

4. **pledge()** — stub as `#define pledge(p, e) (0)`. Standard pattern.

5. **getprogname()** — provided by `amiport_expand_argv()` via `__progname`. Standard pattern.

6. **_exit(0) in signal handler** — Correct on Unix. On AmigaOS the alarmh() function is dead code (SIGALRM never fires). The `_exit()` is fine to keep — it won't be called.

7. **exit(1) in usage()** — Must change to `exit(10)` for AmigaDOS shell compatibility.

8. **sys/time.h** — Required for `struct timespec` (via `timespecclear`/`timespecisset`). Must replace with `<amiport/sys/time.h>` or add the two macros locally.

### What SIGALRM means here

The program registers `alarmh` for SIGALRM, but SIGALRM can ONLY fire if something calls `alarm()` — which this program never does. The `alarmh` handler is dead code in any context. On AmigaOS: stub `signal(SIGALRM, alarmh)` as a no-op (via the shim returning SIG_DFL) and leave `alarmh` defined but unreachable. This is behaviorally equivalent.

### Shims needed

- `#include <amiport/signal.h>` — provides `nanosleep` macro, `signal` macro
- `#define SIGALRM 14` — missing from shim header (or add locally)
- `timespecclear` / `timespecisset` — define locally as macros
- `#define pledge(p, e) (0)` stub

### Why: EASY verdict

Only Tier 1 issues. Core functionality (delay for N seconds/fractions) maps directly to `amiport_nanosleep()` -> `Delay()`. The SIGALRM handler is dead code. No fork/exec, no threads, no network, no complex state.
