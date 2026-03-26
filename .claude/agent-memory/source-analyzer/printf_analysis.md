---
name: printf_analysis
description: OpenBSD printf 1.28 portability analysis: EASY verdict, Category 1 CLI, pledge stub, warnc shim, __dead macro, exit code fixes, %a/%A libnix risk
type: project
---

OpenBSD printf v1.28 (1 source file, 497 lines).

Verdict: EASY
Category: 1 (CLI tool)

Key issues:
- pledge() -> macro stub
- __dead -> #define __dead __attribute__((__noreturn__))
- warnc() -> amiport_warnc() via amiport/err.h
- warnx() -> amiport_warnx() via amiport/err.h
- err(1,...) -> err(10,...) exit code fix
- exit(1) in usage() -> exit(10)
- %a/%A hex float format: libnix printf does NOT support %a/%A -- pass-through to printf will silently output nothing or garbage; must link -lm and warn users
- \e escape: uses #ifdef __GNUC__ guard -- compiles cleanly with bebbo-gcc
- No fork/exec, no mmap, no threads, no sockets, no curses
- No getopt -- no option parsing needed (-- handled manually)
- float output requires -lm link flag (already standard caveat for libnix)

Why: Standard OpenBSD CLI tool; all issues are mechanical substitutions via existing shims.
How to apply: Use as a straightforward Category 1 port. No Tier 2 or Tier 3 blockers.
