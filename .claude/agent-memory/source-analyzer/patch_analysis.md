---
name: patch_analysis
description: OpenBSD patch v1.78 portability analysis — verdict MODERATE, primary blockers are mmap/munmap (Tier 2), fgetln replacement, sys/queue.h, paths.h, stdbool, getopt_long optreset, long long printf cast, SIGHUP stub, off_t 32-bit constraint
type: project
---

OpenBSD patch v1.78 portability analysis completed 2026-03-22.

**Verdict:** MODERATE — all issues are solvable with shims/stubs, no Tier 3 redesign required.

**Key findings:**
- mmap/munmap in inp.c plan_a(): Tier 2 (amiport-emu/mmap.h) — read-only MAP_PRIVATE, full-file load
- madvise() in inp.c: simple no-op macro (#define madvise(p,l,a) (void)0)
- fgetln() in inp.c plan_b(): replace with fgets() into static buffer (bsd-isms.md pattern)
- sys/queue.h in ed.c: must vendor queue.h (LIST_* macros) into ported/ — not in toolchain
- <paths.h> / _PATH_TMP, _PATH_TTY, _PATH_DEVNULL: must define in ported config header
- <stdbool.h> in common.h: replace with manual #define true/false/bool
- getopt_long: libnix has it — no issue
- optreset: libnix may not have optreset global — needs guard (check at build)
- pledge()/unveil(): stub as macros per bsd-isms.md
- SIGHUP: libnix ignores — signal(SIGHUP, ...) call compiles but is no-op; safe to leave
- strtonum(): amiport/err.h — available
- warnc(): amiport/err.h — available
- fseeko/ftello: libnix has both
- fileno(): libnix has it
- getline(): NOT in libnix — use amiport_getline() from amiport/stdio.h
- st_dev/st_ino comparison in backup_file(): STUB VALUE WARNING — used to detect same-file
- off_t: typedef'd as long (32-bit) in amiport — files >2GB fail, acceptable for Amiga
- long long cast in pch.c skip_to(): %lld printf format — NOT supported in libnix printf. Fix: cast to long and use %ld
- d_namlen in backupfile.c: BSD dirent extension — not in amiport struct dirent. Replace with strlen(dp->d_name)
- __dead void: OpenBSD extension for __attribute__((noreturn)) — replace with __attribute__((noreturn)) or leave as void with __attribute__ annotation
- EXDEV errno: may not be defined — needs guard
- <stdint.h> in inp.c, pch.c: C99 — replace with <exec/types.h> or remove if only SIZE_MAX needed
- SIZE_MAX in inp.c: may need explicit define if not in limits.h
- Exit codes: my_exit(2) — should be my_exit(RETURN_ERROR=10) per Amiga convention
- <libgen.h> basename/dirname: libnix has these — no issue

**Required shims:** posix-shim + posix-emu (mmap only)
**Test strategy:** vamos (Category 1)
**Stack:** 65536 recommended (recursive directory operations via makedirs)
