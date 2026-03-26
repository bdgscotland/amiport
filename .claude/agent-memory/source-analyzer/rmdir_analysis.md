---
name: rmdir_analysis
description: OpenBSD rmdir v1.15 portability analysis: EASY verdict, Category 1 CLI, pledge stub, __dead BSD macro, exit code fixes, rm_path -p flag path separator behaviour
type: project
---

OpenBSD rmdir v1.15 (ports/rmdir/original/rmdir.c) — 111 lines, single file.

**Verdict: EASY**

**Why:** Zero Tier 2/3 blockers. Only 5 POSIX functions beyond stdio: rmdir() (shim exists), getopt() (shim exists), warn()/err() (amiport/err.h), pledge() (macro stub). No stat fields, no 64-bit types, no threading, no mmap.

**Tier 1 issues (4):**
- rmdir() -> amiport_rmdir() (DeleteFile, already implemented)
- getopt() -> amiport_getopt() (already implemented)
- warn()/err() -> amiport/err.h (already implemented)
- pledge() -> #define pledge(p, e) (0) macro stub

**Non-shim issues:**
- __dead macro from <sys/cdefs.h>: not available on AmigaOS. Fix: #ifndef __dead / #define __dead __attribute__((noreturn)) / #endif — recurring BSD-ism
- exit(1) in usage() -> exit(10) (RETURN_ERROR for AmigaOS shell scripts)
- errors = 1 -> errors = 10 in rm_path error path
- __progname extern declaration is correct; do NOT define it locally

**Path separator note for -p flag:**
rm_path() strips path components by scanning for '/' only. Works correctly with AmigaOS paths: volume-rooted paths (WORK:dir) have no '/' so loop body never fires after final dir already deleted — correct behaviour. No code change needed but deserves a test case.

**How to apply:** Standard Category 1 transform: replace headers, add pledge macro, fix __dead, fix exit codes, add __stack + verstag + amiport_expand_argv(). ~30 min effort.
