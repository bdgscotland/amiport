---
name: ln_analysis
description: OpenBSD ln v1.25 portability analysis: MODERATE verdict, linkat() Tier 1 needs-shim gap, symlink/lstat shimmed, st_dev/st_ino same-file check present, __dead macro needed, exit code fixes, dirname() corruptible (but not called), warnc shimmed
type: project
---

OpenBSD ln(1) v1.25 portability analysis.

**Why:** Needed for AmigaOS 3.x port. ln creates hard and soft links.

**How to apply:** Use this as the basis for code-transformer work. Key blocker is linkat() which needs a new shim before transformation can proceed.

## Verdict: MODERATE

### Blockers
- `linkat()` — POSIX.1-2008 AT_ function, NOT in shim. Needs new Tier 1 shim (or inline replace with MakeLink() hard link call). This is the primary linker blocker.
- `__dead` macro — OpenBSD-specific, strip or define as `__attribute__((noreturn))`
- Exit codes 1 → 10 needed throughout

### Non-blockers (shimmed)
- symlink() — shimmed (amiport_symlink)
- stat()/lstat() — shimmed
- unlink() — shimmed
- basename() — in libnix
- dirname() — NOT called (only basename used)
- warnc/warn/warnx — shimmed in amiport/err.h
- pledge() — stub
- getopt() — use amiport/getopt.h

### Logic bug flags
- st_dev/st_ino comparison at line 167 — same-file detection, may be unreliable
- symlink creation on AmigaOS: soft links are OS 2.0+ only, may fail on some filesystems
