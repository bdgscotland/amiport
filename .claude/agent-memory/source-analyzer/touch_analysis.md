---
name: touch_analysis
description: OpenBSD touch v1.27 portability analysis: EASY verdict, st_atim/st_mtim timespec fields missing from amiport_stat (needs shim extension), utimensat/futimens shims exist, strptime in libnix, timegm shim, pledge stub, __dead macro, exit code fixes
type: project
---

# OpenBSD touch 1.27 portability analysis

**Verdict: EASY** — single-file, no Tier 3 issues. Key blocker is `st_atim`/`st_mtim` fields (struct timespec) missing from `struct amiport_stat` — needs shim extension via `/extend-shim`.

## Key findings

- `utimensat()` and `futimens()` — BOTH ALREADY SHIMMED in `lib/posix-shim/`. This is the core function; it works.
- `strptime()` — confirmed in libnix libc.a (`_strptime` symbol). No shim needed.
- `timegm()` — shimmed as `amiport_timegm()` in process.c. Macro in unistd.h.
- `st_atim` / `st_mtim` — BLOCKING: `struct amiport_stat` only has `st_atime`/`st_mtime` (ULONG), NOT `.st_atim`/`.st_mtim` (struct timespec). `stime_file()` references `sb.st_atim` and `sb.st_mtim` directly. Needs shim extension to add timespec fields.
- `sys/time.h` — no `struct timeval` needed by touch; only `struct timespec` via `time.h`.
- `DEFFILEMODE` — missing from libnix/NDK headers. Define locally as `0666`.
- `__dead` — OpenBSD-specific macro for `__attribute__((noreturn))`. Strip or define locally.
- `pledge()` — stub as `#define pledge(p,e) (0)`.
- Exit codes: all `err(1,...)`/`errx(1,...)` → `err(10,...)`/`errx(10,...)`, `exit(1)` in usage() → `exit(10)`.
- `err.h` — use `<amiport/err.h>`.

## AmigaOS semantic notes

- AmigaOS only has mtime (no separate atime). utimensat shim correctly applies mtime only.
- `futimens()` shim uses `NameFromFH()` to recover path, then delegates to `utimensat()`. Works for regular files; fails on pipes/console (ENOTSUP). Acceptable for touch.
- The `-a` flag (atime-only) will effectively set mtime on AmigaOS because AmigaOS stores only mtime. Document as known limitation.
