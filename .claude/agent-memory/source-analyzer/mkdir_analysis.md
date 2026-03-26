---
name: mkdir_analysis
description: OpenBSD mkdir v1.31 portability analysis: EASY verdict, setmode/getmode blocker for -m flag, exit code fixes, pledge stub, umask stub
type: project
---

OpenBSD mkdir v1.31 (mkdir.c, 175 lines) — portability verdict: EASY.

**Why:** Category 1 CLI. Single file, no Tier 2/3 issues. One shim gap: `setmode()`/`getmode()` for the `-m mode` flag.

**Key findings:**

1. `setmode()`/`getmode()` — BSD-only, NOT in libnix, no amiport shim. Used only for `-m mode` flag parsing. Fix: replace with `strtol(optarg, NULL, 8)` for octal mode support (covers 90% of real usage). Drop symbolic mode (`u+x`) support.

2. `mkdir()` → `amiport_mkdir()` — mode bits silently ignored (AmigaOS has no Unix perms). Acceptable.

3. `stat()` → `amiport_stat()` — used in mkpath() to distinguish "not exists" from "exists but not dir". Works correctly.

4. `chmod()` → `amiport_chmod()` (no-op stub) — only called for mode > 0777 (sticky/setuid/setgid). Stub returning 0 is correct behavior.

5. `umask()` — not in libnix. Add `#define umask(m) (0)`. Mode defaults to 0777 which is correct.

6. `pledge()` — stub as `#define pledge(p, e) (0)`.

7. `__dead` qualifier — likely defined via GCC headers; add guard if not.

8. Exit codes: change `exit(1)`/`err(1,...)`/`errx(1,...)` to `exit(10)`/`err(10,...)`/`errx(10,...)`. Change `exitval = 1` to `exitval = 10`.

9. `__progname` — keep `extern char *__progname;` declaration, add `amiport_expand_argv()` in main().

**How to apply:** Classify future OpenBSD utilities with `-m mode` flag (install, cp) as having this same setmode/getmode gap.
