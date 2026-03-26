---
name: look_analysis
description: OpenBSD look v1.27 portability analysis: EASY verdict, mmap Tier 2 (MAP_PRIVATE+PROT_READ only), fstat Tier 1, errc shim, pledge stub, SIZE_MAX issue, _PATH_WORDS must be replaced, exit code fixes
type: project
---

OpenBSD look 1.27 portability analysis — EASY verdict.

**Why:** Single C file, no threads, no sockets, no fork/exec. The only non-trivial issue
is mmap(MAP_PRIVATE, PROT_READ) which is covered by Tier 2 amiport_emu_mmap(). All other
POSIX deps are Tier 1 shims. One compile error: SIZE_MAX from <stdint.h> is not available
in libnix without <limits.h>.

**How to apply:** Code-transformer can handle all transforms mechanically. The _PATH_WORDS
default path must become "WORK:dict/words" or similar Amiga path. The mmap emulation
reads the entire file into memory — acceptable for a word-list file (<1MB typical).
Exit codes: err(2,...) must become err(10,...); usage() exits 2, should exit 10.
