---
name: expand_analysis
description: OpenBSD expand v1.15 portability analysis: EASY verdict, Category 1 CLI, pledge stub, exit code fixes, __progname weak symbol, no Tier 2/3 issues
type: project
---

OpenBSD expand v1.15 (single file, 171 lines). Converts tabs to spaces.

**Why:** Filed for pipeline reference. Verdict recorded so future sessions don't re-analyze.

**How to apply:** Use as template for other single-file OpenBSD text-processing utilities.

## Verdict: EASY

- Category 1 CLI tool
- 0 Tier 2, 0 Tier 3 issues
- Required shims: posix-shim only (-lamiport)
- Test strategy: vamos

## Issues (all Tier 1)

1. `pledge()` at line 54 -- stub as `#define pledge(p, e) (0)`, remove the err() call on failure (it can never fire)
2. `getopt()` at line 64 -- `<unistd.h>` -> `<amiport/unistd.h>`, only short options (-t), no getopt_long needed
3. `err()`/`errx()` at lines 55, 80, 149, 154 -- `<err.h>` -> `<amiport/err.h>`, all exit codes must change 1->10
4. `exit(1)` at line 169 in usage() -- change to exit(10)
5. `__progname` extern at line 167 in usage() -- add `char *__progname = "expand";` to fix weak symbol linker bug (known-pitfalls)

## No Issues

- freopen() -- provided by libnix, no shim needed
- Pure C89, no C99 features, no GNU extensions
- No stat() fields, no getuid/getpid/umask, no MB_CUR_MAX
- No offsetof() custom allocators (crash-patterns #15 clean)
- No struct-by-value returns > 8 bytes (crash-patterns #16 clean)
- tabstops[100] = 400 bytes stack -- safe on 68000

## Transform Checklist

1. Replace `#include <unistd.h>` with `#include <amiport/unistd.h>`
2. Replace `#include <err.h>` with `#include <amiport/err.h>`
3. Add `#define pledge(p, e) (0)`
4. Change all err(1,...)/errx(1,...)/exit(1) to exit code 10
5. Add `char *__progname = "expand";`
6. Add `long __stack = 32768;`
7. Add `static const char *verstag = "$VER: expand 1.15 (DD.MM.YYYY)";`
