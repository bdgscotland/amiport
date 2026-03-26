# Memory Index — source-analyzer agent

- [jq_analysis.md](jq_analysis.md) - jq 1.7.1 portability analysis: blockers, pthread TLS rewrite strategy, fdopen crash, -std=c99 requirement
- [patch_analysis.md](patch_analysis.md) - OpenBSD patch v1.78 portability analysis: MODERATE verdict, mmap Tier 2, fgetln→fgets, sys/queue.h vendor, d_namlen, long long printf, getline→amiport_getline
- [bc_analysis.md](bc_analysis.md) - GNU bc 1.07.1 portability analysis: EASY verdict, bundled getopt, signal shim, write(1,...) in signal handler, YY_BUF_SIZE stack issue, readline/libedit must be disabled
- [mg_analysis.md](mg_analysis.md) - Mg 3.8-pre portability analysis: HARD verdict, fork/exec in region.c+dired.c are Tier 3 blockers, disable dired/compile/cscope/grep, WITHOUT_CURSES ANSI mode is Amiga-friendly
- [comm_analysis.md](comm_analysis.md) - OpenBSD comm v1.11 portability analysis: EASY verdict, Category 1 CLI, pledge stub, exit code fixes, LINE_MAX guard needed
- [rev_analysis.md](rev_analysis.md) - OpenBSD rev v1.16 portability analysis: EASY verdict, Category 1 CLI, MB_CUR_MAX runtime pitfall, fclose(stdin) guard needed, exit code fixes, __progname weak symbol
- [expand_analysis.md](expand_analysis.md) - OpenBSD expand v1.15 portability analysis: EASY verdict, Category 1 CLI, pledge stub, exit code fixes, __progname weak symbol, no Tier 2/3 issues
- [cat_analysis.md](cat_analysis.md) - OpenBSD cat v1.34 portability analysis: EASY verdict, st_blksize missing from amiport_stat (compile error), fileno(stdout) via amiport macro returns -1 (runtime breakage), pledge stub, long long counter, fclose(stdout) danger
- [printf_analysis.md](printf_analysis.md) - OpenBSD printf v1.28 portability analysis: EASY verdict, Category 1 CLI, pledge stub, warnc shim, __dead macro, exit codes, %a/%A libnix risk
- [sleep_analysis.md](sleep_analysis.md) - OpenBSD sleep v1.29 portability analysis: EASY verdict, SIGALRM dead code, timespecclear/timespecisset macros missing, nanosleep Tier 1
- [paste_analysis.md](paste_analysis.md) - OpenBSD paste v1.27 portability analysis: EASY verdict, Category 1 CLI, sys/queue.h vendor SIMPLEQ macros, getline shim, __dead strip, pledge stub, exit code fixes
- [rs_analysis.md](rs_analysis.md) - OpenBSD rs v1.30 portability analysis: MODERATE verdict, missing mbsavis() from vis.c is linker blocker, getline shim, ssize_t/%zd C99 fix, MB_CUR_MAX locale risk, exit code fixes
- [printenv_analysis.md](printenv_analysis.md) - OpenBSD printenv v1.8 portability analysis: EASY verdict, Category 1 CLI, environ stub needed (semantic gap: prints nothing with no args), pledge stub, exit code fix
- [mkdir_analysis.md](mkdir_analysis.md) - OpenBSD mkdir v1.31 portability analysis: EASY verdict, setmode/getmode shim gap for -m flag (use strtol octal), exit code fixes, pledge/umask stubs
- [test_analysis.md](test_analysis.md) - OpenBSD test(1) v1.23 portability analysis: EASY verdict, Category 1 CLI, stat/access/lstat shims, exit code 2 semantic split, timespeccmp macro gap, st_dev/st_ino -ef operator
