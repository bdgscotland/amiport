# Memory Index — source-analyzer agent

- [jq_analysis.md](jq_analysis.md) - jq 1.7.1 portability analysis: blockers, pthread TLS rewrite strategy, fdopen crash, -std=c99 requirement
- [patch_analysis.md](patch_analysis.md) - OpenBSD patch v1.78 portability analysis: MODERATE verdict, mmap Tier 2, fgetln→fgets, sys/queue.h vendor, d_namlen, long long printf, getline→amiport_getline
- [bc_analysis.md](bc_analysis.md) - GNU bc 1.07.1 portability analysis: EASY verdict, bundled getopt, signal shim, write(1,...) in signal handler, YY_BUF_SIZE stack issue, readline/libedit must be disabled
