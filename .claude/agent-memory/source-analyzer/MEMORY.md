# Memory Index — source-analyzer agent

- [jq_analysis.md](jq_analysis.md) - jq 1.7.1 portability analysis: blockers, pthread TLS rewrite strategy, fdopen crash, -std=c99 requirement
- [patch_analysis.md](patch_analysis.md) - OpenBSD patch v1.78 portability analysis: MODERATE verdict, mmap Tier 2, fgetln→fgets, sys/queue.h vendor, d_namlen, long long printf, getline→amiport_getline
- [bc_analysis.md](bc_analysis.md) - GNU bc 1.07.1 portability analysis: EASY verdict, bundled getopt, signal shim, write(1,...) in signal handler, YY_BUF_SIZE stack issue, readline/libedit must be disabled
- [mg_analysis.md](mg_analysis.md) - Mg 3.8-pre portability analysis: HARD verdict, fork/exec in region.c+dired.c are Tier 3 blockers, disable dired/compile/cscope/grep, WITHOUT_CURSES ANSI mode is Amiga-friendly
- [comm_analysis.md](comm_analysis.md) - OpenBSD comm v1.11 portability analysis: EASY verdict, Category 1 CLI, pledge stub, exit code fixes, LINE_MAX guard needed
- [rev_analysis.md](rev_analysis.md) - OpenBSD rev v1.16 portability analysis: EASY verdict, Category 1 CLI, MB_CUR_MAX runtime pitfall, fclose(stdin) guard needed, exit code fixes, __progname weak symbol
- [expand_analysis.md](expand_analysis.md) - OpenBSD expand v1.15 portability analysis: EASY verdict, Category 1 CLI, pledge stub, exit code fixes, __progname weak symbol, no Tier 2/3 issues
