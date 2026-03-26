---
name: test_analysis
description: OpenBSD test(1) v1.23 portability analysis: EASY verdict, Category 1 CLI, stat/access/lstat shims, exit code 2 semantic split, timespeccmp macro gap, st_dev/st_ino ef operator
type: project
---

## OpenBSD test(1) v1.23 — Portability Analysis Summary

**Verdict:** EASY
**Category:** CLI tool (Category 1)
**Source:** ports/test/original/test.c (571 lines, single file)
**Required shims:** posix-shim only (-lamiport)
**Test strategy:** vamos

## Key Issues

### Tier 1 shims needed
- stat() -> amiport_stat()
- lstat() -> amiport_lstat() (alias to stat; no symlinks on classic FFS)
- access() -> amiport_access()
- isatty() -> amiport_isatty()
- geteuid() -> amiport_geteuid() (always 0)
- getegid() -> amiport_getegid() (always 0)
- err()/errx() -> <amiport/err.h>
- pledge() -> #define pledge(p,e) (0)
- strtonum() -> available in <amiport/err.h>
- strlcpy() -> available in libnix libc.a (no shim needed)

### Exit code trap
test(1) has TWO distinct exit code semantics:
- exit(0)/exit(1) = POSIX true/false result -- keep as-is (0=true, 1=false)
- errx(2, ...) = usage error -- change to errx(10, ...) ONLY

Do NOT change the return(1) paths for false test results to return(10).

### timespeccmp macro
Used for -nt/-ot operators at lines 549, 558. OpenBSD macro from <sys/time.h>.
Must verify <amiport/sys/stat.h> defines it; if not, add guard:
#ifndef timespeccmp
#define timespeccmp(tvp, uvp, cmp) \
    (((tvp)->tv_sec == (uvp)->tv_sec) ? \
     ((tvp)->tv_nsec cmp (uvp)->tv_nsec) : \
     ((tvp)->tv_sec cmp (uvp)->tv_sec))
#endif

### st_dev/st_ino for -ef operator
equalf() uses st_dev + st_ino for same-file detection (crash-patterns #4).
This is the entire purpose of -ef so it needs explicit test coverage.
amiport_stat() populates st_ino from fib_DiskKey -- should be unique per file
within a volume. Cross-volume behavior needs verification.

### Semantic gaps (always false/true on AmigaOS)
- -L/-h: always false (no symlinks on classic FFS)
- -c/-b/-p/-S: always false (no device/fifo/socket types)
- -u/-g/-k: always false (no SUID/SGID/sticky bits)
- -x: tests existence only, not execute permission
- -O/-G: always true for any existing file (uid/gid always 0)

### __dead macro
Add guard: #ifndef __dead\n#define __dead __attribute__((noreturn))\n#endif

### __progname
Remove extern declaration from main(); use amiport_expand_argv() to populate it.

**Why:** See above details.
**How to apply:** Use as reference when transform-source dispatches for this port.
